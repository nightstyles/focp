
#include "Redo.hpp"
#include "Process.hpp"

FOCP_BEGIN();

static const char* GetShellPath(char* sShellPath)
{
	CString::StringCopy(sShellPath, CFilePathInfo::GetInstance()->GetDir());
#ifdef WINDOWS
	CDiskFileSystem::GetInstance()->GetOsPathName(sShellPath);
#endif
	return sShellPath;
}

CRedoLog::CRedoLog()
{
	m_OnRedo = NULL;
	m_oControlInfo.nFileNo = 0;
	m_oControlInfo.nPos = 0;
	m_oControlInfo.nSequence = 0;
}

CRedoLog::~CRedoLog()
{
}

uint32 CRedoLog::Create(char* sRdoFile, char* sPath, FOnDoRedo OnRedo, uint32 nLogFileSize, uint32 nLogFileNum)
{
	m_nLogFileSize = nLogFileSize;
	m_nLogFileNum = nLogFileNum;
	m_oPath.Print("disk://%s", sPath);
	m_OnRedo = OnRedo;
	CFormatString oFileName;
	oFileName.Print("disk://%s", sRdoFile);
	sRdoFile = (char*)oFileName.GetStr();
	bool bNew = false;
	if(m_nCtrlFile.Open(sRdoFile, "w"))
	{
		bNew = true;
		if(m_nCtrlFile.Open(sRdoFile, "c"))
		{
			FocpError(("create redo control file failure"));
			return 1;
		}
	}
	CRedoControlInfo pControlInfo[2] = { {0, 0, 0}, {0, 0, 0} };
	if(bNew)
	{
		if(m_nCtrlFile.Write(pControlInfo, sizeof(pControlInfo)) != (int32)sizeof(pControlInfo))
		{
			FocpError(("read redo control file failure"));
			CDiskFileSystem::GetInstance()->RemovePath(sRdoFile+7);
			return 1;
		}
	}
	else
	{
		if(m_nCtrlFile.Write(pControlInfo, sizeof(pControlInfo)) != (int32)sizeof(pControlInfo))
		{
			FocpError(("read redo control file failure"));
			return 1;
		}
	}
	if(pControlInfo[0].nSequence != pControlInfo[1].nSequence)
		m_oControlInfo = pControlInfo[0];
	else
		m_oControlInfo = pControlInfo[1];
	Commit();
	return 0;
}

void CRedoLog::Reset()
{
	CloseRedoFile();
	m_oControlInfo.nFileNo = 0;
	m_oControlInfo.nPos = 0;
	Commit();
	char* sPath = new char[FOCP_MAX_PATH];
	CString::StringCopy(sPath, m_oPath.GetStr()+7);
	CDiskFileSystem::GetInstance()->GetOsPathName(sPath);
	CFormatString oCommand;
	if(RunInWindows())
	{
		char* sShellPath = new char[FOCP_MAX_PATH];
		GetShellPath(sPath);
		oCommand.Print("%s/rm.exe -rf %s/*.rdo", sShellPath, sPath);
		delete[] sShellPath;
	}
	else
		oCommand.Print("rm -rf %s/*.rdo", sPath);
	System(oCommand.GetStr());
	delete[] sPath;
}

void CRedoLog::Commit()
{
	CRedoControlInfo pControlInfo[2];
	m_oControlInfo.nSequence++;
	pControlInfo[0] = m_oControlInfo;
	pControlInfo[1] = m_oControlInfo;
	m_nCtrlFile.Seek(FOCP_SEEK_SET, 0);
	m_nCtrlFile.Write(pControlInfo, sizeof(pControlInfo));
}

uint32 CRedoLog::WriteRedo(char* sRedo, uint32 nSize)
{
	m_oMutex.Enter();
	if(m_nFile.GetStatus())
	{
		if(OpenRedoFile())
		{
			m_oMutex.Leave();
			return 1;
		}
	}
	if(m_oControlInfo.nPos + nSize + 4 > m_nLogFileSize)
	{
		CloseRedoFile();
		++m_oControlInfo.nFileNo;
		if(OpenRedoFile())
		{
			--m_oControlInfo.nFileNo;
			m_oMutex.Leave();
			return 1;
		}
		m_oControlInfo.nPos = 0;
		Commit();
	}
	if(m_nFile.Write(&nSize, sizeof(nSize)) != (int32)sizeof(nSize) || m_nFile.Write(sRedo, nSize) != (int32)nSize)
	{
		m_nFile.Seek(FOCP_SEEK_SET, m_oControlInfo.nPos);
		m_nFile.Truncate();
		m_oMutex.Leave();
		return 1;
	}
	m_oControlInfo.nPos += nSize + 4;
	Commit();
	m_oMutex.Leave();
	return 0;
}

uint32 CRedoLog::WriteRedoStream(CMemoryStream* pStream)
{
	char pBuf[1024];

	uint32 nReadSize;
	uint32 nSize = pStream->GetSize();
	pStream->SetPosition(0);

	m_oMutex.Enter();
	if(m_nFile.GetStatus())
	{
		if(OpenRedoFile())
		{
			m_oMutex.Leave();
			return 1;
		}
	}
	if(m_oControlInfo.nPos + nSize + 4 > m_nLogFileSize)
	{
		CloseRedoFile();
		++m_oControlInfo.nFileNo;
		if(OpenRedoFile())
		{
			--m_oControlInfo.nFileNo;
			m_oMutex.Leave();
			return 1;
		}
		m_oControlInfo.nPos = 0;
		Commit();
	}
	if(m_nFile.Write(&nSize, sizeof(nSize)) != (int32)sizeof(nSize))
	{
		m_nFile.Seek(FOCP_SEEK_SET, m_oControlInfo.nPos);
		m_nFile.Truncate();
		m_oMutex.Leave();
		return 1;
	}
	uint32 nOldSize = nSize;
	while(nSize)
	{
		nReadSize = nSize;
		if(nReadSize > 1024)
			nReadSize = 1024;
		pStream->Read(pBuf, nReadSize);
		if(m_nFile.Write(pBuf, nReadSize) != (int32)nReadSize)
		{
			m_nFile.Seek(FOCP_SEEK_SET, m_oControlInfo.nPos);
			m_nFile.Truncate();
			m_oMutex.Leave();
			return 1;
		}
		nSize -= nReadSize;
	}
	m_oControlInfo.nPos += nOldSize + 4;
	Commit();
	m_oMutex.Leave();
	return 0;
}

void CRedoLog::Redo()
{
	char* sRedo = NULL;
	uint32 nFileNo = m_oControlInfo.nFileNo;
	CloseRedoFile();
	for(uint32 i=0; i<=nFileNo; ++i)
	{
		m_oControlInfo.nFileNo = i;
		if(OpenRedoFile(true))
			continue;
		uint32 nSize;
		while(true)
		{
			if(m_nFile.Read(&nSize, sizeof(nSize)) != (int32)sizeof(nSize))
				break;
			if(nSize)
			{
				sRedo = (char*)CMalloc::Realloc(sRedo, nSize);
				if(m_nFile.Read(sRedo, nSize) != (int32)nSize)
					break;
				m_OnRedo(sRedo, nSize);
			}
		}
		CloseRedoFile();
	}
	if(sRedo)
		CMalloc::Free(sRedo);
	m_oControlInfo.nFileNo = nFileNo;
}

uint32 CRedoLog::OpenRedoFile(bool bNotCreate)
{
	CFormatString oFileName;
	oFileName.Print("%s/%.10u.rdo", m_oPath.GetStr(), m_oControlInfo.nFileNo);
	m_nFile.Close();
	if(m_nFile.Open(oFileName.GetStr(), "w"))
	{
		m_nFile.Close();
		if(bNotCreate)
			return 1;
		if(m_oControlInfo.nFileNo >= m_nLogFileNum)
			return 1;
		if(m_nFile.Open(oFileName.GetStr(), "c"))
		{
			m_nFile.Close();
			return 1;
		}
	}
	return 0;
}

void CRedoLog::CloseRedoFile()
{
	m_nFile.Close();
}

FOCP_END();
