
#include "AFC.hpp"

FOCP_BEGIN();

void Compile(const char* sProtocolName)
{
	if(sProtocolName == NULL)
		PrintError("\t%s protocol <CR>\n", CFilePathInfo::GetInstance()->GetName());
	else
	{
		uint32 nWarning;
		char sHppFile[256];
		char sCppFile[256];
		char sMsgFile[256];

		StringPrint(sHppFile, "%s.hpp", sProtocolName);
		StringPrint(sCppFile, "%s.cpp", sProtocolName);
		StringPrint(sMsgFile, "%s.syn", sProtocolName);

		CFile oLangFile("mLang.syn", "r");
		CFile oMsgFile(sMsgFile, "r");
		CFile oHppFile(sHppFile, "wcd");
		CFile oCppFile(sCppFile, "wcd");

		if(!CompileMsg(GetStdErr(), oLangFile, oMsgFile, nWarning))
			CreateMsgCode(oHppFile, oCppFile, sProtocolName);
	}

	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

FOCP_END();

int main(int argc, char* argv[])
{
	char* sProName = NULL;
	if(argc > 1)
		sProName = argv[1];
	FOCP_NAME::Compile(sProName);
	return 0;
}
