// xMoveDlg.h : header file
//

#pragma once

#include "CxMoveMgr.h"

// CxMoveDlg dialog
class CxMoveDlg : public CDialog
{
// Construction
public:
	CxMoveDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_XMOVE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	CxMoveMgr m_mgr;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
protected:
	virtual void OnCancel();
public:
	afx_msg void OnBnClickedEndmove();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedBrowseSrc();
	afx_msg void OnBnClickedBrowseDst();
};
