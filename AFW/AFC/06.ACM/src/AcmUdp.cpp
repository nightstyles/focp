
#include "AcmUdp.hpp"

FOCP_BEGIN();

void* CUdpMsg::operator new(uintptr nSize)//port to 64bit
{
	return CBufferManager::GetInstance()->AllocateBuffer(FOCP_SIZE_OF(CUdpMsg));
}

void CUdpMsg::operator delete(void* pMsg)
{
	CBufferManager::GetInstance()->DeAllocateBuffer(pMsg);
}

CUdpMsg::CUdpMsg()
{
	nSize = 0;
}

CUdpMsg::CUdpMsg(const CUdpMsg& oSrc)
{
	nSize = oSrc.nSize;
	oHead = oSrc.oHead;
	if(nSize > ACM_UDP_HEADSIZE)
		CBinary::MemoryCopy(sBody, oSrc.sBody, nSize - ACM_UDP_HEADSIZE);
}

CUdpMsg& CUdpMsg::operator=(const CUdpMsg& oSrc)
{
	if(this != &oSrc)
	{
		nSize = oSrc.nSize;
		oHead = oSrc.oHead;
		if(nSize > ACM_UDP_HEADSIZE)
			CBinary::MemoryCopy(sBody, oSrc.sBody, nSize - ACM_UDP_HEADSIZE);
	}
	return *this;
}

CAcmUdpModule::CAcmUdpModule()
{
}

CAcmUdpModule::~CAcmUdpModule()
{
}

FOCP_PRIVATE_BEGIN();
CMutex g_oMutex;
CRbMap<uint32, CAcmUdp*> g_oUdpTable;
FOCP_PRIVATE_END();

CAcmContext::CAcmContext()
{
}

CAcmContext::~CAcmContext()
{
}

void CAcmContext::OnNodeRepeat(CAcmUdp* pUdp)
{
}

void CAcmContext::OnAddNode(CAcmUdp* pUdp, uint32 nNode, uint32 nIp, uint16 nPort)
{
}

void CAcmContext::OnWalk(uint32 nNode, const char* sAddr)
{
}

CAcmUdp::CAcmUdp(CAcmContext* pContext, uint32 nDomain, uint32 nNode, bool bMultiCast, bool bReg)
	:m_oRecvThread(this), m_oProcThread(this), m_oRecvThread2(this)
{
	m_bRegistered = bReg;
	if(m_bRegistered)
	{
		g_oMutex.Enter();
		CRbTreeNode* pIt = g_oUdpTable.Find(nDomain);
		if(pIt != g_oUdpTable.End())
			FocpAbort(("CAcmUdp::CAcmUdp(%u), the domain is registered", nDomain));
		g_oUdpTable[nDomain] = this;
		g_oMutex.Leave();
	}
	m_bMultiCast = bMultiCast;
	m_bChecked = false;
	m_pContext = pContext;
	m_nPid = GetPid();
	m_nMultiCastAddr = (uint32)(-1);
	m_nItfAddr = (uint32)(-1);
	m_nDomain = nDomain;
	m_nNode = nNode;
	m_nUniCastPort = 0;
	m_nPort = 0;
}

CAcmUdp::~CAcmUdp()
{
	if(m_bRegistered)
	{
		g_oMutex.Enter();
		g_oUdpTable.Remove(m_nDomain);
		g_oMutex.Leave();
	}
}

void CAcmUdp::GetAllDomains(CVector<uint32> &oDomains)
{
	oDomains.Clear();
	CRbTreeNode* pEnd = g_oUdpTable.End();
	g_oMutex.Enter();
	CRbTreeNode* pIt = g_oUdpTable.First();
	for(uint32 nIdx=0; pIt!=pEnd; pIt=g_oUdpTable.GetNext(pIt))
	{
		uint32 nDomain = g_oUdpTable.GetKey(pIt);
		oDomains.Insert(nIdx, nDomain);
		++nIdx;
	}
	g_oMutex.Leave();
}

CAcmUdp* CAcmUdp::QueryUdp(uint32 nDomain)
{
	CAcmUdp* pUdp = NULL;
	g_oMutex.Enter();
	CRbTreeNode* pIt = g_oUdpTable.Find(nDomain);
	if(pIt != g_oUdpTable.End())
		pUdp = g_oUdpTable.GetItem(pIt);
	g_oMutex.Leave();
	return pUdp;
}

uint32 CAcmUdp::GetDomain()
{
	return m_nDomain;
}

uint32 CAcmUdp::GetNode()
{
	return m_nNode;
}

bool CAcmUdp::AddNode(uint32 nNode, const char* sIp, uint16 nPort)
{
	if(!sIp)
		return false;
	if(m_bMultiCast)
		nPort = 0;
	CIpAddrList oAddrList;
	CFile::GetIpAddrList(sIp, oAddrList);
	if(!oAddrList.oAddrList.GetSize())
		return false;
	return AddNode(nNode, oAddrList.oAddrList[0], nPort);
}

bool CAcmUdp::AddNode(uint32 nNode, uint32 nIp, uint16 nPort)
{
	if(nNode==(uint32)(-1))
		return false;
	if(CFile::IsMulticastAddr(nIp))
		return false;
	if(nIp == 0)
		return false;
	if(nPort == 0 && m_nPort)
		nPort = m_nPort;
	m_oMutex.Enter();
	if(nNode == m_nNode)
	{
		if(m_nItfAddr != (uint32)(-1))
		{
			m_oMutex.Leave();
			return false;
		}
		if(nPort != m_nPort)
			m_nUniCastPort = nPort;
		m_nItfAddr = nIp;
	}
	else
	{
		CRbTreeNode* pIt = m_oNodeList.Find(nNode);
		if(pIt != m_oNodeList.End())
		{
			m_oMutex.Leave();
			return false;
		}
		CIpAddr &oNode = m_oNodeList[nNode];
		oNode.nAddr = nIp;
		oNode.nPort = nPort;
		m_pContext->OnAddNode(this, nNode, nIp, nPort);
	}
	m_oMutex.Leave();
	return true;
}

void CAcmUdp::DelNode(uint32 nNode)
{
	m_oMutex.Enter();
	m_oNodeList.Remove(nNode);
	m_oMutex.Leave();
}

bool CAcmUdp::GetNodeAddr(uint32 nNode, CIpAddr &oAddr)
{
	oAddr.nAddr = -1;
	oAddr.nPort = m_nPort;
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodeList.Find(nNode);
	if(pIt != m_oNodeList.End())
	{
		CIpAddr & oNode = m_oNodeList.GetItem(pIt);
		oAddr.nAddr = oNode.nAddr;
		oAddr.nPort = oNode.nPort;
	}
	else if(nNode == m_nNode)
	{
		oAddr.nAddr = m_nItfAddr;
		if(m_bMultiCast && m_nUniCastPort)
			oAddr.nPort = m_nUniCastPort;
	}
	m_oMutex.Leave();

	return oAddr.nAddr != (uint32)(-1);
}

void CAcmUdp::Walk()
{
	CIpAddr oItfAddr;
	CString oFileName;
	GetInterfaceAddr(oItfAddr);
	CFile::GetIpFileName(oItfAddr, oFileName);
	m_pContext->OnWalk(m_nNode, oFileName.GetStr());
	CRbTreeNode* pEnd = m_oNodeList.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodeList.First();
	for(; pIt!=pEnd; pIt=m_oNodeList.GetNext(pIt))
	{
		uint32 nNode = m_oNodeList.GetKey(pIt);
		CIpAddr& oAddr = m_oNodeList.GetItem(pIt);
		CFile::GetIpFileName(oAddr, oFileName);
		m_pContext->OnWalk(nNode, oFileName.GetStr());
	}
	m_oMutex.Leave();
}

bool CAcmUdp::GetInterfaceAddr(CIpAddr &oAddr)
{
	oAddr.nAddr = m_nItfAddr;
	oAddr.nPort = m_nPort;
	if(m_bMultiCast &&m_nUniCastPort)
		oAddr.nPort = m_nUniCastPort;
	return oAddr.nAddr != (uint32)(-1);
}

uint32 CAcmUdp::GetMultiCastAddr(CIpAddr &oAddr)
{
	oAddr.nAddr = m_nMultiCastAddr;
	oAddr.nPort = m_nPort;
	return m_nTTL;
}

void CAcmUdp::RegisterModule(uint32 nModule, CAcmUdpModule* pModule)
{
	if(!m_oModules.Register(nModule,pModule))
		FocpAbort(("CAcmUdp::RegisterModule(%u) failure", nModule));
}

void CAcmUdp::DeRegisterModule(uint32 nModule)
{
	m_oModules.DeRegister(nModule);
}

CAcmUdpModule* CAcmUdp::QueryModule(uint32 nModule)
{
	return m_oModules.Query(nModule);
}

void CAcmUdp::ReleaseModule(uint32 nModule)
{
	m_oModules.Release(nModule);
}

void CAcmUdp::Send(CUdpMsg &oMsg, uint32 nNode)
{
	if(oMsg.nSize >= ACM_MAX_UDP)
		return;

	oMsg.oHead.nNode = CBinary::U32Code(m_nNode);
	oMsg.oHead.nPid = CBinary::U32Code(m_nPid);
	oMsg.oHead.nModule = CBinary::U16Code(oMsg.oHead.nModule);
	oMsg.oHead.nCmd = CBinary::U16Code(oMsg.oHead.nCmd);

	if(nNode == (uint32)(-1))
	{//组播
		if(m_bMultiCast)
		{//组播
			CIpAddr oMultiCastAddr = {m_nMultiCastAddr, m_nPort};
			m_oFile.WriteTo(&oMsg.oHead, oMsg.nSize, oMultiCastAddr);
		}
		else
		{//单播模拟组播
			CIpAddr oIpAddr;
			CRbTreeNode* pEnd = m_oNodeList.End();
			m_oMutex.Enter();
			CRbTreeNode* pIt = m_oNodeList.First();
			m_oMutex.Leave();
			oIpAddr.nPort = m_nPort;
			while(pIt!=pEnd)
			{
				CIpAddr & oNode = m_oNodeList.GetItem(pIt);
				oIpAddr.nAddr = oNode.nAddr;
				if(oNode.nPort)
					oIpAddr.nPort = oNode.nPort;
				m_oFile.WriteTo(&oMsg.oHead, oMsg.nSize, oIpAddr);
				m_oMutex.Enter();
				pIt = m_oNodeList.GetNext(pIt);
				m_oMutex.Leave();
			}
		}
	}

	else if(nNode != m_nNode)
	{//单播
		CIpAddr oAddr;
		if(GetNodeAddr(nNode, oAddr))
			m_oFile.WriteTo(&oMsg.oHead, oMsg.nSize, oAddr);
	}

	//还原
	oMsg.oHead.nModule = CBinary::U16Code(oMsg.oHead.nModule);
	oMsg.oHead.nCmd = CBinary::U16Code(oMsg.oHead.nCmd);
}

void CAcmUdp::ProcessMessage(CUdpMsg* &pMsg)
{
	uint32 nModule = pMsg->oHead.nModule;
	CAcmUdpModule* pModule = QueryModule(nModule);
	if(pModule)
	{
		pModule->ProcessAcmModuleMsg(this, pMsg);
		ReleaseModule(nModule);
	}
}

bool CAcmUdp::IsMultiCast()
{
	return m_bMultiCast;
}

bool CAcmUdp::IsGood()
{
	return (m_oFile.GetStatus() == FOCP_FILE_NORMAL);
}
CAcmContext* CAcmUdp::GetContext()
{
	return m_pContext;
}

bool CAcmUdp::InitUniCast(uint16 nPort)
{
	if(m_bMultiCast)
		return false;

	if(!nPort)
		return false;

	m_nPort = nPort;

	CFileName oFileName;
	oFileName.oProtocol = "udp";
	CIpAddr oBindAddr = {0, m_nPort};//INADDR_ANY
	CFile::GetIpFileName(oBindAddr, oFileName.oBindName);

	if(m_oFile.Open(oFileName, "rw"))
		return false;

	return true;
}

bool CAcmUdp::InitMultiCast(uint16 nPort, const char* sMultiCastAddr, uint32 nTTL, bool bAllowLoop)
{
	m_nTTL = nTTL;

	if(!m_bMultiCast)
		return false;

	if(!nPort)
		return false;
	m_nPort = nPort;

	if(!CFile::IsMulticastAddr(sMultiCastAddr))
		return false;

	if(m_nItfAddr == (uint32)(-1))
		return false;

	CFileName oFileName;
	oFileName.oProtocol = "udp";
	CIpAddr oBindAddr = {m_nItfAddr, 0};
	CFile::GetIpFileName(oBindAddr,oFileName.oBindName);
	CStringFormatter oFormater(&oFileName.oConnectName);
	oFormater.Print("%s:%u", sMultiCastAddr, m_nPort);

	CFormatString oOption;
	oOption.Print("rw");
	if(nTTL > 1)
		oOption.Print("t:%u", nTTL);
	if(bAllowLoop)
		oOption.Print("m");

	if(m_oFile.Open(oFileName, oOption.GetStr()))
		return false;

	CIpAddrList oAddrList;
	CFile::GetIpAddrList(m_oFile.GetFileName().oConnectName.GetStr(), oAddrList);
	m_nMultiCastAddr = oAddrList.oAddrList[0];

	if(m_nUniCastPort)
	{
		CFileName oFileName;
		oFileName.oProtocol = "udp";
		CIpAddr oBindAddr = {0, m_nUniCastPort};//INADDR_ANY
		CFile::GetIpFileName(oBindAddr, oFileName.oBindName);

		if(m_oUniCastFile.Open(oFileName, "rw"))
			return false;
	}

	return true;
}

bool CAcmUdp::InitMultiCast(uint16 nPort, uint32 nMultiCastAddr, uint32 nTTL, bool bAllowLoop)
{
	m_nTTL = nTTL;

	if(!m_bMultiCast)
		return false;

	if(!nPort)
		return false;
	m_nPort = nPort;

	if(!CFile::IsMulticastAddr(nMultiCastAddr))
		return false;

	if(m_nItfAddr == (uint32)(-1))
		return false;

	CFileName oFileName;
	oFileName.oProtocol = "udp";
	CIpAddr oBindAddr = {m_nItfAddr, 0};
	CFile::GetIpFileName(oBindAddr,oFileName.oBindName);
	CIpAddr oConnAddr = {nMultiCastAddr, m_nPort};
	CFile::GetIpFileName(oConnAddr,oFileName.oConnectName);

	CFormatString oOption;
	oOption.Print("rw");
	if(nTTL > 1)
		oOption.Print("t:%u", nTTL);
	if(bAllowLoop)
		oOption.Print("m");

	if(m_oFile.Open(oFileName, oOption.GetStr()))
		return false;

	m_nMultiCastAddr = nMultiCastAddr;

	if(m_nUniCastPort)
	{
		CFileName oFileName;
		oFileName.oProtocol = "udp";
		CIpAddr oBindAddr = {0, m_nUniCastPort};//INADDR_ANY
		CFile::GetIpFileName(oBindAddr, oFileName.oBindName);

		if(m_oUniCastFile.Open(oFileName, "rw"))
			return false;
	}

	return true;
}

namespace
{
union CLoopBackAddr
{
	uint8 x[4];
	uint32 nAddr;
	CLoopBackAddr()
	{
		x[0] = 127;
		x[1] = x[2] = 0;
		x[2] = 1;
	}
} oLoopBackAddr;
}

bool CAcmUdp::RecvMsg(CFile& oFile)
{
	CFileName oFrom;
	CUdpMsg* pMsg = new CUdpMsg;
	pMsg->nSize = m_oFile.ReadFrom(&pMsg->oHead, ACM_MAX_UDP, oFrom);
	if((int32)pMsg->nSize <= 0)
	{
		delete pMsg;
		return false;
	}
	if(pMsg->nSize >= ACM_UDP_HEADSIZE && pMsg->oHead.nNode != 0xFFFFFFFF)
	{
		pMsg->oHead.nNode = CBinary::U32Code(pMsg->oHead.nNode);
		pMsg->oHead.nPid = CBinary::U32Code(pMsg->oHead.nPid);
		if(m_nNode == pMsg->oHead.nNode)
		{
			if(!m_bChecked)
			{
				if(m_bMultiCast)
				{
					CIpAddrList oAddrList;
					CFile::GetIpAddrList(oFrom.oConnectName.GetStr(), oAddrList);
					uint32 nFromAddr = oAddrList.oAddrList[0];
					if(!(nFromAddr==oLoopBackAddr.nAddr || CFile::CheckHostIp(nFromAddr)) ||
						pMsg->oHead.nPid != m_nPid)
					{ //组播有回环的概念，可能收到自己发出去的消息。
						//不同机器或同一机器不同进程的节点配置重复。
						m_oMutex.Enter();
						if(!m_bChecked)
						{
							m_pContext->OnNodeRepeat(this);
							m_bChecked = true;
						}
						m_oMutex.Leave();
					}
				}
				else
				{//单播情况，单播没有回环，收到相同节点的消息，即冲突
					m_oMutex.Enter();
					if(!m_bChecked)
					{
						m_pContext->OnNodeRepeat(this);
						m_bChecked = true;
					}
					m_oMutex.Leave();
				}
			}
		}
		else
		{
			pMsg->oHead.nModule = CBinary::U16Code(pMsg->oHead.nModule);
			pMsg->oHead.nCmd = CBinary::U16Code(pMsg->oHead.nCmd);
			m_oMutex.Enter();
			if(!m_oMsgQueue.GetSize())
				m_oEvent.Set();
			if(m_oMsgQueue.Push(pMsg))
				pMsg = NULL;
			else
			{
				CUdpMsg* pMsg2;
				m_oMsgQueue.Pop(pMsg2);
				m_oMsgQueue.Push(pMsg);
				pMsg = pMsg2;
			}
			m_oMutex.Leave();
		}
	}
	if(pMsg)
		delete pMsg;
	return true;
}

void CAcmUdp::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(pCooperator == &m_oRecvThread)
	{
		if(!RecvMsg(m_oFile))
			bRunning = false;
	}
	else if(pCooperator == &m_oRecvThread2)
	{
		if(!RecvMsg(m_oUniCastFile))
			bRunning = false;
	}
	else
	{
		CUdpMsg* pMsg = NULL;
		m_oEvent.Wait(1000);
		m_oMutex.Enter();
		if(m_oMsgQueue.Pop(pMsg))
		{
			m_oMutex.Leave();
			ProcessMessage(pMsg);
			if(pMsg)
				delete pMsg;
		}
		else
		{
			m_oEvent.Reset();
			m_oMutex.Leave();
		}
	}
}

bool CAcmUdp::Start()
{
	if(m_oFile.GetStatus())
		return false;
	m_oProcThread.Start();
	m_oRecvThread.Start();
	if(m_bMultiCast &&m_nUniCastPort)
		m_oRecvThread2.Start();
	return true;
}

void CAcmUdp::Stop(bool bBlock)
{
	m_oFile.Close();
	m_oUniCastFile.Close();

	m_oRecvThread.Stop(false);
	if(m_bMultiCast &&m_nUniCastPort)
		m_oRecvThread2.Stop(false);
	m_oProcThread.Stop(false);
	if(bBlock)
	{
		m_oRecvThread.Stop();
		if(m_bMultiCast &&m_nUniCastPort)
			m_oRecvThread2.Stop();
		m_oProcThread.Stop();
	}
}

FOCP_END();
