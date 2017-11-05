#ifndef _BPKG_FILE_H
#define _BPKG_FILE_H

#define pipe(X) (struct pipe_id){ X }
#define fd(X) fds[X].fd
#define fdp(X) fd(X.id)

enum pipes
{
	SOURCE_DIR = 0,
	CURRENT_FILE,
	TARSTREAM_INPUT_R,
	TARSTREAM_INPUT_W,
	XZIP_INPUT_R,
	XZIP_INPUT_W,
	AR_INPUT_W,
	AR_INPUT_R,
	DEB_OUTPUT,
	CONTROL_R,
	CONTROL_W,
	PIPES_MAX
};

struct pipe_id
{
	enum pipes id;
};

struct fd
{
	int fd;
};

extern struct sockaddr_un sock;
extern struct fd fds[ PIPES_MAX ];
extern char tmppath[17];
extern char tmpsock[24];


void close_descriptors();
void close_fd( struct pipe_id const offset );
void create_pipes( struct pipe_id const offset );
void create_fifo( struct pipe_id const sink );
void create_file( char const * const path, struct pipe_id offset );

#endif
