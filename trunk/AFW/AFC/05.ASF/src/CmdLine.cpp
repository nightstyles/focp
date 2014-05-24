
#include "CmdLine.hpp"
#include "Application.hpp"

FOCP_BEGIN();

CHistoryCmd::CHistoryCmd()
{
	for(uint32 i=0; i<FOCP_CLI_MAXHISTORY; ++i)
		m_history[i] = NULL;
}

CHistoryCmd::~CHistoryCmd()
{
	Clear();
}

void CHistoryCmd::Clear()
{
	for(uint32 i=0; i<FOCP_CLI_MAXHISTORY; ++i)
	{
		if(m_history[i])
		{
			delete[] m_history[i];
			m_history[i] = NULL;
		}
	}
}

char* CHistoryCmd::HistoryCmd(uint32 i)
{
	if(i >= FOCP_CLI_MAXHISTORY)
		return NULL;
	return m_history[i];
}

void CHistoryCmd::AddHistory(char* cmd, int32 l)
{
	uint32 i, nSize;
	int32 nIdx = -1;
	for (i = 0; i < FOCP_CLI_MAXHISTORY; i++)
	{
		if(m_history[i])
		{
			if(CString::StringCompare(m_history[i], cmd) == 0)
				return;
		}
		else if(nIdx < 0)
			nIdx = i;
	}
	CString oCmd(cmd, l);
	if(nIdx >= 0)
		m_history[nIdx] = oCmd.Detach(nSize);
	else
	{
		delete[] m_history[0];
		for (i = 0; i < 255; i++)
			m_history[i] = m_history[i+1];
		m_history[255] = oCmd.Detach(nSize);
	}
}

void CHistoryCmd::List(CCmdSession* pSession)
{
	for(uint32 i=0; i<FOCP_CLI_MAXHISTORY; ++i)
	{
		if(m_history[i])
			pSession->Print("%3u. %s\r\n", i+1, m_history[i]);
	}
}

CCmdSession::CCmdSession()
{
	m_pFile = NULL;
	m_bQuit = false;
	m_bHide = false;
	m_bForbidSave = false;
	m_pHistoryCmd = NULL;
	m_nMax = 0;
}

CCmdSession::~CCmdSession()
{
}

void CCmdSession::SetHidden(bool bHide)
{
	m_bHide = bHide;
}

void CCmdSession::SetMaxRead(uint32 nMax)
{
	m_nMax = nMax;
}

void CCmdSession::ForbidSave(bool bForbidSave)
{
	m_bForbidSave = bForbidSave;
}

void CCmdSession::SetHistoryCmd(CHistoryCmd* pHistoryCmd)
{
	m_pHistoryCmd = pHistoryCmd;
}

CHistoryCmd* CCmdSession::GetHistoryCmd()
{
	return m_pHistoryCmd;
}

char* CCmdSession::HistoryCmd(uint32 i)
{
	if(!m_pHistoryCmd)
		return NULL;
	return m_pHistoryCmd->HistoryCmd(i);
}

void CCmdSession::AddHistory(char* cmd, int32 l)
{
	if(m_pHistoryCmd)
		m_pHistoryCmd->AddHistory(cmd, l);
}

void CCmdSession::Print(const char *sFormat, ...)
{
}

void CCmdSession::PrintV(const char* sFormat, CVaList& pArgList)
{
}

void CCmdSession::SetQuit()
{
	m_bQuit = true;
}

void CCmdSession::ClearScreen()
{
}

const char* CCmdSession::ReadLine(bool bPrompt, char cPrompt)
{
	while(true)
	{
		if(m_pFile == NULL)
			return NULL;
		if(m_pFile->Scan("%r", &m_oLine) == 1)
		{
			m_oLine += " ";
			return m_oLine.GetStr();
		}
		delete m_pFile;
		m_pFile = NULL;
		m_oFileList.Pop(m_pFile);
	}
}

void CCmdSession::ProcessText(const char* sScript)
{
	CFormatString oFileName;
	oFileName.Print("memory://|%p:%d", sScript, CString::StringLength(sScript));
	ProcessFile(oFileName.GetStr());
}

void CCmdSession::ProcessScript(const char* sFileName)
{
	CString oFileName("disk://");
	oFileName += sFileName;
	ProcessFile(oFileName.GetStr());
}

void CCmdSession::ProcessFile(const char* sFileName)
{
	CCmdSession* pSession = GetSession();
	if(pSession == NULL)
		SetSession(this);
	CFormatFile* pFile = new CFormatFile;
	if(pFile->Open(sFileName, "r"))
	{
		FocpCmdLog(FOCP_LOG_ERROR, ("Open file '%s' failure.", sFileName));
		if(pSession == NULL)
			SetSession(NULL);
		delete pFile;
		return;
	}
	if(m_pFile)
		m_oFileList.Push(m_pFile);
	m_pFile = pFile;
	const char* sLine;
	CCmdSystem* pCmdSys = CCmdSystem::GetInstance();
	while(true)
	{
		sLine = CCmdSession::ReadLine();
		if(!sLine)
			break;
		sLine = CString::SkipSpace(sLine);
		if(sLine[0])
		{
			CString oLine(sLine);
			sLine = oLine.GetStr();
			pCmdSys->ProcessCmd(this, sLine);
		}
	}
	if(pSession == NULL)
		SetSession(NULL);
}

static CPrivateVariable<CCmdSession*> g_oSessionList;

CCmdSession* CCmdSession::GetSession()
{
	CCmdSession *&pSession = g_oSessionList;
	return pSession;
}

void CCmdSession::SetSession(CCmdSession* pSession)
{
	CCmdSession* &pSession2 = g_oSessionList;
	pSession2 = pSession;
}

FOCP_DETAIL_BEGIN();

CCmdNode::~CCmdNode()
{
	if(m_bBranch)
	{
		CRbTreeNode* pIt = m_oNode.pBranch->First();
		CRbTreeNode* pEnd = m_oNode.pBranch->End();
		for(; pIt != pEnd; pIt = m_oNode.pBranch->GetNext(pIt))
		{
			CCmdNode* pNode = m_oNode.pBranch->GetItem(pIt);
			delete pNode;
		}
		delete m_oNode.pBranch;
	}
	else
		delete m_oNode.pLeaf;
}

CCmdNode::CCmdNode()
{
	m_bBranch = true;
	m_oNode.pBranch = new CRbMap<CString, CCmdNode*, CNameCompare>;
	m_pParent = NULL;
	m_sName= NULL;
}

CCmdNode::CCmdNode(const char* sDescription, CCmdFunc fCallBack)
{
	m_bBranch = false;
	m_oNode.pLeaf = new CLeafNode;
	m_oNode.pLeaf->oDesc = sDescription;
	m_oNode.pLeaf->fCallBack = fCallBack;
	m_pParent = NULL;
	m_sName= NULL;
}

CCmdNode* CCmdNode::FindNode(const char* sName)
{
	if(!m_bBranch)
		return NULL;
	CRbTreeNode* pNode = m_oNode.pBranch->Find(sName);
	if(pNode != m_oNode.pBranch->End())
		return m_oNode.pBranch->GetItem(pNode);
	return NULL;
}

bool CCmdNode::RegisterNode(const char* sName, CCmdNode* pNode)
{
	if(!m_bBranch)
		return false;
	CRbTreeNode* pIt = m_oNode.pBranch->Find(sName);
	if(pIt != m_oNode.pBranch->End())
		return false;
	(*m_oNode.pBranch)[sName] = pNode;
	pNode->m_pParent = this;
	pNode->m_sName = m_oNode.pBranch->GetKey(m_oNode.pBranch->Find(sName)).GetStr();
	return true;
}

FOCP_DETAIL_END();

CCmdSystem::CCmdSystem()
{
	m_pCurrent = &m_oRoot;
	m_pPrev = m_pCurrent;

	RegisterSysCmd("ls", "ls [path]<CR>\r\n\tlist the all commands and sub-directories in the special path or the current path\r\n", List);
	RegisterSysCmd("pwd", "pwd<CR>\r\n\tshow the current path\r\n", Pwd);
	RegisterSysCmd("man", "man cmd<CR>\r\n\tshow the special command's usage\r\n", Help);
	RegisterSysCmd("cd", "cd path<CR>\r\n\tswitch to the special path\r\n", Cd);
	RegisterSysCmd("quit", "quit<CR>\r\n\tquit from telnet service\r\n", Quit);
	RegisterSysCmd("clear", "clear<CR>\r\n\tclear the screen\r\n", Clear);
	RegisterSysCmd("stop", "stop<CR>\r\n\tstop the application\r\n", Stop);
	RegisterSysCmd("call", "call script<CR>\r\n\tbatch process the script file\r\n", Call);

	RegisterCmd("/Log/SetLogMode", "SetLogMode Mode [ModuleName]<CR>\r\n"
				"\tIf there isn't 'ModuleName', the system will set the global log mode\r\n"
				"\tMode:1~7=>1:screen,2:file,4:server\r\n", SetLogModeCmd);
	RegisterCmd("/Log/GetLogMode", "GetLogMode [ModuleName]<CR>\r\n"
				"\tIf there isn't 'ModuleName', the system will get the global log mode\r\n"
				"\tMode:1~7=>1:screen,2:file,4:server\r\n", GetLogModeCmd);
	RegisterCmd("/Log/SetLogLevel", "SetLogLevel Level [ModuleName]<CR>\r\n"
				"\tIf there isn't 'ModuleName', the system will set the global log level\r\n"
				"\tLevel:ERROR,WARNING,DEBUG\r\n", SetLogLevelCmd);
	RegisterCmd("/Log/GetLogLevel", "GetLogLevel [ModuleName]<CR>\r\n\tIf there isn't 'ModuleName', the system will get the global log level\r\n", GetLogLevelCmd);
	RegisterCmd("/Log/SetLogFilter", "SetLogFilter Filter [ModuleName]<CR>\r\n"
				"\tIf there isn't 'ModuleName', the system will set the global log filter\r\n"
				"\tFilter:1~63=>1:host,2=data,4=appname,8=modulename,16=filename,32=function\r\n", SetLogFilterCmd);
	RegisterCmd("/Log/GetLogFilter", "GetLogFilter [ModuleName]<CR>\r\n"
				"\tIf there isn't 'ModuleName', the system will get the global log filter\r\n"
				"\tFilter:1~63=>1:host,2=data,4=appname,8=modulename,16=filename,32=function\r\n", GetLogFilterCmd);
	RegisterCmd("/Log/ListLogModule", "ListLogModule <CR>, list all modules registered in the log system\r\n", ListLogModuleCmd);
}

CCmdSystem::~CCmdSystem()
{
}

CCmdSystem* CCmdSystem::GetInstance()
{
	return CSingleInstance<CCmdSystem>::GetInstance();
}

void CCmdSystem::RegisterCmd(const char* sName, const char* sDescription, CCmdFunc fCallBack)
{
	FOCP_DETAIL_NAME::CCmdNode* pNode;
	CVector<CString> oNames;

	if(!sName)
		FocpThrow(CmdSystem::RegisterCmd);

	if(!sDescription)
		FocpThrow(CmdSystem::RegisterCmd);

	if(!fCallBack)
		FocpThrow(CmdSystem::RegisterCmd);

	if(!ParseCmdTree(sName, oNames))
		FocpThrow(CmdSystem::RegisterCmd);

	m_oMutex.Enter();
	pNode = &m_oRoot;
	uint32 i, nSize = oNames.GetSize();
	for(i=0; i<nSize; ++i)
	{
		FOCP_DETAIL_NAME::CCmdNode* pChild = pNode->FindNode(oNames[i].GetStr());
		if(pChild == NULL)
			break;
		pNode = pChild;
	}
	if(i == nSize)
	{
		m_oMutex.Leave();
		FocpThrow(CmdSystem::RegisterCmd);
	}
	FOCP_DETAIL_NAME::CCmdNode *pNewNode;
	for(; i<nSize-1; ++i)
	{
		pNewNode = new FOCP_DETAIL_NAME::CCmdNode;
		if(!pNode->RegisterNode(oNames[i].GetStr(), pNewNode))
		{
			m_oMutex.Leave();
			delete pNewNode;
			FocpThrow(CmdSystem::RegisterCmd);
		}
		pNode = pNewNode;
	}
	pNewNode = new FOCP_DETAIL_NAME::CCmdNode(sDescription, fCallBack);
	if(!pNode->RegisterNode(oNames[i].GetStr(), pNewNode))
	{
		m_oMutex.Leave();
		delete pNewNode;
		FocpThrow(CmdSystem::RegisterCmd);
	}
	m_oMutex.Leave();
}

void CCmdSystem::RegisterSysCmd(const char* sName, const char* sDescription, CSysCmdFunc fCallBack)
{
	if(!sName)
		FocpThrow(CmdSystem::RegisterSysCmd);

	if(!sDescription)
		FocpThrow(CmdSystem::RegisterSysCmd);

	if(!fCallBack)
		FocpThrow(CmdSystem::RegisterSysCmd);

	CString oName(sName);
	if(!oName.IsIdentifierOfC())
		FocpThrow(CmdSystem::RegisterSysCmd);

	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oSysCmdList.Find(oName);
	if(pIt != m_oSysCmdList.End())
	{
		m_oMutex.Leave();
		FocpThrow(CmdSystem::RegisterSysCmd);
	}
	CSysCmdNode& oCmdNode = m_oSysCmdList[oName];
	oCmdNode.oDesc = sDescription;
	oCmdNode.fCallBack = fCallBack;
	m_oMutex.Leave();
}

void CCmdSystem::ProcessCmd(CCmdSession* pSession, const char *sCmdLine)
{
	const char* sCmdArg = sCmdLine;
	m_oMutex.Enter();
	FOCP_DETAIL_NAME::CCmdNode* pNode = GetCmdNode(sCmdArg);
	if(pNode == NULL)
	{
		CString oName;
		sCmdArg = sCmdLine;
		GetIdentifier(sCmdArg, oName);
		CRbTreeNode* pIt = m_oSysCmdList.Find(oName);
		if(pIt == m_oSysCmdList.End())
			pSession->Print("find the command failure\r\n");
		else
		{
			CSysCmdNode& oCmdNode = m_oSysCmdList.GetItem(pIt);
			oCmdNode.fCallBack(this, pSession, sCmdLine, sCmdArg);
		}
		m_oMutex.Leave();
		return;
	}
	if(pNode->m_bBranch)
	{
		pSession->Print("this is a directory\r\n");
		m_oMutex.Leave();
		return;
	}
	CCmdFunc CallBack = pNode->m_oNode.pLeaf->fCallBack;
	CallBack(pSession, sCmdLine, sCmdArg);
	m_oMutex.Leave();
}

bool CCmdSystem::ParseCmdTree(const char* sName, CVector<CString> &oNames)
{
	while(sName[0])
	{
		if(sName[0] != '/')
			return false;
		while(sName[0] == '/')
			++sName;
		char* pNext = CString::CharOfString(sName, '/');
		uint32 n = pNext?(pNext-sName):(CString::StringLength(sName));
		CString oName(sName, n);
		if(!oName.IsIdentifierOfC())
			return false;
		oNames.Insert(oNames.GetSize(), oName);
		if(pNext)
			sName = pNext;
		else
			break;
	}
	if(oNames.GetSize() == 0)
		return false;
	return true;
}

FOCP_DETAIL_NAME::CCmdNode* CCmdSystem::GetCmdNode(const char *&sCmdLine)
{
	FOCP_DETAIL_NAME::CCmdNode* pCurrent = m_pCurrent;

	sCmdLine = CString::SkipSpace(sCmdLine);

	if(sCmdLine[0] == '/')
	{
		pCurrent = &m_oRoot;
		while(sCmdLine[0] == '/')
			++sCmdLine;
	}

	CString oName;
	bool bFirst = true;
	while(sCmdLine[0])
	{
		if(bFirst)
		{
			if( (sCmdLine[0] >= 'A' && sCmdLine[0] <= 'Z') ||
					(sCmdLine[0] >= 'a' && sCmdLine[0] <= 'z') ||
					(sCmdLine[0] == '_') )
			{
				bFirst = false;
				oName += sCmdLine[0];
				++sCmdLine;
				continue;
			}
		}

		if( (sCmdLine[0] >= 'A' && sCmdLine[0] <= 'Z') ||
				(sCmdLine[0] >= 'a' && sCmdLine[0] <= 'z') ||
				(sCmdLine[0] >= '0' && sCmdLine[0] <= '9') ||
				(sCmdLine[0] == '_') )
		{
			oName += sCmdLine[0];
			++sCmdLine;
			continue;
		}

		if(sCmdLine[0] == '.')
		{
			if(sCmdLine[1] == '.' && sCmdLine[2] == '/')
			{
				FOCP_DETAIL_NAME::CCmdNode* pParent = pCurrent->m_pParent;
				if(pParent)
					pCurrent = pParent;
				sCmdLine += 2;
				while(sCmdLine[0] == '/')
					++sCmdLine;
				continue;
			}
			else if(sCmdLine[1] == '/')
			{
				sCmdLine += 1;
				while(sCmdLine[0] == '/')
					++sCmdLine;
				continue;
			}
		}

		if(sCmdLine[0] == '/')
		{
			while(sCmdLine[0] == '/')
				++sCmdLine;
			pCurrent = pCurrent->FindNode(oName.GetStr());
			if(pCurrent == NULL)
				return NULL;
			oName.Clear();
			bFirst = true;
			continue;
		}

		break;
	};

	sCmdLine = CString::SkipSpace(sCmdLine);

	if(!oName.Empty())
		pCurrent = pCurrent->FindNode(oName.GetStr());

	return pCurrent;
}

void CCmdSystem::GetIdentifier(const char* &sName, CString& oName)
{
	bool bFirst = true;
	sName = CString::SkipSpace(sName);
	while(sName[0])
	{
		if(bFirst)
		{
			if( (sName[0] >= 'A' && sName[0] <= 'Z') ||
					(sName[0] >= 'a' && sName[0] <= 'z') ||
					(sName[0] == '_') )
			{
				bFirst = false;
				oName += sName[0];
				++sName;
				continue;
			}
		}

		if( (sName[0] >= 'A' && sName[0] <= 'Z') ||
				(sName[0] >= 'a' && sName[0] <= 'z') ||
				(sName[0] >= '0' && sName[0] <= '9') ||
				(sName[0] == '_') )
		{
			oName += sName[0];
			++sName;
			continue;
		}

		break;
	}

	sName = CString::SkipSpace(sName);
}

void CCmdSystem::List(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *cmdline, const char* sCmdArg)
{
	FOCP_DETAIL_NAME::CCmdNode* pNode = pCmdSystem->m_pCurrent;
	if(sCmdArg[0])
	{
		pNode = pCmdSystem->GetCmdNode(sCmdArg);
		if(pNode == NULL)
		{
			pSession->Print("find the path failure\r\n");
			return;
		}
		if(!pNode->m_bBranch)
		{
			pSession->Print("this is a command\r\n");
			return;
		}
	}

	uint32 nMaxLen[3] = {0, 0, 0};

	uint32 nCounter = 0;
	CRbTreeNode* pIt = pNode->m_oNode.pBranch->First();
	CRbTreeNode* pEnd = pNode->m_oNode.pBranch->End();
	for(; pIt != pEnd; pIt = pNode->m_oNode.pBranch->GetNext(pIt))
	{
		FOCP_DETAIL_NAME::CCmdNode* &pSubNode = pNode->m_oNode.pBranch->GetItem(pIt);
		const CString& oName = pNode->m_oNode.pBranch->GetKey(pIt);
		uint32 nLen = oName.GetSize();
		if(pSubNode->m_bBranch)
			nLen += 2;
		uint32 &nMax = nMaxLen[nCounter%3];
		if(nLen > nMax)
			nMax = nLen;
		++nCounter;
	}
	pIt = pCmdSystem->m_oSysCmdList.First();
	CRbTreeNode* pEnd2 = pCmdSystem->m_oSysCmdList.End();
	for(; pIt != pEnd2; pIt = pCmdSystem->m_oSysCmdList.GetNext(pIt))
	{
		const CString& oName = pCmdSystem->m_oSysCmdList.GetKey(pIt);
		if(pNode->m_oNode.pBranch->Find(oName) == pEnd)
		{
			uint32 nLen = oName.GetSize();
			uint32 &nMax = nMaxLen[nCounter%3];
			if(nLen > nMax)
				nMax = nLen;
			++nCounter;
		}
	}

	nCounter = 0;
	pIt = pNode->m_oNode.pBranch->First();
	for(; pIt != pEnd; pIt = pNode->m_oNode.pBranch->GetNext(pIt))
	{
		FOCP_DETAIL_NAME::CCmdNode* &pSubNode = pNode->m_oNode.pBranch->GetItem(pIt);
		const CString& oName = pNode->m_oNode.pBranch->GetKey(pIt);
		if(pSubNode->m_bBranch)
			pSession->Print("[");
		pSession->Print("%s", oName.GetStr());
		if(pSubNode->m_bBranch)
			pSession->Print("]");
		uint32 nMax = nMaxLen[nCounter%3];
		++nCounter;
		if(!(nCounter%3))
			pSession->Print("\r\n");
		else
		{
			uint32 nLen = oName.GetSize();
			if(pSubNode->m_bBranch)
				nLen += 2;
			nMax -= nLen;
			for(uint32 i=0; i<=nMax; ++i)
				pSession->Print(" ");
		}
	}

	pIt = pCmdSystem->m_oSysCmdList.First();
	for(; pIt != pEnd2; pIt = pCmdSystem->m_oSysCmdList.GetNext(pIt))
	{
		const CString& oName = pCmdSystem->m_oSysCmdList.GetKey(pIt);
		if(pNode->m_oNode.pBranch->Find(oName) == pEnd)
		{
			pSession->Print("%s", oName.GetStr());
			uint32 nMax = nMaxLen[nCounter%3];
			++nCounter;
			if(!(nCounter%3))
				pSession->Print("\r\n");
			else
			{
				uint32 nLen = oName.GetSize();
				nMax -= nLen;
				for(uint32 i=0; i<=nMax; ++i)
					pSession->Print(" ");
			}
		}
	}
	pSession->Print("\r\n");
}

void CCmdSystem::Pwd(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CString oPath;
	pCmdSystem->GetCurrentPath(oPath);
	pSession->Print("%s\r\n", oPath.GetStr());
}

void CCmdSystem::GetCurrentPath(CString &oPath)
{
	oPath.Clear();
	m_oMutex.Enter();
	GetFullPath(m_pCurrent, oPath);
	if(oPath.Empty())
		oPath = "/";
	m_oMutex.Leave();
}

void CCmdSystem::GetFullPath(FOCP_DETAIL_NAME::CCmdNode* pNode, CString &oPath)
{
	if(pNode->m_pParent)
		GetFullPath(pNode->m_pParent, oPath);
	if(pNode->m_sName)
	{
		oPath += "/";
		oPath += pNode->m_sName;
	}
}

void CCmdSystem::Help(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	const char* sAnother = sCmdArg;
	FOCP_DETAIL_NAME::CCmdNode* pNode = pCmdSystem->GetCmdNode(sCmdArg);
	if(pNode == NULL)
	{
		CString oName;
		sCmdArg = sAnother;
		pCmdSystem->GetIdentifier(sCmdArg, oName);
		CRbTreeNode* pIt = pCmdSystem->m_oSysCmdList.Find(oName);
		if(pIt == pCmdSystem->m_oSysCmdList.End())
			pSession->Print("find the command failure\r\n");
		else
		{
			CSysCmdNode& oCmdNode = pCmdSystem->m_oSysCmdList.GetItem(pIt);
			pSession->Print("%s\r\n", oCmdNode.oDesc.GetStr());
		}
	}
	else if(pNode->m_bBranch)
		pSession->Print("this is a directory\r\n");
	else
		pSession->Print("%s\r\n", pNode->m_oNode.pLeaf->oDesc.GetStr());
}

void CCmdSystem::Cd(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	sCmdArg = CString::SkipSpace(sCmdArg);
	if(!sCmdArg[0])
	{
		pCmdSystem->m_pPrev = pCmdSystem->m_pCurrent;
		pCmdSystem->m_pCurrent = &pCmdSystem->m_oRoot;
	}
	else if(sCmdArg[0] == '-')
		Swap(pCmdSystem->m_pCurrent, pCmdSystem->m_pPrev);
	else
	{
		FOCP_DETAIL_NAME::CCmdNode* pNode = pCmdSystem->GetCmdNode(sCmdArg);
		if(pNode == NULL)
			pSession->Print("the path isn't existed\r\n");
		else if(!pNode->m_bBranch)
			pSession->Print("this isn't a directory\r\n");
		else
		{
			pCmdSystem->m_pPrev = pCmdSystem->m_pCurrent;
			pCmdSystem->m_pCurrent = pNode;
		}
	}
}

void CCmdSystem::Quit(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	pSession->SetQuit();
}

void CCmdSystem::Clear(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	pSession->ClearScreen();
}

void CCmdSystem::Stop(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CApplication::GetInstance()->StopNotice();
}

void CCmdSystem::Call(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	sCmdArg = CString::SkipSpace(sCmdArg);
	if(sCmdArg[0])
		pSession->ProcessScript(sCmdArg);
}

static void PrintLogMode(CCmdSession* pSession, CString &oModuleName, uint32 nMode)
{
	if(nMode & FOCP_LOG_SCREEN)
		pSession->Print("The log of '%s' can print to the screen\r\n", oModuleName.GetStr());
	if(nMode & FOCP_LOG_FILE)
		pSession->Print("The log of '%s' can print to the file\r\n", oModuleName.GetStr());
	if(nMode & FOCP_LOG_SERVER)
		pSession->Print("The log of '%s' can send to the log server\r\n", oModuleName.GetStr());
}

static void PrintLogLevel(CCmdSession* pSession, CString &oModuleName, uint32 nLevel)
{
	pSession->Print("The log level of '%s' is %s\r\n", oModuleName.GetStr(), GetLogLevelName(nLevel));
}

static void PrintLogFilter(CCmdSession* pSession, CString &oModuleName, uint32 nFilter)
{
	if(nFilter & FOCP_LOG_HOST)
		pSession->Print("The log of '%s' can record the host information\r\n", oModuleName.GetStr());
	if(nFilter & FOCP_LOG_DATE)
		pSession->Print("The log of '%s' can record the date information\r\n", oModuleName.GetStr());
	if(nFilter & FOCP_LOG_APPN)
		pSession->Print("The log of '%s' can record the application information\r\n", oModuleName.GetStr());
	if(nFilter & FOCP_LOG_MODU)
		pSession->Print("The log of '%s' can record the module information\r\n", oModuleName.GetStr());
	if(nFilter & FOCP_LOG_SRCF)
		pSession->Print("The log of '%s' can record the file information\r\n", oModuleName.GetStr());
	if(nFilter & FOCP_LOG_FUNC)
		pSession->Print("The log of '%s' can record the function information\r\n", oModuleName.GetStr());
}

void CCmdSystem::SetLogModeCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nVal, nMode;
	CString oModuleName;
	CFormatString oArg(sCmdArg);
	uint32 nArg = oArg.Scan("%u%s", &nVal, &oModuleName);
	if(nArg < 1)
	{
		pSession->Print("Missing the mode argument\r\n");
		return;
	}
	nMode = 0;
	if(nVal & FOCP_LOG_SCREEN)
		nMode |= FOCP_LOG_SCREEN;
	if(nVal & FOCP_LOG_FILE)
		nMode |= FOCP_LOG_FILE;
	if(nVal & FOCP_LOG_SERVER)
		nMode |= FOCP_LOG_SERVER;
	if(!nMode)
	{
		pSession->Print("The mode argument is invalid\r\n");
		return;
	}
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	SetLogMode(oModuleName.GetStr(), nMode);
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogMode(pSession, oModuleName, nMode);
}

void CCmdSystem::GetLogModeCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nMode;
	CString oModuleName;
	CFormatString oArg(sCmdArg);
	oArg.Scan("%s", &oModuleName);
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	nMode = GetLogMode(oModuleName.GetStr());
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogMode(pSession, oModuleName, nMode);
}

void CCmdSystem::SetLogLevelCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nLevel;
	CString oLevel, oModuleName;
	CFormatString oArg(sCmdArg);
	uint32 nArg = oArg.Scan("%s%s", &oLevel, &oModuleName);
	if(nArg < 1)
	{
		pSession->Print("Missing the level argument\r\n");
		return;
	}
	nLevel = 0;
	if(!oLevel.Compare("ERROR", false))
		nLevel = FOCP_LOG_ERROR;
	else if(!oLevel.Compare("WARNING", false))
		nLevel = FOCP_LOG_WARNING;
	else if(!oLevel.Compare("DEBUG", false))
		nLevel = FOCP_LOG_DEBUG;
	else
	{
		pSession->Print("The level argument is invalid\r\n");
		return;
	}
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	SetLogLevel(oModuleName.GetStr(), nLevel);
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogLevel(pSession, oModuleName, nLevel);
}

void CCmdSystem::GetLogLevelCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nLevel;
	CString oModuleName;
	CFormatString oArg(sCmdArg);
	oArg.Scan("%s", &oModuleName);
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	nLevel = GetLogLevel(oModuleName.GetStr());
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogLevel(pSession, oModuleName, nLevel);
}

void CCmdSystem::SetLogFilterCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nVal, nFilter;
	CString oModuleName;
	CFormatString oArg(sCmdArg);
	uint32 nArg = oArg.Scan("%u%s", &nVal, &oModuleName);
	if(nArg < 1)
	{
		pSession->Print("Missing the filter argument\r\n");
		return;
	}
	nFilter = 0;
	if(nVal & FOCP_LOG_HOST)
		nFilter |= FOCP_LOG_HOST;
	if(nVal & FOCP_LOG_DATE)
		nFilter |= FOCP_LOG_DATE;
	if(nVal & FOCP_LOG_APPN)
		nFilter |= FOCP_LOG_APPN;
	if(nVal & FOCP_LOG_MODU)
		nFilter |= FOCP_LOG_MODU;
	if(nVal & FOCP_LOG_SRCF)
		nFilter |= FOCP_LOG_SRCF;
	if(nVal & FOCP_LOG_FUNC)
		nFilter |= FOCP_LOG_FUNC;
	if(!nFilter)
	{
		pSession->Print("The filter argument is invalid\r\n");
		return;
	}
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	SetLogFilter(oModuleName.GetStr(), nFilter);
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogFilter(pSession, oModuleName, nFilter);
}

void CCmdSystem::GetLogFilterCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	uint32 nFilter;
	CString oModuleName;
	CFormatString oArg(sCmdArg);
	oArg.Scan("%s", &oModuleName);
	if(!HaveModule(oModuleName.GetStr()))
	{
		pSession->Print("The module isn't existed\r\n");
		return;
	}
	nFilter = GetLogFilter(oModuleName.GetStr());
	if(oModuleName.Empty())
		oModuleName = "Global";
	PrintLogFilter(pSession, oModuleName, nFilter);
}

static void PrintLogModule(void *pPara, const char* sName)
{
	CCmdSession* pSession = (CCmdSession*)pPara;
	pSession->Print("\t%s\r\n", sName);
}

void CCmdSystem::ListLogModuleCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	pSession->Print("The modules registered in the log system:");
	WalkLogModule(pSession, PrintLogModule);
}

int32 CCmdSystem::GetCompletions(char *sCommand, char **sCompletions, int32 max_completions)
{
	int32 nums = 0;
	uint32 nLen = CString::StringLength(sCommand);
	CRbTreeNode* pIt = m_pCurrent->m_oNode.pBranch->First();
	CRbTreeNode* pEnd = m_pCurrent->m_oNode.pBranch->End();
	for(; pIt!=pEnd && nums<max_completions; pIt=m_pCurrent->m_oNode.pBranch->GetNext(pIt))
	{
		FOCP_DETAIL_NAME::CCmdNode* pNode = m_pCurrent->m_oNode.pBranch->GetItem(pIt);
		if(!CString::StringCompare(sCommand, pNode->m_sName, false, nLen))
		{
			uint32 nSize;
			CString oName(pNode->m_sName);
			sCompletions[nums] = oName.Detach(nSize);
			++nums;
		}
	}
	if(nums<max_completions)
	{
		pIt = m_oSysCmdList.First();
		pEnd = m_oSysCmdList.End();
		for(; pIt!=pEnd && nums<max_completions; pIt=m_oSysCmdList.GetNext(pIt))
		{
			const char* sName = m_oSysCmdList.GetKey(pIt).GetStr();
			if(!CString::StringCompare(sCommand, sName, false, nLen))
			{
				uint32 nSize;
				CString oName(sName);
				sCompletions[nums] = oName.Detach(nSize);
				++nums;
			}
		}
	}
	return nums;
}

FOCP_END();
