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

#include "compat.h"
#include "http.h"
#include "util.h"
#include "debug.h"


/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code	httplib_parse_url(const char *url, URLINFO *urlinfo);
error_code	httplib_parse_sc_header(char *header, SR_HTTP_HEADER *info);
error_code	httplib_construct_sc_request(const char *url, const char* proxyurl, char *buffer, char *useragent);
error_code	httplib_construct_page_request(const char *url, BOOL proxyformat, char *buffer);
error_code	httplib_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size);

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static char* make_auth_header(const char *header_name, 
			      const char *username, const char *password);
char* b64enc(const char *buf, int size);


/*
 * Parse's a url as in http://host:port/path or host/path, etc..
 * and now http://username:password@server:4480
 */
error_code httplib_parse_url(const char *url, URLINFO *urlinfo)
{ 
	//
	// see if we have a proto 
	// 
	char *s = strstr(url, "://");
	int ret;

	//
	// if we have a proto, just skip it
	// JCBUG -- should we care about the proto? like fail if it's not http?
	//
	if (s) url = s + strlen("://");
	memcpy(urlinfo->path, (void *)"/\0", 2);

	//
	// search for a login '@' token
	//
	if (strchr(url, '@') != NULL)
	{
		ret = sscanf(url, "%[^:]:%[^@]", urlinfo->username, urlinfo->password);
		if (ret < 2) return SR_ERROR_PARSE_FAILURE;
		url = strchr(url, '@') + 1;
	}
	else
	{
		urlinfo->username[0] = '\0';
		urlinfo->password[0] = '\0';
	}

	//
	// search for a port seperator
	//
	if (strchr(url, ':') != NULL)
	{
		ret = sscanf(url, "%[^:]:%hu/%s", urlinfo->host, 
			(short unsigned int*)&urlinfo->port, urlinfo->path+1);
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

error_code httplib_construct_sc_request(const char *url, const char* proxyurl, char *buffer, char *useragent)
{
    int ret;
    URLINFO ui;
    URLINFO proxyui;
    char myurl[MAX_URL_LEN];
    if ((ret = httplib_parse_url(url, &ui)) != SR_SUCCESS)
	return ret;

    if (proxyurl)
    {
	sprintf(myurl, "http://%s:%d%s", ui.host, ui.port, ui.path);
	if ((ret = httplib_parse_url(proxyurl, &proxyui)) != SR_SUCCESS)
	    return ret;
    }
    else
	strcpy(myurl, ui.path);

    snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + SR_MAX_PATH,
	     "GET %s HTTP/1.0\r\n"
	     "Host: %s:%d\r\n"
	     "User-Agent: %s\r\n"
	     "Icy-MetaData:1\r\n", 
	     myurl, 
	     ui.host, 
	     ui.port, 
	     useragent[0] ? useragent : "Streamripper/1.x");

    //
    // http authentication (not proxy, see below for that)
    // 
    if (ui.username[0] && ui.password[0])
    {
	char *authbuf = make_auth_header("Authorization", 
					 ui.username,
					 ui.password);
	strcat(buffer, authbuf);
	free(authbuf);
    }

    //
    // proxy auth stuff
    //
    if (proxyurl && proxyui.username[0] && proxyui.password[0])
    {
	char *authbuf = make_auth_header("Proxy-Authorization",
					 proxyui.username,
					 proxyui.password);
	strcat(buffer, authbuf);
	free(authbuf);
    }
	
    strcat(buffer, "\r\n");

    return SR_SUCCESS;
}

// Make the 'Authorization: Basic xxxxxx\r\n' or 'Proxy-Authorization...' 
// headers for the HTTP request.
static char* make_auth_header(const char *header_name, 
			      const char *username, const char *password) 
{
	char *authbuf = malloc(strlen(header_name) 
			       + strlen(username)
			       + strlen(password)
			       + MAX_URI_STRING);
	char *auth64;
	sprintf(authbuf, "%s:%s", username, password);
	auth64 = b64enc(authbuf, strlen(authbuf));
	sprintf(authbuf, "%s: Basic %s\r\n", header_name, auth64);
	free(auth64);
	return authbuf;
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

	snprintf(buffer, MAX_HEADER_LEN + MAX_HOST_LEN + SR_MAX_PATH,
			"GET %s HTTP/1.0\r\n"
			"Host: %s:%d\r\n"
			"Accept: */*\r\n"
			"Accept-Language: en-us\r\n"
			"Accept-Encoding: gzip, deflate\r\n"
			"User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n"
			"Connection: Keep-Alive\r\n\r\n", 
			myurl, 
			ui.host, 
			ui.port);


	return SR_SUCCESS;

}

/* Return 1 if a match was found, 0 if not found */
int
extract_header_value(char *header, char *dest, char *match)
{
    char* start = (char *)strstr(header, match);
    if (start) {
	subnstr_until(start+strlen(match), "\n", dest, MAX_ICY_STRING);
	return 1;
    } else {
	return 0;
    }
}


error_code
httplib_parse_sc_header(char *header, SR_HTTP_HEADER *info)
{
    int rc;
    char *start;
    char versionbuf[64];
    char stempbr[50];

    if (!header || !info)
	return SR_ERROR_INVALID_PARAM;

    memset(info, 0, sizeof(SR_HTTP_HEADER));

    // Get the ICY code.
    start = (char *)strstr(header, "ICY ");
    if (!start) {
	start = (char *)strstr(header, "HTTP/1.");
	if (!start)	return SR_ERROR_NO_RESPOSE_HEADER;
    }

    start = strstr(start, " ") + 1;
    sscanf(start, "%i", &info->icy_code);

    DEBUG2(("header:\n %s", header));

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
	case 403:
	    return SR_ERROR_HTTP_403_ERROR;
	case 407:
	    return SR_ERROR_HTTP_407_ERROR;
	case 502:
	    return SR_ERROR_HTTP_502_ERROR;
	default:
	    return SR_ERROR_NO_ICY_CODE;
	}
    }

    // read the Shoutcast headers
    // apparently some icecast streams use different headers
    // but other then that it always works like this
    extract_header_value(header, info->http_location, "Location:");
    extract_header_value(header, info->server, "Server:");
    rc = extract_header_value(header, info->icy_name, "icy-name:");
    extract_header_value(header, info->icy_url, "icy-url:");
    extract_header_value(header, stempbr, "icy-br:");
    info->icy_bitrate = atoi(stempbr);
    info->have_icy_name = rc;

    // Lets try to guess the server :)
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
	if (!info->server[0])
	{
	    strcpy(info->server, "icecast/");
	    if ((start = (char *)strstr(start, "version ")) != NULL)
	    {
		sscanf(start, "version %[^<]<", versionbuf);
		strcat(info->server, versionbuf);
	    }
	}

	// icecast headers.
	extract_header_value(header, info->icy_url, "x-audiocast-server-url:");
	rc = extract_header_value(header, info->icy_name, "x-audiocast-name:");
	extract_header_value(header, info->icy_genre, "x-audiocast-genre:");
	extract_header_value(header, stempbr, "x-audiocast-bitrate:");
	info->icy_bitrate = atoi(stempbr);
	info->have_icy_name |= rc;
    }
    else if ((start = (char *)strstr(header, "Zwitterion v")) != NULL)
    {
	sscanf(start, "%[^<]<", info->server);
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

#if defined (commentout)
    if (info->icy_name[0])
    {
	sprintf(buf, "icy-name:%s\r\n", info->icy_name);
	strcat(header, buf);
    }
#endif
    if (info->have_icy_name) {
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


//
// taken from:
// Copyright (c) 2000 Virtual Unlimited B.V.
// Author: Bob Deblier <bob@virtualunlimited.com>
// thanks bob ;)
//
#define CHARS_PER_LINE  72
char* b64enc(const char *inbuf, int size)
{

		static const char* to_b64 =
				 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		  /* encode 72 characters per line */

        int div = size / 3;
        int rem = size % 3;
        int chars = div*4 + rem + 1;
        int newlines = (chars + CHARS_PER_LINE - 1) / CHARS_PER_LINE;

        const char* data = inbuf;
        char* string = (char*) malloc(chars + newlines + 1 + 100);

        if (string)
        {
                register char* buf = string;

                chars = 0;

                /*@+charindex@*/
                while (div > 0)
                {
                        buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
                        buf[1] = to_b64[((data[0] << 4) & 0x30) + ((data[1] >> 4) & 0xf)];
                        buf[2] = to_b64[((data[1] << 2) & 0x3c) + ((data[2] >> 6) & 0x3)];
                        buf[3] = to_b64[  data[2] & 0x3f];
                        data += 3;
                        buf += 4;
                        div--;
                        chars += 4;
                        if (chars == CHARS_PER_LINE)
                        {
                                chars = 0;
                                *(buf++) = '\n';
                        }
                }

                switch (rem)
                {
                case 2:
                        buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
                        buf[1] = to_b64[((data[0] << 4) & 0x30) + ((data[1] >> 4) & 0xf)];
                        buf[2] = to_b64[ (data[1] << 2) & 0x3c];
                        buf[3] = '=';
                        buf += 4;
                        chars += 4;
                        break;
                case 1:
                        buf[0] = to_b64[ (data[0] >> 2) & 0x3f];
                        buf[1] = to_b64[ (data[0] << 4) & 0x30];
                        buf[2] = '=';
                        buf[3] = '=';
                        buf += 4;
                        chars += 4;
                        break;
                }

        /*      *(buf++) = '\n'; This would result in a buffer overrun */
                *buf = '\0';
        }

        return string;
}

