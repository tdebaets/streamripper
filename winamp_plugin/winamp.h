#ifndef __WINAMP_H__
#define __WINAMP_H__

#include "types.h"

#define SKIN_PATH "Skins\\SrSkins\\"

typedef struct WINAMP_INFOst
{
	char url[MAX_URL_LEN];
	BOOL is_running;
} WINAMP_INFO;

extern BOOL winamp_init();
extern BOOL winamp_add_relay_to_playlist(char *host, u_short port);
extern BOOL winamp_add_track_to_playlist(char *track);
extern BOOL winamp_get_info(WINAMP_INFO *info, BOOL useoldway);
extern BOOL winamp_get_path(char *path);


#endif //__WINAMP_H__