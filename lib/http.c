/* http.c - jonclegg@yahoo.com
 * library routines for handling shoutcast centric http parsing
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

#include "types.h"
#include "http.h"
#include "util.h"

#if WIN32
#define snprintf	_snprintf
#endif

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	httplib_parse_url(const char *url, URLINFO *urlinfo);
error_code	httplib_parse_sc_header(char *header, SR_HTTP_HEADER *info);
error_code	httplib_construct_sc_request(const char *url, BOOL proxyformat, char *buffer);
error_code	httplib_construct_page_request(const char *url, BOOL proxyformat, char *buffer);
error_code	httplib_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size);

/*
 * Parse's a url as in http://host:port/path or host/path, etc..
 */
error_code httplib_parse_url(const char *url, URLINFO *urlinfo)
{ 
	char *s = strstr(url, "://");
	int ret;

	if (s) url = s + strlen("://");
	memcpy(urlinfo->path, (void *)"/\0", 2);
	if (strchr(url, ':') != NULL)
	{
		ret = sscanf(url, "%[^:]:%hu/%s", urlinfo->host, (unsigned int*)&urlinfo->port, urlinfo->path+1);
		if (urlinfo->port < 1) return SR_ERROR_PARSE_FAILURE;
		ret -= 1;
	}
	else
	{
		urlinfo->port = 80;
		ret = sscanf(url, "%[^/]/%s", urlinfo->host, urlinfo->path+1);
	}
	if (ret < 1) return SR_ERROR_INVALID_URL;

	return SR_SUCCESS;
}

// This pretends to be WinAmp
error_code httplib_construct_sc_request(const char *url, BOOL proxyformat, char *buffer)
{
	int ret;
	URLINFO ui;
	char myurl[MAX_URL_LEN];
	if ((ret = httplib_parse_url(url, &ui)) != SR_SUCCESS)
		return ret;

	if (proxyformat)
		sprintf(myurl, "http://%s:%d%s", ui.host, ui.port, ui.path);
	else
		strcpy(myurl, ui.path);

snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + MAX_PATH_LEN,
"GET %s HTTP/1.0\r\n\
Host: %s:%d\r\n\
User-Agent: WinampMPEG/2.00\r\n\
Icy-MetaData:1\r\n\
Accept: */*\r\n\r\n", myurl, ui.host, ui.port);

    return SR_SUCCESS;
}

// Here we pretend we're IE 5, hehe
error_code httplib_construct_page_request(const char *url, BOOL proxyformat, char *buffer)
{
	int ret;
	URLINFO ui;
	char myurl[MAX_URL_LEN];
	if ((ret = httplib_parse_url(url, &ui)) != SR_SUCCESS)
		return ret;

	if (proxyformat)
		sprintf(myurl, "http://%s:%d%s", ui.host, ui.port, ui.path);
	else
		strcpy(myurl, ui.path);

	snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + MAX_PATH_LEN,
"GET %s HTTP/1.0\r\n\
Host: %s:%d\r\n\
Accept: */*\r\n\
Accept-Language: en-us\r\n\
Accept-Encoding: gzip, deflate\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n\
Connection: Keep-Alive\r\n\r\n", myurl, ui.host, ui.port);


	return SR_SUCCESS;

}


error_code	httplib_parse_sc_header(char *header, SR_HTTP_HEADER *info)
{
    char *start;
	char versionbuf[64];
	if (!header || !info)
		return SR_ERROR_INVALID_PARAM;

	memset(info, 0, sizeof(SR_HTTP_HEADER));

	// Get the ICY code.
	start = (char *)strstr(header, "ICY ");
	if (!start)
	{
		start = (char *)strstr(header, "HTTP/1.");
		if (!start)	return SR_ERROR_NO_RESPOSE_HEADER;
	}

	start = strstr(start, " ") + 1;
	sscanf(start, "%i", &info->icy_code);

	if (info->icy_code >= 400)
	{
		switch (info->icy_code)
		{
			case 400:
				return SR_ERROR_HTTP_400_ERROR;
			case 404:
				return SR_ERROR_HTTP_404_ERROR;
			case 401:
				return SR_ERROR_HTTP_401_ERROR;
			case 502:
				return SR_ERROR_HTTP_502_ERROR;
			default:
				return SR_ERROR_NO_ICY_CODE;
		}
	}

	start = (char *)strstr(header, "Location:");
	if (start)
		sscanf(start, "Location:%[^\n]\n", info->http_location);

	start = (char *)strstr(header, "Server:");
	if (start)
		subnstr_until(start+strlen("Server:"), "\n", info->server, MAX_SERVER_LEN);


    // Get the icy name
    start = (char *)strstr(header, "icy-name:");
    if (start)
		subnstr_until(start+strlen("icy-name:"), "\n", info->icy_name, MAX_ICY_STRING);

	// Get genre
    start = (char *)strstr(header, "icy-genre:");
    if (start)
		subnstr_until(start+strlen("icy-genre:"), "\n", info->icy_genre, MAX_ICY_STRING);

	// Get URL
    start = (char *)strstr(header, "icy-url:");
    if (start)
		subnstr_until(start+strlen("icy-url:"), "\n", info->icy_url, MAX_ICY_STRING);

	// Get bitrate
    start = (char *)strstr(header, "icy-br:");
    if (start)
        sscanf(start, "icy-br:%i", &info->icy_bitrate);

	// Lets try to guess the server :)
	if (info->server[0] == '\0')
	{
		// Try Shoutcast
		if ((start = (char *)strstr(header, "SHOUTcast")) != NULL)
		{
			strcpy(info->server, "SHOUTcast/");
			if ((start = (char *)strstr(start, "Server/")) != NULL)
			{
				sscanf(start, "Server/%[^<]<", versionbuf);
				strcat(info->server, versionbuf);
			}

		}
		else if ((start = (char *)strstr(header, "icecast")) != NULL)
		{
			strcpy(info->server, "icecast/");
			if ((start = (char *)strstr(start, "version ")) != NULL)
			{
				sscanf(start, "version %[^<]<", versionbuf);
				strcat(info->server, versionbuf);
			}

		}

		else if ((start = (char *)strstr(header, "Zwitterion v")) != NULL)
		{
			sscanf(start, "%[^<]<", info->server);
		}

	}

	// Make sure we don't have any CRLF's at the end of our strings
	trim(info->icy_url);
	trim(info->icy_genre);
	trim(info->icy_name);
	trim(info->http_location);
	trim(info->server);

    //get the meta interval
    start = (char*)strstr(header, "icy-metaint:");
    if (start)
    {
        sscanf(start, "icy-metaint:%i", &info->meta_interval);
        if (info->meta_interval < 1)
        {
            return SR_ERROR_NO_META_INTERVAL;
        }
    }
    else
        info->meta_interval = NO_META_INTERVAL;

    return SR_SUCCESS;
}

/* 
 * Constructs a HTTP response header from the SR_HTTP_HEADER struct, if data is null it is not 
 * added to the header
 */
error_code httplib_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size)
{
	char *buf = (char *)malloc(size);

	if (!info || !header || size < 1)
		return SR_ERROR_INVALID_PARAM;

	memset(header, 0, size);
	
	sprintf(buf, "HTTP/1.0 %d\r\n", info->icy_code);
	strcat(header, buf);

	if (info->http_location[0])
	{
		sprintf(buf, "Location:%s\r\n", info->http_location);
		strcat(header, buf);
	}

	if (info->server[0])
	{
		sprintf(buf, "Server:%s\r\n", info->server);
		strcat(header, buf);
	}

	if (info->icy_name[0])
	{
		sprintf(buf, "icy-name:%s\r\n", info->icy_name);
		strcat(header, buf);
	}

	if (info->icy_url[0])
	{
		sprintf(buf, "icy-url:%s\r\n", info->icy_url);
		strcat(header, buf);
	}

	if (info->icy_bitrate)
	{
		sprintf(buf, "icy-br:%d\r\n", info->icy_bitrate);
		strcat(header, buf);
	}

	if (info->icy_genre[0])
	{
		sprintf(buf, "icy-genre:%s\r\n", info->icy_genre);
		strcat(header, buf);
	}

	if (info->meta_interval > 0)
	{
		sprintf(buf, "icy-metaint:%d\r\n", info->meta_interval);
		strcat(header, buf);
	}

	free(buf);
	strcat(header, "\r\n");

	return SR_SUCCESS;
}

