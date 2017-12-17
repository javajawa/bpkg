// vim: nospell
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "common/null-stream.h"

#include "checksums.h"
#include "error.h"
#include "files.h"
#include "pipe-fork-exec.h"
#include "rules.h"
#include "bpkg-ar.h"
#include "process-control.h"
#include "process-data.h"
#include "write-control.h"

void process_output( char *, struct package_data* );

struct fd fds[ PIPES_MAX ];

// Information about the resulting package which is used
// to populate the package index.
struct package_data stats = {
	0,  // Size of files in package
	0,  // Size of the packaged archive
	"", // First checksum
	""  // Second checksum
};

int main( int argc, char ** argv )
{
	char * arstream[]  = {  "ar-stream", NULL };
	char * tarstream[] = { "tar-stream", NULL, NULL };
	char * const xz[]  = { "xz", NULL };
	char * debdir;

	// Parameter check
	if ( argc != 3 )
	{
		errfs( 1, "Usage: %s [directory] [target.deb]", argv[0] );
	}

	// Initialise the list of file descriptors to all be -1,
	// indicating that that file has not been opened
	for ( size_t i = 0; i < PIPES_MAX; ++i )
	{
		fd(i) = -1;
	}

	// Open a DIR file descriptor to the source directory,
	// so that all future file operations can use f*at() and avoid
	// handling string concatenation.
	fd(SOURCE_DIR) = open( argv[1], O_DIRECTORY | O_RDONLY );

	if ( fd(SOURCE_DIR) == -1 )
	{
		errf( 1, "Can not open sourcedir %s", argv[1] );
	}

	// Check that the following files exists, and are readable:
	//
	//  ./debian/control
	//  ./manifest
	if ( faccessat( fd(SOURCE_DIR), "debian", X_OK | R_OK, AT_EACCESS ) == -1 )
	{
		err( 1, "Unable to read or execute debian dir" );
	}

	if ( faccessat( fd(SOURCE_DIR), "debian/control", R_OK, AT_EACCESS ) == -1 )
	{
		err( 1, "Unable to read debian/control" );
	}

	if ( faccessat( fd(SOURCE_DIR), "manifest", R_OK, AT_EACCESS ) == -1 )
	{
		err( 1, "Unable to read manifest" );
	}

	// Load of of the ownership and permission rules from the
	// debian/owners into memory
	load_rules();

	// Put the debian path in a string.
	debdir = calloc( strlen( argv[1] ) + 10, sizeof(char) );
	if ( debdir == NULL )
	{
		err( 1, "Unable to allocate memory" );
	}
	sprintf( debdir, "%s/debian", argv[1] );

	// Create the output (.deb) file
	create_file( argv[2], pipe(DEB_OUTPUT) );

	// Create a named pipe for sending files to ar-stream
	create_fifo( pipe(AR_INPUT_R) );

	// Spawn the ar(1)-like archiver
	pipe_fork_exec( arstream, pipe(AR_INPUT_R), pipe(DEB_OUTPUT) );

	// Write out the Manifest file
	ar_header( "debian-binary" );
		write( fd(AR_INPUT_W), "2.0\n", 4 );
	ar_footer();

	// Write out the control section
	ar_header( "control.tar.xz" );

		// Create pipes $0 | tarstream | xzip
		create_pipes( pipe(TARSTREAM_INPUT_R) );
		create_pipes( pipe(XZIP_INPUT_R) );

		// Fork to tar-stream
		tarstream[1] = debdir;
		pipe_fork_exec( tarstream, pipe(TARSTREAM_INPUT_R), pipe(XZIP_INPUT_W) );

		// Fork to xzip
		pipe_fork_exec( xz, pipe(XZIP_INPUT_R), pipe(AR_INPUT_W) );

		// Write out the data
		stats.installed_size += process_control();

	// End control section
	ar_footer();

	// Write out the data section
	ar_header( "data.tar.xz" );

		// Create pipes $0 | tarstream | xzip
		create_pipes( pipe(TARSTREAM_INPUT_R) );
		create_pipes( pipe(XZIP_INPUT_R) );

		// Fork to tar-stream
		tarstream[1] = argv[1];
		pipe_fork_exec( tarstream, pipe(TARSTREAM_INPUT_R), pipe(XZIP_INPUT_W) );

		// Fork to xzip
		pipe_fork_exec( xz, pipe(XZIP_INPUT_R), pipe(AR_INPUT_W) );

		// Write out the data
		stats.installed_size += process_data();

	// End data sections
	ar_footer();

	// Send an empty file name to tell ar_stream we are done.
	ar_header( "" );
	ar_footer();

	// Wait for all sub processes to close
	while ( wait( NULL ) != -1 );

	// Get the package size and checksusm
	process_output( argv[2], &stats );

	// Write out a modified version of the debian/control file
	// This contains the information that goes in the Packages/index
	// file of the repository, and adds size and checksum information
	// which has been gathered in to the `stats' struct.
	write_control( argv[2], stats );

	close_descriptors();

	// Free the allocated memory for getting the debian dir
	free( debdir );

	// Tidy up and exit cleanly
	clean_exit( 0 );
}
