
#include "Signal.hpp"

#include <signal.h>

FOCP_BEGIN();

static CSignaler* g_pSignaler = NULL;

CSignaler::CSignaler()
{
	uint32 i;
	if(g_pSignaler)
		FocpAbort(("Redefine CSignaler"));
	g_pSignaler = this;
	for(i=0; i<FOCP_MAXSIG; ++i)
		m_pSignals[i] = FOCP_SIGNAL_NOSET;
}

CSignaler::~CSignaler()
{
}

void CSignaler::SetAllSignals(uint32 nSignalMode)
{
	uint32 i;
	if(nSignalMode >= 0 && nSignalMode < FOCP_MAXSIG && nSignalMode<=FOCP_SIGNAL_FORBID)
	{
		for(i=0; i<FOCP_MAXSIG; ++i)
			m_pSignals[i] = nSignalMode;
	}
}

void CSignaler::SetSignal(int32 nSignal, uint32 nSignalMode, bool bCommit)
{
	if(nSignal >= 0 && nSignal < FOCP_MAXSIG && nSignalMode<=FOCP_SIGNAL_FORBID)
	{
		m_pSignals[nSignal] = nSignalMode;
		if(bCommit)
			Commit();
	}
}

void CSignaler::FocpSignal(int32 nSignal)
{
	bool bDone = false;
	int32 nFocpSignal = g_pSignaler->GetFocpSignal(nSignal);
	g_pSignaler->OnSignal(nFocpSignal, bDone);
	if(bDone == false)
		g_pSignaler->OnDefaultSignal(nFocpSignal);
}

void CSignaler::SetSignals()
{
	uint32 i;
	for(i=0; i<FOCP_MAXSIG; ++i)
		m_pSignals[i] = FOCP_SIGNAL_DEFAULT;//FOCP_SIGNAL_DEFAULT;

	m_pSignals[FOCP_SIGCHLD] = FOCP_SIGNAL_FORBID;
	m_pSignals[FOCP_SIGINFO] = FOCP_SIGNAL_FORBID;
	m_pSignals[FOCP_SIGURG] = FOCP_SIGNAL_FORBID;
	m_pSignals[FOCP_SIGWINCH] = FOCP_SIGNAL_FORBID;

	m_pSignals[FOCP_SIGINT] = FOCP_SIGNAL_ACCEPT;
	m_pSignals[FOCP_SIGQUIT] = FOCP_SIGNAL_ACCEPT;
	m_pSignals[FOCP_SIGTERM] = FOCP_SIGNAL_ACCEPT;

	m_pSignals[FOCP_SIGALRM] = FOCP_SIGNAL_FORBID;//终端连接断开
	m_pSignals[FOCP_SIGHUP] = FOCP_SIGNAL_FORBID;//终端连接断开
	m_pSignals[FOCP_SIGIO] = FOCP_SIGNAL_FORBID;//异步IO
	m_pSignals[FOCP_SIGPIPE] = FOCP_SIGNAL_FORBID;//管道读进程终止，我发现闪断也会有该信号
	m_pSignals[FOCP_SIGTSTP] = FOCP_SIGNAL_FORBID;//Ctrl+Z挂起进程
	m_pSignals[FOCP_SIGTTIN] = FOCP_SIGNAL_FORBID;//后台进程读终端
	m_pSignals[FOCP_SIGTTOU] = FOCP_SIGNAL_FORBID;//后台进程写终端
	m_pSignals[FOCP_SIGVTALRM] = FOCP_SIGNAL_FORBID;//settimer
	m_pSignals[FOCP_SIGUSR1] = FOCP_SIGNAL_FORBID;//用户信号
	m_pSignals[FOCP_SIGUSR2] = FOCP_SIGNAL_FORBID;//用户信号
	m_pSignals[FOCP_SIGPROF] = FOCP_SIGNAL_FORBID;//settimer
	m_pSignals[FOCP_SIGPOLL] = FOCP_SIGNAL_FORBID;//poll
}

void CSignaler::FocpSignalIgnore(int32 nSignal)
{
}

void CSignaler::OnSignal(int32 nSignal, bool &bDone)
{
}

void CSignaler::OnDefaultSignal(int32 nSignal)
{
}

void CSignaler::Commit()
{
	CommitSignal(FOCP_SIGABRT);
	CommitSignal(FOCP_SIGFPE);
	CommitSignal(FOCP_SIGILL);
	CommitSignal(FOCP_SIGINT);
	CommitSignal(FOCP_SIGSEGV);
	CommitSignal(FOCP_SIGTERM);
#ifndef WINDOWS
	CommitSignal(FOCP_SIGALRM);
	CommitSignal(FOCP_SIGBUS);
	CommitSignal(FOCP_SIGCHLD);
	CommitSignal(FOCP_SIGCONT);
	CommitSignal(FOCP_SIGEMT);
	CommitSignal(FOCP_SIGHUP);
	CommitSignal(FOCP_SIGINFO);
	CommitSignal(FOCP_SIGIO);
	CommitSignal(FOCP_SIGIOT);
//CommitSignal(FOCP_SIGKILL);
	CommitSignal(FOCP_SIGPIPE);
	CommitSignal(FOCP_SIGPOLL);
	CommitSignal(FOCP_SIGPROF);
	CommitSignal(FOCP_SIGPWR);
	CommitSignal(FOCP_SIGQUIT);
//CommitSignal(FOCP_SIGSTOP);
	CommitSignal(FOCP_SIGSYS);
	CommitSignal(FOCP_SIGTRAP);
	CommitSignal(FOCP_SIGTSTP);
	CommitSignal(FOCP_SIGTTIN);
	CommitSignal(FOCP_SIGTTOU);
	CommitSignal(FOCP_SIGURG);
	CommitSignal(FOCP_SIGUSR1);
	CommitSignal(FOCP_SIGUSR2);
	CommitSignal(FOCP_SIGVTALRM);
	CommitSignal(FOCP_SIGWINCH);
	CommitSignal(FOCP_SIGXCPU);
	CommitSignal(FOCP_SIGXFSZ);
#endif
}

#ifndef WINDOWS
static void SetSignalFilter(int32 nSignal, bool bForbid)
{
	sigset_t oSet;
	sigprocmask(SIG_SETMASK, NULL, &oSet);
	if(bForbid)
		sigaddset(&oSet, nSignal);
	else
		sigdelset(&oSet, nSignal);
	sigprocmask(SIG_SETMASK, &oSet, NULL);
}
#endif

void CSignaler::CommitSignal(int32 nSignal)
{
	int32 nOsSignal = GetOsSignal(nSignal);
	switch(m_pSignals[nSignal])
	{
	case FOCP_SIGNAL_ACCEPT:
#ifndef WINDOWS
		SetSignalFilter(nOsSignal, false);
#endif
		signal(nOsSignal, CSignaler::FocpSignal);
		break;
	case FOCP_SIGNAL_IGNORE:
#ifndef WINDOWS
		SetSignalFilter(nOsSignal, false);
#endif
		signal(nOsSignal, CSignaler::FocpSignalIgnore);
		break;
	case FOCP_SIGNAL_DEFAULT:
#ifndef WINDOWS
		SetSignalFilter(nOsSignal, false);
#endif
		signal(nOsSignal, SIG_DFL);
		break;
	case FOCP_SIGNAL_FORBID:
#ifdef WINDOWS
		signal(nOsSignal, SIG_IGN);
#else
		SetSignalFilter(nOsSignal, true);
#endif
		break;
	}
}

int32 CSignaler::GetOsSignal(int32 nSignal)
{
	int32 nRet = 0;
	switch(nSignal)
	{
	case FOCP_SIGABRT:
		nRet = SIGABRT;
		break;
	case FOCP_SIGFPE:
		nRet = SIGFPE;
		break;
	case FOCP_SIGILL:
		nRet = SIGILL;
		break;
	case FOCP_SIGINT:
		nRet = SIGINT;
		break;
	case FOCP_SIGSEGV:
		nRet = SIGSEGV;
		break;
	case FOCP_SIGTERM:
		nRet = SIGTERM;
		break;
#ifndef WINDOWS
	case FOCP_SIGALRM:
		nRet = SIGALRM;
		break;
	case FOCP_SIGBUS:
		nRet = SIGBUS;
		break;
	case FOCP_SIGCHLD:
		nRet = SIGCHLD;
		break;
	case FOCP_SIGCONT:
		nRet = SIGCONT;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case FOCP_SIGEMT:
		nRet = SIGEMT;
		break;
#endif
	case FOCP_SIGHUP:
		nRet = SIGHUP;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case FOCP_SIGINFO:
		nRet = SIGINFO;
		break;
#endif
	case FOCP_SIGIO:
		nRet = SIGIO;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case FOCP_SIGIOT:
		nRet = SIGIOT;
		break;
#endif
	case FOCP_SIGKILL:
		nRet = SIGKILL;
		break;
	case FOCP_SIGPIPE:
		nRet = SIGPIPE;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case FOCP_SIGPOLL:
		nRet = SIGPOLL;
		break;
#endif
	case FOCP_SIGPROF:
		nRet = SIGPROF;
		break;
	case FOCP_SIGPWR:
		nRet = SIGPWR;
		break;
	case FOCP_SIGQUIT:
		nRet = SIGQUIT;
		break;
	case FOCP_SIGSTOP:
		nRet = SIGSTOP;
		break;
	case FOCP_SIGSYS:
		nRet = SIGSYS;
		break;
	case FOCP_SIGTRAP:
		nRet = SIGTRAP;
		break;
	case FOCP_SIGTSTP:
		nRet = SIGTSTP;
		break;
	case FOCP_SIGTTIN:
		nRet = SIGTTIN;
		break;
	case FOCP_SIGTTOU:
		nRet = SIGTTOU;
		break;
	case FOCP_SIGURG:
		nRet = SIGURG;
		break;
	case FOCP_SIGUSR1:
		nRet = SIGUSR1;
		break;
	case FOCP_SIGUSR2:
		nRet = SIGUSR2;
		break;
	case FOCP_SIGVTALRM:
		nRet = SIGVTALRM;
		break;
	case FOCP_SIGWINCH:
		nRet = SIGWINCH;
		break;
	case FOCP_SIGXCPU:
		nRet = SIGXCPU;
		break;
	case FOCP_SIGXFSZ:
		nRet = SIGXFSZ;
		break;
#endif
	}
	return nRet;
}

int32 CSignaler::GetFocpSignal(int32 nSignal)
{
	int32 nRet = 0;
	switch(nSignal)
	{
	case SIGABRT:
		nRet = FOCP_SIGABRT;
		break;
	case SIGFPE:
		nRet = FOCP_SIGFPE;
		break;
	case SIGILL:
		nRet = FOCP_SIGILL;
		break;
	case SIGINT:
		nRet = FOCP_SIGINT;
		break;
	case SIGSEGV:
		nRet = FOCP_SIGSEGV;
		break;
	case SIGTERM:
		nRet = FOCP_SIGTERM;
		break;
#ifndef WINDOWS
	case SIGALRM:
		nRet = FOCP_SIGALRM;
		break;
	case SIGBUS:
		nRet = FOCP_SIGBUS;
		break;
	case SIGCHLD:
		nRet = FOCP_SIGCHLD;
		break;
	case SIGCONT:
		nRet = FOCP_SIGCONT;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case SIGEMT:
		nRet = FOCP_SIGEMT;
		break;
#endif
	case SIGHUP:
		nRet = FOCP_SIGHUP;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case SIGINFO:
		nRet = FOCP_SIGINFO;
		break;
#endif
	case SIGIO:
		nRet = FOCP_SIGIO;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case SIGIOT:
		nRet = FOCP_SIGIOT;
		break;
#endif
	case SIGKILL:
		nRet = FOCP_SIGKILL;
		break;
	case SIGPIPE:
		nRet = FOCP_SIGPIPE;
		break;
#if !defined(LINUX) && !defined(CYGWIN_NT)
	case SIGPOLL:
		nRet = FOCP_SIGPOLL;
		break;
#endif
	case SIGPROF:
		nRet = FOCP_SIGPROF;
		break;
	case SIGPWR:
		nRet = FOCP_SIGPWR;
		break;
	case SIGQUIT:
		nRet = FOCP_SIGQUIT;
		break;
	case SIGSTOP:
		nRet = FOCP_SIGSTOP;
		break;
	case SIGSYS:
		nRet = FOCP_SIGSYS;
		break;
	case SIGTRAP:
		nRet = FOCP_SIGTRAP;
		break;
	case SIGTSTP:
		nRet = FOCP_SIGTSTP;
		break;
	case SIGTTIN:
		nRet = FOCP_SIGTTIN;
		break;
	case SIGTTOU:
		nRet = FOCP_SIGTTOU;
		break;
	case SIGURG:
		nRet = FOCP_SIGURG;
		break;
	case SIGUSR1:
		nRet = FOCP_SIGUSR1;
		break;
	case SIGUSR2:
		nRet = FOCP_SIGUSR2;
		break;
	case SIGVTALRM:
		nRet = FOCP_SIGVTALRM;
		break;
	case SIGWINCH:
		nRet = FOCP_SIGWINCH;
		break;
	case SIGXCPU:
		nRet = FOCP_SIGXCPU;
		break;
	case SIGXFSZ:
		nRet = FOCP_SIGXFSZ;
		break;
#endif
	}
	return nRet;
}

FOCP_END();
