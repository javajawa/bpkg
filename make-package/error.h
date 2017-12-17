#ifndef _BPKG_ERROR_H
#define _BPKG_ERROR_H

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

#define errfs(code,format,...) fprintf( stderr, "[" __FILE__ ":" STR(__LINE__) "] " format "\n", __VA_ARGS__ ); if ( code ) c_exit( code )
#define err(code,mess)         errfs( code, "%s: %s", mess, strerror( errno ) )
#define errs(code,mess)        errfs( code, "%s",     mess )
#define errf(code,format,...)  errfs( code, format ": %s", __VA_ARGS__, strerror( errno ) )

void c_exit( int const status );

#endif
