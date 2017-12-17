#define _GNU_SOURCE

#include "bpkg-ar.h"

#include <sys/socket.h>
#include <sys/un.h>

#include "error.h"
#include "common/null-stream.h"
#include "files.h"

void ar_header( char const * const filename )
{
	int result;
	char ok;

	fd(AR_INPUT_W) = socket( AF_UNIX, SOCK_STREAM, 0 );
	result = connect( fd(AR_INPUT_W), &sock, sizeof(sock) );

	if ( result == -1 )
	{
		errf( 2, "Unable to connect to ar input fifo %s", sock.sun_path );
	}

	result = read( fd(AR_INPUT_W), &ok, 1 );

	if ( result != 1 )
	{
		errf( 2, "Unable to connect to ar input fifo %s", sock.sun_path );
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
