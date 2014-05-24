
#include "LogListenor.hpp"

#ifndef _AFW_LOG_MONITOR_HPP_
#define _AFW_LOG_MONITOR_HPP_

FOCP_BEGIN();

class CLogMonitor: public CLogProcessor
{
private:
	CCmdSession * m_pCmdSession;
	//主机、日期、应用、模块、文件、函数、内容
	uint32 m_nFilter;

public:
	CLogMonitor(CCmdSession* pCmdSession);
	virtual ~CLogMonitor();

	virtual void Process(CLogMsg& oLog);

	static void RegisterCmd();

private:
	void GetLogInfo(CLogMsg& oLog, CString &oLogInfo);
	static void Select(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static CLogMonitor* GetMonitor(CCmdSession* pSession, bool bCreate=true);
};

struct CMonitorTable
{
	CMutex m_oMutex;
	CRbMap<CCmdSession*, CLogMonitor*> m_oSessionTable;

	CMonitorTable();
	~CMonitorTable();
	void Clear();
	static CMonitorTable* GetInstance();
};

FOCP_END();

#endif
