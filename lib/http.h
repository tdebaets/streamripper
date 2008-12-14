#ifndef __HTTP_H__
#define __HTTP_H__

#include "types.h"

typedef struct SR_HTTP_HEADERst
{
    int content_type;
    int meta_interval;
    int have_icy_name;
    char icy_name[MAX_ICY_STRING];
    int icy_code;
    int icy_bitrate;
    char icy_genre[MAX_ICY_STRING];
    char icy_url[MAX_ICY_STRING];
    char http_location[MAX_HOST_LEN];
    char server[MAX_SERVER_LEN];
} SR_HTTP_HEADER;

typedef struct URLINFOst
{
	char host[MAX_HOST_LEN];
	char path[SR_MAX_PATH];
	u_short port;
	char username[MAX_URI_STRING];
	char password[MAX_URI_STRING];
} URLINFO;

error_code httplib_parse_url(const char *url, URLINFO *urlinfo);
error_code httplib_parse_sc_header(const char* url, char *header, SR_HTTP_HEADER *info);
error_code httplib_construct_sc_request(const char *url, const char* proxyurl, char *buffer, char *useragent);
error_code httplib_construct_page_request(const char *url, BOOL proxyformat, char *buffer);
error_code httplib_construct_sc_response(SR_HTTP_HEADER *info, char *header, int size);

#endif //__HTTP_H__
