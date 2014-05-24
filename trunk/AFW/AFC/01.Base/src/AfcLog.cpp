
//用ADT中头文件时，不能牵涉.cpp文件。
#include "AfcLog.hpp"

#if defined(WINDOWS)
	#include <io.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/file.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <errno.h>
    #include <arpa/inet.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifndef F_OK
#define F_OK 0
#endif

#ifdef MSVC
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

FOCP_BEGIN();

const char* g_sLogLevel[] = {"SYS", "ERR", "WAR", "DBG" };

AFCBASE_API const char* GetLogLevelName(uint32 nLevel)
{
	if(nLevel > FOCP_LOG_DEBUG || nLevel < FOCP_LOG_SYSLOG)
		return NULL;
	return g_sLogLevel[nLevel - 1];
}

CModuleName::~CModuleName()
{
	if(sName)
	{
		delete[] sName;
		sName = NULL;
	}
}

CModuleName::CModuleName(const char* s)
{
	sName = NULL;
	if(s && s[0])
	{
		uint32 nLen = strlen(s) + 1;
		sName = new char[nLen];
		strcpy(sName, s);
	}
}

CModuleName::CModuleName(const CModuleName& oSrc)
{
	sName = NULL;
	if(oSrc.sName)
	{
		uint32 nLen = strlen(oSrc.sName) + 1;
		sName = new char[nLen];
		strcpy(sName, oSrc.sName);
	}
}

CModuleName& CModuleName::operator=(const CModuleName& oSrc)
{
	if(this != &oSrc)
	{
		if(sName)
		{
			delete[] sName;
			sName = NULL;
		}
		sName = NULL;
		if(oSrc.sName)
		{
			uint32 nLen = strlen(oSrc.sName) + 1;
			sName = new char[nLen];
			strcpy(sName, oSrc.sName);
		}
	}
	return *this;
}

CModuleName& CModuleName::operator=(const char* s)
{
	if(sName)
	{
		delete[] sName;
		sName = NULL;
	}
	sName = NULL;
	if(s && s[0])
	{
		uint32 nLen = strlen(s) + 1;
		sName = new char[nLen];
		strcpy(sName, s);
	}
	return *this;
}

bool CModuleName::Empty()
{
	return sName==NULL;
}

static int32 StringCompare(const char* sLeft, const char* sRight, bool bSensitive=true, uint32 nMaxCmp = 0)
{
	if(sLeft == sRight)
		return 0;
	if(sLeft == NULL)
		return -1;
	if(sRight == NULL)
		return 1;
	const uint8* pLeft = (const uint8*)sLeft;
	const uint8* pRight = (const uint8*)sRight;
	for(uint32 i=0; ((nMaxCmp==0) || (i<nMaxCmp)); ++i)
	{
		if(bSensitive)
		{
			if(pLeft[i] > pRight[i])
				return 1;
			else if(pLeft[i] < pRight[i])
				return -1;
		}
		else
		{
			uint8 c1 = pLeft[i];
			uint8 c2 = pRight[i];
			if(c1 >= 'a' && c1 <= 'z')
				c1 -= 'a' - 'A';
			if(c2 >= 'a' && c2 <= 'z')
				c2 -= 'a' - 'A';
			if(c1 > c2)
				return 1;
			else if(c1 < c2)
				return -1;
		}
		if(pLeft[i] == 0)
			break;
	}
	return 0;
}

int32 CModuleNameCompare::Compare(const CModuleName*pLeft, const CModuleName* pRight)
{
	return StringCompare(pLeft->sName, pRight->sName, false);
}

bool CLogManager::GetHostIpList(char sHostIp[20])
{
	struct hostent *hptr;
	char cHostName[256];
	m_oMutex.Enter();
	if (gethostname(cHostName, sizeof(cHostName)))
	{
		m_oMutex.Leave();
		return false;
	}
	hptr = gethostbyname(cHostName);
	if (NULL == hptr)
	{
		m_oMutex.Leave();
		return false;
	}
	uint8* pIp = (uint8*)(hptr->h_addr_list[0]);
	sprintf(sHostIp, "%u.%u.%u.%u", (uint32)pIp[0],(uint32)pIp[1],(uint32)pIp[2],(uint32)pIp[3]);
	m_oMutex.Leave();
	return true;
}

static char * GetAppName(char* sFullName)
{
	uint32 nLen = strlen(sFullName);
	char* pEnd = sFullName + nLen - 1;
	while(pEnd[0] != '/' && pEnd[0] != '\\')
		--pEnd;
	char* sApp = pEnd + 1;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	pEnd = sFullName + nLen;
	if(pEnd - sApp > 4)
	{
		pEnd -= 4;
		if( (pEnd[0] = '.') &&
				((pEnd[1] = 'e') || (pEnd[1] = 'E')) &&
				((pEnd[2] = 'x') || (pEnd[2] = 'X')) &&
				((pEnd[2] = 'e') || (pEnd[2] = 'E'))
		  )
			pEnd[0] = '\0';
	}
#endif
	return sApp;
}

static void GetAppHome(char* sFullName, char* sAppName)
{
	sAppName--;
	sAppName[0] = '\0';
	while(sFullName != sAppName && sAppName[0] != '/' && sAppName[0] != '\\')
		--sAppName;
	if(sFullName != sAppName)
		sAppName[0] = '\0';
}

CLogManager::CLogManager()
{
	uint32 nVal;
	const char* s;

	//FocpLogFileSize
	s = GetEnvVar("FocpLogFileSize");
	nVal = FOCP_LOG_MAXFILE;
	if(s)
	{
		nVal = atoi(s);
		if(nVal < FOCP_LOG_MINFILE)
			nVal = FOCP_LOG_MINFILE;
		else if(nVal > FOCP_LOG_MAXFILE)
			nVal = FOCP_LOG_MAXFILE;
	}
	m_nLogFileSize = nVal*1048576;

	//FocpLogFileSize
	s = GetEnvVar("FocpLogFileNo");
	nVal = FOCP_LOG_FILENO;
	if(s)
	{
		nVal = atoi(s);
		if(nVal < 9)
			nVal = 9;
		else if(nVal > FOCP_LOG_FILENO)
			nVal = FOCP_LOG_FILENO;
	}
	m_nLogFileNo = nVal;

	m_nDmn = 0;
	m_nAin = 0;
	m_sHostIp[0] = '\0';
	GetHostIpList(m_sHostIp);

	m_bHold = true;

	m_bLocked = false;

	char sDate[20];
	GetFileDate(sDate);
	GetProgramFileName(m_sHome);
	m_sApp = GetAppName(m_sHome);
	GetAppHome(m_sHome, m_sApp);
	sprintf(m_sName, "%s%s", m_sApp, sDate);

	m_pFile = NULL;
	m_nFileId = 0;

	m_nSocket = socket(AF_INET, SOCK_DGRAM, 0);

	//FocpLogServerPort
	uint16 nPort = 2269;
	s = GetEnvVar("FocpLogServerPort");
	if(s)
	{
		nVal = atoi(s);
		if(nVal && nVal <= 65535)
			nPort = (uint16)nVal;
	}

	uint32 nAddr;
	s =  GetEnvVar("FocpLogServerAddr");
	if(s)
	{
		nAddr = inet_addr(s);
		if(nAddr == (uint32)(-1))
		{
			m_oMutex.Enter();
			struct hostent *hptr = gethostbyname(s);
			if(hptr)
				nAddr = *(uint32*)(hptr->h_addr_list[0]);
			else
				nAddr = inet_addr("127.0.01");
			m_oMutex.Leave();
		}
	}
	else
		nAddr = inet_addr("127.0.0.1");

	m_oServerAddr.sin_family = AF_INET;
	m_oServerAddr.sin_port = htons(nPort);
	m_oServerAddr.sin_addr.s_addr = nAddr;
}

CLogManager::~CLogManager()
{
	if(m_bHold)
	{
		m_bHold = false;
		void* pNode = m_oLogBuffer.First();
		while(pNode)
		{
			CLogInfo& oLogInfo = m_oLogBuffer.GetItem(pNode);
			WriteLog(oLogInfo, false);
			pNode = m_oLogBuffer.GetNext(pNode);
		}
		m_oLogBuffer.Clear();
	}
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}

#ifdef WINDOWS
	if(m_nSocket != (SOCKET)(-1))
	{
		closesocket(m_nSocket);
		m_nSocket = (SOCKET)(-1);
	}
#endif
#ifdef UNIX
	if(m_nSocket != -1)
	{
		close(m_nSocket);
		m_nSocket = -1;
	}
#endif
}

void CLogManager::SetLogMode(const char* sModuleName, uint32 nMode)
{
	uint32 nVal = 0;
	if(nMode & FOCP_LOG_SCREEN)
		nVal |= FOCP_LOG_SCREEN;
	if(nMode & FOCP_LOG_FILE)
		nVal |= FOCP_LOG_FILE;
	if(nMode & FOCP_LOG_SERVER)
		nVal |= FOCP_LOG_SERVER;

	if(nVal)
	{
		m_oMutex.Enter();
		GetLogAttr(sModuleName).nMode = nVal;
		m_oMutex.Leave();
	}
}

uint32 CLogManager::GetLogMode(const char* sModuleName)
{
	return GetLogAttr(sModuleName).nMode;
}

void CLogManager::SetLogLevel(const char* sModuleName, uint32 nLevel)
{
	if(nLevel >= FOCP_LOG_ERROR && nLevel <= FOCP_LOG_CLOSE)
		GetLogAttr(sModuleName).nLevel = nLevel;
}

uint32 CLogManager::GetLogLevel(const char* sModuleName)
{
	return GetLogAttr(sModuleName).nLevel;
}

void CLogManager::SetLogFilter(const char* sModuleName, uint32 nLogFilter)
{
	uint32 nVal = 0;
	if(nLogFilter & FOCP_LOG_HOST)
		nVal |= FOCP_LOG_HOST;
	if(nLogFilter & FOCP_LOG_DATE)
		nVal |= FOCP_LOG_DATE;
	if(nLogFilter & FOCP_LOG_APPN)
		nVal |= FOCP_LOG_APPN;
	if(nLogFilter & FOCP_LOG_MODU)
		nVal |= FOCP_LOG_MODU;
	if(nLogFilter & FOCP_LOG_SRCF)
		nVal |= FOCP_LOG_SRCF;
	if(nLogFilter & FOCP_LOG_FUNC)
		nVal |= FOCP_LOG_FUNC;

	if(nVal)
		GetLogAttr(sModuleName).nFilter = nVal;
}

uint32 CLogManager::GetLogFilter(const char* sModuleName)
{
	return GetLogAttr(sModuleName).nFilter;
}

void CLogManager::SetLogInstance(uint32 nDmn, uint32 nAin)
{
	if(m_bHold)
	{
		m_nDmn = nDmn;
		m_nAin = nAin;
		m_bHold = false;
		void* pNode = m_oLogBuffer.First();
		while(pNode)
		{
			CLogInfo& oLogInfo = m_oLogBuffer.GetItem(pNode);
			WriteLog(oLogInfo);
			pNode = m_oLogBuffer.GetNext(pNode);
		}
		m_oLogBuffer.Clear();
	}
}

void CLogManager::WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr)
{
	if(sModuleName == NULL || sModuleName[0] == '\0')
		sModuleName = "MMM";
	if(sFileName == NULL || sFileName[0] == '\0')
		sFileName = "FFF";
	if(sFunction == NULL || sFunction[0] == '\0')
		sFunction = "???";
	m_oMutex.Enter();
	CAfcLogAttr& oLogAttr = GetLogAttr(sModuleName);
	CAfcLogAttr& oGlobalAttr = GetLogAttr("");
	if(oLogAttr.nLevel == FOCP_LOG_CLOSE || 
	   oGlobalAttr.nLevel == FOCP_LOG_CLOSE ||
	   oLogAttr.nMode == 0 ||
	   oGlobalAttr.nMode == 0)
	{
		m_oMutex.Leave();
		return;
	}
	if(oLogAttr.nLevel >= nLevel && oGlobalAttr.nLevel >= nLevel)
		WriteLogV2(sModuleName, nLevel, sFileName, sFunction, nLine, sLogInfo, argptr);
	m_oMutex.Leave();
}

void CLogManager::WriteLog1(const char* sModuleName, uint32 nLevel, const char* sFileName, const char *sFunction, int nLine)
{
	if(sModuleName == NULL || sModuleName[0] == '\0')
		sModuleName = "MMM";
	if(sFileName == NULL || sFileName[0] == '\0')
		sFileName = "FFF";
	if(sFunction == NULL || sFunction[0] == '\0')
		sFunction = "???";
	m_oMutex.Enter();
	CAfcLogAttr& oLogAttr = GetLogAttr(sModuleName);
	CAfcLogAttr& oGlobalAttr = GetLogAttr("");
	if(oLogAttr.nLevel == FOCP_LOG_CLOSE || 
	   oGlobalAttr.nLevel == FOCP_LOG_CLOSE ||
	   oLogAttr.nMode == 0 ||
	   oGlobalAttr.nMode == 0)
		return;
	if(oLogAttr.nLevel >= nLevel && oGlobalAttr.nLevel >= nLevel)
	{
		m_bLocked = true;
		m_oLogPos.sAppName = m_sApp;
		m_oLogPos.sModuleName = sModuleName;
		m_oLogPos.sFileName = sFileName;
		m_oLogPos.sFunction = sFunction;
		m_oLogPos.nLevel = nLevel;
		m_oLogPos.nLine = nLine;
	}
}

void CLogManager::WriteLog2(const char* sLogInfo, CVaList& args)
{
	if(m_bLocked)
	{
		CLogInfo oLogInfo;
		oLogInfo.sLogInfo[FOCP_LOG_MAX-1] = '\0';
		vsnprintf(oLogInfo.sLogInfo, FOCP_LOG_MAXMSG-1, sLogInfo, args.args);
		oLogInfo.sAppName = m_oLogPos.sAppName;
		oLogInfo.sModuleName = m_oLogPos.sModuleName;
		oLogInfo.sFileName = m_oLogPos.sFileName;
		oLogInfo.sFunction = m_oLogPos.sFunction;
		oLogInfo.nLevel = m_oLogPos.nLevel;
		oLogInfo.nLine = m_oLogPos.nLine;
		oLogInfo.sDate[0] = '\0';
		WriteLog(oLogInfo);
		m_bLocked = false;
	}
	m_oMutex.Leave();
}

bool CLogManager::HaveModule(const char* sModuleName)
{
	bool bHave = false;
	CModuleName oName(sModuleName);
	if(oName.Empty())
		return true;
	CRbTreeNode* pEnd = m_oLogAttr.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oLogAttr.Find(oName);
	if(pIt != pEnd)
		bHave = true;
	m_oMutex.Leave();
	return bHave;
}

void CLogManager::WalkLogModule(void *pPara, void(*OnWalk)(void*pPara, const char* sModule))
{
	CRbTreeNode* pEnd = m_oLogAttr.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oLogAttr.First();
	for(; pIt!=pEnd; pIt=m_oLogAttr.GetNext(pIt))
	{
		const CModuleName& oName = m_oLogAttr.GetKey(pIt);
		OnWalk(pPara, oName.sName);
	}
	m_oMutex.Leave();
}

CAfcLogAttr& CLogManager::GetLogAttr(const char* sModuleName)
{
	CModuleName oName(sModuleName);
	CRbTreeNode* pEnd = m_oLogAttr.End();
	CRbTreeNode* pIt = m_oLogAttr.Find(oName);
	if(pIt != pEnd)
		return m_oLogAttr.GetItem(pIt);

	CAfcLogAttr& oAttr = m_oLogAttr[oName];
	if(oName.Empty())
	{
		const char* s = GetEnvVar("FocpLogLevel");
		if(!s)
			oAttr.nLevel = FOCP_LOG_ERROR;
		else
		{
			oAttr.nLevel = atoi(s);
			if(oAttr.nLevel < FOCP_LOG_ERROR)
				oAttr.nLevel = FOCP_LOG_ERROR;
			else if(oAttr.nLevel > FOCP_LOG_DEBUG)
				oAttr.nLevel = FOCP_LOG_DEBUG;
		}
		s = GetEnvVar("FocpLogMode");
		if(!s)
			oAttr.nMode = FOCP_LOG_SCREEN;
		else
		{
			uint32 nMode = atoi(s);
			oAttr.nMode = 0;
			if(nMode & FOCP_LOG_SCREEN)
				oAttr.nMode |= FOCP_LOG_SCREEN;
			if(nMode & FOCP_LOG_FILE)
				oAttr.nMode |= FOCP_LOG_FILE;
			if(nMode & FOCP_LOG_SERVER)
				oAttr.nMode |= FOCP_LOG_SERVER;
		}
		s = GetEnvVar("FocpLogFilter");
		if(!s)
			oAttr.nFilter = 14;
		else
		{
			uint32 nFilter = atoi(s);
			if(!nFilter)
				nFilter = 14;
			oAttr.nFilter = 0;
			if(nFilter & FOCP_LOG_HOST)
				oAttr.nFilter |= FOCP_LOG_HOST;
			if(nFilter & FOCP_LOG_DATE)
				oAttr.nFilter |= FOCP_LOG_DATE;
			if(nFilter & FOCP_LOG_APPN)
				oAttr.nFilter |= FOCP_LOG_APPN;
			if(nFilter & FOCP_LOG_MODU)
				oAttr.nFilter |= FOCP_LOG_MODU;
			if(nFilter & FOCP_LOG_SRCF)
				oAttr.nFilter |= FOCP_LOG_SRCF;
			if(nFilter & FOCP_LOG_FUNC)
				oAttr.nFilter |= FOCP_LOG_FUNC;
		}
	}
	else
		oAttr = GetLogAttr("");
	return oAttr;
}

void CLogManager::GetLogDate(char sDate[20])
{
	struct tm *tloc;
	time_t t = time(NULL);
#ifdef UNIX
	struct tm tt;
	localtime_r(&t, &tt);
	tloc = &tt;
#endif
#ifdef WINDOWS
	tloc = localtime((const time_t*)&t);
#endif
	sprintf(sDate, "%04d-%02d-%02d %02d:%02d:%02d",
			1900+tloc->tm_year,tloc->tm_mon+1,tloc->tm_mday,
			tloc->tm_hour,tloc->tm_min,tloc->tm_sec);
}

void CLogManager::GetFileDate(char sDate[20])
{
	struct tm *tloc;
	time_t t = time(NULL);
#ifdef UNIX
	struct tm tt;
	localtime_r(&t, &tt);
	tloc = &tt;
#endif
#ifdef WINDOWS
	tloc = localtime((const time_t*)&t);
#endif
	sprintf(sDate, "%04d%02d%02d%02d%02d%02d",
			1900+tloc->tm_year,tloc->tm_mon+1,tloc->tm_mday,
			tloc->tm_hour,tloc->tm_min,tloc->tm_sec);
}

uint32 CLogManager::CreateLogMsg(CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG])
{
	char* pShift = sLogInfo;
	char* pEnd = pShift+FOCP_LOG_MAXMSG;

	if(oLogInfo.sDate[0] == '\0')
		GetLogDate(oLogInfo.sDate);

	//sHost
	pShift += snprintf(pShift, pEnd-pShift, "%s", m_sHostIp);
	if(pEnd-pShift)++pShift;

	//sDate
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sDate);
	if(pEnd-pShift)++pShift;

	//nLevel;
	pShift += snprintf(pShift, pEnd-pShift, "%u", oLogInfo.nLevel);
	if(pEnd-pShift)++pShift;

	//sAppName
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sAppName);
	if(pEnd-pShift)++pShift;

	//Dmn
	pShift += snprintf(pShift, pEnd-pShift, "%u", m_nDmn);
	if(pEnd-pShift)++pShift;

	//Ain
	pShift += snprintf(pShift, pEnd-pShift, "%u", m_nAin);
	if(pEnd-pShift)++pShift;

	//sModuleName
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sModuleName);
	if(pEnd-pShift)++pShift;

	//sFunction
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sFunction);
	if(pEnd-pShift)++pShift;

	//sFileName
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sFileName);
	if(pEnd-pShift)++pShift;

	//nLine
	pShift += snprintf(pShift, pEnd-pShift, "%u", oLogInfo.nLine);
	if(pEnd-pShift)++pShift;

	//sLogInfo
	pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sLogInfo);
	if(pEnd-pShift)++pShift;

	return FOCP_LOG_MAXMSG - (pEnd-pShift);
}

uint32 CLogManager::GetLogInfo(uint32 nFilter, CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG])
{
	char* pShift = sLogInfo, * pEnd = pShift+FOCP_LOG_MAXMSG-1;

	if(oLogInfo.sDate[0] == '\0')
		GetLogDate(oLogInfo.sDate);

	pShift[FOCP_LOG_MAXMSG-1] = '\0';
	if(nFilter & FOCP_LOG_HOST)
		pShift += snprintf(pShift, pEnd-pShift, "%s ", m_sHostIp);
	if(nFilter & FOCP_LOG_DATE)
		pShift += snprintf(pShift, pEnd-pShift, "%s ", oLogInfo.sDate);
	pShift += snprintf(pShift, pEnd-pShift, "[%s] ", GetLogLevelName(oLogInfo.nLevel));
	if( (nFilter & FOCP_LOG_APPN) || (nFilter & FOCP_LOG_MODU) )
	{
		pShift += snprintf(pShift, pEnd-pShift, "[");
		if(nFilter & FOCP_LOG_APPN)
		{
			pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sAppName);
			if(nFilter & FOCP_LOG_MODU)
				pShift += snprintf(pShift, pEnd-pShift, ":");
		}
		if(nFilter & FOCP_LOG_MODU)
			pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sModuleName);
		pShift += snprintf(pShift, pEnd-pShift, "] ");
	}
	if( (nFilter & FOCP_LOG_SRCF) || (nFilter & FOCP_LOG_FUNC) )
	{
		pShift += snprintf(pShift, pEnd-pShift, "[");
		if(nFilter & FOCP_LOG_FUNC)
		{
			pShift += snprintf(pShift, pEnd-pShift, "%s", oLogInfo.sFunction);
			if(nFilter & FOCP_LOG_SRCF)
				pShift += snprintf(pShift, pEnd-pShift, "@");
		}
		if(nFilter & FOCP_LOG_SRCF)
			pShift += snprintf(pShift, pEnd-pShift, "%s:%u", oLogInfo.sFileName, oLogInfo.nLine);
		pShift += snprintf(pShift, pEnd-pShift, "] ->\n  ");
	}
	else
		pShift += snprintf(pShift, pEnd-pShift, "-> ");
	char* pShift2 = oLogInfo.sLogInfo;
	while(true)
	{
		char* pNewLine = strchr(pShift2, '\n');
		if(pNewLine)
		{
			pNewLine[0] = '\0';
			if(*(pNewLine-1) == '\r')
				*(pNewLine-1) = '\0';
		}
		pShift += snprintf(pShift, pEnd-pShift, "%s\n", pShift2);
		if(!pNewLine)
			break;
		pShift2 = pNewLine + 1;
		if(!pShift2[0])
			break;
		pShift += snprintf(pShift, pEnd-pShift, "  ");
	}
	return FOCP_LOG_MAXMSG - (pEnd-pShift) - 1;
}

void CLogManager::CreateLogFile()
{
#ifdef WINDOWS
	sprintf(m_sFileName, "%s\\log\\%s_%03d.log", m_sHome, m_sName, m_nFileId);
#elif defined(CYGWIN_NT)
	if(RunInWindows())
		sprintf(m_sFileName, "%s\\log\\%s_%03d.log", m_sHome, m_sName, m_nFileId);
	else
		sprintf(m_sFileName, "%s/log/%s_%03d.log", m_sHome, m_sName, m_nFileId);
#elif defined(UNIX)
	sprintf(m_sFileName, "%s/log/%s_%03d.log", m_sHome, m_sName, m_nFileId);
#else
#error "Not support the OS";
#endif
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	m_pFile = fopen(m_sFileName, "w");
}

bool CLogManager::NotAccess()
{
	return (!m_pFile) || access(m_sFileName, F_OK) || (m_nLogFileSize < (uint32)ftell(m_pFile));
}

void CLogManager::PrintScreen(uint32 nLevel, char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen)
{
	if(nLevel == FOCP_LOG_ERROR)
		write(2, sLogInfo, nLen);
	else
		write(1, sLogInfo, nLen);
}

void CLogManager::PrintFile(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen)
{
	if(NotAccess())
	{
		++m_nFileId;
		if(m_nFileId > m_nLogFileNo)
			m_nFileId = 1;
		CreateLogFile();
	}
	if(m_pFile)
		fprintf(m_pFile, "%s", sLogInfo);
}

void CLogManager::PrintServer(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen)
{
	sendto(m_nSocket, (const char*)sLogInfo, nLen, 0, (struct sockaddr*)(void*)&m_oServerAddr, sizeof(m_oServerAddr));
}

void CLogManager::WriteLogV2(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr)
{
	CLogInfo oLogInfo;
	oLogInfo.sLogInfo[FOCP_LOG_MAX-1] = '\0';
	vsnprintf(oLogInfo.sLogInfo, FOCP_LOG_MAXMSG-1, sLogInfo, argptr.args);
	oLogInfo.sAppName = m_sApp;
	oLogInfo.sModuleName = sModuleName;
	oLogInfo.sFileName = sFileName;
	oLogInfo.sFunction = sFunction;
	oLogInfo.nLevel = nLevel;
	oLogInfo.nLine = nLine;
	oLogInfo.sDate[0] = '\0';
	m_oMutex.Enter();
	WriteLog(oLogInfo);
	m_oMutex.Leave();
}

void CLogManager::WriteLog(CLogInfo &oLogInfo, bool bSupportNet)
{
	if(m_bHold)
	{
		GetLogDate(oLogInfo.sDate);
		m_oLogBuffer.Push(oLogInfo);
	}
	else
	{
		CAfcLogAttr& oLogAttr = GetLogAttr(oLogInfo.sModuleName);
		if( (oLogAttr.nMode & FOCP_LOG_SCREEN) || (oLogAttr.nMode & FOCP_LOG_FILE) )
		{
			char sLogInfo[FOCP_LOG_MAXMSG];
			uint32 nLen = GetLogInfo(oLogAttr.nFilter, oLogInfo, sLogInfo);
			if(oLogAttr.nMode & FOCP_LOG_SCREEN)
				PrintScreen(oLogInfo.nLevel, sLogInfo, nLen);
			if(oLogAttr.nMode & FOCP_LOG_FILE)
				PrintFile(sLogInfo, nLen);
		}
		if(bSupportNet && (oLogAttr.nMode & FOCP_LOG_SERVER))
		{
			char sLogInfo[FOCP_LOG_MAXMSG];
			uint32 nLen = CreateLogMsg(oLogInfo, sLogInfo);
			PrintServer(sLogInfo, nLen);
		}
	}
}

void CLogManager::Lock()
{
	m_oMutex.Enter();
}

void CLogManager::UnLock()
{
	m_oMutex.Leave();
}

FOCP_END();
