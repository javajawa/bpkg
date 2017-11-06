#ifndef _TAR_STREAM_H
#define _TAR_STREAM_H

/*
 * The values for typeflag, listing only the values supported
 * by dpkg on unpacking
 */

#define REGTYPE '0' /* Regular file  */
#define LNKTYPE '1' /* Hard link     */
#define SYMTYPE '2' /* Symbolic link */
#define DIRTYPE '5' /* Directory     */

/* Contents of magic field and its length.  */
#define TAR_MAGIC  "ustar"
#define TAR_MAGLEN 6

/* Contents of the version field and its length.  */
#define TAR_VERSION "00"
#define TAR_VERSLEN 2

/* Max length of the username and group fields */
#define TAR_USERLEN 32

/* Max length of the file path, split between 'name' and 'prefix' */
#define TAR_FILELEN 231

/* Size of record blocks in s.tar */
#define TARBLOCKSIZE 512

struct star_header
{                                 /* byte offset */
	char name[100];               /*   0 */
	char mode[8];                 /* 100 */
	char uid[8];                  /* 108 */
	char gid[8];                  /* 116 */
	char size[12];                /* 124 */
	char mtime[12];               /* 136 */
	char chksum[8];               /* 148 */
	char typeflag;                /* 156 */
	char linkname[100];           /* 157 */
	char magic[TAR_MAGLEN];       /* 257 */
	char version[TAR_VERSLEN];    /* 263 */
	char uname[TAR_USERLEN];      /* 265 */
	char gname[TAR_USERLEN];      /* 297 */
	char devmajor[8];             /* 329 */
	char devminor[8];             /* 337 */
	char prefix[131];             /* 345 */
	char atime[12];               /* 476 */
	char ctime[12];               /* 488 */
	char padding[12];             /* 500 */
};                                /* 512 == TARBLOCKSIZE */

#endif
