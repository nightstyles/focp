
#include "LogMonitor.hpp"

FOCP_BEGIN();

CMonitorTable::CMonitorTable()
{
}

CMonitorTable::~CMonitorTable()
{
	Clear();
}

void CMonitorTable::Clear()
{
	m_oMutex.Enter();
	while(m_oSessionTable.GetSize())
		delete m_oSessionTable.GetItem(m_oSessionTable.First());
	m_oMutex.Leave();
}

CMonitorTable* CMonitorTable::GetInstance()
{
	return CSingleInstance<CMonitorTable>::GetInstance();
}

CLogMonitor::CLogMonitor(CCmdSession* pCmdSession)
{
	//主机1、日期2、应用4、模块8、文件16、函数32
	m_nFilter = 15;//主机1、日期2、应用4、模块8
	m_pCmdSession = pCmdSession;
	CMonitorTable* pTable = CMonitorTable::GetInstance();
	pTable->m_oMutex.Enter();
	pTable->m_oSessionTable[m_pCmdSession] = this;
	pTable->m_oMutex.Leave();
}

CLogMonitor::~CLogMonitor()
{
	CMonitorTable* pTable = CMonitorTable::GetInstance();
	pTable->m_oMutex.Enter();
	pTable->m_oSessionTable.Remove(m_pCmdSession);
	pTable->m_oMutex.Leave();
}

void CLogMonitor::Process(CLogMsg& oLog)
{
	CString oLogInfo;
	GetLogInfo(oLog, oLogInfo);
	m_pCmdSession->Print("%s", oLogInfo.GetStr());
}

void CLogMonitor::RegisterCmd()
{
	CCmdSystem* pCmdSystem = CCmdSystem::GetInstance();
	pCmdSystem->RegisterCmd("/Log/Monitor/Select", "Select fields<CR>\r\n"
							"\tSelect some fields to display, these field include:\r\n"
							"\tHost,Date,App,Module,File,Func. The '*' is expressed all fields\r\n", Select);
	pCmdSystem->RegisterCmd("/Log/Monitor/Push", "Push chooser<CR>\r\n"
							"\tAdd a choice rule into the monitor.\r\n"
							"\tThe chooser is same as SQL where clause statement, the field include:\r\n"
							"\tHost,Date,App,Module,Func,File,Key,Level,DMN,AIN,Line.\r\n"
							"\tThe operator incaludes: '=' and '!=' .\r\n"
							"\tThe last 3 fields are integer type,\r\n"
							"\tThe 'Date' field is datetime type: [a,b];[a,b);(a,b];(a,b). 'a' and 'b' is string format\r\n"
							"\tThe other fields are string type:\r\n"
							"\tThe string is a sequences of characters enclosed in single quotation marks,\r\n"
							"\tand two consecutive single quotes is expressed as a single quote\r\n", Push);
	pCmdSystem->RegisterCmd("/Log/Monitor/Pop", "Pop <CR>\r\n"
							"\tRemove the last choice rule from the monitor.\r\n", Pop);
	pCmdSystem->RegisterCmd("/Log/Monitor/Clear", "Clear <CR>\r\n"
							"\tClear all choice rules in the monitor\r\n", Clear);
	pCmdSystem->RegisterCmd("/Log/Monitor/Query", "Query <CR>\r\n"
							"\tQuery the monitor's chooser\r\n", Query);
	pCmdSystem->RegisterCmd("/Log/Monitor/Begin", "Begin <CR>\r\n"
							"\tBegin to monitor the system log information\r\n", Begin);
	pCmdSystem->RegisterCmd("/Log/Monitor/End", "End <CR>\r\n"
							"\tEnd to monitor the system log information\r\n", End);
}

CLogMonitor* CLogMonitor::GetMonitor(CCmdSession* pSession, bool bCreate)
{
	CLogMonitor* pMonitor = NULL;
	CMonitorTable* pTable = CMonitorTable::GetInstance();
	CRbTreeNode* pEnd = pTable->m_oSessionTable.End();
	pTable->m_oMutex.Enter();
	CRbTreeNode* pIt = pTable->m_oSessionTable.Find(pSession);
	if(pIt != pEnd)
		pMonitor = pTable->m_oSessionTable.GetItem(pIt);
	else if(bCreate)
		pMonitor = new CLogMonitor(pSession);
	pTable->m_oMutex.Leave();
	return pMonitor;
}

void CLogMonitor::Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession);
	if(!pMonitor->AddChooser(sCmdArg))
		pSession->Print("Push chooser failure\r\n");
}

void CLogMonitor::Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession);
	pMonitor->PopChooser();
}

void CLogMonitor::Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession);
	pMonitor->CLogChooser::Clear();
}

void CLogMonitor::Select(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession);
	uint32 nFilter = pMonitor->GetFilter(sCmdArg);
	if(nFilter == 0)
		pSession->Print("Invalid filter\r\n");
	else
		pMonitor->m_nFilter = nFilter;
}

void CLogMonitor::Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oChooser;
	CLogMonitor* pMonitor = GetMonitor(pSession);
	pMonitor->GetChooser(oChooser);
	CFormatString oFmt;
	oFmt.Print("The chooser is '%s', \r\nand the filter include:", oChooser.GetStr());
	if(pMonitor->m_nFilter & FOCP_LOG_HOST)
		oFmt.Print("HOST ");
	if(pMonitor->m_nFilter & FOCP_LOG_DATE)
		oFmt.Print("DATE ");
	if(pMonitor->m_nFilter & FOCP_LOG_APPN)
		oFmt.Print("APP ");
	if(pMonitor->m_nFilter & FOCP_LOG_MODU)
		oFmt.Print("MODULE ");
	if(pMonitor->m_nFilter & FOCP_LOG_SRCF)
		oFmt.Print("FILE ");
	if(pMonitor->m_nFilter & FOCP_LOG_FUNC)
		oFmt.Print("FUNC");
	pSession->Print("%s\r\n", oFmt.GetStr());
}

void CLogMonitor::Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession);
	pSession->Print("Begin to monitor the system log info\r\n");
	pMonitor->Register();
}

void CLogMonitor::End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CLogMonitor* pMonitor = GetMonitor(pSession, false);
	pSession->Print("End to monitor the system log info\r\n");
	if(pMonitor)
	{
		pMonitor->DeRegister();
		delete pMonitor;
	}
}

void CLogMonitor::GetLogInfo(CLogMsg& oLog, CString &oLogInfo)
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
		oFmt.Print("] ->\r\n  ");
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
		oFmt.Print("%s\r\n", pShift);
		if(!pNewLine)
			break;
		pShift = pNewLine + 1;
		if(!pShift[0])
			break;
		oFmt.Print("  ");
	}
}

FOCP_END();
