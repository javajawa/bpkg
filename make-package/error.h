#ifndef _BPKG_ERROR_H
#define _BPKG_ERROR_H

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define err(code,fmt,...) fprintf( stderr, fmt, __VA_ARGS__, strerror( errno ) ); c_exit( code )

void c_exit( int const status );

#endif