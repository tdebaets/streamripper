/* live365info.c - jonclegg@yahoo.com
 * hits live365 pages and returns the current track
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
//#include <process.h>
//#include <windows.h>

#include "live365info.h"
#include "http.h"
#include "util.h"
#include "inet.h"
#include "types.h"
#include "socklib.h"
#include "threadlib.h"
#include "live365info.h"


/*********************************************************************************
 * Public functions
 *********************************************************************************/
extern error_code live365info_track_nofity(const char *url, const char *proxyurl, const char *stream_name, 
									void (*callback)(int message, void *data));
extern void live365info_terminate();


/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code	get_current_track(const char *url, const char *stream_name, char *track, int *len);
static error_code	page_get_current_track(char *track, int *len);
static error_code	parse_live365_ids(char *source, char *match, unsigned long **ids, int *list_size);
static error_code	parse_ratings_info(const char *html, char *ip, int *port, char *membername);
static error_code	parse_current_track(const char *html, char *track, int *len);
static error_code	page_get_membername(const char *host, const int port, const char *stream_name, char *membername);
static error_code	get_live365_ids(char *searchstr, unsigned long **ids, int *list_size);
void				thread_track_notify(void *notused);
static void			post_track_changed(char *track, int len);
static void			post_error(int error);
static error_code	find_and_scan(const char *where, const char *what, char *input);
static error_code	get_webpage_alloc(const char *url, char **buffer, unsigned long *size);


/*********************************************************************************
 * Private Vars
 *********************************************************************************/
static const char		*m_directory_cgi = "http://www.live365.com/cgi-bin/directory.cgi?searchdesc=%s";
static const char		*m_ratings_cgi = "http://www.live365.com/cgi-bin/ratings.cgi?alsotry=1&stream=%d&genre=g";
static const char		*m_track_cgi = "http://www.live365.com:89/pls/front?handler=playlist&cmd=view&handle=%s";

static char				m_membername[MAX_MEMBER_NAME];
static char				m_host[MAX_HOST_LEN];
static int				m_port;
static void				(*m_callback)(int message, void *data);
static char				m_url[MAX_URL_LEN];
static char				m_stream_name[MAX_ICY_STRING];	//MAX_ICY_STRING[MAX_SEARCH_STR];
static char				m_proxyurl[MAX_URL_LEN];
static HSOCKET			m_sock;
static THREAD_HANDLE	m_thread;

char *strip_last_word(char *str)
{
    int len = strlen(str)-1;

    while(str[len] != ' ' && len != 0)
	len--;

    str[len] = '\0';
    return str;
}

int word_count(char *str)
{
    int n = 0;
    char *p = str;

    if (!*p)
	return 0; 

    while(*p++)
	if (*p == ' ')
	    n++;

    return n+1;
}

char *escape_string_alloc(const char *str)
{
    static const char c2x_table[] = "0123456789abcdef";
    const unsigned char *spStr = (const unsigned char *)str;
    char *sNewStr = (char *)calloc(3 * strlen(str) + 1, sizeof(char));
    unsigned char *spNewStr = (unsigned char *)sNewStr;
    unsigned c;

    while ((c = *spStr)) {
        if (c < 'A' || c > 'z') 
	{
	    *spNewStr++ = '%';
	    *spNewStr++ = c2x_table[c >> 4];
	    *spNewStr++ = c2x_table[c & 0xf];
        }
        else 
	{
            *spNewStr++ = c;
        }
        ++spStr;
    }
    *spNewStr = '\0';
    return sNewStr;
}

/// FOR TESTING ///
void dump_file(char *name, char *source)
{
	FILE *fp = fopen(name, "w");
	fwrite(source, sizeof(char), strlen(source), fp);
	fclose(fp);
}


void post_track_changed(char *track, int len)
{
	LIVE365_TRACK_INFO li;
	
	strcpy(li.track, track);
	li.sec = len;

	m_callback(SRLIM_TRACK_CHANGED, &li);
}
void post_error(int error)
{
	m_callback(SRLIM_ERROR, (void *)&error);
}

void live365info_terminate()
{
	DEBUG1(("live365info_terminate()\n"));
	threadlib_stop(&m_thread);
	socklib_close(&m_sock);
	DEBUG1(("live365info_terminate() waitforclose\n"));
	threadlib_waitforclose(&m_thread);
	DEBUG1(("live365info_terminate() done!\n"));
}

error_code live365info_track_nofity(const char *url, const char *proxyurl, const char *stream_name, 
									void (*callback)(int message, void *data))
{
	int ret;

	// Start nofity thread
	if (!url || !stream_name || !callback)
		return SR_ERROR_INVALID_PARAM;
	
	if (proxyurl)
		strncpy(m_proxyurl, proxyurl, MAX_URL_LEN);

	strncpy(m_url, url, MAX_URL_LEN);
	strncpy(m_stream_name, stream_name, MAX_SEARCH_STR);

	m_callback = callback;
	if ((ret = threadlib_beginthread(&m_thread, thread_track_notify)) != SR_SUCCESS)
		return ret;

	return SR_SUCCESS;
}

/*
 * Thread for notifying the client when the track changes
 * ***NEEDS WORK***
 */
void thread_track_notify(void *notused)
{
	char track[MAX_TRACK_LEN] = {'\0'};
	char last_track[MAX_TRACK_LEN] = {'\0'};
	int track_len = 0;
	int ret;

	memset(track, 0, MAX_TRACK_LEN);
	memset(last_track, 0, MAX_TRACK_LEN);
	while(TRUE)
	{
		// Hit until the track has changed
		do 
		{
			DEBUG1(("live:sleep\n"));
			threadlib_sleep(&m_thread, 1);
			if (!threadlib_isrunning(&m_thread))
				break;
			DEBUG1(("live: done sleeping\n"));
			strcpy(last_track, track);
			DEBUG1(("live: getting track\n"));
			ret = get_current_track(m_url, m_stream_name, track, &track_len);
			DEBUG1(("live: done getting track\n"));
			if (!threadlib_isrunning(&m_thread))
				break;

			if(ret == SR_ERROR_RECV_FAILED || 
			   ret == SR_ERROR_SOCKET_CLOSED)
			{
				DEBUG1(("\nFailed to get live365 track info"));
				post_error(SR_ERROR_NO_TRACK_INFO);
				continue;
			}

			else if (ret != SR_SUCCESS)
			{
				DEBUG1(("ret: %x\n", ret));
				post_error(SR_ERROR_NO_TRACK_INFO);	// Maybe a different error for this.
				continue;
			}
			DEBUG1(("got track ok: %s\n", track));

			DEBUG1((">\n"));

			// This will just post the first one
			if (!*last_track)
				post_track_changed(track, track_len);

		} while(strcmp(last_track, track) == 0 || *last_track == '\0');

		if (!threadlib_isrunning(&m_thread))
			break;

		// As long as there is a track len, sleep for that long minus a little
		post_track_changed(track, track_len);
		if (track_len > 15)
		{
			DEBUG1(live: long sleep %d\n", track_len-15));
			threadlib_sleep(&m_thread, (track_len - 15));
			DEBUG1(("live: done long, sleep"));
		}
	}
	DEBUG1(("Leaving Live365info thread"));
	threadlib_close(&m_thread);
}

error_code get_current_track(const char *url, const char *stream_name, char *track, int *len)
{
	URLINFO ui;
	int ret;

	if (!url || !stream_name)
		return SR_ERROR_INVALID_PARAM;

	// Parse the url
	if ((ret = httplib_parse_url(url, &ui)) != SR_SUCCESS)
		return ret;

	if ((strcmp(ui.host, m_host) != 0) || (ui.port != m_port))
	{
		if ((ret = page_get_membername(ui.host, ui.port, stream_name, m_membername)) != SR_SUCCESS)
			return ret;

		strcpy(m_host, ui.host);
		m_port = ui.port;
	}

	if ((ret = page_get_current_track(track, len)) != SR_SUCCESS)
		return ret;

	return SR_SUCCESS;
}

error_code page_get_current_track(char *track, int *len)
{
	char tempurl[MAX_URL_LEN];
	char *html;
	unsigned long size;
	int ret;

	if (track == NULL)
		return SR_ERROR_INVALID_PARAM;

	if (m_membername == NULL)
		return SR_ERROR_NULL_MEMBER_NAME;

	sprintf(tempurl, m_track_cgi, m_membername);

	DEBUG1((tempurl));
	if ((ret = get_webpage_alloc(tempurl, &html, &size)) != SR_SUCCESS)
		return ret;

	if ((ret = parse_current_track(html, track, len)) != SR_SUCCESS)
		return ret;

	free(html);
	
	return SR_SUCCESS;
}

error_code get_live365_ids(char *searchstr, unsigned long **ids, int *list_size)
{
	char tempurl[MAX_URL_LEN];
	char *escapedstr;
	char *html;
	int ret;
	u_long size;

	escapedstr = escape_string_alloc(searchstr);
	sprintf(tempurl, m_directory_cgi, escapedstr);
	free(escapedstr);

	if ((ret = get_webpage_alloc(tempurl, &html, &size)) != SR_SUCCESS)
		return ret;
	
	if ((ret = parse_live365_ids(html, searchstr, ids, list_size)) != SR_SUCCESS)
	{
		free(html);
		return ret;
	}

	free(html);

	return SR_SUCCESS;
}

error_code page_get_membername(const char *host, const int port, const char *stream_name, char *membername)
{
	char *searchstr;
	char tempurl[MAX_URL_LEN];
	char test_host[MAX_HOST_LEN];
	int test_port;
	char *html;
	int ret;
	u_long *ids;
	int num_ids = 0;
	int i;
	unsigned long size;

	
	// Format the live365 search string
	if ((searchstr = (char *)malloc(strlen(stream_name)*50)) == NULL)
		return SR_ERROR_CANT_ALLOC_MEMORY;

	strcpy(searchstr, stream_name);
	searchstr = left_str(searchstr, MAX_SEARCH_STR);

	// Search for a match on that search string and return the live365 id's for them
	while(num_ids == 0 && word_count(searchstr) > 0 && threadlib_isrunning(&m_thread))
	{
		get_live365_ids(searchstr, &ids, &num_ids);  
		strip_last_word(searchstr);
	}
	free(searchstr);
	if (num_ids == 0)
		return SR_ERROR_CANT_FIND_MEMBERNAME;

	for(i = 0; i < num_ids && threadlib_isrunning(&m_thread); i++)
	{
		DEBUG1(("%ld\n", ids[i]));
		sprintf(tempurl, m_ratings_cgi, ids[i]);
		if ((ret = get_webpage_alloc(tempurl, &html, &size)) != SR_SUCCESS)
		{
			free(ids);
			return ret;
		}

		if ((ret = parse_ratings_info(html, test_host, &test_port, membername)) != SR_SUCCESS)
		{
			free(ids);
			return ret;
		}

		free(html);

		if ((strcmp(host, test_host) == 0) && (port == test_port))
		{
			free(ids);
			return SR_SUCCESS;
		}
	}

	free(ids);
	return SR_ERROR_CANT_FIND_MEMBERNAME;
}





/*
 * Gets a list of live365 ids from an html source
 */
error_code parse_live365_ids(char *source, char *match, unsigned long **ids, int *list_size)
{
	int size = 0;
	unsigned long *Ids = NULL;
	char *pIdPos, *pMatchPos = 0;
	static char *sIdStr = "javascript:Launch(";
	long lLiveId = -1;


//	dump_file("live_search.txt", source);

	if (!list_size || !source || !match)
		return -1;

	DEBUG1((source));
	pIdPos = source;

	while(threadlib_isrunning(&m_thread))
	{
	
		if ((pIdPos = strstr(pIdPos, sIdStr)) == NULL)
			break;

		// extract the id from the text
		pIdPos += strlen(sIdStr);
		if ((lLiveId = atol(pIdPos)) == 0)
			return SR_ERROR_CANT_GET_LIVE365_ID;

		// add that to the array
		{
			size++;
			if ((Ids = realloc(Ids, sizeof(unsigned long)*size)) == NULL)
				return SR_ERROR_CANT_ALLOC_MEMORY;
			Ids[size-1] = lLiveId;
		}
		pMatchPos++;
	}
	*ids = Ids;
	*list_size = size;
	return SR_SUCCESS;
}

/*
 * Gets the broadcasts ip/port from http://www.live365.com/cgi-bin/ratings.cgi?alsotry=1&stream=%d&genre=g
 * parses: var gStreamUrl			= unescape("166.90.143.141:22144");
 * parses: var gMemberName			= "dj_art";	
 */ 
error_code parse_ratings_info(const char *html, char *ip, int *port, char *membername)
{
	const char *gplayer	= "var gPlayer";
	const char *stream_url = "var gStreamUrl";
	const char *unescape = "unescape(\"";
	const char *membername_tag = "var gMemberName";
	const char *equals_tag = "= \"";
	char *p, *t;


#if DEBUG
	{
		FILE *fp = fopen("c:/temp/dump.html", "wb");
		fprintf(fp, "%s", html);
		fclose(fp);
	}
#endif

	// skip past junk
	if ((p = strstr(html, gplayer)) == NULL)
		return SR_ERROR_CANT_FIND_IP_PORT;


	// get the membor name
	if ((p = strstr(p, membername_tag)) == NULL)
		return SR_ERROR_CANT_FIND_MEMBERNAME;
	
	if ((t = strstr(p, equals_tag)) == NULL)
		return SR_ERROR_CANT_FIND_MEMBERNAME;

	t += strlen(equals_tag);
	if (sscanf(t, "%[^\"]\"", membername) != 1)
		return SR_ERROR_CANT_FIND_MEMBERNAME;


	// Get the ip/port
	if ((p = strstr(p, stream_url)) == NULL)
		return SR_ERROR_CANT_FIND_IP_PORT;

	if ((t = strstr(p, unescape)) == NULL)
		return SR_ERROR_CANT_FIND_IP_PORT;

	// 166.90.143.141:22144");
	t += strlen(unescape);
	if (sscanf(t, "%[^:]:%d", ip, port) != 2)
		return SR_ERROR_CANT_FIND_IP_PORT;

	return SR_SUCCESS;
}

/*
 * Gets the top track from: http://www.live365.com:89/pls/front?handler=playlist&cmd=view&handle=<member_name>
 */
error_code parse_current_track(const char *html, char *track, int *len)
{
	const char *time_tag = "time:\"";
	char titlebuf[MAX_TRACK_LEN] = {'\0'};
	char albumbuf[MAX_TRACK_LEN] = {'\0'};
	char filenamebuf[MAX_TRACK_LEN] = {'\0'};
	char artistbuf[MAX_TRACK_LEN] = {'\0'};

	char *p;
	int minutes, seconds;

	// find the title var
	find_and_scan(html, "title:\"", titlebuf);
	find_and_scan(html, "filename:\"", filenamebuf);
	find_and_scan(html, "album:\"", albumbuf);
	find_and_scan(html, "artist:\"", artistbuf);
	
	*track = '\0';	// Clear the track for strcat's
	if (*artistbuf)
		sprintf(track, "%s - ", artistbuf);
	if (*titlebuf)
		strcat(track, titlebuf);
	else if(*filenamebuf)
		strcpy(track, filenamebuf);
	else
		return SR_ERROR_CANT_FIND_TRACK_NAME;

	if (*albumbuf)
	{
		strcat(track, " - ");
		strcat(track, albumbuf);
	}

	// Get the time tag
	if ((p = strstr(html, time_tag)) == NULL)
		return SR_ERROR_CANT_FIND_TIME_TAG;
	p += strlen(time_tag);
	if (sscanf(p, "%d:%d\"", &minutes, &seconds) != 2)
		return SR_ERROR_CANT_FIND_TIME_TAG;

	*len = (minutes*60)+seconds;

	return SR_SUCCESS;
}


error_code find_and_scan(const char *where, const char *what, char *input)
{
	char *p = strstr(where, what);

	if (p == NULL)
		return SR_EEROR_CANT_FIND_SUBSTR;

	p += strlen(what);
	if (sscanf(p, "%[^\"]\"", input) != 1)
		return SR_EEROR_CANT_FIND_SUBSTR;

	return SR_SUCCESS;
}

error_code get_webpage_alloc(const char *url, char **buffer, unsigned long *size)
{
	char *proxy = NULL;

	if (*m_proxyurl)
		proxy = m_proxyurl;

	return inet_get_webpage_alloc(&m_sock, url, proxy, buffer, size);
}
