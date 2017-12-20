#ifndef _BPKG_PIPEFORK_H
#define _BPKG_PIPEFORK_H

#include <unistd.h>
#include "files.h"

pid_t pipe_fork_exec( char * const * argv, struct pipe_id const in, struct pipe_id const out );

#endif
