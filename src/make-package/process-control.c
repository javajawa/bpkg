#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "process-control.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "files.h"
#include "error.h"

#include "common/null-stream.h"

void process_control()
{
	DIR * dir;
	struct dirent * file;
	ssize_t result;

	char const * const user  = "root";
	char const * const group = "root";
	char const * const mask  = "00755";

	if ( fd(CURRENT_FILE) != -1 )
	{
		errs( 1, "Current directory already open when opening manifest" );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "debian", O_DIRECTORY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		err( 1, "Unable to open debian folder descriptor" );
	}

	dir = fdopendir( fd(CURRENT_FILE) );

	if ( dir == NULL )
	{
		err( 1, "Unable to open debian folder file list" );
	}

	while ( 1 )
	{
		errno = 0;

		file = readdir( dir );

		if ( file == NULL )
		{
			if ( errno )
			{
				err( 1, "Error reading from debian folder" );
			}
			break;
		}

		if ( file->d_type != DT_REG )
		{
			continue;
		}

		result = write_null_stream( fd(TARSTREAM_INPUT_W), file->d_name );

		if ( result == -1 )
		{
			errf( 1, "Error writing to tar-stream (file=%s)", file->d_name );
		}

		result = write_null_stream( fd(TARSTREAM_INPUT_W), user );

		if ( result == -1 )
		{
			errf( 1, "Error writing to tar-stream (file=%s)", file->d_name );
		}

		result = write_null_stream( fd(TARSTREAM_INPUT_W), group );

		if ( result == -1 )
		{
			errf( 1, "Error writing to tar-stream (file=%s)", file->d_name );
		}

		result = write_null_stream( fd(TARSTREAM_INPUT_W), mask );

		if ( result == -1 )
		{
			errf( 1, "Error writing to tar-stream (file=%s)", file->d_name );
		}
	}

	closedir( dir );

	close_fd( pipe(CURRENT_FILE) );
	close_fd( pipe(TARSTREAM_INPUT_W) );

	if ( isatty( STDERR_FILENO ) )
	{
		fprintf( stderr, "Finished processing control file\n" );
	}
}
