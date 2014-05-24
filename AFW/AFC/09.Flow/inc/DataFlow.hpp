
#include "FlowDef.hpp"

#ifndef _FLW_DataFlow_Hpp_
#define _FLW_DataFlow_Hpp_

FOCP_BEGIN();

/*************************************
 * 数据收发返回码定义
 *************************************/
enum
{
	FOCP_DFLOW_SUCCESS, //接收、发送数据成功
	FOCP_DFLOW_BLOCKED, //因阻塞而导致接收、发送数据失败
	FOCP_DFLOW_SORRY,	  //不支持该功能,只有数据流节点才可能返回该结果。
};

/*************************************
 * 数据流原型定义
 *************************************/
class CDataFlowNode;
class CTrashBinNode;
class CQueueNode;
class CListNode;
class CSwitchNode;
class CRelayNode;
class CProcessNode;
class CDataFlow;
class CPortFunctor;
class CPortNode;

/*************************************
 * CDataFlowNode，基本的数据流节点
 *************************************/
class FLW_API CDataFlowNode
{
	FOCP_FORBID_COPY(CDataFlowNode);
	friend class CDataFlow;
	friend class CDispatchNode;
	friend class CSwitchNode;
	friend class CRelayNode;
	friend class CProcessNode;

private:
	uint32 m_nLinkCounter;

public:
	CDataFlowNode();
	virtual ~CDataFlowNode();

	virtual bool Check();

	virtual bool HaveGetInterface();
	virtual bool HavePutInterface();
	virtual bool HaveActiveInterface();
	virtual bool CanLinkFrom();
	virtual bool CanLinkTo();
	virtual bool CanLinkToMore();
	virtual bool CanLinkToPort();

	virtual uint32 GetData(CAny& oData);
	virtual uint32 PutData(CAny& oData);

	virtual bool LinkFrom(CDataFlowNode* pFrom);
	virtual bool LinkTo(CDataFlowNode* pTo);
	virtual bool LinkTo(CDataFlowNode* pTo, uint32 nDataType);
	virtual bool LinkToPort(void* pPort);
	virtual void UnLink();

	uint32 GetLinkCounter();

	virtual void Start();
	virtual void Stop();

protected:
	void AddLinkCounter();
	void DelLinkCounter();
};

/*************************************
 * 回收站节点
 *************************************/
class FLW_API CTrashBinNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CTrashBinNode);
public:
	CTrashBinNode();
	virtual ~CTrashBinNode();

	virtual bool HavePutInterface();

	virtual uint32 PutData(CAny& oData);
};

/*************************************
 * CQueueNode，队列形式的容器节点
 *************************************/
class FLW_API CQueueNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CQueueNode);
private:
	CMutex m_oMutex;
	CEvent m_oGetEvent, m_oPutEvent;
	uint32 m_nSize, m_nCapacity, m_nHead, m_nTail;
	CAny* m_pQueue;

public:
	CQueueNode(uint32 nCapacity=0, bool bThread=true);
	virtual ~CQueueNode();

	virtual bool HaveGetInterface();
	virtual bool HavePutInterface();

	virtual uint32 GetData(CAny& oData);
	virtual uint32 PutData(CAny& oData);
};

/*************************************
 * CListNode，列表形式的容器节点
 *************************************/
class FLW_API CListNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CListNode);
private:
	CMutex m_oMutex;
	CEvent m_oGetEvent;
	void *m_pHead, *m_pTail;

public:
	CListNode(bool bThread=true);
	virtual ~CListNode();

	virtual bool HaveGetInterface();
	virtual bool HavePutInterface();

	virtual uint32 GetData(CAny& oData);
	virtual uint32 PutData(CAny& oData);
};

/*************************************
 * CSwitchNode，数据分发节点，应用还可以定制分发模型，
 * 	通过FGetDataType来实现，该函使用oData.GetType();
 *************************************/
typedef uint32 (*FGetDataType)(const CAny& oData);
class FLW_API CSwitchNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CSwitchNode);
private:
	FGetDataType m_fGetDataType;
	CRbMap<uint32, CDataFlowNode*> m_oDispTable;

public:
	CSwitchNode(FGetDataType fGetDataType=NULL);
	virtual ~CSwitchNode();

	virtual bool HavePutInterface();
	virtual uint32 PutData(CAny& oData);

	virtual bool CanLinkToMore();
	virtual bool LinkTo(CDataFlowNode* pTo, uint32 nDataType);

	virtual void UnLink();
};

/*************************************
 * CSwitchNode，数据分发节点，应用还可以定制分发模型，
 * 	通过FGetDataType来实现，该函使用oData.GetType();
 *************************************/
typedef uint32 (*FGetDataType)(const CAny& oData);
class FLW_API CDispatchNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CDispatchNode);
private:
	CMutex m_oMutex;
	bool m_bStarted;
	FGetDataType m_fGetDataType;
	CRbMap<uint32, CDataFlowNode*> m_oDispTable;

public:
	CDispatchNode(bool bThread, FGetDataType fGetDataType=NULL);
	virtual ~CDispatchNode();

	virtual bool HavePutInterface();
	virtual uint32 PutData(CAny& oData);

	virtual bool CanLinkToMore();
	virtual bool LinkTo(CDataFlowNode* pTo, uint32 nDataType);

	virtual void UnLink();

	virtual bool HaveActiveInterface();
	virtual void Start();
	virtual void Stop();
};

/*************************************
 * CRelayNode，中继节点，具有计算资源
 *  自动从源读取数据，并发到目的节点
 *************************************/
class FLW_API CRelayNode:
	public CDataFlowNode,
	public CCooperateFunction
{
	FOCP_FORBID_COPY(CRelayNode);
private:
	CCooperator m_oCooperator;
	CDataFlowNode* m_pFrom, *m_pTo;
	CEvent m_hEvent;
	bool m_bStarted;

public:
	CRelayNode(bool bThread);
	virtual ~CRelayNode();

	virtual bool Check();

	virtual bool CanLinkFrom();
	virtual bool LinkFrom(CDataFlowNode* pFrom);

	virtual bool CanLinkTo();
	virtual bool LinkTo(CDataFlowNode* pTo);

	virtual void UnLink();

	virtual bool HaveActiveInterface();
	virtual void Start();
	virtual void Stop();

	void Resume();

protected:
	virtual void MainProc(CCooperator* pCooperator, bool &bRunning);
};

/*************************************
 * CProcessNode，数据处理节点，应用从这里继承，重载ProcessData函数。
 * 	如果返回真，表示有数据要下发，至于如何下发由m_pTo决定
 *************************************/
class FLW_API CProcessNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CProcessNode);
private:
	CDataFlowNode* m_pTo;

public:
	CProcessNode();
	virtual ~CProcessNode();

	virtual bool Check();

	virtual bool CanLinkTo();
	virtual bool LinkTo(CDataFlowNode* pTo);
	virtual void UnLink();

	virtual bool HavePutInterface();
	virtual uint32 PutData(CAny& oData);

protected:
	virtual bool ProcessData(CAny& oData);
	void ForwardData(CAny& oData);
};

/*************************************
 * CDataFlow 数据流
 *************************************/
class FLW_API CDataFlow: public CDataFlowNode
{
	FOCP_FORBID_COPY(CDataFlow);
private:
	CVector<CDataFlowNode*> m_oNodes;
	CDataFlowNode* m_pPutNode, *m_pGetNode;
	bool m_bHaveActiveInterface, m_bStarted;

public:
	CDataFlow();
	virtual ~CDataFlow();

	bool LinkFrom(CDataFlowNode &oNode, CDataFlowNode &oFrom);
	bool LinkTo(CDataFlowNode &oNode, CDataFlowNode &oTo);
	bool LinkTo(CDataFlowNode &oNode, CDataFlowNode &oTo, uint32 nDataType);
	bool LinkToPort(CDataFlowNode &oNode, void* pPort);

	virtual void UnLink();

	bool MakePutNode(CDataFlowNode &oNode);
	bool MakeGetNode(CDataFlowNode &oNode);

	virtual bool Check();

	virtual bool HaveGetInterface();
	virtual bool HavePutInterface();
	virtual bool HaveActiveInterface();

	virtual uint32 PutData(CAny& oData);
	virtual uint32 GetData(CAny& oData);

	virtual void Start();
	virtual void Stop();

private:
	uint32 FindNode(CDataFlowNode* pNode);
};

/*************************************
 * CPortNode，IO节点，应用需要派生特定的
 *	节点以实现数据的读写，文件出错的将发生异常。数据流
 *	捕获并处理该异常。并需要重启
 *************************************/
enum
{
	FOCP_DPORT_SUCCESS,
	FOCP_DPORT_BLOCKED,
	FOCP_DPORT_BROKEN
};

class FLW_API CPortFunctor
{
	FOCP_FORBID_COPY(CPortFunctor);
public:
	CPortFunctor();
	virtual ~CPortFunctor();

	virtual uint32 Read(void* pPort, CAny& oData);
	virtual uint32 Write(void* pPort, CAny& oData);
	virtual void Mend(CRelayNode* pRelayNode, CPortNode* pPortNode);
};

class FLW_API CPortNode: public CDataFlowNode
{
	FOCP_FORBID_COPY(CPortNode);
private:
	void * m_pPort;
	CPortFunctor* m_pFunctor;
	bool m_bExceptional;

public:
	CPortNode(CPortFunctor* pFunctor);
	virtual ~CPortNode();

	virtual bool CanLinkToPort();
	virtual bool LinkToPort(void* pPort);
	virtual bool Check();

	virtual bool HaveGetInterface();
	virtual bool HavePutInterface();

	virtual uint32 GetData(CAny& oData);
	virtual uint32 PutData(CAny& oData);

	bool IsExceptional();

	void* GetPort();

	void Mend(CRelayNode* pRelayNode);
};

FOCP_END();

#endif
