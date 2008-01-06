/* plugin_dll_main.c
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
 *
 * References:
 *   Windows Handles can be passed across processes:
 *   http://www.codeproject.com/win32/InsideWindHandles.asp?df=100&forumid=136677&exp=0&select=999258
 *
 * Local access only to named pipes:
 *   http://www.codeproject.com/threads/ipcworkshop.asp
 *   http://blogs.msdn.com/oldnewthing/archive/2004/03/12/88572.aspx
 * However, win9x does not have the capability of creating named pipe
 *   http://support.microsoft.com/kb/177696
 *   http://www.microsoft.com/technet/archive/win98/reskit/part6/wrkc29.mspx?mfr=true
 */
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <windows.h>
#include "resource.h"
#include "winamp_dll.h"
#include "gen.h"
//#include "dsp_sripper.h"
#include "registry.h"

static int  init ();
static void config (); 
static void quit ();
static void create_pipes (void);
static void destroy_pipes (void);
static void launch_pipe_threads (void);
static int spawn_streamripper (void);
static void CALLBACK timer_proc (HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime);

void write_pipe (char* msg);

static HANDLE m_hpipe_dll_read = 0;
static HANDLE m_hpipe_dll_write = 0;
static HANDLE m_hpipe_exe_read = 0;
static HANDLE m_hpipe_exe_write = 0;

static TCHAR m_szToopTip[] = "Streamripper For Winamp";
static int m_enabled;
static UINT m_timer_id;

void debug_popup (char* fmt, ...);

//extern void dock_init (HWND hwnd);

/*****************************************************************************
 * Winamp Interface Functions
 *****************************************************************************/
winampGeneralPurposePlugin g_plugin = {
    GPPHDR_VER,
    m_szToopTip,
    init,
    config,
    quit
};

BOOL WINAPI
_DllMainCRTStartup (HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

__declspec(dllexport) winampGeneralPurposePlugin*
winampGetGeneralPurposePlugin ()
{
    return &g_plugin;
}

int
init ()
{
    int rc;

    winamp_dll_init ();
    create_pipes ();
    launch_pipe_threads ();
    rc = spawn_streamripper ();
    if (rc == 0) {
	/* Failure spawning streamripper */
	m_timer_id = 0;
	destroy_pipes ();
	return 1;
    }

    /* Success spawning streamripper */
    dock_init (g_plugin.hwndParent);     // This function is obsolete
    hook_winamp ();
    write_pipe ("Hello World");
    m_timer_id = SetTimer (NULL, 0, 5000, (TIMERPROC) timer_proc);

    return 0;
}

INT_PTR CALLBACK
EnableDlgProc (HWND hwndDlg, UINT umsg, WPARAM wParam, LPARAM lParam)
{
    switch (umsg)
    {

    case WM_INITDIALOG:
	{
	    HWND hwndctl = GetDlgItem(hwndDlg, IDC_ENABLE);
	    SendMessage(hwndctl, BM_SETCHECK, 
			(LPARAM)m_enabled ? BST_CHECKED : BST_UNCHECKED,
			(WPARAM)NULL);
	}

    case WM_COMMAND:
	switch(LOWORD(wParam))
	{
	case IDC_OK:
	    {
		HWND hwndctl = GetDlgItem(hwndDlg, IDC_ENABLE);
		m_enabled = SendMessage(hwndctl, BM_GETCHECK, (LPARAM)NULL, (WPARAM)NULL);
		EndDialog(hwndDlg, 0);
	    }
	}
	break;
    case WM_CLOSE:
	EndDialog(hwndDlg, 0);
	break;
    }

    return 0;
}

void
config ()
{
    BOOL prev_enabled = m_enabled;

    DialogBox(g_plugin.hDllInstance, 
	      MAKEINTRESOURCE(IDD_ENABLE), 
	      g_plugin.hwndParent,
	      EnableDlgProc);

    if (m_enabled) {
	if (!prev_enabled) {
	    init();
	}
    }
    else if(prev_enabled) {
	quit();
    }
}

void
quit()
{
    int num_written;

    WriteFile (m_hpipe_dll_write, "q", 1, &num_written, NULL);
    KillTimer (NULL, m_timer_id);
    dock_unhook_winamp ();
    destroy_pipes ();
}

/*****************************************************************************
 * Private functions
 *****************************************************************************/
static void
display_last_error (void)
{
    char buf[1023];
    LPVOID lpMsgBuf;
    FormatMessage( 
	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	FORMAT_MESSAGE_FROM_SYSTEM | 
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
	GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	(LPTSTR) &lpMsgBuf,
	0,
	NULL 
    );
    _snprintf (buf, 1023, "Error: %s\n", lpMsgBuf);
    buf[1022] = 0;
    MessageBox (g_plugin.hwndParent, buf, "SR DLL Error", MB_OK);
    LocalFree( lpMsgBuf );
}

static void
create_pipes (void)
{
    HANDLE tmp;
    BOOL rc;
    SECURITY_ATTRIBUTES sa;

    sa.bInheritHandle = TRUE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;

    /* Create a pipe that the child process (exe) will read from */
    tmp = NULL;
    rc = CreatePipe (
	&m_hpipe_exe_read,   // pointer to read handle
	&tmp,                // pointer to write handle
	&sa,                 // pointer to security attributes
	0                    // pipe size
    );
    if (rc == 0) {
	display_last_error ();
	exit (1);	     // ?
    }
    DuplicateHandle (
	GetCurrentProcess(),
	tmp,
	GetCurrentProcess(),
	&m_hpipe_dll_write,
	DUPLICATE_SAME_ACCESS,
	FALSE,
	DUPLICATE_SAME_ACCESS
    );
    CloseHandle (tmp);

    /* Create a pipe that the child process (exe) will write to */
    tmp = NULL;
    rc = CreatePipe (
	&tmp,                // pointer to read handle
	&m_hpipe_exe_write,  // pointer to write handle
	&sa,                 // pointer to security attributes
	0                    // pipe size
    );
    if (rc == 0) {
	display_last_error ();
	exit (1);	     // ?
    }
    DuplicateHandle (
	GetCurrentProcess(),
	tmp,
	GetCurrentProcess(),
	&m_hpipe_dll_read,
	DUPLICATE_SAME_ACCESS,
	FALSE,
	DUPLICATE_SAME_ACCESS
    );
    CloseHandle (tmp);
}

static void
destroy_pipes (void)
{
    CloseHandle (m_hpipe_dll_read);
    CloseHandle (m_hpipe_dll_write);
}

static void
pipe_reader (void* arg)
{
    char msgbuf;
    int num_read;
    while (1) {
	ReadFile (m_hpipe_dll_read, &msgbuf, 1, &num_read, 0);
    }
}

void
write_pipe (char* msg)
{
    unsigned int i;
    int num_written;
    int len = strlen (msg);
    char eom = '\x03';
    BOOL rc;

    for (i=0; i<strlen(msg); i++) {
	rc = WriteFile (m_hpipe_dll_write, &msg[i], 1, &num_written, 0);
	if (rc == 0) {
	    display_last_error ();
	    return;
	}
    }
    WriteFile (m_hpipe_dll_write, &eom, 1, &num_written, 0);
}

static void
launch_pipe_threads (void)
{
#if defined (commentout)
    _beginthread ((void*) pipe_writer, 0, 0);
#endif
}

static int
spawn_exe (char* cwd, char* exe)
{
    char cmd[1024];
    STARTUPINFO startup_info;
    PROCESS_INFORMATION piProcInfo;
    BOOL rc;
    DWORD creation_flags;

    ZeroMemory (&startup_info, sizeof(STARTUPINFO));
    ZeroMemory (&piProcInfo, sizeof(PROCESS_INFORMATION));

    //creation_flags = 0;
    creation_flags = CREATE_NEW_CONSOLE;
    startup_info.cb = sizeof(STARTUPINFO); 
    _snprintf (cmd, 1024, "%s %d %d", exe, m_hpipe_exe_read, m_hpipe_exe_write);

    rc = CreateProcess (
		NULL,           // executable name
		cmd,	        // command line 
		NULL,           // process security attributes 
		NULL,           // primary thread security attributes 
		TRUE,           // handles are inherited 
		creation_flags, // creation flags 
		NULL,           // use parent's environment 
		cwd,            // current directory 
		&startup_info,  // STARTUPINFO pointer
		&piProcInfo);   // receives PROCESS_INFORMATION 
    return rc;

#if defined (commentout)
    if (rc == 0) {
	char buf[1023];
	LPVOID lpMsgBuf;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);
	_snprintf (buf, 1023, "RC=%d pid=%d error=%s\n", rc, piProcInfo.dwProcessId, lpMsgBuf);
	buf[1022] = 0;
	MessageBox (g_plugin.hwndParent, buf, "Hi guys", MB_OK);
	LocalFree( lpMsgBuf );
    }
#endif
}

/* Look for streamripper in a few different places.
   Return 0 if failed to spawn wstreamripper. */
static int
spawn_streamripper (void)
{
    int rc = 0;
    char* sr_home_env;
    char cwd[_MAX_PATH];
    char exe[_MAX_PATH];
    char* exe_name = "wstreamripper.exe";

    /* If environment variables set, try there */
    sr_home_env = getenv ("STREAMRIPPER_HOME");
    if (sr_home_env) {
	strncpy (cwd, sr_home_env, _MAX_PATH);
	cwd[_MAX_PATH-1] = 0;
	_snprintf (exe, _MAX_PATH, "%s\\%s", cwd, exe_name);
	rc = spawn_exe (cwd, exe);
	if (rc) return rc;
    }

    /* Next, try cwd */
    /* To be written */

    /* Next, try where the registry says it was installed */
    if (get_string_from_registry (cwd, HKEY_CURRENT_USER, 
	    TEXT("Software\\Streamripper"),
	    TEXT("")))
    {
	_snprintf (exe, _MAX_PATH, "%s\\%s", cwd, exe_name);
	rc = spawn_exe (cwd, exe);
	if (rc) return rc;
    }

    /* Next, try cannonical locations */
    rc = spawn_exe ("C:\\Program Files\\streamripper", "C:\\Program Files\\streamripper\\wstreamripper.exe");
    return rc;
}

static void CALLBACK
timer_proc (
	    HWND hwnd,     // handle of window for timer messages
	    UINT uMsg,     // WM_TIMER message
	    UINT idEvent,  // timer identifier
	    DWORD dwTime   // current system time
)
{
    char url[MAX_URL_LEN];
    winamp_poll (url);
}
