
#include "FlowDef.hpp"

#ifndef _FLW_DataFlow_Hpp_
#define _FLW_DataFlow_Hpp_

FOCP_BEGIN();

/*************************************
 * �����շ������붨��
 *************************************/
enum
{
	FOCP_DFLOW_SUCCESS, //���ա��������ݳɹ�
	FOCP_DFLOW_BLOCKED, //�����������½��ա���������ʧ��
	FOCP_DFLOW_SORRY,	  //��֧�ָù���,ֻ���������ڵ�ſ��ܷ��ظý����
};

/*************************************
 * ������ԭ�Ͷ���
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
 * CDataFlowNode���������������ڵ�
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
 * ����վ�ڵ�
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
 * CQueueNode��������ʽ�������ڵ�
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
 * CListNode���б���ʽ�������ڵ�
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
 * CSwitchNode�����ݷַ��ڵ㣬Ӧ�û����Զ��Ʒַ�ģ�ͣ�
 * 	ͨ��FGetDataType��ʵ�֣��ú�ʹ��oData.GetType();
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
 * CSwitchNode�����ݷַ��ڵ㣬Ӧ�û����Զ��Ʒַ�ģ�ͣ�
 * 	ͨ��FGetDataType��ʵ�֣��ú�ʹ��oData.GetType();
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
 * CRelayNode���м̽ڵ㣬���м�����Դ
 *  �Զ���Դ��ȡ���ݣ�������Ŀ�Ľڵ�
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
 * CProcessNode�����ݴ���ڵ㣬Ӧ�ô�����̳У�����ProcessData������
 * 	��������棬��ʾ������Ҫ�·�����������·���m_pTo����
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
 * CDataFlow ������
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
 * CPortNode��IO�ڵ㣬Ӧ����Ҫ�����ض���
 *	�ڵ���ʵ�����ݵĶ�д���ļ�����Ľ������쳣��������
 *	���񲢴�����쳣������Ҫ����
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
