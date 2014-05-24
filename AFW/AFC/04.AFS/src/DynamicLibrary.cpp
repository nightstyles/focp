
#include "DynamicLibrary.hpp"
#include "FileSystem.hpp"

#if defined(WINDOWS)// || defined(CYGWIN_NT)
#include <windows.h>
#elif defined(UNIX)
#include <dlfcn.h>
#else
#error CDynamicLibrary only support windows & unix
#endif

//这里不能使用CFile，因为文件驱动的加载需要使用AutoLoad，会形成死递归调用
#include <stdio.h>

FOCP_BEGIN();

class CShareLibrary
{
private:
	void* m_pLib;
	CString m_oFileName;
	uint32 m_nCounter;
	bool m_bNeedFree;

public:
	CShareLibrary()
	{
		m_pLib = NULL;
		m_nCounter = 1;
		m_bNeedFree = false;
	}

	~CShareLibrary()
	{
	}

	bool Load(const char* sLibName, bool bTry=false)
	{
		m_bNeedFree = true;
		if(sLibName == NULL || !sLibName[0])
		{
	#if defined(WINDOWS)// || defined(CYGWIN_NT)
			m_bNeedFree = false;
			m_pLib = GetModuleHandle(NULL);
	#else
			m_pLib = ::dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
	#endif
			return true;
		}
		CPathDetailInfo oDetailInfo;
		CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();
		m_oFileName = sLibName;
		char sLibName2[FOCP_MAX_PATH];
		CString::StringCopy(sLibName2, sLibName);
		if(!pFileSystem->GetOsPathName(sLibName2, &oDetailInfo))
		{
			FocpLog(bTry?FOCP_LOG_WARNING:FOCP_LOG_ERROR, ("Load(%s) failure", sLibName));
			return false;
		}
		if(oDetailInfo.bExist == false || oDetailInfo.sFilePart == NULL)
		{
			FocpLog(bTry?FOCP_LOG_WARNING:FOCP_LOG_ERROR, ("Load(%s) failure", sLibName));
			return false;
		}
#ifdef WINDOWS
		m_pLib = ::LoadLibrary(sLibName2);
#else
		m_pLib = ::dlopen(sLibName2, RTLD_NOW|RTLD_GLOBAL);
#endif
		if(m_pLib == NULL)
		{
			FocpLog(bTry?FOCP_LOG_WARNING:FOCP_LOG_ERROR, ("Load(%s) failure", sLibName));
			return false;
		}

		FocpLog(FOCP_LOG_SYSLOG, ("Load(%s) success", sLibName));
		return true;
	}

	void UnLoad()
	{
		if(m_pLib && m_bNeedFree)
		{
	#if defined(WINDOWS)// || defined(CYGWIN_NT)
			::FreeLibrary((HMODULE)m_pLib);
	#else
			::dlclose(m_pLib);
	#endif
			if(!m_oFileName.Empty())
				FocpLog(FOCP_LOG_SYSLOG, ("UnLoad(%s) success", m_oFileName.GetStr()));
		}
		m_bNeedFree = true;
		m_pLib = NULL;
	}

	void* FindSymbol(const char* sSymbolName)
	{
		if(m_pLib == NULL)
			return NULL;
#if defined(WINDOWS)// || defined(CYGWIN_NT)
		return (void*)::GetProcAddress((HMODULE)m_pLib, sSymbolName);
#else
		return ::dlsym(m_pLib, sSymbolName);
#endif
	}

	const CString& GetFileName() const
	{
		return m_oFileName;
	}

	void AddRef()
	{
		++m_nCounter;
	}

	bool Release()
	{
		--m_nCounter;
		if(m_nCounter == 0)
		{
			UnLoad();
			CShareLibrary* pLib = this;
			delete pLib;
			return true;
		}
		return false;
	}
};

class CShareLibraryManager
{
private:
	struct CShareLibraryGetKey
	{
		static const CString* GetKey(const CShareLibrary* pLib)
		{
			return &pLib->GetFileName();
		}
	};
	CMutex m_oMutex;
	CRbTree<CString, CShareLibrary*, CShareLibraryGetKey> m_oLibs;

public:
	CShareLibraryManager()
	{
	}

	~CShareLibraryManager()
	{
	}

	static CShareLibraryManager* GetInstance()
	{
		return CSingleInstance<CShareLibraryManager>::GetInstance();
	}

	void UnLoad(CShareLibrary* pLib)
	{
		if(pLib)
		{
			CRbTreeNode* pEnd = m_oLibs.End();
			m_oMutex.Enter();
			CRbTreeNode* pIt = m_oLibs.Find(pLib->GetFileName());
			if( (pIt != pEnd) && (pLib == m_oLibs.GetItem(pIt)) && pLib->Release())
				m_oLibs.Remove(pIt);
			m_oMutex.Leave();
		}
	}

	CShareLibrary* Load(const char* sLibName, bool bTry=false)
	{
		char sLibName2[FOCP_MAX_PATH];

		if(sLibName == NULL || !sLibName[0])
			sLibName2[0] = '\0';
		else
		{
			CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();

			if(CString::CharOfString(sLibName, '/') || CString::CharOfString(sLibName, '\\'))
			{
				if(!pFileSystem->GetFullPath(sLibName, sLibName2))
				{
					FocpLog(bTry?FOCP_LOG_WARNING:FOCP_LOG_ERROR, ("Load(%s) failure", sLibName));
					return NULL;
				}
			}
			else
			{
				const char* sPrefixName,* sPostfixName;
#if defined(WINDOWS)// || defined(CYGWIN_NT)
				sPrefixName = NULL;
				sPostfixName = ".dll";
#elif defined(CYGWIN_NT)
				sPrefixName = "cyg";
				sPostfixName = ".dll";
#else
				sPrefixName = "lib";
				sPostfixName = ".so";
#endif
				if(!pFileSystem->SearchFile("FOCP_LIBRARY_PATH", sLibName, sLibName2, sPrefixName, sPostfixName))
				{
					FocpLog(bTry?FOCP_LOG_WARNING:FOCP_LOG_ERROR, ("Load(%s) failure", sLibName));
					return NULL;
				}
			}
		}

		CShareLibrary* pLib = NULL;
		CRbTreeNode* pEnd = m_oLibs.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oLibs.Find(sLibName2);
		if(pIt != pEnd)
		{
			pLib = m_oLibs.GetItem(pIt);
			pLib->AddRef();
		}
		else
		{
			pLib = new CShareLibrary();
			if(!pLib->Load(sLibName2, bTry))
			{
				delete pLib;
				pLib = NULL;
			}
			m_oLibs.Insert(pLib);
		}
		m_oMutex.Leave();
		return pLib;
	}
};

CDynamicLibrary::CDynamicLibrary()
{
	m_pLib = NULL;
}

CDynamicLibrary::CDynamicLibrary(const char* sLibName, bool bTry)
{
	m_pLib = NULL;
	Load(sLibName, bTry);
}

CDynamicLibrary::~CDynamicLibrary()
{
	UnLoad();
}

const char* CDynamicLibrary::GetFileName()
{
	if(m_pLib == NULL)
		return NULL;
	return ((CShareLibrary*)m_pLib)->GetFileName().GetStr();
}

bool CDynamicLibrary::Load(const char* sLibName, bool bTry)
{
	UnLoad();
	m_pLib = CShareLibraryManager::GetInstance()->Load(sLibName, bTry);
	return (m_pLib!=NULL);
}

void CDynamicLibrary::UnLoad()
{
	if(m_pLib)
	{
		CShareLibraryManager::GetInstance()->UnLoad((CShareLibrary*)m_pLib);
		m_pLib = NULL;
	}
}

void* CDynamicLibrary::FindSymbol(const char* sSymbolName)
{
	if(m_pLib == NULL)
		return NULL;
	return ((CShareLibrary*)m_pLib)->FindSymbol(sSymbolName);
}

bool CDynamicLibrary::Valid()
{
	return (m_pLib != NULL);
}

CMainModule::CMainModule():m_oMainLib(NULL)
{
}

CMainModule::~CMainModule()
{
}

void CMainModule::InitializeInstance()
{
	const char* sAppName = CFilePathInfo::GetInstance()->GetName();
#if defined(WINDOWS)// || defined(CYGWIN_NT)
	m_oAppLib.Load(sAppName, true);
#elif defined(CYGWIN_NT)
	CFormatString oAppLib;
	oAppLib.Print("cyg%s.dll", sAppName);
	m_oAppLib.Load(oAppLib.GetStr(), true);
#else
	CFormatString oAppLib;
	oAppLib.Print("lib%s.so", sAppName);
	m_oAppLib.Load(oAppLib.GetStr(), true);
#endif
}

CMainModule* CMainModule::GetInstance()
{
	return CSingleInstanceEx<CMainModule>::GetInstance();
}

void* CMainModule::FindSymbol(const char* sSymbolName)
{
	void* pRet = m_oAppLib.FindSymbol(sSymbolName);
	if(pRet == NULL)
		pRet = m_oMainLib.FindSymbol(sSymbolName);
	return pRet;
}

void CMainModule::UnLoad()
{
	m_oAppLib.UnLoad();
}

bool CMainModule::Valid()
{
	return m_oAppLib.Valid();
}

CAutoLibrary::CAutoLibrary(const char* sLibDir)
{
	char sLibFile[FOCP_MAX_PATH];
#if defined(WINDOWS)// || defined(CYGWIN_NT)
	const char * sPostfix = ".dll";
	const char * sPrefix = "";
#elif defined(CYGWIN_NT)
	const char * sPostfix = ".dll";
	const char * sPrefix = "cyg";
#else
	const char * sPostfix = ".so";
	const char * sPrefix = "lib";
#endif
	CDynamicLibrary* pLibrary = NULL;
	CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();
	void* pDirectory = pFileSystem->OpenDirectory(sLibDir);
	if(pDirectory == NULL)
	{
		FocpLog(FOCP_LOG_ERROR, ("OpenDirectory(%s) failure", sLibDir));
		return;
	}
	//这里不能使用CFile，因为文件驱动的加载需要使用该函数，会形成死递归调用
	FILE* fp;
	CString oLibraryList, oUnLoadList;
	CString::StringCopy(sLibFile, sLibDir);
	CString::StringCatenate(sLibFile, "/load.cfg");
	pFileSystem->GetOsPathName(sLibFile);
	fp = fopen(sLibFile, "rb");
	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		int32 nSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		oLibraryList.Append('A', nSize);
		fread((void*)oLibraryList.GetStr(), 1, nSize, fp);
		fclose(fp);
	}
	CString::StringCopy(sLibFile, sLibDir);
	CString::StringCatenate(sLibFile, "/unload.cfg");
	pFileSystem->GetOsPathName(sLibFile);
	fp = fopen(sLibFile, "rb");
	if(fp)
	{
		fseek(fp, 0, SEEK_END);
		int32 nSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		oUnLoadList.Append('A', nSize);
		fread((void*)oUnLoadList.GetStr(), 1, nSize, fp);
		fclose(fp);
	}

	bool bIsDirectory;
	const char* sFileName;
	uint32 nLen2 = CString::StringLength(sPostfix);
	uint32 nLen3 = CString::StringLength(sPrefix);
	if(!oLibraryList.Empty())
	{
		CString oFileName;
		CStringFormatter oFormatter(&oLibraryList);
		while(oFormatter.Scan("%r", &oFileName)==1)
		{
			oFileName.Trim();
			if(!oFileName.Empty())
			{
				if(!oUnLoadList.Empty())
				{
					uint32 nLen = oFileName.GetSize();
					const char* sHead = oUnLoadList.GetStr();
					char* sLib = CString::StringOfString(sHead, oFileName.GetStr(), false);
					if(sLib &&
							(sLib[nLen] == '\0' || CString::IsSpace(sLib[nLen])) &&
							(sHead == sLib || CString::IsSpace(*(sLib-1))))
						continue;
				}
				CString::StringCopy(sLibFile, sLibDir);
				CString::StringCatenate(sLibFile, "/");
				CString::StringCatenate(sLibFile, sPrefix);
				CString::StringCatenate(sLibFile, oFileName.GetStr());
				CString::StringCatenate(sLibFile, sPostfix);
				if(pLibrary == NULL)
					pLibrary = new CDynamicLibrary;
				if(pLibrary->Load(sLibFile))
				{
					m_oLibrary.Insert(m_oLibrary.GetSize(), pLibrary);
					pLibrary = NULL;
				}
			}
		}
	}
	else while( (sFileName = pFileSystem->ReadDirectory(pDirectory, bIsDirectory)) )
		{
			if(bIsDirectory)
				continue;
			uint32 nLen = CString::StringLength(sFileName);
			if(nLen <= nLen2+nLen3)
				continue;
			if(CString::StringCompare(sFileName+nLen-nLen2, sPostfix))
				continue;
			if(nLen3 && CString::StringCompare(sFileName, sPrefix, true, nLen3))
				continue;
			if(!oUnLoadList.Empty())
			{
				uint32 nLen4 = nLen-nLen2-nLen3;
				CString oSimpleFileName(sFileName+nLen3, nLen4);
				const char* sHead = oUnLoadList.GetStr();
				char* sLib = CString::StringOfString(sHead, oSimpleFileName.GetStr(), false);
				if(sLib && (sLib[nLen4] == '\0' ||
							CString::IsSpace(sLib[nLen4])) &&
						(sHead == sLib || CString::IsSpace(*(sLib-1))))
					continue;
			}
			CString::StringCopy(sLibFile, sLibDir);
			CString::StringCatenate(sLibFile, "/");
			CString::StringCatenate(sLibFile, sFileName);
			if(pLibrary == NULL)
				pLibrary = new CDynamicLibrary;
			if(pLibrary->Load(sLibFile))
			{
				m_oLibrary.Insert(m_oLibrary.GetSize(), pLibrary);
				pLibrary = NULL;
			}
		}
	pFileSystem->CloseDirectory(pDirectory);
	if(pLibrary)
		delete pLibrary;
}

CAutoLibrary::~CAutoLibrary()
{
	int32 nSize = (int32)m_oLibrary.GetSize();//实际不会有超过2G个库文件加载，所以用int32
	for(int32 i=nSize-1; i>=0; --i)
	{
		CDynamicLibrary* pLib = m_oLibrary[i];
		delete pLib;
	}
}

uint32 CAutoLibrary::GetSize()
{
	return m_oLibrary.GetSize();
}

CDynamicLibrary* CAutoLibrary::GetLibrary(uint32 nIdx)
{
	if(nIdx >= m_oLibrary.GetSize())
		return NULL;
	return m_oLibrary[nIdx];
}

CInterface::CInterface(CInterfaceManager* pManager)
{
	m_pManager = pManager;
	if(m_pManager)
		m_pManager->RegisterInterface(this);
}

CInterface::~CInterface()
{
	if(m_pManager)
		m_pManager->DeRegisterInterface(this);
}

const char* CInterface::GetInterfaceName()
{
	return NULL;
}

CInterfaceManager::CInterfaceManager(const char* sLibDir)
{
	m_sLibDir = sLibDir;
	m_pAutoLibrary = NULL;
}

CInterfaceManager::~CInterfaceManager()
{
	UnLoad();
}

void CInterfaceManager::Load()
{
	m_oMutex.Enter();
	if(m_pAutoLibrary == NULL)
	{
		uint32 i, nCount;
		CString oLibDir;
		const char* sAppHome = CFilePathInfo::GetInstance()->GetHome();
		oLibDir += sAppHome;
		oLibDir += "/lib/";
		oLibDir += m_sLibDir;
		m_pAutoLibrary = new CAutoLibrary(oLibDir.GetStr());
		nCount = m_oInterfaces.GetSize();
		for(i=nCount; i; --i)
		{
			CInterface* pInterface = m_oInterfaces[i-1];
			if(QueryInterface(pInterface->GetInterfaceName()) != pInterface)
			{
				m_oMutex.Leave();
				FocpAbort(( "re-register interface :%s",  pInterface->GetInterfaceName()));
			}
		}
	}
	m_oMutex.Leave();
}

void CInterfaceManager::UnLoad()
{
	m_oMutex.Enter();
	if(m_pAutoLibrary)
	{
		delete m_pAutoLibrary;
		m_pAutoLibrary = NULL;
	}
	m_oMutex.Leave();
}

uint32 CInterfaceManager::GetSize()
{
	return m_oInterfaces.GetSize();
}

CInterface* CInterfaceManager::GetInterface(uint32 nIdx)
{
	if(nIdx < m_oInterfaces.GetSize())
		return m_oInterfaces[nIdx];
	return NULL;
}

CInterface* CInterfaceManager::QueryInterface(const char* sInterfaceName)
{
	CInterface* pInterface = NULL;
	m_oMutex.Enter();
	uint32 i, nSize;
loop:
	nSize = m_oInterfaces.GetSize();
	for(i=0; i<nSize; ++i)
	{
		CInterface* pInterface2 = m_oInterfaces[i];
		if(!CString::StringCompare(sInterfaceName, pInterface2->GetInterfaceName(), false))
		{
			pInterface = pInterface2;
			break;
		}
	}
	if(pInterface == NULL && m_pAutoLibrary == NULL)
	{
		Load();
		goto loop;
	}
	m_oMutex.Leave();
	return pInterface;
}

void CInterfaceManager::RegisterInterface(CInterface* pInterface)
{
	//const char* sInterfaceName = pInterface->GetInterfaceName();
	m_oMutex.Enter();
	/*	if(sInterfaceName && QueryInterface(sInterfaceName))
		{
			FocpLog(FOCP_LOG_ERROR, ("Re-Register interface :%s", sInterfaceName));
			m_oMutex.Leave();
			throw "re-register interface";
		}
		else*/
	m_oInterfaces.Insert((uint32)(-1), pInterface);
	m_oMutex.Leave();
}

void CInterfaceManager::DeRegisterInterface(CInterface* pInterface)
{
	uint32 i, nCount;
	m_oMutex.Enter();
	nCount = m_oInterfaces.GetSize();
	for(i=0; i<nCount; ++i)
	{
		if(pInterface == m_oInterfaces[i])
		{
			m_oInterfaces.Remove(i);
			break;
		}
	}
	m_oMutex.Leave();
}

FOCP_END();
