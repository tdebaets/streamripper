/* sockets.c - jonclegg@yahoo.com
 * library routines for handling socket stuff
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#if WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#if __UNIX__
#include <arpa/inet.h>
#elif __BEOS__
#include <be/net/netdb.h>   
#endif

#include "types.h"
#include "util.h"
#include "socklib.h"
#include "threadlib.h"
#include "debug.h"

#if WIN32
#define DEFAULT_TIMEOUT		15
#define FIRST_READ_TIMEOUT	(30 * 1000)
#elif __UNIX__
#define closesocket(socket)	close(socket)
#define DEFAULT_TIMEOUT		15
#define FIRST_READ_TIMEOUT	30
#define Sleep				usleep
#elif __BEOS__
#define DEFAULT_TIMEOUT		15
#define FIRST_READ_TIMEOUT	30
#define Sleep(x)			snooze(x*100)
#define INADDR_NONE			-1	// why don't they have this???
#endif

#ifndef WIN32
#define WSACleanup()
#define SOCKET_ERROR		-1
#endif

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	socklib_init();
error_code	socklib_open(HSOCKET *socket_handle, char *host, int port);
void		socklib_close(HSOCKET *socket_handle);
void		socklib_cleanup();
error_code	socklib_read_header(HSOCKET *socket_handle, char *buffer, int size, 
								int (*recvall)(HSOCKET *socket_handle, char* buffer, int size));
int			socklib_recvall(HSOCKET *socket_handle, char* buffer, int size);
int			socklib_sendall(HSOCKET *socket_handle, char* buffer, int size);
error_code	socklib_recvall_alloc(HSOCKET *socket_handle, char** buffer, unsigned long *size, 
								int (*recvall)(HSOCKET *socket_handle, char* buffer, int size));

/*********************************************************************************
 * Private Vars 
 *********************************************************************************/
static BOOL		m_done_init = FALSE;	// so we don't init the mutex twice.. arg.

/*
 * Init socket stuff
 */
error_code socklib_init()
{
#if WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
#endif

	if (m_done_init)
		return SR_SUCCESS;

#if WIN32
    wVersionRequested = MAKEWORD( 2, 2 );
    err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 )
        return SR_ERROR_WIN32_INIT_FAILURE;
#endif

	m_done_init = TRUE;

	return SR_SUCCESS;
}


/*
 * open's a tcp connection to host at port, host can be a dns name or IP,
 * socket_handle gets assigned to the handle for the connection
 */

error_code socklib_open(HSOCKET *socket_handle, char *host, int port)
{
    struct sockaddr_in address;
    struct hostent *hp;
    int len;

    socket_handle->s = socket(AF_INET, SOCK_STREAM, 0);

	if ((address.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	{
		hp = gethostbyname(host);
		if (hp)
			memcpy(&address.sin_addr, hp->h_addr_list[0], hp->h_length);
		else
		{
			WSACleanup();
			return SR_ERROR_CANT_RESOLVE_HOSTNAME;
		}
	}
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    len = sizeof(address);

    if (connect(socket_handle->s, (struct sockaddr *)&address, len) == SOCKET_ERROR)
		return SR_ERROR_CONNECT_FAILED;

#ifdef WIN32
	{
    struct timeval timeout = {DEFAULT_TIMEOUT*1000, 0};
	if (setsockopt(socket_handle->s, SOL_SOCKET,  SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
		return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
	}
#endif

	socket_handle->closed = FALSE;
    return SR_SUCCESS;
}

void socklib_cleanup()
{
	WSACleanup();
	m_done_init = FALSE;
}

void socklib_close(HSOCKET *socket_handle)
{
	DEBUG1(("socklib: closeing socket"));
	closesocket(socket_handle->s);
	socket_handle->closed = TRUE;
}

error_code	socklib_read_header(HSOCKET *socket_handle, char *buffer, int size, 
								int (*recvall)(HSOCKET *socket_handle, char* buffer, int size))
{
	int i;
#ifdef WIN32
    struct timeval timeout = {FIRST_READ_TIMEOUT, 0};
#endif
	int ret;
	char *t;
	int (*myrecv)(HSOCKET *socket_handle, char* buffer, int size);

	if (socket_handle->closed)
		return SR_ERROR_SOCKET_CLOSED;

	if (recvall)
		myrecv = recvall;
	else
		myrecv = socklib_recvall;
#ifdef WIN32
	if (setsockopt(socket_handle->s, SOL_SOCKET,  SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
		return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
#endif

	memset(buffer, -1, size);
	for(i = 0; i < size; i++)
	{
		if ((ret = (*myrecv)(socket_handle, &buffer[i], 1)) == SOCKET_ERROR)
			return SR_ERROR_RECV_FAILED;
		
		if (ret == 0)
			return SR_ERROR_NO_HTTP_HEADER;

		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;

		//look for the end of the icy-header
		t = buffer + (i > 3 ? i - 3: i);

		if (strncmp(t, "\r\n\r\n", 4) == 0)
			break;

		if (strncmp(t, "\n\0\r\n", 4) == 0)		// wtf? live365 seems to do this
			break;

	}
	
	if (i == size)
		return SR_ERROR_NO_HTTP_HEADER;

	buffer[i] = '\0';

#ifdef WIN32
	timeout.tv_sec = DEFAULT_TIMEOUT*1000;
	if (setsockopt(socket_handle->s, SOL_SOCKET,  SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
		return SR_ERROR_CANT_SET_SOCKET_OPTIONS;
#endif

	return SR_SUCCESS;
}


/*
 * Default recv
 */
int socklib_recvall(HSOCKET *socket_handle, char* buffer, int size)
{
    int ret = 0, read = 0;
    while(size)
    {
		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;
	
		DEBUG2(("calling recv for %d bytes", size));
        ret = recv(socket_handle->s, &buffer[read], size, 0);
		DEBUG2(("recv: %d", ret));
        if (ret == SOCKET_ERROR)
			return SOCKET_ERROR;
				
		if (ret == 0)
			break;

		read += ret;
		size -= ret;
    }

    return read;
}

int socklib_sendall(HSOCKET *socket_handle, char* buffer, int size)
{
    int ret = 0,
	sent = 0;

    while(size)
    {
		if (socket_handle->closed)
			return SR_ERROR_SOCKET_CLOSED;

		DEBUG2(("calling send for %d bytes", size));
        ret = send(socket_handle->s, &buffer[sent], size, 0);
		DEBUG2(("send ret = %d", ret));
        if (ret == SOCKET_ERROR)
			return SOCKET_ERROR;
		
		if (ret == 0)
			break;

		sent += ret;
		size -= ret;
    }

    return sent;
	
}

error_code	socklib_recvall_alloc(HSOCKET *socket_handle, char** buffer, unsigned long *size, 
								int (*recvall)(HSOCKET *socket_handle, char* buffer, int size))
{
	const int CHUNK_SIZE = 8192;
	int (*myrecv)(HSOCKET *socket_handle, char* buffer, int size);
	int ret;
	unsigned long mysize = 0;
	int chunks = 1;
	char *temp = (char *)malloc(CHUNK_SIZE);

	if (socket_handle->closed)
		return SR_ERROR_SOCKET_CLOSED;

	if (!size)
		return SR_ERROR_INVALID_PARAM;

	if (recvall)
		myrecv = recvall;
	else
		myrecv = socklib_recvall;

	while((ret = myrecv(socket_handle, &temp[mysize], (CHUNK_SIZE*chunks)-mysize)) >= 0)
	{
		mysize += ret;

		if (ret == 0)
			break;

		if ((mysize % CHUNK_SIZE) == 0)
		{
			chunks++;
			temp = (char *)realloc(temp, CHUNK_SIZE*chunks);
		}
	}
	if (mysize == 0)
		return SR_ERROR_RECV_FAILED;

	temp[mysize] = '\0';
	*size = mysize;
	*buffer = temp;
	return SR_SUCCESS;
}
