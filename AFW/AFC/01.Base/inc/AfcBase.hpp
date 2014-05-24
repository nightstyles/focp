

#include "../../02.ADT/inc/Convention.hpp"

#ifndef _AFCBASE_HPP_
#define _AFCBASE_HPP_

#ifdef AFCBASE_EXPORTS
#define AFCBASE_API FOCP_EXPORT
#else
#define AFCBASE_API FOCP_IMPORT
#endif

#define AFC_RECURSIVE_LOCK 1
#define AFC_RECURSIVE_UNLOCK 2
#define AFC_SYSTEM_LOCK 3
#define AFC_SYSTEM_UNLOCK 4

FOCP_C_BEGIN();

enum
{
	//日志级别定义：
	FOCP_LOG_SYSLOG=1,
	FOCP_LOG_ERROR=2,
	FOCP_LOG_WARNING=3,
	FOCP_LOG_DEBUG=4,
	FOCP_LOG_CLOSE=5,
	//日志模式定义：
	FOCP_LOG_SCREEN=1,
	FOCP_LOG_FILE=2,
	FOCP_LOG_SERVER=4,
	//日志过滤方式：
	FOCP_LOG_HOST=1,
	FOCP_LOG_DATE=2,
	FOCP_LOG_APPN=4,
	FOCP_LOG_MODU=8,
	FOCP_LOG_SRCF=16,
	FOCP_LOG_FUNC=32,
	//日志消息大小
	FOCP_LOG_MAX=3072,
	FOCP_LOG_MAXMSG=4096,
	//日志服务端口
	FOCP_LOG_PORT=2269,
	//日志文件大小
	FOCP_LOG_MAXFILE=10,
	FOCP_LOG_MINFILE=1,
	FOCP_LOG_FILENO=999,
};

FOCP_C_END();

FOCP_BEGIN();

class AFCBASE_API CFocpVersion
{
public:
	CFocpVersion(const char* sVersionName, const char* sVersionNo);
	static void PrintVersionInfo();
};

AFCBASE_API void BaseCall(uint32 nCmd, void* pArg);
AFCBASE_API const char* GetEnvVar(const char* sVarName);
AFCBASE_API void SetEnvVar(const char* sVarName, const char* sValue);
AFCBASE_API void GetProgramFileName(char* sOsFileName);

AFCBASE_API void* OpenDirectory(const char* sOsDirectory);
AFCBASE_API void CloseDirectory(void* pDirectory);
AFCBASE_API const char* ReadDirectory(void* pDirectory, bool &bIsDirectory);
AFCBASE_API void RewindDirectory(void* pDirectory);

AFCBASE_API bool RunInWindows();

AFCBASE_API const char* GetLogLevelName(uint32 nLevel);

//sModuleName为NULL或'\0'表示全局日志级别
AFCBASE_API bool HaveModule(const char* sModuleName);
AFCBASE_API void SetLogMode(const char* sModuleName, uint32 nMode);
AFCBASE_API uint32 GetLogMode(const char* sModuleName);
AFCBASE_API void SetLogLevel(const char* sModuleName, uint32 nLevel);
AFCBASE_API uint32 GetLogLevel(const char* sModuleName);
AFCBASE_API void SetLogFilter(const char* sModuleName, uint32 nFilter);
AFCBASE_API uint32 GetLogFilter(const char* sModuleName);
AFCBASE_API void SetLogInstance(uint32 nDmn, uint32 nAin);
AFCBASE_API void WalkLogModule(void *pPara, void(*OnWalk)(void*pPara, const char* sModule));

AFCBASE_API void WriteLog(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, ...);
AFCBASE_API void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
AFCBASE_API void WriteLog1(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine);
AFCBASE_API void WriteLog2(const char* sLogInfo, ...);

FOCP_END();

#ifdef MSVC
#if (_MSC_VER < 1300)
extern "C" AFCBASE_API const char* GetFuncName();
#define __FUNCTION__ GetFuncName()
#endif
#endif // MSVC

/////////////////////////////////////////////////////
//以下日志宏的格式串，仅支持标准C的定义。不支持本库04.AFS中的模式。
/////////////////////////////////////////////////////
#ifdef FOCP_MODULE_NAME
#define FocpLog(nLogLevel, sLogInfo) \
	do{\
		FOCP_NAME::WriteLog1(FOCP_STRING_DEFINE(FOCP_MODULE_NAME), nLogLevel, __FILE__, __FUNCTION__, __LINE__); \
		FOCP_NAME::WriteLog2 sLogInfo; \
	}while(0)
#define FocpLogV(nLogLevel, sLogInfo, argptr) \
	do{\
		FOCP_NAME::WriteLogV(FOCP_STRING_DEFINE(FOCP_MODULE_NAME), nLogLevel, __FILE__, __FUNCTION__, __LINE__, sLogInfo, argptr);\
	}while(0)
#else
#define FocpLog(nLogLevel, sLogInfo) \
	do{\
		FOCP_NAME::WriteLog1("", nLogLevel, __FILE__, __FUNCTION__, __LINE__); \
		FOCP_NAME::WriteLog2 sLogInfo; \
	}while(0)
#define FocpLogV(nLogLevel, sLogInfo, argptr) \
	do{\
		FOCP_NAME::WriteLogV("", nLogLevel, __FILE__, __FUNCTION__, __LINE__, sLogInfo, argptr);\
	}while(0)
#endif

#define FocpInfo(sLogInfo) FocpLog(FOCP_LOG_SYSLOG, sLogInfo)
#define FocpError(sLogInfo) FocpLog(FOCP_LOG_ERROR, sLogInfo)
#define FocpWarn(sLogInfo) FocpLog(FOCP_LOG_WARNING, sLogInfo)
#define FocpDebug(sLogInfo) FocpLog(FOCP_LOG_DEBUG, sLogInfo)

#define FocpInfoV(sLogInfo, argptr) FocpLogV(FOCP_LOG_SYSLOG, sLogInfo, argptr)
#define FocpErrorV(sLogInfo, argptr) FocpLogV(FOCP_LOG_ERROR, sLogInfo, argptr)
#define FocpWarnV(sLogInfo, argptr) FocpLogV(FOCP_LOG_WARNING, sLogInfo, argptr)
#define FocpDebugV(sLogInfo, argptr) FocpLogV(FOCP_LOG_DEBUG, sLogInfo, argptr)

#define FocpInfoS(sLogInfo) FocpInfo(("%s", sLogInfo))
#define FocpErrorS(sLogInfo) FocpError(("%s", sLogInfo))
#define FocpWarnS(sLogInfo) FocpWarn(("%s", sLogInfo))
#define FocpDebugS(sLogInfo) FocpDebug(("%s", sLogInfo))

#define FocpLogEx(sModuleName, nLogLevel, sLogInfo) \
	do{\
		FOCP_NAME::WriteLog1(sModuleName, nLogLevel, __FILE__, __FUNCTION__, __LINE__); \
		FOCP_NAME::WriteLog2 sLogInfo; \
	}while(0)

#define FocpLogVx(sModuleName, nLogLevel, sLogInfo, argptr) \
	FOCP_NAME::WriteLogV(sModuleName, nLogLevel, __FILE__, __FUNCTION__, __LINE__, sLogInfo, argptr)

#endif
