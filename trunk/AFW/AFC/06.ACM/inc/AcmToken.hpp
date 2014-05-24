
#include "AcmUdp.hpp"

#ifndef _ACM_TOKEN_HPP_
#define _ACM_TOKEN_HPP_

FOCP_BEGIN();

class CAcmTokenModule;

class ACM_API CAcmToken
{
	friend class CAcmTokenModule;
protected:
	uint32 m_nToken;
	uint32 m_nDomain;
	uint8 m_nWeight;

public:
	CAcmToken(uint32 nDomain, uint32 nToken, uint8 nWeight=64);
	virtual ~CAcmToken();

	bool GetOwner(uint32 &nOwner);
	uint32 GetDomain() const;
	uint32 GetToken() const;

protected:
	virtual void OnRoleChange(bool bTakeUp);
	virtual void OnOwnerChange(uint32 nOldOwner, uint32 nNewOwner);
};

class ACM_API CAcmMutex: public CAcmToken
{
private:
	CMutex m_oMutex;
	CMutex m_oLock;
	CEvent m_oEvent;
	uint32 m_nCounter;
	bool m_bOwned;

public:
	CAcmMutex(uint32 nDomain, uint32 nToken, bool bThread=true);
	virtual ~CAcmMutex();

	bool Enter(uint32 nTimeOut=0xFFFFFFFF);
	void Leave();

protected:
	virtual void OnRoleChange(bool bTakeUp);
};

class ACM_API CAcmTokenWalker
{
	friend class CAcmTokenModule;
public:
	CAcmTokenWalker();
	virtual ~CAcmTokenWalker();

protected:
	virtual void OnWalk(uint32 nToken, uint32 nDomain, uint32 nOwner);
};

class ACM_API CAcmTokenModule: public CAcmUdpModule
{
	struct CAcmTokenBsm
	{
		uint32 nCmd;
		uint32 nSender;
		uint32 nWeight;
	};

	struct CAcmTokenBst
	{
		uint32 nCmd;
		uint32 nTimerId;
	};

	struct CAcmTokenMsg
	{
		uint32 nDomain;
		uint32 nToken;
		CAcmUdp * pUdp;
		union CMsg
		{
			uint32 nCmd;//0=BSM,1=BST
			CAcmTokenBsm oBsm;
			CAcmTokenBst oBst;
		} oMsg;
	};

	struct CToken
	{
		uint32 nStatus;//我对于该Token的状态
		uint32 nWeight;//我的权重
		uint32 nOwner;//目前的所有者
		uint32 nOwnerWeight;//占有者的权重
		uint32 nTimeOut;
		uint32 nTimerId;
		uint32 nNode;
		CAcmToken* pToken;
		CAcmUdp* pUdp;
	};

private:
	CMutex m_oMutex;
	bool m_bStarted;
	uint32 m_nTimeOut[4];
	CRbMap<uint32/*nDomain*/, CRbMap<uint32/*nTokenId*/, CToken> > m_oTokens;

private:
	CAcmTokenModule();

public:
	virtual ~CAcmTokenModule();

	static CAcmTokenModule* GetInstance(bool bQuery=true);

	void ProcessTimer(uint32 nDomain, uint32 nTokenId, uint32 nTimerId);

	bool AddToken(CAcmToken* pToken);
	void DelToken(CAcmToken* pToken);
	void UpdateTimeOut(uint32 nT0);

	bool QueryToken(uint32 nDomain, uint32 nToken, uint32 &nOwner);

	void Walk(CAcmTokenWalker& oWalker);

	void Start();
	void Stop();

protected:
	//如果该模块吞掉该消息，需要把pMsg设置为NULL;
	//oHead中的字段已解析成本地字节序。
	virtual void ProcessAcmModuleMsg(CAcmUdp *pUdp, CUdpMsg* &pMsg);//for CAcmUdpModule

private:
	CToken* GetToken(uint32 nDomain, uint32 nToken);
	void ProcessBsm(CAcmTokenMsg& oTokenMsg);
	bool ProcessBsmA(CToken* pToken, CAcmTokenMsg* pMsg);
	bool ProcessBsmC(CToken* pToken, CAcmTokenMsg* pMsg);
	bool ProcessBsmP(CToken* pToken, CAcmTokenMsg* pMsg);
	bool ProcessBsmE(CToken* pToken, CAcmTokenMsg* pMsg);
	uint32 GetMsgType(CToken* pToken, CAcmTokenMsg* pMsg);
	uint64 ComputeWeight(uint32 nNode, uint32 nWeight);
	uint32 SetTimer(uint32 nToken, CToken* pToken, uint32 nTimerIdx);
	void SendBsm(CAcmUdp* pUdp,uint32 nToken, uint32 nWeight);
	void UpdateTokenTimeOut(CToken* pToken, uint32 nT0);
	void StartToken(uint32 nToken, CToken* pToken);
};

FOCP_END();

#endif
