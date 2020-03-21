#include "openat-tools.h"

#include <sys/file.h> // flock(2)
#include <fcntl.h>    // open(2) and stat(2) and friends
#include <unistd.h>   // readlinkat(2)

#include "error.h"

/**
 * Different libc-s on different operating systems have different
 * flag for the open(2) family of functions.
 *
 * In this code base, we use O_PATH to mean "only open the path/node,
 * not the file contents", and O_SYMLINK as "operate directly on a
 * symlink".
 *
 * On Linux, with O_PATH fills both of these purposes.
 * Some systems do not have the path-only flag.
 */
#ifndef O_PATH
#define O_PATH 0
#endif

#ifndef O_SYMLINK
#define O_SYMLINK O_PATH
#endif

int open_context_dir( char const * const dir )
{
	int context_dir;

	context_dir = open( dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_PATH | O_NOCTTY );

	if ( context_dir == -1 )
	{
		switch ( errno )
		{
			case EPERM:
				errfs( 0, "Not permitted to open %s", dir );
				return EPERM;

			case ELOOP:
				errfs( 0, "Refusing to operate on symlink directory %s", dir );
				return ELOOP;

			case ENOENT:
				errfs( 0, "Unable to find context directory %s", dir );
				return ENOENT;

			default:
				errf( 0, "Error opening %s", dir );
		}
	}

	return context_dir;
}

/**
 * Open a specified file, and run fstat(2) on the opened file descriptor.
 * The file is opened with read-only and with O_NOFOLLOW in all cases.
 * If the file is a symlink, the equivalent of lstat(2) is returns in the
 * stat argument's stat field, and the target path in the link field.
 * For all other file types, the link field will be an empty string.
 *
 * - ctx:   context directory file descriptor, like openat(2)
 * - path:  path relative to ctx to open
 * - stat:  [out] the stat(2) block
 * - flags: extra open(2) flags. Used for a self-call for handling symlinks.
 *
 * Returns the opened file descriptor number, or -1 if an error occured.
 */
int openat_stat_impl( int ctx, char const * const path, struct estat * const stat, int flags )
{
	int fd, code;

	// Attempt to open the file
	fd = openat( ctx, path, O_RDONLY | O_NOCTTY | O_NOFOLLOW | flags );

	if ( fd == -1 )
	{
		code = errno;

		switch ( code )
		{
			// If the file is a symlink, ELOOP is given when O_NOFOLLOW
			// is specified but O_PATH (linux) or O_SYMLINK (*bsd) is not.
			// However, adding O_PATH prevents reading the contents.
			// This is solves with a self call with the extra flag.
			case ELOOP:
				return openat_stat_impl( ctx, path, stat, flags | O_SYMLINK );

			case EACCES:
				errfs( 0, "Access denied to %s", path );
				break;

			case ENOENT:
				errfs( 0, "%s does not exist", path );
				break;

			default:
				errf( 0, "Unable to read file %s", path );
		}

		return -1;
	}

	// Attempt to lock files where possible.
	// If we have used O_PATH in the flag set, we can not lock the file.
	if ( ( flags &~ O_PATH ) && flock( fd, LOCK_SH ) )
	{
		errf( 0, "Unable to lock file %s", path );
		close( fd );

		return -1;
	}

	// Get stat info about the file (descriptor)
	code = fstat( fd, &stat->stat );

	if ( code == -1 )
	{
		errf( 0, "Unable to stat file %s", path );
		close( fd );

		return -1;
	}

	// Check that symlinks meet the requirement of a maximum
	// of 100 characters in the target value.
	// This is a limitation of the ustar format -- see src/common/tar.h.
	if ( ( stat->stat.st_mode & S_IFMT ) == S_IFLNK )
	{
		// code here is the number of bytes read.
		code = readlinkat( ctx, path, stat->link, 101 );

		if ( code == 101 )
		{
			errfs( 0, "Link %s is over 100 characters", path );
			close( fd );

			return -1;
		}

		// Ensure the link propery is null-terminated.
		stat->link[code] = 0;
	}
	else
	{
		// Truncate the link property, as this extended-stat
		// object may have been re-used from a previous link.
		stat->link[0] = 0;
	}

	return fd;
}

int openat_stat( int ctx, char const * const path, struct estat * const stat )
{
	return openat_stat_impl( ctx, path, stat, 0 );
}
