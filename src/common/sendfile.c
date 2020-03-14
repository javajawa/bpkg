#include "common/sendfile.h"

#ifndef _LIBC_SENDFILE

#include <unistd.h>

ssize_t sendfile(int const out_fd, int const in_fd, off_t *offset, size_t count)
{
	char buffer[4096];
	size_t done = 0;
	ssize_t result_read, result_write;

	if ( offset != NULL )
	{
		// TODO: Set error number here.
		return -1;
	}

	while ( done < count )
	{
		result_read = read(in_fd, buffer, 4095);

		if ( result_read == -1 )
		{
			return -1;
		}

		if ( result_read == 0 )
		{
			break;
		}

		result_write = write( out_fd, buffer, result_read );

		if ( result_write == -1 )
		{
			return -1;
		}

		if ( result_write != result_read )
		{
			// TODO: What errno should be set here?
			return -1;
		}

		done += result_read;
	}

	return done;
}

#endif
