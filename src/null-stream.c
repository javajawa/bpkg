#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "null-stream.h"

ssize_t read_null_stream( int fd, char * const dest, size_t const maxlen )
{
	size_t offset = 0;
	ssize_t result;

	while ( offset < maxlen )
	{
		result = read( fd, dest + offset, 1 );


		if ( result == -1 )
		{
			dest[offset] = '\0';
			return -errno;
		}

		if ( result == 0 )
		{
			dest[offset] = '\0';
			return -1;
		}

		if ( dest[offset] == '\0' )
		{
			break;
		}

		++offset;
	}

	return offset;
}

size_t write_null_stream( int fd, char const * const src )
{
	size_t len = strlen( src ) + 1;

	return write( fd, src, len );
}
