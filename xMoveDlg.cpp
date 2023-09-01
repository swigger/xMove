// xMoveDlg.cpp : implementation file
//

#include "stdafx.h"
#include "xMove.h"
#include "xMoveDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMERID_ENABLE_BUTTON 1
#define TIMERID_GETNOTIFY 2

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CxMoveDlg dialog




CxMoveDlg::CxMoveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CxMoveDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CxMoveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CxMoveDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CxMoveDlg::OnBnClickedOk)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_ENDMOVE, &CxMoveDlg::OnBnClickedEndmove)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BROWSE_SRC, &CxMoveDlg::OnBnClickedBrowseSrc)
	ON_BN_CLICKED(IDC_BROWSE_DST, &CxMoveDlg::OnBnClickedBrowseDst)
END_MESSAGE_MAP()


// CxMoveDlg message handlers

BOOL CxMoveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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

	SetDlgItemText(IDC_SLEEPTIME, L"0");
	SetTimer(TIMERID_ENABLE_BUTTON, 1000, NULL);
	SendDlgItemMessage(IDC_FSZ, CB_SETCURSEL, 4, 0);
	OnTimer(TIMERID_GETNOTIFY);
	OnSize(0,0,0);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CxMoveDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CxMoveDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CxMoveDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CxMoveDlg::OnBnClickedOk()
{
	CString src,dst,slTime;
	DWORD dwST = 0;
	BOOL bMove = SendDlgItemMessage(IDC_CHKMOVE, BM_GETCHECK, 0, 0) == BST_CHECKED;

	INT_PTR nsel = SendDlgItemMessage(IDC_FSZ, CB_GETCURSEL, 0, 0);
	DWORD dwSize = 0;
	if (nsel == 0) dwSize = 100 * 1024;
	else if (nsel == 1) dwSize = 1 * 1024*1024;
	else if (nsel == 2) dwSize = 10 * 1024*1024;
	else if (nsel == 3) dwSize = 100 * 1024*1024;
	else dwSize = 0;

	GetDlgItemText(IDC_ED_SRC, src);
	GetDlgItemText(IDC_ED_DST, dst);
	GetDlgItemText(IDC_SLEEPTIME, slTime);
	src.Trim();
	dst.Trim();
	slTime.Trim();
	dwST = _ttoi(slTime);

	if (!src.IsEmpty() && !dst.IsEmpty() && src!=dst)
	{
		CreateDirectory(dst, NULL);
		DWORD attr1 = GetFileAttributes(dst);
		DWORD attr2 = GetFileAttributes(src);
#define ISDIR(x) ((x&FILE_ATTRIBUTE_DIRECTORY) && x!=INVALID_FILE_ATTRIBUTES)

		if (ISDIR(attr1) && ISDIR(attr2))
		{
			m_mgr.Init(src,dst,dwSize, bMove, dwST);
			HWND hW = ::GetDlgItem(m_hWnd, IDC_NOTIFY);
			m_mgr.StartMove(hW);
			if (m_mgr.Busy())
			{
				HWND btn = ::GetDlgItem(m_hWnd, IDOK);
				::EnableWindow(btn, FALSE);
				SetTimer(TIMERID_GETNOTIFY, 200, 0);
			}
		}
	}
}


void CxMoveDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMERID_ENABLE_BUTTON)
	{
		if (!m_mgr.Busy())
		{
			HWND btn = ::GetDlgItem(m_hWnd, IDOK);
			::EnableWindow(btn, TRUE);
			KillTimer(TIMERID_GETNOTIFY);
			nIDEvent = TIMERID_GETNOTIFY; //check one time more.
		}
	}
	if (nIDEvent == TIMERID_GETNOTIFY)
	{
		CString snotify;
		if (m_mgr.GetNotifyStr(snotify))
		{
			SetDlgItemText(IDC_NOTIFY, snotify);
		}
	}
}

void CxMoveDlg::OnCancel()
{
	if (m_mgr.Busy() &&
		MessageBox(L"moving, exit?", L"QUERY", MB_YESNO|MB_ICONQUESTION) == IDNO)
	{
		return ;
	}

	CDialog::OnCancel();
}

void CxMoveDlg::OnBnClickedEndmove()
{
	m_mgr.SetStop();
}

#include <vector>
using std::vector;
#include "sizes2.h"
static const SIZES2::ItemRect s_szs[] =
{
	//rela, out, w, h, {margin:l t r b}
	//src row.
	{-1, 1, 21, 13, {0,4}},
	{-1, 1, -1, 21, {2,0,25,0}},
	{-1, 1, 21, 21, {2,0}},
	SIZES2_USELINE,
	SIZES2_HLINE(5),
	//dst row.
	{-1, 1, 21, 13, {0,4}},
	{-1, 1, -1, 21, {2,0,25,0}},
	{-1, 1, 21, 21, {2,0}},
	SIZES2_USELINE,
	SIZES2_HLINE(5),
	//info row.
	{-1, 1, 21, 13, {0,4}},
	{-1, 1, 60, 21, {2}},
	{-1, 1, 80, 21, {2,0}},
	{-1, 1, 51, 13, {2,4}},
	{-1, 1, 60, 21, {2,0}},
	SIZES2_USELINE,
	SIZES2_HLINE(5),
	//btns.
	{-1, 1, 70, 21},
	{-1, 1, 70, 21,{20}},
	//notifiers
	SIZES2_USELINE,
	{-1, 1, -1, 13, {0,5}},
};

void CxMoveDlg::OnSize(UINT nType, int cx, int cy)
{
	CRect rc;
	GetClientRect(&rc);
	rc.DeflateRect(2,2,2,1);
	RECT out[_countof(s_szs)];
	SIZES2::GetSizes(s_szs, _countof(s_szs), rc, out, 0);

	std::vector<HWND> vh = SIZES2::getChildren(m_hWnd);
	for (size_t i=0,mi=vh.size(); i<mi; ++i)
	{
		::MoveWindow(vh[i], out[i].left, out[i].top, out[i].right-out[i].left, out[i].bottom-out[i].top, false);
	}
	InvalidateRect(NULL, true);
}

static void browse_folder(HWND hWnd, int edid)
{
	TCHAR buf[300] = {0};
	BROWSEINFO bi = {0};
	bi.hwndOwner = hWnd;
	bi.lpszTitle = _T("Please Select a folder for linking to...");
	bi.pszDisplayName = buf;
	bi.ulFlags = BIF_NEWDIALOGSTYLE|BIF_RETURNONLYFSDIRS;
	LPITEMIDLIST  pidl = SHBrowseForFolder(&bi);
	if(pidl && SUCCEEDED(SHGetPathFromIDList(pidl, buf)))
	{
		SetDlgItemText(hWnd, edid, buf);
	}
	if(pidl) ILFree(pidl);
}

void CxMoveDlg::OnBnClickedBrowseSrc()
{
	browse_folder(m_hWnd, IDC_ED_SRC);
}

void CxMoveDlg::OnBnClickedBrowseDst()
{
	browse_folder(m_hWnd, IDC_ED_DST);
}
