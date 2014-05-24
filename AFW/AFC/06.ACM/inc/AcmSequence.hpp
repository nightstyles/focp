
#include "AcmUdp.hpp"

#ifndef _ACM_SEQUENCE_HPP_
#define _ACM_SEQUENCE_HPP_

FOCP_BEGIN();

enum
{
	ASM_UDP_HEADSIZE = ACM_UDP_HEADSIZE + 8,
	ASM_SUPPORT_SEND = 1,
	ASM_SUPPORT_RECV = 2,
	ASM_SUPPORT_ACK = 4,
};

struct CAsmHead: public CUdpHead
{
	uint32 nSequence;
	uint16 nPlugIn;
	uint16 nOp;
};

struct CAsmDataMsg
{
	//辅助
	uint32 nSize;
	//oHead
	CAsmHead oHead;
	//oBody
	uint8 sBody[ACM_MAX_UDP-ASM_UDP_HEADSIZE];
};

class ACM_API CAsmMsg
{
private:
	CUdpMsg* m_pUdpMsg;

public:
	~CAsmMsg();
	CAsmMsg(bool bNew=false);
	CAsmMsg(CUdpMsg* pUdpMsg);
	CAsmMsg(const CAsmMsg& oSrc);
	CAsmMsg& operator=(const CAsmMsg& oSrc);

	CUdpMsg* GetUdpMsg(bool bDetah=false);

	uint32 GetNode();
	uint32 GetSequence();
	void SetSequence(uint32 nSeq);

	CAsmMsg& Swap(CAsmMsg& oSrc);
};

class CAcmSequenceModule;
class ACM_API CAsmPlugIn
{
	FOCP_FORBID_COPY(CAsmPlugIn);
	friend class CAcmSequenceModule;
public:
	CAsmPlugIn();
	virtual ~CAsmPlugIn();

protected:
	virtual void ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg) = 0;
	virtual void Clear(uint32 nNode);
};

struct CAsmNode
{
	uint32 nTime;
	uint16 nPort;//重传端口
	uint16 nTcpPort;
	uint8 bOnLine;
	uint8 bServering;
	uint8 nCounter;//控制bOnLine;
};

class ACM_API CAsmNodeWalker
{
	friend class CAcmSequenceModule;
public:
	CAsmNodeWalker();
	virtual ~CAsmNodeWalker();

protected:
	virtual void OnWalk(uint32 nDomain, uint32 nNode, const CAsmNode& oNode);
};

typedef CSequenceSendQueue<CAsmMsg, CEvent, CMutex> CAsmSendQueue;
typedef CSequenceRecvQueue<CAsmMsg, CAcmSequenceModule, CMutex> CAsmRecvQueue;
typedef CSingleList<CUdpMsg*> CAsmHoldQueue;

class CAsmSystemPlugIn;
class ACM_API CAcmSequenceModule: public CAcmUdpModule, public CCooperateFunction
{
	friend class CAsmSystemPlugIn;
private:
	CMutex m_oMutex, m_oSendMutex;
	CAcmUdp* m_pUdp;
	CObjectRegistry<CAsmPlugIn, uint32> m_oPlugIns;
	CRbMap<uint32, CAsmNode> m_oNodes;
	CAsmSendQueue* m_pSendQueue;
	CAsmRecvQueue* m_pRecvQueue;
	CAsmHoldQueue* m_pHoldQueue;
	CCooperator m_oRegThread, m_oCheckThread, m_oResendThread, m_oHoldThread;
	uint32 m_nRegTimer, m_nCheckTimer, m_nResendTimer, m_nStartTime;
	uint8 m_nBreakHeartBeat, m_nExpireHeartBeat;
	CEvent m_oEvent;
	uint8 m_nStatus;
	uint8 m_bIsMultiCast;

public:
	virtual ~CAcmSequenceModule();
	CAcmSequenceModule(CAcmUdp* pUdp, uint32 nMode=(ASM_SUPPORT_SEND|ASM_SUPPORT_RECV|ASM_SUPPORT_ACK),
					   uint32 nCapacity=50000, uint8 nMaxAckTimes=20);

	bool SupportSend();
	bool SupportRecv();

	bool SupportMultiCastSend();
	bool SupportUniCastSend();

	uint32 GetRecvCapacity(uint32 &nSize);
	uint32 GetSendCapacity(uint32 &nSize);

	CAcmUdp* GetUdp();

	uint32 QueryServiceNode(CAsmNode& oNode);
	void Walk(CAsmNodeWalker &oWalker);

	//应用注册的插件，插件号从1000开始
	void RegisterPlugIn(uint32 nPlugIn, CAsmPlugIn* pPlugIn);
	void DeRegisterPlugIn(uint32 nPlugIn);
	CAsmPlugIn* QueryPlugIn(uint32 nPlugIn);
	void ReleasePlugIn(uint32 nPlugIn);

	//发包
	bool Send(CAsmDataMsg &oMsg, uint32 nNode=(uint32)(-1));

	// for CAsmRecvQueue
	void OnRequestMsg(uint32 nNode, uint32 nCount, uint32 *pSeq);
	void OnMoveRecvWindow(uint32 nNode, uint32 nSeq);
	void OnMessage(CAsmMsg& oMsg);
	void OnLostMsg(uint32 nNode, uint32 nSeq);
	void OnClearRecvWindow(uint32 nNode);
	// for m_oCheckThread
	void OnNodeChange(uint32 nNode, bool bOnLine);

	bool Start(uint32 nRegTimer=1000, uint32 nCheckTimer=2000, uint8 nBreakHeartBeat=20,
			   uint32 nExpireHeartBeat=200, uint32 nResendTimer=1000);
	void Stop(bool bBlock=true);

	void ActiveService();
	bool IsActive();

	//可以拼装仿真数据包，让系统进行自动处理，特别是从CAsmStream读出来的消息
	void ProcessMessage(CAsmDataMsg& oMsg);

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);
	//如果该模块吞掉该消息，需要把pMsg设置为NULL;
	//oHead中的字段已解析成本地字节序。
	virtual void ProcessAcmModuleMsg(CAcmUdp* pUdp, CUdpMsg* &pMsg);//for CAcmUdpModule

private:
	void ReSend(uint32 nNode, uint32 nSeq);
	void ActiveNode(uint32 nNode);
	void ProcessMessage(CAsmMsg& oMsg);
	void ProcessDeRegister(uint32 nNode);
	void ProcessRegister(uint32 nNode, uint8* sBody);
	bool Send2(CAsmDataMsg &oMsg, uint32 nNode);
	bool Send3(CAsmDataMsg &oMsg, uint32 nNode);
	bool Send4(CAsmDataMsg &oMsg, uint32 nNode);
	bool Send5(uint8* pStream, uint32 nStreamSize, uint32 nNode, uint16 *pPackNo);
	static void ClearNode(CAsmPlugIn* pPlugIn, void * pPara);
};

FOCP_END();

#endif
