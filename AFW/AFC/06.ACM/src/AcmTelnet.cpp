
#include "AcmTelnet.hpp"

FOCP_BEGIN();

CAcmTelnetLink::CAcmTelnetLink():m_oEvent(false, false)
{
	SetHistoryCmd(&m_oHistoryCmd);
	m_pCmdSystem = NULL;
	m_nBufLen = 0;
	m_nIdx = 0;
	m_cPrompt = '#';
	m_pServer = NULL;
	m_hLink = 0xFFFFFFFF;
	m_pRunning = NULL;
}

CAcmTelnetLink::~CAcmTelnetLink()
{
}

void CAcmTelnetLink::Initialize(uint32 hLink, CAcmTelnetServer* pServer, CCmdSystem* pCmdSystem)
{
	m_hLink = hLink;
	m_pServer = pServer;
	m_pCmdSystem = pCmdSystem;
	m_nPos = 0;
}

uint32 CAcmTelnetLink::GetClassId() const
{
	return FOCP_TELNET_LINK_FIBER;
}

int32 CAcmTelnetLink::GetWorkerGroupId()
{
	return FOCP_FIBER_TELNET_GROUP;
}

void CAcmTelnetLink::Reset()
{
	ulong nLink = m_hLink;
	if(nLink != 0xFFFFFFFF)
	{
		m_hLink = 0xFFFFFFFF;
		m_pServer->m_pIocpServer->FreeLink(nLink);
	}
	m_bQuit = false;
	m_oHistoryCmd.Clear();
	m_oEvent.Reset();
	m_nPos = 0;
	m_oStream.SetPosition(0);
	m_oStream.Truncate();
	CFiber::Reset();
}

void CAcmTelnetLink::Print(const char *sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	PrintV(sFormat, pArgList);
	VaEnd(pArgList);
}

void CAcmTelnetLink::PrintV(const char* sFormat, CVaList& pArgList)
{
	CFormatString oString;
	oString.PrintV(sFormat, pArgList);
	char* pStr = (char*)oString.GetStr();
	uint32 nSize = oString.GetSize();
	m_pServer->m_pIocpServer->SendTo(m_hLink, pStr, nSize);
}

void CAcmTelnetLink::ClearScreen()
{
	Print("\x1b[H\x1b[J");
}

const char* CAcmTelnetLink::ReadLine(bool bPrompt, char cPrompt)
{
	const char* sLine = CCmdSession::ReadLine(bPrompt, cPrompt);
	if(sLine)
		return sLine;
	bool &bRunning = *m_pRunning;
	char c = m_cPrompt;
	m_cPrompt = cPrompt;
	m_oLine.Clear();
	while(ReadLine(bRunning, m_bPrompt && bPrompt))
	{
		if(m_oContext.cmd[m_oContext.l-1] == '\\')
		{
			--m_oContext.l;
			if(m_oContext.l)
				m_oLine.Append(m_oContext.cmd, m_oContext.l);
			m_bPrompt = false;
			continue;
		}
		m_bPrompt = true;
		m_oLine.Append(m_oContext.cmd, m_oContext.l);
		if(m_oLine.GetSize() == 0)
			continue;
		sLine = m_oLine.GetStr();
		sLine = CString::SkipSpace(sLine);
		if(sLine[0])
		{
			m_cPrompt = c;
			return sLine;
		}
	}
	m_cPrompt = c;
	return "";
}

int32 CAcmTelnetLink::OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg)
{
	m_oMutex.Enter();
	uint32 nPos = m_oStream.GetSize();
	if(nPos == m_nPos)
		m_oEvent.Set();
	m_oStream.SetPosition(nPos);
	m_oStream.Write((void*)sBuf, nBufLen);
	m_oMutex.Leave();
	return 0;//不另外建立消息处理机制。
}

void CAcmTelnetLink::UnBindLink()
{
	if(m_hLink != 0xFFFFFFFF)
	{
		m_hLink = 0xFFFFFFFF;
		StopNotify();
	}
}

void CAcmTelnetLink::MainProc(bool &bRunning)
{
	CBinary::MemorySet(&m_oContext, 0, sizeof(m_oContext));
	m_oContext.insertmode = 1;
	m_bPrompt = true;
	m_pRunning = &bRunning;

	const char *sNegoTiate =
		"\xFF\xFB\x03"
		"\xFF\xFB\x01"
		"\xFF\xFD\x03"
		"\xFF\xFD\x01"
		"Welcome to %s telnet service.\r\n\r\n";
	Print(sNegoTiate, CApplication::GetInstance()->GetAppName());

	while(ReadLine(bRunning, m_bPrompt))
	{
		if(m_oContext.cmd[m_oContext.l-1] == '\\')
		{
			--m_oContext.l;
			if(m_oContext.l)
				m_oLine.Append(m_oContext.cmd, m_oContext.l);
			m_bPrompt = false;
			continue;
		}
		m_bPrompt = true;
		m_oLine.Append(m_oContext.cmd, m_oContext.l);
		if(m_oLine.GetSize() == 0)
			continue;
		const char* sLine = m_oLine.GetStr();
		sLine = CString::SkipSpace(sLine);
		if(sLine[0])
		{
			CString oLine(sLine);
			sLine = oLine.GetStr();
			m_pCmdSystem->ProcessCmd(this, sLine);
			if(m_bQuit)
				break;
		}
		m_oLine.Clear();
	}

	m_pRunning = NULL;
}

void CAcmTelnetLink::StopNotify()
{
	CFiber::StopNotify();
	m_oEvent.Set();
}

int32 CAcmTelnetLink::ReadChar(const bool &bRunning, uint8 & c)
{
loop:
	if(m_nBufLen && m_nIdx < m_nBufLen)
	{
		c = m_sBuf[m_nIdx];
		++m_nIdx;
		return 1;
	}
loop2:
	m_oMutex.Enter();
	m_oStream.SetPosition(m_nPos);
	m_nBufLen = m_oStream.Read(m_sBuf, 1024);
	if(m_nBufLen <= 0)
	{
		m_oEvent.Reset();
		m_oMutex.Leave();
		if(m_hLink == 0xFFFFFFFF)
			return 0;
		m_oEvent.Wait();
		if(!bRunning)
			return 0;
		goto loop2;
	}
	m_oStream.LeftTrim();
	m_nPos = m_oStream.GetPosition();
	m_oMutex.Leave();
	m_nIdx = 0;
	goto loop;
}

bool CAcmTelnetLink::ReadLine(const bool &bRunning, bool bPrompt)
{
	while (bRunning)
	{
		int32 in_history = 0;
		int32 lastchar = 0;
		int32 showit = 1;

		if(m_oContext.oldcmd)
		{
			m_oContext.l = m_oContext.cursor = m_oContext.oldl;
			m_oContext.oldcmd[m_oContext.l] = 0;
			showit = 1;
			m_oContext.oldcmd = NULL;
			m_oContext.oldl = 0;
		}
		else
		{
			CBinary::MemorySet(m_oContext.cmd, 0, 4096);
			m_oContext.l = 0;
			m_oContext.cursor = 0;
		}

		while (bRunning)
		{
			if (showit)
			{
				if(bPrompt)
					Print("%c ", m_cPrompt);
				m_oContext.cmd[m_oContext.l] = 0;
				if(m_oContext.l)
					Print("%s", m_oContext.cmd);
				showit = 0;
			}

			if ((m_oContext.n = ReadChar(bRunning, m_oContext.c)) <= 0)
			{
				m_oContext.l = -1;
				break;
			}

			if (m_oContext.skip)
			{
				m_oContext.skip--;
				continue;
			}

			if (m_oContext.c == 255 && !m_oContext.is_telnet_option)
			{
				m_oContext.is_telnet_option++;//1
				continue;
			}

			if (m_oContext.is_telnet_option)
			{
				if (m_oContext.c >= 251 && m_oContext.c <= 254)
				{
					m_oContext.is_telnet_option = m_oContext.c;
					continue;
				}
				if (m_oContext.c != 255)
				{
					m_oContext.is_telnet_option = 0;
					continue;
				}
				m_oContext.is_telnet_option = 0;
			}

			if (m_oContext.esc)
			{
				/* Handle arrow keys */
				if(m_oContext.esc == 91)
				{
					/* UP & DOWN keys */
					if(m_oContext.c == 65 || m_oContext.c == 66)
					{
						int32 history_found = 0;
						if(m_oContext.c == 65) /* UP */
						{
							in_history--;
							if (in_history < 0)
							{
								for (in_history = FOCP_CLI_MAXHISTORY-1; in_history >= 0; in_history--)
								{
									if (HistoryCmd(in_history))
									{
										history_found = 1;
										break;
									}
								}
							}
							else if(HistoryCmd(in_history))
								history_found = 1;
						}
						else /* Down */
						{
							in_history++;
							if (in_history >= FOCP_CLI_MAXHISTORY || !HistoryCmd(in_history))
							{
								int32 i = 0;
								for (i = 0; i < FOCP_CLI_MAXHISTORY; i++)
								{
									if(HistoryCmd(i))
									{
										in_history = i;
										history_found = 1;
										break;
									}
								}
							}
							else if(HistoryCmd(in_history))
								history_found = 1;
						}
						if (history_found && HistoryCmd(in_history))
						{
							ClearLine(m_oContext.cmd, m_oContext.l, m_oContext.cursor);
							CBinary::MemorySet(m_oContext.cmd, 0, 4096);
							CString::StringCopy(m_oContext.cmd, HistoryCmd(in_history));
							m_oContext.l = m_oContext.cursor = CString::StringLength(m_oContext.cmd);
							m_oContext.cmd[m_oContext.l] = 0;
							Print("%s", m_oContext.cmd);
						}
					}
					else if(m_oContext.c == 68) /* LEFT */
					{
						if (m_oContext.cursor != 0)
						{
							m_oContext.cursor--;
							Print("\x08");
						}
					}
					else if(m_oContext.c == 67) /* RIGHT */
					{
						if (m_oContext.cursor < m_oContext.l)
						{
							Print("%c", m_oContext.cmd[m_oContext.cursor]);
							m_oContext.cursor++;
						}
					}
				}
				else if(m_oContext.c == 91)
				{
					m_oContext.esc = m_oContext.c;
					continue;
				}
				m_oContext.esc = 0;
				continue;
			}
			if (m_oContext.c == '\n')
				continue;
			if (m_oContext.c == '\r')
			{
				Print("\r\n");
				break;
			}
			if (m_oContext.c == 0)
				continue;
			if (m_oContext.c == 3)
			{
				Print("\x07");
				continue;
			}
			if (m_oContext.c == 27)
			{
				m_oContext.esc = 1;
				continue;
			}
			if (m_oContext.c == 127 || m_oContext.c == 8)/* Backspace */
			{
				if(m_oContext.l == 0 || m_oContext.cursor == 0)
					Print("\x07");
				else if(m_oContext.l == m_oContext.cursor)
				{
					--m_oContext.cursor;
					m_oContext.cmd[m_oContext.cursor] = '\0';
					Print("\x08\x20\x08");
					--m_oContext.l;
				}
				else
				{
					int32 i;
					--m_oContext.cursor;
					for (i = m_oContext.cursor; i <= m_oContext.l; i++)
						m_oContext.cmd[i] = m_oContext.cmd[i+1];
					Print("\x08");
					Print("%s", m_oContext.cmd + m_oContext.cursor);
					Print(" ");
					for (i = 0; i <= (int32)CString::StringLength(m_oContext.cmd + m_oContext.cursor); i++)
						Print("\x08");
					--m_oContext.l;
				}
				continue;
			}
			if (m_oContext.c == 12)/* Ctrl-L */
			{
				int32 i;
				int32 totallen = m_oContext.l + 1;
				int32 cursorback = m_oContext.l - m_oContext.cursor;
				for (i = 0; i < totallen; i++)
					Print("\x08");
				for (i = 0; i < totallen; i++)
					Print(" ");
				for (i = 0; i < totallen; i++)
					Print("\x08");
				Print("%%");
				m_oContext.cmd[m_oContext.l] = 0;
				Print("%s", m_oContext.cmd);
				for (i = 0; i < cursorback; i++)
					Print("\x08");
				continue;
			}
			if (m_oContext.c == 21)/* Ctrl-U */
			{
				ClearLine(m_oContext.cmd, m_oContext.l, m_oContext.cursor);
				m_oContext.l = 0;
				m_oContext.cursor = 0;
				continue;
			}
			if (m_oContext.c == 4)/* Ctrl-D */
			{
				CString::StringCopy(m_oContext.cmd, "quit");
				m_oContext.l = m_oContext.cursor = CString::StringLength(m_oContext.cmd);
				Print("quit\r\n");
				break;
			}
			if(m_oContext.c == 9)/* Tab completion */
			{
				char *completions[128];
				int32 num_completions = 0;
				if(m_oContext.cursor == m_oContext.l)
					continue;
				if(m_oContext.l > 0)
					num_completions = m_pCmdSystem->GetCompletions(m_oContext.cmd, completions, 128);
				if (num_completions == 0)
				{
					m_oContext.c = 7;
					Print("\x07");
				}
				else if(num_completions == 1)
				{
					/* Single completion */
					int32 i;
					for (i = m_oContext.l; i > 0; i--, m_oContext.cursor--)
					{
						Print("\x08");
						if (i == ' ')
							break;
					}
					int32 nLen = CString::StringLength(completions[0]);
					CString::StringCopy(m_oContext.cmd+i, completions[0]);
					m_oContext.l += nLen;
					m_oContext.cursor = m_oContext.l;
					Print("%s", completions[0]);
				}
				else if(lastchar == 9)/* double tab */
				{
					int32 i;
					Print("\r\n");
					for (i = 0; i < num_completions; i++)
					{
						Print("%s", completions[i]);
						if (i % 4 == 3)
							Print("\r\n");
						else
							Print("\t");
					}
					if (i % 4 != 3)
						Print("\r\n");
					showit = 1;
				}
				else
				{
					/* More than one completion */
					lastchar = m_oContext.c;
					m_oContext.c = 0x07;
					Print("\x07");
				}
				continue;
			}
			if(m_nMax && m_oContext.l >= (int32)m_nMax)//最大长度限制
				continue;
			if (m_oContext.cursor == m_oContext.l)/* Normal character typed */
			{
				m_oContext.cmd[m_oContext.cursor] = m_oContext.c;
				++m_oContext.l;
			}
			else if (m_oContext.insertmode)/* Middle of text */
			{
				int32 i;
				for (i = m_oContext.l; i >= m_oContext.cursor; i--)
					m_oContext.cmd[i+1] = m_oContext.cmd[i];
				m_oContext.cmd[m_oContext.cursor] = m_oContext.c;
				Print("%s", m_oContext.cmd+m_oContext.cursor);
				for (i = 0; i < (m_oContext.l - m_oContext.cursor + 1); i++)
					Print("\x08");
				++m_oContext.l;
			}
			else
				m_oContext.cmd[m_oContext.cursor] = m_oContext.c;
			++m_oContext.cursor;
			if (m_oContext.c == 63 && m_oContext.cursor == m_oContext.l)
			{
				Print("\r\n");
				m_oContext.oldcmd = m_oContext.cmd;
				m_oContext.oldl = m_oContext.cursor = m_oContext.l - 1;
				break;
			}
			if(m_bHide)
				Print("*");
			else
				Print("%c", m_oContext.c);
			m_oContext.oldcmd = NULL;
			m_oContext.oldl = 0;
			lastchar = m_oContext.c;
		}
		if (m_oContext.l < 0)
			return false;
		if (m_oContext.l == 0)
			continue;
		if (!m_bHide && !m_bForbidSave && m_oContext.cmd[m_oContext.l-1] != '?' && CString::StringCompare(m_oContext.cmd, "history", false) != 0)
			AddHistory(m_oContext.cmd, m_oContext.l);
		break;
	}
	return bRunning;
}

void CAcmTelnetLink::ClearLine(char* cmd, int32 l, int32 cursor)
{
	int32 i;
	if (cursor < l)
	{
		for (i = 0; i < (l - cursor); i++)
			Print(" ");
	}
	for (i = 0; i < l; i++)
		cmd[i] = '\x08';
	for (; i < l * 2; i++)
		cmd[i] = '\x20';
	for (; i < l * 3; i++)
		cmd[i] = '\x08';
	Print("%s", cmd);
	CBinary::MemorySet(cmd, 0, i);
}

void CAcmTelnetLink::History(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CHistoryCmd* pCmd = pSession->GetHistoryCmd();
	if(pCmd)
		pCmd->List(pSession);
}

static CFiberClass<CAcmTelnetLink> g_oTelnetLinkFiberClass(10, false);

CAcmTelnetServer::CAcmTelnetServer():m_oLsnThread(this)
{
	m_pIocpServer = CIocpServer::GetInstance();
	m_pCmdSystem = CCmdSystem::GetInstance();
	m_hLink = 0xFFFFFFFF;
	m_pCmdSystem->RegisterSysCmd("history", "history<CR>\r\n\tshow all history commands\r\n", CAcmTelnetLink::History);
}

CAcmTelnetServer::~CAcmTelnetServer()
{
}

bool CAcmTelnetServer::Initialize(uint16 nServerPort)
{
	if(nServerPort == 0)
		return false;
	bool bRet = false;
	CFormatString oFileName;
	oFileName.Print("tcp://|127.0.0.1:%u16", nServerPort);
	switch(m_oLsnTcp.Open(oFileName.GetStr(), "l"))
	{
	case 0:
		bRet = true;
		break;
	case -1:
		FocpErrorS("missing the tcp file driver");
		break;
	case -2:
	case -3:
		FocpError(("bind(127.0.0.1:%u16) failure", nServerPort));
		break;
	default:
		FocpErrorS("other error");
		break;
	}
	return bRet;
}

void CAcmTelnetServer::Cleanup()
{
	m_oLsnTcp.Close();
}

void CAcmTelnetServer::Start()
{
	m_oLsnThread.Start();
}

void CAcmTelnetServer::Stop(bool bBlock)
{
	m_oLsnTcp.Close();
	m_oLsnThread.Stop(bBlock);
	if(bBlock)
	{
		if(m_hLink != 0xFFFFFFFF)
		{
			m_pIocpServer->FreeLink(m_hLink);
			m_hLink = 0xFFFFFFFF;
		}
	}
}

void CAcmTelnetServer::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(m_hLink == 0xFFFFFFFF)
	{
		m_hLink = m_pIocpServer->AllocLink();
		if(m_hLink == 0xFFFFFFFF)
			return;
	}
	CFile oFile;
	if(m_oLsnTcp.Accept(oFile))
	{
		uint32 nIdx = 0;
		CAcmTelnetLink* pLink;
		while(true)
		{
			pLink = (CAcmTelnetLink*)(void*)CFiberManager::GetInstance()->
					CreateFiber(FOCP_TELNET_LINK_FIBER);
			if(pLink)
				break;
			pLink = (CAcmTelnetLink*)(void*)g_oTelnetLinkFiberClass.QueryObject(nIdx);
			if(pLink)
			{
				pLink->UnBindLink();
				++nIdx;
			}
			CCooperator::Sleep(100);
		}
		ulong hHandle;
		oFile.Close(&hHandle);
		pLink->Initialize(m_hLink, this, m_pCmdSystem);
		m_pIocpServer->BindLink(m_hLink, hHandle, pLink);
		if(!pLink->Start())
			CFiberManager::GetInstance()->GetFactory()->DestroyObject(pLink);
		m_hLink = 0xFFFFFFFF;
	}
}

FOCP_END();
