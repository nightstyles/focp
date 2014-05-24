
#include "DiskFile.hpp"

FOCP_BEGIN();

static CDiskFileInterface g_oDiskFileInterface;

const char* CDiskFileInterface::GetProtocol()
{
	return "disk";
}

CBaseFile* CDiskFileInterface::CreateFile()
{
	return new CDiskFile;
}

void CDiskFileInterface::DestroyFile(CBaseFile* pFile)
{
	if(pFile)
		delete pFile;
}

static void AfcCreateDirectory(const char* sFullName)
{
	CDiskFileSystem::GetInstance()->CreateDirectory(sFullName);
}

FOCP_END();

#if defined(WINDOWS)
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/file.h>
	#include <fcntl.h>
	#include <errno.h>
#endif

FOCP_BEGIN();

CDiskFile::CDiskFile()
{
	m_hHandle = (ulong)(-1);
	m_oFileName.oProtocol = "disk";
}

CDiskFile::~CDiskFile()
{
}

CFileInterface* CDiskFile::GetInterface()
{
	return &g_oDiskFileInterface;
}

int32 CDiskFile::Open(const CFileName& oFileName, const char* sOption)
{
	char sFullName[FOCP_MAX_PATH];
	CPathDetailInfo oDetailInfo;
	CDiskFileSystem* pFileSytem;
	uint32 nOption = GetOpenOption(sOption);
	if(nOption & FOCP_FILE_OPTION_LISTEN)
	{
		if(oFileName.oBindName.Empty())
			return FOCP_FILE_BINDNAME_ERROR;
		return FOCP_FILE_BIND_ERROR;
	}
	if(oFileName.oConnectName.Empty())
		return FOCP_FILE_CONNECTNAME_ERROR;
	pFileSytem = CDiskFileSystem::GetInstance();
	if(!pFileSytem->GetFullPath(oFileName.oConnectName.GetStr(), sFullName, &oDetailInfo))
		return FOCP_FILE_CONNECTNAME_ERROR;
	if(oDetailInfo.bExist && !oDetailInfo.sFilePart)
		return FOCP_FILE_CONNECTNAME_ERROR;
	char* pDir = CString::CharOfString(sFullName, '/', true);
	pDir[0] = '\0';
	AfcCreateDirectory(sFullName);
	pDir[0] = '/';
	m_oFileName.oConnectName = sFullName;
	pFileSytem->GetOsPathName(sFullName);
#if defined(WINDOWS)
	uint32 dwDesiredAccess = 0;
	uint32 dwShareMode = 0;
	uint32 dwCreationDisposition = 0;
	uint32 dwFlagsAndAttributes = 0;
	if(nOption & FOCP_FILE_OPTION_READ)
		dwDesiredAccess |= GENERIC_READ;
	if(nOption & FOCP_FILE_OPTION_WRITE)
		dwDesiredAccess |= GENERIC_WRITE;
	if(nOption & FOCP_FILE_OPTION_SHARE_READ)
		dwShareMode |= FILE_SHARE_READ;
	if(nOption & FOCP_FILE_OPTION_SHARE_WRITE)
		dwShareMode |= FILE_SHARE_WRITE;
	if(nOption & FOCP_FILE_OPTION_CREATE)
		dwCreationDisposition = OPEN_ALWAYS;
	else if(nOption & FOCP_FILE_OPTION_NEW)
		dwCreationDisposition = CREATE_NEW;
	else if(nOption & FOCP_FILE_OPTION_DESTROY)
		dwCreationDisposition = CREATE_ALWAYS;
	else if(nOption & FOCP_FILE_OPTION_APPEND)
		dwCreationDisposition = OPEN_ALWAYS;
	else
		dwCreationDisposition = OPEN_EXISTING;
	dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
	if(nOption & FOCP_FILE_OPTION_WRITE)
		dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
	else
		dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
	m_hHandle = (ulong)CreateFile(sFullName, dwDesiredAccess, dwShareMode, NULL,
								  dwCreationDisposition, dwFlagsAndAttributes, NULL);
	if(m_hHandle == (ulong)(-1))
		return FOCP_FILE_CONNECT_ERROR;
	if(nOption & FOCP_FILE_OPTION_APPEND)
	{
		uint32 nLowSize, nHighSize;
		nLowSize = GetFileSize((HANDLE)m_hHandle, (DWORD*)&nHighSize);
		if(nHighSize || nLowSize > 0x7FFFFFFF)
			nLowSize = 0x7FFFFFFF;
		SetFilePointer((HANDLE)m_hHandle, nLowSize, NULL, FILE_BEGIN);
	}
#else
	uint32 nFlags = 0, nLock;
	if(nOption & FOCP_FILE_OPTION_READ)
		nFlags |= O_RDONLY;
	if(nOption & FOCP_FILE_OPTION_WRITE)
	{
		nFlags &= ~(uint32)O_RDONLY;
		nFlags |= O_RDWR;
	}
	if(nOption & FOCP_FILE_OPTION_CREATE)
		nFlags |= O_CREAT;
	else if(nOption & FOCP_FILE_OPTION_NEW)
		nFlags |= O_CREAT|O_EXCL;
	else if(nOption & FOCP_FILE_OPTION_DESTROY)
		nFlags |= O_CREAT|O_TRUNC;
	else if(nOption & FOCP_FILE_OPTION_APPEND)
		nFlags |= O_CREAT|O_APPEND;
//else
//	nFlags |= OPEN_EXISTING;
	nFlags |= O_SYNC;
	m_hHandle = (ulong)open(sFullName, (int32)nFlags);
	if(m_hHandle == (ulong)(-1))
		return FOCP_FILE_CONNECT_ERROR;
	if(nFlags & O_CREAT)
#ifdef WINDOWS
		chmod(sFullName, _S_IREAD | _S_IWRITE);
#else
		chmod(sFullName, S_IRUSR | S_IWUSR);
#endif
	if( (nOption & FOCP_FILE_OPTION_SHARE_READ) ||
			(nOption & FOCP_FILE_OPTION_SHARE_WRITE))
		nLock = LOCK_SH|LOCK_NB;
	else
		nLock = LOCK_EX|LOCK_NB;
	while(flock((int32)m_hHandle, nLock))
	{
		if(errno != EINTR)
		{
			close(m_hHandle);
			m_hHandle = (ulong)(-1);
			return FOCP_FILE_CONNECT_ERROR;
		}
	}
#endif
	SetStatus(FOCP_FILE_NORMAL);
	return 0;
}

void CDiskFile::Close(ulong* pHandle)
{
	if(pHandle)
		pHandle[0] = m_hHandle;
	if(m_hHandle != (ulong)(-1))
	{
		if(pHandle == NULL)
		{
#if defined(WINDOWS)
			CloseHandle((HANDLE)m_hHandle);
#else
			flock((int32)m_hHandle, LOCK_UN);
			close((int32)m_hHandle);
#endif
		}
		m_hHandle = (ulong)(-1);
		SetStatus(FOCP_FILE_CLOSED);
	}
}

const CFileName& CDiskFile::GetFileName()
{
	return m_oFileName;
}

uint32 CDiskFile::GetType()
{
	return FOCP_STORAGE_FILE|FOCP_CONNECT_FILE|FOCP_STREAM_FILE;
}

int32 CDiskFile::Read(void* pBuf, int32 nBufLen, uint32)
{
	uint32 nRet;
	if(m_hHandle == (ulong)(-1))
		return -1;
#if defined(WINDOWS)
	if(!ReadFile((HANDLE)m_hHandle, pBuf, (uint32)nBufLen, (DWORD*)&nRet, NULL))
	{
		if(GetLastError() == ERROR_HANDLE_EOF)
			nRet = 0;
		else
			nRet = (uint32)(-1);
	}
#else
loop:
	nRet = read((int32)m_hHandle, pBuf, nBufLen);
	if(nRet == (uint32)(-1))
	{
		if(errno == EINTR)
			goto loop;
	}
#endif
	if(nRet == 0)
		SetStatus(FOCP_FILE_BROKEN);
	else if(0 > (int32)nRet)
		SetStatus(FOCP_FILE_BAD);
	return (int32)nRet;
}

int32 CDiskFile::Write(const void* pBuf, int32 nBufLen)
{
	uint32 nRet;
	if(m_hHandle == (ulong)(-1))
		return -1;
#if defined(WINDOWS)
	if(!WriteFile((HANDLE)m_hHandle, pBuf, (uint32)nBufLen, (DWORD*)&nRet, NULL))
		nRet = (uint32)(-1);
#else
loop:
	nRet = write((int32)m_hHandle, pBuf, nBufLen);
	if(nRet == (uint32)(-1))
	{
		if(errno == EINTR)
			goto loop;
	}
#endif
	if(nRet == 0)
		SetStatus(FOCP_FILE_BROKEN);
	else if(0 > (int32)nRet)
		SetStatus(FOCP_FILE_BAD);
	return (int32)nRet;
}

int32 CDiskFile::GetPosition()
{
	if(m_hHandle == (ulong)(-1))
		return -1;
#if defined(WINDOWS)
	return SetFilePointer((HANDLE)m_hHandle, 0, NULL, FILE_CURRENT);
#else
	return lseek((int32)m_hHandle, 0, SEEK_CUR);
#endif
}

void CDiskFile::SetPosition(int32 nPos)
{
	if(m_hHandle == (ulong)(-1))
		return;
#if defined(WINDOWS)
	SetFilePointer((HANDLE)m_hHandle, nPos, NULL, FILE_BEGIN);
#else
	lseek((int32)m_hHandle, nPos, SEEK_SET);
#endif
}

void CDiskFile::Seek(uint32 nOrigin, int32 nOffset)
{
	if(m_hHandle == (ulong)(-1))
		return;
#if defined(WINDOWS)
	int32 nCurPos;
	uint32 nHighSize;
	switch(nOrigin)
	{
	case FOCP_SEEK_SET:
		nOrigin = FILE_BEGIN;
		nCurPos = 0;
		break;
	case FOCP_SEEK_CUR:
		nOrigin = FILE_CURRENT;
		nCurPos = SetFilePointer((HANDLE)m_hHandle, 0, NULL, nOrigin);
		break;
	case FOCP_SEEK_END:
		nOrigin = FILE_END;
		nCurPos = GetFileSize((HANDLE)m_hHandle, (DWORD*)&nHighSize);
		if(nHighSize || nCurPos > 0x7FFFFFFF)
			nCurPos = 0x7FFFFFFF;
		break;
	}
	nOffset += nCurPos;
	if(nOffset < 0)
		nOffset = 0;
	else if(nOffset > 0x7FFFFFFF)
		nOffset = 0x7FFFFFFF;
	SetFilePointer((HANDLE)m_hHandle, nOffset - nCurPos, NULL, nOrigin);
#else
	switch(nOrigin)
	{
	case FOCP_SEEK_SET:
		nOrigin = SEEK_SET;
		break;
	case FOCP_SEEK_CUR:
		nOrigin = SEEK_CUR;
		break;
	case FOCP_SEEK_END:
		nOrigin = SEEK_END;
		break;
	}
	lseek((int32)m_hHandle, nOffset, nOrigin);
#endif
}

void CDiskFile::Truncate()
{
	if(m_hHandle == (ulong)(-1))
		return;
#if defined(WINDOWS)
	SetEndOfFile((HANDLE)m_hHandle);
#else
	ftruncate((int32)m_hHandle, lseek((int32)m_hHandle, 0, SEEK_CUR));
#endif
}

bool CDiskFile::Lock(uint32 nLock, int32 nSize)
{
	if(m_hHandle == (ulong)(-1))
		return false;
#if defined(WINDOWS)
	BOOL bRet;
	switch(nLock)
	{
	default:
		return false;
	case FOCP_FILE_LOCK:
		bRet = LockFile((HANDLE)m_hHandle, GetPosition(), 0, nSize, 0);
		break;
	case FOCP_FILE_TLOCK:
		bRet = LockFileEx((HANDLE)m_hHandle, LOCKFILE_FAIL_IMMEDIATELY, 0, nSize, 0, NULL);
		break;
	case FOCP_FILE_ULOCK:
		bRet = UnlockFile((HANDLE)m_hHandle, GetPosition(), 0, nSize, 0);
		break;
	}
	if(bRet)
		return true;
	return false;
#else
	int32 nRet;
	switch(nLock)
	{
	default:
		return false;
	case FOCP_FILE_LOCK:
		nLock = F_LOCK;
		break;
	case FOCP_FILE_TLOCK:
		nLock = F_TLOCK;
		break;
	case FOCP_FILE_ULOCK:
		nLock = F_ULOCK;
		break;
	}
loop:
	nRet = lockf((int32)m_hHandle, nLock, nSize);
	if(nRet)
	{
		if(EINTR == errno)
			goto loop;
		return false;
	}
	return true;
#endif
}

void CDiskFile::Flush()
{
#ifdef WINDOWS
	FlushFileBuffers((HANDLE)m_hHandle);
#else
	fdatasync((int32)m_hHandle);
#endif
}

bool CDiskFile::ReadWriteCritical()
{
	return true;
}

FOCP_END();
