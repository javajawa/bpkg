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
	int input_socket;
	char buffer[4096];
	ssize_t bytes_read;
	ssize_t size = 0;
	off_t seek;

	struct sockaddr sock;
	socklen_t sock_size = sizeof(sock);

	bytes_read = getsockname( STDIN_FILENO, &sock, &sock_size );

	char const * const zero = "0               ";

	if ( bytes_read == -1 )
	{
		fprintf( stderr, "ar-stream: Error getting socket info: %s\n", strerror( errno ) );
		return 2;
	}

	write( STDOUT_FILENO, "!<arch>\n", 8 );

	while ( 1 )
	{
		input_socket = accept( STDIN_FILENO, &sock, &sock_size );

		if ( input_socket == -1 )
		{
			fprintf( stderr, "ar-stream: Accept resturned %s\n", strerror( errno ) );
			return 2;
		}

		size = 0;
		memset( buffer, ' ', 17 );

		write( input_socket, "k", 1 );
		bytes_read = read_null_stream( input_socket, buffer, 16 );

		if ( bytes_read == 0 )
		{
			close( STDIN_FILENO );
			return 0;
		}

		if ( bytes_read < 0 )
		{
			fprintf( stderr, "ar-stream: Error reading from input: %s\n", strerror( -bytes_read ) );
			return 1;
		}

		buffer[bytes_read] = ' ';
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
			bytes_read = read( input_socket, buffer, 4096 );

			if ( bytes_read == 0 )
			{
				close( input_socket );
				break;
			}
			if ( bytes_read == -1 )
			{
				fprintf( stderr, "ar-stream: Error reading from input: %s", strerror(errno ) );
				close( input_socket );
				break;
			}

			size += bytes_read;
			write( STDOUT_FILENO, buffer, bytes_read );
		}

		if ( seek )
		{
			bytes_read = lseek( STDOUT_FILENO, seek, SEEK_SET );

			if ( bytes_read < 0 )
			{
				fprintf( stderr, "Error seeking back to %lu to write size: %s\n", seek, strerror( errno ) );
				continue;
			}

			// TODO: replace 10 with a #define
			bytes_read = sprintf( buffer, "%-10lu", size );

			if ( bytes_read < 0 )
			{
				fprintf( stderr, "Error converting size to decimal: %s\n", strerror( errno ) );
				continue;
			}

			if ( bytes_read > 10 )
			{
				fprintf( stderr, "Filed to write size: Size %lu exceeds 12 decimal digits\n", size );
				continue;
			}

			bytes_read = write( STDOUT_FILENO, buffer, bytes_read );

			if ( bytes_read < 0 )
			{
				// TODO: error
			}

			bytes_read = lseek( STDOUT_FILENO, 0, SEEK_END );

			if ( bytes_read == -1 )
			{
				fprintf( stderr, "Unable to see back to the end of the output file: %s\n", strerror( errno ) );
				return 4;
			}
		}
	}
}

