
#include "Object.hpp"

#ifndef _APU_COOPERATOR_HPP_
#define _APU_COOPERATOR_HPP_

//原型描述

FOCP_C_BEGIN();

typedef void (* FDestroyFiberPrivate)(void* pPrivate);

#ifdef WINDOWS
FOCP_NAME::uint32 FOCP_CALL FocpThreadProc(void* lpParameter);
#endif

#ifdef UNIX
void* FocpThreadProc(void* lpParameter);
#endif

FOCP_C_END();

FOCP_BEGIN();
class CCooperateFunction;
class CCooperator;
class CThreadPool;
class CFiber;
class CFiberManager;
struct CBreak;
template<typename TFiber> class CFiberClass;
template<typename TData> class CThreadVariable;
template<typename TData> class CFiberVariable;
FOCP_DETAIL_BEGIN();
struct CFiberContext;
struct CFiberData;
class CFiberDataClass;
class CFiberWorker;
class CFiberReclaim;
FOCP_DETAIL_END();
void FiberTimeOut(uint8* msg, uint32 msglen, uint32 nTimerId);
FOCP_END();

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

struct CFiberContext
{
	void* pData;
	FDestroyFiberPrivate Destroy;
};

struct CFiberData
{
	CMutex oMutex;
	CCooperator* pCooperator;
	CFiberData *pPrev, *pNext, *pWait;

	CRbMap<char*, CFiberContext>* pLocalStorage;

	CBreak* pFiberBreak, *pCurBreak, *pTopBreak;
	ulong *pStackTop, *pStackBottom, *pStackBackup;
	CFiberWorker* pWorker;
	uint32 nFiberStatus;
	uint32 nTimerId, nFiberId;
	bool bTimeOut, bRunning;
};

class APU_API CFiberDataClass: public CGeneralClass<CFiberData>
{
public:
	virtual void ResetObject(void* pObject) const;
	virtual void ConstructInstance(void* pBuf) const;
	virtual void DestructInstance(void* pObject) const;
};

FOCP_DETAIL_END();

class APU_API CCooperateFunction
{
	friend class CCooperator;
public:
	CCooperateFunction();
	virtual ~CCooperateFunction();

protected:
	virtual void StopNotify(CCooperator* pCooperator);
	virtual void MainProc(CCooperator* pCooperator, bool &bRunning);
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);
	virtual void Release(CCooperator* pCooperator);
};

class APU_API CCooperator
{
	FOCP_FORBID_COPY(CCooperator);

#ifdef WINDOWS
	friend uint32 FOCP_CALL ::FocpThreadProc(void* lpParameter);
#endif
#ifdef UNIX
	friend void* ::FocpThreadProc(void* lpParameter);
#endif
	friend class CCooperateFunction;
	friend class CFiberManager;
	friend class FOCP_DETAIL_NAME::CFiberWorker;
	friend class FOCP_DETAIL_NAME::CFiberDataClass;
	friend class FOCP_DETAIL_NAME::CFiberReclaim;
	friend class CThreadPool;

private:
	CCooperateFunction* m_pFunction;
	bool m_bThread;
	void* m_pData;

public:
    //bNeedRelease仅针对bThread=false有效，并且CCooperator对象必须是全局的
	CCooperator(CCooperateFunction* pFunction=NULL, bool bThread=true, bool bNeedRelease=true);
	virtual ~CCooperator();

	bool Start();
	void Stop(bool bBlock=true);

	bool IsThread();
	bool IsFiber();
	void* GetCooperatorData();

	threadid_t GetThreadId();
	uint32 GetFiberId();

	void SetFunction(CCooperateFunction* pFunction);

	static threadid_t GetCurrentThreadId();

	static CCooperator* GetCurrentCooperator(bool bDefault=true, bool bThread=true);//返回当前的纤程或线程
	static void Sleep(uint32 nTimeOut, bool bDefault=true, bool bThread=true);

#ifdef UNIX
	// for thread signal management
	void Kill(int32 nSignal);
	static void OpenSignal(int32 nSignal, void (*pSignaler)(int)=NULL);
	static void CloseSignal(int32 nSignal);
#endif

#ifdef WINDOWS
	static uint32 GetMinSleep();
#endif

protected:
	virtual void StopNotify();
	virtual void MainProc(bool &bRunning);
	virtual void ProcessOnce(bool &bRunning);
	virtual void Release();
	virtual int32 GetWorkerGroupId();//for fiber

private:
	void CallThread();// for thread

	void DoSleep(uint32 nTimeOut);
	static void ThreadSleep(uint32 nTimeOut);
};

//注意基类的顺序，不能搞反了，因为从工厂分配的对象类型是CObject，
//而实际是CFiber类型，只有CObject在前面，才和CFiber拥有相同地址。
//CFiber的派生类中，CFiber也必须是第一个基类
class APU_API CFiber: public CObject, public CCooperator
{
public:
	virtual ~CFiber();
	CFiber();

protected:
	virtual void Release();
};

template<uint32 nClassId, uint32 nGroupId=0> class CCommonFiber: public CFiber
{
public:
	inline virtual ~CCommonFiber(){};
	inline CCommonFiber(){};
	inline uint32 GetClassId() const{return nClassId;}
	inline int32 GetWorkerGroupId(){return nGroupId;}
};

FOCP_DETAIL_BEGIN();
class APU_API CFiberReclaim: public CCooperator
{
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CSingleList<CFiberData*> m_oQueue;

public:
	CFiberReclaim();
	void KillFiber(CFiberData* pFiber);

protected:
	virtual void ProcessOnce(bool &bRunning);
};
FOCP_DETAIL_END();

class APU_API CFiberManager
{
	friend class CCooperateFunction;
	friend class CCooperator;
	friend class FOCP_DETAIL_NAME::CFiberWorker;
	friend class FOCP_DETAIL_NAME::CFiberReclaim;
	friend class CMutex;
	friend class CEvent;
	friend class CSemaphore;
	friend class CChannel;
	friend class CRwMutex;

	friend void FiberTimeOut(uint8* msg, uint32 msglen, uint32 nTimerId);

private:
	CMutex m_oMutex;

	//Fiber Container
	CFactory m_oFactory;
	CAllocator m_oFiberContainer;
	uint32 m_nMaxFiberCount;
	uint16 m_nLeftBits, m_nRightBits, m_nMaxFiberIdBits;
	uint32 m_nMaxFiberId;

	//Worker Manager
	uint32 m_nStop;
	FOCP_DETAIL_NAME::CFiberWorker* m_pWorkers;
	int32 m_nWorkerCount, m_nWorkerGroupCount;
	int32* m_pWorkerGroupSize;
	int32* m_pWorkerGroupIdx;
	CBaseDoubleList<FOCP_DETAIL_NAME::CFiberWorker>* m_pWorkerList;

	//SleepList
	CBaseDoubleList<FOCP_DETAIL_NAME::CFiberData> m_oSleepList;

	//CStackAttr
	CStackAttr m_oStackAttr;

public:
	CFiberManager();
	virtual ~CFiberManager();

	bool Initialize(uint32 nMaxFiberIdBits, int32 nWorkerGroupCount);
	void Cleanup();

	bool SetWorkerCount(int32 nWorkerGroupId, int32 nWorkerCount);

	bool Start();
	void Stop(bool bBlock=true);
	void StopFiber(CCooperator* pFiber);

	CCooperator* QueryFiber(uint32 nFiberId);
	CFiber* CreateFiber(uint32 nClassId);

	static CFiberManager* GetInstance();
	static CFactory* GetFactory();

	static int32 SuspendFiber(FOCP_DETAIL_NAME::CFiberData* pFiber, uint32 nTimeOut);

	void* GetFiberVariable(FOCP_DETAIL_NAME::CFiberData* pFiber, void* pKey);
	void SetFiberVariable(FOCP_DETAIL_NAME::CFiberData* pFiber, void* pKey, void* pData, FDestroyFiberPrivate Destroy);

private:
	uint32 GetFiberId(void* pFiber);

	FOCP_DETAIL_NAME::CFiberData* AllocFiber();
	void FreeFiber(FOCP_DETAIL_NAME::CFiberData* pFiber);
	void BindFiber(FOCP_DETAIL_NAME::CFiberData* pFiber);
	void UnBindFiber(FOCP_DETAIL_NAME::CFiberData* pFiber);
	void SuspendFiber2(FOCP_DETAIL_NAME::CFiberData* pFiber, bool bLock=true);
	void ResumeFiber2(FOCP_DETAIL_NAME::CFiberData* pFiber, bool bLock=true);

	void AdjustWorker(FOCP_DETAIL_NAME::CFiberWorker* pWorker, int32 nGroupIdx, bool bAddFiber);

	uint32 SetTimer(uint32 nTimeOut, uint32 nFiberId);
	void ProcessTimer(uint32 nFiberId, uint32 nTimerId);

	void StopAllFiber();
};

class APU_API CThreadPool
{
private:
	bool m_bRuning;
	uint32 m_nPoolSize;
	CCooperator* m_pCooperators;

	CThreadPool(const CThreadPool&);
	CThreadPool& operator=(const CThreadPool&);

public:
	~CThreadPool();
	CThreadPool();

	uint32 GetCooperatorIndex(CCooperator* pCooperator);

	void Initialzie(uint32 nPoolSize, CCooperateFunction* pCooperateFunction);
	void Cleanup();

	bool Start();
	void Stop(bool bBlock=true);
};

template<typename TFiber> class CFiberClass: public CCommonClass<TFiber>
{
private:
	bool m_bDefault, m_bForbidCustom;
	uint32 m_nCapacity;

public:
	inline CFiberClass()
		:CCommonClass<TFiber>(CFiberManager::GetFactory())
	{
		m_bDefault = false;
		m_bForbidCustom = false;
		m_nCapacity = 0;
	}

	inline CFiberClass(uint32 nCapacity, bool bForbidCustomPolicy)
		:CCommonClass<TFiber>(CFiberManager::GetFactory())
	{
		m_bDefault = true;
		m_nCapacity = nCapacity;
		m_bForbidCustom = bForbidCustomPolicy;
	}

protected:
	inline virtual bool SetAllocatePolicy(const CAllocatePolicy &oPolicy)
	{
		return false;
	}

	inline virtual bool SetCapacity(uint32 nCapacity)
	{
		if(nCapacity == 0)
			nCapacity = 1024;
		CAllocatePolicy oPolicy = {nCapacity, nCapacity, nCapacity};
		return CClass::SetAllocatePolicy(oPolicy);
	}

	inline virtual bool GetDefaultPolicy(CAllocatePolicy &oPolicy, bool &bForbidCustomPolicy)
	{
		if(m_bDefault)
		{
			oPolicy.nInitializeObjectNum = m_nCapacity;
			oPolicy.nMaxObjectNum = m_nCapacity;
			oPolicy.nRecycleSize = m_nCapacity;
			bForbidCustomPolicy = m_bForbidCustom;
		}
		return m_bDefault;
	}
};

class APU_API CFiberMsg
{
	friend class CFiberMsgQueue;
private:
	CFiberMsg* m_pNext;

	CFiberMsg(const CFiberMsg& oMsg);
	CFiberMsg& operator=(const CFiberMsg& oMsg);

public:
	//为兼容32位和64位系统，参数采用uintptr类型
	void* operator new(uintptr nSize);
	void operator delete(void* pMsg);

	virtual ~CFiberMsg();
	CFiberMsg();
};

class APU_API CFiberMsgQueue
{
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CBaseSingleList<CFiberMsg> m_oMsgQueue;
	CFiberMsg* m_pMsg;

public:
	CFiberMsgQueue();
	virtual ~CFiberMsgQueue();

	void SendMsg(CFiberMsg* pMsg, bool bAppend=true);

	int32 GetMsg(uint32 nTimeOut, CFiberMsg* &pMsg, bool bDetach=false);
	int32 TopMsg(CFiberMsg* &pMsg);

	bool DetachMsg(CFiberMsg* pMsg);

	void Clear();

	void Lock();
	void UnLock();
};

template<typename TMsg> class CMsgQueue
{
private:
	CQueue<TMsg> m_oQueue;
	CMutex m_oMutex;
	CEvent m_oPopEvent, m_oPushEvent;

	CMsgQueue(const CMsgQueue<TMsg>& oQueue);
	CMsgQueue<TMsg>& operator=(const CMsgQueue<TMsg>& oQueue);

public:
	inline CMsgQueue(bool bThread, uint32 nCapacity=0)
		:m_oQueue(nCapacity),m_oMutex(bThread),m_oPushEvent(false, bThread),m_oPopEvent(false, bThread)
	{
		m_oPushEvent.Set();
	}
	inline ~CMsgQueue()
	{
	}

	//可指定从尾部加还是头加
	//-1: timeout, 0: normal
	inline int32 Push(const TMsg& oMsg, uint32 nTimeOut=1000, bool bAppend=true)
	{
		if(nTimeOut)
			Wait(nTimeOut, true);
		Lock();
		bool bRet = m_oQueue.Push(oMsg, bAppend);
		if(bRet)
		{
			uint32 nSize = m_oQueue.GetSize();
			if(nSize == 1)
				Set(false);
			else if(nSize == m_oQueue.GetCapacity())
				Reset(true);
		}
		UnLock();
		return bRet?0:-1;
	}

	//可指定从尾部还是头部取对象
	//-1: timeout, 0: normal
	inline int32 Pop(TMsg& oMsg, uint32 nTimeOut=1000, bool bFromHead=true)
	{
		if(nTimeOut)
			Wait(nTimeOut, false);
		Lock();
		bool bRet = m_oQueue.Pop(oMsg, bFromHead);
		if(bRet)
		{
			uint32 nSize = m_oQueue.GetSize();
			if(nSize == 0)
				Reset(false);
			else if(nSize == m_oQueue.GetCapacity()-1)
				Set(true);
		}
		UnLock();
		return bRet?0:-1;
	}

	//获取顶部消息
	inline TMsg* TopMsg(uint32 nTimeOut=1000)
	{
		if(nTimeOut)
			Wait(nTimeOut, false);
		Lock();
		TMsg* pMsg = m_oQueue.At(0);
		UnLock();
		return pMsg;
	}

	//Push & Pop & TopMsg已经自动加锁，提供该函数的目的是应用可以共享该消息队列的锁资源。
	inline void Lock()
	{
		m_oMutex.Enter();
	}
	inline void UnLock()
	{
		m_oMutex.Leave();
	}

private:
	inline int32 Wait(uint32 nTimeOut, bool bPush)
	{
		if(bPush)
			return m_oPushEvent.Wait(nTimeOut)?0:-1;
		else
			return m_oPopEvent.Wait(nTimeOut)?0:-1;
	}
	inline void Set(bool bPush)
	{
		if(bPush)
			m_oPushEvent.Set();
		else
			m_oPopEvent.Set();
	}
	inline void Reset(bool bPush)
	{
		if(bPush)
			m_oPushEvent.Reset();
		else
			m_oPopEvent.Reset();
	}
};

class APU_API CThreadVariablePool
{
private:
	CMutex m_oMutex;
	CRbMap< threadid_t, CRbMap<char*, FOCP_DETAIL_NAME::CFiberContext> > m_oPool;

public:
	CThreadVariablePool();
	~CThreadVariablePool();

	static CThreadVariablePool* GetInstance();

	void * GetThreadVariable(void* pKey);
	void SetThreadVariable(void* pKey, void* pData, FDestroyFiberPrivate Destroy);

	void ClearThreadVariable();
};

template<typename TData> class CThreadVariable
{
private:
	CThreadVariable(const CThreadVariable<TData> &oSrc);
	CThreadVariable(const TData &oSrc);

public:
	inline CThreadVariable()
	{
		GetVariable();
	}

	inline ~CThreadVariable()
	{
	}

	inline CThreadVariable<TData>& operator=(const CThreadVariable<TData> &oSrc)
	{
		if(this != &oSrc)
			GetVariable() = oSrc.GetVariable();
		return *this;
	}

	inline CThreadVariable<TData>& operator=(const TData &oSrc)
	{
		GetVariable() = oSrc;
		return *this;
	}

	inline operator TData&()
	{
		return GetVariable();
	}

private:
	inline TData& GetVariable()
	{
		CThreadVariablePool* pPool = CThreadVariablePool::GetInstance();
		TData* pData = (TData*)pPool->GetThreadVariable(this);
		if(pData == NULL)
		{
			pData = new TData();
			pPool->SetThreadVariable(this, pData, &CThreadVariable::Destory);
		}
		return *pData;
	}
	inline static void Destory(void* pPrivate)
	{
		delete (TData*)pPrivate;
	}
};

template<typename TData> class CFiberVariable
{
private:
	CFiberVariable(const CFiberVariable<TData> &oSrc);
	CFiberVariable(const TData &oSrc);

public:
	inline CFiberVariable()
	{
	}

	inline ~CFiberVariable()
	{
	}

	inline CFiberVariable<TData>& operator=(const CFiberVariable<TData> &oSrc)
	{
		if(this != &oSrc)
			GetVariable() = oSrc.GetVariable();
		return *this;
	}

	inline CFiberVariable<TData>& operator=(const TData &oSrc)
	{
		GetVariable() = oSrc;
		return *this;
	}

	inline operator TData&()
	{
		return GetVariable();
	}

private:
	inline TData& GetVariable()
	{
		CCooperator* pFiber = CCooperator::GetCurrentCooperator(false, false);
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		FOCP_DETAIL_NAME::CFiberData* pFiberData = (FOCP_DETAIL_NAME::CFiberData*)pFiber->GetCooperatorData();
		TData* pData = (TData*)pFiberManager->GetFiberVariable(pFiberData, this);
		if(pData == NULL)
		{
			pData = new TData();
			pFiberManager->SetFiberVariable(pFiberData, this, pData, &CFiberVariable::Destory);
		}
		return *pData;
	}

	inline static void Destory(void* pPrivate)
	{
		delete (TData*)pPrivate;
	}
};

template<typename TData> class CPrivateVariable
{
private:
	CThreadVariable<TData> m_oThreadVariable;
	CFiberVariable<TData> m_oFiberVariable;

public:
	inline CPrivateVariable()
	{
	}

	inline ~CPrivateVariable()
	{
	}

	inline void Clear()
	{
		m_oThreadVariable.Clear();
		m_oFiberVariable.Clear();
	}

	inline CPrivateVariable<TData>& operator=(const CPrivateVariable<TData> &oSrc)
	{
		if(this != &oSrc)
			GetVariable() = oSrc.GetVariable();
		return *this;
	}

	inline CPrivateVariable<TData>& operator=(const TData &oSrc)
	{
		GetVariable() = oSrc;
		return *this;
	}

	inline operator TData&()
	{
		return GetVariable();
	}

private:
	TData& GetVariable()
	{
		if(CCooperator::GetCurrentCooperator())
			return m_oFiberVariable;
		else
			return m_oThreadVariable;
	}
};

FOCP_DETAIL_BEGIN();

class APU_API CFiberWorker: public CCooperator
{
public:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CBreak* m_pTopBreak;
	CBaseSingleList<CFiberData> m_oReadyQueue;
	uint32 m_nFiberCount;
	CFiberWorker* m_pPrev, *m_pNext;
	CBufferManager m_oCache;

public:
	CFiberWorker();
	~CFiberWorker();

	void PutInFiber(CFiberData* pFiber);
	CFiberData* PullOutFiber();

protected:
	virtual void MainProc(bool &bRunning);

private:
	static void Run(CFiberWorker* pWorker, CFiberData*& pCurFiber, CFiberManager* pFiberManager, bool &bRunning);
};

FOCP_DETAIL_END();

template<typename TObject, typename TKey=uint32>class CObjectRegistry
{
	struct CItem
	{
		TObject* pItem;
		uint32 nCounter;
	};
private:
	CMutex m_oMutex;
	CRbMap<TKey, CItem> m_oRegistry;

public:
	inline CObjectRegistry()
	{
	}

	inline ~CObjectRegistry()
	{
		if(m_oRegistry.GetSize())
			FocpAbort(("CObjectRegistry::m_oRegistry isn't empty"));
	}

	inline bool Register(const TKey& oKey, TObject* pObject)
	{
		if(!pObject)
			return false;
		bool bRet = false;
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRegistry.Find(oKey);
		if(pIt == m_oRegistry.End())
		{
			bRet = true;
			CItem & oItem = m_oRegistry[oKey];
			oItem.pItem = pObject;
			oItem.nCounter = 0;
		}
		m_oMutex.Leave();
		return bRet;
	}

	inline TObject* DeRegister(const TKey& oKey)
	{
		TObject* pObject = NULL;
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRegistry.Find(oKey);
		if(pIt != m_oRegistry.End())
		{
			CItem &oItem = m_oRegistry.GetItem(pIt);
			pObject = oItem.pItem;
			while(oItem.nCounter)
			{
				m_oMutex.Leave();
				CCooperator::Sleep(100);
				m_oMutex.Enter();
			}
			m_oRegistry.Remove(pIt);
		}
		m_oMutex.Leave();
		return pObject;
	}

	inline TObject* Query(const TKey& oKey)
	{
		TObject* pObject = NULL;
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRegistry.Find(oKey);
		if(pIt != m_oRegistry.End())
		{
			CItem &oItem = m_oRegistry.GetItem(pIt);
			pObject = oItem.pItem;
			++oItem.nCounter;
		}
		m_oMutex.Leave();
		return pObject;
	}

	inline void Release(const TKey& oKey)
	{
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRegistry.Find(oKey);
		if(pIt != m_oRegistry.End())
		{
			CItem &oItem = m_oRegistry.GetItem(pIt);
			if(oItem.nCounter)
				--oItem.nCounter;
		}
		m_oMutex.Leave();
	}

	inline void Walk(void* pPara, void (*OnWalk)(TObject* pObject, void* pPara))
	{
		CRbTreeNode* pEnd = m_oRegistry.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oRegistry.First();
		for(; pIt!=pEnd; pIt=m_oRegistry.GetNext(pIt))
		{
			CItem &oItem = m_oRegistry.GetItem(pIt);
			OnWalk(oItem.pItem, pPara);
		}
		m_oMutex.Leave();
	}
};

FOCP_END();

#endif
