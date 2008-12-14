/* touch_page.c - jonclegg@yahoo.com
 * requests a page on the sourceforge site to report who's ripping what.. yes spyware. i suck
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

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "touch_page.h"
#include "socklib.h"
#include "http.h"
#include "threadlib.h"
#include "..\lib\util.h"


/**********************************************************************************
 * Public functions
 **********************************************************************************/
void touch_page_listing(char *proxyurl, char *streamname, char *track, int bitrate);

/**********************************************************************************
 * Private Stuff
 **********************************************************************************/
static void thread_touch_page(void *url);

static char	m_touch_url[MAX_URL_LEN];
static char	m_proxy_url[MAX_URL_LEN];

void touch_page_listing(char *proxyurl, char *streamname, char *track, int bitrate)
{
	static time_t started = 0;

	THREAD_HANDLE h;	// we don't care about this
	time_t now;
	char *streamname_esc = escape_string_alloc(streamname);
	char *track_esc = escape_string_alloc(track);

	if (started == 0)
		time(&started);
	time(&now);
	
	_snprintf(m_touch_url, MAX_URL_LEN, 
		"http://streamripper.sourceforge.net/listing.php"
		"?streamname=%s"
		"&track=%s"
		"&runningtime=%d"
		"&bitrate=%d",
			streamname_esc,
			track_esc,
			now-started,
			bitrate);

	httplib_construct_page_request(m_touch_url, proxyurl!=NULL, m_touch_url);
	strcpy(m_proxy_url, proxyurl);

	threadlib_beginthread(&h, thread_touch_page);
}


void thread_touch_page(void *notused)
{

	URLINFO url_info;
	HSOCKET sock;

	if (m_proxy_url[0])
	{
		if (httplib_parse_url(m_proxy_url, &url_info) != SR_SUCCESS)
			return;
	}
	else if (httplib_parse_url(m_touch_url, &url_info) != SR_SUCCESS)
	{
		return;
	}

	if (socklib_init() != SR_SUCCESS)
		return;

	if (socklib_open(&sock, url_info.host, url_info.port) != SR_SUCCESS)
		return;

	socklib_sendall(&sock, m_touch_url, strlen(m_touch_url));
	socklib_close(&sock);
}
