
#include "VmmFile.hpp"

FOCP_BEGIN();

CVirtualFile::CVirtualDiskFile::CVirtualDiskFile()
{
	nSize = 0;
	bDirty = 0;
}

CVirtualFile::CVirtualDiskFile::~CVirtualDiskFile()
{
	bDirty = 0;
	nSize = 0;
}

CVirtualFile::CVirtualMemFile::CVirtualMemFile()
{
	nSize = 0;
	pPages = NULL;
}

CVirtualFile::CVirtualMemFile::~CVirtualMemFile()
{
	if(pPages)
	{
		CMalloc::Free(pPages);
		pPages = NULL;
	}
	nSize = 0;
}

CVirtualFile::~CVirtualFile()
{
	Close();
}

void CVirtualFile::Close()
{
	if(m_pMemFile)
	{
		delete m_pMemFile;
		m_pMemFile = NULL;
	}
	if(m_pDiskFile)
	{
		delete m_pDiskFile;
		m_pDiskFile = NULL;
	}
}

CVirtualFile::CVirtualFile(uint32 nSize)
{
	if(nSize > VMM_MAX_FILESIZE)
		nSize = VMM_MAX_FILESIZE;
	m_pDiskFile = NULL;
	m_pMemFile = new(std::nothrow) CVirtualMemFile;
	if(!m_pMemFile)
		FocpError(("CVirtualFile::CVirtualFile(%u) failure: memory is not enough", nSize));
	else
	{
		m_pMemFile->pPages = (uint8*)CMalloc::Malloc(nSize);
		if(m_pMemFile->pPages)
		{
			m_pMemFile->nSize = nSize;
			CBinary::MemorySet(m_pMemFile->pPages, 0, nSize);
		}
		else
			FocpError(("CVirtualFile::CVirtualFile(%u) failure: memory is not enough", nSize));		
	}
}

CVirtualFile::CVirtualFile(const char* sFileName, uint32 nSize, bool bCreate)
{
	CString oFileName("disk://");
	oFileName+=sFileName;
	const char* sOption;
	if(bCreate)
		sOption = "rwn";
	else
		sOption = "rw";
	sFileName = oFileName.GetStr();
	m_pMemFile = NULL;
	m_pDiskFile = new(std::nothrow) CVirtualDiskFile;
	if(!m_pDiskFile)
	{
		FocpError(("CVirtualFile::CVirtualFile(%s) failue: memory is not enough", sFileName));
		return;
	}
	switch(m_pDiskFile->nFile.Open(sFileName, sOption))
	{
	case FOCP_FILE_PROTOCOL_ERROR:
		FocpError(("CVirtualFile::CreateFile(%s) failure: protcol error", sFileName));
		return;
	case FOCP_FILE_CONNECTNAME_ERROR:
		FocpError(("CVirtualFile::CreateFile(%s) failure: FileName error", sFileName));
		return;
	case FOCP_FILE_CONNECT_ERROR:
		FocpError(("CVirtualFile::CreateFile(%s) failure: Open failure", sFileName));
		return;
	case FOCP_FILE_OPTION_ERROR:
		FocpError(("CVirtualFile::CreateFile(%s) failure: Option failure", sFileName));
		return;
	case FOCP_FILE_OTHER_ERROR:
		FocpError(("CVirtualFile::CreateFile(%s) failure: Other failure", sFileName));
		return;
	}

	if(bCreate)
	{
		static char c = 0;
		if(nSize > VMM_MAX_FILESIZE)
			nSize = VMM_MAX_FILESIZE;
		m_pDiskFile->nFile.SetPosition(nSize-1);
		if(m_pDiskFile->nFile.Write(&c, 1) == 1)
		{
			m_pDiskFile->nSize = nSize;
			m_pDiskFile->bDirty = 1;
		}
		else
		{
			FocpError(("CVirtualFile::CreateFile(%s) failure: disk space isnot enough", sFileName));
			m_pDiskFile->nFile.Close();
		}
	}
	else
	{
		m_pDiskFile->nFile.Seek(FOCP_SEEK_END, 0);
		if(nSize == (uint32)m_pDiskFile->nFile.GetPosition())
			m_pDiskFile->nSize = nSize;
		else
		{
			FocpError(("CVirtualFile::OpenFile(%s) failure: invalid file formart", sFileName));
			m_pDiskFile->nFile.Close();
		}
	}
}

bool CVirtualFile::IsValidFile()
{
	if(m_pMemFile && m_pMemFile->pPages)
		return true;
	if(m_pDiskFile && m_pDiskFile->nFile.GetStatus() == 0)
		return true;
	return false;
}

uint32 CVirtualFile::GetSize()
{
	uint32 nSize = 0;
	if(m_pMemFile)
		nSize = m_pMemFile->nSize;
	else if(m_pDiskFile)
		nSize = m_pDiskFile->nSize;
	return nSize;
}

int32 CVirtualFile::Expand(uint32 nSize)
{
	if(m_pMemFile)
		return 1;
	if(m_pDiskFile)
	{
		static char c = 0;
		nSize += m_pDiskFile->nSize;
		m_pDiskFile->nFile.SetPosition(nSize-1);
		if(m_pDiskFile->nFile.Write(&c, 1) == 1)
		{
			m_pDiskFile->nSize = nSize;
			m_pDiskFile->bDirty = 1;
			return 0;
		}
		else
			m_pDiskFile->nFile.Truncate();
	}
	return 1;
}

void CVirtualFile::Flush()
{
	if(m_pDiskFile)
	{
		if(m_pDiskFile->bDirty)
		{
			m_pDiskFile->bDirty = 0;
			m_pDiskFile->nFile.Flush();
		}
	}
}

void* CVirtualFile::GetMemoryAddr(uint32 nAddr)
{
	if(m_pMemFile && nAddr < m_pMemFile->nSize)
		return m_pMemFile->pPages+nAddr;
	return NULL;
}

int32 CVirtualFile::Read(uint32 nAddr, char* pBuf, int32 nSize)
{
	int32 nRet = -1;
	if(m_pDiskFile)
	{
		int32 nPos = m_pDiskFile->nFile.GetPosition();
		m_pDiskFile->nFile.Seek(FOCP_SEEK_CUR, (int32)nAddr-nPos);
		nRet = m_pDiskFile->nFile.Read(pBuf, nSize);
	}
	else if(m_pMemFile)
	{
		nRet = 0;
		uint32 nFileSize = m_pMemFile->nSize;
		if(nAddr >= nFileSize)
			return nRet;
		nRet = nFileSize - nAddr;
		if(nRet > nSize)
			nRet = nSize;
		CBinary::MemoryCopy(pBuf, m_pMemFile->pPages+nAddr, nRet);
	}
	return nRet;
}

int32 CVirtualFile::Write(uint32 nAddr, char* pBuf, int32 nSize)
{
	int32 nRet = -1;
	if(m_pDiskFile)
	{
		int32 nPos = m_pDiskFile->nFile.GetPosition();
		m_pDiskFile->nFile.Seek(FOCP_SEEK_CUR, (int32)nAddr-nPos);
		nRet = m_pDiskFile->nFile.Read(pBuf, nSize);
		if(nRet > 0)
			m_pDiskFile->bDirty = 1;
	}
	else if(m_pMemFile)
	{
		nRet = 0;
		uint32 nFileSize = m_pMemFile->nSize;
		if(nAddr >= nFileSize)
			return nRet;
		nRet = nFileSize - nAddr;
		if(nRet > nSize)
			nRet = nSize;
		CBinary::MemoryCopy(m_pMemFile->pPages+nAddr, pBuf, nRet);
	}
	return nRet;
}

FOCP_END();
