#ifndef _BPKG_OPENAT_H
#define _BPKG_OPENAT_H

#include <sys/stat.h>

/**
 * Extended stat structure that contains a symbolic link of up to 100 characters.
 * 100 is used over PATH_LENGTH as it is the limit of the us-tar format.
 */
struct estat
{
	struct stat stat;
	char link[100];
};

int open_context_dir( char const * const dir );

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
 *
 * Returns the opened file descriptor number, or -1 if an error occured.
 */
int openat_stat( int ctx, char const * const path, struct estat * const stat );

#endif
