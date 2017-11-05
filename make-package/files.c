#define _GNU_SOURCE

// Must be included first to avoid conflict with pipe(2) and
// the #define pipe(X) macro
#include <fcntl.h>
#include <unistd.h>

#include "files.h"
#include "error.h"

#include <stdlib.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

char tmppath[17] = "/tmp/bpkg.XXXXXX";
char tmpsock[24] = "";
struct sockaddr_un sock;

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
	int result;

	if ( fdp(sink) != - 1 )
	{
		fprintf( stderr, "make-package: Trying to re-open socket\n" );
		c_exit( 2 );
	}

	path = mkdtemp( tmppath );

	if ( path == NULL )
	{
		fprintf( stderr, "make-package: Failed to generate temp path: %s\n", strerror( errno ) );
		c_exit( 2 );
	}

	snprintf( tmpsock,       23, "%s/%s", tmppath, "sock" );
	snprintf( sock.sun_path, 23, "%s/%s", tmppath, "sock" );

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
