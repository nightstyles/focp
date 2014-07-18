
#include "AFC.hpp"

FOCP_BEGIN();

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

static void GetMdbAccessInfo()
{
	//????, MODE, HOSTID, USER, PASSWD
	uint32 nLen;
	uint64 nHostId = (uint64)CFile::GetHostId();
	CString oRandom, oUser, oPasswd;
	GetRandomString(oRandom, 10, "");
loop2:
	Print("Please input the MDB USER(max 8 non-white-space character):\n");
	if(Scan("%8s", &oUser) != 1)
		goto loop2;
	oUser.Trim();
	nLen = oUser.GetSize();
	if(nLen == 0 || nLen > 8)
		goto loop2;
loop3:
	Print("Please input the MDB PASSWORD(max 8 non-white-space character):\n");
	if(Scan("%8s", &oPasswd) != 1)
		goto loop3;
	oPasswd.Trim();
	nLen = oPasswd.GetSize();
	if(nLen == 0 || nLen > 8)
		goto loop3;
	CFormatString oInfo;
	oInfo.Print("%s    2    %u64    %s    %s", oRandom.GetStr(), nHostId, oUser.GetStr(), oPasswd.GetStr());
	nLen = oInfo.GetSize();
	uint8* p = (uint8*)oInfo.GetStr();
	for(uint32 i=0; i<nLen; ++i)
		p[i] ^= (uint8)'J';//Jacky's First Char
	char* s = Base64Encode((const char*)p,  nLen, &nLen);
	Print("This MDB access info is:\n%s\n", s);
	Print("Customer No. is %u64\n", nHostId);
	CMalloc::Free(s);

	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
}

FOCP_END();

int main(int argc, char* argv[])
{
	FOCP_NAME::RandomSeed(FOCP_NAME::CTimer::GetTime());
	FOCP_NAME::GetMdbAccessInfo();
	return 0;
}
