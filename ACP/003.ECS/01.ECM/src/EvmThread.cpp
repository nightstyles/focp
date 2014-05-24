
#include "EvmThread.hpp"

FOCP_BEGIN();

CEvmMutex::CEvmMutex():
	m_oWaitors(FocpFieldOffset(FOCP_DETAIL_NAME::CEvmThread, m_pWait)
{
	m_pCur = NULL;
	m_nRecursion = 0;
}

CEvmMutex::~CEvmMutex()
{
}

void CEvmMutex::Enter(void* &reg)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmThread* pThread = (CEvmThread*)pReg->CP;
	pThread->m_pWait = NULL;
	m_oMutex.Enter();
	if(m_pCur == NULL || m_pCur == pThread)
	{
		m_pCur = pThread;
		++m_nRecursion;
	}
	else
	{
		m_oWaitors.Push(pThread);
		pThread->Suspend(reg, 0xFFFFFFFF);
	}
	m_oMutex.Leave();
}

void CEvmMutex::Leave(void* &reg)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmThread* pThread = (CEvmThread*)pReg->CP;
	m_oMutex.Enter();
	if(m_pCur == pThread)
	{
		--m_nRecursion;
		if(m_nRecursion == 0)
		{
			m_pCur = m_oWaitors.Pop();
			if(m_pCur)
			{
				m_nRecursion = 1;
				m_pCur->Resume();
			}
		}
	}
	m_oMutex.Leave();
}

CEvmEvent::CEvmEvent(bool bAutoReset):
	m_oWaitors(FocpFieldOffset(FOCP_DETAIL_NAME::CEvmThread, m_pWait)
{
	m_bSignal = false;
	m_bManualReset = !bAutoReset;
}

CEvmEvent::~CEvmEvent()
{
}

void CEvmEvent::Wait(void* &reg, uint32 nTimeOut)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmThread* pThread = (CEvmThread*)pReg->CP;
	m_oMutex.Enter();
	if(m_bSignal)
	{
		if(m_bManualReset == false)
			m_bSignal = false;
	}
	else
	{
		m_oWaitors.Push(pThread);
		pThread->Suspend(reg, nTimeOut);
	}
	m_oMutex.Leave();
}

void CEvmEvent::Set()
{
	CEvmThead* pThread;
	m_oMutex.Enter();
	if(m_bSignal == false)
	{
		m_bSignal = true;
		while(m_bSignal && (pThread=m_oWaitors.Pop()))
		{
			if(m_bManualReset == false)
				m_bSignal = false;
			pThread->Resume();
		}
	}
	m_oMutex.Leave();
}

void CEvmEvent::Reset()
{
	m_oMutex.Enter();
	if(m_bSignal)
		m_bSignal = false;
	m_oMutex.Leave();
}

ehc_void CEvmThread::Suspend(void* &reg, uint32 nTimeOut)
{
	CEvmThreadManager* pThreadManager = CEvmThreadManager::GetInstance();
	pThreadManager->m_oMutex.Enter();
	m_pReg = reg;//保存寄存器到当前的线程
	reg = NULL;//当前的线程被挂起
	m_nTimerId = 0xFFFFFFFF;
	m_bTimeOut = false;
	if(nTimeOut == 0)
		Resume();
	else if(nTimeOut != 0xFFFFFFFF)
	{
		m_nTimerId = pThreadManager->SetTimer(nTimeOut, m_nTid);
		if(m_nTimerId == 0xFFFFFFFF)
			FocpAbort(("CEvmThread::Suspend(), nTimerId is invalid "));
	}
	pThreadManager->m_oMutex.Leave();
}

ehc_void CEvmThread::Resume(bool bTimeOut)
{
	CEvmThreadManager* pThreadManager = CEvmThreadManager::GetInstance();
	pThreadManager->m_oMutex.Enter();
	if(m_pReg)
	{
		void* reg = m_pReg;
		m_pReg = NULL;
		m_bTimeOut = bTimeOut;
		if(m_nTimerId != 0xFFFFFFFF)
			pThreadManager->KillTimer(m_nTimerId);
		m_nTimerId = 0xFFFFFFFF;
		CEvmCpu::GetInstance()->Resume(reg);
	}
	pThreadManager->m_oMutex.Leave();
}

bool CEvmThread::IsTimeOut()
{
	bool bTimeOut = m_bTimeOut;
	m_bTimeOut = false;
	return bTimeOut;
}

FOCP_END();
