#ifndef _BPKG_ERROR_H
#define _BPKG_ERROR_H

#ifndef PROGRAM_NAME
#error "No program name define"
#endif

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

#ifdef DEBUG
#define errfs(code,format,...) fprintf( stderr, "[" __FILE__ ":" STR(__LINE__) "] " format "\n", __VA_ARGS__ ); if ( code ) clean_exit( code )
#else
#define errfs(code,format,...) fprintf( stderr, PROGRAM_NAME ": " format "\n", __VA_ARGS__ ); if ( code ) clean_exit( code )
#endif

#define err(code,mess)         errfs( code, "%s: %s", mess, strerror( errno ) )
#define errs(code,mess)        errfs( code, "%s",     mess )
#define errf(code,format,...)  errfs( code, format ": %s", __VA_ARGS__, strerror( errno ) )

void clean_exit( int const status );

#endif
