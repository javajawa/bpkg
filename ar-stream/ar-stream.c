#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "common/null-stream.h"

int main( void ) // int argc, char** argv )
{
	int fd;
	char buffer[4096];
	ssize_t r;
	ssize_t size = 0;
	off_t seek;

	struct sockaddr sock;
	socklen_t sock_size = sizeof(sock);

	r = getsockname( STDIN_FILENO, &sock, &sock_size );

	char const * const zero = "0               ";

	if ( r == -1 )
	{
		fprintf( stderr, "ar-stream: Error getting socket info: %s\n", strerror( errno ) );
		return 2;
	}

	write( STDOUT_FILENO, "!<arch>\n", 8 );

	while ( 1 )
	{
		fd = accept( STDIN_FILENO, &sock, &sock_size );

		if ( fd == -1 )
		{
			fprintf( stderr, "ar-stream: Accept resturned %s\n", strerror( errno ) );
			return 2;
		}

		size = 0;
		memset( buffer, ' ', 17 );

		write( fd, "k", 1 );
		r = read_null_stream( fd, buffer, 16 );

		if ( r == 0 )
		{
			close( STDIN_FILENO );
			return 0;
		}

		if ( r < 0 )
		{
			fprintf( stderr, "ar-stream: Error reading from input: %s\n", strerror( -r ) );
			return 1;
		}

		buffer[r] = ' ';
		write( STDOUT_FILENO, buffer, 16 );
		write( STDOUT_FILENO, zero, 12 ); // Timestamp
		write( STDOUT_FILENO, zero, 6 );  // Owner
		write( STDOUT_FILENO, zero, 6 );  // Group
		write( STDOUT_FILENO, "100644  ", 8 );  // Mode
		seek = lseek( STDOUT_FILENO, 0, SEEK_CUR );
		write( STDOUT_FILENO, zero, 10 ); // Size
		write( STDOUT_FILENO, "`\n", 2 ); // End

		if ( seek == -1 )
		{
			fprintf( stderr, "Error getting seek location for file %s: %s\n", buffer, strerror( errno ) );
			seek = 0;
		}

		while ( 1 )
		{
			r = read( fd, buffer, 4096 );

			if ( r == 0 )
			{
				close( fd );
				break;
			}
			if ( r == -1 )
			{
				fprintf( stderr, "ar-stream: Error reading from input: %s", strerror(errno ) );
				close( fd );
				break;
			}

			size += r;
			write( STDOUT_FILENO, buffer, r );
		}

		if ( seek )
		{
			r = lseek( STDOUT_FILENO, seek, SEEK_SET );

			if ( r < 0 )
			{
				fprintf( stderr, "Error seeking back to %lu to write size: %s\n", seek, strerror( errno ) );
				continue;
			}

// TODO: replace 10 with a #define
			r = sprintf( buffer, "%-10lu", size );

			if ( r < 0 )
			{
				fprintf( stderr, "Error converting size to decimal: %s\n", strerror( errno ) );
				continue;
			}

			if ( r > 10 )
			{
				fprintf( stderr, "Filed to write size: Size %lu exceeds 12 decimal digits\n", size );
				continue;
			}

			r = write( STDOUT_FILENO, buffer, r );

			if ( r < 0 )
			{
				// TODO: error
			}

			r = lseek( STDOUT_FILENO, 0, SEEK_END );

			if ( r == -1 )
			{
				fprintf( stderr, "Unable to see back to the end of the output file: %s\n", strerror( errno ) );
				return 4;
			}
		}
	}
}

