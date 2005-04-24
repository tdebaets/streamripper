/* riplive365.c - jonclegg@yahoo.com
 * ripper lib for live365..mainly just hosts the live365info.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "riplive365.h"
#include "live365info.h"
#include "srtypes.h"
#include "threadlib.h"

/* Note; 
 * Need to do something about track info failing, right now it's really stupid about trying to make 
 * correct tracks
 */

/*********************************************************************************
 * Public functions
 *********************************************************************************/
error_code		riplive365_init(IO_DATA_INPUT *in, IO_GET_STREAM *getstream, char *stream_name, 
									char *url, char *proxyurl);
void			riplive365_destroy();

/*********************************************************************************
 * Private functions
 *********************************************************************************/
static error_code	getdata(char *data, char *track);
static void			live365info_callback(int message, void *data);


/*********************************************************************************
 * Private vars
 *********************************************************************************/
#define BUFSIZE	1024

static IO_DATA_INPUT	*m_in;
static char				m_current_track[MAX_TRACK_LEN];

error_code riplive365_init(IO_DATA_INPUT *in, IO_GET_STREAM *getstream, char *stream_name, 
									char *url, char *proxyurl)
{
	int ret;

	if (!in || !stream_name || !url)
		return SR_ERROR_INVALID_PARAM;

	m_in = in;

	getstream->get_data = getdata;
	getstream->getsize = BUFSIZE;

	// This might need to move to getdata, but then I would need to keep track of the url, proxyurl, etc.. 
	// and whether or not it's started
	if ((ret = live365info_track_nofity(url, proxyurl, stream_name, live365info_callback)) != SR_SUCCESS)
		return ret;


	return SR_SUCCESS;
}

void riplive365_destroy()
{
	live365info_terminate();
	m_current_track[0] = '\0';
	m_in = NULL;
}

void live365info_callback(int message, void *data)
{
	LIVE365_TRACK_INFO *trackinfo;

	switch(message)
	{
		case SRLIM_TRACK_CHANGED:
			trackinfo = (LIVE365_TRACK_INFO *)data;
			strcpy(m_current_track, trackinfo->track);

			break;
		case SRLIM_ERROR:
			/* NOT SURE YET */
			strcpy(m_current_track, NO_TRACK_STR);
			break;
	}
}


error_code getdata(char *data, char *track)
{
	int ret;
	if ((ret = m_in->get_data(data, BUFSIZE)) <= 0)
		return SR_ERROR_RECV_FAILED;
	
	strcpy(track, m_current_track);

	return SR_SUCCESS;
}

