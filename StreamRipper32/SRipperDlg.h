/*
Oddsock - StreamRipper32 (Win32 front end to streamripper library)
Copyright (C) 2000  Oddsock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// SRipperDlg.h : header file
//
#include "ResizableDialog.h"

#if !defined(AFX_SRIPPERDLG_H__69F60A13_1005_11D4_B1AF_00E0293B4DC4__INCLUDED_)
#define AFX_SRIPPERDLG_H__69F60A13_1005_11D4_B1AF_00E0293B4DC4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSRipperDlg dialog

class CSRipperDlg : public CResizableDialog
{
// Construction
public:
	CSRipperDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CSRipperDlg)
	enum { IDD = IDD_SRIPPER_DIALOG };
	CEdit	m_URLctrl;
	CEdit	m_RelayPortCtrl;
	CButton	m_ConnectRelay;
	CListCtrl	m_cListCtrl;
	CEdit	m_MaxBytesCtl;
	CButton	m_Folder;
	CButton	m_OKctrl;
	CEdit	m_Destinationctrl;
	CButton	m_StopRipping;
	CButton	m_Refresh;
	CButton	m_Ripaway;
	CListBox	m_Broadcasts;
	CComboBox	m_Genre;
	CString	m_Destination;
	CString	m_SelectedBroadcast;
	CString	m_GenreValue;
	CString	m_SongTitle;
	int		m_MaxBytes;
	CString	m_StreamName;
	int		m_RelayPort;
	CString	m_Bytes;
	CString	m_Status;
	CString	m_URL;
	BOOL	m_addID3;
	BOOL	m_Overwrite;
	BOOL	m_AddSeq;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSRipperDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CSRipperDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnRipaway();
	afx_msg void OnEditchangeGenre();
	afx_msg void OnRefresh();
	afx_msg void OnStopripping();
	virtual void OnOK();
	afx_msg void OnUpdateBytes();
	afx_msg void OnUpdateSongtitle();
	afx_msg void OnUpdateDestination();
	afx_msg void OnFolder();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblclkListCtrl(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnConnectRelay();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
		void OnDblclkBroadcasts();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRIPPERDLG_H__69F60A13_1005_11D4_B1AF_00E0293B4DC4__INCLUDED_)
