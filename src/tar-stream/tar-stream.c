// vim: nospell

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdint.h>   // unint*_t for checksums and verification
#include <sys/file.h> // flock(2)

#include <pwd.h>
#include <grp.h>

#include "error.h"
#include "openat-tools.h"

#include "common/null-stream.h"
#include "common/tar.h"


#define BUFFER_SIZE 4096

#if ( BUFFER_SIZE < TARBLOCKSIZE )
#error "tar-stream buffer size must be equal to or larger than tar's block size"
#endif

static char buffer[BUFFER_SIZE];

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

	while ( 1 )
	{
		buffer_usage = read( fd, buffer, BUFFER_SIZE );

		if ( buffer_usage == -1 )
		{
			err( 0, "Unable to read file" );
			return;
		}

		if ( buffer_usage == 0 )
		{
			break;
		}

		buffer_usage = write( STDOUT_FILENO, buffer, buffer_usage );

		if ( buffer_usage == -1 )
		{
			err( 0, "Unable to write file" );
			return;
		}

		written += buffer_usage;
	}

	if ( written != size )
	{
		errfs( 0, "Size mismatch: wrote %lu of %lu", written, size );
	}

	while ( written % TARBLOCKSIZE != 0 )
	{
		buffer_usage = TARBLOCKSIZE - ( written % TARBLOCKSIZE );
		memset( buffer, 0, buffer_usage );

		written += write( STDOUT_FILENO, buffer, buffer_usage );
	}
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
	if ( file_len > TAR_FILELEN )
	{
		errfs( 0, "RECORD ERROR: filename %s... is too long", file );
		err = 1;
	}

	if ( file_len == 0 )
	{
		errs( 0, "RECORD ERROR: filename is empty" );
		err = 1;
	}

	if ( file[0] == '/' )
	{
		errfs( 0, "RECORD ERROR: path %s is not relative", file );
		err = 1;
	}

	if ( valid_name( file, file_len ) )
	{
		errfs( 0, "RECORD ERROR: File name %s is invalid", file );
		err = 1;
	}

	/* Checks for username validity */
	if ( user_len > TAR_USERLEN )
	{
		errfs( 0, "RECORD ERROR: user name %s... is too long", user );
		err = 1;
	}
	else if ( valid_name( user, user_len ) )
	{
		errfs( 0, "RECORD ERROR: user name %s is invalid", user );
	}

	/* Checks for groupname validity */
	if ( group_len > TAR_USERLEN )
	{
		errfs( 0, "RECORD ERROR: group name %s... is too long", group );
		err = 1;
	}
	else if ( valid_name( group, TAR_USERLEN ) )
	{
		errfs( 0, "RECORD ERROR: Group name %s is invalid", group );
		err = 1;
	}

	/* Checks for mode validity */
	if ( mask_len > 6 )
	{
		errfs( 0, "RECORD ERROR: mask value %s... is too long", mask );
		err = 1;
	}

	sscanf( mask, "%ho", mode );

	if ( *mode > 07777 )
	{
		errfs( 0, "RECORD ERROR: Mask value %ho is invalid", *mode );
		err = 1;
	}

	return err;
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
			errfs( 0, "Unknown file type for inode %s", file );

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
		errfs( 0, "No suitable path seperator in %s", file );

		return -1;
	}

	//fprintf( stderr, "Create mode %05ho %s:%s %s\n", mode, user, group, file );

	// Configure the headers
	strncpy ( header->name,       file + sep,  100 );
	snprintf( header->mode,    8, "%07o",      mode );
	snprintf( header->uid,     8, "%07o",      uid );
	snprintf( header->gid,     8, "%07o",      gid );
	snprintf( header->size,   12, "%011lo",    size );
	snprintf( header->mtime,  12, "%011lo",    stat.stat.st_mtim.tv_sec );
	memcpy  ( header->chksum,     "        ",  8 );
	header->typeflag = file_type;
	strncpy ( header->linkname,   stat.link,   100 );
	strncpy ( header->magic,      TAR_MAGIC,   TAR_MAGLEN );
	memcpy  ( header->version,    TAR_VERSION, TAR_VERSLEN );
	strncpy ( header->uname,      user,        TAR_USERLEN );
	strncpy ( header->gname,      group,       TAR_USERLEN );
	memset  ( header->devmajor,   0,           16 + 131 + 36 );
	strncpy ( header->prefix,     file,        sep ? sep - 1 : 0 );
	snprintf( header->chksum,  8, "%07o",      header_checksum( header ) );

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
		errs( 0, "No context directory defined" );
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
			err( 0, "Error reading input stream" );
			return 3;
		}

		user_len  = read_null_stream( STDIN_FILENO, user,  TAR_USERLEN + 2 );
		group_len = read_null_stream( STDIN_FILENO, group, TAR_USERLEN + 2 );
		mask_len  = read_null_stream( STDIN_FILENO, mask,  8 );

		if ( verify_input_data( file, file_len, user, user_len, group, group_len, mask, mask_len, &mode ) )
		{
			return 3;
		}

		if ( ( fd = openat_stat( context_dir, file, &file_stat ) ) == -1 )
		{
			continue;
		}

		stream_size = make_header(
			(struct star_header *)header,
			file_stat, file, file_len, user, group, mode
		);

		if ( stream_size > -1 )
		{
			file_len = write( STDOUT_FILENO, header, TARBLOCKSIZE );

			if ( file_len == -1 )
			{
				errf( 0, "Error writing file header for %s", file );

				return 4;
			}
			if ( file_len != TARBLOCKSIZE )
			{
				errfs( 0, "Error writing file header for %s", file );

				return 4;
			}

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
