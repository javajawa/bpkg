#define _GNU_SOURCE

#include "pipe-fork-exec.h"

#include <stdio.h>

#include "error.h"

pid_t pipe_fork_exec( char * const * argv, struct pipe_id const in, struct pipe_id const out )
{
	pid_t pid = fork();

	if ( pid )
	{
		if ( pid == -1 )
		{
			perror( argv[0] );
		}

		close_fd( in  );
		close_fd( out );

		return pid;
	}

	close( STDIN_FILENO  );
	close( STDOUT_FILENO );

	if ( dup2( fdp(in),  STDIN_FILENO  ) == -1 )
	{
		switch ( errno )
		{
			case EBADF:
				fprintf( stderr, "Unable to open %d for read, not a valid fd for %s\n", fdp(in), argv[0] );
				break;

			case EMFILE:
				fprintf( stderr, "File descriptor limit exceeding forking to %s\n", argv[0] );
		}

		return 0;
	}

	if ( dup2( fdp(out), STDOUT_FILENO ) == -1 )
	{
		switch ( errno )
		{
			case EBADF:
				fprintf( stderr, "Unable to open %d for write, not a valid fd for %s\n", fdp(out), argv[0] );
				break;

			case EMFILE:
				fprintf( stderr, "File descriptor limit exceeding forking to %s\n", argv[0] );
		}

		return 0;
	}

	close_descriptors();

	execvp( argv[0], argv );

	err( 1, "Error calling execvp to %s: %s\n", argv[0] );

	return 0;
}
