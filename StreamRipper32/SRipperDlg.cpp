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

#include "ResizableDialog.h"
#include "stdafx.h"
#include "SRipper.h"
#include "SRipperDlg.h"
extern "C" {
#include <rip_manager.h>
#include <util.h>
}
#include <afxtempl.h>
#include <afxmt.h>
#include <process.h>
#include "frontend.h"
#include "FolderDialog.h"
#include "MirrorSelect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef struct tag_parms {
	char *ip;
	int	port;
	char *dest;
} parms;


bool	bRipping = false;

int		lastByteUpdate = 0;

bool	gIsRipping = FALSE;
CSRipperDlg *m_CurrentDialog;

extern int GetHTTP(LPCSTR , int , LPCSTR , char *, char *);

CArray<CString, CString>	IPs;
CArray<CString, CString>	Descs;
CArray<CString, CString>	TuneIns;
CArray<bool, bool>			ClusterFlags;

RIP_MANAGER_INFO 	m_curinfo;

CString URLize(CString input)
{
	CString tempString;

	tempString = input;

	tempString.Replace("%", "%25");
	tempString.Replace(";", "%3B");
	tempString.Replace("/", "%2F");
	tempString.Replace("?", "%3F");
	tempString.Replace(":", "%3A");
	tempString.Replace("@", "%40");
	tempString.Replace("&", "%26");
	tempString.Replace("=", "%3D");
	tempString.Replace("+", "%2B");
	tempString.Replace(" ", "%20");
	tempString.Replace("\"", "%5C");
	tempString.Replace("#", "%23");
	
	tempString.Replace("<", "%3C");
	tempString.Replace(">", "%3E");
	tempString.Replace("!", "%21");
	tempString.Replace("*", "%2A");
	tempString.Replace("'", "%27");
	tempString.Replace("(", "%28");
	tempString.Replace(")", "%29");
	tempString.Replace(",", "%2C");

	return tempString;
}

void rip_callback(int message, void *data)
{
	RIP_MANAGER_INFO *info;
	ERROR_INFO *err;
	char	filesize_str[128] = "";

	switch(message)
	{
		case RM_UPDATE:
			info = (RIP_MANAGER_INFO*)data;
			memcpy(&m_curinfo, info, sizeof(RIP_MANAGER_INFO));
			format_byte_size(filesize_str, m_curinfo.filesize);

			m_CurrentDialog->SetDlgItemText(IDC_STREAMNAME, m_curinfo.streamname);
			switch(m_curinfo.status)
			{
				case RM_STATUS_BUFFERING:
					m_CurrentDialog->SetDlgItemText(IDC_RIPPING, "Buffering");
					break;

				case RM_STATUS_RIPPING:
					m_CurrentDialog->m_SongTitle = m_curinfo.filename;
					m_CurrentDialog->SetDlgItemText(IDC_SONGTITLE, m_curinfo.filename);
					lastByteUpdate = 0;
					m_CurrentDialog->SetDlgItemText(IDC_RIPPING, "Ripping");
					break;
				case RM_STATUS_RECONNECTING:
					m_CurrentDialog->SetDlgItemText(IDC_RIPPING, "Reconnecting");
					break;
			}
			m_CurrentDialog->m_Bytes = filesize_str;
			if (m_CurrentDialog->m_MaxBytes > 0) {

				m_CurrentDialog->m_MaxBytes = m_CurrentDialog->m_MaxBytes - (atol(filesize_str) - lastByteUpdate);
				lastByteUpdate = atol(filesize_str);
			}
			if (m_CurrentDialog->m_MaxBytes < 0) {
				m_CurrentDialog->m_MaxBytes = 0;
			}
			m_CurrentDialog->SetDlgItemText(IDC_BYTES, filesize_str);
			m_CurrentDialog->SetDlgItemInt(IDC_MAXBYTES, m_CurrentDialog->m_MaxBytes);
			break;
		case RM_ERROR:
			err = (ERROR_INFO*)data;
			MessageBox(NULL, err->error_str, "StreamRipper Error", MB_OK);
			break;
		case RM_DONE:
			break;
		case RM_NEW_TRACK:
			bool bNewTrack;
			bNewTrack = true;
			break;
		case RM_STARTED:
			m_CurrentDialog->m_ConnectRelay.EnableWindow(TRUE);
			break;
	}
}

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSRipperDlg dialog

CSRipperDlg::CSRipperDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSRipperDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSRipperDlg)
	m_Destination = _T("");

	m_SelectedBroadcast = _T("");

	m_GenreValue = _T("");

	m_SongTitle = _T("");

	m_MaxBytes = 0;

	m_StreamName = _T("");

	m_RelayPort = 0;

	m_Bytes = _T("");

	m_Status = _T("");

	m_URL = _T("");

	m_addID3 = FALSE;

	m_Overwrite = FALSE;

	m_AddSeq = FALSE;

	//}}AFX_DATA_INIT

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSRipperDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSRipperDlg)
	DDX_Control(pDX, IDC_URL, m_URLctrl);

	DDX_Control(pDX, IDC_RELAYPORT, m_RelayPortCtrl);

	DDX_Control(pDX, IDC_CONNECT_RELAY, m_ConnectRelay);

	DDX_Control(pDX, IDC_LIST_CTRL, m_cListCtrl);

	DDX_Control(pDX, IDC_MAXBYTES, m_MaxBytesCtl);

	DDX_Control(pDX, IDC_FOLDER, m_Folder);

	DDX_Control(pDX, IDOK, m_OKctrl);

	DDX_Control(pDX, IDC_DESTINATION, m_Destinationctrl);

	DDX_Control(pDX, IDC_STOPRIPPING, m_StopRipping);

	DDX_Control(pDX, IDC_REFRESH, m_Refresh);

	DDX_Control(pDX, IDC_RIPAWAY, m_Ripaway);

	DDX_Control(pDX, IDC_GENRE, m_Genre);

	DDX_Text(pDX, IDC_DESTINATION, m_Destination);

	DDX_CBString(pDX, IDC_GENRE, m_GenreValue);

	DDX_Text(pDX, IDC_SONGTITLE, m_SongTitle);

	DDX_Text(pDX, IDC_MAXBYTES, m_MaxBytes);

	DDX_Text(pDX, IDC_STREAMNAME, m_StreamName);

	DDX_Text(pDX, IDC_RELAYPORT, m_RelayPort);

	DDX_Text(pDX, IDC_BYTES, m_Bytes);

	DDX_Text(pDX, IDC_RIPPING, m_Status);

	DDX_Text(pDX, IDC_URL, m_URL);

	DDX_Check(pDX, IDC_ID3, m_addID3);

	DDX_Check(pDX, IDC_OVERWRITE, m_Overwrite);

	DDX_Check(pDX, IDC_ADD_SEQ, m_AddSeq);

	//}}AFX_DATA_MAP

}

BEGIN_MESSAGE_MAP(CSRipperDlg, CResizableDialog)
	//{{AFX_MSG_MAP(CSRipperDlg)
	ON_WM_SYSCOMMAND()
	ON_BN_CLICKED(IDC_RIPAWAY, OnRipaway)
	ON_CBN_EDITCHANGE(IDC_GENRE, OnEditchangeGenre)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_STOPRIPPING, OnStopripping)
	ON_EN_UPDATE(IDC_BYTES, OnUpdateBytes)
	ON_EN_UPDATE(IDC_SONGTITLE, OnUpdateSongtitle)
	ON_EN_UPDATE(IDC_DESTINATION, OnUpdateDestination)
	ON_BN_CLICKED(IDC_FOLDER, OnFolder)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CTRL, OnDblclkListCtrl)
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONNECT_RELAY, OnConnectRelay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSRipperDlg message handlers



BOOL CSRipperDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	UpdateData(TRUE);

	m_Genre.AddString("TopTen");
	m_Genre.AddString("Classical");
	m_Genre.AddString("Comedy");
	m_Genre.AddString("Country");
	m_Genre.AddString("Techno");
	m_Genre.AddString("Dance");
	m_Genre.AddString("Funk");
	m_Genre.AddString("Jazz");
	m_Genre.AddString("Metal");
	m_Genre.AddString("Mixed");
	m_Genre.AddString("Pop");
	m_Genre.AddString("R&B");
	m_Genre.AddString("Rap");
	m_Genre.AddString("Rock");
	m_Genre.AddString("Talk");
	m_Genre.AddString("Various");
	m_Genre.AddString("80s");
	m_Genre.AddString("70s");


	UpdateData(TRUE);

	char	dest[1024] = "";
	char	tmpbuf[255] = "";
	char	strAppendStreamName[25] = "";
	char	strWinampBuffer[25] = "";

	GetPrivateProfileString("SRipper","OutputDir","",dest,sizeof(dest), "sripper.ini");
	memset(tmpbuf, '\000', sizeof(tmpbuf));
	GetPrivateProfileString("SRipper","lastURL", "", tmpbuf, sizeof(tmpbuf), "sripper.ini");
	m_URL = tmpbuf;
	memset(tmpbuf, '\000', sizeof(tmpbuf));
	GetPrivateProfileString("SRipper","relayPort", "10069", tmpbuf, sizeof(tmpbuf), "sripper.ini");

	m_RelayPort = atoi(tmpbuf);



	GetPrivateProfileString("SRipper","addID3", "TRUE", tmpbuf, sizeof(tmpbuf), "sripper.ini");

	if (!strcmp(tmpbuf, "TRUE")) {

		m_addID3 = TRUE;

	}

	else {

		m_addID3 = FALSE;

	}

	GetPrivateProfileString("SRipper","addSeqNumber", "FALSE", tmpbuf, sizeof(tmpbuf), "sripper.ini");

	if (!strcmp(tmpbuf, "TRUE")) {

		m_AddSeq = TRUE;

	}

	else {

		m_AddSeq = FALSE;

	}

	GetPrivateProfileString("SRipper","Overwrite", "FALSE", tmpbuf, sizeof(tmpbuf), "sripper.ini");

	if (!strcmp(tmpbuf, "TRUE")) {

		m_Overwrite = TRUE;

	}

	else {

		m_Overwrite = FALSE;

	}


	m_Destination = dest;

	m_StreamName = "StreamRipper 32";
	UpdateData(FALSE);
	m_CurrentDialog = this;

	m_ConnectRelay.EnableWindow(FALSE);

	CRect rect;
	m_cListCtrl.GetClientRect(&rect);

	int nColInterval = rect.Width()/10;
	m_cListCtrl.InsertColumn(0, _T("Description"), LVCFMT_LEFT, nColInterval*7+21);
	m_cListCtrl.InsertColumn(1, _T("Bitrate"), LVCFMT_LEFT, nColInterval+8);
	m_cListCtrl.InsertColumn(2, _T("Track Info"), LVCFMT_LEFT, /*rect.Width()-4* */nColInterval+15);

	AddAnchor(IDC_LIST_CTRL, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STATIC1, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_STREAMNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SONGTITLE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC5, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC3, BOTTOM_LEFT);
	AddAnchor(IDC_RIPPING, BOTTOM_LEFT);

	EnableSaveRestore("StreamRipper32", "winposition");

	return TRUE;  // return TRUE  unless you set the focus to a control
}



void CSRipperDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CResizableDialog::OnSysCommand(nID, lParam);
	}
}


HANDLE	ripThread = 0;
HANDLE	winampThread = 0;


void CSRipperDlg::OnRipaway() 
{
	char	url[1024] = "";

	UpdateData(TRUE);

	if (m_URL.Find("http://") == -1) {
		m_URL = "http://" + m_URL;
	}
	strcpy(url, LPCSTR(m_URL));
	SetDlgItemText(IDC_URL, url);

	m_StopRipping.EnableWindow(TRUE);
	m_Ripaway.EnableWindow(FALSE);
	m_Genre.EnableWindow(FALSE);
	m_Refresh.EnableWindow(FALSE);
	m_Destinationctrl.EnableWindow(FALSE);
	m_URLctrl.EnableWindow(FALSE);
	m_OKctrl.EnableWindow(FALSE);
	m_Folder.EnableWindow(FALSE);
	m_MaxBytesCtl.EnableWindow(FALSE);
	m_RelayPortCtrl.EnableWindow(FALSE);


	RIP_MANAGER_OPTIONS	m_opt;


	memset(&m_opt, '\000', sizeof(m_opt));

	if (m_RelayPort > 0) {
		m_opt.relay_port = m_RelayPort;
		m_opt.flags = OPT_AUTO_RECONNECT | OPT_SEPERATE_DIRS | OPT_SEARCH_PORTS;
	}
	else {
		m_opt.flags =  OPT_AUTO_RECONNECT | OPT_SEPERATE_DIRS | OPT_SEARCH_PORTS;
	}
	m_opt.max_port = 18000;
//	m_opt.search_ports = TRUE;
//	m_opt.auto_reconnect = TRUE;		// tested ok
	strcpy(m_opt.output_directory, LPCSTR(m_Destination));	// tested ok
	m_opt.proxyurl[0] = (char)NULL;		// not tested
//	m_opt.seperate_dirs = TRUE;		// tested
	m_opt.over_write_existing_tracks = m_Overwrite; // not tested

	m_opt.add_id3tag = m_addID3;

	m_opt.add_seq_number = m_AddSeq;


	strncpy(m_opt.url, url, MAX_URL_LEN);

	int	ret = 0;
	SetDlgItemText(IDC_STREAMNAME, "");
	SetDlgItemText(IDC_SONGTITLE, "");
	SetDlgItemText(IDC_RIPPING, "Connecting");
	UpdateWindow();

	if ((ret = rip_manager_start(rip_callback, &m_opt)) != SR_SUCCESS)
	{
		char	msg[1024] = "";

		if (ret == SR_ERROR_LIVE365) {

			sprintf(msg, "Sorry, but StreamRipper32 does not support ripping Live365 streams");

		}

		else {
			sprintf(msg, "Couldn't connect to %s\n", m_opt.url);

		}
		MessageBox(msg, "Error", MB_OK);
		bRipping = false;
		OnStopripping();
		return;
	}
	bRipping = true;
}

void CSRipperDlg::OnEditchangeGenre() 
{
	// TODO: Add your control notification handler code here
	;
}

void CSRipperDlg::OnDblclkBroadcasts() 
{
	// TODO: Add your control notification handler code here

	CString	ip;
	CString	tunein;
	CArray<CString, CString>	ClusterEntries;
	CArray<CString, CString>	ClusterEntryDescs;
	char	result[100000] = "";
	char	request[100000] = "";
	UpdateData(TRUE);
	ip = IPs.GetAt(m_Broadcasts.GetCurSel());
	tunein = TuneIns.GetAt(m_Broadcasts.GetCurSel());

	ClusterEntries.RemoveAll();
	ClusterEntryDescs.RemoveAll();
	if (ClusterFlags.GetAt(m_Broadcasts.GetCurSel())) {
		memset(result, '\000', sizeof(result));
		strcpy(request, "/sbin/shoutcast-playlist.pls");
		strcat(request, tunein);
		GetHTTP("yp.shoutcast.com", 80, request, "Host: yp.shoutcast.com\r\nAccept: */*", result);

		char *p1 = 0;
		p1 = strstr(result, "[playlist]");
		if (p1 == NULL) {
			MessageBox("Could not retrieve cluster information", "Error", MB_OK);
		}
		else {
			char	*p2;
			char	strnumentries[25] = "";
			int		numentries = 0;
			p2 = strstr(p1, "numberofentries=");
			if (p2 != NULL) {
				p2 = p2 + strlen("numberofentries=");
				char	*p3;
				p3 = strstr(p2, "\n");
				if (p3 != NULL) {
					memset(strnumentries, '\000', sizeof(strnumentries));
					strncpy(strnumentries, p2, p3-p2);
					numentries = atoi(strnumentries);
				}
				CMirrorSelect	mirrorDialog;

				for (int i=1;i<=numentries;i++) {
					char	*p4;
					char	*p5;
					char	Tag[25] = "";
					char	IP[35] = "";
					char	Desc[255] = "";
					memset(IP, '\000', sizeof(IP));
					memset(Desc, '\000', sizeof(Desc));
					sprintf(Tag, "File%d", i);
					p4 = strstr(p3, Tag);
					if (p4 != NULL) {
						p4 = p4 + strlen(Tag) + 1 + strlen("http://");
						p5 = strstr(p4, "\n");
						if (p5 != NULL) {
							strncpy(IP, p4, p5-p4);
						}
					}
					sprintf(Tag, "Title%d", i);
					p4 = strstr(p3, Tag);
					if (p4 != NULL) {
						p4 = p4 + strlen(Tag) + 1;
						p5 = strstr(p4, "\n");
						if (p5 != NULL) {
							strncpy(Desc, p4, p5-p4);
						}
					}
					mirrorDialog.m_ClusterEntries.Add(IP);
					mirrorDialog.m_ClusterEntryDescs.Add(Desc);
				}


				mirrorDialog.m_Selected = -1;
				mirrorDialog.DoModal();
			
				if (mirrorDialog.m_Selected != -1) {
					ip = mirrorDialog.m_ClusterEntries.GetAt(mirrorDialog.m_Selected);
				}
	
			}
		}


	}

	char	*p1;
	char	theip[255] = "";
	char	theport[25] = "";
	p1 = strchr(ip, ':');
	memset(theip, '\000', sizeof(theip));
	memset(theport, '\000', sizeof(theport));

	if (p1 != NULL) {
		strncpy(theip, ip, p1 - ip);
		strcpy(theport, p1 + 1);
		CString m_IP = theip;
		int m_Port = atoi(theport);
		char	bleh[255] = "";
		sprintf(bleh, "http://%s:%d", m_IP, m_Port);
		m_URL = bleh;
		UpdateData(FALSE);
	}

}

void CSRipperDlg::OnRefresh() 
{
	char	request[255] = "";
	char	result[1000000] = "";
	LVITEM lvi;
	CString strItem;

	BeginWaitCursor();

	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_GenreValue.GetLength() > 0) {
		CString output;
		output = URLize(m_GenreValue);
		sprintf(request, "?genre=%s", output);
		memset(result, '\000', sizeof(result));
		GetHTTP("www.shoutcast.com", 80, request, "Host: www.shoutcast.com\r\nAccept: */*", result);
		IPs.RemoveAll();
		Descs.RemoveAll();
		TuneIns.RemoveAll();
		ClusterFlags.RemoveAll();
		char	*p1;
		char	*p2;
		int		loop = 1;
		char	IP[255] = "";
		char	Desc[255] = "";
		char	BitRate[25] = "";
		char	*p8;
		char	*p9;
		int		metaflag;
		bool	clusterFlag = FALSE;
		char	tuneinURL[1024] = "";

		p1 = result;
		int count = 0;
		while (loop) {
			metaflag = 0;
			memset(IP, '\000', sizeof(IP));
			memset(Desc, '\000', sizeof(Desc));

			p2 = strstr(p1, "shoutcast-playlist.pls");
			if (p2 == NULL) {
				loop = 0;
			}
			else {
				p2 = p2 + strlen("shoutcast-playlist.pls");
				char	*pTune;
				pTune = strchr(p2, '"');
				memset(tuneinURL, '\000', sizeof(tuneinURL));
				strncpy(tuneinURL, p2, pTune - p2);
				char	*p3;
				p3 = strstr(p2, "addr=");
				p8 = strstr(p2, "Now Playing:");
				p9 = strstr(p2, "</tr>");
				if ((p8 != 0) && (p8 < p9)) {
					metaflag = 1;
				}
				if (p3 == NULL) {
					loop = 0;
				}
				else {
					p3 = p3 + strlen("addr=");
					char	*p4;
					p4 = strstr(p3, "&");
					if (p4 == NULL) {
						loop = 0;
					}
					else {
						strncpy(IP, p3, p4-p3);
						char	*p5;
						p5 = strstr(p4, "scurl\"");
						if (p5 == NULL) {
							loop = 0;
						}
						else {
							/* Check Cluster Flag */
							char	substring[1024] = "";
							memset(substring, '\000', sizeof(substring));
							strncpy(substring, p4, p5-p4);
							char	*pCluster;
							clusterFlag = FALSE;
							pCluster = strstr(substring, "<font color=#ff0000 size=-2>CLUSTER");
							if (pCluster == NULL) {
								clusterFlag = FALSE;
							}
							else {
								clusterFlag = TRUE;
							}
							char	*p6;
							p6 = strstr(p5, ">");
							if (p6 == NULL) {
								loop = 0;
							}
							else {
								p6 = p6 + strlen(">");
								char	*p7;
								p7 = strstr(p6, "</a>");
								if (p7 == NULL) {
									loop = 0;
								}
								else {
									strncpy(Desc, p6, p7 - p6);			
									char	*p8;
									p8 = strstr(p7, "</font>");
									if (p8 == NULL) {
										loop = 0;
									}
									else {
										p8++;
										char	*p9;
										p9 = strstr(p8, "</font>");
										if (p9 == NULL) {
											loop = 0;
										}
										else {
											p9++;
											char	*p10;
											p10 = strstr(p9, "</font>");
											if (p10 == NULL) {
												loop = 0;
											}
											else {
												if (metaflag) {
													p10++;
													char	*p11;
													p11 = strstr(p10, "</font>");
													if (p11 == NULL) {
														loop = 0;
													}
													else {
														p11++;
														char	*p12;
														p12 = strstr(p11, "</font>");
														if (p12 == NULL) {
															loop = 0;
														}
														else {
															char	*p13;
															p13 = p12;
															while (*p13 != '>') {
																p13--;
															}
															if (p13 == NULL) {
																loop = 0;
															}
															else {
																p13++;
																memset(BitRate, '\000', sizeof(BitRate));
																strncpy(BitRate, p13, p12-p13);
																p1 = p13;
															}
														}
													}
												}
												else {
													char	*p13;
													p13 = p10;
													while (*p13 != '>') {
														p13--;
													}
													if (p13 == NULL) {
														loop = 0;
													}
													else {
														p13++;
														memset(BitRate, '\000', sizeof(BitRate));
														strncpy(BitRate, p13, p10-p13);
														p1 = p13;
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			CString	ip;
			CString desc;
			CString tunein;
			tunein = tuneinURL;
			ip = IP;

			if (strlen(IP) > 0) {
				// Jay's Way..
				CString cluster = "No";
				CString bitsamp = BitRate;
				CString tt = "No";
				
				bitsamp += " kbps";
				if (metaflag) {
					tt = "Yes";
				}
				if (clusterFlag) {
					cluster = "Yes";
				}
				desc = Desc;
				//desc += "                                                                                                            ";
				// Insert the first item
				lvi.mask =  LVIF_IMAGE | LVIF_TEXT;
				// strItem.Format(_T("Item %i"), i);
				lvi.iItem = count;
				lvi.iSubItem = 0;
				lvi.pszText = (LPTSTR)(LPCTSTR)(desc);
				m_cListCtrl.InsertItem(&lvi);

				lvi.iSubItem = 1;
				lvi.pszText = (LPTSTR)(LPCTSTR)(bitsamp);
				m_cListCtrl.SetItem(&lvi);

				lvi.iSubItem = 2;
				lvi.pszText = (LPTSTR)(LPCTSTR)(tt);
				m_cListCtrl.SetItem(&lvi);

				count++;

				if (ip.GetLength() > 0) {
					IPs.Add(ip);
					Descs.Add(Desc);
					TuneIns.Add(tunein);
					ClusterFlags.Add(clusterFlag);
				}
			}

		}
	}
	else {
		MessageBox("You must supply a Genre", "Error", MB_OK);
	}

	UpdateData(FALSE);
	EndWaitCursor();
}

void CSRipperDlg::OnStopripping() 
{
	// TODO: Add your control notification handler code here
	if (bRipping) {
		rip_manager_stop();
	}
	m_CurrentDialog->SetDlgItemText(IDC_RIPPING, "");
	m_CurrentDialog->SetDlgItemText(IDC_SONGTITLE, "");
	m_CurrentDialog->SetDlgItemText(IDC_BYTES, "");

	m_CurrentDialog->m_StopRipping.EnableWindow(FALSE);
	m_CurrentDialog->m_Ripaway.EnableWindow(TRUE);
	m_CurrentDialog->m_Genre.EnableWindow(TRUE);
	m_CurrentDialog->m_Refresh.EnableWindow(TRUE);
	m_CurrentDialog->m_Destinationctrl.EnableWindow(TRUE);
	m_CurrentDialog->m_URLctrl.EnableWindow(TRUE);
	m_CurrentDialog->m_OKctrl.EnableWindow(TRUE);
	m_CurrentDialog->m_Folder.EnableWindow(TRUE);
	m_CurrentDialog->m_MaxBytesCtl.EnableWindow(TRUE);
	m_CurrentDialog->m_ConnectRelay.EnableWindow(FALSE);
	m_CurrentDialog->m_RelayPortCtrl.EnableWindow(TRUE);
}

void CSRipperDlg::OnOK() 
{
	// TODO: Add extra validation here

	UpdateData(TRUE);
	WritePrivateProfileString("SRipper","OutputDir",m_Destination, "sripper.ini");
	WritePrivateProfileString("SRipper","lastURL", m_URL, "sripper.ini");
	char relayPort[255] = "";
	sprintf(relayPort, "%d", m_RelayPort);
	WritePrivateProfileString("SRipper","relayPort", relayPort, "sripper.ini");


	if (m_addID3) {

		WritePrivateProfileString("SRipper","addID3", "TRUE", "sripper.ini");

	}

	else {

		WritePrivateProfileString("SRipper","addID3", "FALSE", "sripper.ini");

	}



	if (m_AddSeq) {

		WritePrivateProfileString("SRipper","addSeqNumber", "TRUE","sripper.ini");

	}

	else {

		WritePrivateProfileString("SRipper","addSeqNumber", "FALSE","sripper.ini");

	}

	if (m_Overwrite) {

		WritePrivateProfileString("SRipper","Overwrite", "TRUE", "sripper.ini");

	}

	else {

		WritePrivateProfileString("SRipper","Overwrite", "FALSE", "sripper.ini");

	}


	CResizableDialog::OnOK();
}


void CSRipperDlg::OnUpdateBytes() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here

//	UpdateData(FALSE);
}

void CSRipperDlg::OnUpdateSongtitle() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	
}


void CSRipperDlg::OnUpdateDestination() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_UPDATE flag ORed into the lParam mask.
	
	// TODO: Add your control notification handler code here
	

	UpdateData(TRUE);
}



void CSRipperDlg::OnFolder() 
{
	// TODO: Add your control notification handler code here
	CFolderDialog	folderdialog;

	if (folderdialog.DoModal() == IDOK) {
		m_Destination = folderdialog.GetPathName() + "\\";
		UpdateData(FALSE);
	}
}


void CSRipperDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
		// TODO: Add your message handler code here
	CMenu menu;

	menu.LoadMenu(IDR_MENU1);
	CMenu *pContextMenu = menu.GetSubMenu(0);

	pContextMenu->TrackPopupMenu(TPM_LEFTALIGN |
		TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
		point.x, point.y, AfxGetMainWnd());
}

void CSRipperDlg::OnDblclkListCtrl(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	

	if (gIsRipping) {
		MessageBox("Cannot select while ripping..", "Error", MB_OK);
	}
	else {
		int selected = m_cListCtrl.GetSelectionMark();

		CString	ip;
		CString	tunein;
		CString desc;

		CArray<CString, CString>	ClusterEntries;
		CArray<CString, CString>	ClusterEntryDescs;
		char	result[100000] = "";
		char	request[100000] = "";
		UpdateData(TRUE);
		ip = IPs.GetAt(selected);
		tunein = TuneIns.GetAt(selected);
		desc = Descs.GetAt(selected);

		m_StreamName = desc;
		UpdateData(FALSE);

		ClusterEntries.RemoveAll();
		ClusterEntryDescs.RemoveAll();
		if (ClusterFlags.GetAt(selected)) {
			memset(result, '\000', sizeof(result));
			strcpy(request, "/sbin/shoutcast-playlist.pls");
			strcat(request, tunein);
			GetHTTP("yp.shoutcast.com", 80, request, "Host: yp.shoutcast.com\r\nAccept: */*", result);

			char *p1 = 0;
			p1 = strstr(result, "[playlist]");
			if (p1 == NULL) {
				MessageBox("Could not retrieve cluster information", "Error", MB_OK);
			}
			else {
				char	*p2;
				char	strnumentries[25] = "";
				int		numentries = 0;
				p2 = strstr(p1, "numberofentries=");
				if (p2 != NULL) {
					p2 = p2 + strlen("numberofentries=");
					char	*p3;
					p3 = strstr(p2, "\n");
					if (p3 != NULL) {
						memset(strnumentries, '\000', sizeof(strnumentries));
						strncpy(strnumentries, p2, p3-p2);
						numentries = atoi(strnumentries);
					}
					CMirrorSelect	mirrorDialog;

					for (int i=1;i<=numentries;i++) {
						char	*p4;
						char	*p5;
						char	Tag[25] = "";
						char	IP[35] = "";
						char	Desc[255] = "";
						memset(IP, '\000', sizeof(IP));
						memset(Desc, '\000', sizeof(Desc));
						sprintf(Tag, "File%d", i);
						p4 = strstr(p3, Tag);
						if (p4 != NULL) {
							p4 = p4 + strlen(Tag) + 1 + strlen("http://");
							p5 = strstr(p4, "\n");
							if (p5 != NULL) {
								strncpy(IP, p4, p5-p4);
							}
						}
						sprintf(Tag, "Title%d", i);
						p4 = strstr(p3, Tag);
						if (p4 != NULL) {
							p4 = p4 + strlen(Tag) + 1;
							p5 = strstr(p4, "\n");
							if (p5 != NULL) {
								strncpy(Desc, p4, p5-p4);
							}
						}
						mirrorDialog.m_ClusterEntries.Add(IP);
						mirrorDialog.m_ClusterEntryDescs.Add(Desc);
					}


					mirrorDialog.m_Selected = -1;
					mirrorDialog.DoModal();
				
					if (mirrorDialog.m_Selected != -1) {
						ip = mirrorDialog.m_ClusterEntries.GetAt(mirrorDialog.m_Selected);
					}
				}
			}
		}

		char	*p1;
		char	theip[255] = "";
		char	theport[25] = "";
		p1 = strchr(ip, ':');
		memset(theip, '\000', sizeof(theip));
		memset(theport, '\000', sizeof(theport));

		if (p1 != NULL) {
			strncpy(theip, ip, p1 - ip);
			strcpy(theport, p1 + 1);
			CString m_IP = theip;
			int m_Port = atoi(theport);
			char	bleh[255] = "";
			sprintf(bleh, "http://%s:%d", m_IP, m_Port);
			m_URL = bleh;
			UpdateData(FALSE);
		}
	}
	*pResult = 0;
}

void CSRipperDlg::OnConnectRelay() 
{
	if (m_RelayPort == 0) {
		MessageBox("Relay port cannot be zero if you want to use this feature", "Error", MB_OK);
		return;
	}
	// TODO: Add your control notification handler code here
	HWND hwndWinamp = ::FindWindow("Winamp v1.x",NULL); 

	if (hwndWinamp != NULL) {
		::SendMessage(hwndWinamp,WM_WA_IPC,0,IPC_DELETE);
		COPYDATASTRUCT cds;
		cds.dwData = IPC_PLAYFILE;
		char	url[255] = "";
		sprintf(url, "http://127.0.0.1:%d", m_RelayPort);
		cds.lpData = (void *) url;
		cds.cbData = strlen((char *) cds.lpData)+1; // include space for null char
		::SendMessage(hwndWinamp, WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
		::SendMessage(hwndWinamp, WM_WA_IPC,0,IPC_STARTPLAY);
	}
	else {
		MessageBox("Winamp must be running if you want to use this feature", "Error", MB_OK);
	}

}


