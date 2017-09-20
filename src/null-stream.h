#ifndef _NULL_STREAM_H
#define _NULL_STREAM_H

#include <unistd.h>

ssize_t read_null_stream( int fd, char * const dest, size_t const maxlen );
size_t write_null_stream( int fd, char const * const src );

#endif
