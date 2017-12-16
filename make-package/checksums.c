#include "checksums.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "error.h"
#include "files.h"
#include "pipe-fork-exec.h"

void process_output( char * output, struct package_data* stats )
{
	char * const chk1cmd[] = { CHK1_COMMAND, NULL };
	char * const chk2cmd[] = { CHK2_COMMAND, NULL };

	char buffer[4096];

	ssize_t b_read;

	if ( fd(CURRENT_FILE) != -1 )
	{
		// TODO: Error handling
	}

	fd(CURRENT_FILE) = open( output, O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		// TODO: Error handling
	}

	create_pipes( pipe(CHK1_INPUT_R) );
	create_pipes( pipe(CHK1_OUTPUT_R) );
	create_pipes( pipe(CHK2_INPUT_R) );
	create_pipes( pipe(CHK2_OUTPUT_R) );

	pipe_fork_exec( chk1cmd, pipe(CHK1_INPUT_R), pipe(CHK1_OUTPUT_W) );
	pipe_fork_exec( chk2cmd, pipe(CHK2_INPUT_R), pipe(CHK2_OUTPUT_W) );

	while ( 1 )
	{
		b_read = read( fd(CURRENT_FILE), buffer, 4096 );

		if ( b_read == 0 )
		{
			break;
		}

		if ( b_read == -1 )
		{
			perror( "Error reading output file" );
			c_exit( 1 );
		}

		stats->packed_size += b_read;

		write( fd(CHK1_INPUT_W), buffer, b_read );
		write( fd(CHK2_INPUT_W), buffer, b_read );

		// TODO: Check write lengths are correct
	}

	close_fd( pipe(CHK1_INPUT_W) );
	close_fd( pipe(CHK2_INPUT_W) );

	read( fd(CHK1_OUTPUT_R), stats->chk1, CHK1_SIZE );
	read( fd(CHK2_OUTPUT_R), stats->chk2, CHK2_SIZE );

	close_fd( pipe(CHK1_OUTPUT_R) );
	close_fd( pipe(CHK2_OUTPUT_R) );
}

