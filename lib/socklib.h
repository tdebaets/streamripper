#ifndef __SOCKETS_H__
#define __SOCKETS_H__

#include "types.h"

#ifndef WIN32
typedef int SOCKET;
#endif

#if defined (WIN32)
typedef unsigned int u_int32_t;
#endif

typedef struct HSOCKETst
{
	SOCKET	s;
	BOOL	closed;
} HSOCKET;



extern error_code	socklib_init();
extern error_code	socklib_open(HSOCKET *socket_handle, char *host, int port, char *if_name);
extern void			socklib_close(HSOCKET *socket_handle);
extern void			socklib_cleanup();
extern error_code	socklib_read_header(HSOCKET *socket_handle, char *buffer, int size, 
							int (*recvall)(HSOCKET *sock, char* buffer, int size, int timeout));
extern int			socklib_recvall(HSOCKET *socket_handle, char* buffer, int size, int timeout);
extern int			socklib_sendall(HSOCKET *socket_handle, char* buffer, int size);
extern error_code	socklib_recvall_alloc(HSOCKET *socket_handle, char** buffer, unsigned long *size, 
								int (*recvall)(HSOCKET *sock, char* buffer, int size, int timeout));
extern error_code read_interface(char *if_name, u_int32_t *addr);

#endif	//__SOCKETS_H__
