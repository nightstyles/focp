
#include "RuleDef.hpp"

FOCP_BEGIN();

RULE_API void InitRuleArgv(CRuleArgv& oArgv, CRuleStack &oStack)
{
	oArgv.pArgv = (char*)oStack.pArgv + oStack.nArgOff;
}

RULE_API bool& GetRuleArgBool(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(bool);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	bool &nRet = *(bool*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API char& GetRuleArgChar(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(char);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	char &nRet = *(char*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API int16& GetRuleArgShort(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(int16);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	int16 &nRet = *(int16*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API uint16& GetRuleArgUShort(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(uint16);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	uint16 &nRet = *(uint16*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API int32& GetRuleArgInt(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(int32);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	int32 &nRet = *(int32*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API uint32& GetRuleArgUInt(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(uint32);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	uint32 &nRet = *(uint32*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API int64& GetRuleArgLong(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(int64);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	int64 &nRet = *(int64*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API uint64& GetRuleArgULong(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(uint64);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	uint64 &nRet = *(uint64*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API float& GetRuleArgFloat(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(float);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	float &nRet = *(float*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API double& GetRuleArgDouble(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(double);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	double &nRet = *(double*)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return nRet;
}

RULE_API CString& GetRuleArgString(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(CString*);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	CString* &nRet = *(CString**)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	if(nRet == NULL)
		nRet = new CString;
	return *nRet;
}

// ---------------------------------------------------
// CRuleFileInfo
// ---------------------------------------------------
CRuleFileInfo::CRuleFileInfo()
{
	m_sFileName = NULL;
	m_nLine = 1;
	m_nCol = 1;
}

CRuleFileInfo::~CRuleFileInfo()
{
}

void CRuleFileInfo::SetFile(const char* sFileName, uint32 nLine, uint32 nCol)
{
	m_sFileName = sFileName;
	m_nLine = nLine;
	m_nCol = nCol;
}

void CRuleFileInfo::SetFile(const CRuleFileInfo &oSrc)
{
	m_sFileName = oSrc.m_sFileName;
	m_nLine = oSrc.m_nLine;
	m_nCol = oSrc.m_nCol;
}

// ---------------------------------------------------
// CRuleChecker
// ---------------------------------------------------
CRuleChecker::CRuleChecker()
{
}

CRuleChecker::~CRuleChecker()
{
}

void CRuleChecker::OnError(const CRuleFileInfo &oFileInfo, const char* sFormat, ...)
{
	CVaList oVaList;
	CFormatString oInfo;
	VaStart(oVaList, sFormat);
	if(oFileInfo.m_sFileName)
	{
		if(oFileInfo.m_nCol)
			oInfo.Print("[%s:%u-%u] ", oFileInfo.m_sFileName, oFileInfo.m_nLine, oFileInfo.m_nCol);
		else
			oInfo.Print("[%s:%u] ", oFileInfo.m_sFileName, oFileInfo.m_nLine);
	}
	oInfo.PrintV(sFormat, oVaList);
	VaEnd(oVaList);
	FocpError(("%s", oInfo.GetStr()));
}

FOCP_END();
