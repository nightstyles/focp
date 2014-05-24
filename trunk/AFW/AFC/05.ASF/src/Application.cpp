
#include "Application.hpp"
#include "Argument.hpp"

#ifdef WINDOWS
#include <io.h>
#include <sys/locking.h>
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <sys/file.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

FOCP_BEGIN();

#ifndef FOCP_FREE_LICENSE
static void GetRandomChar(char &c, char d)
{
	uint32 x;
loop:
	x = (uint32)Random();
	x %= 26;
	c = x + 'A';
	if(c == d)
		goto loop;
}

static void GetRandomString(CString &oStr, uint32 nLen, const char* sNot)
{
	char c;
	for(uint32 i=0; i<nLen; ++i)
	{
		GetRandomChar(c, *sNot);
		oStr += c;
		if(*sNot)
			++sNot;
	}
}

static void CreateHostInfo(CString &oHostInfo)
{
	uint64 nHostId = CFile::GetHostId();
	CFormatString oHostId;
	oHostId.Print("%u64", nHostId);

	uint32 nPos = (uint32)Random();
	nPos %= 5;
	nPos += 3;

	uint32 nLen = (uint32)Random();
	nLen %= 80;
	nLen += 20;

	uint32 nSize = (uint32)Random();
	nSize %= 20;
	nSize += 10;

	for(uint32 i=0; i<nSize; ++i)
	{
		CString oLine;
		GetRandomString(oLine, nLen, "");

		if(i)
			oHostInfo += " ";

		if(i == nPos)
		{
			oHostInfo += oHostId;
			oHostInfo += " ";
			oHostInfo += CFilePathInfo::GetInstance()->GetName();
			oHostInfo += " ";
		}

		oHostInfo += oLine;
	}
}

static void GetHostInfo(CString &oHostInfo, bool bAuth)
{
	CFile oFile;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	CFormatString oFileName;
	if(bAuth)
		oFileName.Print("disk://%s/%s.auth", pHome->GetDir(), pHome->GetName());
	else
		oFileName.Print("disk://%s/%s.host", pHome->GetDir(), pHome->GetName());
	int32 nRet = oFile.Open(oFileName.GetStr(), "r");
	if(!nRet)
	{
		CString oLine;
		CFileFormatter oFormat(&oFile);
		while(oFormat.Scan("%r", &oLine) == 1)
			oHostInfo += oLine;
	}
}

static bool SetHostInfo(CVector<CString> &oHostInfo)
{
	CFile oFile;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	CFormatString oFileName;
	oFileName.Print("disk://%s/%s.host", pHome->GetDir(), pHome->GetName());
	if(oFile.Open(oFileName.GetStr(), "wcd"))
		return false;
	CFileFormatter oFormat(&oFile);
	uint32 nSize = oHostInfo.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		oFormat.Print("%s\n", oHostInfo[i].GetStr());
	return true;
}

static bool DecodeHostInfo(const char* sHostInfo, CVector<CString> &oInfo)
{
	uint32 nLen = CString::StringLength(sHostInfo);
	char* sInfo = Base64Decode(sHostInfo, nLen, &nLen);
	if(!sInfo)
		return false;
	for(uint32 i=0; i<nLen; ++i)
		sInfo[i] = (char)(((uint8)sInfo[i]) ^ ((uint8)'J'));//Jacky's First Char
	CString oLine;
	CFormatString oFormat(sInfo);
	while(oFormat.Scan("%s", &oLine) == 1)
		oInfo.Insert((uint32)(-1), oLine);
	CMalloc::Free(sInfo);
	return true;
}

static void EncodeHostInfo(CString &oHostInfo, CVector<CString> &oOut)
{
	char* p = (char*)oHostInfo.GetStr();
	uint32 nLen = oHostInfo.GetSize();
	for(uint32 i=0; i<nLen; ++i)
		p[i] = (char)(((uint8)p[i]) ^ ((uint8)'J'));//Jacky's First Char

	char* s = Base64Encode((const char*)p,  nLen, &nLen);
	CString oText(s);
	CMalloc::Free(s);

	s = (char*)oText.GetStr();
	nLen = 0;
	CString oLine;
	while(*s)
	{
		if(nLen>=80)
		{
			oOut.Insert((uint32)(-1), oLine);
			oLine = "";
			nLen = 0;
		}
		oLine += *s;
		++s;
		++nLen;
	}
	if(!oLine.Empty())
		oOut.Insert((uint32)(-1), oLine);
}

static bool CheckHostInfo(CVector<CString> &oInfo)
{
	uint64 nHostId = CFile::GetHostId();
	CFormatString oFormat;
	oFormat.Print("%u64", nHostId);
	bool bRet = false;
	uint32 i, nSize = oInfo.GetSize();
	for(i=0; i<nSize; ++i)
	{
		CString &oLine = oInfo[i];
		if(!oLine.Compare(oFormat))
		{
			++i;
			bRet = true;
			break;
		}
	}
	if(bRet)
	{
		bRet = false;
		const char* sName = CFilePathInfo::GetInstance()->GetName();
		if(i < nSize && !oInfo[i].Compare(sName))
			bRet = true;
	}
	return bRet;
}

static bool CompareHostInfo(CVector<CString> &oInfo1, CVector<CString> &oInfo2)
{
	uint32 nSize = oInfo1.GetSize();
	if(nSize != oInfo2.GetSize())
		return false;
	bool bRet = true;
	for(uint32 i=0; i<nSize; ++i)
	{
		bool bFind = false;
		CString &oLine1 = oInfo1[i];
		for(uint32 j=0; j<nSize; ++j)
		{
			CString &oLine2 = oInfo2[j];
			if(!oLine2.Compare(oLine1))
			{
				bFind = true;
				break;
			}
		}
		if(!bFind)
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}

static void ProcessError1()
{
	CFormatString oFileName;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	oFileName.Print("%s/%s.host", pHome->GetDir(), pHome->GetName());
	Print("Authenticate failure, please you remove the file '%s'\n", oFileName.GetStr());
}

static void ProcessError2()
{
	CFormatString oFileName;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	oFileName.Print("%s/%s.auth", pHome->GetDir(), pHome->GetName());
	Print("Invalid authentication file '%s'\n", oFileName.GetStr());
}

static void ProcessError3()
{
	CFormatString oFileName;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	oFileName.Print("%s/%s.host", pHome->GetDir(), pHome->GetName());
	Print("Create authentication file '%s' failure\n", oFileName.GetStr());
}

static void ProcessError4()
{
	CFormatString oFileName;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	oFileName.Print("%s/%s.host", pHome->GetDir(), pHome->GetName());
	Print("Send authentication file '%s' to vncore.cao@139.com\n", oFileName.GetStr());
	Print("You will receive the file '%s.auth', please you put it into the directory '%s'\n", pHome->GetName(), pHome->GetDir());
}

static void ProcessError5()
{
	CFormatString oFileName;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	oFileName.Print("%s/%s.auth", pHome->GetDir(), pHome->GetName());
	Print("Authenticate failure, invalid authentication file '%s'\n", oFileName.GetStr());
}

static bool CheckLicense()
{
	CString oHostInfo, oAuthInfo;
	CVector<CString> oDecodeHost, oDecodeAuth;

	RandomSeed(CTimer::GetTime());

	GetHostInfo(oHostInfo, false);
	if(oHostInfo.Empty())
	{
		CreateHostInfo(oHostInfo);
		EncodeHostInfo(oHostInfo, oDecodeHost);
		if(SetHostInfo(oDecodeHost))
			ProcessError4();
		else
			ProcessError3();
		goto end;
	}
	if(!DecodeHostInfo(oHostInfo.GetStr(), oDecodeHost) || !CheckHostInfo(oDecodeHost))
	{
		ProcessError1();
		goto end;
	}
	GetHostInfo(oAuthInfo, true);
	if(oAuthInfo.Empty() || !DecodeHostInfo(oAuthInfo.GetStr(), oDecodeAuth) || !CheckHostInfo(oDecodeAuth))
	{
		ProcessError2();
		goto end;
	}
	if(!CompareHostInfo(oDecodeHost, oDecodeAuth))
	{
		ProcessError5();
		goto end;
	}
	return true;
end:
	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
	return false;
}

#endif

ASF_API bool StartFocpService(bool bCheckLicense)
{
	if(bCheckLicense && !CheckLicense())
		return false;
	CServiceManager* pServiceManager = CServiceManager::GetInstance();
	if(!pServiceManager->Initialize() || !pServiceManager->Start())
	{
		FocpLog(FOCP_LOG_ERROR, ("Focp startup failure for '%s'", CFilePathInfo::GetInstance()->GetName()));
		return false;
	}
	return true;
}

ASF_API void StopFocpService()
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();
	pServiceManager->Stop();
	pServiceManager->Cleanup();

	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();

	FocpLog(FOCP_LOG_SYSLOG, ("Focp is exited from '%s'", CFilePathInfo::GetInstance()->GetName()));

	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

typedef CApplication* (* FGetApplicationInstance)();
struct CAbsApplication
{
	CApplication* pInstance;
	CAbsApplication()
	{
		FGetApplicationInstance GetApplicationInstance = (FGetApplicationInstance)CMainModule::GetInstance()->FindSymbol("GetApplicationInstance");
		pInstance = GetApplicationInstance?GetApplicationInstance():NULL;
		if(pInstance == NULL)
			pInstance = CSingleInstance<CApplication>::GetInstance();
	}
};

CApplication::CApplication()
{
	m_bStoped = true;
	m_bWarm = true;
	m_bDisableSignal = false;
	m_bBlock = true;
#ifdef WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup(wVersionRequested, &wsaData);
#endif
}

CApplication::~CApplication()
{
#ifdef WINDOWS
	WSACleanup();
#endif
}

CApplication* CApplication::GetInstance()
{
	return CSingleInstance<CAbsApplication>::GetInstance()->pInstance;
}

void CApplication::DisableSignaler()
{
	m_bDisableSignal = true;
}

void CApplication::MakeAsynRun()
{
	m_bBlock = false;
}

const char* CApplication::GetAppName()
{
	return CFilePathInfo::GetInstance()->GetName();
}

void CApplication::RunEnd()
{
	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();

	FocpLog(FOCP_LOG_SYSLOG, ("%s is exited", CFilePathInfo::GetInstance()->GetName()));

	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

void CApplication::WaitRun()
{
	while(!m_bStoped)
		CCooperator::Sleep(50);

	FocpLog(FOCP_LOG_SYSLOG, ("%s is exiting", CFilePathInfo::GetInstance()->GetName()));
	if(m_bWarm)
	{
		OnRunEnd();
		RunEnd();
	}
}

int32 CApplication::Run(int32 argc, char*argv[], bool bCheckLicense)
{
	char cOpt;
	char* sArg;
	int32 nIdx = 0;

	if(bCheckLicense && !CheckLicense())
		return -1;

#ifdef WINDOWS
	bool bBackGround = false;
	if(!CString::StringCompare(argv[argc-1], "&"))
	{
		--argc;
		bBackGround = true;
	}
#endif

	if(m_bDisableSignal == false)
	{
		SetSignals();
		Commit();
	}

	const char* sAppName = CFilePathInfo::GetInstance()->GetName();

	CArguments* pArguments = CArguments::GetInstance();
	pArguments->SetArgv(argc, argv);

	while((cOpt=pArguments->WalkOption(nIdx, sArg)))
	{
		if(cOpt == 'v')
		{
			CFocpVersion::PrintVersionInfo();
			RunEnd();
			return 0;
		}
		else if(!ProcessOption(cOpt, sArg))
		{
			FocpLog(FOCP_LOG_ERROR, ("%s startup failure", sAppName));
			RunEnd();
			return -1;
		}
	}

	FocpLog(FOCP_LOG_SYSLOG, ("Process command line: %s", pArguments->GetCmdLine()));

	if(OnRun())
	{
		RunEnd();
		return -1;
	}

	m_bStoped = false;

	FocpLog(FOCP_LOG_SYSLOG, ("%s is started\n", sAppName));

#ifdef WINDOWS
	if(bBackGround)
	{
		Print("%s is going back ground\n", sAppName);
		CCooperator::Sleep(1000);
		FreeConsole();
	}
#endif

	if(m_bBlock)
		WaitRun();

	return 0;
}

int32 CApplication::OnRun()
{
	return 0;
}

void CApplication::OnRunEnd()
{
}

void CApplication::StopNotice(bool bWarm)
{
	m_bWarm = bWarm;
	m_bStoped = true;
}

void CApplication::OnDefaultSignal(int32 nSignal)
{
	switch(nSignal)
	{
	case FOCP_SIGINT:
	case FOCP_SIGQUIT:
	case FOCP_SIGTERM:
		StopNotice();
		break;
	}
}

bool CApplication::ProcessOption(char cOpt, const char* sArg)
{
	return true;
}

CServiceApplication::CServiceApplication()
{
}

CServiceApplication::~CServiceApplication()
{
}

int32 CServiceApplication::OnRun()
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();

	if(!StartServiceBefore())
	{
		FocpLog(FOCP_LOG_ERROR, ("%s startup failure", GetAppName()));
		return -1;
	}

	if(!pServiceManager->Initialize() || !pServiceManager->Start())
	{
		StopServiceBefore();
		pServiceManager->Stop();
		StopServiceAfter();
		pServiceManager->Cleanup();
		FocpLog(FOCP_LOG_ERROR, ("%s startup failure", GetAppName()));
		return -1;
	}

	if(!StartServiceAfter())
	{
		StopServiceBefore();
		pServiceManager->Stop();
		StopServiceAfter();
		pServiceManager->Cleanup();
		FocpLog(FOCP_LOG_ERROR, ("%s startup failure", GetAppName()));
		return -1;
	}

	return 0;
}

void CServiceApplication::OnRunEnd()
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();
	StopServiceBefore();
	pServiceManager->Stop();
	StopServiceAfter();
	pServiceManager->Cleanup();
}

bool CServiceApplication::StartServiceBefore()
{
	return true;
}

bool CServiceApplication::StartServiceAfter()
{
	return true;
}

void CServiceApplication::StopServiceBefore()
{
}

void CServiceApplication::StopServiceAfter()
{
}

FOCP_END();
