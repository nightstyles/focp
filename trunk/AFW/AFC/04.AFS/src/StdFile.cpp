
#include "StdFile.hpp"

FOCP_BEGIN();

static CFileInterface& GetStdFileInterface()
{
	return *CSingleInstance<CStdFileInterface>::GetInstance();
}

const char* CStdFileInterface::GetProtocol()
{
	return "stdio";
}

CBaseFile* CStdFileInterface::CreateFile()
{
	return new CStdFile;
}

void CStdFileInterface::DestroyFile(CBaseFile* pFile)
{
	if(pFile)
		delete pFile;
}

const char* CMemoryFileInterface::GetProtocol()
{
	return "memory";
}

CBaseFile* CMemoryFileInterface::CreateFile()
{
	return new CMemoryFile;
}

void CMemoryFileInterface::DestroyFile(CBaseFile* pFile)
{
	if(pFile)
		delete pFile;
}

static CMemoryFileInterface g_oMemoryFileItf;

FOCP_END();

#if defined(WINDOWS)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

FOCP_BEGIN();

CStdFile::CStdFile()
{
	m_hHandle = -1;
	m_oFileName.oProtocol = "stdio";
}

CStdFile::~CStdFile()
{
}

CFileInterface* CStdFile::GetInterface()
{
	return &GetStdFileInterface();
}

int32 CStdFile::Open(const CFileName& oFileName, const char* sOption)
{
	uint32 nOption = GetOpenOption(sOption);
	if(nOption & FOCP_FILE_OPTION_LISTEN)
		return FOCP_FILE_BIND_ERROR;
	if(!oFileName.oBindName.Empty())
		return FOCP_FILE_BINDNAME_ERROR;
	if(!oFileName.oConnectName.CString::Compare("in", false))
		m_hHandle = 0;
	else if(!oFileName.oConnectName.CString::Compare("out", false))
		m_hHandle = 1;
	else if(!oFileName.oConnectName.CString::Compare("err", false))
		m_hHandle = 2;
	else
		return FOCP_FILE_CONNECTNAME_ERROR;
	m_oFileName.oConnectName = oFileName.oConnectName;
	SetStatus(FOCP_FILE_NORMAL);
	return 0;
}

void CStdFile::Close()
{
	m_hHandle = -1;
	SetStatus(FOCP_FILE_CLOSED);
}

const CFileName& CStdFile::GetFileName()
{
	return m_oFileName;
}

uint32 CStdFile::GetType()
{
	return FOCP_STREAM_FILE;
}

int32 CStdFile::Read(void* pBuf, int32 nBufLen, uint32)
{
	int32 nRet;
	if(m_hHandle == -1)
		return -1;
	if(m_hHandle)
		return 0;
#ifdef UNIX
loop:
#endif
	nRet = read(m_hHandle, pBuf, nBufLen);
#ifdef UNIX
	if(nRet == -1 && errno == EINTR)
		goto loop;
#endif
	if(nRet == 0)
		SetStatus(FOCP_FILE_BROKEN);
	else if(0 > nRet)
		SetStatus(FOCP_FILE_BAD);
	return nRet;
}

int32 CStdFile::Write(const void* pBuf, int32 nBufLen)
{
	int32 nRet;
	if(m_hHandle == -1)
		return -1;
	if(!m_hHandle)
		return 0;
#ifdef UNIX
loop:
#endif
	nRet = write(m_hHandle, pBuf, nBufLen);
#ifdef UNIX
	if(nRet == -1 && errno == EINTR)
		goto loop;
#endif
	if(nRet == 0)
		SetStatus(FOCP_FILE_BROKEN);
	else if(0 > nRet)
		SetStatus(FOCP_FILE_BAD);
	return nRet;
}

CMemoryFile::CMemoryFile()
{
	m_pStream = NULL;
	m_oFileName.oProtocol = "memory";
}

CMemoryFile::~CMemoryFile()
{
	if(m_pStream)
	{
		delete m_pStream;
		m_pStream = NULL;
	}
}

CFileInterface* CMemoryFile::GetInterface()
{
	return &g_oMemoryFileItf;
}

int32 CMemoryFile::Open(const CFileName& oFileName, const char* sOption)
{
	uint32 nOption = GetOpenOption(sOption);
	if(nOption & FOCP_FILE_OPTION_LISTEN)
		return FOCP_FILE_BIND_ERROR;
	if(oFileName.oBindName.Empty())
		return FOCP_FILE_BINDNAME_ERROR;
	if(!oFileName.oConnectName.Empty())
		return FOCP_FILE_BINDNAME_ERROR;
	if(FOCP_FILE_OPTION_READ & nOption)
		m_bRead = true;
	else
		m_bRead = false;
	if(FOCP_FILE_OPTION_WRITE & nOption)
		m_bWrite = true;
	else
		m_bWrite = false;
	m_oFileName.oBindName = oFileName.oBindName;
	m_oFileName.oConnectName = oFileName.oConnectName;
	char* pBuf = NULL;
	uint32 nSize = 0;
	CStringFormatter oFmt((CString*)&oFileName.oBindName);
	oFmt.Scan("%p:%d", &pBuf, &nSize);
	m_pStream = new CMemoryStream(pBuf, nSize);
	SetStatus(FOCP_FILE_NORMAL);
	return 0;
}

void CMemoryFile::Close()
{
	if(m_pStream)
	{
		delete m_pStream;
		SetStatus(FOCP_FILE_CLOSED);
	}
}

uint32 CMemoryFile::GetType()
{
	return FOCP_STREAM_FILE;
}

const CFileName& CMemoryFile::GetFileName()
{
	return m_oFileName;
}

int32 CMemoryFile::Read(void* pBuf, int32 nBufLen, uint32)
{
	if(m_pStream == NULL)
		return -1;
	if(!m_bRead)
		return 0;
	return m_pStream->Read(pBuf, nBufLen);
}

int32 CMemoryFile::Write(const void* pBuf, int32 nBufLen)
{
	if(m_pStream == NULL)
		return -1;
	if(!m_bWrite)
		return 0;
	return m_pStream->Write((void*)pBuf, nBufLen);
}

struct CStdInFile
{
	CFile oFile;
	CStdInFile()
	{
		GetStdFileInterface();
		oFile.Open("stdio://in");
	}
};

AFS_API CFile& GetStdIn()
{
	return CSingleInstance<CStdInFile>::GetInstance()->oFile;
}

struct CStdOutFile
{
	CFile oFile;
	CStdOutFile()
	{
		GetStdFileInterface();
		oFile.Open("stdio://out");
	}
};

AFS_API CFile& GetStdOut()
{
	return CSingleInstance<CStdOutFile>::GetInstance()->oFile;
}

struct CStdErrFile
{
	CFile oFile;
	CStdErrFile()
	{
		GetStdFileInterface();
		oFile.Open("stdio://err");
	}
};

AFS_API CFile& GetStdErr()
{
	return CSingleInstance<CStdErrFile>::GetInstance()->oFile;
}

FOCP_END();
