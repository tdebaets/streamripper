#ifndef __DSP_SRIPPER_H__
#define __DSP_SRIPPER_H__

typedef struct GUI_OPTIONSst
{
	BOOL	m_add_finshed_tracks_to_playlist;
	BOOL	m_start_minimized;
	POINT	oldpos;
	BOOL	m_enabled;
	char	localhost[MAX_HOST_LEN];		// hostname of 'localhost' 
	char	default_skin[MAX_PATH];
	BOOL	use_old_playlist_ret;
} GUI_OPTIONS;

#endif //__DSP_SRIPPER_H__
