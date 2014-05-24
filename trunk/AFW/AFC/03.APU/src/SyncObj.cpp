
#include "Cooperator.hpp"

#include <time.h>

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef UNIX
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

FOCP_BEGIN();

FOCP_PRIVATE_BEGIN();

#ifdef UNIX
struct CRITICAL_SECTION
{
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
};
struct pthread_event_t
{
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	bool bmanual, bvalue;
};
#endif

#ifdef WINDOWS
struct pthread_event_t
{
	HANDLE hEvent;
};
#endif

struct CThreadMutex
{
	bool bThread;
	CRITICAL_SECTION oMutex;
	threadid_t nThreadId;
	uint32 nCounter;
};

struct CFiberMutex
{
	bool bThread;
	uint32 nRecursion;
	FOCP_DETAIL_NAME::CFiberData* pCur;
	char sWaitTable[sizeof(CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>)];
};

struct CThreadEvent
{
	bool bThread;
	pthread_event_t oEvent;
};

struct CFiberEvent
{
	bool bThread;
	char sWaitTable[sizeof(CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>)];
	bool bManualReset, bSignal;
};

FOCP_PRIVATE_END();

CMutex::CMutex(bool bThread)
{
	if(bThread)
	{
		CThreadMutex* pMutex = (CThreadMutex*)CMalloc::Malloc(sizeof(CThreadMutex));
		pMutex->bThread = true;
		pMutex->nThreadId = 0;
		pMutex->nCounter = 0;
#ifdef WINDOWS
		InitializeCriticalSection(&pMutex->oMutex);
#endif
#ifdef UNIX
		pthread_mutexattr_init(&pMutex->oMutex.attr);
		pthread_mutexattr_settype(&pMutex->oMutex.attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&pMutex->oMutex.mutex, &pMutex->oMutex.attr);
#endif
		m_pMutex = pMutex;
	}
	else
	{
		CFiberMutex* pMutex = (CFiberMutex*)CMalloc::Malloc(sizeof(CFiberMutex));
		new(pMutex->sWaitTable) CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>(FocpFieldOffset(FOCP_DETAIL_NAME::CFiberData, pWait));
		pMutex->bThread = false;
		pMutex->pCur = NULL;
		pMutex->nRecursion = 0;
		m_pMutex = pMutex;
	}
}

CMutex::~CMutex()
{
	bool &bThread = *(bool*)m_pMutex;
	if(bThread)
	{
		CThreadMutex* pMutex = (CThreadMutex*)m_pMutex;
		if(pMutex->nCounter && pMutex->nThreadId != CCooperator::GetCurrentThreadId())
			FocpAbort(("CMutex::`CMutex() failure"));
#ifdef WINDOWS
		DeleteCriticalSection(&pMutex->oMutex);
#endif
#ifdef UNIX
		pthread_mutex_destroy(&pMutex->oMutex.mutex);
		pthread_mutexattr_destroy(&pMutex->oMutex.attr);
#endif
	}
	else
	{
		CFiberMutex* pMutex = (CFiberMutex*)m_pMutex;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pMutex->sWaitTable;
		pWaitTable->~CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>();
	}
	CMalloc::Free(m_pMutex);
	m_pMutex = NULL;
}

void CMutex::Enter()
{
	bool &bThread = *(bool*)m_pMutex;
	if(bThread)
	{
		CThreadMutex* pMutex = (CThreadMutex*)m_pMutex;
#ifdef WINDOWS
		EnterCriticalSection(&pMutex->oMutex);
#endif
#ifdef UNIX
		pthread_mutex_lock(&pMutex->oMutex.mutex);
#endif
		pMutex->nThreadId = CCooperator::GetCurrentThreadId();
		++pMutex->nCounter;
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		CFiberMutex* pMutex = (CFiberMutex*)m_pMutex;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pMutex->sWaitTable;
		CCooperator* pCooperator = CCooperator::GetCurrentCooperator(false,false);
		if(pCooperator == NULL)
			FocpAbort(("CMutex::Enter() failure: not in the any fiber"));
		FOCP_DETAIL_NAME::CFiberData* pFiber = (FOCP_DETAIL_NAME::CFiberData*)pCooperator->GetCooperatorData();
		pFiber->pWait = NULL;
		pFiberManager->m_oMutex.Enter();
		if(pMutex->pCur == NULL || pMutex->pCur == pFiber)
		{
			pMutex->pCur = pFiber;
			++pMutex->nRecursion;
		}
		else
		{
			pWaitTable->Push(pFiber);
			CFiberManager::SuspendFiber(pFiber, 0xFFFFFFFF);
		}
		pFiberManager->m_oMutex.Leave();
	}
}

void CMutex::Leave()
{
	bool &bThread = *(bool*)m_pMutex;
	if(bThread)
	{
		CThreadMutex* pMutex = (CThreadMutex*)m_pMutex;
		if(!pMutex->nCounter || pMutex->nThreadId != CCooperator::GetCurrentThreadId())
			FocpAbort(("CMutex::Leave() failure"));
		--pMutex->nCounter;
#ifdef WINDOWS
		LeaveCriticalSection(&pMutex->oMutex);
#endif

#ifdef UNIX
		pthread_mutex_unlock(&pMutex->oMutex.mutex);
#endif
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		CFiberMutex* pMutex = (CFiberMutex*)m_pMutex;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pMutex->sWaitTable;
		CCooperator* pCooperator = CCooperator::GetCurrentCooperator(false,false);
		if(pCooperator == NULL)
			FocpAbort(("CMutex::Leave() failure: not in the any fiber"));
		FOCP_DETAIL_NAME::CFiberData* pFiber = (FOCP_DETAIL_NAME::CFiberData*)pCooperator->GetCooperatorData();
		pFiberManager->m_oMutex.Enter();
		if(pMutex->pCur == pFiber)
		{
			--pMutex->nRecursion;
			if(pMutex->nRecursion == 0)
			{
				pMutex->pCur = pWaitTable->Pop();
				if(pMutex->pCur)
				{
					pMutex->nRecursion = 1;
					pFiberManager->ResumeFiber2(pMutex->pCur, false);
				}
			}
		}
		pFiberManager->m_oMutex.Leave();
	}
}

CLock::CLock(CMutex &oMutex):m_oMutex(oMutex)
{
	m_oMutex.Enter();
}

CLock::~CLock()
{
	m_oMutex.Leave();
}

CEvent::CEvent(bool bAutoReset, bool bThread)
{
	if(bThread)
	{
		CThreadEvent* pEvent = (CThreadEvent*)CMalloc::Malloc(sizeof(CThreadEvent));
		pEvent->bThread = true;
#ifdef WINDOWS
		pEvent->oEvent.hEvent = CreateEvent(NULL, bAutoReset?FALSE:TRUE, FALSE, NULL);
#endif
#ifdef UNIX
		pEvent->oEvent.bmanual = bAutoReset?false:true;
		pEvent->oEvent.bvalue = false;
		pthread_mutex_init(&(pEvent->oEvent.mutex), NULL);
		pthread_cond_init(&(pEvent->oEvent.cond), NULL);
#endif
		m_pEvent = pEvent;
	}
	else
	{
		CFiberEvent* pEvent = (CFiberEvent*)CMalloc::Malloc(sizeof(CFiberEvent));
		pEvent->bThread = false;
		new(pEvent->sWaitTable) CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>(FocpFieldOffset(FOCP_DETAIL_NAME::CFiberData, pWait));
		pEvent->bSignal = false;
		pEvent->bManualReset = !bAutoReset;
		m_pEvent = pEvent;
	}
}

CEvent::~CEvent()
{
	bool &bThread = *(bool*)m_pEvent;
	if(bThread)
	{
		CThreadEvent* pEvent = (CThreadEvent*)m_pEvent;
#ifdef WINDOWS
		CloseHandle(pEvent->oEvent.hEvent);
#endif
#ifdef UNIX
		pthread_cond_destroy(&(pEvent->oEvent.cond));
		pthread_mutex_destroy(&(pEvent->oEvent.mutex));
#endif
	}
	else
	{
		CFiberEvent* pEvent = (CFiberEvent*)m_pEvent;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pEvent->sWaitTable;
		pWaitTable->~CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>();
	}
	CMalloc::Free(m_pEvent);
	m_pEvent = NULL;
}

bool CEvent::Wait(uint32 nTimeOut)
{
	bool &bThread = *(bool*)m_pEvent;
	if(bThread)
	{
		CThreadEvent* pEvent = (CThreadEvent*)m_pEvent;
#ifdef WINDOWS
		if(nTimeOut)
		{
			uint32 nMinSleep = CCooperator::GetMinSleep();
			if(nTimeOut < nMinSleep)
				nTimeOut = nMinSleep;
		}
		return WAIT_OBJECT_0 == WaitForSingleObject(pEvent->oEvent.hEvent, nTimeOut);
#endif
#ifdef UNIX
		timespec tt;
		uint32 nRet;

		if(nTimeOut != (uint32)(-1))
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			tt.tv_sec = tv.tv_sec;
			tt.tv_nsec = tv.tv_usec*1000;
			tt.tv_sec += nTimeOut/1000;
			tt.tv_nsec += (nTimeOut%1000)*1000000;
			if(tt.tv_nsec >= 1000000000)
			{
				tt.tv_nsec -= 1000000000;
				tt.tv_sec++;
			}
		}

		pthread_mutex_lock(&(pEvent->oEvent.mutex));

		if(pEvent->oEvent.bvalue)
		{
			if(pEvent->oEvent.bmanual == 0)
				pEvent->oEvent.bvalue = 0;
			pthread_mutex_unlock(&(pEvent->oEvent.mutex));
			return true;
		}

		nRet = 0;
		while(pEvent->oEvent.bvalue == false)
		{
			if(nTimeOut == (uint32)(-1))
				pthread_cond_wait(&(pEvent->oEvent.cond), &(pEvent->oEvent.mutex));
			else
			{
				nRet = pthread_cond_timedwait(&(pEvent->oEvent.cond), &(pEvent->oEvent.mutex), &tt);
				if(nRet)
					break;
			}
		}
		if(pEvent->oEvent.bvalue)
		{
			nRet = 0;
			if(pEvent->oEvent.bmanual == 0)
				pEvent->oEvent.bvalue = false;
			else
				pthread_cond_signal(&(pEvent->oEvent.cond));
		}
		pthread_mutex_unlock(&(pEvent->oEvent.mutex));

		return (nRet == 0);
#endif
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		CFiberEvent* pEvent = (CFiberEvent*)m_pEvent;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pEvent->sWaitTable;
		CCooperator* pCooperator = CCooperator::GetCurrentCooperator(false,false);
		if(pCooperator == NULL)
			FocpAbort(("CEvent::Wait() failure: not in the any fiber"));
		FOCP_DETAIL_NAME::CFiberData* pFiber = (FOCP_DETAIL_NAME::CFiberData*)pCooperator->GetCooperatorData();
		pFiber->pWait = NULL;
		pFiberManager->m_oMutex.Enter();
		if(pEvent->bSignal)
		{
			if(pEvent->bManualReset == false)
				pEvent->bSignal = false;
			pFiberManager->m_oMutex.Leave();
			return true;
		}
		pWaitTable->Push(pFiber);
		int32 nRet = CFiberManager::SuspendFiber(pFiber, nTimeOut);
		if(nRet == -1)
		{
			FOCP_DETAIL_NAME::CFiberData* pHead = pWaitTable->First(), *pPrev = NULL;
			while(pHead)
			{
				FOCP_DETAIL_NAME::CFiberData* pWait = pWaitTable->GetNext(pHead);
				if(pHead == pFiber)
				{
					pWaitTable->RemoveNext(pPrev);
					break;
				}
				pPrev = pHead;
				pHead = pWait;
			}
		}
		pFiberManager->m_oMutex.Leave();
		return (nRet != -1);
	}
}

void CEvent::Set()
{
	bool &bThread = *(bool*)m_pEvent;
	if(bThread)
	{
		CThreadEvent* pEvent = (CThreadEvent*)m_pEvent;
#ifdef WINDOWS
		SetEvent(pEvent->oEvent.hEvent);
#endif
#ifdef UNIX
		if(!pEvent->oEvent.bvalue)
		{
			pthread_mutex_lock(&(pEvent->oEvent.mutex));
			if(!pEvent->oEvent.bvalue)
			{
				pEvent->oEvent.bvalue = true;
				pthread_cond_signal(&(pEvent->oEvent.cond));
			}
			pthread_mutex_unlock(&(pEvent->oEvent.mutex));
		}
#endif
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		CFiberEvent* pEvent = (CFiberEvent*)m_pEvent;
		CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>* pWaitTable = (CBaseSingleList<FOCP_DETAIL_NAME::CFiberData>*)pEvent->sWaitTable;
		FOCP_DETAIL_NAME::CFiberData* pFiber;
		pFiberManager->m_oMutex.Enter();
		if(pEvent->bSignal == false)
		{
			pEvent->bSignal = true;
			while(pEvent->bSignal && (pFiber=pWaitTable->Pop()))
			{
				if(pEvent->bManualReset == false)
					pEvent->bSignal = false;
				pFiberManager->ResumeFiber2(pFiber, false);
			}
		}
		pFiberManager->m_oMutex.Leave();
	}
}

void CEvent::Reset()
{
	bool &bThread = *(bool*)m_pEvent;
	if(bThread)
	{
		CThreadEvent* pEvent = (CThreadEvent*)m_pEvent;
#ifdef WINDOWS
		ResetEvent(pEvent->oEvent.hEvent);
#endif
#ifdef UNIX
		if(pEvent->oEvent.bvalue)
		{
			pthread_mutex_lock(&(pEvent->oEvent.mutex));
			if(pEvent->oEvent.bvalue)
			{
				timespec tt;
				tt.tv_sec = time(NULL);
				tt.tv_nsec = 0;
				pthread_cond_timedwait(&(pEvent->oEvent.cond), &(pEvent->oEvent.mutex), &tt);
				pEvent->oEvent.bvalue = false;
			}
			pthread_mutex_unlock(&(pEvent->oEvent.mutex));
		}
#endif
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		CFiberEvent* pEvent = (CFiberEvent*)m_pEvent;
		pFiberManager->m_oMutex.Enter();
		if(pEvent->bSignal)
			pEvent->bSignal = false;
		pFiberManager->m_oMutex.Leave();
	}
}

CSemaphore::CSemaphore(uint32 nInitialSignal, bool bThread)
	:m_oMutex(bThread), m_oEvent(bThread)
{
	m_nSignal = nInitialSignal;
	if(m_nSignal)
		m_oEvent.Set();
}

CSemaphore::~CSemaphore()
{
}

bool CSemaphore::Wait(uint32 nTimeOut)
{
	m_oEvent.Wait(nTimeOut);
	m_oMutex.Enter();
	bool bRet = (m_nSignal > 0);
	if(bRet)
	{
		--m_nSignal;
		if(m_nSignal == 0)
			m_oEvent.Reset();
	}
	m_oMutex.Leave();
	return bRet;
}

void CSemaphore::Release(uint32 nSignal)
{
	m_oMutex.Enter();
	if(m_nSignal < (uint32)(-1))
	{
		uint32 nRest = (uint32)(-1) - m_nSignal;
		if(nSignal > nRest)
			nSignal = nRest;
		if(m_nSignal == 0)
			m_oEvent.Set();
		m_nSignal += nSignal;
	}
	m_oMutex.Leave();
}

CChannel::CChannel(uint32 nCount, bool bThread)
	:m_oMutex(bThread), m_oEvent(bThread), m_oBits(nCount)
{
	m_nSignal = 0;
}

CChannel::~CChannel()
{
}

bool CChannel::Wait(uint32 &nChannel, uint32 nTimeOut)
{
	m_oEvent.Wait(nTimeOut);
	m_oMutex.Enter();
	bool bRet = (m_nSignal > 0);
	if(bRet)
	{
		nChannel = m_oBits.First();
		m_oBits.Set(nChannel, false);
		--m_nSignal;
		if(m_nSignal == 0)
			m_oEvent.Reset();
	}
	m_oMutex.Leave();
	return bRet;
}

void CChannel::Set(uint32 nChannel)
{
	m_oMutex.Enter();
	if(m_oBits.Get(nChannel) == false)
	{
		m_oBits.Set(nChannel, true);
		++m_nSignal;
		if(m_nSignal == 1)
			m_oEvent.Set();
	}
	m_oMutex.Leave();
}

void CChannel::Reset(uint32 nChannel)
{
	m_oMutex.Enter();
	if(m_oBits.Get(nChannel))
	{
		m_oBits.Set(nChannel, false);
		--m_nSignal;
		if(m_nSignal == 0)
			m_oEvent.Reset();
	}
	m_oMutex.Leave();
}

CRwMutex::CRwMutex(bool bThread)
	:m_oMutex(bThread), m_bThread(bThread)
{
	m_nReadCounter = 0;
	m_nWriteCounter = 0;
	m_nWaitWriteCounter = 0;
	if(bThread)
		m_nWriter.nTid = 0;
	else
		m_nWriter.pFiber = NULL;
}

CRwMutex::~CRwMutex()
{
}

void CRwMutex::EnterRead()
{
	CCooperatorId oCid;
	if(!GetCurrentCooperator(oCid))
		FocpAbort(("CRwMutex::EnterRead() failure"));
	bool bNeedWait = true;
	void * pReaders = NULL;
	m_oMutex.Enter();
	if(!m_nWriteCounter || CidEqual(m_nWriter, oCid))
	{
		pReaders = m_oReaderList.First();
		while(pReaders)
		{
			CReaderNode& oNode = m_oReaderList.GetItem(pReaders);
			if(CidEqual(oNode.oCid, oCid))
				break;
			pReaders = m_oReaderList.GetNext(pReaders);
		}
		if(pReaders || !m_nWaitWriteCounter)
			bNeedWait = false;
	}
	if(bNeedWait)
	{
		CEvent oEvent(false, m_bThread);
		CWaitNode oWaitNode;
		if(m_bThread)
			oWaitNode.oCid.nTid = oCid.nTid;
		else
			oWaitNode.oCid.pFiber = oCid.pFiber;
		oWaitNode.bReader = true;
		oWaitNode.pEvent = &oEvent;
		m_oWaitList.Push(oWaitNode);
		m_oMutex.Leave();
		oEvent.Wait(0xFFFFFFFF);
		m_oMutex.Enter();
	}
	if(!pReaders)
	{
		CReaderNode oNode;
		if(m_bThread)
			oNode.oCid.nTid = oCid.nTid;
		else
			oNode.oCid.pFiber = oCid.pFiber;
		oNode.nCounter = 0;
		pReaders = m_oReaderList.Push(oNode);
	}
	++m_oReaderList.GetItem(pReaders).nCounter;
	m_oMutex.Leave();
}

void CRwMutex::LeaveRead()
{
	CCooperatorId oCid;
	if(!GetCurrentCooperator(oCid))
		FocpAbort(("CRwMutex::LeaveRead() failure"));
	void * pReaders = NULL, *pPrev = NULL;
	m_oMutex.Enter();
	pReaders = m_oReaderList.First();
	while(pReaders)
	{
		CReaderNode& oNode = m_oReaderList.GetItem(pReaders);
		if(CidEqual(oNode.oCid, oCid))
			break;
		pPrev = pReaders;
		pReaders = m_oReaderList.GetNext(pReaders);
	}
	if(pReaders)
	{
		CReaderNode& oNode = m_oReaderList.GetItem(pReaders);
		if(!(--oNode.nCounter))
		{
			m_oReaderList.RemoveNext(pPrev);
			if(m_oReaderList.GetSize() == 0 && !m_nWriteCounter && m_oWaitList.GetSize())
			{
				CWaitNode oWaitNode;
				m_oWaitList.Pop(oWaitNode);
				oWaitNode.pEvent->Set();
			}
		}
	}
	m_oMutex.Leave();
}

void CRwMutex::EnterWrite()
{
	CCooperatorId oCid;
	if(!GetCurrentCooperator(oCid))
		FocpAbort(("CRwMutex::EnterWrite() failure"));
	bool bNeedWait = false;
	m_oMutex.Enter();
	if(m_nWriteCounter)
	{
		if(CidEqual(m_nWriter, oCid))
			++m_nWriteCounter;
		else
			bNeedWait = true;
	}
	else if(m_oWaitList.GetSize())
		bNeedWait = true;
	else
	{
		m_nWriteCounter = 1;
		if(m_bThread)
			m_nWriter.nTid = oCid.nTid;
		else
			m_nWriter.pFiber = oCid.pFiber;
	}
	if(bNeedWait)
	{
		CEvent oEvent(false, m_bThread);
		CWaitNode oWaitNode;
		if(m_bThread)
			oWaitNode.oCid.nTid = oCid.nTid;
		else
			oWaitNode.oCid.pFiber = oCid.pFiber;
		oWaitNode.bReader = false;
		oWaitNode.pEvent = &oEvent;
		++m_nWaitWriteCounter;
		m_oWaitList.Push(oWaitNode);
		m_oMutex.Leave();
		oEvent.Wait(0xFFFFFFFF);
		m_oMutex.Enter();
		--m_nWaitWriteCounter;
		m_nWriteCounter = 1;
		if(m_bThread)
			m_nWriter.nTid = oCid.nTid;
		else
			m_nWriter.pFiber = oCid.pFiber;
	}
	m_oMutex.Leave();
}

void CRwMutex::LeaveWrite()
{
	CCooperatorId oCid;
	if(!GetCurrentCooperator(oCid))
		FocpAbort(("CRwMutex::LeaveWrite() failure"));
	m_oMutex.Enter();
	if(m_nWriteCounter && CidEqual(m_nWriter, oCid) &&
			!(--m_nWriteCounter) && m_oWaitList.GetSize())
	{
		CWaitNode oWaitNode;
		m_oWaitList.Pop(oWaitNode);
		oWaitNode.pEvent->Set();
		if(oWaitNode.bReader)
		{
			void* pIt = m_oWaitList.First(), *pPrev = NULL;
			while(pIt)
			{
				void* pNext = m_oWaitList.GetNext(pIt);
				CWaitNode &oNode = m_oWaitList.GetItem(pIt);
				if(oNode.bReader)
				{
					oNode.pEvent->Set();
					m_oWaitList.RemoveNext(pPrev);
				}
				else
					pPrev = pIt;
				pIt = pNext;
			}
		}
	}
	m_oMutex.Leave();
}

bool CRwMutex::GetCurrentCooperator(CCooperatorId& oCoop)
{
	bool bRet = true;
	if(m_bThread)
		oCoop.nTid = CCooperator::GetCurrentThreadId();
	else
	{
		oCoop.pFiber = CCooperator::GetCurrentCooperator(false, false);
		bRet = (oCoop.pFiber != NULL);
	}
	return bRet;
}

bool CRwMutex::CidEqual(const CCooperatorId& l, const CCooperatorId& r)
{
	if(m_bThread)
		return l.nTid == r.nTid;
	return l.pFiber == r.pFiber;
}

FOCP_END();
