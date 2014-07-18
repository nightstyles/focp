
#include "AFC.hpp"

#ifndef AFC_APP_CLASS
#define AFC_APP_CLASS CServiceApplication
#else
#include FOCP_STRING_DEFINE(FOCP_APP_NAME.hpp)
#endif

FOCP_C_BEGIN();
FOCP_EXPORT FOCP_NAME::CApplication* GetApplicationInstance()
{
	return FOCP_NAME::CSingleInstance<FOCP_NAME::AFC_APP_CLASS>::GetInstance();
}
FOCP_C_END();

FOCP_BEGIN();

static bool CheckAppName()
{
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	if(CString(FOCP_STRING_DEFINE(FOCP_APP_NAME)).Compare(pHome->GetName()))
	{
		CFormatString oFileName;
		oFileName.Print("%s/%s", pHome->GetDir(), pHome->GetName());
		CString oSuffix(pHome->GetPath()+oFileName.GetSize());
		oFileName.Clear();
		oFileName.Print("%s/%s%s", pHome->GetDir(), FOCP_STRING_DEFINE(FOCP_APP_NAME), oSuffix.GetStr());
		Print("Please you modify the file '%s' to '%s'.\n", pHome->GetPath(), oFileName.GetStr());
		CFileInterface::GetInterfaceManager()->UnLoad();
		CMainModule::GetInstance()->UnLoad();
		CThreadVariablePool::GetInstance()->ClearThreadVariable();
		return false;
	}
	return true;
}

FOCP_END();

int main(int argc, char* argv[])
{
	bool bCheck = true;
#ifdef FOCP_APPLICATION_TYPE_NUMBER
	FOCP_NAME::CServiceManager::GetInstance()->SetATN(FOCP_APPLICATION_TYPE_NUMBER);
#endif
#ifdef FOCP_FREE_LICENSE
	bCheck = false;
#else
	if(!FOCP_NAME::CheckAppName())
		return -1;
#endif
	return FOCP_NAME::CApplication::GetInstance()->Run(argc, argv, bCheck);
}

