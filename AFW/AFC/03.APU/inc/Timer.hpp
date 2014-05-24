
#include "Cooperator.hpp"

#ifndef _APU_TIMER_HPP_
#define _APU_TIMER_HPP_

#define FOCP_TIMER_PRIVATE_DATA_LEN	32

FOCP_BEGIN();

typedef void (*FTimeOutFunction)(uint8* msg, uint32 msglen, uint32 nTimerId);

FOCP_DETAIL_BEGIN();

struct APU_API CTimerItem: public CObject
{
	CRbTreeNode oNode;
	FTimeOutFunction Function;
	uint32 nTimeOut;
	uint32 nDataLen;
	uint8 pData[FOCP_TIMER_PRIVATE_DATA_LEN];
	CTimerItem();
};

struct APU_API CQueryTimerKey
{
	static const uint32* QueryKey(const CRbTreeNode* pNode);
};

FOCP_DETAIL_END();

class APU_API CTimer: public CCooperateFunction
{
private:
	CMutex m_oMutex;
	CBaseRbTree<uint32/*nEndTime*/, FOCP_DETAIL_NAME::CQueryTimerKey> m_oTimerTable;
	CCommonClass<FOCP_DETAIL_NAME::CTimerItem> m_oAllocator;
	CEvent m_oEvent;

public:
	CTimer();
	virtual ~CTimer();

	static CTimer* GetInstance();

	bool Initialize(uint32 nCapacity);
	void Cleanup();

	uint32 SetTimer(uint32 nElapse, FTimeOutFunction Function, uint8* pData, uint32 nDataLen);
	bool KillTimer(uint32 nTimerId);

	static uint32 GetTickCount();
	static uint32 GetTime();

protected:
	virtual void ProcessOnce(CCooperator* pThread, bool &bRunning);
};

FOCP_END();

#endif //_Afc_Timer_Hpp_
