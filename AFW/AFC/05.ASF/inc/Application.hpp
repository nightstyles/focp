
#include "Signal.hpp"
#include "Service.hpp"

#ifndef _Asf_Application_Hpp_
#define _Asf_Application_Hpp_

FOCP_BEGIN();

ASF_API bool StartFocpService(bool bCheckLicense);
ASF_API void StopFocpService();

/*
需要配置环境变量：
FocpTmpDir:指示临时文件目录
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
	void WaitRun();//等待运行结束

protected:
	//应用可以重载
	virtual int32 OnRun();
	virtual void OnRunEnd();
	virtual void OnDefaultSignal(int32 nSignal);//默认处理几个退出信号
	virtual bool ProcessOption(char cOpt, const char* sArg);//处理选项，sArg可能为NULL

	//Run流程控制接口
	void DisableSignaler();//不启动信号处理模块
	void MakeAsynRun();//异步运行Run函数，当启动成功后，不等待运行结束，而直接返回。

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
	virtual bool StartServiceBefore();//启动服务前
	virtual bool StartServiceAfter();//启动服务后
	virtual void StopServiceBefore();//停止服务前
	virtual void StopServiceAfter();//停止服务后
};

class CApplication;

FOCP_END();

#endif //_Afc_Application_Hpp_
