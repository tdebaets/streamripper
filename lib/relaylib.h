#ifndef __RELAYLIB_H__
#define __RELAYLIB_H__

#include "types.h"

extern error_code	relaylib_init(BOOL search_ports, int relay_port, int max_port, int *port_used);
extern void			relaylib_shutdown();
extern error_code	relaylib_set_response_header(char *http_header);
extern error_code	relaylib_start();
extern error_code	relaylib_send(char *data, int len);
extern BOOL			relaylib_isrunning();

#endif //__RELAYLIB__

