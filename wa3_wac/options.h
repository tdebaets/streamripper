#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "lib/rip_manager.h"

typedef struct GUI_OPTIONSst
{
	BOOL	m_add_finshed_tracks_to_playlist;
	char	localhost[MAX_HOST_LEN];		// hostname of 'localhost' 
	BOOL	use_old_playlist_ret;
} GUI_OPTIONS;

extern void options_dialog_show(HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
extern BOOL options_load(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
extern BOOL options_save(RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);

#endif //__OPTIONS_H__
