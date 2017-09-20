// vim: nospell

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <limits.h>
#include <fcntl.h>

#include <assert.h>
#include <errno.h>

#include <pwd.h>
#include <grp.h>

#include "null-stream.h"
#include "tar-stream.h"

#define BUFFER_SIZE 4096

static char buffer[BUFFER_SIZE];

struct estat
{
	struct stat stat;
	char link[100];
};

size_t valid_name( char const * const name, size_t const len )
{
	for ( size_t i = 0; i < len; ++i )
	{
		if ( name[ i ] == 0 )
		{
			return 0;
		}

		if ( name[i] < 32 )
		{
			return i;
		}

#if CHAR_MAX == 255
		if ( name[i] > 127 )
		{
			return i;
		}
#endif
	}

	return 0;
}

uint32_t header_checksum( struct star_header const * const header )
{
	unsigned char const * chk_pointer = (unsigned char *)header;
	uint32_t checksum = 0;

	for ( size_t i = 0; i < TARBLOCKSIZE; ++i )
	{
		checksum += chk_pointer[i];
	}

	return checksum;
}

uid_t name_to_uid( char const * const name )
{
	long const buflen = sysconf( _SC_GETPW_R_SIZE_MAX );
	char buf[buflen];
	struct passwd pwbuf, *pwbufp;

	if ( ! name )
	{
		return -1;
	}

	if ( buflen == -1 )
	{
		return -1;
	}

	if ( 0 != getpwnam_r( name, &pwbuf, buf, buflen, &pwbufp ) || !pwbufp )
	{
		return -1;
	}

	return pwbufp->pw_uid;
}

uid_t name_to_gid( char const * const name )
{
	long const buflen = sysconf( _SC_GETGR_R_SIZE_MAX );
	char buf[buflen];
	struct group grbuf, *grbufp;

	if ( ! name )
	{
		return -1;
	}

	if ( buflen == -1 )
	{
		return -1;
	}

	if ( 0 != getgrnam_r( name, &grbuf, buf, buflen, &grbufp ) || !grbufp )
	{
		return -1;
	}

	return grbufp->gr_gid;
}

void write_tar_stream( int const fd, ssize_t const size )
{
	ssize_t written = 0;
	ssize_t buffer_usage;

	//written = splice( fd, NULL, STDOUT_FILENO, NULL, size, 0 );
	while ( 1 )
	{
		buffer_usage = read( fd, buffer, BUFFER_SIZE );

		if ( buffer_usage == -1 )
		{
			fprintf( stderr, "Unable to read file: %s\n", strerror( errno ) );
			return;
		}

		if ( buffer_usage == 0 )
		{
			break;
		}

		buffer_usage = write( STDOUT_FILENO, buffer, buffer_usage );

		if ( buffer_usage == -1 )
		{
			fprintf( stderr, "Unable to write file: %s\n", strerror( errno ) );
			return;
		}

		written += buffer_usage;
	}

	if ( written != size )
	{
		fprintf( stderr, "Size mismatch: wrote %lu of %lu\n", written, size );
	}

	// TODO: reuse a buffer for this
	while ( written % TARBLOCKSIZE != 0 )
	{
		written += write( STDOUT_FILENO, "\0", 1 );
	}
}

int open_context_dir( char const * const dir )
{
	int context_dir;

	context_dir = open( dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_PATH | O_NOCTTY );

	if ( context_dir == -1 )
	{
		switch ( errno )
		{
			case EPERM:
				fprintf( stderr, "Not permitted to open %s\n", dir );
				return EPERM;

			case ELOOP:
				fprintf( stderr, "Refusing to operate on symlink directory %s\n", dir );
				return ELOOP;

			case ENOENT:
				fprintf( stderr, "Unable to find context directory %s\n", dir );
				return ENOENT;

			default:
				fprintf( stderr, "Error opening %s: %s\n", dir, strerror( errno ) );
		}
	}

	return context_dir;
}

uint8_t verify_input_data(
	const char * const file,  const size_t file_len,
	const char * const user,  const size_t user_len,
	const char * const group, const size_t group_len,
	const char * const mask,  const size_t mask_len, uint16_t * mode
)
{
	uint8_t err = 0;

	/* Checks for filename validity */

	if ( file_len > TFILELEN )
	{
		fprintf( stderr, "RECORD ERROR: filename %s... is too long\n", file );

		return 1;
	}

	if ( file_len == 0 )
	{
		fprintf( stderr, "RECORD ERROR: filename is empty\n" );

		return 1;
	}

	if ( file[0] == '/' )
	{
		fprintf( stderr, "RECORD ERROR: path %s is not relative\n", file );
		err = 1;
	}

	if ( valid_name( file, file_len ) )
	{
		fprintf( stderr, "RECORD ERROR: File name %s is invalid\n", file );
		err = 1;
	}

	/* Checks for username validity */

	if ( user_len > TUSERLEN )
	{
		fprintf( stderr, "RECORD ERROR: user name %s... is too long\n", user );

		return 1;
	}

	if ( valid_name( user, user_len ) )
	{
		fprintf( stderr, "RECORD ERROR: user name %s is invalid\n", user );
	}

	/* Checks for groupname validity */

	if ( group_len > TUSERLEN )
	{
		fprintf( stderr, "RECORD ERROR: group name %s... is too long\n", group );

		return 1;
	}

	if ( valid_name( group, TUSERLEN ) )
	{
		fprintf( stderr, "RECORD ERROR: Group name %s is invalid\n", group );
	}

	/* Checks for mode validity */

	if ( mask_len > 6 )
	{
		fprintf( stderr, "RECORD ERROR: mask value %s... is too long\n", mask );

		return 1;
	}

	sscanf( mask, "%ho", mode );

	if ( *mode > 07777 )
	{
		fprintf( stderr, "RECORD ERROR: Mask value %ho is invalid\n", *mode );
	}

	return err;
}

int openat_stat( int ctx, char const * const path, struct estat * const stat, int flags )
{
	int fd, code;

	// Attempt to open the file
	fd = openat( ctx, path, O_RDONLY | O_NOCTTY | O_NOFOLLOW | flags );

	if ( fd == -1 )
	{
		code = errno;

		switch ( code )
		{
			case ELOOP:
				return openat_stat( ctx, path, stat, O_PATH );

			case EACCES:
				fprintf( stderr, "Access denied to %s\n", path );
				break;

			case ENOENT:
				fprintf( stderr, "%s does not exist\n", path );
				break;

			default:
				fprintf( stderr, "Unable to read file %s -- err %d\n", path, code );
		}

		return 0;
	}


	if ( ( flags &~ O_PATH ) && flock( fd, LOCK_SH ) )
	{
		fprintf( stderr, "Unable to lock file %s -- err %d\n", path, errno );
		close( fd );

		return 0;
	}

	// Get stat info about the file
	code = fstat( fd, &stat->stat );

	if ( ( stat->stat.st_mode & S_IFMT ) == S_IFLNK )
	{
		code = readlinkat( ctx, path, stat->link, 101 );

		if ( code == 101 )
		{
			fprintf( stderr, "Link %s is over 100 characters\n", path );
			close( fd );

			return 0;
		}

		stat->link[code] = 0;
	}
	else
	{
		stat->link[0] = 0;
	}

	if ( code == -1 )
	{
		code = errno;

		fprintf( stderr, "Unable to stat file %s -- err %d\n", path, code );
		close( fd );

		return 0;
	}

	return fd;
}

ssize_t make_header( struct star_header * const header, struct estat const stat, char const * const file, size_t const file_len, char const * const user, char const * const group, uint16_t const mask )
{
	uid_t    uid, gid;
	size_t   size;
	uint16_t mode;
	char     file_type;

	// Get the file type
	switch ( stat.stat.st_mode & S_IFMT )
	{
		case S_IFREG:
			file_type = REGTYPE;
			size = stat.stat.st_size;
			break;

		case S_IFDIR:
			file_type = DIRTYPE;
			size = 0;
			break;

		case S_IFLNK:
			file_type = SYMTYPE;
			size = 0;
			break;

		default:
			fprintf( stderr, "Unknown file type for inode %s\n", file );

			return -1;
	}

	uid = name_to_uid( user );
	if ( uid == 1 || uid > 41 )
	{
		uid = 0;
	}

	gid = name_to_gid( group );
	if ( gid == 1 || gid > 41 )
	{
		gid = 0;
	}

	// Calculate file mask (as supplied on the CLI, honouring only exec bit)
	mode = mask;

	if ( ( stat.stat.st_mode & 0111 ) == 0 )
	{
		mode &= 07666;
	}

	size_t sep = 0;

	if ( file_len > 100 )
	{
		sep = 255;

		for ( size_t i = file_len; i >= file_len - 100; --i )
		{
			if ( file[i] == '/' )
			{
				sep = i + 1;
			}
		}
	}

	if ( sep > 131 )
	{
		fprintf( stderr, "No suitable path seperator in %s\n", file );

		return -1;
	}

	//fprintf( stderr, "Create mode %05ho %s:%s %s\n", mode, user, group, file );

	// Configure the headers
	strncpy ( header->name,       file + sep, 100 );
	snprintf( header->mode,    8, "%07o",    mode );
	snprintf( header->uid,     8, "%07o",    uid );
	snprintf( header->gid,     8, "%07o",    gid );
	snprintf( header->size,   12, "%011lo",  size );
	snprintf( header->mtime,  12, "%011lo",  stat.stat.st_mtim.tv_sec );
	strncpy ( header->chksum,     TCHKSUM,   8 );
	header->typeflag = file_type;
	strncpy ( header->linkname,   stat.link, 100 );
	strncpy ( header->magic,      TMAGIC,    TMAGLEN );
	strncpy ( header->version,    TVERSION,  TVERSLEN );
	strncpy ( header->uname,      user,      TUSERLEN );
	strncpy ( header->gname,      group,     TUSERLEN );
	memset  ( header->devmajor,   0,         16 + 131 + 36 );
	strncpy ( header->prefix,     file,      sep ? sep - 1 : 0 );

	snprintf( header->chksum,  8, "%07o",   header_checksum( header ) );

#ifdef HEADER_DEBUG
	fputc_unlocked( '\n', stderr );
	fputc_unlocked( '-', stderr );
	fputc_unlocked( '-', stderr );
	fputc_unlocked( '\n', stderr );

	for ( size_t i = 0; i < TARBLOCKSIZE; ++i )
	{
		if ( i == 100 || i == 157 || i == 257 || i == 329 )
		{
			fputc_unlocked( '\n', stderr );
		}

		if ( ((char*)header)[ i ] == 0 )
		{
			fputc_unlocked( ' ', stderr );
		}
		else
		{
			fputc_unlocked( ((char*)header)[ i ], stderr );
		}
	}

	fputc_unlocked( '\n', stderr );
#endif

	return size;
}

int main( int argc, char** argv )
{
	char * file  = buffer;
	char * user  = &(buffer[256]);
	char * group = &(buffer[320]);
	char * mask  = &(buffer[384]);

	void * header = &(buffer[1024]);

	ssize_t file_len, user_len, group_len, mask_len, stream_size;

	uint16_t mode;

	struct estat file_stat;

	int context_dir, fd;

	if ( argc != 2 )
	{
		fprintf( stderr, "No context directory defined\n" );
		return 2;
	}

	context_dir = open_context_dir( argv[1] );

	if ( context_dir == -1 )
	{
		return 2;
	}

	while ( 1 )
	{
		// Read in the next file to store in the archive
		file_len  = read_null_stream( STDIN_FILENO, file, 230 );

		// file_len indicates EOF
		if ( file_len == -1 )
		{
			break;
		}

		if ( file_len < -1 )
		{
			fprintf( stderr, "Error reading input stream: %s\n", strerror( -file_len ) );
			return 3;
		}

		user_len  = read_null_stream( STDIN_FILENO, user,  TUSERLEN + 2 );
		group_len = read_null_stream( STDIN_FILENO, group, TUSERLEN + 2 );
		mask_len  = read_null_stream( STDIN_FILENO, mask, 8 );

		if ( verify_input_data( file, file_len, user, user_len, group, group_len, mask, mask_len, &mode ) )
		{
			return 3;
		}

		if ( ( fd = openat_stat( context_dir, file, &file_stat, 0 ) ) == 0 )
		{
			continue;
		}

		stream_size = make_header(
			(struct star_header *)header,
			file_stat, file, file_len, user, group, mode
		);

		if ( stream_size > -1 )
		{
			write( STDOUT_FILENO, header, TARBLOCKSIZE );

			if ( stream_size > 0 )
			{
				write_tar_stream( fd, stream_size );
			}
		}

		flock( fd, LOCK_UN );
		close( fd );
	}

	return 0;
}
