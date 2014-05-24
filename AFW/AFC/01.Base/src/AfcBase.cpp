
#include "AfcLog.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(WINDOWS) || defined(CYGWIN_NT)
#include <windows.h>
#endif

#ifdef UNIX
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef CYGWIN_NT
#include <sys/cygwin.h>
#endif

FOCP_BEGIN();

namespace
{
	FOCP_DETAIL_NAME::CRecursiveTest g_oRecursiveTest;
	FOCP_DETAIL_NAME::CThreadMutex g_oSystemLock;
}

AFCBASE_API void BaseCall(uint32 nCmd, void* pArg)
{
	switch(nCmd)
	{
	case AFC_RECURSIVE_LOCK:
		*(uint32*)((char*)pArg+sizeof(void*)) = g_oRecursiveTest.IsRecursive(*(void**)pArg)?1:0;
		break;
	case AFC_RECURSIVE_UNLOCK:
		g_oRecursiveTest.UnRecursive(pArg);
		break;
	case AFC_SYSTEM_LOCK:
		g_oSystemLock.Enter();
		break;
	case AFC_SYSTEM_UNLOCK:
		g_oSystemLock.Leave();
		break;
	}
}

namespace
{
#ifdef CYGWIN_NT

struct CRunInWindows
{
	bool m_bRunInWindows;
	CRunInWindows()
	{
		void* pDir = OpenDirectory("/tmp");
		if(pDir)
			CloseDirectory(pDir);
		m_bRunInWindows = (pDir==NULL);
	}
};

void GetPathSection(char* &sList, FOCP_DETAIL_NAME::CString &oPath)
{
	char* pShift = (char*)strchr(sList, ';');
	if(pShift)
		pShift[0] = '0';
	oPath = sList;
	if(pShift)
		sList = pShift+1;
	else
		sList = NULL;
}

struct CInitializePathEnv
{
	CInitializePathEnv()
	{
		if(RunInWindows())
		{
			const char* sVar = (const char*)getenv("path");
			if(sVar)
			{
				FOCP_DETAIL_NAME::CString oNewVar;
				FOCP_DETAIL_NAME::CString oVar2;
				oVar2 = sVar;
				char* sList = (char*)oVar2.GetStr();
				while(sList)
				{
					FOCP_DETAIL_NAME::CString oPath;
					GetPathSection(sList, oPath);
					char* sPath = (char*)oPath.GetStr();
					if(oNewVar.Empty())
						oNewVar += ":";
					if((sPath[0] >= 'A' && sPath[0] <= 'Z') || (sPath[0] >= 'a' && sPath[0] <= 'z'))
					{
						if(sPath[1] == ':')
						{
							char s[3] = {sPath[0], '\0'};
							if(s[0] >= 'A' && s[0] <= 'Z')
							{
								s[0] -= 'A';
								s[0] += 'a';
							}
							oNewVar += "/cygdrive/";
							oNewVar += s;
							sPath += 2;
						}
					}
					char* pShift = sPath;
					while(*pShift)
					{
						if(*pShift == '\\')
							*pShift = '/';
						++pShift;
					}
					oNewVar += sPath;
				}
				SetEnvVar("path", oNewVar.GetStr());
			}
		}
	}
};
CRunInWindows g_oRunInWindows;
CInitializePathEnv g_oInitializePathEnv;
#endif
}

AFCBASE_API const char* GetEnvVar(const char* sVarName)
{
	return (const char*)getenv(sVarName);
}

AFCBASE_API void SetEnvVar(const char* sVarName, const char* sValue)
{
	uint32 nLen = strlen(sVarName)+strlen(sValue)+2;
	char* s = new char[nLen];
	strcpy(s, sVarName);
	if(sValue && sValue[0])
	{
		strcat(s, "=");
		strcat(s, sValue);
	}
	putenv(s);
	delete[] s;
}

AFCBASE_API bool RunInWindows()
{
#if defined(WINDOWS)
	return true;
#elif defined(CYGWIN_NT)
	return g_oRunInWindows.m_bRunInWindows;
#else
	return false;
#endif
}

AFCBASE_API void GetProgramFileName(char* sFileName)
{
#if defined(WINDOWS)
	GetModuleFileName(NULL, sFileName, FOCP_MAX_PATH-1);
	sFileName[FOCP_MAX_PATH-1] = '\0';
#elif defined(CYGWIN_NT)
	GetModuleFileName(NULL, sFileName, FOCP_MAX_PATH-1);
	sFileName[FOCP_MAX_PATH-1] = '\0';
	if(!RunInWindows())
	{
		char s[FOCP_MAX_PATH];
		char* pShift = sFileName;
		while(pShift[0])
		{
			if(pShift[0] == '\\')
				pShift[0] = '/';
			++pShift;
		}
		if(!cygwin_conv_path(CCP_WIN_A_TO_POSIX, sFileName, s, sizeof(s)))
			strcpy(sFileName, s);
	}
#elif defined(LINUX)
	char sPath[30];
	sprintf(sPath, "/proc/%d/exe", getpid());
	readlink(sPath, sFileName, FOCP_MAX_PATH-1);
	sFileName[FOCP_MAX_PATH-1] = '\0';
#else
	strcpy(sFileName, GetEnvVar("_"));
#endif
}

namespace{ CLogManager g_oLogManager; }

AFCBASE_API void SetLogMode(const char* sModuleName, uint32 nMode)
{
	g_oLogManager.Lock();
	g_oLogManager.SetLogMode(sModuleName, nMode);
	g_oLogManager.UnLock();
}

AFCBASE_API uint32 GetLogMode(const char* sModuleName)
{
	g_oLogManager.Lock();
	uint32 nRet = g_oLogManager.GetLogMode(sModuleName);
	g_oLogManager.UnLock();
	return nRet;
}

AFCBASE_API void SetLogLevel(const char* sModuleName, uint32 nLevel)
{
	g_oLogManager.Lock();
	g_oLogManager.SetLogLevel(sModuleName, nLevel);
	g_oLogManager.UnLock();
}

AFCBASE_API uint32 GetLogLevel(const char* sModuleName)
{
	g_oLogManager.Lock();
	uint32 nRet = g_oLogManager.GetLogLevel(sModuleName);
	g_oLogManager.UnLock();
	return nRet;
}

AFCBASE_API void SetLogFilter(const char* sModuleName, uint32 nFilter)
{
	g_oLogManager.Lock();
	g_oLogManager.SetLogFilter(sModuleName, nFilter);
	g_oLogManager.UnLock();
}

AFCBASE_API uint32 GetLogFilter(const char* sModuleName)
{
	g_oLogManager.Lock();
	uint32 nRet = g_oLogManager.GetLogFilter(sModuleName);
	g_oLogManager.UnLock();
	return nRet;
}

AFCBASE_API void SetLogInstance(uint32 nDmn, uint32 nAin)
{
	g_oLogManager.Lock();
	g_oLogManager.SetLogInstance(nDmn, nAin);
	g_oLogManager.UnLock();
}

AFCBASE_API void WriteLog(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, ...)
{
	CVaList args;
	VaStart(args, sLogInfo);
	WriteLogV(sModuleName, nLevel, sFileName, sFunction, nLine, sLogInfo, args);
}

AFCBASE_API void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr)
{
	g_oLogManager.WriteLogV(sModuleName, nLevel, sFileName, sFunction, nLine, sLogInfo, argptr);
}

AFCBASE_API void WriteLog1(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine)
{
	g_oLogManager.WriteLog1(sModuleName, nLevel, sFileName, sFunction, nLine);
}

AFCBASE_API void WriteLog2(const char* sLogInfo, ...)
{
	CVaList args;
	VaStart(args, sLogInfo);
	g_oLogManager.WriteLog2(sLogInfo, args);
}

AFCBASE_API bool HaveModule(const char* sModuleName)
{
	return g_oLogManager.HaveModule(sModuleName);
}

AFCBASE_API void WalkLogModule(void *pPara, void(*OnWalk)(void*pPara, const char* sModule))
{
	g_oLogManager.WalkLogModule(pPara, OnWalk);
}

namespace{

struct CFocpVersionNode
{
	CModuleName oVersionName;
	CModuleName oVersionNo;
	CFocpVersionNode* pNext;

	CFocpVersionNode(const char* sVersionName, const char* sVersionNo):
		oVersionName(sVersionName), oVersionNo(sVersionNo)
	{
		pNext = NULL;
	}

	~CFocpVersionNode()
	{
		if(pNext)
		{
			delete pNext;
			pNext = NULL;
		}
	}
	void PrintVersionInfo()
	{
		printf("%s=%s\n", oVersionName.sName, oVersionNo.sName);
		if(pNext)
			pNext->PrintVersionInfo();
	}
};

struct CFocpVersionList
{
	CFocpVersionNode* pHead, *pTail;
	
	CFocpVersionList()
	{
		pHead = NULL;
		pTail = NULL;
	}
	~CFocpVersionList()
	{
		if(pHead)
			delete pHead;
		pHead = NULL;
		pTail = NULL;
	}
	void AddVersion(const char* sVersionName, const char* sVersionNo)
	{
		CFocpVersionNode* pNode = new CFocpVersionNode(sVersionName, sVersionNo);
		if(pTail)
			pTail->pNext = pNode;
		else
			pHead = pNode;
		pTail = pNode;
	}
	void PrintVersionInfo()
	{
		if(pHead)
			pHead->PrintVersionInfo();
	}
};
CFocpVersionList g_oVersionList;
}

CFocpVersion::CFocpVersion(const char* sVersionName, const char* sVersionNo)
{
	g_oVersionList.AddVersion(sVersionName, sVersionNo);
}

void CFocpVersion::PrintVersionInfo()
{
	g_oVersionList.PrintVersionInfo();
}

#ifdef FOCP_VERSION
static CFocpVersion oFocpVersion("focp", FOCP_STRING_DEFINE(FOCP_VERSION));
#endif

FOCP_END();
