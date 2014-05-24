
#include "AFC.hpp"

FOCP_BEGIN();

static void GetOdbcSourceInfo()
{
	CString oDsn, oUser, oPasswd;
	try
	{
loop1:
		Print("Please input the ODBC DSN (attention: cann't include the white-space char):\n");
		if(Scan("%s", &oDsn) != 1)
			goto loop1;
loop2:
		Print("Please input the ODBC USER (attention: cann't include the white-space char):\n");
		if(Scan("%s", &oUser) != 1)
			goto loop2;
loop3:
		Print("Please input the ODBC PASSWORD (attention: cann't include the white-space char):\n");
		if(Scan("%s", &oPasswd) != 1)
			goto loop3;
	}
	catch (...)
	{
		return;
	}
	CFormatString oInfo;
	oInfo.Print("%u64    %s    %s    %s", (uint64)CFile::GetHostId(), oDsn.GetStr(), oUser.GetStr(), oPasswd.GetStr());
	uint32 nLen = oInfo.GetSize();
	uint8* p = (uint8*)oInfo.GetStr();
	for(uint32 i=0; i<nLen; ++i)
		p[i] ^= (uint8)'J';//Jacky's First Char
	char* s = Base64Encode((const char*)p,  nLen, &nLen);
	Print("This ODBC data source's cryptograph is:\n%s\n", s);
	CMalloc::Free(s);

	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

FOCP_END();

int main(int argc, char* argv[])
{
	FOCP_NAME::GetOdbcSourceInfo();
	return 0;
}
