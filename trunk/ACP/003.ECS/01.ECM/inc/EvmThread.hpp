
#include "EvmCpu.hpp"

#ifndef _EVM_THREAD_HPP_
#define _EVM_THREAD_HPP_

FOCP_BEGIN();

class CEvmMutex;
class CEvmEvent;
class CEvmWaitor;
class CEvmThread;
class CEvmThreadManager;

class EVM_API CEvmMutex
{
private:
	CEvmThread* m_pCur;
	CMutex m_oMutex;
	CBaseSingleList<CEvmThread> m_oWaitors;
	uint32 m_nRecursion;

public:
	CEvmMutex();
	~CEvmMutex();

	void Enter(void* &reg);
	void Leave(void* &reg);
};

class EVM_API CEvmEvent
{
private:
	CMutex m_oMutex;
	CBaseSingleList<CEvmThread> m_oWaitors;
	bool m_bManualReset, m_bSignal;

public:
	CEvmEvent(bool bAutoReset=false);
	~CEvmEvent();

	void Wait(void* &reg, uint32 nTimeOut=0xFFFFFFFF);

	void Set();
	void Reset();
};

class EVM_API CEvmWaitor
{
private:
	CMutex m_oMutex;
	CBaseSingleList<CEvmThread> m_oWaitors;
	ehc_uint m_nCounter;

public:
	CEvmWaitor();
	~CEvmWaitor();

	bool Wait(void* reg, ehc_int& nRet, uint32 nTimeOut=0xFFFFFFFF);
};

class EVM_API CEvmThread
{
	friend class CEvmMutex;
	friend class CEvmEvent;
	friend class CEvmWaitor;
private:
	void* m_pReg;
	CEvmThread* m_pPrev, *m_pNext, *m_pWait;
	ehc_uint m_nTid, m_nTimerId;
	bool m_bTimeOut;

public:
	CEvmThread();
	~CEvmThread();

	ehc_void Suspend(void* &reg, uint32 nTimeOut);
	ehc_void Resume(bool bTimeOut=false);
	bool IsTimeOut();
};

class EVM_API CEvmThreadManager
{
	friend class CEvmMutex;
	friend class CEvmEvent;
private:
	CMutex m_oMutex;
	ehc_uint m_nTid;
	CRbTree<ehc_uint, CEvmThread*> m_oThreads;

public:
	CEvmThreadManager();
	~CEvmThreadManager();

	static CEvmThreadManager* GetInstance();

	CEvmThread* CreateThread(CEvmProcess* pProc, ehc_char* pFun, ehc_void* pPara);
};

FOCP_END();

#endif
