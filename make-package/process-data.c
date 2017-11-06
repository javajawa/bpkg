#define _GNU_SOURCE

#include "process-data.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "files.h"
#include "error.h"
#include "rules.h"

#include "common/null-stream.h"
#include "common/tar.h"

size_t process_data()
{
	char file[256];
	char byte;
	char const * user, * group, * mask;

	struct stat statf;
	size_t size = 0;

	size_t offset = 0;
	size_t read_c = 0;
	size_t total  = 0;
	size_t line   = 1;
	ssize_t result;

	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Current directory already open when opening manifest\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "manifest", O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		fprintf( stderr, "Unable to open manifest file: %s\n", strerror( errno ) );
		c_exit( 1 );
	}

	result = fstat( fd(CURRENT_FILE), &statf );

	if ( result == -1 )
	{
		fprintf( stderr, "Unable to stat manifest file: %s\n", strerror( errno ) );
	}

	total = statf.st_size;

	while ( 1 )
	{
		// TODO: This is really slow. Buffer the read?
		result = read( fd(CURRENT_FILE), &byte, 1 );

		if ( result != 1 )
		{
			if ( result < 0 )
			{
				fprintf( stderr, "Error reading from manifest file at line %lu: %s\n", line, strerror( errno ) );
			}
			break;
		}

		if ( byte != '\n' )
		{
			file[offset] = byte;
			++offset;
			++read_c;
			continue;
		}

		file[offset] = '\0';

		// Special case -- ignore debian
		if ( strcmp( file, "./debian" ) == 0 || strncmp( file, "./debian/", 9 ) == 0 )
		{
			fprintf( stderr, "Found debian path in manifest file on line %lu: %s\n", line, file );

			offset = 0;
			++line;

			continue;
		}

		if ( offset > TAR_FILELEN )
		{
			fprintf( stderr, "Path too long on line %lu: %s\n", line, file );

			offset = 0;
			++line;

			continue;
		}

		if ( stat( file, &statf ) )
		{
			fprintf( stderr, "Error calling stat on %s: %s\n", file, strerror( errno ) );
		}
		else
		{
			size += statf.st_size;
		}

		match_rule( file, offset, &user, &group, &mask );

		write_null_stream( fd(TARSTREAM_INPUT_W), file );
		write_null_stream( fd(TARSTREAM_INPUT_W), user );
		write_null_stream( fd(TARSTREAM_INPUT_W), group );
		write_null_stream( fd(TARSTREAM_INPUT_W), mask );

		offset = 0;
		++line;

		if ( total && isatty( STDERR_FILENO ) )
		{
			fprintf( stderr, "\rProcessing data: %.1f%%", ( (float)read_c / total * 100 ) );
		}
	}

	close_fd( pipe(CURRENT_FILE) );
	close_fd( pipe(TARSTREAM_INPUT_W) );

	if ( isatty( STDERR_FILENO ) )
	{
		fprintf( stderr, "\rFinished processing manifest\n" );
	}

	return size;
}
