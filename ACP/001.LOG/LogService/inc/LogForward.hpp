
#include "LogListenor.hpp"

#ifndef _AFW_LOG_FORWARD_HPP_
#define _AFW_LOG_FORWARD_HPP_

FOCP_BEGIN();

class CLogForwarder: public CLogProcessor
{
private:
	CFile m_oFile;
	CIpAddr m_oServerAddr;
	CString m_oName;

public:
	CLogForwarder();
	virtual ~CLogForwarder();

	virtual void Process(CLogMsg& oLog);

	static void RegisterCmd();

private:
	bool Initialize(const char* sServerAddr, uint16 nPort);
	uint32 CreateLogMsg(CLogMsg& oLog, char sLogInfo[FOCP_LOG_MAXMSG]);

	static void Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Add(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Del(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void List(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
};

struct CForwarderTable
{
	CMutex m_oMutex;
	CRbMap<CString, CLogForwarder*> m_oFwdTable;

	CForwarderTable();
	~CForwarderTable();

	void Clear();
	void StartAll();

	static CForwarderTable* GetInstance();
};

FOCP_END();

#endif
