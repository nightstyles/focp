
#include "Timer.hpp"
#include "LongJmpWin32.hpp"

#include <time.h>

#ifdef WINDOWS
#include <windows.h>
#include <process.h>
#endif

#ifdef UNIX
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <errno.h>

FOCP_BEGIN();

FOCP_PRIVATE_BEGIN();

#ifdef WINDOWS
struct CThreadData
{
	HANDLE hThread;
	DWORD nThreadId;
	bool bRunning;
};
#endif

#ifdef UNIX
struct CThreadData
{
	pthread_attr_t nAttr;
	pthread_t nThreadId;
	bool bRunning;
};
#endif

enum CFibeStatus
{
	FOCP_FIBER_INITED,
	FOCP_FIBER_READY,
	FOCP_FIBER_RUNNING,
	FOCP_FIBER_SLEEP
};

CThreadVariable<CCooperator*>& GetThreadVariable()
{
	return *CSingleInstance< CThreadVariable<CCooperator*> >::GetInstance();
}

CThreadVariable<FOCP_DETAIL_NAME::CFiberData*>& GetFiberVariable()
{
	return *CSingleInstance< CThreadVariable<FOCP_DETAIL_NAME::CFiberData*> >::GetInstance();
}

FOCP_PRIVATE_END();

FOCP_DETAIL_BEGIN();

void CFiberDataClass::ResetObject(void* pObject) const
{
	CFiberData* pFiber = (CFiberData*)pObject;
	if(pFiber->pLocalStorage)
	{
		CRbTreeNode* pIt = pFiber->pLocalStorage->First();
		CRbTreeNode* pEnd = pFiber->pLocalStorage->End();
		for(; pIt!=pEnd; pIt=pFiber->pLocalStorage->GetNext(pIt))
		{
			FOCP_DETAIL_NAME::CFiberContext &oContext = pFiber->pLocalStorage->GetItem(pIt);
			if(oContext.pData && oContext.Destroy)
				oContext.Destroy(oContext.pData);
		}
		pFiber->pLocalStorage->Clear();
	}
	pFiber->pCooperator->Release();
	pFiber->pCooperator = NULL;
	pFiber->pPrev = NULL;
	pFiber->pNext = NULL;
	pFiber->pWait = NULL;
	pFiber->pCurBreak = NULL;
	pFiber->pTopBreak = NULL;
	pFiber->pStackTop = NULL;
	pFiber->pStackBottom = NULL;
	pFiber->pStackBackup = NULL;
	pFiber->pWorker = NULL;
	pFiber->nFiberStatus = FOCP_FIBER_INITED;
	pFiber->nTimerId = 0xFFFFFFFF;
	pFiber->bTimeOut = false;
	pFiber->bRunning = false;
}

void CFiberDataClass::ConstructInstance(void* pBuf) const
{
	CGeneralClass<CFiberData>::ConstructInstance(pBuf);
	CFiberData* pFiber = (CFiberData*)pBuf;
	pFiber->pCooperator = NULL;
	pFiber->pPrev = NULL;
	pFiber->pNext = NULL;
	pFiber->pWait = NULL;
	pFiber->pLocalStorage = NULL;
	pFiber->pFiberBreak = new CBreak;
	pFiber->pCurBreak = NULL;
	pFiber->pTopBreak = NULL;
	pFiber->pStackTop = NULL;
	pFiber->pStackBottom = NULL;
	pFiber->pStackBackup = NULL;
	pFiber->pWorker = NULL;
	pFiber->nFiberStatus = FOCP_FIBER_INITED;
	pFiber->nTimerId = 0xFFFFFFFF;
	pFiber->nFiberId = 0;
	pFiber->bTimeOut = false;
}

void CFiberDataClass::DestructInstance(void* pObject) const
{
	CFiberData* pFiber = (CFiberData*)pObject;
	if(pFiber->pLocalStorage)
	{
		delete pFiber->pLocalStorage;
		pFiber->pLocalStorage = NULL;
	}
	delete pFiber->pFiberBreak;
	pFiber->pFiberBreak = NULL;
	pFiber->pCooperator = NULL;
	pFiber->pPrev = NULL;
	pFiber->pNext = NULL;
	pFiber->pWait = NULL;
	pFiber->pCurBreak = NULL;
	pFiber->pTopBreak = NULL;
	pFiber->pStackTop = NULL;
	pFiber->pStackBottom = NULL;
	pFiber->pStackBackup = NULL;
	pFiber->pWorker = NULL;
	pFiber->nFiberStatus = FOCP_FIBER_INITED;
	pFiber->nTimerId = 0xFFFFFFFF;
	pFiber->nFiberId = 0;
	pFiber->bTimeOut = false;
	CGeneralClass<CFiberData>::DestructInstance(pObject);
}

CFiberWorker::CFiberWorker()
	:m_oReadyQueue(FocpFieldOffset(CFiberData, pNext)), m_oCache(true, 256)
{
	m_pTopBreak = new CBreak;
	m_nFiberCount = 0;
	m_pPrev = NULL;
	m_pNext = NULL;
}

CFiberWorker::~CFiberWorker()
{
	delete m_pTopBreak;
}

void CFiberWorker::PutInFiber(CFiberData* pFiber)
{
	m_oMutex.Enter();
	if(!m_oReadyQueue.GetSize())
		m_oEvent.Set();
	m_oReadyQueue.Push(pFiber);
	m_oMutex.Leave();
}

CFiberData* CFiberWorker::PullOutFiber()
{
	m_oEvent.Wait(2000);
	m_oMutex.Enter();
	CFiberData* pFiber = m_oReadyQueue.Pop();
	if(!m_oReadyQueue.GetSize())
		m_oEvent.Reset();
	m_oMutex.Leave();
	return pFiber;
}

void CFiberWorker::MainProc(bool &bRunning)
{
	CFiberData*& pFiber = (CFiberData*&)GetFiberVariable();
	CFiberManager* pFiberManager = CFiberManager::GetInstance();
	Run(this, pFiber, pFiberManager, bRunning);
}

FOCP_DETAIL_END();

CCooperateFunction::CCooperateFunction()
{
}

CCooperateFunction::~CCooperateFunction()
{
}

void CCooperateFunction::StopNotify(CCooperator* pCooperator)
{
}

void CCooperateFunction::MainProc(CCooperator* pCooperator, bool &bRunning)
{
	while(bRunning)
		ProcessOnce(pCooperator, bRunning);
}

void CCooperateFunction::ProcessOnce(CCooperator*, bool&)
{
}

void CCooperateFunction::Release(CCooperator* pCooperator)
{
}

CCooperator::CCooperator(CCooperateFunction* pFunction, bool bThread, bool bNeedRelease)
{
	m_pFunction = pFunction;
	m_bThread = bThread;
	m_pData = NULL;
    if(!bNeedRelease)
    {
        //但是CFiberData需要增加，并且该协程必须是全局对象
        //不需要指导Fiber类型，但是需要获得FiberId;
        ++CFiberManager::GetInstance()->m_nMaxFiberCount;
    }
}

void CCooperator::SetFunction(CCooperateFunction* pFunction)
{
	m_pFunction = pFunction;
}

CCooperator::~CCooperator()
{
	Stop();
}

bool CCooperator::Start()
{
	if(m_pData)
		return true;

	if(m_bThread)
	{
		CThreadData* pThread = new CThreadData;
		pThread->bRunning = true;
		m_pData = pThread;
#ifdef WINDOWS
		pThread->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)::FocpThreadProc, this, 0, &pThread->nThreadId);
		if(pThread->hThread == NULL)
		{
			delete pThread;
			m_pData = NULL;
			return false;
		}
#endif
#ifdef UNIX
		if(pthread_attr_init(&pThread->nAttr))
		{
			delete pThread;
			m_pData = NULL;
			return false;
		}
		if(pthread_create(&pThread->nThreadId, &pThread->nAttr, ::FocpThreadProc, this))
		{
			pthread_attr_destroy(&pThread->nAttr);
			delete pThread;
			m_pData = NULL;
			return false;
		}
#endif
	}
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		if(pFiberManager->m_nStop)
			return false;
		FOCP_DETAIL_NAME::CFiberData *pFiber = (FOCP_DETAIL_NAME::CFiberData*)pFiberManager->AllocFiber();
		if(pFiber == NULL)
			return false;
		m_pData = pFiber;
		pFiber->pCooperator = this;
		pFiber->bRunning = true;
		pFiberManager->BindFiber(pFiber);
	}
	return true;
}

void CCooperator::Stop(bool bBlock)
{
	if(m_pData)
	{
		if(m_bThread)
		{
			CThreadData* pThread = (CThreadData*)m_pData;
			if(pThread->bRunning == true)
				StopNotify();
		}
		else
			CFiberManager::GetInstance()->StopFiber(this);
		if(bBlock)
		{
			if(m_bThread)
			{
				CThreadData* pThread = (CThreadData*)m_pData;
#ifdef WINDOWS
				WaitForSingleObject(pThread->hThread, 0xFFFFFFFF);
				CloseHandle(pThread->hThread);
#else
				pthread_join(pThread->nThreadId, NULL);
				pthread_attr_destroy(&pThread->nAttr);
#endif
				delete pThread;
				m_pData = NULL;
			}
			else while(m_pData)
					Sleep(5);
		}
	}
}

bool CCooperator::IsThread()
{
	return m_bThread;
}

bool CCooperator::IsFiber()
{
	return !m_bThread;
}

void* CCooperator::GetCooperatorData()
{
	return m_pData;
}

threadid_t CCooperator::GetThreadId()
{
	if(m_pData == NULL || m_bThread == false)
		return (threadid_t)(-1);
	CThreadData* pThread = (CThreadData*)m_pData;
	return (threadid_t)pThread->nThreadId;
}

uint32 CCooperator::GetFiberId()
{
	if(m_pData == NULL || m_bThread)
		return (uint32)(-1);
	return CFiberManager::GetInstance()->GetFiberId(m_pData);
}

threadid_t CCooperator::GetCurrentThreadId()
{
#ifdef WINDOWS
	return (threadid_t)::GetCurrentThreadId();
#endif

#ifdef UNIX
	return (threadid_t)::pthread_self();
#endif
}

CCooperator* CCooperator::GetCurrentCooperator(bool bDefault, bool bThread)
{
	CCooperator* pCooperator;
	if(bDefault)
	{
		FOCP_DETAIL_NAME::CFiberData* pFiberData = (FOCP_DETAIL_NAME::CFiberData*&)GetFiberVariable();
		if(pFiberData == NULL)
			pCooperator = (CCooperator*&)GetThreadVariable();
		else
			pCooperator = pFiberData->pCooperator;
	}
	else if(bThread)
		pCooperator = (CCooperator*&)GetThreadVariable();
	else
	{
		FOCP_DETAIL_NAME::CFiberData* pFiberData = (FOCP_DETAIL_NAME::CFiberData*&)GetFiberVariable();
		if(pFiberData)
			pCooperator = pFiberData->pCooperator;
		else
			pCooperator = NULL;
	}
	return pCooperator;
}

void CCooperator::Sleep(uint32 nTimeOut, bool bDefault, bool bThread)
{
	CCooperator* pCooperator = GetCurrentCooperator(bDefault, bThread);
	if(pCooperator)
		pCooperator->DoSleep(nTimeOut);
	else
		ThreadSleep(nTimeOut);
}

void CCooperator::DoSleep(uint32 nTimeOut)
{
	if(m_bThread)
		ThreadSleep(nTimeOut);
	else
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		pFiberManager->m_oMutex.Enter();
		CFiberManager::SuspendFiber((FOCP_DETAIL_NAME::CFiberData*)m_pData, nTimeOut);
		pFiberManager->m_oMutex.Leave();
	}
}

#ifdef WINDOWS
static uint32 g_nMinSleep = 0;
static void InitMinSleep()
{
	SystemLock();
	if(!g_nMinSleep)
	{
		for(uint32 i=1; i<1000; ++i)
		{
			uint32 nTime1 = ::GetTickCount();
			::Sleep(1);
			uint32 nTime = ::GetTickCount() - nTime1;
			if(nTime)
			{
				g_nMinSleep = nTime;
				break;
			}
		}
	}
	SystemUnLock();
}
#endif

#ifdef WINDOWS
uint32 CCooperator::GetMinSleep()
{
	if(!g_nMinSleep)
		InitMinSleep();
	return g_nMinSleep;
}
#endif

void CCooperator::ThreadSleep(uint32 nTimeOut)
{
#ifdef UNIX
	timespec tt;
	tt.tv_sec = nTimeOut/1000;
	tt.tv_nsec = (nTimeOut%1000)*1000000;
	nanosleep(&tt,  NULL);
#endif

#ifdef WINDOWS
	if(!g_nMinSleep)
		InitMinSleep();
	if(nTimeOut && nTimeOut < g_nMinSleep)
		nTimeOut = g_nMinSleep;
	::Sleep(nTimeOut);
#endif
}

void CCooperator::MainProc(bool &bRunning)
{
	if(m_pFunction)
		m_pFunction->MainProc(this, bRunning);
	else while(bRunning)
			ProcessOnce(bRunning);
}

void CCooperator::StopNotify()
{
	if(m_pData)
	{
		if(m_bThread)
		{
			CThreadData* pThread = (CThreadData*)m_pData;
			pThread->bRunning = false;
		}
		else
		{
			FOCP_DETAIL_NAME::CFiberData *pFiber = (FOCP_DETAIL_NAME::CFiberData*)m_pData;
			pFiber->bRunning = false;
		}
		if(m_pFunction)
			m_pFunction->StopNotify(this);
	}
}

void CCooperator::ProcessOnce(bool &bRunning)
{
}

void CCooperator::Release()
{
	if(m_pFunction)
		m_pFunction->Release(this);
	if(!m_bThread)
		m_pData = NULL;
}

int32 CCooperator::GetWorkerGroupId()
{
	return 0;
}

#ifdef UNIX

void CCooperator::Kill(int32 nSignal)
{
	if(m_bThread)
	{
		CThreadData* pThread = (CThreadData*)m_pData;
		pthread_kill(pThread->nThreadId, nSignal);
	}
}

static void SignalIgnoreForThread(int32 nSignal)
{
}

void CCooperator::OpenSignal(int32 nSignal, void (*pSignaler)(int))
{
	sigset_t oSet;
	pthread_sigmask(SIG_SETMASK, NULL, &oSet);
	sigdelset(&oSet, nSignal);
	pthread_sigmask(SIG_SETMASK, &oSet, NULL);
	if(pSignaler == NULL)
		pSignaler = SignalIgnoreForThread;
	signal(nSignal, pSignaler);
}

void CCooperator::CloseSignal(int32 nSignal)
{
	sigset_t oSet;
	pthread_sigmask(SIG_SETMASK, NULL, &oSet);
	sigaddset(&oSet, nSignal);
	pthread_sigmask(SIG_SETMASK, &oSet, NULL);
}

#endif

void CCooperator::CallThread()
{
	CThreadVariable<CCooperator*> &oThread = GetThreadVariable();
	CThreadData* pThread = (CThreadData*)m_pData;

	oThread = this;
	MainProc(pThread->bRunning);
	oThread = (CCooperator*)NULL;

	Release();

	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

CFiber::CFiber():CCooperator(NULL, false)
{
}

CFiber::~CFiber()
{
}

void CFiber::Release()
{
	CCooperator::Release();
	CFiberManager::GetFactory()->DestroyObject(this);
}

FOCP_DETAIL_BEGIN();

CFiberReclaim::CFiberReclaim()
	:CCooperator(NULL, false, false), m_oEvent(false,false)
{
}

void CFiberReclaim::KillFiber(CFiberData* pFiber)
{
	m_oMutex.Enter();
	if(m_oQueue.GetSize() == 0)
		m_oEvent.Set();
	m_oQueue.Push(pFiber);
	m_oMutex.Leave();
}

void CFiberReclaim::ProcessOnce(bool &bRunning)
{
	bool bFound;
	CFiberData* pFiber;
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	bFound = m_oQueue.Pop(pFiber);
	if(bFound && !m_oQueue.GetSize())
		m_oEvent.Reset();
	m_oMutex.Leave();
	if(bFound)
	{
		if(pFiber)
			pFiber->pCooperator->StopNotify();
		else
		{
			while(bFound)
			{
				m_oMutex.Enter();
				bFound = m_oQueue.Pop(pFiber);
				if(bFound && !m_oQueue.GetSize())
					m_oEvent.Reset();
				m_oMutex.Leave();
				if(bFound && pFiber)
					pFiber->pCooperator->StopNotify();
			}
			CFiberManager::GetInstance()->StopAllFiber();
			bRunning = false;
		}
	}
}

FOCP_DETAIL_END();

FOCP_DETAIL_NAME::CFiberReclaim g_oReclaimFiber;

CFiberManager::CFiberManager()
	:m_oFiberContainer(CSingleInstance<FOCP_DETAIL_NAME::CFiberDataClass>::GetInstance()),
	 m_oSleepList(FocpFieldOffset(FOCP_DETAIL_NAME::CFiberData, pPrev), FocpFieldOffset(FOCP_DETAIL_NAME::CFiberData, pNext))
{
	GetStackAttr(m_oStackAttr);

	m_nMaxFiberCount = 0;
	m_nLeftBits = 0;
	m_nRightBits = 0;
	m_nMaxFiberIdBits = 0;
	m_nMaxFiberId = 0;

	m_nWorkerCount = 0;
	m_nStop = 3;
	m_pWorkers = NULL;
	m_nWorkerGroupCount = 0;
	m_pWorkerGroupSize = NULL;
	m_pWorkerList = NULL;
}

CFiberManager::~CFiberManager()
{
}

bool CFiberManager::Initialize(uint32 nMaxFiberIdBits, int32 nWorkerGroupCount)
{
	int32 i;
	uint32 nFiberCount = 0;
	m_oFactory.SetDefaultPolicy();
	m_oFactory.GetCapacity(nFiberCount);
	m_nMaxFiberCount += nFiberCount;
	m_nMaxFiberIdBits = nMaxFiberIdBits;
	if(nWorkerGroupCount <= 0 && m_nMaxFiberCount)
		nWorkerGroupCount = 1;
	m_nWorkerGroupCount = nWorkerGroupCount;
	if(m_nWorkerGroupCount > 0)
	{
		m_pWorkerGroupSize = new int32[m_nWorkerGroupCount];
		m_pWorkerGroupIdx = new int32[m_nWorkerGroupCount];
		m_pWorkerList = (CBaseDoubleList<FOCP_DETAIL_NAME::CFiberWorker>*)CMalloc::Malloc(m_nWorkerGroupCount*sizeof(CBaseDoubleList<FOCP_DETAIL_NAME::CFiberWorker>));
		for(i=0; i<m_nWorkerGroupCount; ++i)
		{
			m_pWorkerGroupSize[i] = 1;
			new(m_pWorkerList+i) CBaseDoubleList<FOCP_DETAIL_NAME::CFiberWorker>(FocpFieldOffset(FOCP_DETAIL_NAME::CFiberWorker, m_pPrev), FocpFieldOffset(FOCP_DETAIL_NAME::CFiberWorker, m_pNext));
		}
	}

	if(m_nMaxFiberCount == 0)
		return true;

	uint32 nBit = 0x80000000;
	for(m_nLeftBits=0; 1; ++m_nLeftBits)
	{
		if(m_nMaxFiberCount&nBit)
			break;
		nBit >>= 1;
	}
	m_nRightBits = 32 - m_nLeftBits;
	if(m_nRightBits >= m_nMaxFiberIdBits)
		return false;

	m_nMaxFiberId = 0xFFFFFFFF>>(32-(m_nMaxFiberIdBits - m_nRightBits));

	CAllocatePolicy oPolicy = {m_nMaxFiberCount, m_nMaxFiberCount, m_nMaxFiberCount};
	if(!m_oFiberContainer.SetAllocatePolicy(oPolicy))
		return false;

	return true;
}

void CFiberManager::Cleanup()
{
	if(m_pWorkers)
	{
		delete[] m_pWorkers;
		m_pWorkers = NULL;
	}
	if(m_pWorkerGroupSize)
	{
		delete[] m_pWorkerGroupSize;
		m_pWorkerGroupSize = NULL;
	}
	if(m_pWorkerGroupIdx)
	{
		delete[] m_pWorkerGroupIdx;
		m_pWorkerGroupIdx = NULL;
	}
	if(m_pWorkerList)
	{
		for(int32 i=0; i<m_nWorkerGroupCount; ++i)
			m_pWorkerList[i].~CBaseDoubleList<FOCP_DETAIL_NAME::CFiberWorker>();
		CMalloc::Free(m_pWorkerList);
		m_pWorkerList = NULL;
	}
}

bool CFiberManager::SetWorkerCount(int32 nWorkerGroupId, int32 nWorkerCount)
{
	if(nWorkerGroupId < 0 || nWorkerGroupId >= m_nWorkerGroupCount)
		return false;
	if(nWorkerCount <= 0)
	{
		if(nWorkerGroupId == 0)
			nWorkerCount = 5;
		else
			nWorkerCount = 1;
	}
	m_pWorkerGroupSize[nWorkerGroupId] = nWorkerCount;
	return true;
}

uint32 CFiberManager::GetFiberId(void* pFiber)
{
	uint32 nIndex = 0xFFFFFFFF, nFiberId=0xFFFFFFFF;
	if(pFiber)
	{
		nIndex = m_oFiberContainer.GetObjectIndex(pFiber);
		nFiberId = ((FOCP_DETAIL_NAME::CFiberData*)pFiber)->nFiberId;
	}
	return (nIndex<<m_nLeftBits) | nFiberId;
}

FOCP_DETAIL_NAME::CFiberData* CFiberManager::AllocFiber()
{
	FOCP_DETAIL_NAME::CFiberData* pFiber = (FOCP_DETAIL_NAME::CFiberData*)m_oFiberContainer.CreateObject();
	if(pFiber == NULL)
		return NULL;
	++pFiber->nFiberId;
	if(pFiber->nFiberId >= m_nMaxFiberId)
		pFiber->nFiberId = 0;
	return pFiber;
}

void CFiberManager::FreeFiber(FOCP_DETAIL_NAME::CFiberData* pFiber)
{
	m_oFiberContainer.DestroyObject(pFiber);
}

void CFiberManager::BindFiber(FOCP_DETAIL_NAME::CFiberData* pFiber)
{
	CCooperator* pCooperator = pFiber->pCooperator;
	int32 nGroupIdx = pCooperator->GetWorkerGroupId();
	if(nGroupIdx >= m_nWorkerGroupCount || nGroupIdx < 0)
		nGroupIdx = 0;
	m_oMutex.Enter();
	if(pFiber->nFiberStatus != FOCP_FIBER_INITED)
	{
		m_oMutex.Leave();
		return;
	}
	pFiber->pWorker = m_pWorkerList[nGroupIdx].First();
	pFiber->nFiberStatus = FOCP_FIBER_READY;
	AdjustWorker(pFiber->pWorker, nGroupIdx, true);
	m_oMutex.Leave();
	pFiber->pWorker->PutInFiber(pFiber);
}

void CFiberManager::UnBindFiber(FOCP_DETAIL_NAME::CFiberData* pFiber)
{
	CCooperator* pCooperator = pFiber->pCooperator;
	int32 nGroupIdx = pCooperator->GetWorkerGroupId();
	if(nGroupIdx >= m_nWorkerGroupCount || nGroupIdx < 0)
		nGroupIdx = 0;
	m_oMutex.Enter();
	if(pFiber->nFiberStatus == FOCP_FIBER_RUNNING)
	{
		pFiber->nFiberStatus = FOCP_FIBER_INITED;
		AdjustWorker(pFiber->pWorker, nGroupIdx, false);
	}
	m_oMutex.Leave();
}

void CFiberManager::SuspendFiber2(FOCP_DETAIL_NAME::CFiberData* pFiber, bool bLock)
{
	if(bLock)
		m_oMutex.Enter();
	if(pFiber->nFiberStatus == FOCP_FIBER_RUNNING)
	{
		pFiber->nFiberStatus = FOCP_FIBER_SLEEP;
		m_oSleepList.Push(pFiber);
	}
	if(bLock)
		m_oMutex.Leave();
}

void CFiberManager::ResumeFiber2(FOCP_DETAIL_NAME::CFiberData* pFiber, bool bLock)
{
	if(pFiber)
	{
		if(bLock)
			m_oMutex.Enter();
		if(pFiber->nFiberStatus != FOCP_FIBER_SLEEP)
		{
			if(bLock)
				m_oMutex.Leave();
			return;
		}
		m_oSleepList.Remove(pFiber);
		pFiber->nFiberStatus = FOCP_FIBER_READY;
		if(bLock)
			m_oMutex.Leave();
		pFiber->pWorker->PutInFiber(pFiber);
	}
}

void CFiberManager::ProcessTimer(uint32 nFiberId, uint32 nTimerId)
{
	CCooperator* pCooperator = QueryFiber(nFiberId);
	if(pCooperator == NULL)
		return;
	FOCP_DETAIL_NAME::CFiberData* pFiber = (FOCP_DETAIL_NAME::CFiberData*)pCooperator->GetCooperatorData();
	if(pFiber == NULL)
		return;
	m_oMutex.Enter();
	if(pFiber->nFiberStatus == FOCP_FIBER_SLEEP && pFiber->nTimerId == nTimerId)
	{
		pFiber->bTimeOut = true;
		pFiber->nTimerId = 0xFFFFFFFF;
		ResumeFiber2(pFiber, false);
	}
	m_oMutex.Leave();
}

void CFiberManager::AdjustWorker(FOCP_DETAIL_NAME::CFiberWorker* pWorker, int32 nGroupIdx, bool bAddFiber)
{
	if(bAddFiber)
	{
		++pWorker->m_nFiberCount;
		FOCP_DETAIL_NAME::CFiberWorker* pNext = pWorker->m_pNext;
		while(pNext && pNext->m_nFiberCount < pWorker->m_nFiberCount)
			pNext = pNext->m_pNext;
		if(pNext != pWorker->m_pNext)
		{
			m_pWorkerList[nGroupIdx].Remove(pWorker);
			m_pWorkerList[nGroupIdx].Insert(pNext, pWorker);
		}
	}
	else
	{
		--pWorker->m_nFiberCount;
		FOCP_DETAIL_NAME::CFiberWorker* pPrev = pWorker->m_pPrev;
		while(pPrev && pPrev->m_nFiberCount > pWorker->m_nFiberCount)
			pPrev = pPrev->m_pPrev;
		if(pPrev != pWorker->m_pPrev)
		{
			m_pWorkerList[nGroupIdx].Remove(pWorker);
			m_pWorkerList[nGroupIdx].Append(pPrev, pWorker);
		}
	}
}

bool CFiberManager::Start()
{
	int32 i, j;
	if(m_nStop != 3)
		return false;

	m_nWorkerCount = 0;
	for(i=0; i<m_nWorkerGroupCount; ++i)
	{
		m_pWorkerGroupIdx[i] = m_nWorkerCount;
		m_nWorkerCount += m_pWorkerGroupSize[i];
	}
	m_pWorkers = new FOCP_DETAIL_NAME::CFiberWorker[m_nWorkerCount];
	for(i=0, j=0; i<m_nWorkerGroupCount; ++i)
	{
		int32 nSize = m_pWorkerGroupSize[i]-1;
		while(nSize>=0)
		{
			m_pWorkerList[i].Push(m_pWorkers+j);
			++j;
			--nSize;
		}
	}

	m_nStop = 0;
	for(i=0; i<m_nWorkerCount; ++i)
	{
		if(!m_pWorkers[i].Start())
			return false;
	}

	if(!g_oReclaimFiber.Start())
		return false;

	return true;
}

void CFiberManager::Stop(bool bBlock)
{
	if(m_nStop == 0)
	{
		m_nStop = 1;
		g_oReclaimFiber.KillFiber(NULL);
		while(m_nStop!=2)
			CCooperator::Sleep(10);
		for(int32 i=0; i<m_nWorkerCount; ++i)
		{
			m_pWorkers[i].Stop(false);
			m_pWorkers[i].m_oEvent.Set();
		}
	}
	if(bBlock)
	{
		for(int32 i=0; i<m_nWorkerCount; ++i)
			m_pWorkers[i].Stop(true);
		m_nStop = 3;
	}
}

void CFiberManager::StopAllFiber()
{
	FOCP_DETAIL_NAME::CFiberData* pFiber;

	m_oMutex.Enter();
	pFiber = m_oSleepList.First();
	while(pFiber)
	{
		FOCP_DETAIL_NAME::CFiberData* pNext = m_oSleepList.GetNext(pFiber);
		pFiber->pCooperator->StopNotify();
		pFiber = pNext;
	}
	m_oMutex.Leave();

	m_nStop = 2;
}

void CFiberManager::StopFiber(CCooperator* pFiber)
{
	g_oReclaimFiber.KillFiber((FOCP_DETAIL_NAME::CFiberData*)pFiber->GetCooperatorData());
}

CFiber* CFiberManager::CreateFiber(uint32 nFiberKind)
{
	CFiber* pFiber = (CFiber*)m_oFactory.CreateObject(nFiberKind);
	return pFiber;
}

CCooperator* CFiberManager::QueryFiber(uint32 nFiberId)
{
	FOCP_DETAIL_NAME::CFiberData* pFiber = NULL;
	uint32 nIndex = (nFiberId >> m_nLeftBits);
	nFiberId = ( (nFiberId << m_nRightBits) >> m_nRightBits);
	pFiber = (FOCP_DETAIL_NAME::CFiberData*)m_oFiberContainer.QueryObject(nIndex);
	if( (pFiber == NULL) || (pFiber->nFiberStatus == FOCP_FIBER_INITED) ||
			(pFiber->nFiberId != nFiberId))
		return NULL;
	return pFiber->pCooperator;
}

CFiberManager* CFiberManager::GetInstance()
{
	return CSingleInstance<CFiberManager>::GetInstance();
}

CFactory* CFiberManager::GetFactory()
{
	CFiberManager* pFiberManager = GetInstance();
	return &pFiberManager->m_oFactory;
}

uint32 CFiberManager::SetTimer(uint32 nTimeOut, uint32 nFiberId)
{
	return CTimer::GetInstance()->SetTimer(nTimeOut, FiberTimeOut, (uint8*)&nFiberId, sizeof(nFiberId));
}

void* CFiberManager::GetFiberVariable(FOCP_DETAIL_NAME::CFiberData* pFiber, void* pKey)
{
	void * pRet = NULL;
	pFiber->oMutex.Enter();
	if(pFiber->pLocalStorage)
	{
		CRbTreeNode* pIt = pFiber->pLocalStorage->Find((char*)pKey);
		if(pIt != pFiber->pLocalStorage->End())
		{
			FOCP_DETAIL_NAME::CFiberContext &oContext = pFiber->pLocalStorage->GetItem(pIt);
			pRet = oContext.pData;
		}
	}
	pFiber->oMutex.Leave();
	return pRet;
}

void CFiberManager::SetFiberVariable(FOCP_DETAIL_NAME::CFiberData* pFiber, void* pKey, void* pData, FDestroyFiberPrivate Destroy)
{
	pFiber->oMutex.Enter();
	if(pFiber->pLocalStorage == NULL && pData)
		pFiber->pLocalStorage = new CRbMap<char*, FOCP_DETAIL_NAME::CFiberContext>;
	if(pFiber->pLocalStorage)
	{
		CRbTreeNode* pIt = pFiber->pLocalStorage->Find((char*)pKey);
		if(pIt != pFiber->pLocalStorage->End())
		{
			FOCP_DETAIL_NAME::CFiberContext &oContext = pFiber->pLocalStorage->GetItem(pIt);
			void* pOld = oContext.pData;
			if(pData)
			{
				if(pOld != pData)
				{
					if(oContext.Destroy)
						oContext.Destroy(pOld);
				}
				oContext.pData = pData;
				oContext.Destroy = Destroy;
			}
			else
				pFiber->pLocalStorage->Remove(pIt);
		}
		else if(pData)
		{
			FOCP_DETAIL_NAME::CFiberContext oContext = {pData, Destroy};
			pFiber->pLocalStorage->Insert((char*)pKey, oContext);
		}
	}
	pFiber->oMutex.Leave();
}

CThreadPool::~CThreadPool()
{
	Cleanup();
}

CThreadPool::CThreadPool()
{
	m_pCooperators = NULL;
	m_nPoolSize = 0;
	m_bRuning = false;
}

void CThreadPool::Cleanup()
{
	if(m_pCooperators)
	{
		for(uint32 i=0; i<m_nPoolSize; ++i)
			m_pCooperators[i].~CCooperator();
		CMalloc::Free(m_pCooperators);
	}
	m_pCooperators = NULL;
	m_nPoolSize = 0;
	m_bRuning = false;
}

uint32 CThreadPool::GetCooperatorIndex(CCooperator* pCooperator)
{
	if(pCooperator < m_pCooperators || pCooperator>=m_pCooperators+m_nPoolSize)
		return (uint32)(-1);
	return pCooperator - m_pCooperators;
}

void CThreadPool::Initialzie(uint32 nPoolSize, CCooperateFunction* pCooperateFunction)
{
	uint32 i;
	if(nPoolSize == 0)
		nPoolSize = 5;
	m_nPoolSize = nPoolSize;
	m_pCooperators = (CCooperator*)CMalloc::Malloc(m_nPoolSize*FOCP_SIZE_OF(CCooperator));
	for(i=0; i<m_nPoolSize; ++i)
		new(m_pCooperators+i) CCooperator(pCooperateFunction);
}

bool CThreadPool::Start()
{
	bool bRet = false;
	if(m_pCooperators)
	{
		m_bRuning = true;
		for(uint32 i=0; i<m_nPoolSize; ++i)
			if(m_pCooperators[i].Start())
				bRet = true;
	}
	return bRet;
}

void CThreadPool::Stop(bool bBlock)
{
	if(m_pCooperators)
	{
		uint32 i;
		if(m_bRuning)
		{
			m_bRuning = false;
			for(i=0; i<m_nPoolSize; ++i)
				m_pCooperators[i].Stop(false);
		}
		if(bBlock)
		{
			for(i=0; i<m_nPoolSize; ++i)
				m_pCooperators[i].Stop(true);
		}
	}
}

void* CFiberMsg::operator new(uintptr nSize)//port to 64bit
{
	return CBufferManager::GetInstance()->AllocateBuffer(nSize);
}

void CFiberMsg::operator delete(void* pMsg)
{
	CBufferManager::GetInstance()->DeAllocateBuffer(pMsg);
}

CFiberMsg::~CFiberMsg()
{
}

CFiberMsg::CFiberMsg()
{
	m_pNext = NULL;
}

CFiberMsgQueue::CFiberMsgQueue()
	:m_oEvent(false, false), m_oMsgQueue(FocpFieldOffset(CFiberMsg, m_pNext))
{
	m_pMsg = NULL;
}

CFiberMsgQueue::~CFiberMsgQueue()
{
	Clear();
}

void CFiberMsgQueue::SendMsg(CFiberMsg* pMsg, bool bAppend)
{
	if(pMsg == NULL)
		m_oEvent.Set();
	else if(bAppend)
	{
		m_oMutex.Enter();
		if(pMsg == m_pMsg)
			m_pMsg = NULL;
		if(m_oMsgQueue.GetSize() == 0)
			m_oEvent.Set();
		m_oMsgQueue.Push(pMsg);
		m_oMutex.Leave();
	}
	else
	{
		m_oMutex.Enter();
		if(pMsg == m_pMsg)
			m_pMsg = NULL;
		if(m_oMsgQueue.GetSize() == 0)
			m_oEvent.Set();
		m_oMsgQueue.Push(pMsg, false);
		m_oMutex.Leave();
	}
}

bool CFiberMsgQueue::DetachMsg(CFiberMsg* pMsg)
{
	if(pMsg == NULL)
		return true;
	m_oMutex.Enter();
	if(pMsg!=m_pMsg)
	{
		m_oMutex.Leave();
		return false;
	}
	m_pMsg = NULL;
	m_oMutex.Leave();
	return true;
}

int32 CFiberMsgQueue::GetMsg(uint32 nTimeOut, CFiberMsg* &pMsg, bool bDetach)
{
	int32 nRet;
	m_oEvent.Wait(nTimeOut);
	nRet = -1;
	m_oMutex.Enter();
	if(m_pMsg)
	{
		delete m_pMsg;
		m_pMsg = NULL;
	}
	pMsg = m_oMsgQueue.Pop();
	if(pMsg)
		nRet = 0;
	if(m_oMsgQueue.GetSize() == 0)
		m_oEvent.Reset();
	if(bDetach == false)
		m_pMsg = pMsg;
	m_oMutex.Leave();
	return nRet;
}

int32 CFiberMsgQueue::TopMsg(CFiberMsg* &pMsg)
{
	m_oMutex.Enter();
	pMsg = m_oMsgQueue.First();
	m_oMutex.Leave();
	return pMsg?0:-1;
}

void CFiberMsgQueue::Clear()
{
	CFiberMsg* pMsg;
	m_oMutex.Enter();
	while((pMsg=m_oMsgQueue.Pop()))
		delete pMsg;
	if(m_pMsg)
	{
		delete m_pMsg;
		m_pMsg = NULL;
	}
	m_oMutex.Leave();
	m_oEvent.Reset();
}

void CFiberMsgQueue::Lock()
{
	m_oMutex.Enter();
}

void CFiberMsgQueue::UnLock()
{
	m_oMutex.Leave();
}

CThreadVariablePool::CThreadVariablePool()
{
}

CThreadVariablePool::~CThreadVariablePool()
{
	if(m_oPool.GetSize())
		FocpAbort(("CThreadVariablePool::`CThreadVariablePool(), the pool isn't empty"));
}

CThreadVariablePool* CThreadVariablePool::GetInstance()
{
	return CSingleInstance< CThreadVariablePool >::GetInstance();
}

void * CThreadVariablePool::GetThreadVariable(void* pKey)
{
	void * pRet = NULL;
	CRbMap<char*, FOCP_DETAIL_NAME::CFiberContext>* pPool = NULL;
	threadid_t nTid = CCooperator::GetCurrentThreadId();

	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oPool.Find(nTid);
	pPool = &(m_oPool.GetItem(pIt));
	m_oMutex.Leave();

	if(pPool)
	{
		CRbTreeNode* pIt = pPool->Find((char*)pKey);
		if(pIt != pPool->End())
			pRet = pPool->GetItem(pIt).pData;
	}
	return pRet;
}

void CThreadVariablePool::SetThreadVariable(void* pKey, void* pData, FDestroyFiberPrivate Destroy)
{
	CRbMap<char*, FOCP_DETAIL_NAME::CFiberContext>* pPool;
	threadid_t nTid = CCooperator::GetCurrentThreadId();

	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oPool.Find(nTid);
	if(pIt == m_oPool.End())
		pPool = &(m_oPool[nTid]);
	else
		pPool = &(m_oPool.GetItem(pIt));
	m_oMutex.Leave();

	pIt = pPool->Find((char*)pKey);
	if(pIt != pPool->End())
	{
		FOCP_DETAIL_NAME::CFiberContext& oItem = pPool->GetItem(pIt);
		if(oItem.pData && oItem.Destroy)
			oItem.Destroy(oItem.pData);
		oItem.pData = pData;
		oItem.Destroy = Destroy;
	}
	else
	{
		FOCP_DETAIL_NAME::CFiberContext& oItem = (*pPool)[(char*)pKey];
		oItem.pData = pData;
		oItem.Destroy = Destroy;
	}
}

void CThreadVariablePool::ClearThreadVariable()
{
	CRbMap<char*, FOCP_DETAIL_NAME::CFiberContext>* pPool = NULL;
	threadid_t nTid = CCooperator::GetCurrentThreadId();

	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oPool.Find(nTid);
	pPool = &(m_oPool.GetItem(pIt));
	m_oMutex.Leave();

	if(pPool)
	{
		CRbTreeNode* pEnd = pPool->End();
		CRbTreeNode* pIdx = pPool->First();
		for(; pIdx!=pEnd; pIdx = pPool->GetNext(pIdx))
		{
			FOCP_DETAIL_NAME::CFiberContext& oItem = pPool->GetItem(pIdx);
			if(oItem.pData && oItem.Destroy)
				oItem.Destroy(oItem.pData);
		}
		m_oMutex.Enter();
		m_oPool.Remove(pIt);
		m_oMutex.Leave();
	}
}

void FiberTimeOut(uint8* msg, uint32 msglen, uint32 nTimerId)
{
	CFiberManager::GetInstance()->ProcessTimer(*(uint32*)msg, nTimerId);
}

APU_API void EnterSystemRead()
{
	CSingleInstance<CRwMutex>::GetInstance()->EnterRead();
}
APU_API void LeaveSystemRead()
{
	CSingleInstance<CRwMutex>::GetInstance()->LeaveRead();
}
APU_API void EnterSystemWrite()
{
	CSingleInstance<CRwMutex>::GetInstance()->EnterWrite();
}
APU_API void LeaveSystemWrite()
{
	CSingleInstance<CRwMutex>::GetInstance()->LeaveWrite();
}
FOCP_END();

FOCP_C_BEGIN();

#ifdef WINDOWS
FOCP_NAME::uint32 FOCP_CALL FocpThreadProc(void* lpParameter)
#endif
#ifdef UNIX
void* FocpThreadProc(void* lpParameter)
#endif
{
	FOCP_NAME::CCooperator* pThread = (FOCP_NAME::CCooperator*)lpParameter;
	pThread->CallThread();
	return NULL;
}

FOCP_C_END();
