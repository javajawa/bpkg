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
			errf( 1, "Unable to fork (attempt to fork-exec to %s)", argv[0] );
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
				errfs( 0, "Unable to open %d for read, not a valid fd for %s", fdp(in), argv[0] );
				break;

			case EMFILE:
				errfs( 0, "File descriptor limit exceeding forking to %s", argv[0] );
				break;

			default:
				errf( 0, "Error dup-ing input file descriptor for %s", argv[0] );
		}

		return 0;
	}

	if ( dup2( fdp(out), STDOUT_FILENO ) == -1 )
	{
		switch ( errno )
		{
			case EBADF:
				errfs( 0, "Unable to open %d for read, not a valid fd for %s", fdp(in), argv[0] );
				break;

			case EMFILE:
				errfs( 0, "File descriptor limit exceeding forking to %s", argv[0] );
				break;

			default:
				errf( 0, "Error dup-ing input file descriptor for %s", argv[0] );
		}

		return 0;
	}

	close_descriptors();

	execvp( argv[0], argv );

	errf( 1, "Error calling execvp to %s", argv[0] );

	return 0;
}
