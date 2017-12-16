#ifndef _BPKG_CHECKSUMS_H
#define _BPKG_CHECKSUMS_H

#include <stddef.h>

// vim: nospell

// Define two checksum commands
// The command _COMMAND is run with no arguments,
// and the first _SIZE bytes of the output is taken as
// the checksum.

#define CHK1_COMMAND "sha256sum"
#define CHK1_NAME    "SHA256"
#define CHK1_SIZE    64

#define CHK2_COMMAND "sha512sum"
#define CHK2_NAME    "SHA512"
#define CHK2_SIZE    128

struct package_data
{
	size_t packed_size;
	size_t installed_size;
	char chk1[CHK1_SIZE + 1];
	char chk2[CHK2_SIZE + 1];
};

#endif
