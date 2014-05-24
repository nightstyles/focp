
#include "AcmSequence.hpp"
#include "AcmTcp.hpp"

#include <time.h>

FOCP_BEGIN();

enum
{
	ASM_REGISTER_MSG,
	ASM_DEREGISTER_MSG,
	ASM_DATA_MSG,
	ASM_RESEND_MSG,
	ASM_ACK_MSG,
	ASM_REDATA_MSG,
	ASM_RESEND_MAX_PACK = (ACM_MAX_UDP-ACM_UDP_HEADSIZE-sizeof(uint32))/sizeof(uint32),
};

#ifndef ASM_SYSPLUGIN_OP
#define ASM_SYSPLUGIN_OP
enum//系统插件操作定义
{
	ASM_SYSPLUGIN_OP_STREAM = 0,	//Common Stream
	ASM_SYSPLUGIN_OP_BIGPACK = 1,	//大包
};
#endif

struct CAsmRegisterMsg
{
	uint32 nTime;
	uint32 nHost;//不能转为本地序列
	uint16 nPort;//对于单播必须要
	uint16 nTcpPort;//TcpPort, Extend Para;
	uint8 bUniCast;//0=多播；1=单播；
	uint8 nStatus;//0=初始状态；1=服务状态；
};

struct CAsmDeRegisterMsg
{
};

struct CAsmResendReq
{
	uint32 nCount;
	uint32 nSeqs[ASM_RESEND_MAX_PACK];
};

struct CAsmAckMsg
{
	uint32 nNodeId;
	uint32 nSequence;
};

CAsmMsg::~CAsmMsg()
{
	if(m_pUdpMsg)
		delete m_pUdpMsg;
}

CAsmMsg::CAsmMsg(bool bNew)
{
	if(bNew)
		m_pUdpMsg = new CUdpMsg;
	else
		m_pUdpMsg = NULL;
}

CAsmMsg::CAsmMsg(const CAsmMsg& oSrc)
{
	if(oSrc.m_pUdpMsg)
		m_pUdpMsg = new CUdpMsg(*oSrc.m_pUdpMsg);
	else
		m_pUdpMsg = NULL;
}

CAsmMsg& CAsmMsg::operator=(const CAsmMsg& oSrc)
{
	if(this != &oSrc)
	{
		if(oSrc.m_pUdpMsg)
		{
			if(m_pUdpMsg == NULL)
				m_pUdpMsg = new CUdpMsg(*oSrc.m_pUdpMsg);
			else
				*m_pUdpMsg = *oSrc.m_pUdpMsg;
		}
		else
		{
			if(m_pUdpMsg)
				delete m_pUdpMsg;
			m_pUdpMsg = NULL;
		}
	}
	return *this;
}

CAsmMsg::CAsmMsg(CUdpMsg* pUdpMsg)
{
	m_pUdpMsg = pUdpMsg;
}

CUdpMsg* CAsmMsg::GetUdpMsg(bool bDetah)
{
	CUdpMsg* pRet = m_pUdpMsg;
	if(bDetah)
		m_pUdpMsg = NULL;
	return pRet;
}

uint32 CAsmMsg::GetNode()
{
	return m_pUdpMsg->oHead.nNode;
}

uint32 CAsmMsg::GetSequence()
{
	return ((CAsmDataMsg*)m_pUdpMsg)->oHead.nSequence;
}

void CAsmMsg::SetSequence(uint32 nSeq)
{
	((CAsmDataMsg*)m_pUdpMsg)->oHead.nSequence = nSeq;
}

CAsmMsg& CAsmMsg::Swap(CAsmMsg& oSrc)
{
	if(this != &oSrc)
	{
		CUdpMsg* pRet = m_pUdpMsg;
		m_pUdpMsg = oSrc.m_pUdpMsg;
		oSrc.m_pUdpMsg = pRet;
	}
	return *this;
}

CAsmPlugIn::CAsmPlugIn()
{
}

CAsmPlugIn::~CAsmPlugIn()
{
}

void CAsmPlugIn::Clear(uint32 nNode)
{
}

CAsmNodeWalker::CAsmNodeWalker()
{
}

CAsmNodeWalker::~CAsmNodeWalker()
{
}

void CAsmNodeWalker::OnWalk(uint32 nDomain, uint32 nNode, const CAsmNode& oNode)
{
}

CAcmSequenceModule::~CAcmSequenceModule()
{
	m_pUdp->DeRegisterModule(ACM_SERIAL_MODULE);
	if(m_pSendQueue)
	{
		delete m_pSendQueue;
		m_pSendQueue = NULL;
	}
	if(m_pRecvQueue)
	{
		delete m_pRecvQueue;
		m_pRecvQueue = NULL;
	}
	if(m_pHoldQueue)
	{
		delete m_pHoldQueue;
		m_pHoldQueue = NULL;
	}
}

CAcmSequenceModule::CAcmSequenceModule(CAcmUdp* pUdp, uint32 nMode, uint32 nCapacity, uint8 nMaxAckTimes)
	:m_oRegThread(this), m_oCheckThread(this), m_oResendThread(this), m_oHoldThread(this)
{
	m_nStatus = 0;
	m_pUdp = pUdp;
	m_bIsMultiCast = m_pUdp->IsMultiCast();
	if(!(nMode & (ASM_SUPPORT_SEND|ASM_SUPPORT_RECV)))
		nMode |= ASM_SUPPORT_SEND|ASM_SUPPORT_RECV;
	if(m_bIsMultiCast)
		nMode &= ~ASM_SUPPORT_ACK;
	if(nMode & ASM_SUPPORT_RECV)
	{
		m_pRecvQueue = new CAsmRecvQueue(this, nCapacity, nMaxAckTimes);
		m_pHoldQueue = new CAsmHoldQueue;
	}
	else
	{
		m_pRecvQueue = NULL;
		m_pHoldQueue = NULL;
	}
	if(nMode & ASM_SUPPORT_SEND)
		m_pSendQueue = new CAsmSendQueue(nCapacity, (nMode&ASM_SUPPORT_ACK)?true:false);
	else
		m_pSendQueue = NULL;
	m_pUdp->RegisterModule(ACM_SERIAL_MODULE, this);
}

bool CAcmSequenceModule::SupportSend()
{
	return m_pSendQueue!=NULL;
}

bool CAcmSequenceModule::SupportRecv()
{
	return m_pRecvQueue!=NULL;
}

bool CAcmSequenceModule::SupportMultiCastSend()
{
	return m_pSendQueue && (!m_pSendQueue->NeedAck());
}

bool CAcmSequenceModule::SupportUniCastSend()
{
	return m_pSendQueue && m_pSendQueue->NeedAck();
}

uint32 CAcmSequenceModule::GetRecvCapacity(uint32 &nSize)
{
	if(m_pRecvQueue)
		return m_pRecvQueue->GetCapacity(nSize);
	nSize = 0;
	return 0;
}

uint32 CAcmSequenceModule::GetSendCapacity(uint32 &nSize)
{
	if(m_pSendQueue)
		return m_pSendQueue->GetCapacity(nSize);
	nSize = 0;
	return 0;
}

CAcmUdp* CAcmSequenceModule::GetUdp()
{
	return m_pUdp;
}

uint32 CAcmSequenceModule::QueryServiceNode(CAsmNode& oNode)
{
	uint32 nNode = (uint32)-1;
	CRbTreeNode* pEnd = m_oNodes.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodes.First();
	for(; pIt!=pEnd; pIt=m_oNodes.GetNext(pIt))
	{
		CAsmNode &oItem = m_oNodes.GetItem(pIt);
		if(oItem.bOnLine && oItem.bServering)
		{
			nNode = m_oNodes.GetKey(pIt);
			oNode = oItem;
			break;
		}
	}
	m_oMutex.Leave();
	if(nNode == (uint32)(-1))
	{
		CIpAddr oAddr;
		m_pUdp->GetInterfaceAddr(oAddr);
		oNode.nTime = m_nStartTime;
		oNode.nPort = oAddr.nPort;
		oNode.nTcpPort = CAcmTcpServer::GetInstance()->GetPort();
		oNode.bServering = m_nStatus;
		oNode.bOnLine = 1;
		oNode.nCounter = 0;
		nNode = m_pUdp->GetNode();
	}
	return nNode;
}

void CAcmSequenceModule::Walk(CAsmNodeWalker &oWalker)
{
	CAsmNode oNode2;
	CIpAddr oAddr;
	m_pUdp->GetInterfaceAddr(oAddr);
	oNode2.nTime = m_nStartTime;
	oNode2.nPort = oAddr.nPort;
	oNode2.nTcpPort = CAcmTcpServer::GetInstance()->GetPort();
	oNode2.bServering = m_nStatus;
	oNode2.bOnLine = 1;
	oNode2.nCounter = 0;
	uint32 nDomain = m_pUdp->GetDomain();
	oWalker.OnWalk(nDomain, m_pUdp->GetNode(), oNode2);
	CRbTreeNode* pEnd = m_oNodes.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodes.First();
	for(; pIt!=pEnd; pIt=m_oNodes.GetNext(pIt))
	{
		uint32 nNode = m_oNodes.GetKey(pIt);
		CAsmNode&oNode = m_oNodes.GetItem(pIt);
		oWalker.OnWalk(nDomain, nNode, oNode);
	}
	m_oMutex.Leave();
}

void CAcmSequenceModule::ActiveService()
{
	m_nStatus = 1;
}

bool CAcmSequenceModule::IsActive()
{
	return (m_nStatus == 1);
}

bool CAcmSequenceModule::Start(uint32 nRegTimer, uint32 nCheckTimer, uint8 nBreakHeartBeat, uint32 nExpireHeartBeat, uint32 nResendTimer)
{
	CCooperator::Sleep(1000);//以防重启太快
	m_nStartTime = (uint32)time(NULL);
	if(!nRegTimer)
		nRegTimer = 1000;
	if(!nCheckTimer)
		nCheckTimer = 2000;
	if(!nBreakHeartBeat)
		nBreakHeartBeat = 20;
	if(!nExpireHeartBeat)
		nExpireHeartBeat = 200;
	if(!nResendTimer)
		nResendTimer = 1000;
	m_nBreakHeartBeat = nBreakHeartBeat;
	m_nExpireHeartBeat = nExpireHeartBeat;
	m_nRegTimer = nRegTimer;
	m_nCheckTimer = nCheckTimer;
	m_nResendTimer = nResendTimer;
	m_oRegThread.Start();
	m_oCheckThread.Start();
	if(m_pRecvQueue)
	{
		m_oResendThread.Start();
		m_oHoldThread.Start();
	}
	return true;
}

void CAcmSequenceModule::Stop(bool bBlock)
{
	char sMsg[32];

	m_oRegThread.Stop(bBlock);
	if(bBlock)
	{
		CUdpMsg* pMsg = (CUdpMsg*)sMsg;
		pMsg->nSize = ACM_UDP_HEADSIZE;
		pMsg->oHead.nModule = 0;
		pMsg->oHead.nCmd = ASM_DEREGISTER_MSG;
		m_pUdp->Send(*pMsg);
	}
	m_oCheckThread.Stop(bBlock);
	if(m_pRecvQueue)
	{
		m_oResendThread.Stop(bBlock);
		m_oHoldThread.Stop(bBlock);
	}
}

void CAcmSequenceModule::RegisterPlugIn(uint32 nPlugIn, CAsmPlugIn* pPlugIn)
{
	if(!m_oPlugIns.Register(nPlugIn, pPlugIn))
		FocpAbort(("CAcmSequenceModule::RegisterPlugIn(%u) failure", nPlugIn));
}

void CAcmSequenceModule::DeRegisterPlugIn(uint32 nPlugIn)
{
	m_oPlugIns.DeRegister(nPlugIn);
}

CAsmPlugIn* CAcmSequenceModule::QueryPlugIn(uint32 nPlugIn)
{
	return m_oPlugIns.Query(nPlugIn);
}

void CAcmSequenceModule::ReleasePlugIn(uint32 nPlugIn)
{
	m_oPlugIns.Release(nPlugIn);
}

void CAcmSequenceModule::ReSend(uint32 nNode, uint32 nSeq)
{
	if(m_pSendQueue == NULL)
		return;
	CAsmMsg oAsmMsg(true);
	CAsmDataMsg& oMsg = *(CAsmDataMsg*)oAsmMsg.GetUdpMsg();
	if(!m_pSendQueue->Query(oAsmMsg, nSeq, nNode))
	{
		oMsg.oHead.nSequence = nSeq;
		oMsg.nSize = ACM_UDP_HEADSIZE + 4;
	}
	else
	{
		oMsg.oHead.nPlugIn = CBinary::U16Code(oMsg.oHead.nPlugIn);
		oMsg.oHead.nOp = CBinary::U16Code(oMsg.oHead.nOp);
	}
	oMsg.oHead.nModule = 0;
	oMsg.oHead.nCmd = ASM_REDATA_MSG;
	oMsg.oHead.nSequence = CBinary::U32Code(oMsg.oHead.nSequence);
	m_pUdp->Send(*(CUdpMsg*)&oMsg, nNode);
}

FOCP_PRIVATE_BEGIN();
enum
{
	ASM_STREAM_UNITSIZE = ACM_MAX_UDP - ASM_UDP_HEADSIZE - 2,
	ASM_MAX_STREAMSIZE = 65536 * ASM_STREAM_UNITSIZE,
};
FOCP_PRIVATE_END();

bool CAcmSequenceModule::Send2(CAsmDataMsg &oMsg, uint32 nNode)
{
	CAsmMsg oAsmMsg((CUdpMsg*)&oMsg);
	m_oSendMutex.Enter();
	bool bRet = m_pSendQueue->Send(oAsmMsg, nNode);
	m_oSendMutex.Leave();
	if(bRet)
	{
		oMsg.oHead.nModule = 0;
		oMsg.oHead.nCmd = ASM_DATA_MSG;
		oMsg.oHead.nSequence = CBinary::U32Code(oMsg.oHead.nSequence);
		oMsg.oHead.nPlugIn = CBinary::U16Code(oMsg.oHead.nPlugIn);
		oMsg.oHead.nOp = CBinary::U16Code(oMsg.oHead.nOp);
		m_pUdp->Send(*(CUdpMsg*)&oMsg, nNode);
		oMsg.oHead.nPlugIn = CBinary::U16Code(oMsg.oHead.nPlugIn);
		oMsg.oHead.nOp = CBinary::U16Code(oMsg.oHead.nOp);
	}
	oAsmMsg.GetUdpMsg(true);
	return bRet;
}

bool CAcmSequenceModule::Send3(CAsmDataMsg &oMsg, uint32 nNode)
{
	uint32 nMaxSize = ASM_MAX_STREAMSIZE + ASM_UDP_HEADSIZE;
	if(oMsg.nSize > nMaxSize)
		return false;
	CAsmDataMsg oMsg2;
	oMsg2.oHead = oMsg.oHead;
	oMsg2.oHead.nPlugIn = 0;
	oMsg2.oHead.nOp = CBinary::U16Code(ASM_SYSPLUGIN_OP_BIGPACK);
	oMsg2.oHead.nModule = 0;
	oMsg2.oHead.nCmd = ASM_DATA_MSG;
	uint8* pShift = (uint8*)&oMsg.oHead.nPlugIn;
	uint32 nSize = oMsg.nSize - ASM_UDP_HEADSIZE + 4;
	uint16 &nPackNo = *(uint16*)oMsg2.sBody;
	uint8* sCopy = oMsg2.sBody + 2;
	m_oSendMutex.Enter();
	for(uint16 i=0; nSize; ++i)
	{
		uint32 nCopySize = ASM_STREAM_UNITSIZE;
		if(nCopySize > nSize)
			nCopySize = nSize;
		CBinary::MemoryCopy(sCopy, pShift, nCopySize);
		nSize -= nCopySize;
		pShift += nCopySize;
		nPackNo = CBinary::U16Code(i);
		CAsmMsg oAsmMsg((CUdpMsg*)&oMsg2);
		m_pSendQueue->Send(oAsmMsg, nNode, true);
		oMsg2.oHead.nSequence = CBinary::U32Code(oMsg2.oHead.nSequence);
		m_pUdp->Send(*(CUdpMsg*)&oMsg2, nNode);
		oAsmMsg.GetUdpMsg(true);
	}
	m_oSendMutex.Leave();
	return true;
}

bool CAcmSequenceModule::Send4(CAsmDataMsg &oMsg, uint32 nNode)
{
	if(m_pSendQueue == NULL)
		return false;
	if(nNode == (uint32)(-1))
	{
		if(m_pSendQueue->NeedAck())
			return false;
	}
	else if(!m_pSendQueue->NeedAck())
		return false;
	if(oMsg.oHead.nPlugIn == 0)
		return false;//阻止应用发流包，流由系统自行决定。
	if(oMsg.nSize <= ACM_MAX_UDP)
		return Send2(oMsg, nNode);
	return Send3(oMsg, nNode);
}

//发送普通数据，如果一个包想分多次发送，请指示pPackNo
bool CAcmSequenceModule::Send5(uint8* pStream, uint32 nStreamSize, uint32 nNode, uint16 *pPackNo)
{
	if(m_pSendQueue == NULL)
		return false;
	if(nNode == (uint32)(-1))
	{
		if(m_pSendQueue->NeedAck())
			return false;
	}
	else if(!m_pSendQueue->NeedAck())
		return false;

	uint16 i = 0;
	if(i)
		i = *pPackNo;
	uint32 nPackNum = nStreamSize / ASM_STREAM_UNITSIZE;
	if(nStreamSize % ASM_STREAM_UNITSIZE)
		++nPackNum;
	if(nPackNum > 65536)
		return false;
	nPackNum += i;
	if(nPackNum > 65536)
		return false;
	CAsmDataMsg oMsg;
	oMsg.oHead.nPlugIn = 0;
	oMsg.oHead.nOp = ASM_SYSPLUGIN_OP_STREAM;
	oMsg.oHead.nModule = 0;
	oMsg.oHead.nCmd = ASM_DATA_MSG;
	uint8* pShift = pStream;
	uint32 nSize = nStreamSize;
	uint16 &nPackNo = *(uint16*)oMsg.sBody;
	uint8* sCopy = oMsg.sBody + 2;
	m_oSendMutex.Enter();
	for(; nSize; ++i)
	{
		uint32 nCopySize = ASM_STREAM_UNITSIZE;
		if(nCopySize > nSize)
			nCopySize = nSize;
		CBinary::MemoryCopy(sCopy, pShift, nCopySize);
		nSize -= nCopySize;
		pShift += nCopySize;
		nPackNo = CBinary::U16Code(i);
		CAsmMsg oAsmMsg((CUdpMsg*)&oMsg);
		m_pSendQueue->Send(oAsmMsg, nNode, true);
		oMsg.oHead.nSequence = CBinary::U32Code(oMsg.oHead.nSequence);
		m_pUdp->Send(*(CUdpMsg*)&oMsg, nNode);
		oAsmMsg.GetUdpMsg(true);
	}
	m_oSendMutex.Leave();
	if(pPackNo)
		*pPackNo = i;
	return true;
}

bool CAcmSequenceModule::Send(CAsmDataMsg &oMsg, uint32 nNode)
{
	if(m_pSendQueue == NULL)
		return false;
	if(nNode == (uint32)(-1))
	{
		if(m_pSendQueue->NeedAck())
			return false;
	}
	else if(!m_pSendQueue->NeedAck())
		return false;
	if(oMsg.oHead.nPlugIn == 0)
		return false;//阻止应用发流包，流由系统自行决定。
	if(oMsg.nSize <= ACM_MAX_UDP)
		return Send2(oMsg, nNode);
	return false;
}

void CAcmSequenceModule::ProcessAcmModuleMsg(CAcmUdp* pUdp, CUdpMsg* &pMsg)
{
	uint32 nNode = pMsg->oHead.nNode;
	switch(pMsg->oHead.nCmd)
	{
	case ASM_REGISTER_MSG:
		if(pMsg->nSize < ACM_UDP_HEADSIZE + 14)
			break;
		ProcessRegister(nNode, pMsg->sBody);
		break;
	case ASM_DEREGISTER_MSG:
		ProcessDeRegister(nNode);
		break;
	case ASM_DATA_MSG:
		ActiveNode(nNode);
		if(m_pRecvQueue == NULL)
			break;
		if(pMsg->nSize == ACM_UDP_HEADSIZE + 4)
		{
			CAsmDataMsg& oData = *(CAsmDataMsg*)pMsg;
			oData.oHead.nSequence = CBinary::U32Code(oData.oHead.nSequence);
			m_pRecvQueue->PutInNull(nNode, oData.oHead.nSequence);
		}
		else if(pMsg->nSize < ASM_UDP_HEADSIZE)
			break;
		else
		{
			CAsmMsg oAsmMsg(pMsg);
			CAsmDataMsg& oData = *(CAsmDataMsg*)pMsg;
			oData.oHead.nSequence = CBinary::U32Code(oData.oHead.nSequence);
			m_pRecvQueue->PutIn(oAsmMsg);
			pMsg = oAsmMsg.GetUdpMsg(true);
		}
		break;
	case ASM_REDATA_MSG:
		ActiveNode(nNode);
		if(m_pRecvQueue == NULL)
			break;
		if(pMsg->nSize == ACM_UDP_HEADSIZE + 4)
		{//重传的空消息
			CAsmDataMsg& oData = *(CAsmDataMsg*)pMsg;
			oData.oHead.nSequence = CBinary::U32Code(oData.oHead.nSequence);
			m_pRecvQueue->PutInNack(nNode, oData.oHead.nSequence);
		}
		else if(pMsg->nSize < ASM_UDP_HEADSIZE)
			break;
		else
		{
			CAsmMsg oAsmMsg(pMsg);
			CAsmDataMsg& oData = *(CAsmDataMsg*)pMsg;
			oData.oHead.nSequence = CBinary::U32Code(oData.oHead.nSequence);
			m_pRecvQueue->PutInNack(oAsmMsg);
			pMsg = oAsmMsg.GetUdpMsg(true);
		}
		break;
	case ASM_RESEND_MSG:
	{
		ActiveNode(nNode);
		if(pMsg->nSize < ACM_UDP_HEADSIZE+4)
			break;
		CAsmResendReq* pReq = (CAsmResendReq*)pMsg->sBody;
		uint32 nCount = CBinary::U32Code(pReq->nCount);
		if(pMsg->nSize < ACM_UDP_HEADSIZE+(nCount+1)*4)
			break;
		for(uint32 i=0; i<nCount; ++i)
		{
			uint32 nSeq = CBinary::U32Code(pReq->nSeqs[i]);
			ReSend(nNode, nSeq);
		}
		break;
	}
	case ASM_ACK_MSG:
	{
		ActiveNode(nNode);
		if(pMsg->nSize < ACM_UDP_HEADSIZE+8)
			break;
		if(m_pSendQueue)
		{
			CAsmAckMsg* pAck = (CAsmAckMsg*)pMsg->sBody;
			uint32 nNodeId = CBinary::U32Code(pAck->nNodeId);
			uint32 nSequence = CBinary::U32Code(pAck->nSequence);
			m_pSendQueue->Ack(nSequence, nNodeId);
		}
	}
	}
}

void CAcmSequenceModule::ProcessRegister(uint32 nNode, uint8* sBody)
{
	CAsmRegisterMsg * pRegMsg = (CAsmRegisterMsg*)sBody;
	if(pRegMsg->bUniCast == m_bIsMultiCast)
		return;
	pRegMsg->nTime = CBinary::U32Code(pRegMsg->nTime);
	pRegMsg->nPort = CBinary::U16Code(pRegMsg->nPort);
	pRegMsg->nTcpPort = CBinary::U16Code(pRegMsg->nTcpPort);
	m_pUdp->AddNode(nNode, pRegMsg->nHost, pRegMsg->nPort);
	if(m_pSendQueue)
		m_pSendQueue->CreateWindow(nNode);
	if(m_pRecvQueue)
		m_pRecvQueue->CreateWindow(nNode);
	bool bOnLine = false;
	CRbTreeNode* pEnd = m_oNodes.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodes.Find(nNode);
	if(pIt == pEnd)
	{
		CAsmNode& oNode = m_oNodes[nNode];
		oNode.bOnLine = 1;
		oNode.bServering = pRegMsg->nStatus;
		oNode.nCounter = 0;
		oNode.nTime = pRegMsg->nTime;
		oNode.nPort = pRegMsg->nPort;
		oNode.nTcpPort = pRegMsg->nTcpPort;
		bOnLine = true;
	}
	else
	{
		CAsmNode& oNode = m_oNodes.GetItem(pIt);
		if(oNode.bOnLine == 0)
			bOnLine = true;
		if(oNode.nTime != pRegMsg->nTime)
		{
			ProcessDeRegister(nNode);
			oNode.nTime = pRegMsg->nTime;
		}
		oNode.bOnLine = 1;
		oNode.bServering = pRegMsg->nStatus;
		oNode.nPort = pRegMsg->nPort;
		oNode.nTcpPort = pRegMsg->nTcpPort;
		oNode.nCounter = 0;
	}
	m_oMutex.Leave();
	if(bOnLine)
		OnNodeChange(nNode, true);
}

void CAcmSequenceModule::ClearNode(CAsmPlugIn* pPlugIn, void * pPara)
{
	pPlugIn->Clear(*(uint32*)pPara);
}

void CAcmSequenceModule::ProcessDeRegister(uint32 nNode)
{
	if(m_pRecvQueue)
		m_pRecvQueue->Clear(nNode);
	if(m_pSendQueue)
		m_pSendQueue->Clear(nNode);
	CRbTreeNode* pEnd = m_oNodes.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodes.Find(nNode);
	if(pIt != pEnd)
	{
		CAsmNode& oNode = m_oNodes.GetItem(pIt);
		oNode.bOnLine = 0;
		oNode.bServering = 0;
		oNode.nCounter = 0;
	}
	m_oMutex.Leave();
	m_oPlugIns.Walk(&nNode, ClearNode);
}

void CAcmSequenceModule::OnRequestMsg(uint32 nNode, uint32 nCount, uint32 *pSeq)
{
	CUdpMsg oMsg;
	oMsg.oHead.nModule = 0;
	oMsg.oHead.nCmd = ASM_RESEND_MSG;
	CAsmResendReq* pReq = (CAsmResendReq*)oMsg.sBody;
	pReq->nCount = CBinary::U32Code(nCount);
	for(uint32 i=0; i<nCount; ++i)
		pReq->nSeqs[i] = CBinary::U32Code(pSeq[i]);
	oMsg.nSize = ACM_UDP_HEADSIZE + (nCount + 1) * sizeof(uint32);
	m_pUdp->Send(oMsg, nNode);
}

void CAcmSequenceModule::OnLostMsg(uint32 nNode, uint32 nSeq)
{
	FocpWarn(("Missing the message '%u' from the node '%u'",nSeq, nNode));
}

void CAcmSequenceModule::OnClearRecvWindow(uint32 nNode)
{
	FocpWarn(("Clear the recv window of the node '%u'",nNode));
}

void CAcmSequenceModule::OnNodeChange(uint32 nNode, bool bOnLine)
{
	if(bOnLine)
		FocpWarn(("The node '%u' is online",nNode));
	else
		FocpWarn(("The node '%u' is offline",nNode));
}

void CAcmSequenceModule::OnMoveRecvWindow(uint32 nNode, uint32 nSeq)
{
	uint8 sMsg[32];
	CUdpMsg* pMsg = (CUdpMsg*)sMsg;
	pMsg->oHead.nModule = 0;
	pMsg->oHead.nCmd = ASM_ACK_MSG;
	CAsmAckMsg* pAck = (CAsmAckMsg*)pMsg->sBody;
	pAck->nNodeId = CBinary::U32Code(nNode);
	pAck->nSequence = CBinary::U32Code(nSeq);
	pMsg->nSize = ACM_UDP_HEADSIZE + sizeof(CAsmAckMsg);
	m_pUdp->Send(*pMsg, nNode);
}

void CAcmSequenceModule::OnMessage(CAsmMsg& oMsg)
{
	m_oMutex.Enter();
	uint32 nSize = m_pHoldQueue->GetSize();
	if(m_nStatus == 0 || nSize)
	{
		CUdpMsg* pMsg = oMsg.GetUdpMsg(true);
		m_pHoldQueue->Push(pMsg);
		if(nSize == 0)
			m_oEvent.Set();
		m_oMutex.Leave();
	}
	else
	{
		m_oMutex.Leave();
		ProcessMessage(oMsg);
	}
}

void CAcmSequenceModule::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(pCooperator == &m_oRegThread)
	{
		uint8 sMsg[64], sNull[64];
		CIpAddr oAddr;
		m_pUdp->GetInterfaceAddr(oAddr);
		CUdpMsg* pMsg = (CUdpMsg*)sMsg;
		CAsmRegisterMsg* pReg = (CAsmRegisterMsg*)pMsg->sBody;
		pMsg->oHead.nModule = 0;
		pMsg->oHead.nCmd = ASM_REGISTER_MSG;
		pReg->nTime = CBinary::U32Code(m_nStartTime);
		pReg->nHost = oAddr.nAddr;
		pReg->nPort = CBinary::U16Code(oAddr.nPort);
		pReg->nTcpPort = CBinary::U16Code(CAcmTcpServer::GetInstance()->GetPort());
		pReg->bUniCast = m_bIsMultiCast?0:1;
		pMsg->nSize = ACM_UDP_HEADSIZE + 14;

		CUdpMsg* pNullMsg = (CUdpMsg*)sNull;
		bool bNeedAck = true;
		if(m_pSendQueue)
		{
			bNeedAck = m_pSendQueue->NeedAck();
			if(!bNeedAck)
			{
				pNullMsg->oHead.nModule = 0;
				pNullMsg->oHead.nCmd = ASM_DATA_MSG;
				pNullMsg->nSize = ACM_UDP_HEADSIZE + 4;
			}
		}

		while(bRunning)
		{
			//发送注册消息，并同时更新状态
			pReg->nStatus = m_nStatus;
			m_pUdp->Send(*pMsg);

			//发空消息，以弥补收端确认机制的不足。
			if(!bNeedAck)
			{
				CAsmMsg oAsmMsg(pNullMsg);
				m_oSendMutex.Enter();
				bool bRet = m_pSendQueue->Send(oAsmMsg);
				m_oSendMutex.Leave();
				if(bRet)
				{
					CAsmDataMsg* pDataMsg = (CAsmDataMsg*)sNull;
					pDataMsg->oHead.nSequence = CBinary::U32Code(pDataMsg->oHead.nSequence);
					m_pUdp->Send(*pNullMsg);
				}
				oAsmMsg.GetUdpMsg(true);
			}

			CCooperator::Sleep(m_nRegTimer);
		}
	}
	else if(pCooperator == &m_oCheckThread)
	{
		CCooperator::Sleep(m_nCheckTimer);
		CRbTreeNode* pEnd = m_oNodes.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oNodes.First();
		m_oMutex.Leave();
		while(pIt != pEnd)
		{
			bool bOffLine = false;
			uint32 nNode = -1;
			m_oMutex.Enter();
			CAsmNode &oNode = m_oNodes.GetItem(pIt);
			++oNode.nCounter;
			if(oNode.bOnLine)
			{
				if(oNode.nCounter >= m_nBreakHeartBeat)
				{
					oNode.bOnLine = 0;
					nNode = m_oNodes.GetKey(pIt);
				}
				pIt = m_oNodes.GetNext(pIt);
			}
			else
			{
				if(oNode.nCounter >= m_nExpireHeartBeat)
				{
					bOffLine = true;
					nNode = m_oNodes.GetKey(pIt);
					CRbTreeNode* pNext = m_oNodes.GetNext(pIt);
					m_oNodes.Remove(pIt);
					pIt = pNext;
				}
				else
					pIt = m_oNodes.GetNext(pIt);
			}
			m_oMutex.Leave();
			if(nNode != (uint32)(-1))
			{
				if(bOffLine)
				{
					if(m_pRecvQueue)
						m_pRecvQueue->RemoveWindow(nNode);
					if(m_pSendQueue)
						m_pSendQueue->RemoveWindow(nNode);
					m_pUdp->DelNode(nNode);
				}
				OnNodeChange(nNode, false);
			}
		}
	}
	else if(pCooperator == &m_oResendThread)
	{
		CCooperator::Sleep(m_nResendTimer);
		if(m_pRecvQueue)
		{
			uint32 nSeqs[ASM_RESEND_MAX_PACK];
			m_pRecvQueue->RequireResendAll(ASM_RESEND_MAX_PACK, nSeqs);
		}
	}
	else if(pCooperator == &m_oHoldThread)
	{
		CUdpMsg* pMsg = NULL;
		m_oEvent.Wait(1000);
		m_oMutex.Enter();
		if(!m_pHoldQueue->Pop(pMsg))
		{
			if(m_nStatus)
				bRunning = false;
			else
				m_oEvent.Reset();
		}
		m_oMutex.Leave();
		if(pMsg)
		{
			CAsmMsg oMsg(pMsg);
			ProcessMessage(oMsg);
		}
	}
}

void CAcmSequenceModule::ActiveNode(uint32 nNode)
{
	bool bOnLine = false;
	CRbTreeNode* pEnd = m_oNodes.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oNodes.Find(nNode);
	if(pIt == pEnd)
	{
		m_oMutex.Leave();
		return;
	}
	CAsmNode& oNode = m_oNodes.GetItem(pIt);
	if(oNode.bOnLine == 0)
		bOnLine = true;
	oNode.bOnLine = 1;
	oNode.nCounter = 0;
	m_oMutex.Leave();
	if(bOnLine)
		OnNodeChange(nNode, true);
}

void CAcmSequenceModule::ProcessMessage(CAsmMsg& oMsg)
{
	CAsmDataMsg* pMsg = (CAsmDataMsg*)oMsg.GetUdpMsg();
	pMsg->oHead.nPlugIn = CBinary::U16Code(pMsg->oHead.nPlugIn);
	pMsg->oHead.nOp = CBinary::U16Code(pMsg->oHead.nOp);
	ProcessMessage(*pMsg);
}

void CAcmSequenceModule::ProcessMessage(CAsmDataMsg& oMsg)
{
	uint32 nPlugIn = oMsg.oHead.nPlugIn;
	CAsmPlugIn* pPlugIn = QueryPlugIn(nPlugIn);
	if(pPlugIn)
	{
		pPlugIn->ProcessAsmPlugInMsg(this, oMsg);
		ReleasePlugIn(nPlugIn);
	}
}

FOCP_END();
