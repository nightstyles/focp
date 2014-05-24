
#include "ApuDef.hpp"

#ifndef _APU_SYNCOBJ_HPP_
#define _APU_SYNCOBJ_HPP_

FOCP_BEGIN();

class CCooperator;

typedef CInteger<FOCP_THREADID_SIZEOF, FOCP_THREADID_SIGN>::xint threadid_t;

class APU_API CMutex
{
	FOCP_FORBID_COPY(CMutex);
private:
	void* m_pMutex;

public:
	CMutex(bool bThread=true);
	~CMutex();

	void Enter();
	void Leave();
};

class APU_API CLock
{
private:
	CMutex& m_oMutex;

public:
	CLock(CMutex &oMutex);
	virtual ~CLock();
};

class APU_API CEvent
{
	FOCP_FORBID_COPY(CEvent);
private:
	void* m_pEvent;

public:
	CEvent(bool bAutoReset=false, bool bThread=true);
	~CEvent();

	bool Wait(uint32 nTimeOut=0xFFFFFFFF);

	void Set();
	void Reset();
};

class APU_API CSemaphore
{
	FOCP_FORBID_COPY(CSemaphore);
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	uint32 m_nSignal;

public:
	CSemaphore(uint32 nInitialSignal, bool bThread=true);
	~CSemaphore();

	bool Wait(uint32 nTimeOut=0xFFFFFFFF);
	void Release(uint32 nSignal=1);
};

class APU_API CChannel
{
	FOCP_FORBID_COPY(CChannel);
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CBits m_oBits;
	uint32 m_nSignal;

public:
	CChannel(uint32 nCount, bool bThread=true);
	~CChannel();

	bool Wait(uint32 &nChannel, uint32 nTimeOut=0xFFFFFFFF);

	void Set(uint32 nChannel);
	void Reset(uint32 nChannel);
};

class APU_API CRwMutex
{
private:
	union CCooperatorId
	{
		threadid_t nTid;
		CCooperator* pFiber;
	};
	struct CWaitNode
	{
		CCooperatorId oCid;
		bool bReader;
		CEvent* pEvent;
	};
	struct CReaderNode
	{
		CCooperatorId oCid;
		uint32 nCounter;
	};

	CMutex m_oMutex;
	CSingleList<CWaitNode> m_oWaitList;
	CSingleList<CReaderNode> m_oReaderList;
	uint32 m_nReadCounter, m_nWriteCounter, m_nWaitWriteCounter;
	CCooperatorId m_nWriter;
	bool m_bThread;

	CRwMutex(const CRwMutex&);
	CRwMutex& operator=(const CRwMutex&);

public:
	CRwMutex(bool bThread=true);
	~CRwMutex();

	void EnterRead();
	void LeaveRead();
	void EnterWrite();
	void LeaveWrite();

private:
	bool GetCurrentCooperator(CCooperatorId& oCoop);
	bool CidEqual(const CCooperatorId& l, const CCooperatorId& r);
};

template<typename TLockType> class CObjectLock
{
private:
	bool m_bThread;
	CMutex m_oMutex;
	CRbMap<TLockType, CRwMutex*> m_oLockPool;

public:
	CObjectLock(bool bThread=true)
		:m_bThread(bThread),m_oMutex(bThread)
	{
	}

	~CObjectLock()
	{
		DeleteLock();
	}

	CRwMutex* FindLock(TLockType nLockId)
	{
		CRwMutex* pMutex;
		m_oMutex.Enter();
		CRbTreeNode* idx = m_oLockPool.Find(nLockId);
		if(idx != m_oLockPool.End())
			pMutex = m_oLockPool.GetItem(idx);
		else
		{
			pMutex = new CRwMutex(m_bThread);
			m_oLockPool[nLockId] = pMutex;
		}
		m_oMutex.Leave();
		return pMutex;
	}

	void DeleteLock(TLockType nLockId)
	{
		m_oMutex.Enter();
		CRbTreeNode* idx = m_oLockPool.Find(nLockId);
		if(idx != m_oLockPool.End())
		{
			CRwMutex* pMutex = m_oLockPool.GetItem(idx);
			m_oLockPool.Remove(idx);
			delete pMutex;
		}
		m_oMutex.Leave();
	}

	void DeleteLock()
	{
		m_oMutex.Enter();
		CRbTreeNode* idx = m_oLockPool.First();
		CRbTreeNode* end = m_oLockPool.End();
		for(; idx!=end; ++idx)
		{
			CRwMutex* pMutex = m_oLockPool.GetItem(idx);
			delete pMutex;
		}
		m_oLockPool.Clear();
		m_oMutex.Leave();
	}
};

FOCP_END();

#endif
