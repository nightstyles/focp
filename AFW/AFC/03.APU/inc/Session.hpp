
#include "Allocator.hpp"

#ifndef _Afc_Session_Hpp_
#define _Afc_Session_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_SESSION_NORMAL=0,
	FOCP_SESSION_TIMEOUT=1,
	FOCP_SESSION_INVALID=2
};

class CSyncCallee;
class CSyncCaller;
class CAsyncCallee;
class CAsyncCaller;

class APU_API CSyncCallee
{
	FOCP_FORBID_COPY(CSyncCallee);
	friend class CSyncCaller;
private:
	CEvent m_oEvent, m_oLock;
	uint32 m_nStatus;
	uint32 m_nLocked;
	void* m_pObject;

public:
	CSyncCallee(void* pCallObject, bool bThread=true);
	~CSyncCallee();

	void* QueryCallObject();

	void Answer();
};

class APU_API CSyncCaller
{
private:
	CMutex m_oMutex;
	CRbMap<uint32, CSyncCallee*> m_oObjects;
	uint32 m_nSessionId;

public:
	CSyncCaller();
	virtual ~CSyncCaller();

	uint32 ApplyCall(CSyncCallee * pObject);
	uint32 WaitCall(uint32 nSessionId, uint32 nTimeOut);

	CSyncCallee* QueryCall(uint32 nSessionId);
	void ReleaseCall(CSyncCallee* pCallee);

	void AnswerAll(uint32 nStatus);
};

FOCP_DETAIL_BEGIN();
class CAsyncCalleeClass;
FOCP_DETAIL_END();

class APU_API CAsyncCallee
{
	FOCP_FORBID_COPY(CAsyncCallee);
	friend class CAsyncCaller;
	friend class FOCP_DETAIL_NAME::CAsyncCalleeClass;
private:
	CEvent m_oLock;
	uint32 m_nStatus;
	uint32 m_nLocked;
	uint32 m_nSessionId;
	void* m_pObject;
	CAsyncCallee* m_pNext;

public:
	CAsyncCallee();
	~CAsyncCallee();

	void* QueryCallObject();
};

FOCP_DETAIL_BEGIN();
class APU_API CAsyncCalleeClass: public CGeneralPyramidClass<CAsyncCallee>
{
private:
	CAsyncCaller* m_pCaller;

public:
	CAsyncCalleeClass(CAsyncCaller* pCaller);

	virtual uint32 GetObjectId(void* pObject) const;
	virtual void SetObjectId(void* pBuf, uint32 nId) const;
};
FOCP_DETAIL_END();

class APU_API CAsyncCaller
{
	friend class FOCP_DETAIL_NAME::CAsyncCalleeClass;
private:
	CMutex m_oMutex;
	CBaseSingleList<CAsyncCallee> m_oQueue;
	CEvent m_oEvent;
	FOCP_DETAIL_NAME::CAsyncCalleeClass m_oCalleeClass;
	CPyramidAllocator m_oObjects;
	uint32 m_nCapacity, m_nRightBits, m_nMaxCounter;

public:
	CAsyncCaller(uint32 nCapacity, bool bThread=true);
	virtual ~CAsyncCaller();

	uint32 ApplyCall(void* pCallObject, uint32 nTimeOut);
	uint32 PullCall(uint32 &nSessionId, void* &pCallObject);

	CAsyncCallee* QueryCall(uint32 nSessionId);
	void AnswerCall(CAsyncCallee* pCallee, bool bTimeOut=false);
	void ReleaseCall(CAsyncCallee* pCallee);

	uint32 GetSize();
};

FOCP_END();

#endif
