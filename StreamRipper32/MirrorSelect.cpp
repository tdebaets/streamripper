// MirrorSelect.cpp : implementation file
//

#include "stdafx.h"
#include "SRipper.h"
#include "MirrorSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMirrorSelect dialog


CMirrorSelect::CMirrorSelect(CWnd* pParent /*=NULL*/)
	: CDialog(CMirrorSelect::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMirrorSelect)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMirrorSelect::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMirrorSelect)
	DDX_Control(pDX, IDC_MIRRORS, m_Mirrors);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMirrorSelect, CDialog)
	//{{AFX_MSG_MAP(CMirrorSelect)
	ON_LBN_DBLCLK(IDC_MIRRORS, OnDblclkMirrors)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMirrorSelect message handlers

void CMirrorSelect::OnDblclkMirrors() 
{
	// TODO: Add your control notification handler code here
	m_Selected = m_Mirrors.GetCurSel();

//	this->DestroyWindow();
	CDialog::OnOK();
}

BOOL CMirrorSelect::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	for (int i=0;i < m_ClusterEntries.GetSize();i++) {
		m_Mirrors.AddString(m_ClusterEntryDescs.GetAt(i));
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
