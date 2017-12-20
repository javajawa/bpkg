#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

#define PROGRAM_NAME "ar-stream"

#include "common/null-stream.h"
#include "common/error.h"

void clean_exit( int code )
{
	close( STDIN_FILENO );
	exit( code );
}

int main( void )
{
	char const * const zero = "0               ";

	int     input_socket;
	char    buffer[4096];
	ssize_t bytes_read;
	ssize_t size = 0;
	off_t   seek;

	struct  sockaddr sock;
	socklen_t sock_size = sizeof(sock);

	bytes_read = getsockname( STDIN_FILENO, &sock, &sock_size );

	if ( bytes_read == -1 )
	{
		err( 1, "Error getting socket info" );
	}

	write( STDOUT_FILENO, "!<arch>\n", 8 );

	while ( 1 )
	{
		input_socket = accept( STDIN_FILENO, &sock, &sock_size );

		if ( input_socket == -1 )
		{
			err( 1, "Accept failed" );
		}

		size = 0;
		memset( buffer, ' ', 17 );

		write( input_socket, "k", 1 );
		bytes_read = read_null_stream( input_socket, buffer, 16 );

		if ( bytes_read == 0 )
		{
			clean_exit( 0 );
		}

		if ( bytes_read < 0 )
		{
			err( 1, "Error reading from input" );
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
			errf( 0, "Error getting seek location for file %s", buffer );
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
				err( 0, "Error reading from input socket" );
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
				errf( 0, "Error seeking back to %lu to write size\n", seek );
				continue;
			}

			bytes_read = sprintf( buffer, "%-10lu", size );

			if ( bytes_read < 0 )
			{
				err( 0, "Error converting file size to decimal" );
				continue;
			}

			if ( bytes_read > 10 )
			{
				errfs( 0, "Filed to write size: Size %lu exceeds 12 decimal digits\n", size );
				continue;
			}

			bytes_read = write( STDOUT_FILENO, buffer, bytes_read );

			if ( bytes_read < 0 )
			{
				err( 0, "Error writing size back to archive" );
			}

			bytes_read = lseek( STDOUT_FILENO, 0, SEEK_END );

			if ( bytes_read == -1 )
			{
				err( 1, "Unable to seek back to the end of the output file" );
			}
		}
	}
}

