#ifndef __RELAYLIB_H__
#define __RELAYLIB_H__

#include "srtypes.h"

error_code relaylib_init (BOOL search_ports, int relay_port, int max_port, 
			  int *port_used, char *if_name, 
			  int max_connections, char *relay_ip);
void relaylib_shutdown();
error_code relaylib_set_response_header(char *http_header);
error_code relaylib_start();
error_code relaylib_send(char *data, int len, int accept_new, int is_meta);
BOOL relaylib_isrunning();
error_code relaylib_send_meta_data(char *track);
int get_relay_sockname(struct sockaddr_in *relay);

#endif //__RELAYLIB__
