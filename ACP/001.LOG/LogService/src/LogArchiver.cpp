
#include "LogArchiver.hpp"

FOCP_BEGIN();

CArchiverTable::CArchiverTable()
{
}

CArchiverTable::~CArchiverTable()
{
	Clear();
}

void CArchiverTable::Clear()
{
	m_oMutex.Enter();
	while(m_oArchiverTable.GetSize())
		delete m_oArchiverTable.GetItem(m_oArchiverTable.First());
	m_oMutex.Leave();
}

void CArchiverTable::StartAll()
{
	CRbTreeNode* pEnd = m_oArchiverTable.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oArchiverTable.First();
	for(; pIt!=pEnd; pIt=m_oArchiverTable.GetNext(pIt))
	{
		CLogArchiver* pItem = m_oArchiverTable.GetItem(pIt);
		pItem->Register();
	}
	m_oMutex.Leave();
}

CArchiverTable* CArchiverTable::GetInstance()
{
	return CSingleInstance<CArchiverTable>::GetInstance();
}

CLogArchiver::CLogArchiver()
{
	m_bNeedArch = false;
	m_nFilter = 15;
}

CLogArchiver::~CLogArchiver()
{
	m_oFile.Redirect(NULL);
	if(m_bNeedArch)
		DoArchive();
	if(!m_oLogName.Empty())
	{
		CArchiverTable* pTable = CArchiverTable::GetInstance();
		pTable->m_oMutex.Enter();
		pTable->m_oArchiverTable.Remove(m_oLogName);
		pTable->m_oMutex.Leave();
	}
}

bool CLogArchiver::Initialize(const char* sLogName, const char* sLogDir, uint32 nMaxLogNo, uint32 nLogSize,
							  bool bNeedArch, const char* sArchDir, uint32 nMaxArchNo)
{
	uint32 nVal;
	const char* s;
	CPathDetailInfo oDetail;
	char sTmp[FOCP_MAX_PATH];
	CDiskFileSystem* pFS = CDiskFileSystem::GetInstance();
	m_oLogName = sLogName;
	if(!m_oLogName.IsIdentifierOfC())
	{
		FocpError(("Logname '%s' isn't a c-identifier", m_oLogName.GetStr()));
		return false;
	}
	if(!pFS->GetFullPath(sLogDir, sTmp, &oDetail))
	{
		FocpError(("Check logdir('%s') failure", sLogDir));
		return false;
	}
	if(!oDetail.bExist || oDetail.sFilePart)
	{
		FocpError(("Check logdir('%s') failure", sLogDir));
		return false;
	}
	m_oLogDir = sTmp;

	//FocpLogFileSize
	s = GetEnvVar("FocpLogFileNo");
	nVal = FOCP_LOG_FILENO;
	if(s)
	{
		nVal = CString::Atoi(s);
		if(nVal < 9)
			nVal = 9;
		else if(nVal > FOCP_LOG_FILENO)
			nVal = FOCP_LOG_FILENO;
	}
	if(!nMaxLogNo || nMaxLogNo>nVal)
		nMaxLogNo = nVal;
	else if(nMaxLogNo < 9)
		nMaxLogNo = 9;
	if(!nMaxArchNo || nMaxArchNo>nVal)
		nMaxArchNo = nVal;
	else if(nMaxArchNo < 9)
		nMaxArchNo = 9;
	m_nLogNo = nMaxLogNo;
	m_nArchNo = nMaxArchNo;

	s = GetEnvVar("FocpLogFileSize");
	nVal = FOCP_LOG_MAXFILE;
	if(s)
	{
		nVal = CString::Atoi(s);
		if(nVal < FOCP_LOG_MINFILE)
			nVal = FOCP_LOG_MINFILE;
		else if(nVal > FOCP_LOG_MAXFILE)
			nVal = FOCP_LOG_MAXFILE;
	}
	if(!nLogSize || nLogSize>nVal)
		nLogSize = nVal;
	else if(nLogSize < FOCP_LOG_MINFILE)
		nLogSize = FOCP_LOG_MINFILE;
	m_nLogSize = nLogSize * 1048576;

	void *pDir;
	bool bIsDirectory;
	const char* pFileName;
	CFormatString oFmt;
	if(bNeedArch)
	{
		if(!sArchDir || !sArchDir[0])
			CString::StringCopy(sTmp, m_oLogDir.GetStr());
		else
		{
			if(!pFS->GetFullPath(sArchDir, sTmp, &oDetail))
			{
				FocpError(("Check archdir('%s') failure", sLogDir));
				return false;
			}
			if(!oDetail.bExist || oDetail.sFilePart)
			{
				FocpError(("Check archdir('%s') failure", sLogDir));
				return false;
			}
		}
		sArchDir = sTmp;
		m_nArchId = 0;
		oFmt.Print("%s.%%3d.log.gz%%r", m_oLogName.GetStr());
		pDir = pFS->OpenDirectory(sArchDir);
		while((pFileName = pFS->ReadDirectory(pDir, bIsDirectory)))
		{
			if(!bIsDirectory)
			{
				uint32 nArchId;
				CString oLine;
				int32 nLen = CFormatString(pFileName).Scan(oFmt.GetStr(), &nArchId, &oLine);
				if(nLen == 1 && nArchId > m_nArchId)
					m_nArchId = nArchId;
			}
		}
		pFS->CloseDirectory(pDir);
		pFS->GetOsPathName(sTmp);
		m_oArchDir = sTmp;
	}
	m_nLogId = 0;
	oFmt.Clear();
	oFmt.Print("%s.%%3d.log%%r", m_oLogName.GetStr());
	pDir = pFS->OpenDirectory(m_oLogDir.GetStr());
	while((pFileName = pFS->ReadDirectory(pDir, bIsDirectory)))
	{
		if(!bIsDirectory)
		{
			uint32 nLogId;
			CString oLine;
			int32 nLen = CFormatString(pFileName).Scan(oFmt.GetStr(), &nLogId, &oLine);
			if(nLen == 1 && nLogId > m_nLogId)
				m_nLogId = nLogId;
		}
	}
	pFS->CloseDirectory(pDir);
	m_bNeedArch = bNeedArch;
	return true;
}

void CLogArchiver::Process(CLogMsg& oLog)
{
	CString oLogInfo;
	GetLogInfo(oLog, oLogInfo);
	if(NotAccess())
	{
		++m_nLogId;
		if(m_nLogId > m_nLogNo)
		{
			if(m_bNeedArch)
			{
				m_oFile.Redirect(NULL);
				DoArchive();
			}
			m_nLogId = 1;
		}
		CreateLogFile();
	}
	m_oFile.Write(oLogInfo.GetStr(), oLogInfo.GetSize());
}

void CLogArchiver::RegisterCmd()
{
	CCmdSystem* pCmdSystem = CCmdSystem::GetInstance();
	pCmdSystem->RegisterCmd("/Log/Archive/Select", "Select logname fields<CR>\r\n"
							"\tSelect some fields to archive, these field include:\r\n"
							"\tHost,Date,App,Module,File,Func. The '*' is expressed all fields\r\n", Select);
	pCmdSystem->RegisterCmd("/Log/Archive/Push", "Push logname chooser<CR>\r\n"
							"\tAdd a choice rule into the archiver.\r\n"
							"\tThe chooser is same as SQL where clause statement, the field include:\r\n"
							"\tHost,Date,App,Module,Func,File,Key,Level,DMN,AIN,Line.\r\n"
							"\tThe operator incaludes: '=' and '!=' .\r\n"
							"\tThe last 3 fields are integer type,\r\n"
							"\tThe 'Date' field is datetime type: [a,b];[a,b);(a,b];(a,b). 'a' and 'b' is string format\r\n"
							"\tThe other fields are string type:\r\n"
							"\tThe string is a sequences of characters enclosed in single quotation marks,\r\n"
							"\tand two consecutive single quotes is expressed as a single quote\r\n", Push);
	pCmdSystem->RegisterCmd("/Log/Archive/Pop", "Pop logname <CR>\r\n"
							"\tRemove the last choice rule from the archiver.\r\n", Pop);
	pCmdSystem->RegisterCmd("/Log/Archive/Clear", "Clear logname <CR>\r\n"
							"\tClear all choice rules in the archiver\r\n", Clear);
	pCmdSystem->RegisterCmd("/Log/Archive/Query", "Query logname <CR>\r\n"
							"\tQuery the archiver's chooser\r\n", Query);
	pCmdSystem->RegisterCmd("/Log/Archive/Add", "Add arguments<CR>\r\n"
							"\tAdd a new archive, the arguments's format is 'Name=Value{,Name=Value}'\r\n"
							"\tThe name include: LogName, LogDir, MaxLogNo, LogSize, NeedArch, ArchDir, MaxArchNo\r\n"
							"\t'MaxLogNo', 'LogSize', 'NeedArch' and 'MaxArchNo' are integer type\r\n"
							"\tThe Other arguments are string type:\r\n"
							"\tThe string is a sequences of characters enclosed in single quotation marks,\r\n"
							"\tand two consecutive single quotes is expressed as a single quote\r\n"
							"\t'LogName' is the required parameter, the others are optional.\r\n", Add);
	pCmdSystem->RegisterCmd("/Log/Archive/Del", "Del logname<CR>\r\n"
							"\tRemove the archive\r\n", Del);
	pCmdSystem->RegisterCmd("/Log/Archive/List", "List <CR>\r\n"
							"\tList all archives\r\n", List);
	pCmdSystem->RegisterCmd("/Log/Archive/Begin", "Begin logname<CR>\r\n"
							"\tStartup the archive\r\n", Begin);
	pCmdSystem->RegisterCmd("/Log/Archive/End", "End logname<CR>\r\n"
							"\tSuspend the archive\r\n", End);
}

void CLogArchiver::CreateLogFile()
{
	m_oFileName.Clear();
	CStringFormatter(&m_oFileName).Print("disk://%s/%s.%03d.log", m_oLogDir.GetStr(), m_oLogName.GetStr(), m_nLogId);
	m_oFile.Redirect(NULL);
	m_oFile.Open(m_oFileName.GetStr(), "rwcd");
}

bool CLogArchiver::NotAccess()
{
	bool bDirectory = false;
	return (m_oFile.GetStatus() != FOCP_FILE_NORMAL) ||
		   !CDiskFileSystem::GetInstance()->AccessPath(m_oFileName.GetStr()+7, FOCP_FILEACCESS_WRITABLE, bDirectory) ||
		   bDirectory || m_nLogSize<(uint32)m_oFile.GetPosition();
}

void CLogArchiver::DoArchive(bool bDispErr)
{
	uint32 nOldArchId = m_nArchId;
	++m_nArchId;
	if(m_nArchId > m_nArchNo)
		m_nArchId = 1;
	char sLogDir[FOCP_MAX_PATH];
	CFormatString oCmd;
	CString::StringCopy(sLogDir, m_oLogDir.GetStr());
	CDiskFileSystem::GetInstance()->GetOsPathName(sLogDir);
#ifdef WINDOWS
	oCmd.Print("tar -czf %s\\%s.%03d.log.tar.gz %s\\%s.*.log 1>1.txt 2>2.txt", m_oArchDir.GetStr(), m_oLogName.GetStr(), m_nArchId, sLogDir, m_oLogName.GetStr());
#else
	oCmd.Print("tar -czf %s/%s.%03d.log.tar.gz %s/%s.*.log 1>1.txt 2>2.txt", m_oArchDir.GetStr(), m_oLogName.GetStr(), m_nArchId, sLogDir, m_oLogName.GetStr());
#endif
	int32 nRet = System(oCmd.GetStr());
	char sErrFile[FOCP_MAX_PATH];
	CString::StringCopy(sErrFile, "disk://");
	CDiskFileSystem::GetInstance()->GetFullPath("2.txt", sErrFile+7);
	CFile oFile(sErrFile, "r");
	bool bHaveError = false;
	CString oError;
	while(true)
	{
		int32 nLen = oFile.Read(sErrFile, FOCP_MAX_PATH);
		if(nLen <= 0)
			break;
		bHaveError = true;
		oError.Append(sErrFile, nLen);
	}
	oFile.Redirect(NULL);
	if(bHaveError)
	{
		m_nArchId = nOldArchId;
		if(bDispErr)
			FocpError(("%s", oError.GetStr()));
	}
	else if(nRet)
	{
		m_nArchId = nOldArchId;
		if(bDispErr)
			FocpError(("system('%s') failure", oCmd.GetStr()));
	}
	else
	{
		oCmd.Clear();
		oCmd.Print("rm -rf %s\\%s.*.log 1>1.txt 2>2.txt", sLogDir, m_oLogName.GetStr());
		System(oCmd.GetStr());
	}
	System("rm -rf 1.txt");
	System("rm -rf 2.txt");
}

void CLogArchiver::GetLogInfo(CLogMsg& oLog, CString &oLogInfo)
{
	CStringFormatter oFmt(&oLogInfo);
	if(m_nFilter & FOCP_LOG_HOST)
		oFmt.Print("%s ", oLog.oHost.GetStr());
	if(m_nFilter & FOCP_LOG_DATE)
	{
		CString oDate;
		CDateTime(oLog.nDate).GetString(oDate);
		oFmt.Print("%s ", oDate.GetStr());
	}
	oFmt.Print("[%s] ", GetLogLevelName(oLog.nLevel));
	if( (m_nFilter & FOCP_LOG_APPN) || (m_nFilter & FOCP_LOG_MODU) )
	{
		oFmt.Print("[");
		if(m_nFilter & FOCP_LOG_APPN)
		{
			oFmt.Print("%u:", oLog.nDMN);
			oFmt.Print("%s:", oLog.oAppName.GetStr());
			oFmt.Print("%u", oLog.nAIN);
			if(m_nFilter & FOCP_LOG_MODU)
				oFmt.Print("-");
		}
		if(m_nFilter & FOCP_LOG_MODU)
			oFmt.Print("%s", oLog.oModuleName.GetStr());
		oFmt.Print("] ");
	}
	if( (m_nFilter & FOCP_LOG_SRCF) || (m_nFilter & FOCP_LOG_FUNC) )
	{
		oFmt.Print("[");
		if(m_nFilter & FOCP_LOG_FUNC)
		{
			oFmt.Print("%s", oLog.oFuncName.GetStr());
			if(m_nFilter & FOCP_LOG_SRCF)
				oFmt.Print("@");
		}
		if(m_nFilter & FOCP_LOG_SRCF)
			oFmt.Print("%s:%u", oLog.oFile.GetStr(), oLog.nLine);
#ifdef WINDOWS
		oFmt.Print("] ->\r\n  ");
#else
		oFmt.Print("] ->\n  ");
#endif
	}
	else
		oFmt.Print("-> ");
	CString oInfo(oLog.oInfo);
	char* pShift = (char*)oInfo.GetStr();
	while(true)
	{
		char* pNewLine = CString::CharOfString(pShift, '\n');
		if(pNewLine)
		{
			pNewLine[0] = '\0';
			if(*(pNewLine-1) == '\r')
				*(pNewLine-1) = '\0';
		}
#ifdef WINDOWS
		oFmt.Print("%s\r\n", pShift);
#else
		oFmt.Print("%s\n", pShift);
#endif
		if(!pNewLine)
			break;
		pShift = pNewLine + 1;
		if(!pShift[0])
			break;
		oFmt.Print("  ");
	}
}

void CLogArchiver::Select(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	uint32 nLen = oFmt.GetReadCount();
	sCmdArg += nLen;
	uint32 nFilter = pItem->GetFilter(sCmdArg);
	if(nFilter == 0)
		pSession->Print("Invalid filter\r\n");
	else
		pItem->m_nFilter = nFilter;
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	uint32 nLen = oFmt.GetReadCount();
	if(!pItem->AddChooser(sCmdArg + nLen))
		pSession->Print("Push chooser failure\r\n");
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	pItem->PopChooser();
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	else
	{
		CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
		pItem->CLogChooser::Clear();
	}
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CString oChooser;
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	pItem->GetChooser(oChooser);
	CFormatString oFmt2;
	oFmt2.Print("The chooser is '%s', \r\nand the filter include:", oChooser.GetStr());
	if(pItem->m_nFilter & FOCP_LOG_HOST)
		oFmt2.Print("HOST ");
	if(pItem->m_nFilter & FOCP_LOG_DATE)
		oFmt2.Print("DATE ");
	if(pItem->m_nFilter & FOCP_LOG_APPN)
		oFmt2.Print("APP ");
	if(pItem->m_nFilter & FOCP_LOG_MODU)
		oFmt2.Print("MODULE ");
	if(pItem->m_nFilter & FOCP_LOG_SRCF)
		oFmt2.Print("FILE ");
	if(pItem->m_nFilter & FOCP_LOG_FUNC)
		oFmt2.Print("FUNC");
	pSession->Print("%s\r\n", oFmt2.GetStr());
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Add(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	//LogName, LogDir, MaxLogNo, LogSize, NeedArch, ArchDir, MaxArchNo
	CString oLogName, oLogDir, oArchDir;
	uint32 nMaxLogNo=FOCP_LOG_FILENO, nLogSize=FOCP_LOG_MAXFILE, nNeedArch=0, nMaxArchNo=FOCP_LOG_FILENO;

	while(sCmdArg[0])
	{
		uint32 nName, nValue;
		CString oValue;
		if(GetNameValue(sCmdArg, nName, oValue, nValue))
		{
			pSession->Print("Invalid command line arguments\r\n");
			return;
		}
		switch(nName)
		{
		case 0:
			oLogName = oValue;
			break;
		case 1:
			oLogDir = oValue;
			break;
		case 2:
			nMaxLogNo = nValue;
			break;
		case 3:
			nLogSize = nValue;
			break;
		case 4:
			nNeedArch = nValue;
			break;
		case 5:
			oArchDir = oValue;
			break;
		case 6:
			nMaxArchNo = nValue;
			break;
		}
		SkipWhiteSpace(sCmdArg);
		if(sCmdArg[0])
		{
			if(sCmdArg[0] == ',')
				++sCmdArg;
			else
			{
				pSession->Print("Invalid command line arguments\r\n");
				return;
			}
		}
	}
	if(oLogName.Empty())
	{
		pSession->Print("Missing logname\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	if(oLogDir.Empty())
	{
		CStringFormatter oFmt(&oLogDir);
		oFmt.Print("%s/log", CFilePathInfo::GetInstance()->GetHome());
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt != pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname is existed\r\n");
		return;
	}
	CLogArchiver* pItem = new CLogArchiver;
	if(!pItem->Initialize(oLogName.GetStr(), oLogDir.GetStr(), nMaxLogNo, nLogSize, (nNeedArch!=0), oArchDir.GetStr(), nMaxArchNo))
	{
		pTable->m_oMutex.Leave();
		pSession->Print("Initialize CLogArchiver failure\r\n");
		return;
	}
	pTable->m_oArchiverTable[oLogName] = pItem;
	if(nNeedArch)
		pItem->DoArchive(false);
	pTable->m_oMutex.Leave();
}

void CLogArchiver::Del(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	delete pItem;
	pTable->m_oMutex.Leave();
}

void CLogArchiver::List(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CFormatString oFmt;
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	CRbTreeNode* pEnd = pTable->m_oArchiverTable.End();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.First();
	for(; pIt!=pEnd; pIt=pTable->m_oArchiverTable.GetNext(pIt))
	{
		const CString &oLogName = pTable->m_oArchiverTable.GetKey(pIt);
		CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
		oFmt.Print("  %s:%s\r\n", oLogName.GetStr(), pItem->Registered()?"normal":"suspend");
	}
	pTable->m_oMutex.Leave();
	if(oFmt.Empty())
		pSession->Print("There isn't archiver.\r\n");
	else
		pSession->Print("%s", oFmt.GetStr());
}

void CLogArchiver::Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	pItem->Register();
	pTable->m_oMutex.Leave();
}

void CLogArchiver::End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oLogName;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oLogName);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	if(!oLogName.IsIdentifierOfC())
	{
		pSession->Print("Invalid logname\r\n");
		return;
	}
	CArchiverTable* pTable = CArchiverTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oArchiverTable.Find(oLogName);
	if(pIt == pTable->m_oArchiverTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The logname isn't existed\r\n");
		return;
	}
	CLogArchiver* pItem = pTable->m_oArchiverTable.GetItem(pIt);
	pItem->DeRegister();
	pTable->m_oMutex.Leave();
}

uint32 CLogArchiver::GetNameValue(const char* &pStr, uint32 &nName, CString &oValue, uint32 &nValue)
{
	bool bInt;
	uint32 nRet;
	CString oName;
	nRet = GetIdentifier(pStr, oName);
	if(!nRet)
		nRet = GetNameType(oName, nName, bInt);
	if(!nRet)
	{
		SkipWhiteSpace(pStr);
		if(pStr[0] != '=')
			nRet = 1;
		else
			++pStr;
	}
	if(!nRet)
	{
		if(bInt)
		{
			nRet = GetInt(pStr, oValue);
			if(!nRet)
				nValue = CString::Atoi(oValue.GetStr());
		}
		else
			nRet = GetString(pStr, oValue);
	}
	return nRet;
}

uint32 CLogArchiver::GetNameType(const CString &oName, uint32 &nName, bool &bInt)
{
	if(!oName.Compare("LogName", false))
	{
		nName = 0;
		bInt = false;
	}
	else if(!oName.Compare("LogDir", false))
	{
		nName = 1;
		bInt = false;
	}
	else if(!oName.Compare("MaxLogNo", false))
	{
		nName = 2;
		bInt = true;
	}
	else if(!oName.Compare("LogSize", false))
	{
		nName = 3;
		bInt = true;
	}
	else if(!oName.Compare("NeedArch", false))
	{
		nName = 4;
		bInt = true;
	}
	else if(!oName.Compare("ArchDir", false))
	{
		nName = 5;
		bInt = false;
	}
	else if(!oName.Compare("MaxArchNo", false))
	{
		nName = 6;
		bInt = true;
	}
	else return 1;
	return 0;
}

uint32 CLogArchiver::GetInt(const char* &pStr, CString &oValue)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] =='+' || pStr[0] == '-')
	{
		++pStr;
		oValue = pStr[0];
	}
	while(pStr[0] >= '0' && pStr[0] <= '9')
	{
		oValue += pStr[0];
		++pStr;
	}
	if(oValue.GetSize())
		return 0;
	return 1;
}

uint32 CLogArchiver::GetString(const char* &pStr, CString &oValue)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] =='\'')
	{
		++pStr;
		while(pStr[0])
		{
			if(pStr[0] == '\'' && pStr[1] == '\'')
			{
				pStr += 2;
				oValue += '\'';
			}
			else if(pStr[0] == '\'' || pStr[0] == '\n' || pStr[0] == '\r')
				break;
			oValue += pStr[0];
			++pStr;
		}
		if(pStr[0] == '\'')
		{
			++pStr;
			return 0;
		}
	}
	return 1;
}

uint32 CLogArchiver::GetIdentifier(const char* &pStr, CString &oIdentifier)
{
	SkipWhiteSpace(pStr);
	if( (pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z') || pStr[0] == '_')
	{
		oIdentifier += pStr[0];
		++pStr;
		while( (pStr[0] >= 'a' && pStr[0] <= 'z') ||
				(pStr[0] >= 'A' && pStr[0] <= 'Z') ||
				pStr[0] == '_' ||
				(pStr[0] >= '0' && pStr[0] <= '9') )
		{
			oIdentifier += pStr[0];
			++pStr;
		}
		return 0;
	}
	return 1;
}

void CLogArchiver::SkipWhiteSpace(const char* &pStr)
{
	while(pStr[0] == ' ' || pStr[0] == '\t')
		++pStr;
}

FOCP_END();
