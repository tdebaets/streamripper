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
#ifdef errno
#undef errno
#endif
#define errno WSAGetLastError()
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
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
#include "compat.h"

/*********************************************************************************
 * Public functions
 *********************************************************************************/
void            relaylib_shutdown();
error_code      relaylib_set_response_header(char *http_header);
error_code      relaylib_start();
error_code      relaylib_send(char *data, int len);
BOOL            relaylib_isrunning();

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static void                     thread_accept(void *notused);
static error_code       try_port(u_short port, char *if_name);
static void thread_send(void *notused);


/*********************************************************************************
 * Private vars 
 *********************************************************************************/
static HSEM             m_sem_not_connected;
static char             m_http_header[MAX_HEADER_LEN];
static SOCKET   m_listensock = SOCKET_ERROR;
static BOOL             m_running = FALSE;
static BOOL             m_initdone = FALSE;
static THREAD_HANDLE m_hthread;
static THREAD_HANDLE m_hthread2;

static struct hostsocklist_t
{
        SOCKET m_hostsock;
        int   m_Offset;
        int   m_LeftToSend;
        char* m_Buffer;
        int   m_BufferSize;
        struct hostsocklist_t *m_next;
} *m_hostsocklist = NULL;

unsigned long m_hostsocklist_len = 0;
static HSEM m_sem_listlock;
int m_max_connections;


#define BUFSIZE (1024)


static void destroy_all_hostsocks(void)
{
    struct hostsocklist_t       *ptr;

    threadlib_waitfor_sem(&m_sem_listlock);
    while(m_hostsocklist != NULL)
    {
        ptr = m_hostsocklist;
        closesocket(ptr->m_hostsock);
        m_hostsocklist = ptr->m_next;
        if( ptr->m_Buffer != NULL )
            free( ptr->m_Buffer );

        free(ptr);
    }
    m_hostsocklist_len = 0;
    threadlib_signel_sem(&m_sem_listlock);
}


// Quick function to "eat" incoming data from a socket
// All data is discarded
// Returns 0 if successful or SOCKET_ERROR if error
static int swallow_receive(int sock)
{
    fd_set fds;
    struct timeval tv;
    int ret = 0;
    char buf[BUFSIZE];
    BOOL hasmore = TRUE;
        
    FD_ZERO(&fds);
    while(hasmore)
    {
        // Poll the socket to see if it has anything to read
        hasmore = FALSE;
        FD_SET(sock, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(sock + 1, &fds, NULL, NULL, &tv);
        if (ret == 1)
        {
            // Read and throw away data, ignoring errors
            ret = recv(sock, buf, BUFSIZE, 0);
            if (ret > 0)
            {
                hasmore = TRUE;
            }
            else if (ret == SOCKET_ERROR)
            {
                break;
            }
        }
        else if (ret == SOCKET_ERROR)
        {
            break;
        }
    }
        
    return ret;
}


// Makes a socket non-blocking
void make_nonblocking(int sock)
{
    int opt;

#ifndef WIN32
    opt = fcntl(sock, F_GETFL);
    if (opt != SOCKET_ERROR)
    {
        fcntl(sock, F_SETFL, opt | O_NONBLOCK);
    }
#else
    opt = 1;
    ioctlsocket(sock, FIONBIO, &opt);
#endif
}


BOOL relaylib_isrunning()
{
    return m_running;
}


error_code
relaylib_set_response_header(char *http_header)
{
    if (!http_header)
        return SR_ERROR_INVALID_PARAM;

    strcpy(m_http_header, http_header);

    return SR_SUCCESS;
}

#ifndef WIN32
void catch_pipe(int code)
{
    //m_hostsock = 0;
    //m_connected = FALSE;
    // JCBUG, not sure what to do about this
}
#endif

error_code
relaylib_init(BOOL search_ports, int relay_port, int max_port, 
              int *port_used, char *if_name, int max_connections)
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

    if (m_initdone != TRUE) {
        m_sem_not_connected = threadlib_create_sem();
        m_sem_listlock = threadlib_create_sem();
        threadlib_signel_sem(&m_sem_listlock);

        // NOTE: we need to signel it here in case we try to destroy
        // relaylib before the thread starts!
        threadlib_signel_sem(&m_sem_not_connected);
        m_initdone = TRUE;
    }
    m_max_connections = max_connections;

    *port_used = 0;
    if (!search_ports)
        max_port = relay_port;

    for(;relay_port <= max_port; relay_port++) {
        ret = try_port((u_short)relay_port, if_name);
        if (ret == SR_ERROR_CANT_BIND_ON_PORT)
            continue;           // Keep searching.

        if (ret == SR_SUCCESS) {
            *port_used = relay_port;
            debug_printf("Relay: Listening on port %d\n", relay_port);
            return SR_SUCCESS;
        } else {
            return ret;
        }
    }

    return SR_ERROR_CANT_BIND_ON_PORT;
}

error_code
try_port(u_short port, char *if_name)
{
    struct sockaddr_in local;

    m_listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_listensock == SOCKET_ERROR)
        return SR_ERROR_SOCK_BASE;
    make_nonblocking(m_listensock);

    if (read_interface(if_name,&local.sin_addr.s_addr) != 0)
        local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_family = AF_INET;
    local.sin_port = htons(port);

#ifndef WIN32
    {
        // Prevent port error when restarting quickly after a previous exit
        int opt = 1;
        setsockopt(m_listensock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
#endif
                        
    if (bind(m_listensock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
    {
        closesocket(m_listensock);
        m_listensock = SOCKET_ERROR;
        return SR_ERROR_CANT_BIND_ON_PORT;
    }
        
    if (listen(m_listensock, 1) == SOCKET_ERROR)
    {
        closesocket(m_listensock);
        m_listensock = SOCKET_ERROR;
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
    threadlib_signel_sem(&m_sem_not_connected);
    if (closesocket(m_listensock) == SOCKET_ERROR)
    {   
        // JCBUG, what can we do?
    }
    m_listensock = SOCKET_ERROR;                // Accept thread will watch for this and not try to accept anymore
    memset(m_http_header, 0, MAX_HEADER_LEN);
    DEBUG2(("waiting for relay close"));
    threadlib_waitforclose(&m_hthread);
    destroy_all_hostsocks();
    threadlib_destroy_sem(&m_sem_not_connected);
    m_initdone = FALSE;

    DEBUG1(("relaylib_shutdown:done!"));
}


error_code relaylib_start()
{
    int ret;

    m_running = TRUE;
    // Spawn on a thread so it's non-blocking
    if ((ret = threadlib_beginthread(&m_hthread, thread_accept)) != SR_SUCCESS)
        return ret;

    if ((ret = threadlib_beginthread(&m_hthread2, thread_send)) != SR_SUCCESS)
        return ret;

    return SR_SUCCESS;
}


void thread_accept(void *notused)
{
    int ret;
    int newsock;
    BOOL good;
    struct sockaddr_in client;
    int iAddrSize = sizeof(client);
    struct hostsocklist_t *newhostsock;

    DEBUG1(("***thread_accept:start"));

    while(m_running)
    {
        fd_set fds;
        struct timeval tv;
                
        // this event will keep use from accepting while we have a connection active
        // when a connection gets dropped, or when streamripper shuts down
        // this event will get signaled
        DEBUG1(("***thread_accept:threadlib_waitfor_sem"));
        threadlib_waitfor_sem(&m_sem_not_connected);
        DEBUG1(("***thread_accept:threadlib_waitfor_sem returned!"));
        if (!m_running)
            break;

        // Poll once per second, instead of blocking forever in accept(), so that we can regain control if relaylib_shutdown() called
        FD_ZERO(&fds);
        while (m_listensock != SOCKET_ERROR)
        {
            FD_SET(m_listensock, &fds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            ret = select(m_listensock + 1, &fds, NULL, NULL, &tv);
            if (ret == 1) {
                unsigned long num_connected;
                /* If connections are full, do nothing.  Note that 
                    m_max_connections is 0 for infinite connections allowed. */
                threadlib_waitfor_sem(&m_sem_listlock);
                num_connected = m_hostsocklist_len;
                threadlib_signel_sem(&m_sem_listlock);
                if (m_max_connections > 0 && num_connected >= (unsigned long) m_max_connections) {
                    continue;
                }
                /* Check for connections */
                newsock = accept(m_listensock, (struct sockaddr *)&client, &iAddrSize);
                if (newsock != SOCKET_ERROR) {
                    // Got successful accept
                    make_nonblocking(newsock);

                    debug_printf("Relay: Client %d new from %s:%hd\n", newsock,
                                 inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                    // Socket is new and its buffer had better have room to hold the entire HTTP header!
                    good = FALSE;
                    if (swallow_receive(newsock) == 0) {
                        ret = send(newsock, m_http_header, strlen(m_http_header), 0);
                        if (ret == (int) strlen(m_http_header)) {
                            newhostsock = malloc(sizeof(*newhostsock));
                            if (newhostsock != NULL)
                            {
                                // Add new client to list (headfirst)
                                threadlib_waitfor_sem(&m_sem_listlock);
                                newhostsock->m_Offset= 0;
                                newhostsock->m_LeftToSend=0;
                                newhostsock->m_BufferSize=0;
                                newhostsock->m_Buffer=NULL;

                                newhostsock->m_hostsock = newsock;
                                newhostsock->m_next = m_hostsocklist;

                                m_hostsocklist = newhostsock;
                                m_hostsocklist_len++;
                                threadlib_signel_sem(&m_sem_listlock);
                                good = TRUE;
                            }
                        }
                    }

                    if (!good)
                    {
                        closesocket(newsock);
                        debug_printf("Relay: Client %d disconnected (Unable to receive HTTP header)\n", newsock);
                    }
                }
            }
            else if (ret == SOCKET_ERROR)
            {
                // Something went wrong with select
                break;
            }
        }
        threadlib_signel_sem(&m_sem_not_connected);     // go back to accept
    }

    m_running = FALSE;
}

// this function will be called for us, if we're connected then we send 
// some data otherwise we just return with not connected
error_code
relaylib_send(char *data, int len)
{
    struct hostsocklist_t *prev;
    struct hostsocklist_t *ptr;
    struct hostsocklist_t *next;
    error_code err = SR_SUCCESS;

    if (m_initdone != TRUE)
    {
	// Do nothing if relaylib not in use
	return SR_ERROR_HOST_NOT_CONNECTED;
    }
    if( len <= 0 )
	return SR_SUCCESS;

    debug_printf("Relay: relaylib_send %5d bytes\n",len);

    threadlib_waitfor_sem(&m_sem_listlock);
    ptr = m_hostsocklist;
    if (ptr != NULL)
    {
        prev = NULL;
        while(ptr != NULL)
        {
	    int bsz;
            next = ptr->m_next;

	    //Up to two full packets
	    bsz= ( 8*len > BUFSIZE ) ? 8*len : BUFSIZE;
	    if( bsz > ptr->m_BufferSize ) {
		ptr->m_Buffer= realloc( ptr->m_Buffer, bsz );
		ptr->m_BufferSize= bsz;
	    }

	    if( ptr->m_Buffer != NULL ) {
		if( ptr->m_LeftToSend < 0 )
		    ptr->m_LeftToSend=0;

		if( ( ptr->m_Offset > 0 )&&( ptr->m_LeftToSend > 0 ) ) {
		    memmove( ptr->m_Buffer, ptr->m_Buffer+ptr->m_Offset, ptr->m_LeftToSend );
		    ptr->m_Offset=0;
		}
		ptr->m_Offset=0;
		if( ptr->m_LeftToSend + len < ptr->m_BufferSize ) {
		    memcpy( ptr->m_Buffer + ptr->m_LeftToSend, data, len );
		    ptr->m_LeftToSend+= len;
		} else {
		    debug_printf("Relay: overflow copying %d data bytes to %d\n", len , ptr->m_BufferSize-len );
		    memcpy( ptr->m_Buffer + ptr->m_BufferSize-len, data, len );
		    ptr->m_LeftToSend = ptr->m_BufferSize;
		}
	    }         
            if (ptr != NULL)
            {
                prev = ptr;
            }
            ptr = next;
        }
    }
    else
    {
        err = SR_ERROR_HOST_NOT_CONNECTED;
    }
    threadlib_signel_sem(&m_sem_listlock);

    debug_printf("Relay: relaylib_send done\n" );

    return err;
}

error_code
relay_send_meta_data(char *track)
{
    int track_len, meta_len, extra_len, chunks, header_len, footer_len;
    char zerobuf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    const char header[] = "StreamTitle='";
    const char footer[] = "';StreamUrl='';";
    unsigned char c;
    char buf[BUFSIZE];
    char *bufptr;
    int buflen = 0;

    // Default to sending zero metadata if any errors occur
    buf[0] = 0;
    buflen = 1;

    if (!track || !*track) {
        return relaylib_send(buf, 1);
    }
    track_len = strlen(track);
    header_len = strlen(header);
    footer_len = strlen(footer);
    meta_len = track_len+header_len+footer_len;
    chunks = 1 + (meta_len-1) / 16;
    /* GCS the following test is because c is assumed unsigned when read 
       (see above code) */
    if (chunks > 127) {
        return relaylib_send(buf, 1);
    }

    c = chunks;
    extra_len = c*16 - meta_len;

    buflen = 1 + header_len + track_len + footer_len + extra_len;
    bufptr = buf;
    if (buflen > BUFSIZE)
    {
        // Avoid buffer overflow by just sending zero metadata instead
        debug_printf("Relay: Metadata overflow (%d bytes)\n", buflen);
        return relaylib_send(buf, 1);
    }
        
    memcpy(bufptr, &c, 1);
    bufptr += 1;
    memcpy(bufptr, header, header_len);
    bufptr += header_len;
    memcpy(bufptr, track, track_len);
    bufptr += track_len;
    memcpy(bufptr, footer, footer_len);
    bufptr += footer_len;
    memcpy(bufptr, zerobuf, extra_len);
    bufptr += extra_len;
        
    return relaylib_send(buf, buflen);
}

void thread_send(void *notused)
{
    struct hostsocklist_t *prev;
    struct hostsocklist_t *ptr;
    struct hostsocklist_t *next;
    int sock;
    int ret;
    BOOL good;
    error_code err = SR_SUCCESS;
    int err_errno;

    while(m_running)
    {
	threadlib_waitfor_sem(&m_sem_listlock);
	ptr = m_hostsocklist;
	if (ptr != NULL)
	{
	    prev = NULL;
	    while(ptr != NULL)
	    {
		sock = ptr->m_hostsock;
		next = ptr->m_next;
		good = TRUE;

		// Replicate the same data to each socket
		if (swallow_receive(sock) != 0)
		{
		    good = FALSE;
		}
		else
		{
		    if( ptr->m_LeftToSend > 0 ) {
			debug_printf("Relay: Sending Client %d to the client\n", ptr->m_LeftToSend );
			ret = send(sock, ptr->m_Buffer+ptr->m_Offset, ptr->m_LeftToSend, 0);
			debug_printf("Relay: Sending to Client returned %d\n", ret );
			if (ret == SOCKET_ERROR)
			{
			    /* Sometimes windows gives me an errno of 0
			       Sometimes windows gives me an errno of 183 
			       See this thread for details: 
				http://groups.google.com/groups?hl=en&lr=&ie=UTF-8&selm=8956d3e8.0309100905.6ba60e7f%40posting.google.com
				*/
			    err_errno = errno;
			    if (err_errno == EWOULDBLOCK || err_errno == 0 || err_errno == 183) {
				WSASetLastError (0);
				// Client is slow.  Retry later.
			    } else {
				debug_printf ("Relay: socket error is %d\n",errno);
				good = FALSE;
			    }
			} else { 
			    // Client is slow.  Retry later.
			    ptr->m_Offset+= ret;
			    ptr->m_LeftToSend-=ret;
			    if( ptr->m_LeftToSend < 0 )
				ptr->m_LeftToSend=0;

			}
		    }        
		}

	       
		if (!good)
		{
		    debug_printf("Relay: Client %d disconnected (%s)\n", sock, strerror(errno));
		    closesocket(sock);
                                   
		    // Carefully delete this client from list without affecting list order
		    if (prev != NULL)
		    {
			prev->m_next = next;
		    }
		    else
		    {
			m_hostsocklist = next;
		    }
		    if( ptr->m_Buffer != NULL )
			free( ptr->m_Buffer );

		    free(ptr);
		    ptr = NULL;
		    m_hostsocklist_len --;
		}
                           
		if (ptr != NULL)
		{
		    prev = ptr;
		}
		ptr = next;
	    }
	}
	else
	{
	    err = SR_ERROR_HOST_NOT_CONNECTED;
	}
	threadlib_signel_sem(&m_sem_listlock);
	Sleep( 50 );
    }
}
