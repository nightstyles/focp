
#include "Signal.hpp"
#include "Service.hpp"

#ifndef _Asf_Application_Hpp_
#define _Asf_Application_Hpp_

FOCP_BEGIN();

ASF_API bool StartFocpService(bool bCheckLicense);
ASF_API void StopFocpService();

/*
��Ҫ���û���������
FocpTmpDir:ָʾ��ʱ�ļ�Ŀ¼
*/
class ASF_API CApplication: public CSignaler
{
private:
	bool m_bStoped, m_bWarm, m_bDisableSignal, m_bBlock;
	CFile m_oLockFile;

public:
	CApplication();
	virtual ~CApplication();

	static CApplication* GetInstance();

	const char* GetAppName();

	virtual int32 Run(int32 argc, char*argv[], bool bCheckLicense=false);
	void StopNotice(bool bWarm=true);
	void WaitRun();//�ȴ����н���

protected:
	//Ӧ�ÿ�������
	virtual int32 OnRun();
	virtual void OnRunEnd();
	virtual void OnDefaultSignal(int32 nSignal);//Ĭ�ϴ������˳��ź�
	virtual bool ProcessOption(char cOpt, const char* sArg);//����ѡ�sArg����ΪNULL

	//Run���̿��ƽӿ�
	void DisableSignaler();//�������źŴ���ģ��
	void MakeAsynRun();//�첽����Run�������������ɹ��󣬲��ȴ����н�������ֱ�ӷ��ء�

private:
	void RunEnd();
};

class ASF_API CServiceApplication: public CApplication, public CServiceChecker
{
public:
	CServiceApplication();
	virtual ~CServiceApplication();

protected:
	virtual int32 OnRun();
	virtual void OnRunEnd();
	virtual bool StartServiceBefore();//��������ǰ
	virtual bool StartServiceAfter();//���������
	virtual void StopServiceBefore();//ֹͣ����ǰ
	virtual void StopServiceAfter();//ֹͣ�����
};

class CApplication;

FOCP_END();

#endif //_Afc_Application_Hpp_
