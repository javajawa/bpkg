#define _GNU_SOURCE

#include "rules.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/types.h>

#include "files.h"
#include "error.h"

struct rule_list rules = { NULL, 0 };

void load_rules()
{
	if ( fd(CURRENT_FILE) != -1 )
	{
		fprintf( stderr, "Already reading a file when calling load_rules\n" );
		c_exit( 1 );
	}

	fd(CURRENT_FILE) = openat( fd(SOURCE_DIR), "owners", O_RDONLY );

	if ( fd(CURRENT_FILE) == -1 )
	{
		perror( "Unable to open debian/onwers" );
		c_exit( 1 );
	}

	if ( lseek( fd(CURRENT_FILE), 0, SEEK_SET ) == -1 )
	{
		perror( "Unable to seek in debian/onwers" );
		c_exit( 1 );
	}

	if ( flock( fd(CURRENT_FILE), LOCK_SH ) == -1 )
	{
		perror( "Unable to open debian/onwers" );
		c_exit( 1 );
	}


	FILE* file = fdopen( fd(CURRENT_FILE), "r" );

	while ( !feof( file ) )
	{
		if ( fgetc( file ) == '\n' )
		{
			++rules.count;
		}
	}

	rules.rules = calloc( rules.count, sizeof( struct owner_rule ) );

	if ( rules.rules == NULL )
	{
		perror( "Error attempt to allocate rules memory" );
	}

	rewind( file );

	for ( size_t i = 0; i < rules.count; ++i )
	{
		rule(i).path = malloc( 256 );
		fscanf(
			file, "%31s %31s %7s %250[^\n]\n",
			rule(i).user, rule(i).group, rule(i).mask, rule(i).path
		);
		rule(i).path_len = strlen( rule(i).path );
	}

	fclose( file );
	close_fd( pipe(CURRENT_FILE) );
}

void match_rule( char const * const file, size_t const file_len, char const ** user, char const ** group, char const ** mask )
{
	*user  = default_user;
	*group = default_group;
	*mask  = default_mask;

	for ( size_t i = 0; i < rules.count; ++i )
	{
		if ( file_len < rule(i).path_len )
		{
			continue;
		}

		if ( strncmp( file + 1, rule(i).path, rule(i).path_len ) == 0 )
		{
			*user  = rule(i).user;
			*group = rule(i).group;
			*mask  = rule(i).mask;
		}
	}
}

void free_rules()
{
	for ( size_t i = 0; i < rules.count; ++i )
	{
		free( rule(i).path );
	}
	free( rules.rules );
}
