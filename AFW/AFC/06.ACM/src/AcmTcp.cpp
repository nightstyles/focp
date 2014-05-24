
#include "AcmTcp.hpp"

FOCP_BEGIN();

enum
{
	//基本TCP消息定义
	TCP_HELLO_REQ = 0X11111110,
	TCP_LOGIN_REQ,
	TCP_LOGIN_RESP,
	TCP_INIT_FINISH,
};

CAcmTcpLink::CAcmTcpLink(CAcmTcpServer* pServer)
{
	m_hLink = 0xFFFFFFFF;
	m_nSize = 0;
	m_pNext = NULL;
	m_pPrev = NULL;
	m_pServer = pServer;
	m_nStatus = 0;
	m_bHello = true;
}

CAcmTcpLink::~CAcmTcpLink()
{
}

uint32 CAcmTcpLink::GetStatus()
{
	return m_nStatus;
}

int32 CAcmTcpLink::OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg)
{
	uint32 nRest, nSize = m_oStream.GetSize();
	if(nSize < sizeof(m_nSize))
	{
		nRest = sizeof(m_nSize) - nSize;
		if(nRest <= (uint32)nBufLen)
		{
			m_oStream.Write((void*)sBuf, nRest);
			m_oStream.SetPosition(0);
			m_oStream.Read(m_nSize);
			if(m_nSize >= sizeof(CTcpHead))
			{
				m_oStream.SetPosition(sizeof(m_nSize));
				if(m_nSize-sizeof(m_nSize) <= (uint32)nBufLen-nRest)
				{
					m_oStream.Write((void*)(sBuf+nRest), m_nSize - sizeof(m_nSize));
					oMsg.Swap(m_oStream);
					nBufLen -= m_nSize;
					m_nSize = 0;
					return nBufLen;
				}
				nBufLen -= nRest;
				if(nBufLen)
					m_oStream.Write((void*)(sBuf+nRest), nBufLen - nRest);
				return 0;
			}
			m_oStream.SetPosition(0);
			m_oStream.Truncate();
			m_nSize = 0;
			return nBufLen;
		}
		m_oStream.Write((void*)sBuf, nBufLen);
		return 0;
	}
	nRest = m_nSize - nSize;
	if(nRest <= (uint32)nBufLen)
	{
		m_oStream.Write((void*)sBuf, nRest);
		oMsg.Swap(m_oStream);
		m_nSize = 0;
		return nBufLen - nRest;
	}
	m_oStream.Write((void*)sBuf, nBufLen);
	return 0;
}

void CAcmTcpLink::UnBindLink()
{
	if(m_hLink != 0xFFFFFFFF)
	{
		m_hLink = 0xFFFFFFFF;
		m_oStream.SetPosition(0);
		m_oStream.Truncate();
		m_nSize = 0;
		m_bHello = true;
		m_pServer->FreeLink(this);
	}
}

void CAcmTcpLink::Close()
{
	if(m_hLink != 0xFFFFFFFF)
		CIocpServer::GetInstance()->FreeLink(m_hLink);
}

bool CAcmTcpLink::SendStream(CMemoryStream &oStream)
{
	char * pBuf;
	uint32 nCopySize;
	oStream.SetPosition(0);
	m_oMutex.Enter();
	while((pBuf = oStream.GetBuf(nCopySize)) && nCopySize)
	{
		int32 nRet = m_pServer->m_pIocpServer->SendTo(m_hLink, pBuf, nCopySize);
		if(nRet != (int32)nCopySize)
		{
			m_oMutex.Leave();
			return false;
		}
		oStream.Seek(nRet);
	}
	m_oMutex.Leave();
	return false;
}

void CAcmTcpLink::Hello(uint32 nMagic)
{
	if(m_nStatus != 3)
		return;
	if(m_bHello)
	{
		CMemoryStream oStream;
		oStream.Write((uint32)16);
		oStream.Write(nMagic);
		oStream.Write((uint32)ACM_HELLO_MODULE);
		oStream.Write((uint32)TCP_HELLO_REQ);
		SendStream(oStream);
	}
	else
		m_bHello = true;
}

void CAcmTcpLink::CreateMsgHead(CMemoryStream &oStream, uint32 nMagic, uint32 nModule, uint32 nCmd)
{
	oStream.SetLocalCode(false);
	oStream.SetPosition(0);
	oStream.Truncate();
	oStream.Write((uint32)0);
	oStream.Write(nMagic);
	oStream.Write(nModule);
	oStream.Write(nCmd);
}

void CAcmTcpLink::BuildMsgHead(CMemoryStream &oStream)
{
	oStream.SetLocalCode(false);
	oStream.SetPosition(0);
	oStream.Write(oStream.GetSize());
}

void CAcmTcpLink::Send(CMemoryStream &oStream)
{
	if(SendStream(oStream))
		m_bHello = false;
}

CAcmTcpModule::CAcmTcpModule()
{
}

CAcmTcpModule::~CAcmTcpModule()
{
}

CAcmTcpServer::CAcmTcpServer():
	m_oLsnThread(this), m_oHelloThread(this),
	m_oContainer(FocpFieldOffset(CAcmTcpLink, m_pNext)),
	m_oRunning(FocpFieldOffset(CAcmTcpLink, m_pPrev),FocpFieldOffset(CAcmTcpLink, m_pNext))
{
	m_pIocpServer = CIocpServer::GetInstance();
	m_hLink = 0xFFFFFFFF;
	m_pLink = NULL;
	m_nServerPort = 0;
}

CAcmTcpServer::~CAcmTcpServer()
{
	Cleanup();
}

CAcmTcpServer* CAcmTcpServer::GetInstance()
{
	return CSingleInstance<CAcmTcpServer>::GetInstance();
}

void CAcmTcpServer::RegisterModule(uint32 nModule, CAcmTcpModule* pModule)
{
	if(!m_oModules.Register(nModule,pModule))
		FocpAbort(("CAcmTcpServer::RegisterModule(%u) failure", nModule));
}

void CAcmTcpServer::DeRegisterModule(uint32 nModule)
{
	m_oModules.DeRegister(nModule);
}

CAcmTcpModule* CAcmTcpServer::QueryModule(uint32 nModule)
{
	return m_oModules.Query(nModule);
}

void CAcmTcpServer::ReleaseModule(uint32 nModule)
{
	m_oModules.Release(nModule);
}

uint32 CAcmTcpServer::Initialize(uint16 nServerPort, uint32 nWorkerNum, uint32 nMagic)
{
	CFormatString oFileName;
	oFileName.Print("tcp://|0.0.0.0:%u16", nServerPort);
	if(m_oLsnTcp.Open(oFileName.GetStr(), "l"))
	{
		m_oLsnTcp.Close();
		FocpLog(FOCP_LOG_ERROR, ("CAcmTcpServer::Initialize(%u16), bind failure", nServerPort));
		return 1;
	}
	m_nMagic = nMagic;
	if(nWorkerNum == 0)
		nWorkerNum = 5;
	m_oProcThreadPool.Initialzie(nWorkerNum, this);
	m_nServerPort = nServerPort;
	return 0;
}

uint16 CAcmTcpServer::GetPort()
{
	return m_nServerPort;
}

void CAcmTcpServer::Cleanup()
{
	m_oLsnTcp.Redirect(NULL, true);
	DestroyAllLinks();
}

uint32 CAcmTcpServer::Start(bool bNeedHello)
{
	if(bNeedHello)
		m_oHelloThread.Start();
	m_oLsnThread.Start();
	m_oProcThreadPool.Start();
	return 0;
}

void CAcmTcpServer::Stop(bool bBlock)
{
	m_oLsnTcp.Close();
	CloseAll();
	m_oLsnThread.Stop(false);
	m_oHelloThread.Stop(false);
	m_oProcThreadPool.Stop(false);
	if(bBlock)
	{
		m_oLsnThread.Stop(true);
		m_oHelloThread.Stop(true);
		m_oProcThreadPool.Stop(true);
		if(m_hLink != 0xFFFFFFFF)
		{
			m_pIocpServer->FreeLink(m_hLink);
			m_hLink = 0xFFFFFFFF;
		}
		if(m_pLink)
		{
			FreeLink(m_pLink);
			m_pLink = NULL;
		}
	}
}

void CAcmTcpServer::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(pCooperator == &m_oHelloThread)
	{
		Hello();
		CCooperator::Sleep(1000);
	}
	else if(pCooperator == &m_oLsnThread)
	{
		if(m_hLink == 0xFFFFFFFF)
		{
			m_hLink = m_pIocpServer->AllocLink();
			if(m_hLink == 0xFFFFFFFF)
			{
				CCooperator::Sleep(1000);//防止空转
				return;
			}
		}
		if(!m_pLink)
		{
			m_pLink =  AllocLink();
			if(!m_pLink)
			{
				CCooperator::Sleep(1000);//防止空转
				return;
			}
		}
		CFile oFile;
		if(m_oLsnTcp.Accept(oFile))
		{
			if(ProcessLogin(&oFile))
			{
				ulong hHandle;
				oFile.Close(&hHandle);
				m_pLink->m_hLink = m_hLink;
				m_pLink->m_nStatus = 2;
				m_pIocpServer->BindLink(m_hLink, hHandle, m_pLink);
				m_hLink = 0xFFFFFFFF;
				m_pLink = NULL;
			}
		}
	}
	else
	{//处理
		uint32 hLink;
		CMemoryStream oStream;
		if(m_pIocpServer->ReadFrom(oStream, hLink))
			ProcessMsg(hLink, oStream);
	}
}

CAcmTcpLink* CAcmTcpServer::AllocLink()
{
	m_oMutex.Enter();
	CAcmTcpLink* pRet = m_oContainer.Pop();
	if(pRet == NULL)
		pRet = new CAcmTcpLink(this);
	m_oRunning.Push(pRet);
	m_oMutex.Leave();
	return pRet;
}

void CAcmTcpServer::FreeLink(CAcmTcpLink* pLink)
{
	m_oMutex.Enter();
	m_oRunning.Remove(pLink);
	m_oContainer.Push(pLink);
	m_oMutex.Leave();
}

void CAcmTcpServer::CloseAll()
{
	CAcmTcpLink* pLink;
	m_oMutex.Enter();
	pLink = m_oRunning.First();
	while(pLink)
	{
		CAcmTcpLink* pNext = pLink->m_pNext;
		pLink->Close();
		pLink = pNext;
	}
	m_oMutex.Leave();
}

void CAcmTcpServer::DestroyAllLinks()
{
	CAcmTcpLink* pLink;
	CloseAll();
	while(m_oRunning.GetSize())
		CCooperator::Sleep(20);
	m_oMutex.Enter();
	while((pLink = m_oContainer.Pop()))
		delete pLink;
	m_oMutex.Leave();
}

void CAcmTcpServer::Hello()
{
	CAcmTcpLink* pLink;
	m_oMutex.Enter();
	pLink = m_oRunning.First();
	while(pLink)
	{
		CAcmTcpLink* pNext = pLink->m_pNext;
		pLink->Hello(m_nMagic);
		pLink = pNext;
	}
	m_oMutex.Leave();
}

bool CAcmTcpServer::ProcessLogin(CFile* pFile)
{
	CMemoryStream oStream;
	uint32 nSize, nMagic, nModule, nOp, nSessionId;
	int32 nRet = pFile->Read(oStream, 20, 2000);
	if(nRet != 20)
		return false;
	oStream.SetPosition(0);
	oStream.Read(nSize);
	oStream.Read(nMagic);
	oStream.Read(nModule);
	oStream.Read(nOp);
	if(nMagic != m_nMagic || nOp != TCP_LOGIN_REQ || nModule != ACM_HELLO_MODULE)
		return false;
	oStream.Read(nSessionId);
	oStream.Write((uint32)0);//Status
	oStream.SetPosition(0);
	oStream.Write((uint32)24);
	oStream.SetPosition(12);
	oStream.Write((uint32)TCP_LOGIN_RESP);
	oStream.SetPosition(0);
	pFile->Write(oStream, 24);
	return true;
}

void CAcmTcpServer::ProcessMsg(uint32 hLink, CMemoryStream &oStream)
{
	CAcmTcpLink* pLink = (CAcmTcpLink*)m_pIocpServer->GetAssember(hLink);
	if(pLink == NULL || !pLink->m_nStatus)
		return;
	uint32 nSize, nMagic, nModule, nOp;
	oStream.SetPosition(0);
	oStream.Read(nSize);
	oStream.Read(nMagic);
	oStream.Read(nModule);
	oStream.Read(nOp);
	if(nMagic != m_nMagic)
		return;
	if(nModule == ACM_HELLO_MODULE)
	{
		if(nOp == TCP_INIT_FINISH && pLink->m_nStatus == 2)
			pLink->m_nStatus = 3;
	}
	else
	{
		CAcmTcpModule* pModule = QueryModule(nModule);
		if(pModule)
		{
			pModule->ProcessAcmModuleMsg(pLink, nOp, oStream);
			ReleaseModule(nModule);
		}
	}
}

CAcmTcpClientContext::CAcmTcpClientContext()
{
}

CAcmTcpClientContext::~CAcmTcpClientContext()
{
}

bool CAcmTcpClientContext::OnReConnect(CAcmTcpClient* pClient)
{
	return true;
}

CAcmTcpClient::CAcmTcpClient(CAcmTcpClientContext* pContext):m_oRecvThread(this), m_oConnThread(this), m_oHelloThread(this)
{
	m_bConnected = false;
	m_bLogin = false;
	m_bHello = true;
	m_bFailure = false;
	m_pContext = pContext;
	m_bConnectFailure = false;
}

CAcmTcpClient::~CAcmTcpClient()
{
	Cleanup();
}

void CAcmTcpClient::Initialize(const char* sServerAddr, uint16 nServerPort, uint32 nMagic)
{
	m_oServerAddr = sServerAddr;
	m_nServerPort = nServerPort;
	m_nMagic = nMagic;
}

void CAcmTcpClient::Initialize(uint32 nServerAddr, uint16 nServerPort, uint32 nMagic)
{
	CIpAddr oAddr;
	m_nServerPort = nServerPort;
	m_nMagic = nMagic;
	oAddr.nAddr = nServerAddr;
	oAddr.nPort = 0;
	CFile::GetIpFileName(oAddr, m_oServerAddr);
}

void CAcmTcpClient::Cleanup()
{
	m_oTcp.Close();
}

bool CAcmTcpClient::Start(bool bNeedHello)
{
	if(bNeedHello)
		m_oHelloThread.Start();
	m_oRecvThread.Start();
	m_oConnThread.Start();
	while(true)
	{
		if(m_bFailure || m_bLogin)
			break;
		CCooperator::Sleep(100);
	}
	if(m_bFailure)
		return false;
	return true;
}

void CAcmTcpClient::Stop(bool bBlock)
{
	m_oHelloThread.Stop(false);
	m_oRecvThread.Stop(false);
	m_oConnThread.Stop(false);
	m_bFailure = true;
	m_oTcp.Close();
	if(bBlock)
	{
		m_oHelloThread.Stop(true);
		m_oRecvThread.Stop(true);
		m_oConnThread.Stop(true);
	}
}

bool CAcmTcpClient::Send(char* sMsg, uint32 nSize, bool bHello)
{
	m_oMutex.Enter();
	while(nSize)
	{
		int32 nRet = m_oTcp.Write(sMsg, nSize);
		if(nRet <= 0)
		{
			m_oTcp.Close();
			break;
		}
		nSize -= nRet;
		sMsg += nRet;
	}
	bool bRet = (nSize == 0);
	if(bRet && !bHello)
		m_bHello = false;
	m_oMutex.Leave();
	return bRet;
}

void CAcmTcpClient::CreateMsgHead(CMemoryStream &oStream, uint32 nMagic, uint32 nModule, uint32 nCmd)
{
	oStream.SetLocalCode(false);
	oStream.SetPosition(0);
	oStream.Truncate();
	oStream.Write((uint32)0);
	oStream.Write(nMagic);
	oStream.Write(nModule);
	oStream.Write(nCmd);
}

void CAcmTcpClient::BuildMsgHead(CMemoryStream &oStream)
{
	oStream.SetLocalCode(false);
	oStream.SetPosition(0);
	oStream.Write(oStream.GetSize());
}

bool CAcmTcpClient::Send(CMemoryStream& oStream, bool bHello)
{
	oStream.SetPosition(0);
	uint32 nSize = oStream.GetSize();
	m_oMutex.Enter();
	int32 nRet = m_oTcp.Write(oStream, (int32)nSize);
	if(nRet != (int32)nSize)
	{
		m_oTcp.Close();
		return false;
	}
	if(!bHello)
		m_bHello = false;
	m_oMutex.Leave();
	return true;
}

bool CAcmTcpClient::Recv(char* sMsg, uint32 nSize)
{
	while(nSize)
	{
		int32 nRet = m_oTcp.Read(sMsg, nSize);
		if(nRet <= 0)
		{
			m_oTcp.Close();
			break;
		}
		nSize -= nRet;
		sMsg += nRet;
	}
	if(nSize == 0)
	{
		m_bHello = false;
		return true;
	}
	return false;
}

void CAcmTcpClient::Connect(bool bReConnect)
{
	if(bReConnect)
	{
		if(!m_pContext->OnReConnect(this))
		{
			CCooperator::Sleep(1000);
			return;
		}
	}
	CFormatString oConnStr;
	oConnStr.Print("tcp://%s:%u16", m_oServerAddr.GetStr(), m_nServerPort);
	FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Start() is connecting to %s:%u16", m_oServerAddr.GetStr(), m_nServerPort));
	if(m_oTcp.Open(oConnStr.GetStr(), "rw"))
	{
		m_oTcp.Close();
		if(!m_bConnectFailure)
		{
			m_bConnectFailure = true;
			FocpLog(FOCP_LOG_ERROR, ("CAcmTcpClient::Start() is connecting to %s:%u16 failure", m_oServerAddr.GetStr(), m_nServerPort));
		}
	}
	else
	{
		m_bConnected = true;
		if(m_bConnectFailure)
		{
			m_bConnectFailure = false;
			FocpLog(FOCP_LOG_SYSLOG, ("CAcmTcpClient::Start() is connecting to %s:%u16 success", m_oServerAddr.GetStr(), m_nServerPort));
		}
	}
}

void CAcmTcpClient::Hello()
{
	char sMsg[16];
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	oStream.Write((uint32)sizeof(sMsg));
	oStream.Write(m_nMagic);
	oStream.Write((uint32)ACM_HELLO_MODULE);
	oStream.Write((uint32)TCP_HELLO_REQ);
	Send(sMsg, sizeof(sMsg), true);
}

uint32 CAcmTcpClient::Login(bool bFirst)
{
	char sMsg[20];
	CSyncCallee oCallee(NULL);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	uint32 nSessionId = ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16): system busy", m_oServerAddr.GetStr(), m_nServerPort));
		return 1;
	}
	oStream.Write((uint32)sizeof(sMsg));
	oStream.Write(m_nMagic);
	oStream.Write((uint32)ACM_HELLO_MODULE);
	oStream.Write((uint32)TCP_LOGIN_REQ);
	oStream.Write(nSessionId);
	if(!Send(sMsg, sizeof(sMsg)))
	{
		ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16) send error", m_oServerAddr.GetStr(), m_nServerPort));
		return 1;
	}
	switch(WaitCall(nSessionId, 2000))
	{
	case FOCP_SESSION_NORMAL:
		if(m_nStatus)
		{
			FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16) failure", m_oServerAddr.GetStr(), m_nServerPort));
			return 1;
		}
		break;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16) timeout", m_oServerAddr.GetStr(), m_nServerPort));
		return 1;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16) session break", m_oServerAddr.GetStr(), m_nServerPort));
		return 1;
	default:
		FocpLog(FOCP_LOG_WARNING, ("CAcmTcpClient::Login(%s:%u16): invalid session", m_oServerAddr.GetStr(), m_nServerPort));
		return 1;
	}
	uint32 nRet = m_pContext->OnLogin(this, !bFirst);
	if(!nRet)
		FinishInitialize();
	return nRet;
}

void CAcmTcpClient::FinishInitialize()
{
	char sMsg[16];
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	oStream.Write((uint32)sizeof(sMsg));
	oStream.Write(m_nMagic);
	oStream.Write((uint32)ACM_HELLO_MODULE);
	oStream.Write((uint32)TCP_INIT_FINISH);
	Send(sMsg, sizeof(sMsg));
}

void CAcmTcpClient::ReturnLogin(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(m_nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = QueryCall(nSessionId);
		if(pCallee)
		{
			pCallee->Answer();
			ReleaseCall(pCallee);
		}
	}
}

void CAcmTcpClient::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(pCooperator == &m_oHelloThread)
	{
		if(m_bFailure)
			CCooperator::Sleep(10);
		else
		{
			if(m_bLogin)
			{
				if(m_bHello)
					;//Hello();
				else
					m_bHello = true;
			}
			CCooperator::Sleep(1000);
		}
	}
	else if(pCooperator == &m_oConnThread)
	{
		if(m_bFailure)
			CCooperator::Sleep(10);
		else if(m_bConnected)
			CCooperator::Sleep(1000);
		else if(m_bLogin)
		{
			AnswerAll(FOCP_SESSION_BREAK);
			Connect(true);
			if(m_bConnected)
				Login(false);
			CCooperator::Sleep(2000);
		}
		else
		{
			uint32 i;
			for(i=0; i<10; ++i)
			{
				Connect(false);
				if(m_bConnected)
					break;
				CCooperator::Sleep(5000);
			}
			if(m_bConnected)
			{
				if(Login(true))
					m_bFailure = true;
				else
					m_bLogin = true;
			}
			else
				m_bFailure = true;
		}
	}
	else if(pCooperator == &m_oRecvThread)
	{
		CTcpHead oHead;
		char sMsg[4096], *pMsg=NULL;
		if(m_bFailure)
		{
			CCooperator::Sleep(10);
			return;
		}
		if(!m_bConnected)
		{
			if(m_bLogin)
				CCooperator::Sleep(1000);
			else
				CCooperator::Sleep(10);
			return;
		}
		if(!Recv((char*)&oHead, sizeof(oHead)))
		{
			m_bConnected = false;
			return;
		}
		oHead.nSize = CBinary::U32Code(oHead.nSize);
		oHead.nMagic = CBinary::U32Code(oHead.nMagic);
		oHead.nModule = CBinary::U32Code(oHead.nModule);
		oHead.nCmd = CBinary::U32Code(oHead.nCmd);

		if(oHead.nSize < sizeof(oHead))
		{
			m_oTcp.Close();
			m_bConnected = false;
			return;
		}
		uint32 nSize = oHead.nSize - sizeof(oHead);
		if(nSize <= 4096)
			pMsg = sMsg;
		else
		{
			uint32 nMod = nSize % 256;
			if(nMod)
				nSize += 256 - nMod;
			pMsg = new char[nSize];
		}
		if(nSize && !Recv(pMsg, nSize))
		{
			m_bConnected = false;
			if(pMsg != sMsg)
				delete[] pMsg;
			return;
		}
		if(oHead.nMagic == m_nMagic)
		{
			CMemoryStream oStream(pMsg, nSize);
			ProcessMsg(oHead, oStream);
		}
		if(pMsg != sMsg)
			delete[] pMsg;
	}
}

void CAcmTcpClient::ProcessMsg(CTcpHead& oHead, CMemoryStream &oStream)
{
	if(oHead.nModule == ACM_HELLO_MODULE)
	{
		if(oHead.nCmd == TCP_LOGIN_RESP)
			ReturnLogin(oStream);
	}
	else
		m_pContext->ProcessMsg(this, oHead, oStream);
}

const char* CAcmTcpClient::GetServerAddr()
{
	return m_oServerAddr.GetStr();
}

uint16 CAcmTcpClient::GetServerPort()
{
	return m_nServerPort;
}

uint32 CAcmTcpClient::GetMagic()
{
	return m_nMagic;
}

FOCP_END();
