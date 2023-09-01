#pragma once

#include "../brcopy/cputil.h"

class CxMoveMgr
{
private:
	enum {BUFSZ = 12*1024*1024};
	BYTE * m_bufMem;
	HANDLE m_hThread;
	HWND m_hwNotify;
	CMTBuf m_notify;
	DWORD m_lastver = ~0u;

private:
	TCHAR m_src[MAX_PATH],m_dst[MAX_PATH];
	DWORD m_fsz, m_dwSleepTime;
	bool m_bMove;
	bool m_bStop;

public:
	CxMoveMgr(void);
	~CxMoveMgr(void);

	void Init(LPCTSTR src,LPCTSTR dst,DWORD fsz, BOOL bMove, DWORD dwSleepTime)
	{
		wcsncpy(m_src , src, _countof(m_src));
		wcsncpy(m_dst , dst, _countof(m_dst));
		m_fsz = fsz;
		m_bMove = (bMove!=0);
		m_dwSleepTime = dwSleepTime;
	}
	bool Busy(){return m_hThread != NULL;}

	void StartMove(HWND hWnd2);
	void SetStop()
	{
		m_bStop = true;
	}
	void SetNotifyStr(LPCWSTR src)
	{
		m_notify.write(src);
	}
	bool GetNotifyStr(CString & snotify)
	{
		uint32_t ver1 = 0;
		m_notify.Lock();
		snotify = m_notify.get_info(&ver1);
		m_notify.Unlock();
		bool br = ver1 != m_lastver;
		m_lastver = ver1;
		return br;
	}

private:
	static void ath(void * pt);
	void StartMove_();
	void Move(const CString & src, const CString & dst);
	__int64 myMoveFile(LPCTSTR srcf, LPCTSTR dstf, const WIN32_FIND_DATA & srcwfd);
	BOOL myCreateDirectory(LPCTSTR dir, const WIN32_FIND_DATA * dirwfd);
	BOOL DoCopy(HANDLE hi, HANDLE ho, __int64 dstsz, __int64 srcsz, __int64 & copsz, LPCWSTR dstfile);
};
