
#if defined(MSVC)

#include "AfcThread.hpp"

#if defined(_DEBUG)

#include "FuncNameDetail.hpp"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

FOCP_BEGIN();

namespace
{
template<typename TData> class CThreadVariable
{
private:
	FOCP_DETAIL_NAME::CThreadMutex m_oMutex;
	CRbMap<DWORD, TData> m_oTable;

	CThreadVariable(const CThreadVariable<TData> &oSrc);
	CThreadVariable(const TData &oSrc);

public:
	CThreadVariable()
	{
		GetVariable();
	}

	virtual ~CThreadVariable()
	{
	}

	virtual void Clear()
	{
		m_oTable.Clear();
	}

	CThreadVariable<TData>& operator=(const CThreadVariable<TData> &oSrc)
	{
		if(this != &oSrc)
			GetVariable() = oSrc.GetVariable();
		return *this;
	}

	CThreadVariable<TData>& operator=(const TData &oSrc)
	{
		GetVariable() = oSrc;
		return *this;
	}

	operator TData&()
	{
		return GetVariable();
	}

private:
	TData& GetVariable()
	{
		DWORD nTid = ::GetCurrentThreadId();
		m_oMutex.Enter();
		TData & dat = m_oTable[nTid];
		m_oMutex.Leave();
		return dat;
	}
};

struct CFunctionName
{
	char sName[1024];
};

CThreadVariable<CFunctionName> g_oFunctionName;
};

static FOCP_DETAIL_NAME::CString& GetAppHome(FOCP_DETAIL_NAME::CString &oHome)
{
	char m_sHome[FOCP_MAX_PATH];
	GetProgramFileName(m_sHome);
	uint32 nLen = strlen(m_sHome);
	char* pEnd = m_sHome + nLen - 1;
	while(pEnd[0] != '/' && pEnd[0] != '\\')
		--pEnd;
	pEnd[0] = '\0';
	while(pEnd != m_sHome && pEnd[0] != '/' && pEnd[0] != '\\')
		--pEnd;
	if(pEnd != m_sHome)
		pEnd[0] = '\0';
	oHome = m_sHome;
	return oHome;
}

static const char* GetAllSymPath(FOCP_DETAIL_NAME::CString &oHome, FOCP_DETAIL_NAME::CString &oOut)
{
	const char* sSubDir;
	bool bIsDirectory;
	char* sPath = (char*)oHome.GetStr();
	uint32 nLen = strlen(sPath);
	char* pEnd = sPath + nLen;
	void* pDir = OpenDirectory(sPath);

	if(oOut.GetStr()[0])
		oOut += ";";
	oOut += sPath;

	while((sSubDir = ReadDirectory(pDir, bIsDirectory)))
	{
		if(!bIsDirectory)
			continue;
		if(!strcmp(sSubDir, "..") || !strcmp(sSubDir, "."))
			continue;
		oHome += "/";
		oHome += sSubDir;
		GetAllSymPath(oHome, oOut);
		sPath = (char*)oHome.GetStr();
		pEnd = sPath + nLen;
		pEnd[0] = '\0';
	}
	CloseDirectory(pDir);
	return oOut.GetStr();
}

class CMyStackWalker: public StackWalker
{
private:
	FOCP_DETAIL_NAME::CThreadMutex m_oMutex;
	char* m_sName;

public:
	CMyStackWalker(FOCP_DETAIL_NAME::CString oHome=FOCP_DETAIL_NAME::CString(), FOCP_DETAIL_NAME::CString oStr=FOCP_DETAIL_NAME::CString())
		:StackWalker(OptionsAll, GetAllSymPath(GetAppHome(oHome), oStr))
	{
	}

	virtual void OnOutput(LPCSTR szText)
	{
		CFunctionName& oFuncName = g_oFunctionName;
		m_sName = oFuncName.sName;
		strcpy(m_sName, szText);
	}

	const char* GetFuncName()
	{
		m_oMutex.Enter();
		m_sName = NULL;
		GetFuncionName();
		char* sRet = m_sName;
		m_oMutex.Leave();
		if(sRet)
		{
			if(!memcmp(sRet, "`anonymous namespace\'", (sizeof("`anonymous namespace\'"))-1))
			{
				sRet += (sizeof("`anonymous namespace\'"))-4;
				sRet[0] = '\'';
				sRet[1] = '?';
				sRet[2] = '\'';
			}
			return sRet;
		}
		return "";
	}

};

FOCP_END();

namespace
{
FOCP_NAME::CMyStackWalker g_oStackWalker;
};

extern "C" AFCBASE_API const char* GetFuncName()
{
	return g_oStackWalker.GetFuncName();
}

#else
extern "C" AFCBASE_API const char* GetFuncName()
{
	return "";
}
#endif

#endif //_DEBUG && MSVC
