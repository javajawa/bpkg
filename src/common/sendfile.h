#ifndef _SENDFILE_H
#define _SENDFILE_H

#include "features.h"

#if __GLIBC__ >= 2

	#define _LIBC_SENDFILE
	#include <sys/sendfile.h>

#else

	#include <sys/types.h>

	ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

#endif

#endif
