#ifndef __RELAYLIB_H__
#define __RELAYLIB_H__

#include "types.h"

error_code relaylib_init(BOOL search_ports, int relay_port, int max_port, 
	      int *port_used, char *if_name, int max_connections);
void relaylib_shutdown();
error_code relaylib_set_response_header(char *http_header);
error_code relaylib_start();
error_code relaylib_send(char *data, int len);
BOOL relaylib_isrunning();
error_code relay_send_meta_data(char *track);

#endif //__RELAYLIB__
