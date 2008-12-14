/* inet.c - jonclegg@yahoo.com
 * handles connecting to shoutcast streams, and getting webpages
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

#include <stdlib.h>
#include <string.h>

//#include <windows.h>
#include "types.h"
#include "http.h"
#include "socklib.h"
#include "debug.h"

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code get_sc_header(const char* url, HSOCKET *sock, SR_HTTP_HEADER *info);

/*
 * Connects to a shoutcast type stream, leaves when it's about to get the header info
 */
error_code
inet_sc_connect(HSOCKET *sock, const char *url, const char *proxyurl, 
		SR_HTTP_HEADER *info, char *useragent, char *if_name)
{
    char headbuf[MAX_HEADER_LEN];
    URLINFO url_info;
    int ret;

    debug_printf("inet_sc_connect(): calling httplib_parse_url\n");
    if (proxyurl) {
	if ((ret = httplib_parse_url(proxyurl, &url_info)) != SR_SUCCESS)
	    return ret;
    }
    else if ((ret = httplib_parse_url(url, &url_info)) != SR_SUCCESS)
    {
	return ret;
    }

    debug_printf("inet_sc_connect(): calling sockinit\n");
    if ((ret = socklib_init()) != SR_SUCCESS)
	return ret;

    debug_printf("inet_sc_connect(): calling sock_open: host=%s, port=%d\n",
	url_info.host, url_info.port);
    if ((ret = socklib_open(sock, url_info.host, url_info.port, if_name)) != SR_SUCCESS)
	return ret;

    debug_printf("inet_sc_connect(): calling httplib_construct_sc_request\n");
    if ((ret = httplib_construct_sc_request(url, proxyurl, headbuf, useragent)) != SR_SUCCESS)
	return ret;

    debug_printf("inet_sc_connect(): calling socklib_sendall\n");
    if ((ret = socklib_sendall(sock, headbuf, strlen(headbuf))) < 0)
	return ret;

    debug_printf("inet_sc_connect(): calling get_sc_header\n");
    if ((ret = get_sc_header(url, sock, info)) != SR_SUCCESS)
	return ret;

    if (*info->http_location) {
	/* RECURSIVE CASE */
	inet_sc_connect(sock, info->http_location, proxyurl, info, useragent, if_name);
    }

    return SR_SUCCESS;
}

error_code
get_sc_header(const char* url, HSOCKET *sock, SR_HTTP_HEADER *info)
{
	int ret;
	char headbuf[MAX_HEADER_LEN] = {'\0'};

	if ((ret = socklib_read_header(sock, headbuf, MAX_HEADER_LEN, NULL)) != SR_SUCCESS)
		return ret;

	if ((ret = httplib_parse_sc_header(url, headbuf, info)) != SR_SUCCESS)
		return ret;

	return SR_SUCCESS;
}


error_code inet_get_webpage_alloc(HSOCKET *sock, const char *url, const char *proxyurl, char **buffer, unsigned long *size)
{
	char headbuf[MAX_HEADER_LEN];
	URLINFO url_info;
	int ret;

	if (proxyurl)
	{
		if ((ret = httplib_parse_url(proxyurl, &url_info)) != SR_SUCCESS)
			return ret;
	}
	else if ((ret = httplib_parse_url(url, &url_info)) != SR_SUCCESS)
	{
		return ret;
	}

	if ((ret = socklib_init()) != SR_SUCCESS)
		return ret;
		
	if ((ret = socklib_open(sock, url_info.host, url_info.port, NULL)) != SR_SUCCESS)
		return ret;
	
	if ((ret = httplib_construct_page_request(url, proxyurl != NULL, headbuf)) != SR_SUCCESS)
	{
		socklib_close(sock);
		return ret;
	}

	if ((ret = socklib_sendall(sock, headbuf, strlen(headbuf))) < 0)
	{
		socklib_close(sock);
		return ret;
	}

	memset(headbuf, 0, MAX_HEADER_LEN);
/*	if ((ret = socklib_read_header(sock, headbuf, MAX_HEADER_LEN, NULL)) != SR_SUCCESS)
	{
		socklib_close(*sock);
		return ret;
	}
*/

	if ((ret = socklib_recvall_alloc(sock, buffer, size, NULL)) != SR_SUCCESS)
	{
		socklib_close(sock);
		return ret;
	}
	socklib_close(sock);
	return SR_SUCCESS;
}

