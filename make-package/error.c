#include "error.h"

#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

#include "files.h"
#include "rules.h"

void clean_exit( int const status )
{
	close_descriptors();

	while ( wait( NULL ) != -1 );

	free_rules();

	if ( tmpsock[0] == '/' )
	{
		if ( unlink( tmpsock ) == -1 )
		{
			errf( 0, "Error unlinking %s", tmpsock );
		}

		if ( rmdir( tmppath ) == -1 )
		{
			errf( 0, "Error unlinking %s", tmppath );
		}
	}

	exit( status );
}
