#ifndef __SOCKETS_H__
#define __SOCKETS_H__

#include "types.h"

#ifndef WIN32
	typedef int SOCKET;
#endif
	
typedef struct HSOCKETst
{
	SOCKET	s;
	BOOL	closed;
} HSOCKET;


extern error_code	socklib_init();
extern error_code	socklib_open(HSOCKET *socket_handle, char *host, int port);
extern void			socklib_close(HSOCKET *socket_handle);
extern void			socklib_cleanup();
extern error_code	socklib_read_header(HSOCKET *socket_handle, char *buffer, int size, 
							int (*recvall)(HSOCKET *sock, char* buffer, int size));
extern int			socklib_recvall(HSOCKET *socket_handle, char* buffer, int size);
extern int			socklib_sendall(HSOCKET *socket_handle, char* buffer, int size);
extern error_code	socklib_recvall_alloc(HSOCKET *socket_handle, char** buffer, unsigned long *size, 
								int (*recvall)(HSOCKET *sock, char* buffer, int size));

#endif	//__SOCKETS_H__
