#ifndef __INET_H__
#define __INET_H__

#include "types.h"
#include "socklib.h"
#include "http.h"

extern error_code inet_get_webpage_alloc(HSOCKET *sock, const char *url,
					 const char *proxyurl, 
					 char **buffer, unsigned long *size);
extern error_code inet_sc_connect(HSOCKET *sock, const char *url, 
				  const char *proxyurl, 
				  SR_HTTP_HEADER *info, char *useragent, 
				  char *ifr_name);

#endif //__INET_H__





