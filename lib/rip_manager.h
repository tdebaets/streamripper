#ifndef __RIP_MANANGER_H__
#define __RIP_MANANGER_H__

#include "types.h"

#define MAX_STATUS_LEN		256
#define MAX_FILENAME_LEN	255
#define MAX_STREAMNAME_LEN	1024
#define MAX_SERVER_LEN		1024
#define MAX_DIR_LEN			100

// Messages for status_callback hook in rip_manager_init()
#define RM_UPDATE		0x01		// returns a pointer RIP_MANAGER_INFO struct
#define RM_ERROR		0x02		// returns the error code
#define RM_DONE			0x03		// NULL
#define RM_STARTED		0x04		// NULL
#define RM_NEW_TRACK	0x05		// Name of the new track
#define RM_TRACK_DONE	0x06		// Name of the track completed
#define RM_OUTPUT_DIR	0x07		// Full path of the output directory

#define RM_STATUS_BUFFERING		0x01
#define RM_STATUS_RIPPING		0x02
#define RM_STATUS_RECONNECTING	0x03
#define RM_STATUS_DONE			0x04

typedef struct RIP_MANAGER_INFOst
{
	char	streamname[MAX_STREAMNAME_LEN];
	char	server_name[MAX_SERVER_LEN];
	int		bitrate;
	int		meta_interval;
	char	filename[MAX_FILENAME_LEN];
	u_long	filesize;
	int	status;
} RIP_MANAGER_INFO;

// Use to set and get the flags
#define OPT_FLAG_ISSET(flags, opt)	((flags & opt) > 0)

#define OPT_AUTO_RECONNECT		0x00000001
#define OPT_SEPERATE_DIRS		0x00000002
#define OPT_OVER_WRITE_TRACKS	0x00000004
#define OPT_SEARCH_PORTS		0x00000008
#define OPT_NO_RELAY			0x00000010 
#define OPT_COUNT_FILES			0x00000020 
#define OPT_ADD_ID3				0x00000040 
#define OPT_DATE_STAMP			0x00000100
#define OPT_CHECK_MAX_BYTES		0x00000200

typedef struct RIP_MANAGER_OPTIONSst
{
	char	url[MAX_URL_LEN];
	char	proxyurl[MAX_URL_LEN];
	char	output_directory[MAX_PATH_LEN];
	BOOL	over_write_existing_tracks;
	int	relay_port;
	u_short	max_port;
	u_long	maxMB_rip_size;
	u_short	flags;
} RIP_MANAGER_OPTIONS;

typedef struct ERROR_INFOst
{
	char		error_str[MAX_ERROR_STR];
	error_code	error_code;
} ERROR_INFO;


extern error_code	rip_manager_start(void (*status_callback)(int message, void *data), RIP_MANAGER_OPTIONS *options);
extern void			rip_manager_stop();
extern char			*rip_manager_get_error_str(int code);


#endif //__RIP_MANANGER_H__


 
