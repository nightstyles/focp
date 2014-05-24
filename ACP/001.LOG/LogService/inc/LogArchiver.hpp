
#include "LogListenor.hpp"

#ifndef _AFW_LOG_ARCHIVER_HPP_
#define _AFW_LOG_ARCHIVER_HPP_

FOCP_BEGIN();
class CLogArchiver: public CLogProcessor
{
private:
	CFile m_oFile;
	uint32 m_nLogSize;
	uint32 m_nLogId, m_nArchId;
	uint32 m_nLogNo, m_nArchNo;
	uint32 m_nFilter;
	CString m_oLogDir, m_oArchDir, m_oLogName, m_oFileName;
	bool m_bNeedArch;

public:
	CLogArchiver();
	virtual ~CLogArchiver();

	virtual void Process(CLogMsg& oLog);

	static void RegisterCmd();

private:
	void DoArchive(bool bDispErr=true);
	void GetLogInfo(CLogMsg& oLog, CString &oLogInfo);
	void CreateLogFile();
	bool NotAccess();

	bool Initialize(const char* sLogName, const char* sLogDir, uint32 nMaxLogNo, uint32 nLogSize,
					bool bNeedArch=true, const char* sArchDir=NULL, uint32 nMaxArchNo=0);

	static void Select(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Push(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Pop(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Clear(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Query(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Add(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Del(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void List(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void Begin(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
	static void End(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);

	static uint32 GetNameValue(const char* &pStr, uint32 &nName, CString &oValue, uint32 &nValue);
	static uint32 GetNameType(const CString &oName, uint32 &nName, bool &bInt);
	static uint32 GetInt(const char* &pStr, CString &oValue);
	static uint32 GetString(const char* &pStr, CString &oValue);
	static uint32 GetIdentifier(const char* &pStr, CString &oIdentifier);
	static void SkipWhiteSpace(const char* &pStr);
};

struct CArchiverTable
{
	CMutex m_oMutex;
	CRbMap<CString, CLogArchiver*> m_oArchiverTable;

	CArchiverTable();
	~CArchiverTable();

	void Clear();
	void StartAll();

	static CArchiverTable* GetInstance();
};

FOCP_END();

#endif
