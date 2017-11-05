#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#include "files.h"
#include "rules.h"

void c_exit( int const status )
{
	close_descriptors();

	while ( wait( NULL ) != -1 );

	free_rules();

	if ( tmpsock[0] == '/' )
	{
		if ( unlink( tmpsock ) == -1 )
		{
			fprintf( stderr, "Error unlinking %s: %s\n", tmpsock, strerror( errno ) );
		}
	}

	if ( rmdir( tmppath ) == -1 )
	{
		fprintf( stderr, "Error unlinking %s: %s\n", tmppath, strerror( errno ) );
	}

	exit( status );
}
