#ifndef _BPKG_WRITE_CONTROL_H
#define _BPKG_WRITE_CONTROL_H

#include "checksums.h"

#include <stddef.h>

struct package_data
{
	size_t packed_size;
	size_t installed_size;
	char chk1[CHK1_SIZE + 1];
	char chk2[CHK2_SIZE + 1];
};

void write_control( char * out, struct package_data const );

#endif
