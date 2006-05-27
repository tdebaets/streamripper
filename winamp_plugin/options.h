#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include "rip_manager.h"
#include "dsp_sripper.h"

void options_dialog_show (HINSTANCE inst, HWND parent, RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_load (RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);
BOOL options_save (RIP_MANAGER_OPTIONS *opt, GUI_OPTIONS *guiOpt);

#endif //__OPTIONS_H__
