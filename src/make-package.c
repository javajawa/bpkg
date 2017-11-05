#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sendfile.h>

#include "null-stream.h"

enum pipes
{
	SOURCE_DIR = 0,
	CURRENT_FILE,
	TARSTREAM_INPUT_R,
	TARSTREAM_INPUT_W,
	XZIP_INPUT_R,
	XZIP_INPUT_W,
	AR_INPUT_W,
	AR_INPUT_R,
	DEB_OUTPUT,
	CONTROL_R,
	CONTROL_W,
	PIPES_MAX
};

struct pipe_id {
	enum pipes id;
};

struct fd {
	int fd;
};

static struct fd fds[ PIPES_MAX ];
static struct sockaddr_un sock;
static size_t children = 0;
static char* tmppath = NULL;
static char* tmpsock = NULL;

struct owner_rule
{
	size_t path_len;
	char   user[32]; // TODO: Pull from tar.h ?
	char   group[32];
	char   mask[8];
	char*  path;
};

static struct rule_list
{
	struct owner_rule * rules;
	size_t count;
} rules = { NULL, 0 };

#define rule(i) rules.rules[i]
#define pipe(X) (struct pipe_id){ X }
#define fd(X) fds[X].fd
#define fdp(X) fd(X.id)

#define err(code,fmt,...) fprintf( stderr, fmt, __VA_ARGS__, strerror( errno ) ); c_exit( code )

static char const * const default_user  = "root";
static char const * const default_group = "root";
static char const * const default_mask  = "0755";

void close_descriptors()
{
	for ( size_t i = 0; i < PIPES_MAX; ++i )
	{
		if ( fd(i) != -1 )
		{
			close( fd(i) );
		}
	}
}

void free_rules()
{
	for ( size_t i = 0; i < rules.count; ++i )
	{
		free( rule(i).path );
	}
	free( rules.rules );
}

void c_exit( int const status )
{
	close_descriptors();

	while ( wait( NULL ) != -1 );

	free_rules();

	if ( tmpsock )
	{
		if ( unlink( tmpsock ) == -1 )
		{
			fprintf( stderr, "Error unlinking %s: %s\n", tmpsock, strerror( errno ) );
		}
		free( tmpsock );
		tmpsock = NULL;
	}

	if ( tmppath )
	{
		if ( rmdir( tmppath ) == -1 )
		{
			fprintf( stderr, "Error unlinking %s: %s\n", tmppath, strerror( errno ) );
		}
		free( tmppath );
		tmppath = NULL;
	}

	exit( status );
}

void close_fd( struct pipe_id const offset )
{
	if ( fd(offset.id) != -1 )
	{
		close( fd(offset.id) );

		fd(offset.id) = -1;
	}
	else
	{
		fprintf( stderr, "Re-closing closed file %d\n", offset.id );
	}
}

void create_pipes( struct pipe_id const offset )
{
	int pipes[2];

	if ( fdp(offset) != -1 )
	{
		fprintf( stderr, "make-package: Attempting to reopen a file descriptor\n" );
		return;
	}

	if ( (pipe)( pipes ) == -1 )
	{
		perror( "Error creating pipes" );
		c_exit( 2 );
	}

	fdp(offset) = pipes[0];
	fd(offset.id + 1) = pipes[1];
}

void create_fifo( struct pipe_id const sink )
{
	char* path;
	char template[] = "/tmp/bpkg.XXXXXX";
	int result;

	if ( fdp(sink) != - 1 )
	{
		fprintf( stderr, "make-package: Trying to re-open socket\n" );
		c_exit( 2 );
	}

	path = mkdtemp( template );

	if ( path == NULL )
	{
		fprintf( stderr, "make-package: Failed to generate temp path: %s\n", strerror( errno ) );
		c_exit( 2 );
	}

	tmppath = calloc( 1, sizeof("/tmp/bpkg.XXXXXX") );
	tmpsock = calloc( 108, sizeof(char) );

	sprintf ( tmppath, path );
	snprintf( tmpsock,       108, "%s/%s", tmppath, "sock" );
	snprintf( sock.sun_path, 108, "%s/%s", tmppath, "sock" );

	sock.sun_family = AF_UNIX;

	fdp(sink)  = socket( AF_UNIX, SOCK_STREAM, 0 );

	if ( fdp(sink) == -1 )
	{
		err( 2, "make-package: Error opening named pipe %s: %s\n", tmppath );
	}

	result = bind( fdp(sink), &sock, sizeof(sock) );

	if ( result != 0 )
	{
		err( 2, "make-package: Error binding named pipe %s: %s\n", tmppath );
	}

	result = listen( fdp(sink), 1 );

	if ( result != 0 )
	{
		fprintf( stderr, "make-package: Error switching named pipe to listen mode: %s\n", strerror( errno ) );
	}
}

void create_file( char const * const path, struct pipe_id offset )
{
	fdp(offset) = open( path, O_WRONLY | O_CREAT | O_EXCL, 0600 );

	if ( fdp(offset) != -1 )
	{
		return;
	}

	switch ( errno )
	{
		case EEXIST:
			fprintf( stderr, "%s already exists\n", path );
			break;

		case EACCES:
			fprintf( stderr, "%s not writable\n", path );
			break;

		default:
			err( 1, "Unhandled error opening %s: %s\n", path );
	}

	c_exit( 1 );
}

pid_t pipe_fork_exec( char * const * argv, struct pipe_id const in, struct pipe_id const out )
{
	pid_t pid = fork();

	if ( pid )
	{
		if ( pid == -1 )
		{
			perror( argv[0] );
		}

		close_fd( in  );
		close_fd( out );
		++children;

		return pid;
	}

	close( STDIN_FILENO  );
	close( STDOUT_FILENO );

	if ( dup2( fdp(in),  STDIN_FILENO  ) == -1 )
	{
		switch ( errno )
		{
			case EBADF:
				fprintf( stderr, "Unable to open %d for read, not a valid fd for %s\n", fdp(in), argv[0] );
				break;

			case EMFILE:
				fprintf( stderr, "File descriptor limit exceeding forking to %s\n", argv[0] );
		}

		return 0;
	}

	if ( dup2( fdp(out), STDOUT_FILENO ) == -1 )
	{
		switch ( errno )
		{
			case EBADF:
				fprintf( stderr, "Unable to open %d for write, not a valid fd for %s\n", fdp(out), argv[0] );
				break;

			case EMFILE:
				fprintf( stderr, "File descriptor limit exceeding forking to %s\n", argv[0] );
		}

		return 0;
	}

	close_descriptors();

	execvp( argv[0], argv );

	err( 1, "Error calling execvp to %s: %s\n", argv[0] );

	return 0;
}

void check_dir( char const * const path )
{
	fd(SOURCE_DIR) = open( path, O_DIRECTORY | O_RDONLY );

	if ( fd(SOURCE_DIR) == -1 )
	{
		err( 1, "Can not open sourcedir %s: %s\n", path );
	}

	if ( faccessat( fd(SOURCE_DIR), "debian", X_OK | R_OK, AT_EACCESS ) == -1 )
	{
		perror( "Unable to read or execute debian dir" );
		c_exit( 1 );
	}

	if ( faccessat( fd(SOURCE_DIR), "debian/control", R_OK, AT_EACCESS ) == -1 )
	{
		perror( "Unable to read debian/control" );
		c_exit( 1 );
	}

	if ( faccessat( fd(SOURCE_DIR), "owners", R_OK, AT_EACCESS ) == -1 )
	{
		perror( "Unable to read owners" );
		c_exit( 1 );
	}

	if ( faccessat( fd(SOURCE_DIR), "manifest", R_OK, AT_EACCESS ) == -1 )
	{
		perror( "Unable to read manifest" );
		c_exit( 1 );
	}
}

void load_rules()
{
	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Already reading a file when calling load_rules\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "owners", O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		perror( "Unable to open debian/onwers" );
		c_exit( 1 );
	}

	if ( lseek( fd(CURRENT_FILE), 0, SEEK_SET ) == -1 )
	{
		perror( "Unable to seek in debian/onwers" );
		c_exit( 1 );
	}

	if ( flock( fd(CURRENT_FILE), LOCK_SH ) == -1 )
	{
		perror( "Unable to open debian/onwers" );
		c_exit( 1 );
	}


	FILE* file = fdopen( fd(CURRENT_FILE), "r" );

	while ( !feof( file ) )
	{
		if ( fgetc( file ) == '\n' )
		{
			++rules.count;
		}
	}

	rules.rules = calloc( rules.count, sizeof( struct owner_rule ) );

	if ( rules.rules == NULL )
	{
		perror( "Error attempt to allocate rules memory" );
	}

	rewind( file );

	for ( size_t i = 0; i < rules.count; ++i )
	{
		rule(i).path = malloc( 256 );
		fscanf(
			file, "%31s %31s %7s %250[^\n]\n",
			rule(i).user, rule(i).group, rule(i).mask, rule(i).path
		);
		rule(i).path_len = strlen( rule(i).path );
	}

	fclose( file );
	close_fd( pipe(CURRENT_FILE) );
}

void match_rule( char const * const file, size_t const file_len, char const ** user, char const ** group, char const ** mask )
{
	*user  = default_user;
	*group = default_group;
	*mask  = default_mask;

	for ( size_t i = 0; i < rules.count; ++i )
	{
		if ( file_len < rule(i).path_len )
		{
			continue;
		}

		if ( strncmp( file + 1, rule(i).path, rule(i).path_len ) == 0 )
		{
			*user  = rule(i).user;
			*group = rule(i).group;
			*mask  = rule(i).mask;
		}
	}
}

size_t process_control()
{
	DIR * dir;
	struct dirent * file;
	struct stat stat;
	size_t size = 0;

	char const * const user  = "root";
	char const * const group = "root";
	char const * const mask  = "00755";

	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Current directory already open when opening manifest\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "debian", O_DIRECTORY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		fprintf( stderr, "Unable to open debian folder: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	dir = fdopendir( fd(CURRENT_FILE) );

	if ( dir == NULL )
	{
		fprintf( stderr, "Unable to open debian folder: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	while ( 1 )
	{
		errno = 0;

		file = readdir( dir );

		if ( file == NULL )
		{
			if ( errno )
			{
				fprintf( stderr, "Error reading from debian folder: %s\n", strerror( errno ) );
			}
			break;
		}

		if ( file->d_type != DT_REG )
		{
			continue;
		}

		if ( fstatat( fd(CURRENT_FILE), file->d_name, &stat, AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT ) )
		{
			fprintf( stderr, "Error calling stat on debian/%s: %s\n", file->d_name, strerror( errno ) );
		}
		else
		{
			size += stat.st_size;
		}

		write_null_stream( fd(TARSTREAM_INPUT_W), file->d_name );
		write_null_stream( fd(TARSTREAM_INPUT_W), user );
		write_null_stream( fd(TARSTREAM_INPUT_W), group );
		write_null_stream( fd(TARSTREAM_INPUT_W), mask );
	}

	closedir( dir );

	close_fd( pipe(CURRENT_FILE) );
	close_fd( pipe(TARSTREAM_INPUT_W) );

	return size;
}

size_t process_data()
{
	char file[256];
	char byte;
	char const * user, * group, * mask;

	struct stat fstat;
	size_t size = 0;

	size_t offset = 0;
	size_t line   = 1;
	ssize_t result;

	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Current directory already open when opening manifest\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "manifest", O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		fprintf( stderr, "Unable to open manifest file: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	while ( 1 )
	{
		result = read( fd(CURRENT_FILE), &byte, 1 );

		if ( result != 1 )
		{
			if ( result < 0 )
			{
				fprintf( stderr, "Error reading from manifest file at line %lu: %s\n", line, strerror( errno ) );
			}
			break;
		}

		if ( byte != '\n' )
		{
			file[offset] = byte;
			++offset;
			continue;
		}

		file[offset] = '\0';

		// Special case -- ignore debian
		if ( strcmp( file, "./debian" ) == 0 || strncmp( file, "./debian/", 9 ) == 0 )
		{
			fprintf( stderr, "Found debian path in manifest file on line %lu: %s\n", line, file );

			offset = 0;
			++line;

			continue;
		}

		if ( offset > 231 ) // TODO: Get from tar.h
		{
			fprintf( stderr, "Path too long on line %lu: %s\n", line, file );

			offset = 0;
			++line;

			continue;
		}

		if ( stat( file, &fstat ) )
		{
			fprintf( stderr, "Error calling stat on %s: %s\n", file, strerror( errno ) );
		}
		else
		{
			size += fstat.st_size;
		}

		match_rule( file, offset, &user, &group, &mask );

		write_null_stream( fd(TARSTREAM_INPUT_W), file );
		write_null_stream( fd(TARSTREAM_INPUT_W), user );
		write_null_stream( fd(TARSTREAM_INPUT_W), group );
		write_null_stream( fd(TARSTREAM_INPUT_W), mask );

		offset = 0;
		++line;
	}

	close_fd( pipe(CURRENT_FILE) );
	close_fd( pipe(TARSTREAM_INPUT_W) );

	return size;
}

void ar_header( char const * const filename )
{
	int result;
	char ok;

	fd(AR_INPUT_W) = socket( AF_UNIX, SOCK_STREAM, 0 );
	result = connect( fd(AR_INPUT_W), &sock, sizeof(sock) );

	if ( result == -1 )
	{
		err( 2, "make-package: Unable to connect to ar input fifo %s: %s\n", sock.sun_path );
	}

	result = read( fd(AR_INPUT_W), &ok, 1 );

	if ( result != 1 )
	{
		err( 2, "make-package: Unable to connect to ar input fifo %s: %s\n", sock.sun_path );
	}

	write_null_stream( fd(AR_INPUT_W), filename );
}

void ar_footer()
{
	if ( fd(AR_INPUT_W) != -1 )
	{
		close_fd( pipe(AR_INPUT_W) );
	}
}

void write_control( char * out, size_t packed_size, size_t installed_size )
{
	ssize_t result;

	size_t len = strlen( out );

	char * cin  = "debian/control";
	char * cout = calloc( len + 5, sizeof(char) );
	char buffer[32];

	if ( !cout )
	{
		err( 1, "Error allocating %lu bytes for data file name: %s\n", len+5 );
	}

	fd(CONTROL_R) = openat( fd(SOURCE_DIR), cin, O_RDONLY );

	if ( fd(CONTROL_R) == -1 )
	{
		err( 1, "Error opening %s: %s\n", cin );
	}

	strcpy( cout, out );
	strcpy( cout + len, ".dat" );

	create_file( cout, pipe(CONTROL_W) );

	while ( 1 )
	{
		result = sendfile( fd(CONTROL_W), fd(CONTROL_R), NULL, 4096 );

		if ( result == -1 )
		{
			err( 1, "Error writing to %s: %s\n", cout );
		}
		if ( result == 0 )
		{
			break;
		}
	}

	close_fd( pipe(CONTROL_R) );

	len = snprintf( buffer, 30, "Size: %lu\n", packed_size );
	write( fd(CONTROL_W), buffer, len );

	len = snprintf( buffer, 30, "Installed-Size: %lu\n", installed_size );
	write( fd(CONTROL_W), buffer, len );

	close_fd( pipe(CONTROL_W) );

	free( cout );
}

int main( int argc, char ** argv )
{
	char * arstream[]  = {  "ar-stream", NULL };
	char * tarstream[] = { "tar-stream", NULL, NULL };
	char * const xz[]  = { "xz", NULL };
	char * debdir;
	size_t installed_size = 0;
	struct stat package_stat;

	if ( argc != 3 )
	{
		fprintf( stderr, "Usage: %s [directory] [target.deb]\n", argv[0] );
		return 1;
	}

	for ( size_t i = 0; i < PIPES_MAX; ++i )
	{
		fd(i) = -1;
	}

	check_dir( argv[1] );
	load_rules();

	// Put the debian path in a string
	debdir = calloc( strlen( argv[1] ) + 10, sizeof(char) );
	sprintf( debdir, "%s/debian", argv[1] );

	// Create the output (.deb) file
	create_file( argv[2], pipe(DEB_OUTPUT) );
	// Create a named pipe for the 'ar' program
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
		installed_size += process_control();

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
		installed_size += process_data();

	// End data sections
	ar_footer();

	ar_header( "" );
	ar_footer();

	while ( wait( NULL ) != -1 );

	stat( argv[2], &package_stat );

	write_control( argv[2], package_stat.st_size, installed_size );

	close_descriptors();

	free( debdir );

	c_exit( 0 );
}
