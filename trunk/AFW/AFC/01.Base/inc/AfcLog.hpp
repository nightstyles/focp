
#include "AfcThread.hpp"
#include "List.hpp"

#ifdef WINDOWS
#include <windows.h>
#else
#include <netinet/in.h>
#endif

#include <stdio.h>

#ifndef _AFC_LOG_HPP_
#define _AFC_LOG_HPP_

/*
日志简易设计方案：
1. 放置于AfcBase模块，是为了让系统一启动就有日志能力。
2. 日志模块的初始化是内部自动进行的，无需外部特别代码级支持。
3. 日志初始化的参数全部通过环境变量传递，为第2点提供技术保障。
3. 日志模块向外仅提供基础函数：
	void WriteLog(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, ...);
	void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog1(const char* sModuleName, uint32 nLevel, CString &oFileName, const char *sFunction, int nLine);
	void WriteLog2(const char* sLogInfo, ...);

	//sModuleName为NULL或'\0'表示全局日志级别
	void SetLogMode(const char* sModuleName, uint32 nMode);
	uint32 GetLogMode(const char* sModuleName);
	void SetLogLevel(const char* sModuleName, uint32 nLevel);
	uint32 GetLogLevel(const char* sModuleName);
	void SetLogFilter(const char* sModuleName, uint32 nLogFilter);
	uint32 GetLogFilter(const char* sModuleName);
	void SetLogHold(const char* sModuleName, bool bHold);
	bool GetLogHold(const char* sModuleName);

4. 日志初始参数定义：
	FocpLogLevel，定义全局的日志级别，缺省值为FOCP_LOG_ERROR.
		FOCP_LOG_SYSLOG=1, //必须要打印的日志，但不表示错误
		FOCP_LOG_ERROR=2,  //错误信息
		FOCP_LOG_WARNING=3,//告警信息
		FOCP_LOG_DEBUG=4,  //调试信息
		FOCP_LOG_CLOSE=5,  //
	FocpLogMode，定义日志打印方式（可位合），缺省为1
		FOCP_LOG_SCREEN=1, //屏幕打印日志
		FOCP_LOG_FILE=2, //在文件中打印日志
			FocpLogFileSize, 定义日志文件的最大大小（单位M），缺省10.
			FocpLogFileNo，定义最大文件号，缺省999（即从001~999）
		FOCP_LOG_SERVER=4, //将日志发往日志服务器
			FocpLogServerAddr，定义日志服务器地址，缺省为127.0.0.1
			FocpLogServerPort，定义日志服务器端口，缺省为2269
	FocpLogFilter，定义日志过滤模式（可位合）：缺省为15
		FOCP_LOG_HOST=1, //将打印主机信息
		FOCP_LOG_DATE=2, //将打印时间信息
		FOCP_LOG_APPN=4, //将打印应用信息[包括实例号]
		FOCP_LOG_MODU=8, //将打印模块信息
		FOCP_LOG_SRCF=16,//将打印文件信息[包括行号]，如果包含该标示，打印具体日志时将换行，并跳2个空格。
		FOCP_LOG_FUNC=32,//将打印函数信息，如果包含该标示，打印具体日志时将换行，并跳2个空格。
5. 日志服务器设计：另行设计
*/

FOCP_BEGIN();

struct  CAfcLogAttr
{
	uint32 nMode;
	uint32 nLevel;
	uint32 nFilter;
};

struct CLogInfo
{
	char sDate[20];
	char sLogInfo[FOCP_LOG_MAX];
	const char* sAppName;
	const char* sModuleName;
	const char* sFileName;
	const char* sFunction;
	uint32 nLevel, nLine;
};

struct CLogPosInfo
{
	const char* sAppName;
	const char* sModuleName;
	const char* sFileName;
	const char* sFunction;
	uint32 nLevel, nLine;
};

struct CModuleName
{
	char* sName;

	~CModuleName();
	CModuleName(const char* s=NULL);
	CModuleName(const CModuleName& oSrc);
	CModuleName& operator=(const CModuleName& oSrc);
	CModuleName& operator=(const char* s);
	bool Empty();
};

struct CModuleNameCompare
{
	static int32 Compare(const CModuleName*pLeft, const CModuleName* pRight);
};

class CLogManager
{
private:
	CRbMap<CModuleName, CAfcLogAttr, CModuleNameCompare> m_oLogAttr;//以模块分离
	CSingleList<CLogInfo> m_oLogBuffer;
	CLogPosInfo m_oLogPos;
	uint32 m_nLogFileSize;
	uint32 m_nLogFileNo;
	uint32 m_nDmn, m_nAin;
	char m_sHostIp[20];
	bool m_bHold, m_bLocked;

	char m_sHome[FOCP_MAX_PATH];
	char* m_sApp;
	char m_sName[FOCP_MAX_PATH];
	char m_sFileName[FOCP_MAX_PATH];
	FILE* m_pFile;
	uint32 m_nFileId;

#ifdef WINDOWS
	SOCKET m_nSocket;
#endif

#ifdef UNIX
	int32 m_nSocket;
#endif

	sockaddr_in m_oServerAddr;
	FOCP_DETAIL_NAME::CThreadMutex m_oMutex;

public:
	CLogManager();
	~CLogManager();

	void SetLogMode(const char* sModuleName, uint32 nMode);
	uint32 GetLogMode(const char* sModuleName);

	void SetLogLevel(const char* sModuleName, uint32 nLevel);
	uint32 GetLogLevel(const char* sModuleName);

	void SetLogFilter(const char* sModuleName, uint32 nLogFilter);
	uint32 GetLogFilter(const char* sModuleName);

	void SetLogInstance(uint32 nDmn, uint32 nAin);

	void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog1(const char* sModuleName, uint32 nLevel, const char* sFileName, const char *sFunction, int nLine);
	void WriteLog2(const char* sLogInfo, CVaList& args);

	bool HaveModule(const char* sModuleName);
	void WalkLogModule(void *pPara, void(*OnWalk)(void*pPara, const char* sModule));

	void Lock();
	void UnLock();

private:
	CAfcLogAttr& GetLogAttr(const char* sModuleName);
	void GetLogDate(char sDate[20]);
	void GetFileDate(char sDate[20]);

	uint32 CreateLogMsg(CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG]);
	uint32 GetLogInfo(uint32 nFilter, CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG]);
	void CreateLogFile();
	bool NotAccess();

	void PrintScreen(uint32 nLevel, char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);
	void PrintFile(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);
	void PrintServer(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);

	void WriteLogV2(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog(CLogInfo &oLogInfo, bool bSupportNet=true);

	bool GetHostIpList(char sHostIp[20]);
};

FOCP_END();

#endif
