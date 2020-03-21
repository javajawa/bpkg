#include "checksums.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "error.h"
#include "files.h"
#include "pipe-fork-exec.h"

void process_output( char * output, struct package_data* stats )
{
	char buffer[4096];

	ssize_t b_read;
	ssize_t b_write;

	if ( fd(CURRENT_FILE) != -1 )
	{
		err( 1, "'Current' file descriptor still open" );
	}

	fd(CURRENT_FILE) = open( output, O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		errf( 1, "Unable to open %s for reading", output );
	}

	create_pipes( pipe(CHK1_INPUT_R) );
	create_pipes( pipe(CHK1_OUTPUT_R) );
	create_pipes( pipe(CHK2_INPUT_R) );
	create_pipes( pipe(CHK2_OUTPUT_R) );

	pipe_fork_exec( CHK1_COMMAND, pipe(CHK1_INPUT_R), pipe(CHK1_OUTPUT_W) );
	pipe_fork_exec( CHK2_COMMAND, pipe(CHK2_INPUT_R), pipe(CHK2_OUTPUT_W) );

	while ( 1 )
	{
		b_read = read( fd(CURRENT_FILE), buffer, 4096 );

		if ( b_read == 0 )
		{
			break;
		}

		if ( b_read == -1 )
		{
			err( 1, "Error reading from output file" );
		}

		stats->packed_size += b_read;

		b_write = write( fd(CHK1_INPUT_W), buffer, b_read );

		if ( b_write == -1 )
		{
			err( 0, "Error writing to '" CHK1_NAME "' tool" );
		}
		if ( b_write != b_read )
		{
			errfs( 0, "Size mismatch: wrote %lu bytes of %lu to '" CHK1_NAME "'", b_write, b_read );
		}

		b_write = write( fd(CHK2_INPUT_W), buffer, b_read );

		if ( b_write == -1 )
		{
			err( 0, "Error writing to '" CHK2_NAME "' tool" );
		}
		if ( b_write != b_read )
		{
			errfs( 0, "Size mismatch: wrote %lu bytes of %lu to '" CHK2_NAME "'", b_write, b_read );
		}
	}

	close_fd( pipe(CHK1_INPUT_W) );
	close_fd( pipe(CHK2_INPUT_W) );

	b_read = read( fd(CHK1_OUTPUT_R), stats->chk1, CHK1_SIZE );

	if ( b_read == -1 )
	{
		err( 0, "Error reading response for checksum '" CHK1_NAME "'" );
	}
	if ( b_read != CHK1_SIZE )
	{
		errfs( 0, "Incorrect size for checksum '" CHK1_NAME "', expected %d, got %lu", CHK1_SIZE, b_read );
	}

	b_read = read( fd(CHK2_OUTPUT_R), stats->chk2, CHK2_SIZE );

	if ( b_read == -1 )
	{
		err( 0, "Error reading response for checksum '" CHK2_NAME "'" );
	}
	if ( b_read != CHK2_SIZE )
	{
		errfs( 0, "Incorrect size for checksum '" CHK2_NAME "', expected %d, got %lu", CHK2_SIZE, b_read );
	}

	close_fd( pipe(CHK1_OUTPUT_R) );
	close_fd( pipe(CHK2_OUTPUT_R) );
}

