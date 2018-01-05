#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>

#define PROGRAM_NAME "ar-stream"

#include "common/error.h"

void clean_exit( int code )
{
	close( STDIN_FILENO );
	exit( code );
}

#define checked_header_write( field, buffer, length ) \
	bytes_written = write( STDOUT_FILENO, buffer, length );\
	if ( bytes_written < 0 )\
	{\
		err( 0, "Error writing ar header field" field );\
	}\
	if ( bytes_written != length )\
	{\
		errfs( 0, "Only wrote %lu of %d bytes for ar header field" field, bytes_written, length );\
	}

int main( void )
{
	char const * const zero = "0               ";

	int     input_socket;
	char    buffer[4096];
	ssize_t bytes_read;
	ssize_t bytes_written;
	ssize_t size = 0;
	off_t   seek;

	struct  sockaddr sock;
	socklen_t sock_size = sizeof(sock);

	bytes_read = getsockname( STDIN_FILENO, &sock, &sock_size );

	if ( bytes_read == -1 )
	{
		err( 1, "Error getting socket info" );
	}

	checked_header_write( "Archive Marker", "!<arch>\n", 8 );

	while ( 1 )
	{
		input_socket = accept( STDIN_FILENO, &sock, &sock_size );

		if ( input_socket == -1 )
		{
			err( 1, "Accept failed" );
		}

		size = 0;
		memset( buffer, ' ', 17 );

		bytes_written = write( input_socket, "k", 1 );

		if ( bytes_written < 0 )
		{
			err( 1, "Error sending ACK packet on input socket" );
		}

		if ( bytes_written != 1 )
		{
			errs( 1, "Error sending ACK packet on input socket" );
		}

		bytes_read = read( input_socket, buffer, 16 );

		if ( bytes_read < 0 )
		{
			err( 1, "Error reading from input" );
		}

		if ( buffer[bytes_read-1] != 0 )
		{
			errs( 1, "Error reading from input: filename not terminated" );
		}

		if ( bytes_read == 1 )
		{
			clean_exit( 0 );
		}

		bytes_written = write( input_socket, "k", 1 );

		if ( bytes_written < 0 )
		{
			err( 1, "Error sending ACK packet on input socket" );
		}

		if ( bytes_written != 1 )
		{
			errs( 1, "Error sending ACK packet on input socket" );
		}

		buffer[bytes_read-1] = ' ';
		checked_header_write( "Filename",  buffer, 16 );
		checked_header_write( "Timestamp", zero, 12 );
		checked_header_write( "Owner",     zero, 6 );
		checked_header_write( "Group",     zero, 6 );
		checked_header_write( "File Mode", "100644  ", 8 );
		seek = lseek( STDOUT_FILENO, 0, SEEK_CUR );
		checked_header_write( "Size",       zero, 10 );
		checked_header_write( "EOH",        "`\n", 2 );

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
			bytes_written = write( STDOUT_FILENO, buffer, bytes_read );

			if ( bytes_written < 0 )
			{
				err( 0, "Error writing size back to archive" );
			}

			if ( bytes_written != bytes_read )
			{
				errfs( 0, "Only wrote %ld of %ld bytes to archive", bytes_written, bytes_read );
			}

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

			bytes_written = write( STDOUT_FILENO, buffer, bytes_read );

			if ( bytes_written < 0 )
			{
				err( 0, "Error writing size back to archive" );
			}

			if ( bytes_written != bytes_read )
			{
				errfs( 0, "Only wrote %ld of %ld bytes to archive", bytes_written, bytes_read );
			}

			bytes_read = lseek( STDOUT_FILENO, 0, SEEK_END );

			if ( bytes_read == -1 )
			{
				err( 1, "Unable to seek back to the end of the output file" );
			}
		}
	}
}

