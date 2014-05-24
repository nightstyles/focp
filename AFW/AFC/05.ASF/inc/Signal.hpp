
#include "AsfDef.hpp"

#ifndef _Asf_Signal_Hpp_
#define _Asf_Signal_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_SIGNAL_NOSET=0,
	FOCP_SIGNAL_ACCEPT=1,//通知进程,应用可以定制处理动作
	FOCP_SIGNAL_IGNORE=2,//通知进程,但是不做任何事情
	FOCP_SIGNAL_DEFAULT=3,//通知进程,默认为操作系统行为
	FOCP_SIGNAL_FORBID=4,//不通知进程

	FOCP_SIGABRT=0,
	FOCP_SIGALRM,
	FOCP_SIGBUS,
	FOCP_SIGCHLD,
	FOCP_SIGCONT,
	FOCP_SIGEMT,
	FOCP_SIGFPE,
	FOCP_SIGHUP,
	FOCP_SIGILL,
	FOCP_SIGINFO,
	FOCP_SIGINT,
	FOCP_SIGIO,
	FOCP_SIGIOT,
	FOCP_SIGKILL,
	FOCP_SIGPIPE,
	FOCP_SIGPOLL,
	FOCP_SIGPROF,
	FOCP_SIGPWR,
	FOCP_SIGQUIT,
	FOCP_SIGSEGV,
	FOCP_SIGSTOP,
	FOCP_SIGSYS,
	FOCP_SIGTERM,
	FOCP_SIGTRAP,
	FOCP_SIGTSTP,
	FOCP_SIGTTIN,
	FOCP_SIGTTOU,
	FOCP_SIGURG,
	FOCP_SIGUSR1,
	FOCP_SIGUSR2,
	FOCP_SIGVTALRM,
	FOCP_SIGWINCH,
	FOCP_SIGXCPU,
	FOCP_SIGXFSZ,
	FOCP_MAXSIG
};

class ASF_API CSignaler
{
	FOCP_FORBID_COPY(CSignaler);
private:
	int32 m_pSignals[FOCP_MAXSIG];//FOCP_SIGNAL_ACCEPT,FOCP_SIGNAL_IGNORE,FOCP_SIGNAL_FORBID

public:
	CSignaler();
	virtual ~CSignaler();

	void SetSignal(int32 nSignal, uint32 nSignalMode, bool bCommit=false);//nSignalMode:FOCP_SIGNAL_ACCEPT,FOCP_SIGNAL_IGNORE,FOCP_SIGNAL_FORBID
	void Commit();

	virtual void SetSignals();//设置信号处理，缺省设置了信号
	void SetAllSignals(uint32 nSignalMode);

protected:
	virtual void OnSignal(int32 nSignal, bool &bDone);//缺省什么都不做，如果拦截了信号，那么需要设置bDone为true;
	virtual void OnDefaultSignal(int32 nSignal);

private:
	static void FocpSignal(int32 nSignal);
	static void FocpSignalIgnore(int32 nSignal);
	void CommitSignal(int32 nSignal);
	int32 GetOsSignal(int32 nSignal);
	int32 GetFocpSignal(int32 nSignal);
};

FOCP_END();

#endif
