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
#include "debug.h"


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
static HEVENT	m_event_not_connected = NULL;
static char		m_http_header[MAX_HEADER_LEN];
static SOCKET	m_hostsock = 0;
static SOCKET	m_listensock = 0;
static BOOL		m_running = FALSE;
static THREAD_HANDLE m_hthread;

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

	m_event_not_connected = threadlib_create_event();
	if (m_event_not_connected == NULL)
		return SR_ERROR_CANT_CREATE_EVENT;
	//
	// NOTE: we need to signel it here in case we try to destroy
	// relaylib before the thread starts!
	//
	threadlib_signel_event(&m_event_not_connected);

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

	m_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (m_listensock == SOCKET_ERROR)
		return SR_ERROR_SOCK_BASE;
	
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	
	if (bind(m_listensock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
	{
		int x = WSAGetLastError();
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
	DEBUG1(("relaylib_shutdown:start"));
	if (!relaylib_isrunning())
	{
		DEBUG1(("***relaylib_shutdown:return"));
		return;
	}
	m_running = FALSE;
	threadlib_signel_event(&m_event_not_connected);
	if (closesocket(m_listensock) == SOCKET_ERROR)
	{
		int x = WSAGetLastError();
	}
	if (m_hostsock && closesocket(m_hostsock) == SOCKET_ERROR)
	{
		int x = WSAGetLastError();
	}
	memset(m_http_header, 0, MAX_HEADER_LEN);
	DEBUG2(("waiting for relay close"));
	threadlib_waitforclose(&m_hthread);
	threadlib_destroy_event(&m_event_not_connected);
	m_hostsock = m_listensock = 0;

	DEBUG1(("relaylib_shutdown:done!"));
}


error_code relaylib_start()
{
	int ret;

	m_running = TRUE;
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

	DEBUG1(("***thread_accept:start"));

	while(m_running)
	{
		
		//
		// this event will keep use from accepting while we have a connection active
		// when a connection gets dropped, or when streamripper shuts down
		// this event will get signaled
		//
		DEBUG1(("***thread_accept:threadlib_waitfor_event"));
		threadlib_waitfor_event(&m_event_not_connected);
		DEBUG1(("***thread_accept:threadlib_waitfor_event returned!"));
		if (!m_running)
			break;

		m_hostsock = accept(m_listensock, (struct sockaddr *)&client, &iAddrSize);
		if (m_hostsock == SOCKET_ERROR)
		{
			break;
		}
		
		//
		// receive _some_ data, just enough to accept the request really.
		// then send out http header
		//

		recv(m_hostsock, &headbuf, 1, 0); 
		ret = send(m_hostsock, m_http_header, strlen(m_http_header), 0);
		if (ret < 0)
		{
			threadlib_signel_event(&m_event_not_connected);	// go back to accept
		}
	}
	m_running = FALSE;
	threadlib_endthread(&m_hthread);
}

//
// this function will be called for us, if we're connected then we send some data
// otherwise we just return with not connected
//
error_code relaylib_send(char *data, int len)
{
	int ret;

	if (threadlib_event_signaled(&m_event_not_connected))
		return SR_ERROR_HOST_NOT_CONNECTED;

	ret = send(m_hostsock, data, len, 0);
	if (ret < 0)
	{
		m_hostsock = 0;
		threadlib_signel_event(&m_event_not_connected);
		return SR_ERROR_HOST_NOT_CONNECTED;
	}
	return SR_SUCCESS;
}
