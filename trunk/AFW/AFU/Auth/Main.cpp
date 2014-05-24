
#include "AFC.hpp"

FOCP_BEGIN();

static bool GetHostInfo(const char* sName, uint64 &nHostId, CString& oAppName, CVector<CString> &oHostInfo, uint32 &nIdx)
{
	CFile oFile;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	CFormatString oFileName;
	oFileName.Print("disk://%s/%s.host", pHome->GetDir(), sName);
	int32 nRet = oFile.Open(oFileName.GetStr(), "r");
	if(nRet)
		return false;
	CString oLine, oText;
	CFileFormatter oFormat(&oFile);
	while(oFormat.Scan("%r", &oLine) == 1)
	{
		if(!oLine.Empty())
			oText += oLine;
	}
	uint32 i, nLen = oText.GetSize();
	char* sInfo = Base64Decode(oText.GetStr(), nLen, &nLen);
	if(!sInfo)
		return false;
	for(i=0; i<nLen; ++i)
		sInfo[i] = (char)(((uint8)sInfo[i]) ^ ((uint8)'J'));//Jacky's First Char
	CFormatString oNewText(sInfo);
	CMalloc::Free(sInfo);
	while(oNewText.Scan("%s", &oLine) == 1)
	{
		if(!oLine.Empty())
			oHostInfo.Insert((uint32)(-1), oLine);
	}
	nLen = oHostInfo.GetSize();
	for(i=0; i<nLen; ++i)
	{
		CFormatString x(oHostInfo[i]);
		if(x.Scan("%u64", &nHostId) == 1)
		{
			oHostInfo.Remove(i);
			nIdx = i;
			break;
		}
	}
	if(i >= nLen - 1)
		return false;
	oAppName = oHostInfo[i];
	if(oAppName.Compare(sName))
		return false;
	oHostInfo.Remove(i);
	return true;
}

static uint32 CreateAuthInfo(CVector<CString> oHostInfo, CVector<CString> &oAuthInfo, uint64 nHostId, CString& oAppName)
{
	uint32 nIdx, nSize;
	nSize = oHostInfo.GetSize();
	while(nSize)
	{
		nIdx = (uint32)Random();
		nIdx %= nSize;
		oAuthInfo.Insert((uint32)(-1), oHostInfo[nIdx]);
		oHostInfo.Remove(nIdx);
		--nSize;
	}
	nIdx = (uint32)Random();
	nIdx %= oAuthInfo.GetSize();
	return nIdx;
}

static void Encode(CVector<CString> &oInfo)
{
	CString oText;
	uint32 i, nSize = oInfo.GetSize();
	for(i=0; i<nSize; ++i)
	{
		if(i)
			oText += " ";
		oText += oInfo[i];
	}

	char* p = (char*)oText.GetStr();
	nSize = oText.GetSize();
	for(i=0; i<nSize; ++i)
		p[i] = (char)(((uint8)p[i]) ^ ((uint8)'J'));//Jacky's First Char

	char* s = Base64Encode((const char*)p,  nSize, &nSize);
	oText = s;
	CMalloc::Free(s);

	oInfo.Clear();

	s = (char*)oText.GetStr();
	nSize = 0;
	CString oLine;
	while(*s)
	{
		if(nSize>=80)
		{
			oInfo.Insert((uint32)(-1), oLine);
			oLine = "";
			nSize = 0;
		}
		oLine += *s;
		++s;
		++nSize;
	}
	if(!oLine.Empty())
		oInfo.Insert((uint32)(-1), oLine);
}

static bool CreateAuthFile(uint64 nHostId, const char* sAppName, CVector<CString> &oInfo, bool bAuth, bool bForce)
{
	CFile oFile;
	CFilePathInfo* pHome = CFilePathInfo::GetInstance();
	CFormatString oFileName;
	oFileName.Print("%s/cfg/%u64", pHome->GetHome(), nHostId);
	CDiskFileSystem::GetInstance()->CreateDirectory(oFileName.GetStr());
	oFileName.Clear();
	if(bAuth)
		oFileName.Print("disk://%s/cfg/%u64/%s.auth", pHome->GetHome(), nHostId, sAppName);
	else
		oFileName.Print("disk://%s/cfg/%u64/%s.host", pHome->GetHome(), nHostId, sAppName);
	int32 nRet = oFile.Open(oFileName.GetStr(), "r");
	if(!nRet)
	{
		Print("The file '%s' is existent\n", oFileName.GetStr() + 7);
		if(!bForce)
			return false;
		Print("Try rebuild the file '%s'\n", oFileName.GetStr() + 7);
		oFile.Close();
	}
	nRet = oFile.Open(oFileName.GetStr(), "wcd");
	if(nRet)
	{
		Print("Create the file '%s' failure\n", oFileName.GetStr() + 7);
		return false;
	}
	CFileFormatter oFormat(&oFile);
	uint32 nSize = oInfo.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		oFormat.Print("%s\n", oInfo[i].GetStr());
	Print("Create the file '%s' success\n", oFileName.GetStr() + 7);
	return true;
}

static bool Auth(int argc, char* argv[])
{
	uint32 nIdx1, nIdx2;
	uint64 nHostId;
	CString oAppName;
	CFormatString oHostId;
	bool bForce = false;
	CVector<CString> oHostInfo, oAuthInfo;
	if(argc < 2)
	{
		CFilePathInfo* pHome = CFilePathInfo::GetInstance();
		Print("The usage is: %s AppName [force]\n");
		goto end;
	}
	if(!GetHostInfo(argv[1], nHostId, oAppName, oHostInfo, nIdx1))
	{
		CFilePathInfo* pHome = CFilePathInfo::GetInstance();
		Print("Invalid host file '%s/%s.host'\n", pHome->GetDir(), argv[1]);
		goto end;
	}
	nIdx2 = CreateAuthInfo(oHostInfo, oAuthInfo, nHostId, oAppName);
	oHostId.Print("%u64", nHostId);
	oHostInfo.Insert(nIdx1, oAppName);
	oHostInfo.Insert(nIdx1, oHostId);
	oAuthInfo.Insert(nIdx2, oAppName);
	oAuthInfo.Insert(nIdx2, oHostId);
	Encode(oHostInfo);
	Encode(oAuthInfo);
	if(argc > 2 && !CString::StringCompare(argv[2], "force", false))
		bForce = true;
	if(!CreateAuthFile(nHostId, oAppName.GetStr(), oHostInfo, false, bForce))
		goto end;
	if(!CreateAuthFile(nHostId, oAppName.GetStr(), oAuthInfo, true, bForce))
		goto end;
	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
	return true;
end:
	CFileInterface::GetInterfaceManager()->UnLoad();
	CMainModule::GetInstance()->UnLoad();
	CThreadVariablePool::GetInstance()->ClearThreadVariable();
	return false;
}

FOCP_END();

int main(int argc, char* argv[])
{
	FOCP_NAME::RandomSeed(FOCP_NAME::CTimer::GetTime());
	if(!FOCP_NAME::Auth(argc, argv))
		return -1;
	return 0;
}
