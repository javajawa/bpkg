#define _GNU_SOURCE

#include "write-control.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#include "files.h"
#include "error.h"

void write_control( char * out, struct package_data stats )
{
	ssize_t result;

	size_t len = strlen( out );

	char * cin  = "debian/control";
	char * cout = calloc( len + 5, sizeof(char) );
	char buffer[256];

	if ( !cout )
	{
		err( 1, "Error allocating %lu bytes for data file name: %s\n", len+5 );
	}

	fd(CONTROL_R) = openat( fd(SOURCE_DIR), cin, O_RDONLY );

	if ( fd(CONTROL_R) == -1 )
	{
		err( 1, "Error opening %s: %s\n", cin );
	}

	strcpy( cout, out );
	strcpy( cout + len, ".dat" );

	create_file( cout, pipe(CONTROL_W) );

	while ( 1 )
	{
		result = sendfile( fd(CONTROL_W), fd(CONTROL_R), NULL, 4096 );

		if ( result == -1 )
		{
			err( 1, "Error writing to %s: %s\n", cout );
		}
		if ( result == 0 )
		{
			break;
		}
	}

	close_fd( pipe(CONTROL_R) );

	len = snprintf( buffer, 30, "Size: %lu\n", stats.packed_size );
	write( fd(CONTROL_W), buffer, len );

	len = snprintf( buffer, 30, "Installed-Size: %lu\n", stats.installed_size );
	write( fd(CONTROL_W), buffer, len );

	len = snprintf( buffer, 254, CHK1_NAME ": %s\n", stats.chk1 );
	write( fd(CONTROL_W), buffer, len );

	len = snprintf( buffer, 254, CHK2_NAME ": %s\n", stats.chk2 );
	write( fd(CONTROL_W), buffer, len );

	close_fd( pipe(CONTROL_W) );

	free( cout );
}
