#ifndef __LIVE365INFO_H__
#define __LIVE365INFO_H__

#include "srtypes.h"

#define MAX_SEARCH_STR	64
#define	MAX_MEMBER_NAME	256
#define GET_TRACK_FAILED	-1

#define SRLIM_TRACK_CHANGED		0x00
#define SRLIM_ERROR				0x01

typedef struct LIVE365_TRACK_INFOst
{
	char track[MAX_TRACK_LEN];
	int sec;
} LIVE365_TRACK_INFO;


extern error_code live365info_track_nofity(const char *url, const char *proxyurl, const char *stream_name, 
									void (*callback)(int message, void *data));
extern void live365info_terminate();

#endif //__LIVE365INFO_H__
