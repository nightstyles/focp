
#include "AsfDef.hpp"

#ifndef _Asf_Service_Hpp_
#define _Asf_Service_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_SERVICE_IDLE=0,
	FOCP_SERVICE_INITIALIZING=1,
	FOCP_SERVICE_INITIALIZED=100,
	FOCP_SERVICE_STARTING=101,
	FOCP_SERVICE_STARTED=200,
	FOCP_SERVICE_STOPING=201,
	FOCP_SERVICE_STOPED=300,
	FOCP_SERVICE_CLEANING=301,
	FOCP_SERVICE_CLEANED=400,
};

class CService;
class ASF_API CServiceChecker
{
	FOCP_FORBID_COPY(CServiceChecker);
	friend class CService;
public:
	CServiceChecker();
	virtual ~CServiceChecker();

protected:
	virtual void OnSetServiceStateBefore(CService* pService, uint32 nState, bool &bAllowed);
	virtual void OnSetServiceStateAfter(CService* pService, uint32 nState);
};

struct CBreak;
class CServiceManager;

class ASF_API CService: public CInterface
{
	FOCP_FORBID_COPY(CService);
	friend class CServiceManager;

private:
	bool m_bFailed;//for wait
	uint32 m_nState;
	uint32 m_nWaitState;//for wait

	CBreak* m_pServiceBreak, *m_pCurBreak, *m_pTopBreak;
	ulong *m_pStackTop, *m_pStackBottom, *m_pStackBackup;
	CService* m_pNext, *m_pWait;
	CBaseSingleList<CService> m_oWaitList;
	void StateNotice(bool bForce=false);
	bool LoopWaitCheck(CService* pService);
	static void Switch(CService* pService);

	bool SetDefaultState(uint32 nState);

protected:
	CService(bool bFake);//for CServiceManager
	virtual const char* GetInterfaceName();

public:
	CService();
	virtual ~CService();

	virtual const char* GetServiceName();

	virtual bool HaveAbility(uint32 nOption);

	uint32 GetState();
	const char* GetStateName(uint32 nState);

	//����ֵ��ʾ�ɹ����
	//�ú���ֻ������OnInitialize/OnStart/OnStop�����С�
	bool Wait(CService* pService, uint32 nState);

	virtual void * GetFiberTimer();

protected:
	//һ�¼�����������˽�еģ�Ŀ���ǲ��ṩ����ҵ�����ͣ���ơ�
	bool Initialize();
	bool Start();
	void Stop();
	void Cleanup();

	bool SetState(uint32 nState);

//Ӧ����Ҫ�����⼸������
	virtual bool OnInitialize();
	virtual bool OnStart();

	//�����ɹ���񣬾�Ӧ���ܵ��øú���
	//��ط�������״̬����Ҫʹ��Wait�ȴ���
	virtual void OnStop();

	//��ʼ�ɹ���񣬾����Ե��øú���
	virtual void OnCleanup();

	//Ӧ��ϸ��״̬�Ļ�����Ҫ���ظú�������������ȫ�����˻��ຯ��
	virtual const char* OnGetStateName(uint32 nState);
};

//��Ҫ���û���������FocpNmsCfgHome:ָʾ���������ļ�Ŀ¼
class ASF_API CServiceManager: public CService
{
	friend class CService;
	friend struct CSingleInstance<CServiceManager>;
private:
	CMutex m_oMutex;
	bool m_bResult, m_bUnLoad;
	CInterfaceManager m_oServiceTable;
	uint32 m_nServiceCount;
	uint32 m_nDMN, m_nCNN, m_nAIN, m_nATN;

	CBaseSingleList<CService> m_oReadyQueue;
	CBreak* m_pTopBreak;
	CStackAttr m_oStackAttr;

	static void Run(CServiceManager* pServiceManager, uint32 nStep);
	CServiceManager();

public:
	virtual ~CServiceManager();

	void DontUnLoad();//��ж�س�ʼ����

	uint32 GetDMN();
	uint32 GetCNN();
	uint32 GetAIN();
	uint32 GetATN();

	void SetATN(uint32 nATN);
	void SetInstance(uint32 nDMN, uint32 nCNN, uint32 nAIN);

	uint32 GetServiceCount();
	CService* GetService(uint32 nServiceIdx);
	CService* QueryService(const char* sServiceName);

	using CService::Initialize;
	using CService::Start;
	using CService::Stop;
	using CService::Cleanup;

	virtual const char* GetServiceName();

	static CServiceManager* GetInstance();

protected:
	virtual bool OnInitialize();
	virtual bool OnStart();
	virtual void OnStop();
	virtual void OnCleanup();
};

FOCP_END();

#endif //_Afc_Service_Hpp_
