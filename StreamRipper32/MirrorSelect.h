#if !defined(AFX_MIRRORSELECT_H__A06FFEEC_D021_443A_97E1_B4390E73C5C3__INCLUDED_)
#define AFX_MIRRORSELECT_H__A06FFEEC_D021_443A_97E1_B4390E73C5C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MirrorSelect.h : header file
//
#include <afxtempl.h>

/////////////////////////////////////////////////////////////////////////////
// CMirrorSelect dialog

class CMirrorSelect : public CDialog
{
// Construction
public:
	CMirrorSelect(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMirrorSelect)
	enum { IDD = IDD_MIRRORSELECT };
	CListBox	m_Mirrors;
	//}}AFX_DATA


	CArray<CString, CString>	m_ClusterEntries;
	CArray<CString, CString>	m_ClusterEntryDescs;
	int							m_Selected;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMirrorSelect)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMirrorSelect)
	afx_msg void OnDblclkMirrors();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MIRRORSELECT_H__A06FFEEC_D021_443A_97E1_B4390E73C5C3__INCLUDED_)
