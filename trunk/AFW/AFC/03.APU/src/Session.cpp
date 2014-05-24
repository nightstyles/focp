
#include "Session.hpp"
#include "Timer.hpp"

FOCP_BEGIN();

static uint32 g_nBitMask[] =
{
	0x1, 0x3, 0x7, 0xF,
	0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF,
	0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
	0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
	0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
	0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
	0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

CSyncCallee::CSyncCallee(void* pCallObject, bool bThread): m_oEvent(false, bThread), m_oLock(false, bThread)
{
	m_pObject = pCallObject;
}

CSyncCallee::~CSyncCallee()
{
}

void* CSyncCallee::QueryCallObject()
{
	return m_pObject;
}

void CSyncCallee::Answer()
{
	m_nStatus = FOCP_SESSION_NORMAL;
	m_oEvent.Set();
}

CSyncCaller::CSyncCaller()
{
	m_nSessionId = 0;
}

CSyncCaller::~CSyncCaller()
{
}

uint32 CSyncCaller::ApplyCall(CSyncCallee * pObject)
{
	uint32 nSessionId;

	CRbTreeNode* pEnd = m_oObjects.End();

	pObject->m_oEvent.Reset();
	pObject->m_oLock.Set();
	pObject->m_nLocked = 0;
	pObject->m_nStatus = FOCP_SESSION_TIMEOUT;

	m_oMutex.Enter();
	nSessionId = m_nSessionId;
	do
	{
		++nSessionId;
		if(!nSessionId)
			++nSessionId;
		if(nSessionId == m_nSessionId)
		{
			nSessionId = 0;
			break;
		}
	}while(m_oObjects.Find(nSessionId) != pEnd);
	if(nSessionId)
	{
		m_oObjects[nSessionId] = pObject;
		m_nSessionId = nSessionId;
	}
	m_oMutex.Leave();

	return nSessionId;
}

uint32 CSyncCaller::WaitCall(uint32 nSessionId, uint32 nTimeOut)
{
	uint32 nStatus = FOCP_SESSION_INVALID;

	if(nSessionId == 0)
		return nStatus;

	CSyncCallee * pObject = NULL;
	CRbTreeNode* pEnd = m_oObjects.End();

	m_oMutex.Enter();
	CRbTreeNode* pIdx = m_oObjects.Find(nSessionId);
	if(pIdx != pEnd)
		pObject = m_oObjects.GetItem(pIdx);
	m_oMutex.Leave();

	if(pObject)
	{
		bool bLoop = true;
		if(nTimeOut)
			pObject->m_oEvent.Wait(nTimeOut);
		while(bLoop)
		{
			pObject->m_oLock.Wait(0xFFFFFFFF);
			m_oMutex.Enter();
			if(!pObject->m_nLocked)
			{
				m_oObjects.Remove(pIdx);
				bLoop = false;
			}
			m_oMutex.Leave();
		}
		nStatus = pObject->m_nStatus;
	}

	return nStatus;
}

CSyncCallee* CSyncCaller::QueryCall(uint32 nSessionId)
{
	CSyncCallee * pObject = NULL;
	CRbTreeNode* pEnd = m_oObjects.End();
	m_oMutex.Enter();
	CRbTreeNode* pIdx = m_oObjects.Find(nSessionId);
	if(pIdx != pEnd)
	{
		pObject = m_oObjects.GetItem(pIdx);
		if(!pObject->m_nLocked)
			pObject->m_oLock.Reset();
		pObject->m_nLocked++;
	}
	m_oMutex.Leave();

	return pObject;
}

void CSyncCaller::ReleaseCall(CSyncCallee* pCallee)
{
	m_oMutex.Enter();
	if(pCallee->m_nLocked)
	{
		--pCallee->m_nLocked;
		if(!pCallee->m_nLocked)
			pCallee->m_oLock.Set();
	}
	m_oMutex.Leave();
}

void CSyncCaller::AnswerAll(uint32 nStatus)
{
	CRbTreeNode* pEnd = m_oObjects.End();
	m_oMutex.Enter();
	CRbTreeNode* pIdx = m_oObjects.First();
	for(; pIdx!=pEnd; pIdx = m_oObjects.GetNext(pIdx))
	{
		CSyncCallee * pObject = m_oObjects.GetItem(pIdx);
		if(!pObject->m_nLocked)
			pObject->m_oLock.Reset();
		pObject->m_nLocked++;
		pObject->m_nStatus = nStatus;
		pObject->m_oEvent.Set();
		--pObject->m_nLocked;
		if(!pObject->m_nLocked)
			pObject->m_oLock.Set();
	}
	m_oMutex.Leave();
}

CAsyncCallee::CAsyncCallee()
{
	m_pObject = NULL;
	m_nSessionId = 0;
}

CAsyncCallee::~CAsyncCallee()
{
}

void* CAsyncCallee::QueryCallObject()
{
	return m_pObject;
}

FOCP_DETAIL_BEGIN();

CAsyncCalleeClass::CAsyncCalleeClass(CAsyncCaller* pCaller)
{
	m_pCaller = pCaller;
}

uint32 CAsyncCalleeClass::GetObjectId(void* pObject) const
{
	return ((CAsyncCallee*)pObject)->m_nSessionId & g_nBitMask[m_pCaller->m_nRightBits];
}

void CAsyncCalleeClass::SetObjectId(void* pBuf, uint32 nId) const
{
	uint32 nCounter = ((CAsyncCallee*)pBuf)->m_nSessionId >> (m_pCaller->m_nRightBits);
	++nCounter;
	if(nId == 0)//È·±£session id·Ç0
	{
		if(nCounter == 0)
			++nCounter;
		if(nCounter > m_pCaller->m_nMaxCounter)
			nCounter = 1;
	}
	((CAsyncCallee*)pBuf)->m_nSessionId = nId | (nCounter << m_pCaller->m_nRightBits);
}

FOCP_DETAIL_END();

FOCP_PRIVATE_BEGIN();
struct AsyncCallerTimerMsg
{
	CAsyncCaller* pCaller;
	uint32 nSessionId;
};
void AsyncCallerTimer(uint8* msg, uint32 msglen, uint32 nTimerId)
{
	AsyncCallerTimerMsg* pMsg = (AsyncCallerTimerMsg*)msg;
	CAsyncCallee* pCallee = pMsg->pCaller->QueryCall(pMsg->nSessionId);
	if(pCallee)
	{
		pMsg->pCaller->AnswerCall(pCallee,true);
		pMsg->pCaller->ReleaseCall(pCallee);
	}
}
FOCP_PRIVATE_END();

CAsyncCaller::CAsyncCaller(uint32 nCapacity, bool bThread)
	:m_oQueue(FocpFieldOffset(CAsyncCallee,m_pNext)), m_oEvent(false, bThread), m_oCalleeClass(this), m_oObjects(&m_oCalleeClass)
{
	uint32 nLeftBits;
	if(!nCapacity)
		nCapacity = 1024;
	m_nCapacity = nCapacity;

	uint32 nBit = 0x80000000;
	for(nLeftBits=0; 1; ++nLeftBits)
	{
		if(m_nCapacity&nBit)
			break;
		nBit >>= 1;
	}
	if(nLeftBits <= 1)
		FocpAbort(("CAsyncCaller::CAsyncCaller(%u) failure", nCapacity));
	m_nMaxCounter = g_nBitMask[nLeftBits-1];
	m_nRightBits = 32 - nLeftBits;
}

CAsyncCaller::~CAsyncCaller()
{
}

uint32 CAsyncCaller::GetSize()
{
	return m_oObjects.GetSize();
}

uint32 CAsyncCaller::ApplyCall(void* pCallObject, uint32 nTimeOut)
{
	if(nTimeOut == 0)
		return 0;

	m_oMutex.Enter();

	if(m_oObjects.GetSize() >= m_nCapacity)
	{
		m_oMutex.Leave();
		return 0;
	}

	CAsyncCallee* pCallee = (CAsyncCallee*)m_oObjects.CreateObject();
	if(pCallee == NULL)
	{
		m_oMutex.Leave();
		return 0;
	}
	uint32 nSessionId = pCallee->m_nSessionId;
	pCallee->m_oLock.Set();
	pCallee->m_nLocked = 0;
	pCallee->m_pObject = pCallObject;
	if(nTimeOut != 0xFFFFFFFF)
	{
		AsyncCallerTimerMsg oMsg = {this, nSessionId};
		CTimer::GetInstance()->SetTimer(nTimeOut, AsyncCallerTimer, (uint8*)&oMsg, sizeof(AsyncCallerTimerMsg));
	}

	m_oMutex.Leave();

	return nSessionId;
}

uint32 CAsyncCaller::PullCall(uint32 &nSessionId, void* &pCallObject)
{
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	CAsyncCallee* pCallee = m_oQueue.Pop();
	m_oMutex.Leave();
	if(pCallee == NULL)
	{
		pCallObject = NULL;
		return FOCP_SESSION_INVALID;
	}
	pCallObject = pCallee->m_pObject;
	nSessionId = pCallee->m_nSessionId;
	uint32 nStatus;
	bool bLoop = true;
	while(bLoop)
	{
		pCallee->m_oLock.Wait(0xFFFFFFFF);
		m_oMutex.Enter();
		if(!pCallee->m_nLocked)
		{
			pCallee->m_pObject = NULL;
			m_oObjects.DestroyObject(pCallee);
			nStatus = pCallee->m_nStatus;
			bLoop = false;
		}
		m_oMutex.Leave();
	}
	return nStatus;
}

CAsyncCallee* CAsyncCaller::QueryCall(uint32 nSessionId)
{
	CAsyncCallee* pCallee = (CAsyncCallee*)m_oObjects.QueryObject(nSessionId & g_nBitMask[m_nRightBits]);
	if(pCallee)
	{
		m_oMutex.Enter();
		if(pCallee->m_nSessionId != nSessionId)
			pCallee = NULL;
		else
		{
			if(!pCallee->m_nLocked)
				pCallee->m_oLock.Reset();
			pCallee->m_nLocked++;
		}
		m_oMutex.Leave();
	}
	return pCallee;
}

void CAsyncCaller::AnswerCall(CAsyncCallee* pCallee, bool bTimeOut)
{
	if(bTimeOut)
		pCallee->m_nStatus = FOCP_SESSION_TIMEOUT;
	else
		pCallee->m_nStatus = FOCP_SESSION_NORMAL;
	m_oMutex.Enter();
	m_oQueue.Push(pCallee);
	m_oMutex.Leave();
}

void CAsyncCaller::ReleaseCall(CAsyncCallee* pCallee)
{
	m_oMutex.Enter();
	if(pCallee->m_nLocked)
	{
		--pCallee->m_nLocked;
		if(!pCallee->m_nLocked)
			pCallee->m_oLock.Set();
	}
	m_oMutex.Leave();
}

FOCP_END();
