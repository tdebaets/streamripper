/* relaylib.c - jonclegg@yahoo.com
 * little relay server
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

#if WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#endif

#if __UNIX__
#include <arpa/inet.h>
#elif __BEOS__
#include <be/net/netdb.h>   
#endif

#include <string.h>
#include "util.h"
#include "relaylib.h"
#include "types.h"
#include "http.h"
#include "socklib.h"
#include "threadlib.h"


#ifdef __UNIX__
#define Sleep		usleep
#define closesocket	close
#define SOCKET_ERROR	-1
#define WSAGetLastError() errno
#elif __BEOS__
#define Sleep(x)	snooze(x*100)
#define SOCKET_ERROR	-1
#define WSAGetLastError() errno
#define IPPROTO_IP	IPPROTO_TCP
#elif WIN32
#define EAGAIN		WSAEWOULDBLOCK
#define EWOULDBLOCK	WSAEWOULDBLOCK
#endif

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	relaylib_init(BOOL search_ports, int base_port, int max_port, int *port_used);
void		relaylib_shutdown();
error_code	relaylib_set_response_header(char *http_header);
error_code	relaylib_start();
error_code	relaylib_send(char *data, int len);
BOOL		relaylib_isrunning();

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static void			thread_accept(void *notused);
static error_code	try_port(u_short port);


/*********************************************************************************
 * Private vars 
 *********************************************************************************/
static BOOL		m_connected = FALSE;
static char		m_http_header[MAX_HEADER_LEN];
static SOCKET	m_hostsock = 0;
static SOCKET	m_listensock = 0;
static BOOL		m_running = FALSE;
static THREAD_HANDLE	m_hthread;

BOOL relaylib_isrunning()
{
	return m_running;
}


error_code	relaylib_set_response_header(char *http_header)
{
	if (!http_header)
		return SR_ERROR_INVALID_PARAM;

	strcpy(m_http_header, http_header);

	return SR_SUCCESS;
}

#ifndef WIN32
void catch_pipe(int code)
{
	m_hostsock = 0;
	m_connected = FALSE;
}
#endif

error_code relaylib_init(BOOL search_ports, int relay_port, int max_port, int *port_used)
{
	int ret;
#ifdef WIN32
	WSADATA wsd;

	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
		return SR_ERROR_CANT_BIND_ON_PORT;
#endif

	if (relay_port < 1 || !port_used)
		return SR_ERROR_INVALID_PARAM;

#ifndef WIN32
	// catch a SIGPIPE if send fails
	signal(SIGPIPE, catch_pipe);
#endif

	*port_used = 0;
	if (!search_ports)
		max_port = relay_port;

	for(;relay_port <= max_port; relay_port++)
	{
		if ((ret = try_port((u_short)relay_port)) == SR_ERROR_CANT_BIND_ON_PORT)
			continue;		// Keep searching.

		if (ret == SR_SUCCESS)
		{
			*port_used = relay_port;
			return SR_SUCCESS;
		}
		else
			return ret;
	}

	return SR_ERROR_CANT_BIND_ON_PORT;
}

error_code try_port(u_short port)
{
	struct sockaddr_in local;
	int ret;

	m_connected = FALSE;


	m_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (m_listensock == SOCKET_ERROR)
		return SR_ERROR_SOCK_BASE;
	
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	
	if ((ret = bind(m_listensock, (struct sockaddr *)&local, sizeof(local))) == SOCKET_ERROR)
	{
		closesocket(m_listensock);
		return SR_ERROR_CANT_BIND_ON_PORT;
	}
	
	if (listen(m_listensock, 1) == SOCKET_ERROR)
	{
		closesocket(m_listensock);
		return SR_ERROR_SOCK_BASE;
	}

	return SR_SUCCESS;
}


void relaylib_shutdown()
{
	if (!relaylib_isrunning())
		return;
debug_printf("in relaylib_shutdown()\n");
	m_running = FALSE;
debug_printf("1\n");
	m_connected = FALSE;
	threadlib_stop(&m_hthread);
debug_printf("2\n");
	closesocket(m_listensock);
	closesocket(m_hostsock);
debug_printf("3\n");
	memset(m_http_header, 0, MAX_HEADER_LEN);
debug_printf("waiting for close()\n");
	threadlib_waitforclose(&m_hthread);
debug_printf("done with()\n");
}


error_code relaylib_start()
{
	int ret;

	// Spawn on a thread so it's non-blocking
	if ((ret = threadlib_beginthread(&m_hthread, thread_accept)) != SR_SUCCESS)
		return ret;
	return SR_SUCCESS;
}

void thread_accept(void *notused)
{
	int ret;
	struct sockaddr_in client;
	int iAddrSize = sizeof(client);
	char headbuf;

	// Set listen and host sock to non blocking
	#ifdef WIN32
		{
		unsigned long lame = 1;
		ioctlsocket(m_hostsock, FIONBIO, &lame);
		ioctlsocket(m_listensock, FIONBIO, &lame);
		}
	#else
		fcntl(m_hostsock, F_SETFL, O_NONBLOCK);
		fcntl(m_listensock, F_SETFL, O_NONBLOCK);
	#endif


	m_running = TRUE;
	while(m_running)
	{

		// if we're connected we don't want to accept
		while(m_connected)
			Sleep(10);

		m_hostsock = accept(m_listensock, (struct sockaddr *)&client, &iAddrSize);
		if (m_hostsock == SOCKET_ERROR)
		{
			if (WSAGetLastError() == EWOULDBLOCK)
			{
				Sleep(10);
				continue;
			}
			break;
		}

		// Wait for some data
		ret = 0;
		while(!ret)
		{
			fd_set set;
			struct timeval t;
			t.tv_sec = t.tv_usec = 0;
			FD_ZERO(&set); FD_SET(m_hostsock, &set);
			ret = select(m_hostsock+1, &set, NULL, NULL, &t);
		}
		recv(m_hostsock, &headbuf, 1, 0); 
		ret = send(m_hostsock, m_http_header, strlen(m_http_header), 0);
		if (ret >= 0)
		{
			m_connected = TRUE;
		}
	}
	threadlib_close(&m_hthread);
	m_running = FALSE;
	return;
}

error_code relaylib_send(char *data, int len)
{
	int ret;

	if (!m_connected)
		return SR_ERROR_HOST_NOT_CONNECTED;

	for(;;)
	{
		ret = send(m_hostsock, data, len, 0);
		if (ret == -1 && WSAGetLastError() == EAGAIN)
		{
			Sleep(100);
			continue;
		}
		break;
	}

	if (ret < 0)
	{
		m_hostsock = 0;
		m_connected = FALSE;
		return SR_ERROR_HOST_NOT_CONNECTED;
	}
	return SR_SUCCESS;
}
