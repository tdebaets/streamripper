/* sripperwnd.c - jonclegg@yahoo.com
 * 
 * Portions from the ExampleVisData example from the Winamp3 SDK
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


#include "sripper.h"
#include "sripperwnd.h"
#include "resource.h"
#include <assert.h>

#include "bfc/std.h"
#include "common/buttwnd.h"
#include "common/textbar.h"
#include "common/corehandle.h"
#include "bfc/notifmsg.h" 
#include "bfc/canvas.h"
#include "pledit/svc_pldir.h"
#include "pledit/playlist.h"



#include <string>
extern "C" {
#include "lib/util.h"
}

#define snprintf	_snprintf

enum {
	// Core Buttons Tabsheet IDs
	SRB_START = 1,
	SRB_STOP,
	SRB_OPTIONS,
	SRB_RELAY,
};

enum {
	SR_WINDOW_WIDTH	= 265,
	SR_WINDOW_HEIGHT = 91,
};


enum {
	// Core Buttons and Frames Tabsheets
	EXB_LEFT_BUTTON_OFFSET = 5,
	EXB_TOP_BUTTON_OFFSET = 67,
	EXB_BUTTON_WIDTH  = 50,
	EXB_BUTTON_HEIGHT = 20,
	EXB_BUTTON_FONT_SIZE = 14,
	EXB_TEXT_FONT_SIZE = 14,

	EXB_TOP_LABEL_OFFSET = 0,
	EXB_LEFT_LABEL_OFFSET = 20,
	EXB_LABEL_INC_AMOUNT = 12,
	EXB_LABEL_WIDTH = SR_WINDOW_WIDTH-EXB_LEFT_LABEL_OFFSET-20
};

#define NO_STREAM_MSG	"Winamp is not listening to a stream"


// static membor vars
//////////////////////////////////////////////////////////////
SRipperWnd* SRipperWnd::m_pcurInst = NULL;


// =========================================================================
//
//  SRIPPERDATA: Methods required by Window Creation Services
//
const char *SRipperWnd::getWindowTypeName() { return /**/"Streamripper for Winamp3"/*EDITME*/; }
GUID SRipperWnd::getWindowTypeGuid() { return the->getGUID(); }
void SRipperWnd::setIconBitmaps(ButtonWnd *button) {
  // Note that IDB_TAB_XXXXXX defines come from resource.h and the data is in resource.rc
  button->setBitmaps(the->gethInstance(), IDB_TAB_NORMAL, NULL, IDB_TAB_HILITED, IDB_TAB_SELECTED);
}


SRipperWnd::SRipperWnd() 
: SRIPPERWND_PARENT() 
{  

	m_pcurInst = this;
	m_ripState = NOT_RIPPING;

	setName("Streamripper for Winamp3");

	m_start = new ButtonWnd();
	m_start->setButtonText("Start", EXB_BUTTON_FONT_SIZE);
	m_start->setNotifyId(SRB_START);
	m_start->setNotifyWindow(this);
	addChild(m_start, EXB_LEFT_BUTTON_OFFSET, EXB_TOP_BUTTON_OFFSET,
		EXB_BUTTON_WIDTH, EXB_BUTTON_HEIGHT, 0/*"Invalidate on Resize*/);

	m_stop = new ButtonWnd();
	m_stop->setButtonText("Stop", EXB_BUTTON_FONT_SIZE);
	m_stop->setNotifyId(SRB_STOP);
	m_stop->setNotifyWindow(this);
	addChild(m_stop, EXB_BUTTON_WIDTH+10, EXB_TOP_BUTTON_OFFSET,
		EXB_BUTTON_WIDTH, EXB_BUTTON_HEIGHT, 0/*"Invalidate on Resize*/);

	m_options = new ButtonWnd();
	m_options->setButtonText("Options", EXB_BUTTON_FONT_SIZE);
	m_options->setNotifyId(SRB_OPTIONS);
	m_options->setNotifyWindow(this);
	addChild(m_options, SR_WINDOW_WIDTH-EXB_BUTTON_WIDTH-5, EXB_TOP_BUTTON_OFFSET,
		EXB_BUTTON_WIDTH, EXB_BUTTON_HEIGHT, 0/*"Invalidate on Resize*/);

	m_titleText = new TextBar();
	m_titleText->setName("Winamp is not listening to a stream");
	m_titleText->setTextSize(EXB_TEXT_FONT_SIZE);
	addChild(m_titleText, EXB_LEFT_LABEL_OFFSET, EXB_TOP_LABEL_OFFSET, 
		EXB_LABEL_WIDTH, 20);

	m_trackText = new TextBar();
	m_trackText->setName("[250kb] Big track of doom.mp3");
	m_trackText->setTextSize(EXB_TEXT_FONT_SIZE);
	addChild(m_trackText, EXB_LEFT_LABEL_OFFSET, 
		EXB_TOP_LABEL_OFFSET + 17, EXB_LABEL_WIDTH, 20);

	m_infoText1 = new TextBar();
	m_infoText1->setName("shoutcast/sparc v1.5");
	m_infoText1->setTextSize(EXB_TEXT_FONT_SIZE);
	addChild(m_infoText1, EXB_LEFT_LABEL_OFFSET, 
		EXB_TOP_LABEL_OFFSET + 32, EXB_LABEL_WIDTH, 20);

	m_infoText2 = new TextBar();
	m_infoText2->setName("info2: 128kbit metaint: bla");
	m_infoText2->setTextSize(EXB_TEXT_FONT_SIZE);
	addChild(m_infoText2, EXB_LEFT_LABEL_OFFSET, 
		EXB_TOP_LABEL_OFFSET + 44, EXB_LABEL_WIDTH, 20);


	options_load(&m_rmoOpt, &m_guiOpt);
	ClearTextFields();
}

SRipperWnd::~SRipperWnd() 
{
	api->core_delCallback(0, this);
	rip_manager_stop();
	options_save(&m_rmoOpt, &m_guiOpt);
	m_pcurInst = NULL;
}


int SRipperWnd::onInit() {
	int retval = SRIPPERWND_PARENT::onInit();

	setPreferences( SUGGESTED_W, SR_WINDOW_WIDTH ); 
	setPreferences( SUGGESTED_H, SR_WINDOW_HEIGHT ); 
	setPreferences( MAXIMUM_W, SR_WINDOW_WIDTH ); 
	setPreferences( MAXIMUM_H, SR_WINDOW_HEIGHT ); 
	setPreferences( MINIMUM_W, SR_WINDOW_WIDTH ); 
	setPreferences( MINIMUM_H, SR_WINDOW_HEIGHT ); 

	api->core_addCallback(0, this);	// for on status change

	m_hInst = the->gethInstance(); //api->main_gethInstance();
	m_hWnd = gethWnd(); 


	appcmds_addCmd("RLY", SRB_RELAY); 
	// Add your AppCmdI to the guiobjectwnd 
	getGuiObject()->guiobject_addAppCmds(this); 

	return retval;
}

int 
SRipperWnd::corecb_onStarted()
{
	SetUrlBasedOfWhatsPlaying();
	return 1;
}

void 
SRipperWnd::appcmds_onCommand(int id, const RECT *buttonRect)
{
	char s[255];
	sprintf(s, "%d\n", id);
	OutputDebugString(s);
	switch(id)
	{
	case SRB_RELAY:
		{
		char s[255];
		sprintf(s, "http://localhost:%d", rip_mananger_get_relay_port());
		QueuePlaylistItem(s);
		}
	}
}


void 
SRipperWnd::SetUrlBasedOfWhatsPlaying()
{
	const char *szurl = api->core_getCurrent(0);
	if (szurl == NULL || strstr(szurl, "http://") == NULL)
	{
		m_titleText->setName(NO_STREAM_MSG);
		return;
	}
	strcpy(m_rmoOpt.url, szurl);
	std::string msg = "Press start to rip ";
	msg +=  szurl;
	m_titleText->setName(msg.c_str());
}

int 
SRipperWnd::handleChildNotify (int msg, int objId, int param1, int param2)
{
	if (msg != ChildNotify::BUTTON_LEFTPUSH)
		return 0;
	error_code ret;

	switch(objId) 
	{
	case SRB_START:
		OutputDebugString(("Button pushed corresponds to SRB_START ButtonID\n"));
		if (strncmp(m_rmoOpt.url, "http://", strlen("http://")) != 0)
			return 1;
		if (m_ripState != NOT_RIPPING)
			return 1;

		m_ripState = START_RIPPING;
		if ((ret = rip_manager_start(ripCallback, &m_rmoOpt)) != SR_SUCCESS)
		{
			MessageBox(m_hWnd, rip_manager_get_error_str(ret), "Failed to connect to stream", MB_ICONSTOP);
			m_ripState = NOT_RIPPING;
			return 1;
		}
		break;
	case SRB_STOP:
		rip_manager_stop();
		m_ripState = NOT_RIPPING;
		ClearTextFields();
		break;
	case SRB_OPTIONS: 
		{
		options_dialog_show(m_hInst, m_hWnd, &m_rmoOpt, &m_guiOpt);
		}
		break;
	}
	return 1;
}

void 
SRipperWnd::QueuePlaylistItem(const std::string& item)
{
	svc_plDir *pldir = SvcEnumByGuid<svc_plDir>();
	assert(pldir); if (!pldir) return;
	PlaylistHandle hand = pldir->getCurrentlyOpen(); 
	Playlist *pl = pldir->getPlaylist(hand);
	assert(pldir); if (!pl) return;
	pl->addPlaylistItem(item.c_str());
}

int 
SRipperWnd::childNotify(RootWnd *which, int msg, int param1, int param2)
{
	 // for an interesting reason, it is valid to be given NULL as a which pointer.
	if (which != NULL) 
	{
		if (handleChildNotify(msg, which->getNotifyId(), param1, param2) == -1) 
		{
			return 0;
		}
	}
	return SRIPPERWND_PARENT::childNotify(which, msg, param1, param2);
}

void 
SRipperWnd::ClearTextFields()
{
	SetUrlBasedOfWhatsPlaying();
	m_trackText->setName("");
	m_infoText1->setName("");
	m_infoText2->setName("");
	getGuiObject()->guiobject_setStatusText("");
}


void 
SRipperWnd::ripCallback(int message, void *data)
{
	if (!m_pcurInst)
		return;

	RIP_MANAGER_INFO *info;
	ERROR_INFO *err;
	switch(message)
	{
		case RM_UPDATE:
			{
			info = (RIP_MANAGER_INFO*)data;
			m_pcurInst->m_titleText->setName(info->streamname);

			char strsize[255];
			const int STEMP_SIZE = 255;
			char stemp[STEMP_SIZE];
			format_byte_size(strsize, info->filesize);
			snprintf(stemp, STEMP_SIZE, "[%s] - %s", strsize, info->filename);
			m_pcurInst->m_trackText->setName(stemp);

			m_pcurInst->m_infoText1->setName(info->server_name);

			char stemp2[STEMP_SIZE];
			if(info->meta_interval)
			{
				snprintf(stemp2, STEMP_SIZE, "MetaInt:%d", info->meta_interval);
			}
			else
			{
				snprintf(stemp2, STEMP_SIZE, "No track data");
			}
			snprintf(stemp, STEMP_SIZE, "%dkbit %s", info->bitrate, stemp2);
			m_pcurInst->m_infoText2->setName(stemp);


			GuiObject *pgui = m_pcurInst->getGuiObject();
			switch(info->status)
			{
				case RM_STATUS_BUFFERING:
						pgui->guiobject_setStatusText("Buffering... ");
						break;
				case RM_STATUS_RIPPING:
						pgui->guiobject_setStatusText("Ripping...");
						break;
				case RM_STATUS_RECONNECTING:
						pgui->guiobject_setStatusText("Re-connecting...");
						break;
				default:
					OutputDebugString("************ what am i doing here?");
			}


			}
			break;
		case RM_ERROR:
			err = (ERROR_INFO*)data;
			MessageBox(m_pcurInst->m_hWnd, err->error_str, "Streamripper", MB_SETFOREGROUND);
			break;
		case RM_DONE:
			m_pcurInst->m_ripState = NOT_RIPPING;
			break;
		case RM_TRACK_DONE:
			// NOP?
			break;
		case RM_NEW_TRACK:
			break;
		case RM_STARTED:
			m_pcurInst->m_ripState = RIPPING;
			break;
		default:
			{
			char s[255];
			sprintf(s, "Unknown RipMsg: %x\n", message);
			OutputDebugString(s);
			}
	}
}
