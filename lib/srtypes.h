#ifndef __SRTYPES_H__
#define __SRTYPES_H__

////////////////////////////////////////////////
// Types
////////////////////////////////////////////////
#include "srconfig.h"
#if WIN32
/* Warning: Not allowed to mix windows.h & cdk.h */
#include <windows.h>
#else
#include <sys/types.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#if HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

/* Note: uint32_t is standardized in ISO C99, so let's use that one */
#if !HAVE_UINT32_T
# if HAVE_U_INT32_T
typedef u_int32_t uint32_t;
# else
typedef unsigned int uint32_t;
# endif
#endif

#if HAVE_WCHAR_SUPPORT
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if HAVE_WCTYPE_H
#include <wctype.h>
#endif
#endif
#if STDC_HEADERS
#include <stddef.h>
#endif

#define BOOL	int
#define TRUE	1
#define FALSE	0

#define NO_META_INTERVAL	-1

/* GCS - Grr. I don't care.  Max path is 254 until I get around to
    fixing this for other platforms. */
#define SR_MAX_PATH		254
#define MAX_HOST_LEN		512
#define MAX_IP_LEN			3+1+3+1+3+1+3+1
#define MAX_HEADER_LEN		8192
#define MAX_URL_LEN		8192
#define MAX_ICY_STRING		4024
#define MAX_SERVER_LEN		1024
//#define MAX_TRACK_LEN		MAX_PATH
#define MAX_TRACK_LEN		SR_MAX_PATH /* GCS - be careful here... */
#define MAX_URI_STRING		1024
#define MAX_ERROR_STR           (4096)
#define MAX_USERAGENT_STR	1024
#define MAX_AUTH_LEN            255
//#define MAX_DROPSTRING_LEN      255

#define MAX_METADATA_LEN (127*16)


#ifdef WIN32
  #ifndef _WINSOCKAPI_
    #define __DEFINE_TYPES__
  #endif
#endif

#ifdef __DEFINE_TYPES__
typedef unsigned long u_long;
typedef unsigned char u_char;
typedef unsigned short u_short;
#endif

/* Different types of streams */
#define CONTENT_TYPE_MP3		1
#define CONTENT_TYPE_NSV		2
#define CONTENT_TYPE_OGG    		3
#define CONTENT_TYPE_ULTRAVOX		4
#define CONTENT_TYPE_AAC		5
#define CONTENT_TYPE_PLS		6
#define CONTENT_TYPE_M3U		7
#define CONTENT_TYPE_UNKNOWN		99

/* 
 * IO_DATA_INPUT is a interface for socket input data, it has one 
 * method 'get_data' and is called by a "ripper" which is effectivly 
 * only ripshout.c (and the R.I.P. riplive365.c)
 */
typedef struct IO_DATA_INPUTst{
	int (*get_input_data)(char* buffer, int size);
} IO_DATA_INPUT;

#define NO_TRACK_STR	"No track info..."

/* 
 * IO_GET_STREAM is an interface for getting data and track info from
 * a better splite on the track seperation. it keeps a back buffer and 
 * does the "find silent point" shit.
 */
#if defined (commentout)
typedef struct IO_GET_STREAMst{
	int (*get_stream_data)(char* data_buf, char *track_buf);
	u_long getsize;
} IO_GET_STREAM;
#endif

/* 
 * SPLITPOINT_OPTIONS are the options used to tweek how the silence 
 * separation is done.
 */
typedef struct SPLITPOINT_OPTIONSst
{
    int	xs;
    int xs_min_volume;
    int xs_silence_length;
    int xs_search_window_1;
    int xs_search_window_2;
    int xs_offset;
    //int xd_offset;
    //int xpadding_1;
    //int xpadding_2;
    int xs_padding_1;
    int xs_padding_2;
} SPLITPOINT_OPTIONS;

/* 
 * CODESET_OPTIONS are the options used to decide how to parse
 * and convert the metadata
 */
#define MAX_CODESET_STRING 128
typedef struct CODESET_OPTIONSst
{
    char codeset_locale[MAX_CODESET_STRING];
    char codeset_filesys[MAX_CODESET_STRING];
    char codeset_id3[MAX_CODESET_STRING];
    char codeset_metadata[MAX_CODESET_STRING];
    char codeset_relay[MAX_CODESET_STRING];
} CODESET_OPTIONS;

/* 
 * Various CODESET types
 */
#define CODESET_UTF8          1
#define CODESET_LOCALE        2
#define CODESET_FILESYS       3
#define CODESET_ID3           4
#define CODESET_METADATA      5
#define CODESET_RELAY         6

/* 
 * Wide character support
 */
#if HAVE_WCHAR_SUPPORT
typedef wchar_t mchar;
#else
typedef char mchar;
#endif

/* 
 * TRACK_INFO is the parsed metadata
 */
typedef struct TRACK_INFOst
{
    int have_track_info;
    char raw_metadata[MAX_TRACK_LEN];
    mchar w_raw_metadata[MAX_TRACK_LEN];
    mchar artist[MAX_TRACK_LEN];
    mchar title[MAX_TRACK_LEN];
    mchar album[MAX_TRACK_LEN];
    mchar track[MAX_TRACK_LEN];
    char composed_metadata[MAX_METADATA_LEN+1];      /* For relay stream */
    BOOL save_track;
} TRACK_INFO;

#ifndef WIN32
typedef int SOCKET;
#endif

typedef struct HSOCKETst
{
	SOCKET	s;
	BOOL	closed;
} HSOCKET;

/* 
 * OverwriteOpt controls how files in complete directory are overwritten
 */
enum OverwriteOpt {
    OVERWRITE_UNKNOWN,	// Error case
    OVERWRITE_ALWAYS,	// Always replace file in complete with newer
    OVERWRITE_NEVER,	// Never replace file in complete with newer
    OVERWRITE_LARGER	// Replace file in complete if newer is larger
};


#endif //__SRIPPER_H__
