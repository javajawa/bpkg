#define _GNU_SOURCE

#include "process-control.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "files.h"
#include "error.h"

#include "common/null-stream.h"

size_t process_control()
{
	DIR * dir;
	struct dirent * file;
	struct stat stat;
	size_t size = 0;

	char const * const user  = "root";
	char const * const group = "root";
	char const * const mask  = "00755";

	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Current directory already open when opening manifest\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "debian", O_DIRECTORY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		fprintf( stderr, "Unable to open debian folder: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	dir = fdopendir( fd(CURRENT_FILE) );

	if ( dir == NULL )
	{
		fprintf( stderr, "Unable to open debian folder: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	while ( 1 )
	{
		errno = 0;

		file = readdir( dir );

		if ( file == NULL )
		{
			if ( errno )
			{
				fprintf( stderr, "Error reading from debian folder: %s\n", strerror( errno ) );
			}
			break;
		}

		if ( file->d_type != DT_REG )
		{
			continue;
		}

		if ( fstatat( fd(CURRENT_FILE), file->d_name, &stat, AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT ) )
		{
			fprintf( stderr, "Error calling stat on debian/%s: %s\n", file->d_name, strerror( errno ) );
		}
		else
		{
			size += stat.st_size;
		}

		write_null_stream( fd(TARSTREAM_INPUT_W), file->d_name );
		write_null_stream( fd(TARSTREAM_INPUT_W), user );
		write_null_stream( fd(TARSTREAM_INPUT_W), group );
		write_null_stream( fd(TARSTREAM_INPUT_W), mask );
	}

	closedir( dir );

	close_fd( pipe(CURRENT_FILE) );
	close_fd( pipe(TARSTREAM_INPUT_W) );

	if ( isatty( STDERR_FILENO ) )
	{
		fprintf( stderr, "Finished processing control file\n" );
	}

	return size;
}
