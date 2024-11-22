#ifndef _BPKG_CHECKSUMS_H
#define _BPKG_CHECKSUMS_H

// vim: nospell

#include <stddef.h>

// Defines the two checksum commands.
//
// The command _COMMAND is is in execv(3) format, a list of strings
// with the first one being the program name, and a NULL terminator.
//
// This program will be run, and the input passed via standard input.
// When the package is finished, standard input will be closed, and the
// first _SIZE bytes of the output is taken as the checksum.
//
// The _NAME field is the field name in the outputted control file.

// Precisely two checksums are used; generally these represent the
// current compatiable with everyone version, and the aspirational version.

#define CHK1_NAME "SHA256"
#define CHK1_SIZE 64
static char * const CHK1_COMMAND[] = {"sha256sum", NULL};

#define CHK2_NAME "SHA512"
#define CHK2_SIZE 128
static char * const CHK2_COMMAND[] = {"sha512sum", NULL};

struct package_data
{
	size_t packed_size;
	char chk1[CHK1_SIZE + 1];
	char chk2[CHK2_SIZE + 1];
};

#endif
