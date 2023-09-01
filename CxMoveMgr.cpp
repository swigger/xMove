#include "StdAfx.h"
#include "CxMoveMgr.h"
#include <process.h>

CxMoveMgr::CxMoveMgr(void)
{
	m_src[0] = m_dst[0] =0;
	m_hThread = NULL;
	m_bStop = false;
	m_bMove = false;
	m_bufMem = (BYTE*)VirtualAlloc(NULL, BUFSZ, MEM_COMMIT, PAGE_READWRITE);
	m_dwSleepTime = 0;
}

CxMoveMgr::~CxMoveMgr(void)
{
}

void CxMoveMgr::StartMove(HWND hWnd2)
{
	if (m_hThread == NULL)
	{
		SetNotifyStr(L"");
		m_hwNotify = hWnd2;
		m_hThread = (HANDLE)_beginthread(ath, 0, this);
	}
}

void CxMoveMgr::ath(void * pt)
{
	CxMoveMgr* p = (CxMoveMgr*)pt;
	p->StartMove_();
	p->m_hThread = NULL;
}

void CxMoveMgr::StartMove_()
{
	CString src = m_src;
	CString dst = m_dst;

	if (src.Right(1) != '\\') src += _T('\\');
	if (dst.Right(1) != '\\') dst += _T('\\');

	WIN32_FIND_DATA wfd = {0}, * pwfd = NULL;
	HANDLE hf = CreateFile(src, FILE_READ_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hf != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fio={0};
		GetFileInformationByHandle(hf, &fio);
		wfd.ftCreationTime = fio.ftCreationTime;
		wfd.ftLastAccessTime = fio.ftLastAccessTime;
		wfd.ftLastWriteTime = fio.ftLastWriteTime;
		wfd.dwFileAttributes = fio.dwFileAttributes;
		wfd.nFileSizeHigh = fio.nFileSizeHigh;
		wfd.nFileSizeLow = fio.nFileSizeLow;
		CloseHandle(hf);
		pwfd = &wfd;
	}

	m_bStop = false;
	myCreateDirectory(dst, NULL);
	Move(src, dst);
	myCreateDirectory(dst, pwfd);
	if (m_bMove)
	{
		SetFileAttributes(src, FILE_ATTRIBUTE_NORMAL);
		RemoveDirectory(src);
	}
	LPCWSTR lp = m_bStop ? L"stop!": L"done!";
	SetNotifyStr(lp);
}

void CxMoveMgr::Move(const CString & src, const CString & dst)
{
	TCHAR pathbuf[MAX_PATH] = {0};
	WIN32_FIND_DATA wfd;
	CString find_wc = src + L"*";

	HANDLE hd = FindFirstFile(find_wc, &wfd);
	if (hd != INVALID_HANDLE_VALUE)
	{
		do 
		{
			if (wfd.cFileName[0] == '.' && (wfd.cFileName[1] == 0 || (wfd.cFileName[1]=='.'&&wfd.cFileName[2]==0)))
			{
				continue;
			}

			CString srcf = src+wfd.cFileName;
			CString dstf = dst+wfd.cFileName;
			__int64 copsz = 0;
			if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				myCreateDirectory(dstf, NULL);
				Move(srcf+L"\\", dstf+L"\\");
				myCreateDirectory(dstf, &wfd);
				if (m_bMove)
				{
					SetFileAttributes(srcf, FILE_ATTRIBUTE_NORMAL);
					RemoveDirectory(srcf);
				}
			}
			else
			{
				if (m_fsz != 0)
				{
					if (wfd.nFileSizeHigh == 0  && wfd.nFileSizeLow <= m_fsz)
					{
						copsz = myMoveFile(srcf, dstf, wfd);
					}
				}
				else
				{
					copsz = myMoveFile(srcf, dstf, wfd);
				}
			}
			if (m_bStop) break;
			if (m_dwSleepTime && copsz > 0)
			{
				Sleep(m_dwSleepTime);
			}
			if (m_bStop) break;
		} while(FindNextFile(hd, &wfd));
		FindClose(hd);
	}
}

#define BTRY(x) ( b = (b && ((x)!=0) ) )

bool NeedCopy(LPCTSTR dstf, const WIN32_FIND_DATA & wfd)
{
	HANDLE ho = CreateFile(dstf, FILE_READ_ATTRIBUTES|FILE_READ_EA, 
		FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,	NULL, OPEN_EXISTING, 0, NULL);
	if (ho != INVALID_HANDLE_VALUE)
	{
		BY_HANDLE_FILE_INFORMATION fio={0};
		GetFileInformationByHandle(ho, &fio);
		CloseHandle(ho);

		if (fio.nFileSizeHigh == wfd.nFileSizeHigh && fio.nFileSizeLow == wfd.nFileSizeLow)
		{
			return false;
		}
	}
	return true;
}

static DWORD WINAPI CopyProgCallback(
	_In_     LARGE_INTEGER TotalFileSize,
	_In_     LARGE_INTEGER TotalBytesTransferred,
	_In_     LARGE_INTEGER StreamSize,
	_In_     LARGE_INTEGER StreamBytesTransferred,
	_In_     DWORD dwStreamNumber,
	_In_     DWORD dwCallbackReason,
	_In_     HANDLE hSourceFile,
	_In_     HANDLE hDestinationFile,
	_In_opt_ LPVOID lpData
)
{
	CxMoveMgr* me = (CxMoveMgr*)0[(void**)lpData];
	LPCWSTR srcfile = (LPCWSTR)1[(void**)lpData];

	CString progresst;
	double pct = (double)TotalBytesTransferred.QuadPart / (double)TotalFileSize.QuadPart;
	progresst.Format(L"%I64dM(%.2lf%%) %s", TotalBytesTransferred.QuadPart / (1024 * 1024), pct * 100.0, srcfile);
	me->SetNotifyStr(progresst);

	return PROGRESS_CONTINUE;
}


__int64 CxMoveMgr::myMoveFile(LPCTSTR srcf, LPCTSTR dstf, const WIN32_FIND_DATA & wfd)
{
	//send a message to UI here cause performance problems.
	//SendMessage(m_hwNotify, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)srcf);
	SetNotifyStr(srcf);

	BOOL b = false;
	__int64 copsz = 0;
	if (NeedCopy(dstf, wfd))
	{
		HANDLE hf = CreateFile(srcf, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (hf == INVALID_HANDLE_VALUE) return 0;

		SetFileAttributes(dstf, FILE_ATTRIBUTE_NORMAL);
		HANDLE ho = CreateFile(dstf, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
		if (ho != INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER dstfsz = {0}, srcfsz = {0};
			LARGE_INTEGER zero = {0}, newfp;
			b = true;
			BTRY(GetFileSizeEx(ho, &dstfsz));
			BTRY(GetFileSizeEx(hf, &srcfsz));
			if (b && dstfsz.QuadPart > srcfsz.QuadPart)
			{
				SetFilePointerEx(ho, zero, &newfp, FILE_BEGIN);
				SetEndOfFile(ho);
				dstfsz.QuadPart = 0;
			}

			if (b && dstfsz.QuadPart < srcfsz.QuadPart)
			{
				if (dstfsz.QuadPart == 0 && srcfsz.QuadPart > BUFSZ)
				{
					// file too large. try syscopy.
					const void* par[] = { this, srcf };
					CloseHandle(ho); ho = INVALID_HANDLE_VALUE;
					b = !!CopyFileExW(srcf, dstf, CopyProgCallback, &par[0], 0, COPY_FILE_ALLOW_DECRYPTED_DESTINATION);
				}
				else
				{
					SetFilePointerEx(ho, zero, &newfp, FILE_END);
					BTRY(SetFilePointerEx(hf, dstfsz, &newfp, FILE_BEGIN));
					BTRY(DoCopy(hf, ho, dstfsz.QuadPart, srcfsz.QuadPart, copsz, srcf));
				}
			}
			if (b)
			{
				SetFileTime(ho, &wfd.ftCreationTime, &wfd.ftLastAccessTime, &wfd.ftLastWriteTime);
			}
		}
		CloseHandle(hf);
		CloseHandle(ho);
		if (b)
		{
			SetFileAttributes(dstf, wfd.dwFileAttributes);
		}
	}
	else
	{
		b = true;
	}

	if (b && m_bMove)
	{
		SetFileAttributes(srcf, FILE_ATTRIBUTE_NORMAL);
		BOOL b = DeleteFile(srcf);
		if (!b && 0)
		{
			DWORD dwErr = GetLastError();
			CString s;
			s.Format(L"delete <%s>\r\nerror: %d", srcf, dwErr);
			MessageBox(NULL, s, NULL, MB_ICONWARNING);
		}
	}
	return copsz;
}

BOOL CxMoveMgr::DoCopy(HANDLE hi, HANDLE ho, __int64 dstsz, __int64 srcsz, __int64 & copsz, LPCWSTR srcfile)
{
	copsz = 0;
	for (;;)
	{
		DWORD dwRead = 0;
		const BOOL b = ReadFile(hi, m_bufMem, BUFSZ, &dwRead, NULL);
		if (b && dwRead == 0)
		{
			return true;
		}
		if (!b) return false;
		DWORD dwWrite = 0;
		WriteFile(ho, m_bufMem, dwRead, &dwWrite, NULL);
		copsz += dwWrite;
		dstsz += dwWrite;

		if (dwRead == BUFSZ)
		{
			CString progresst;
			double pct = (double)dstsz / (double)srcsz;
			progresst.Format(L"%I64dM(%.2lf%%) %s", dstsz/(1024*1024), pct*100.0, srcfile);
			SetNotifyStr(progresst);
		}
		if (m_bStop && srcsz > dstsz)
		{
			return false;
		}
	}
}

BOOL CxMoveMgr::myCreateDirectory(LPCTSTR dir, const WIN32_FIND_DATA * dirwfd)
{
	BOOL b = CreateDirectory(dir, NULL);
	if (dirwfd)
	{
		SetFileAttributes(dir, FILE_ATTRIBUTE_NORMAL);
		HANDLE hf = CreateFile(dir, FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		SetFileTime(hf, &dirwfd->ftCreationTime, &dirwfd->ftLastAccessTime, &dirwfd->ftLastWriteTime);
		CloseHandle(hf);
		SetFileAttributes(dir, dirwfd->dwFileAttributes);
		return dir!=INVALID_HANDLE_VALUE;
	}
	return b;
}
