#ifndef _TAR_STREAM_H
#define _TAR_STREAM_H

/********************************************************************
 * This following section is lifted from glibc's tar.h,
 * and is embedded here to avoid issues with difference with
 * other tar.h versions.
 *
 * Copyright (C) 1992-2016 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 * Written by David J. MacKenzie.
 * Licensed under GPL 2.1 (or later)
 */

/*
 * The values for typeflag:
 * Values 'A'-'Z' are reserved for custom implementations.
 * All other values are reserved for future POSIX.1 revisions.
 */

#define REGTYPE '0' /* Regular file (preferred code).  */
#define LNKTYPE '1' /* Hard link.  */
#define SYMTYPE '2' /* Symbolic link (hard if not supported).  */
#define DIRTYPE '5' /* Directory.  */

/* Contents of magic field and its length.  */
#define TMAGIC  "ustar"
#define TMAGLEN 6

/* Contents of the version field and its length.  */
#define TVERSION "00"
#define TVERSLEN 2

/*******************************************************************/

/* Max length of the username and group fields */
#define TUSERLEN 32

/* Max length of the file path */
#define TFILELEN 231

/* Size of record blocks in star */
#define TARBLOCKSIZE 512

/* Initial value for the checksum */
#define TCHKSUM  "          "

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
	char magic[TMAGLEN];          /* 257 */
	char version[TVERSLEN];       /* 263 */
	char uname[TUSERLEN];         /* 265 */
	char gname[TUSERLEN];         /* 297 */
	char devmajor[8];             /* 329 */
	char devminor[8];             /* 337 */
	char prefix[131];             /* 345 */
	char atime[12];               /* 476 */
	char ctime[12];               /* 488 */
	char padding[12];             /* 500 */
};                                /* 512 == TARBLOCKSIZE */

#endif
