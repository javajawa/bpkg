#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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
	ssize_t len = strlen( out );

	char * cin  = "debian/control";
	char * cout = calloc( len + 5, sizeof(char) );
	char buffer[256];

	if ( !cout )
	{
		errf( 1, "Error allocating %lu bytes for data file name", len+5 );
	}

	fd(CONTROL_R) = openat( fd(SOURCE_DIR), cin, O_RDONLY );

	if ( fd(CONTROL_R) == -1 )
	{
		errf( 1, "Error opening %s", cin );
	}

	strcpy( cout, out );
	strcpy( cout + len, ".dat" );

	create_file( cout, pipe(CONTROL_W) );

	while ( 1 )
	{
		result = sendfile( fd(CONTROL_W), fd(CONTROL_R), NULL, 4096 );

		if ( result == -1 )
		{
			errf( 1, "Error writing to %s", cout );
		}
		if ( result == 0 )
		{
			break;
		}
	}

	close_fd( pipe(CONTROL_R) );

	len = snprintf( buffer, 254, "Filename: %s\n", out );
	result = write( fd(CONTROL_W), buffer, len );

	if ( result == -1 )
	{
		err( 0, "Error writing " CHK2_NAME ": to control file" );
	}
	if ( result != len )
	{
		errf( 0, "Only wrote %lu of %lu bytes of " CHK2_NAME ": to control file", result, len );
	}

	len = snprintf( buffer, 30, "Size: %lu\n", stats.packed_size );
	result = write( fd(CONTROL_W), buffer, len );

	if ( result == -1 )
	{
		err( 0, "Error writing Size: to control file" );
	}
	if ( result != len )
	{
		errf( 0, "Only wrote %lu of %lu bytes of Size: to control file", result, len );
	}

	len = snprintf( buffer, 30, "Installed-Size: %lu\n", stats.installed_size >> 10 );
	result = write( fd(CONTROL_W), buffer, len );

	if ( result == -1 )
	{
		err( 0, "Error writing Installed-Size: to control file" );
	}
	if ( result != len )
	{
		errf( 0, "Only wrote %lu of %lu bytes of Installed-Size: to control file", result, len );
	}

	len = snprintf( buffer, 254, CHK1_NAME ": %s\n", stats.chk1 );
	result = write( fd(CONTROL_W), buffer, len );

	if ( result == -1 )
	{
		err( 0, "Error writing " CHK1_NAME ": to control file" );
	}
	if ( result != len )
	{
		errf( 0, "Only wrote %lu of %lu bytes of " CHK1_NAME ": to control file", result, len );
	}

	len = snprintf( buffer, 254, CHK2_NAME ": %s\n", stats.chk2 );
	result = write( fd(CONTROL_W), buffer, len );

	if ( result == -1 )
	{
		err( 0, "Error writing " CHK2_NAME ": to control file" );
	}
	if ( result != len )
	{
		errf( 0, "Only wrote %lu of %lu bytes of " CHK2_NAME ": to control file", result, len );
	}

	result = write( fd(CONTROL_W), "\n", 1 );

	if ( result == -1 )
	{
		err( 0, "Error writing " CHK2_NAME ": to control file" );
	}
	if ( result != 1 )
	{
		errf( 0, "Wrote %lu of 1 bytes for trailing line in control file", result );
	}

	close_fd( pipe(CONTROL_W) );

	free( cout );
}
