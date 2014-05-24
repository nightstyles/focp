
#include "AsfDef.hpp"

#ifndef _Asf_CmdLine_Hpp_
#define _Asf_CmdLine_Hpp_

FOCP_BEGIN();

class ASF_API CCmdSession;
class ASF_API CCmdSystem;

typedef void (*FOnProcessSession)(CCmdSession* pSession);

enum
{
	FOCP_CLI_MAXHISTORY = 256,
};

class ASF_API CHistoryCmd
{
private:
	char *m_history[FOCP_CLI_MAXHISTORY];

public:
	CHistoryCmd();
	~CHistoryCmd();

	void AddHistory(char* cmd, int32 l);
	void List(CCmdSession* pSession);
	void Clear();
	char* HistoryCmd(uint32 i);
};

class ASF_API CCmdSession
{
	FOCP_FORBID_COPY(CCmdSession);
private:
	CSingleList<CFormatFile*> m_oFileList;
	CFormatFile* m_pFile;

protected:
	CString m_oLine;
	bool m_bQuit;
	bool m_bHide;
	bool m_bForbidSave;
	uint32 m_nMax;
	CHistoryCmd* m_pHistoryCmd;

public:
	CCmdSession();
	virtual ~CCmdSession();

	virtual void Print(const char *sFormat, ...);
	virtual void PrintV(const char* sFormat, CVaList& pArgList);
	virtual void SetQuit();
	virtual void ClearScreen();

	void SetHidden(bool bHide);//for ReadLine
	void SetMaxRead(uint32 nMax);
	void ForbidSave(bool bForbidSave);

	void SetHistoryCmd(CHistoryCmd* pHistoryCmd);
	CHistoryCmd* GetHistoryCmd();
	char* HistoryCmd(uint32 i);
	void AddHistory(char* cmd, int32 l);

	virtual const char* ReadLine(bool bPrompt=true, char cPrompt='#');

	void ProcessFile(const char* sFileName);
	void ProcessScript(const char* sFileName);
	void ProcessText(const char* sScript);

	static CCmdSession* GetSession();
	static void SetSession(CCmdSession* pSession);
};

typedef void (*CCmdFunc) (CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
typedef void (*CSysCmdFunc) (CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);

FOCP_DETAIL_BEGIN();

struct CLeafNode
{
	CString oDesc;
	CCmdFunc fCallBack;
};

class CCmdNode
{
	friend class CCmdSystem;
public:
	bool m_bBranch;
	union CNodeInfo
	{
		CLeafNode* pLeaf;
		CRbMap<CString, CCmdNode*, CNameCompare>* pBranch;
	} m_oNode;
	CCmdNode* m_pParent;
	const char* m_sName;

public:
	~CCmdNode();
	CCmdNode();
	CCmdNode(const char* sDescription, CCmdFunc fCallBack);

	CCmdNode* FindNode(const char* sName);
	bool RegisterNode(const char* sName, CCmdNode* pNode);
};

FOCP_DETAIL_END();

class ASF_API CCmdSystem
{
private:
	struct CSysCmdNode
	{
		CString oDesc;
		CSysCmdFunc fCallBack;
	};
private:
	CMutex m_oMutex;
	FOCP_DETAIL_NAME::CCmdNode m_oRoot;
	FOCP_DETAIL_NAME::CCmdNode* m_pCurrent, *m_pPrev;
	CRbMap<CString, CSysCmdNode, CNameCompare> m_oSysCmdList;

public:
	CCmdSystem();
	~CCmdSystem();

	static CCmdSystem* GetInstance();

	void RegisterCmd(const char* sName, const char* sDescription, CCmdFunc fCallBack);
	void RegisterSysCmd(const char* sName, const char* sDescription, CSysCmdFunc fCallBack);

	void ProcessCmd(CCmdSession* pSession, const char *sCmdLine);

	void GetCurrentPath(CString &oPath);

	int32 GetCompletions(char *sCommand, char **sCompletions, int32 max_completions);

private:
	bool ParseCmdTree(const char* sName, CVector<CString> &oNames);
	FOCP_DETAIL_NAME::CCmdNode* GetCmdNode(const char *&sCmdLine);
	static void List(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Pwd(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Help(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Cd(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Quit(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Clear(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Stop(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Call(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);

	static void SetLogModeCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void GetLogModeCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void SetLogLevelCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void GetLogLevelCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void SetLogFilterCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void GetLogFilterCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void ListLogModuleCmd(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);

	void GetIdentifier(const char* &sName, CString& oName);
	void GetFullPath(FOCP_DETAIL_NAME::CCmdNode* pNode, CString &oPath);
};

FOCP_END();

#define FocpCmdLog(nLogLevel, sLogInfo) \
	do{\
		FOCP_NAME::CCmdSession* pSession = FOCP_NAME::CCmdSession::GetSession(); \
		FocpLog(nLogLevel, sLogInfo); \
		pSession->Print sLogInfo; \
		pSession->Print("\r\n"); \
	}while(0)

#define FocpCmdLogEx(sModuleName, nLogLevel, sLogInfo) \
	do{\
		FOCP_NAME::CCmdSession* pSession = FOCP_NAME::CCmdSession::GetSession(); \
		FocpLogEx(sModuleName, nLogLevel, sLogInfo); \
		pSession->Print sLogInfo; \
		pSession->Print("\r\n"); \
	}while(0)

#endif
