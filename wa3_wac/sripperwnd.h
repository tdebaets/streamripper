/* srippernd.c - jonclegg@yahoo.com
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

#ifndef _SRIPPERWND_H
#define _SRIPPERWND_H

//#include "bfc/virtualwnd.h"
#include "common/contwnd.h"
#include "bfc/appcmds.h"
#include "bfc/virtualwnd.h"
#include "bfc/wnds/buttwnd.h"
#include "studio/corecb.h"
#include <string>
extern "C" {
#include "lib/rip_manager.h"
#include "options.h"
}


#define SRIPPERWND_PARENT ContWnd	//VirtualWnd

class TextBar;

class SRipperWnd : public SRIPPERWND_PARENT, 
						  public CoreCallbackI,
						  public AppCmdsI
{
public:
	SRipperWnd();
	virtual ~SRipperWnd();

	static const char *getWindowTypeName();
	static GUID getWindowTypeGuid();
	static void setIconBitmaps(ButtonWnd *button);    

	virtual int onInit();
//	virtual void timerCallback(int id);
//	virtual int onPaint(Canvas *canvas);

private:
	int handleChildNotify (int msg, int objId, int param1, int param2);
	virtual int childNotify(RootWnd *which, int msg, int param1, int param2);
	virtual int corecb_onStarted();
	virtual void appcmds_onCommand(int id, const RECT *buttonRect);

	static void ripCallback(int message, void *data);
	
	void ClearTextFields();
	void SetUrlBasedOfWhatsPlaying();
	void QueuePlaylistItem(const std::string&);

	static SRipperWnd* m_pcurInst;
	RIP_MANAGER_OPTIONS	m_rmoOpt;
	GUI_OPTIONS m_guiOpt;

	ButtonWnd *m_start;
	ButtonWnd *m_stop;
	ButtonWnd *m_options;

	TextBar *m_titleText;
	TextBar *m_trackText;
	TextBar *m_infoText1;
	TextBar *m_infoText2;

	HINSTANCE m_hInst;
	HWND m_hWnd;

	enum RipStates {
		START_RIPPING,
		RIPPING,
		NOT_RIPPING
	};
	RipStates m_ripState;

};

#endif _SRIPPERWND_H
