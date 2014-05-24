
#include "AcmDef.hpp"

#ifndef _ACM_UDP_HPP_
#define _ACM_UDP_HPP_

FOCP_BEGIN();

enum
{
	ACM_MAX_UDP = 4096,
	ACM_UDP_HEADSIZE = 12,
};

struct CUdpHead
{
	uint32 nNode;
	uint32 nPid;
	uint16 nModule;
	uint16 nCmd;
};

struct CUdpMsg;
class CAcmUdpModule;
class CAcmContext;
class CAcmUdp;

struct ACM_API CUdpMsg
{
	//����
	uint32 nSize;
	//oHead
	CUdpHead oHead;
	//oBody
	uint8 sBody[ACM_MAX_UDP-ACM_UDP_HEADSIZE];

	//Ϊ����32λ��64λϵͳ����������uintptr����
	void* operator new(uintptr nSize);
	void operator delete(void* pMsg);

	CUdpMsg();
	CUdpMsg(const CUdpMsg& oSrc);
	CUdpMsg& operator=(const CUdpMsg& oSrc);
};

class ACM_API CAcmUdpModule
{
	friend class CAcmUdp;
	FOCP_FORBID_COPY(CAcmUdpModule);
public:
	CAcmUdpModule();
	virtual ~CAcmUdpModule();

protected:
	//�����ģ���̵�����Ϣ����Ҫ��pMsg����ΪNULL;
	//oHead�е��ֶ��ѽ����ɱ����ֽ���
	virtual void ProcessAcmModuleMsg(CAcmUdp* pUdp, CUdpMsg* &pMsg) = 0;
};

class ACM_API CAcmContext
{
	FOCP_FORBID_COPY(CAcmContext);
	friend class CAcmUdp;
public:
	CAcmContext();
	virtual ~CAcmContext();

protected:
	virtual void OnNodeRepeat(CAcmUdp* pUdp);
	virtual void OnAddNode(CAcmUdp* pUdp, uint32 nNode, uint32 nIp, uint16 nPort);
	virtual void OnWalk(uint32 nNode, const char* sAddr);
};

class CAcmSequenceModule;
class ACM_API CAcmUdp: public CCooperateFunction
{
	friend class CAcmSequenceModule;
private:
	CFile m_oFile;
	CFile m_oUniCastFile;
	CMutex m_oMutex;
	CEvent m_oEvent;
	bool m_bChecked, m_bMultiCast, m_bRegistered;
	uint16 m_nPort, m_nUniCastPort;
	uint32 m_nDomain, m_nNode, m_nPid;
	uint32 m_nItfAddr, m_nMultiCastAddr, m_nTTL;
	CRbMap<uint32, CIpAddr> m_oNodeList;
	CObjectRegistry<CAcmUdpModule, uint32> m_oModules;
	CQueue<CUdpMsg*> m_oMsgQueue;
	CCooperator m_oRecvThread, m_oProcThread, m_oRecvThread2;
	CAcmContext* m_pContext;

public:
	CAcmUdp(CAcmContext* pContext, uint32 nDomain, uint32 nNode, bool bMultiCast=false, bool bReg=false);
	~CAcmUdp();

	bool IsMultiCast();
	uint32 GetMultiCastAddr(CIpAddr &oAddr);
	bool IsGood();

	CAcmContext* GetContext();

	bool InitUniCast(uint16 nPort);
	bool InitMultiCast(uint16 nPort, const char* sMultiCastAddr, uint32 nTTL=1, bool bAllowLoop=false);
	bool InitMultiCast(uint16 nPort, uint32 nMultiCastAddr, uint32 nTTL=1, bool bAllowLoop=false);

	bool Start();
	void Stop(bool bBlock=true);

	static CAcmUdp* QueryUdp(uint32 nDomain);
	static void GetAllDomains(CVector<uint32> &oDomains);

	uint32 GetDomain();
	uint32 GetNode();
	bool GetInterfaceAddr(CIpAddr &oAddr);

	bool AddNode(uint32 nNode, const char* sIp, uint16 nPort=0);//���鲥����Ҫ��д�˿ڣ��Ե���������д�˿ڣ�
	bool AddNode(uint32 nNode, uint32 nIp, uint16 nPort=0);//�Ա���ͬһ�����ܲ�����ͨ�Žڵ㡣
	void DelNode(uint32 nNode);
	bool GetNodeAddr(uint32 nNode, CIpAddr &oAddr);
	void Walk();

	//Ӧ��ע���ģ��
	void RegisterModule(uint32 nModule, CAcmUdpModule* pModule);
	void DeRegisterModule(uint32 nModule);
	CAcmUdpModule* QueryModule(uint32 nModule);
	void ReleaseModule(uint32 nModule);

	//���͵���Ϣ��
	//	nSize��ʾ������Ϣ��С���������ֽ�����д��
	//	nNode��nPid������д
	//	nModule��nCmd�������ֽ�����д
	//������
	//	nNodeָ�����ͽڵ㣬���Ϊ-1����ʾ���������Ⱥ����
	void Send(CUdpMsg &oMsg, uint32 nNode=(-1));

	//�ú�����CAcmUdp����
	//Ӧ��Ҳ����ƴװ�������ݰ�����ϵͳ�����Զ�����
	void ProcessMessage(CUdpMsg* &pMsg);

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private:
	bool RecvMsg(CFile& oFile);
};

FOCP_END();

#endif
