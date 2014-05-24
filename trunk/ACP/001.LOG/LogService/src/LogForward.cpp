
#include "LogForward.hpp"

FOCP_BEGIN();

CForwarderTable::CForwarderTable()
{
}

CForwarderTable::~CForwarderTable()
{
	Clear();
}

void CForwarderTable::Clear()
{
	m_oMutex.Enter();
	while(m_oFwdTable.GetSize())
		delete m_oFwdTable.GetItem(m_oFwdTable.First());
	m_oMutex.Leave();
}

void CForwarderTable::StartAll()
{
	CRbTreeNode* pEnd = m_oFwdTable.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oFwdTable.First();
	for(; pIt!=pEnd; pIt=m_oFwdTable.GetNext(pIt))
	{
		CLogForwarder* pItem = m_oFwdTable.GetItem(pIt);
		pItem->Register();
	}
	m_oMutex.Leave();
}

CForwarderTable* CForwarderTable::GetInstance()
{
	return CSingleInstance<CForwarderTable>::GetInstance();
}

CLogForwarder::CLogForwarder()
{
}

CLogForwarder::~CLogForwarder()
{
	if(!m_oName.Empty())
	{
		CForwarderTable* pTable = CForwarderTable::GetInstance();
		pTable->m_oMutex.Enter();
		pTable->m_oFwdTable.Remove(m_oName);
		pTable->m_oMutex.Leave();
	}
}

bool CLogForwarder::Initialize(const char* sServerAddr, uint16 nPort)
{
	m_oServerAddr.nAddr = CFile::GetIpAddr(sServerAddr);
	if((m_oServerAddr.nAddr==0) || (m_oServerAddr.nAddr==0xFFFFFFFF))
		return false;
	if(!nPort)
		nPort = 2269;
	m_oServerAddr.nPort = 0;
	CFileName oUdpName;
	oUdpName.oProtocol = "udp";
	if(m_oFile.Open(oUdpName, "rw"))
		return false;
	CFile::GetIpFileName(m_oServerAddr, m_oName);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(m_oName);
	if(pIt != pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		m_oFile.Redirect(NULL);
		return false;
	}
	pTable->m_oFwdTable[m_oName] = this;
	pTable->m_oMutex.Leave();
	m_oServerAddr.nPort = nPort;
	return true;
}

uint32 CLogForwarder::CreateLogMsg(CLogMsg& oLog, char sLogInfo[FOCP_LOG_MAXMSG])
{
	CString oDate;
	uint32 nCount = 0;
	CDateTime(oLog.nDate).GetString(oDate);
	CFormatBinary oFmt((uint8*)sLogInfo, FOCP_LOG_MAXMSG);

	//sHost
	oFmt.Print("%s%c", oLog.oHost.GetStr(), (char)'\0');
	nCount += oFmt.GetWriteCount();

	//sDate
	oFmt.Print("%s%c", oDate.GetStr(), (char)'\0');
	nCount += oFmt.GetWriteCount();

	//nLevel;
	oFmt.Print("%u%c", oLog.nLevel, (char)'\0');
	nCount += oFmt.GetWriteCount();

	//sAppName
	oFmt.Print("%s%c", oLog.oAppName.GetStr(), '\0');
	nCount += oFmt.GetWriteCount();

	//nDMN
	oFmt.Print("%u%c", oLog.nDMN, '\0');
	nCount += oFmt.GetWriteCount();

	//nAIN
	oFmt.Print("%u%c", oLog.nAIN, '\0');
	nCount += oFmt.GetWriteCount();

	//sModuleName
	oFmt.Print("%s%c", oLog.oModuleName.GetStr(), '\0');
	nCount += oFmt.GetWriteCount();

	//sFunction
	oFmt.Print("%s%c", oLog.oFuncName.GetStr(), '\0');
	nCount += oFmt.GetWriteCount();

	//sFileName
	oFmt.Print("%s%c", oLog.oFile.GetStr(), '\0');
	nCount += oFmt.GetWriteCount();

	//nLine
	oFmt.Print("%u%c", oLog.nLine, '\0');
	nCount += oFmt.GetWriteCount();

	//sLogInfo
	oFmt.Print("%s%c", oLog.oInfo.GetStr(), '\0');
	nCount += oFmt.GetWriteCount();

	return nCount;
}

void CLogForwarder::Process(CLogMsg& oLog)
{
	char sLogInfo[FOCP_LOG_MAXMSG];
	uint32 nLen = CreateLogMsg(oLog, sLogInfo);
	m_oFile.WriteTo(sLogInfo, nLen, m_oServerAddr);
}

void CLogForwarder::RegisterCmd()
{
	CCmdSystem* pCmdSystem = CCmdSystem::GetInstance();

	pCmdSystem->RegisterCmd("/Log/Forward/Push", "Push server chooser<CR>\r\n"
							"\tAdd a choice rule into the forwarder.\r\n"
							"\tThe chooser is same as SQL where clause statement, the field include:\r\n"
							"\tHost,Date,App,Module,Func,File,Key,Level,DMN,AIN,Line.\r\n"
							"\tThe operator incaludes: '=' and '!=' .\r\n"
							"\tThe last 3 fields are integer type,\r\n"
							"\tThe 'Date' field is datetime type: [a,b];[a,b);(a,b];(a,b). 'a' and 'b' is string format\r\n"
							"\tThe other fields are string type:\r\n"
							"\tThe string is a sequences of characters enclosed in single quotation marks,\r\n"
							"\tand two consecutive single quotes is expressed as a single quote\r\n", Push);
	pCmdSystem->RegisterCmd("/Log/Forward/Pop", "Pop server<CR>\r\n"
							"\tRemove the last choice rule from the forwarder.\r\n", Pop);
	pCmdSystem->RegisterCmd("/Log/Forward/Clear", "Clear server<CR>\r\n"
							"\tClear all choice rules in the forwarder\r\n", Clear);
	pCmdSystem->RegisterCmd("/Log/Forward/Query", "Query server<CR>\r\n"
							"\tQuery the forwarder's chooser\r\n", Query);
	pCmdSystem->RegisterCmd("/Log/Forward/Add", "Add server port<CR>\r\n"
							"\tAdd a new forward server\r\n", Add);
	pCmdSystem->RegisterCmd("/Log/Forward/Del", "Del server<CR>\r\n"
							"\tRemove the forward server\r\n", Del);
	pCmdSystem->RegisterCmd("/Log/Forward/List", "List <CR>\r\n"
							"\tList all forward servers\r\n", List);
	pCmdSystem->RegisterCmd("/Log/Forward/Begin", "Begin server<CR>\r\n"
							"\tStartup the forward server\r\n", Begin);
	pCmdSystem->RegisterCmd("/Log/Forward/End", "End server<CR>\r\n"
							"\tSuspend the forward server\r\n", End);
}

void CLogForwarder::Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		uint32 nLen = oFmt.GetReadCount();
		if(!pItem->AddChooser(sCmdArg+nLen))
			pSession->Print("Push chooser into '%s' failure\r\n", oServerAddr.GetStr());
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		pItem->PopChooser();
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		pItem->CLogChooser::Clear();
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CString oChooser;
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		pItem->GetChooser(oChooser);
		pSession->Print("The chooser is '%s' \r\n", oChooser.GetStr());
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::Add(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint16 nPort = 0;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s%u16", &oServerAddr, &nPort);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	CLogForwarder* pFwd = new CLogForwarder;
	if(!pFwd->Initialize(oServerAddr.GetStr(), nPort))
	{
		pSession->Print("Create forwarder failure\r\n");
		return;
	}
}

void CLogForwarder::Del(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		delete pItem;
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::List(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CFormatString oFmt;
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	CRbTreeNode* pEnd = pTable->m_oFwdTable.End();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.First();
	for(; pIt!=pEnd; pIt=pTable->m_oFwdTable.GetNext(pIt))
	{
		const CString &oServer = pTable->m_oFwdTable.GetKey(pIt);
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		oFmt.Print("  %s:%s\r\n", oServer.GetStr(), pItem->Registered()?"normal":"suspend");
	}
	pTable->m_oMutex.Leave();
	if(oFmt.Empty())
		pSession->Print("There isn't forwarder.\r\n");
	else
		pSession->Print("%s", oFmt.GetStr());
}

void CLogForwarder::Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		pItem->Register();
	}
	pTable->m_oMutex.Leave();
}

void CLogForwarder::End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CIpAddr oIpAddr;
	CString oServerAddr;
	CFormatString oFmt(sCmdArg);
	int32 nRet = oFmt.Scan("%s", &oServerAddr);
	if(nRet < 1)
	{
		pSession->Print("Missing the server address argument\r\n");
		return;
	}
	oIpAddr.nAddr = CFile::GetIpAddr(oServerAddr.GetStr());
	oIpAddr.nPort = 0;
	CFile::GetIpFileName(oIpAddr, oServerAddr);
	CForwarderTable* pTable = CForwarderTable::GetInstance();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oFwdTable.Find(oServerAddr);
	if(pIt == pTable->m_oFwdTable.End())
	{
		pTable->m_oMutex.Leave();
		pSession->Print("The server address isn't existed\r\n");
		return;
	}
	else
	{
		CLogForwarder* pItem = pTable->m_oFwdTable.GetItem(pIt);
		pItem->DeRegister();
	}
	pTable->m_oMutex.Leave();
}

FOCP_END();
