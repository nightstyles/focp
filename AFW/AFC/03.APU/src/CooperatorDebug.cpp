
#include "Timer.hpp"
#include "LongJmpWin32.hpp"

#ifdef WINDOWS
#include <malloc.h>
#else
#include <alloca.h>
#endif

FOCP_BEGIN();

static CMutex g_oFiberScheduleMutex;
static FOCP_DETAIL_NAME::CFiberData* g_pFiber = NULL;

//本文件中的函数只能用全局函数实现

int32 CFiberManager::SuspendFiber(FOCP_DETAIL_NAME::CFiberData* pFiber, uint32 nTimeOut)
{
	CFiberManager* pFiberManager = CFiberManager::GetInstance();

	if(pFiberManager->m_oStackAttr.bCallDown)
		pFiber->pStackBottom = (ulong*)(void*)(&pFiberManager) + 1;
	else
		pFiber->pStackBottom = (ulong*)(void*)(&pFiberManager);

	pFiber->pCurBreak = pFiber->pFiberBreak;

#if defined(MSVC)
	if(FocpSetJmp(pFiber->pCurBreak->oBreak))
#else
	if(setjmp(pFiber->pCurBreak->oBreak))
#endif
	{
		//resume
		pFiber = g_pFiber;
		g_oFiberScheduleMutex.Leave();
		pFiberManager = CFiberManager::GetInstance();//需要重取
		if(pFiberManager->m_oStackAttr.bCallDown)//恢复栈
		
			ArrayCopy(pFiber->pStackBottom, pFiber->pStackBackup, pFiber->pStackTop - pFiber->pStackBottom);
		else
			ArrayCopy(pFiber->pStackTop, pFiber->pStackBackup, pFiber->pStackBottom - pFiber->pStackTop);
		if(pFiber->nTimerId != 0xFFFFFFFF)
		{
			CTimer::GetInstance()->KillTimer(pFiber->nTimerId);
			pFiber->nTimerId = 0xFFFFFFFF;
		}
		pFiber->pCurBreak = NULL;
		pFiber->pWorker->m_oCache.DeAllocateBuffer(pFiber->pStackBackup, false);
		pFiberManager->m_oMutex.Enter();
	}
	else
	{
		//suspend
		ulong nStackLen;
		pFiber->bTimeOut = false;
		if(pFiberManager->m_oStackAttr.bCallDown)
			nStackLen = pFiber->pStackTop - pFiber->pStackBottom;
		else
			nStackLen = pFiber->pStackBottom - pFiber->pStackTop;
		pFiber->pStackBackup = (ulong*)pFiber->pWorker->m_oCache.AllocateBuffer(nStackLen*sizeof(ulong), false);
		if(pFiberManager->m_oStackAttr.bCallDown)
			ArrayCopy(pFiber->pStackBackup, pFiber->pStackBottom, nStackLen);
		else
			ArrayCopy(pFiber->pStackBackup, pFiber->pStackTop, nStackLen);
		if(nTimeOut)
		{
			pFiberManager->SuspendFiber2(pFiber);
			if(nTimeOut != 0xFFFFFFFF)
			{
				uint32 nFiberId = pFiberManager->GetFiberId(pFiber);
				if(nFiberId == 0xFFFFFFFF)
					FocpAbort(("CFiberManager::SuspendFiber(), FiberId is invalid"));
				pFiber->nTimerId = pFiberManager->SetTimer(nTimeOut, nFiberId);
				if(pFiber->nTimerId == 0xFFFFFFFF)
					FocpAbort(("CFiberManager::SuspendFiber(), nTimerId is invalid "));
			}
		}
		else
			pFiber->pWorker->PutInFiber(pFiber);
		pFiberManager->m_oMutex.Leave();
#if defined(MSVC)
		FocpLongJmp(pFiber->pTopBreak->oBreak, 1);
#else
		longjmp(pFiber->pTopBreak->oBreak, 1);
#endif
	}
	if(pFiber->bTimeOut)
	{
		pFiber->bTimeOut = false;
		return -1;
	}
	return 0;
}

FOCP_DETAIL_BEGIN();

enum CFibeStatus
{
	FOCP_FIBER_INITED,
	FOCP_FIBER_READY,
	FOCP_FIBER_RUNNING,
	FOCP_FIBER_SLEEP
};

void CFiberWorker::Run(CFiberWorker* pWorker, CFiberData*& pCurFiber, CFiberManager* pFiberManager, bool &bRunning)
{
	CFiberData* pFiber;//该变量之后不能再有变量定义

#if defined(MSVC)
	FocpSetJmp(pWorker->m_pTopBreak->oBreak);
#else
	setjmp(pWorker->m_pTopBreak->oBreak);
#endif

	while(bRunning)
	{
		pFiber = pWorker->PullOutFiber();
		if(pFiber)
		{
			pCurFiber = pFiber;
			pFiber->nFiberStatus = FOCP_FIBER_RUNNING;
			if(pFiber->pCurBreak)
			{
				g_oFiberScheduleMutex.Enter();
				g_pFiber = pFiber;
#if defined(MSVC)
				FocpLongJmp(pFiber->pCurBreak->oBreak, 1);
#else
				longjmp(pFiber->pCurBreak->oBreak, 1);
#endif
			}
			else
			{
				pFiber->pTopBreak = pWorker->m_pTopBreak;
				if(pFiberManager->m_oStackAttr.bCallDown)
					pFiber->pStackTop = (ulong*)&pFiber;
				else
					pFiber->pStackTop = ((ulong*)&pFiber)+1;
				pFiber->pCooperator->MainProc(pFiber->bRunning);
				pFiberManager->UnBindFiber(pFiber);
				pFiberManager->FreeFiber(pFiber);
			}
		}
	}

	while(pWorker->m_nFiberCount || pFiberManager->m_oSleepList.GetSize())
	{
		pFiber = pWorker->PullOutFiber();
		if(pFiber)
		{
			pCurFiber = pFiber;
			pFiber->nFiberStatus = FOCP_FIBER_RUNNING;
			if(pFiber->pCurBreak)
			{
				g_oFiberScheduleMutex.Enter();
				g_pFiber = pFiber;
#if defined(MSVC)
				FocpLongJmp(pFiber->pCurBreak->oBreak, 1);
#else
				longjmp(pFiber->pCurBreak->oBreak, 1);
#endif
			}
			else
			{
				pFiberManager->UnBindFiber(pFiber);
				pFiberManager->FreeFiber(pFiber);
			}
		}
	}
}

FOCP_DETAIL_END();

FOCP_END();

