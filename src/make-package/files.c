#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
		errfs( 0, "Re-closed file %d\n", offset.id );
	}
}

void create_pipes( struct pipe_id const offset )
{
	int pipes[2];

	if ( fdp(offset) != -1 )
	{
		errf( 0, "Attempting to reopen file descriptor %d", fdp(offset) );
		return;
	}

	if ( (pipe)( pipes ) == -1 )
	{
		errf( 1, "Error creating pipe for descriptor %d", fdp(offset) );
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
		errs( 1, "Trying to re-open socket" );
	}

	path = mkdtemp( tmppath );

	if ( path == NULL )
	{
		err( 1, "Failed to generate temp path" );
	}

	snprintf( tmpsock,       23, "%s/%s", tmppath, "sock" );
	snprintf( sock.sun_path, 23, "%s/%s", tmppath, "sock" );

	sock.sun_family = AF_UNIX;

	fdp(sink)  = socket( AF_UNIX, SOCK_STREAM, 0 );

	if ( fdp(sink) == -1 )
	{
		errf( 1, "Error opening named pipe %s", tmppath );
	}

	result = bind( fdp(sink), &sock, sizeof(sock) );

	if ( result != 0 )
	{
		errf( 1, "Error binding named pipe %s", tmppath );
	}

	result = listen( fdp(sink), 1 );

	if ( result != 0 )
	{
		err( 1, "Error switching named pipe to listen mode" );
	}
}

void create_file( char const * const path, struct pipe_id offset )
{
	fdp(offset) = open( path, O_WRONLY | O_CREAT | O_EXCL, 0640 );

	if ( fdp(offset) != -1 )
	{
		return;
	}

	switch ( errno )
	{
		case EEXIST:
			errfs( 1, "%s already exists", path );
			break;

		case EACCES:
			errfs( 1, "%s not writable", path );
			break;
	}

	errf( 1, "Unhandled error opening %s", path );
}
