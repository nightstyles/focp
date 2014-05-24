
#include "DataFlow.hpp"

FOCP_BEGIN();

FOCP_PRIVATE_BEGIN();

struct CExceptionData
{
	CRelayNode* pRelayNode;
	CPortNode* pPortNode;
};

class CDataFlowMonitor: public CProcessNode
{
private:
	CListNode m_oQueueNode;
	CRelayNode m_oEngine;

public:
	CDataFlowMonitor()
		:m_oQueueNode(true),m_oEngine(true)
	{
		m_oEngine.LinkFrom(&m_oQueueNode);
		m_oEngine.LinkTo(this);
		m_oEngine.Start();
	}

	virtual ~CDataFlowMonitor()
	{
		m_oEngine.Stop();
	}

	static void ThrowException(CExceptionData &oException)
	{
		static CDataFlowMonitor g_oDataFlowMonitor;
		CAny oAny(oException);
		g_oDataFlowMonitor.m_oQueueNode.PutData(oAny);
	}

protected:
	virtual bool ProcessData(CAny& oData)
	{
		CExceptionData& oException = (CExceptionData&)oData;
		oException.pPortNode->Mend(oException.pRelayNode);
		return false;
	}
};

FOCP_PRIVATE_END();

CDataFlowNode::CDataFlowNode()
{
	m_nLinkCounter = 0;
}

CDataFlowNode::~CDataFlowNode()
{
}

bool CDataFlowNode::Check()
{
	return true;
}

bool CDataFlowNode::HaveGetInterface()
{
	return false;
}

bool CDataFlowNode::HavePutInterface()
{
	return false;
}

bool CDataFlowNode::HaveActiveInterface()
{
	return false;
}

bool CDataFlowNode::CanLinkFrom()
{
	return false;
}

bool CDataFlowNode::CanLinkTo()
{
	return false;
}

bool CDataFlowNode::CanLinkToMore()
{
	return false;
}

bool CDataFlowNode::CanLinkToPort()
{
	return false;
}

uint32 CDataFlowNode::GetLinkCounter()
{
	return m_nLinkCounter;
}

void CDataFlowNode::UnLink()
{
}

void CDataFlowNode::AddLinkCounter()
{
	++m_nLinkCounter;
}

void CDataFlowNode::DelLinkCounter()
{
	--m_nLinkCounter;
}

uint32 CDataFlowNode::GetData(CAny& oData)
{
	return FOCP_DFLOW_BLOCKED;
}

uint32 CDataFlowNode::PutData(CAny& oData)
{
	return FOCP_DFLOW_BLOCKED;
}

void CDataFlowNode::Start()
{
}

void CDataFlowNode::Stop()
{
}

bool CDataFlowNode::LinkFrom(CDataFlowNode* pFrom)
{
	return false;
}

bool CDataFlowNode::LinkTo(CDataFlowNode* pTo)
{
	return false;
}

bool CDataFlowNode::LinkTo(CDataFlowNode* pTo, uint32 nDataType)
{
	return false;
}

bool CDataFlowNode::LinkToPort(void* pPort)
{
	return false;
}

CTrashBinNode::CTrashBinNode()
{
}

CTrashBinNode::~CTrashBinNode()
{
}

bool CTrashBinNode::HavePutInterface()
{
	return true;
}

uint32 CTrashBinNode::PutData(CAny& oData)
{
	return FOCP_DFLOW_SUCCESS;
}

CQueueNode::CQueueNode(uint32 nCapacity, bool bThread)
	:m_oMutex(bThread),m_oGetEvent(bThread),m_oPutEvent(bThread)
{
	if(nCapacity == 0)
		nCapacity = 1024;
	m_nCapacity = nCapacity;
	m_nHead = m_nTail = m_nSize = 0;
	m_pQueue = new CAny[m_nCapacity+1];
}

CQueueNode::~CQueueNode()
{
	delete[] m_pQueue;
}

bool CQueueNode::HaveGetInterface()
{
	return true;
}

bool CQueueNode::HavePutInterface()
{
	return true;
}

uint32 CQueueNode::GetData(CAny& oData)
{
	uint32 nRet = FOCP_DFLOW_BLOCKED;
	m_oGetEvent.Wait(1000);
	m_oMutex.Enter();
	if(m_nSize)
	{
		CAny& oItem = m_pQueue[m_nHead];
		oData.Swap(oItem);
		if(m_nSize == 1)
			m_oGetEvent.Reset();
		else if(m_nSize == m_nCapacity)
			m_oPutEvent.Set();
		m_nHead = (m_nHead+1)%m_nCapacity;
		nRet = FOCP_DFLOW_SUCCESS;
		--m_nSize;
	}
	m_oMutex.Leave();
	return nRet;
}

uint32 CQueueNode::PutData(CAny& oData)
{
	uint32 nRet = FOCP_DFLOW_BLOCKED;
	m_oPutEvent.Wait(1000);
	m_oMutex.Enter();
	if(m_nSize < m_nCapacity)
	{
		CAny& oItem = m_pQueue[m_nTail];
		oData.Swap(oItem);
		if(m_nSize == m_nCapacity - 1)
			m_oPutEvent.Reset();
		else if(m_nSize == 0)
			m_oGetEvent.Set();
		m_nTail = (m_nTail+1)%m_nCapacity;
		nRet = FOCP_DFLOW_SUCCESS;
		++m_nSize;
	}
	m_oMutex.Leave();
	return nRet;
}

FOCP_PRIVATE_BEGIN();

struct CListNodeItem
{
	CAny oData;
	CListNodeItem* pNext;
};

struct CListNodeContainer
{
	CMutex oMutex;
	CListNodeItem * pHead;

	CListNodeContainer()
	{
		pHead = NULL;
	}

	~CListNodeContainer()
	{
		while(pHead)
		{
			CListNodeItem* pNext = pHead->pNext;
			delete pHead;
			pHead = pNext;
		}
	}

	CListNodeItem* AllocateNode()
	{
		oMutex.Enter();
		CListNodeItem* pItem = pHead;
		if(pItem)
			pHead = pItem->pNext;
		oMutex.Leave();
		if(pItem == NULL)
			pItem = new CListNodeItem;
		return pItem;
	}

	void DeAllocateNode(CListNodeItem* pNode)
	{
		oMutex.Enter();
		pNode->pNext = pHead;
		pHead = pNode;
		oMutex.Leave();
	}
};

CListNodeContainer& GetNodeContainer()
{
	static CListNodeContainer g_oContainer;
	return g_oContainer;
}

FOCP_PRIVATE_END();

CListNode::CListNode(bool bThread)
	:m_oMutex(bThread),m_oGetEvent(bThread)
{
	m_pHead = m_pTail = NULL;
}

CListNode::~CListNode()
{
	CListNodeContainer& oContainer = GetNodeContainer();
	CListNodeItem* pHead = (CListNodeItem*)m_pHead;
	while(pHead)
	{
		CListNodeItem* pNext = pHead->pNext;
		oContainer.DeAllocateNode(pHead);
		pHead = pNext;
	}
	m_pHead = m_pTail = NULL;
}

bool CListNode::HaveGetInterface()
{
	return true;
}

bool CListNode::HavePutInterface()
{
	return true;
}

uint32 CListNode::GetData(CAny& oData)
{
	uint32 nRet = FOCP_DFLOW_BLOCKED;
	m_oGetEvent.Wait(1000);
	m_oMutex.Enter();
	if(m_pHead)
	{
		nRet = FOCP_DFLOW_SUCCESS;
		CListNodeItem* pNode = (CListNodeItem*)m_pHead;
		pNode->oData.Swap(oData);
		m_pHead = pNode->pNext;
		if(m_pHead == NULL)
		{
			m_pTail = NULL;
			m_oGetEvent.Reset();
		}
		GetNodeContainer().DeAllocateNode(pNode);
	}
	m_oMutex.Leave();
	return nRet;
}

uint32 CListNode::PutData(CAny& oData)
{
	CListNodeItem* pNode = GetNodeContainer().AllocateNode();
	pNode->oData.Swap(oData);
	m_oMutex.Enter();
	if(m_pTail)
		((CListNodeItem*)m_pTail)->pNext = pNode;
	else
	{
		m_pHead = pNode;
		m_oGetEvent.Set();
	}
	m_oMutex.Leave();
	return FOCP_DFLOW_SUCCESS;
}

CSwitchNode::CSwitchNode(FGetDataType fGetDataType)
{
	m_fGetDataType = fGetDataType;
}

CSwitchNode::~CSwitchNode()
{
}

bool CSwitchNode::HavePutInterface()
{
	return true;
}

uint32 CSwitchNode::PutData(CAny& oData)
{
	uint32 nType = m_fGetDataType?m_fGetDataType(oData):oData.GetType();
	CRbTreeNode* pIt = m_oDispTable.Find(nType);
	if(pIt == m_oDispTable.End())
		return FOCP_DFLOW_SORRY;
	CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
	return pNode->PutData(oData);
}

bool CSwitchNode::CanLinkToMore()
{
	return true;
}

bool CSwitchNode::LinkTo(CDataFlowNode* pTo, uint32 nDataType)
{
	if(!nDataType || !pTo || !pTo->HavePutInterface())
		return false;
	if(m_oDispTable.Find(nDataType) != m_oDispTable.End())
		return false;
	m_oDispTable[nDataType] = pTo;
	pTo->AddLinkCounter();
	return true;
}

void CSwitchNode::UnLink()
{
	CRbTreeNode* pIt = m_oDispTable.First();
	CRbTreeNode* pEnd = m_oDispTable.End();
	for(; pIt != pEnd; pIt = m_oDispTable.GetNext(pIt))
	{
		CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
		pNode->DelLinkCounter();
	}
	m_oDispTable.Clear();
}

CDispatchNode::CDispatchNode(bool bThread, FGetDataType fGetDataType)
	:m_oMutex(bThread)
{
	m_fGetDataType = fGetDataType;
	m_bStarted = false;
}

CDispatchNode::~CDispatchNode()
{
}

bool CDispatchNode::HavePutInterface()
{
	return true;
}

uint32 CDispatchNode::PutData(CAny& oData)
{
	uint32 nType = m_fGetDataType?m_fGetDataType(oData):oData.GetType();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oDispTable.Find(nType);
	if(pIt == m_oDispTable.End())
	{
		m_oMutex.Leave();
		return FOCP_DFLOW_SORRY;
	}
	CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
	uint32 nRet = pNode->PutData(oData);
	m_oMutex.Leave();
	return nRet;
}

bool CDispatchNode::CanLinkToMore()
{
	return true;
}

bool CDispatchNode::LinkTo(CDataFlowNode* pTo, uint32 nDataType)
{
	if(!nDataType)
		return false;
	if(pTo)
	{
		if(!pTo->HavePutInterface() || pTo->GetLinkCounter() || !pTo->Check())
			return false;
		m_oMutex.Enter();
		if(m_oDispTable.Find(nDataType) != m_oDispTable.End())
		{
			m_oMutex.Leave();
			return false;
		}
		m_oDispTable[nDataType] = pTo;
		pTo->AddLinkCounter();
		if(m_bStarted)
			pTo->Start();
		m_oMutex.Leave();
	}
	else
	{
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oDispTable.Find(nDataType);
		if(pIt == m_oDispTable.End())
			m_oMutex.Leave();
		else
		{
			CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
			m_oDispTable.Remove(pIt);
			m_oMutex.Leave();
			pNode->Stop();
			pNode->DelLinkCounter();
			pNode->UnLink();
			delete pNode;
		}
	}
	return true;
}

void CDispatchNode::UnLink()
{
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oDispTable.First();
	CRbTreeNode* pEnd = m_oDispTable.End();
	for(; pIt != pEnd; pIt = m_oDispTable.GetNext(pIt))
	{
		CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
		pNode->DelLinkCounter();
	}
	m_oDispTable.Clear();
	m_oMutex.Leave();
}

bool CDispatchNode::HaveActiveInterface()
{
	return true;
}

void CDispatchNode::Start()
{
	m_oMutex.Enter();
	if(m_bStarted == false)
	{
		m_bStarted = true;
		CRbTreeNode* pIt = m_oDispTable.First();
		CRbTreeNode* pEnd = m_oDispTable.End();
		for(; pIt != pEnd; pIt = m_oDispTable.GetNext(pIt))
		{
			CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
			pNode->Start();
		}
	}
	m_oMutex.Leave();
}

void CDispatchNode::Stop()
{
	m_oMutex.Enter();
	if(m_bStarted)
	{
		CRbTreeNode* pIt = m_oDispTable.First();
		CRbTreeNode* pEnd = m_oDispTable.End();
		for(; pIt != pEnd; pIt = m_oDispTable.GetNext(pIt))
		{
			CDataFlowNode* pNode = m_oDispTable.GetItem(pIt);
			pNode->Stop();
		}
		m_bStarted = false;
	}
	m_oMutex.Leave();
}

CRelayNode::CRelayNode(bool bThread)
	:m_oCooperator(this, bThread), m_hEvent(bThread)
{
	m_pFrom = NULL;
	m_pTo = NULL;
	m_bStarted = false;
}

CRelayNode::~CRelayNode()
{
}

bool CRelayNode::Check()
{
	return m_pFrom!=NULL && m_pTo!=NULL;
}

bool CRelayNode::CanLinkFrom()
{
	return true;
}

bool CRelayNode::LinkFrom(CDataFlowNode* pFrom)
{
	if(m_pFrom || !pFrom || !pFrom->HaveGetInterface())
		return false;
	m_pFrom = pFrom;
	m_pFrom->AddLinkCounter();
	return true;
}

bool CRelayNode::CanLinkTo()
{
	return true;
}

bool CRelayNode::LinkTo(CDataFlowNode* pTo)
{
	if(m_pTo || !pTo || !pTo->HavePutInterface())
		return false;
	m_pTo = pTo;
	pTo->AddLinkCounter();
	return true;
}

void CRelayNode::UnLink()
{
	if(m_pFrom)
		m_pFrom->DelLinkCounter();
	if(m_pTo)
		m_pTo->DelLinkCounter();
}

bool CRelayNode::HaveActiveInterface()
{
	return true;
}

void CRelayNode::Start()
{
	if(m_bStarted == false)
	{
		m_oCooperator.Start();
		m_bStarted = true;
	}
}

void CRelayNode::Stop()
{
	if(m_bStarted)
	{
		m_oCooperator.Stop(false);
		m_hEvent.Set();
		m_oCooperator.Stop();
		m_bStarted = false;
	}
}

void CRelayNode::Resume()
{
	m_hEvent.Set();
}

void CRelayNode::MainProc(CCooperator* pCooperator, bool &bRunning)
{
	uint32 nRet;
	CAny oData;
	while(bRunning)
	{
		try
		{
			nRet = m_pFrom->GetData(oData);
			if(nRet == 0)
			{
				nRet = m_pTo->PutData(oData);
				if(nRet)
					FocpWarn(("CRelayNode::SendData Failure(%u)", nRet));
			}
		}
		catch(CPortNode* pNode)
		{
			m_hEvent.Reset();
			CExceptionData oException = {this, pNode};
			CDataFlowMonitor::ThrowException(oException);
			while(bRunning)
				m_hEvent.Wait(1000);
		}
	}
}

CProcessNode::CProcessNode()
{
	m_pTo = NULL;
}

CProcessNode::~CProcessNode()
{
}

bool CProcessNode::CanLinkTo()
{
	return true;
}

bool CProcessNode::LinkTo(CDataFlowNode* pTo)
{
	if(m_pTo || !pTo || !pTo->HavePutInterface())
		return false;
	m_pTo = pTo;
	pTo->AddLinkCounter();
	return true;
}

void CProcessNode::UnLink()
{
	if(m_pTo)
		m_pTo->DelLinkCounter();
}

bool CProcessNode::HavePutInterface()
{
	return true;
}

bool CProcessNode::Check()
{
	return m_pTo!=NULL;
}

uint32 CProcessNode::PutData(CAny& oData)
{
	if(ProcessData(oData))
		return m_pTo->PutData(oData);
	return FOCP_DFLOW_SUCCESS;
}

void CProcessNode::ForwardData(CAny& oData)
{
	m_pTo->PutData(oData);
}

bool CProcessNode::ProcessData(CAny& oData)
{
	return false;
}

CDataFlow::CDataFlow()
{
	m_pPutNode = NULL;
	m_pGetNode = NULL;
	m_bHaveActiveInterface = false;
	m_bStarted = false;
}

CDataFlow::~CDataFlow()
{
}

bool CDataFlow::LinkFrom(CDataFlowNode &oNode, CDataFlowNode &oFrom)
{
	if(!oNode.CanLinkFrom() || !oFrom.HaveGetInterface())
		return false;
	if(!oNode.LinkFrom(&oFrom))
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	if(FindNode(&oFrom) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oFrom);
	if(oNode.HaveActiveInterface() || oFrom.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::LinkTo(CDataFlowNode &oNode, CDataFlowNode &oTo)
{
	if(!oNode.CanLinkTo() || !oTo.HavePutInterface())
		return false;
	if(!oNode.LinkTo(&oTo))
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	if(FindNode(&oTo) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oTo);
	if(oNode.HaveActiveInterface() || oTo.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::LinkTo(CDataFlowNode &oNode, CDataFlowNode &oTo, uint32 nDataType)
{
	if(!oNode.CanLinkToMore() || !oTo.HavePutInterface())
		return false;
	if(!oNode.LinkTo(&oTo, nDataType))
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	if(FindNode(&oTo) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oTo);
	if(oNode.HaveActiveInterface() || oTo.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::LinkToPort(CDataFlowNode &oNode, void* pPort)
{
	if(pPort == NULL)
		return false;
	if(!oNode.CanLinkToPort())
		return false;
	if(!oNode.LinkToPort(pPort))
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	if(oNode.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::MakePutNode(CDataFlowNode &oNode)
{
	if(!oNode.HavePutInterface())
		return false;
	if(m_pPutNode)
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	m_pPutNode = &oNode;
	if(oNode.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::MakeGetNode(CDataFlowNode &oNode)
{
	if(!oNode.HaveGetInterface())
		return false;
	if(m_pGetNode)
		return false;
	if(FindNode(&oNode) >= m_oNodes.GetSize())
		m_oNodes.Insert(m_oNodes.GetSize(), &oNode);
	m_pGetNode = &oNode;
	if(oNode.HaveActiveInterface())
		m_bHaveActiveInterface = true;
	return true;
}

bool CDataFlow::Check()
{
	uint32 nSize = m_oNodes.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CDataFlowNode* pNode = m_oNodes[i];
		if(!pNode->Check())
			return false;
	}
	return true;
}

bool CDataFlow::HaveGetInterface()
{
	return m_pGetNode != NULL;
}

bool CDataFlow::HavePutInterface()
{
	return m_pPutNode != NULL;
}

bool CDataFlow::HaveActiveInterface()
{
	return m_bHaveActiveInterface;
}

uint32 CDataFlow::PutData(CAny& oData)
{
	if(m_pPutNode == NULL)
		return FOCP_DFLOW_SORRY;
	return m_pPutNode->PutData(oData);
}

uint32 CDataFlow::GetData(CAny& oData)
{
	if(m_pGetNode == NULL)
		return FOCP_DFLOW_SORRY;
	return m_pGetNode->GetData(oData);
}

void CDataFlow::Start()
{
	if(m_bHaveActiveInterface && !m_bStarted)
	{
		uint32 i, nSize = m_oNodes.GetSize();
		for(i=0; i<nSize; ++i)
		{
			CDataFlowNode* pNode = m_oNodes[i];
			pNode->Start();
		}
		m_bStarted = true;
	}
}

void CDataFlow::Stop()
{
	if(m_bHaveActiveInterface && m_bStarted)
	{
		uint32 i, nSize = m_oNodes.GetSize();
		for(i=0; i<nSize; ++i)
		{
			CDataFlowNode* pNode = m_oNodes[i];
			pNode->Stop();
		}
		m_bStarted = false;
	}
}

uint32 CDataFlow::FindNode(CDataFlowNode* pNode)
{
	uint32 i, nSize = m_oNodes.GetSize();
	for(i=0; i<nSize; ++i)
	{
		CDataFlowNode* pNode2 = m_oNodes[i];
		if(pNode2 == pNode)
			return i;
	}
	return 0xFFFFFFFF;
}

void CDataFlow::UnLink()
{
	uint32 nSize = m_oNodes.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		m_oNodes[i]->UnLink();
	m_oNodes.Clear();
	m_pPutNode = NULL;
	m_pGetNode = NULL;
	m_bHaveActiveInterface = false;
}

CPortFunctor::CPortFunctor()
{
}

CPortFunctor::~CPortFunctor()
{
}

void CPortFunctor::Mend(CRelayNode* pRelayNode, CPortNode* pPortNode)
{
}

uint32 CPortFunctor::Read(void* pPort, CAny& oData)
{
	return FOCP_DPORT_BLOCKED;
}

uint32 CPortFunctor::Write(void* pPort, CAny& oData)
{
	return FOCP_DPORT_BLOCKED;
}

CPortNode::CPortNode(CPortFunctor* pFunctor)
{
	m_pFunctor = pFunctor;
	m_pPort = NULL;
	m_bExceptional = true;
}

CPortNode::~CPortNode()
{
}

bool CPortNode::CanLinkToPort()
{
	return true;
}

bool CPortNode::LinkToPort(void* pPort)
{
	if(pPort == NULL || !m_bExceptional)
		return false;
	m_pPort = pPort;
	m_bExceptional = false;
	return true;
}

bool CPortNode::Check()
{
	return m_pPort != NULL;
}

bool CPortNode::HaveGetInterface()
{
	return m_pFunctor!=NULL;
}

bool CPortNode::HavePutInterface()
{
	return m_pFunctor!=NULL;
}

uint32 CPortNode::GetData(CAny& oData)
{
	uint32 nRet;
	if(m_bExceptional)
		FocpAbort(("CPortNode::GetData() failure"));
	switch(m_pFunctor->Read(m_pPort, oData))
	{
	case FOCP_DPORT_SUCCESS:
		nRet = FOCP_DFLOW_SUCCESS;
		break;
	case FOCP_DPORT_BLOCKED:
		nRet = FOCP_DFLOW_BLOCKED;
		break;
	default:
		FocpAbort(("CPortNode::GetData() failure"));
		break;
	}
	return nRet;
}

uint32 CPortNode::PutData(CAny& oData)
{
	uint32 nRet;
	if(m_bExceptional)
		FocpAbort(("CPortNode::PutData() failure"));
	switch(m_pFunctor->Write(m_pPort, oData))
	{
	case FOCP_DPORT_SUCCESS:
		nRet = FOCP_DFLOW_SUCCESS;
		break;
	case FOCP_DPORT_BLOCKED:
		nRet = FOCP_DFLOW_BLOCKED;
		break;
	default:
		FocpAbort(("CPortNode::PutData() failure"));
		break;
	}
	return nRet;
}

bool CPortNode::IsExceptional()
{
	return m_bExceptional;
}

void* CPortNode::GetPort()
{
	return m_pPort;
}

void CPortNode::Mend(CRelayNode* pRelayNode)
{
	m_pFunctor->Mend(pRelayNode, this);
}

FOCP_END();
