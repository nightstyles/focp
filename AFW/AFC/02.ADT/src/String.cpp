
#include "Formatter.hpp"
#include "Arithmetic.hpp"

#include <ctype.h>

FOCP_BEGIN();

CString::~CString()
{
	Clear();
}

CString::CString()
{
	m_nSize = 0;
	m_pData = NULL;
	m_bOwned = true;
}

CString::CString(const char* pStr, uint32 nCount)
{
	uint32 i;
	if(nCount == 0)
		nCount = StringLength(pStr);
	if(nCount == 0)
	{
		m_nSize = 0;
		m_pData = NULL;
	}
	else
	{
		m_pData = new char[nCount+1];
		for(i=0; i<nCount; ++i)
			m_pData[i] = pStr[i];
		m_pData[i] = 0;
		m_nSize = nCount;
	}
	m_bOwned = true;
}

CString::CString(char nCh, uint32 nCount)
{
	uint32 i;
	m_nSize = 0;
	m_pData = NULL;
	if(nCount)
	{
		m_pData = new char[nCount+1];
		for(i=0; i<nCount; ++i)
			m_pData[i] = nCh;
		m_pData[i] = 0;
		m_nSize = nCount;
	}
	m_bOwned = true;
}

CString::CString(const CString& oStr)
{
	m_nSize = oStr.m_nSize;
	if(m_nSize == 0)
		m_pData = NULL;
	else
	{
		m_pData = new char[m_nSize+1];
		for(uint32 i=0; i<=m_nSize; ++i)
			m_pData[i] = oStr.m_pData[i];
	}
	m_bOwned = true;
}

CString::CString(const CString& oStr, uint32 nIdx, uint32 nCount)
{
	if(nIdx >= oStr.m_nSize)
		nIdx = oStr.m_nSize;
	uint32 nRest = oStr.m_nSize - nIdx;
	if(nCount > nRest)
		nCount = nRest;
	m_nSize = 0;
	m_pData = NULL;
	m_bOwned = true;
	if(nCount)
	{
		CString o(oStr.m_pData+nIdx, nCount);
		Swap(o);
	}
}

CString& CString::operator=(const CString& oStr)
{
	if(this != &oStr)
	{
		CString o(oStr);
		Swap(o);
	}
	return *this;
}

CString& CString::operator=(const char* pStr)
{
	CString o(pStr);
	Swap(o);
	return *this;
}

CString& CString::operator=(char nCh)
{
	CString o(nCh);
	Swap(o);
	return *this;
}

CString& CString::operator+=(const CString& oStr)
{
	Append(oStr);
	return *this;
}

CString& CString::operator+=(const char* pStr)
{
	Append(pStr);
	return *this;
}

CString& CString::operator+=(char nCh)
{
	Append(nCh);
	return *this;
}

CString CString::operator+(const char* pStr)
{
	CString oRet(*this);
	oRet += pStr;
	return oRet;
}

CString CString::operator+(const CString& oStr)
{
	CString oRet(*this);
	oRet += oStr;
	return oRet;
}

CString CString::operator+(char nCh)
{
	CString oRet(*this);
	oRet += nCh;
	return oRet;
}

void CString::Clear()
{
	if(m_pData && m_bOwned)
		delete[] m_pData;
	m_pData = NULL;
	m_nSize = 0;
	m_bOwned = true;
}

char* CString::StringCopy(char* sDst, const char* sSrc, uint32 nMaxCopy)
{
	uint32 i;
	if(sSrc == sDst)
	{
		if(nMaxCopy)
		{
			for(i=0; (i<nMaxCopy) && sSrc[i]; ++i);
			sDst[i] = '\0';
		}
	}
	else if(sSrc > sDst)
	{
		if(nMaxCopy==0)for(i=0; 1; ++i)
			{
				sDst[i] = sSrc[i];
				if(sSrc[i] == '\0')
					break;
			}
		else for(i=0; i<nMaxCopy; ++i)
			{
				sDst[i] = sSrc[i];
				if(sSrc[i] == '\0')
					break;
			}
	}
	else if(nMaxCopy==0)
	{
		for(i=0; sSrc[i]; ++i);
		for(; i; --i)
			sDst[i] = sSrc[i];
		sDst[i] = sSrc[i];
	}
	else
	{
		for(i=0; i<nMaxCopy && sSrc[i]; ++i);
		if(i == nMaxCopy)
			--i;
		for(; i; --i)
			sDst[i] = sSrc[i];
		sDst[i] = sSrc[i];
	}
	return sDst;
}

char* CString::StringCatenate(char* sDst, const char* sSrc, uint32 nMaxCopy)
{
	StringCopy(sDst+StringLength(sDst), sSrc, nMaxCopy);
	return sDst;
}

int32 CString::StringCompare(const char* sLeft, const char* sRight, bool bSensitive, uint32 nMaxCmp)
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

uint32 CString::StringLength(const char* sStr, uint32 nMaxSize)
{
	uint32 nRet;
	if(sStr == NULL)
		nRet = 0;
	else if(nMaxSize == 0)
	{
		const char* pOld = sStr;
		while(*sStr)
			++sStr;
		nRet = sStr - pOld;
	}
	else
	{
		for(nRet=0; *sStr; ++sStr)
		{
			++nRet;
			if(nRet==nMaxSize)
				break;
		}
	}
	return nRet;
}

char* CString::CharOfString(const char* sStr, char nCh, bool bReverse)
{
	if(bReverse)
	{
		const char* sEnd = sStr + StringLength(sStr);
		while(sEnd != sStr)
		{
			--sEnd;
			if(nCh == *sEnd)
				return (char*)sEnd;
		}
	}
	else
	{
		while(*sStr)
		{
			if(nCh == *sStr)
				return (char*)sStr;
			++sStr;
		}
	}
	return NULL;
}

char* CString::SetOfString(const char* sStr, const char* sCharSet, bool bReverse)
{
	if(bReverse)
	{
		const char* sEnd = sStr + StringLength(sStr);
		while(sEnd != sStr)
		{
			--sEnd;
			if(CharOfString(sCharSet, *sEnd))
				return (char*)sEnd;
		}
	}
	else
	{
		while(*sStr)
		{
			if(CharOfString(sCharSet, *sStr))
				return (char*)sStr;
			++sStr;
		}
	}
	return NULL;
}

char* CString::StringOfString(const char* sStr, const char* sSubStr, bool bSensitive)
{
	if(sStr == NULL || sSubStr == NULL)
		return NULL;
	uint32 nLen = StringLength(sSubStr);
	while(*sStr)
	{
		if(!StringCompare(sStr, sSubStr, bSensitive, nLen))
			return (char*)sStr;
		++sStr;
	}
	return NULL;
}

char* CString::TokenOfString(const char* &sStr, const char* sDelimiters)
{
	const char* sLocal = " \t\r\n\v\f";
	if(!sDelimiters || !sDelimiters[0])
		sDelimiters = sLocal;

	while(*sStr)
	{
		if(!CharOfString(sDelimiters, *sStr))
			break;
		++sStr;
	}
	char* pRet = (char*)((*sStr)?sStr:NULL);
	while(*sStr)
	{
		if(CharOfString(sDelimiters, *sStr))
			break;
		++sStr;
	}
	return pRet;
}

char* CString::Detach(uint32 &nSize)
{
	char* sRet = m_pData;
	nSize = m_nSize;
	m_pData = NULL;
	m_nSize = 0;
	m_bOwned = true;
	return sRet;
}

CString& CString::Bind(char* pStr, uint32 nCount)
{
	Clear();
	m_pData = pStr;
	if(!nCount)
		nCount = StringLength(pStr);
	m_nSize = nCount;
	m_bOwned = false;
	return *this;
}

char& CString::operator[](uint32 nIdx)
{
	return ((char*)m_pData)[nIdx];
}

const char* CString::GetStr() const
{
	if(m_pData == NULL)
		return "";
	return m_pData;
}

bool CString::Empty() const
{
	return m_pData==NULL;
}

void CString::Pack()
{
	CString o(m_pData);
	Swap(o);
}

uint32 CString::GetSize() const
{
	return m_nSize;
}

void CString::SetSize(uint32 nNewSize, char nFillChar)
{
	if(!m_bOwned || nNewSize == m_nSize)
		return;
	if(nNewSize == 0)
		Clear();
	else
	{
		char* pData = new char[nNewSize+1];
		pData[nNewSize] = '\0';
		uint32 i, nCopySize = m_nSize;
		if(nCopySize > nNewSize)
			nCopySize = nNewSize;
		for(i=0; i<nCopySize; ++i)
			pData[i] = m_pData[i];
		for(; i<nNewSize; ++i)
			pData[i] = nFillChar;
		if(m_pData)
			delete[] m_pData;
		m_pData = pData;
		m_nSize = nNewSize;
	}
}

CString& CString::TrimLeft()
{
	TrimDetail(1);
	return *this;
}

CString& CString::TrimRight()
{
	TrimDetail(2);
	return *this;
}

CString& CString::Trim()
{
	TrimDetail(3);
	return *this;
}

CString& CString::Append(const CString& oStr)
{
	return Insert(m_nSize, oStr);
}

CString& CString::Append(const char* pStr, uint32 nCount)
{
	return Insert(m_nSize, pStr, nCount);
}

CString& CString::Append(char nCh, uint32 nCount)
{
	return Insert(m_nSize, nCh, nCount);
}

CString& CString::Assign(const CString& oStr)
{
	if(this != &oStr)
	{
		CString oTmp(oStr);
		Swap(oTmp);
	}
	return *this;
}

CString& CString::Assign(const char* pStr, uint32 nCount)
{
	CString oTmp(pStr, nCount);
	Swap(oTmp);
	return *this;
}

CString& CString::Assign(char nCh, uint32 nCount)
{
	CString oTmp(nCh, nCount);
	Swap(oTmp);
	return *this;
}

CString& CString::Insert(uint32 nIdx, const CString& oStr)
{
	return Insert(nIdx, oStr.m_pData, oStr.m_nSize);
}

CString& CString::Insert(uint32 nIdx, char nCh, uint32 nCount)
{
	if(nCount)
	{
		CString o(nCh, nCount);
		Insert(nIdx, o.m_pData, o.m_nSize);
	}
	return *this;
}

CString& CString::Insert(uint32 nIdx, const char* pStr, uint32 nCount)
{
	uint32 i, nSize, j, nRest;
	if(nCount == 0)
		nCount = StringLength(pStr);
	if(nCount == 0)
		return *this;
	if(m_pData && pStr>=m_pData && pStr<m_pData+m_nSize)
	{
		if(nCount+nIdx > m_nSize)
			FocpAbort(( "read memory beyond the border" ));
		return Insert(nIdx, CString(pStr, nCount));
	}
	if(nIdx > m_nSize)
		nIdx = m_nSize;
	nRest = m_nSize - nIdx;
	if(m_bOwned)
	{
		nSize = m_nSize + nCount;
		char* pData = new char[nSize+1];
		pData[nSize] = '\0';
		for(i=0; i<nIdx; ++i)
			pData[i] = m_pData[i];
		for(j=0; j<nCount; ++i,++j)
			pData[i] = pStr[j];
		for(j=0; j<nRest; ++i,++j)
			pData[i] = m_pData[j+nIdx];
		if(m_pData)
			delete[] m_pData;
		m_pData = pData;
		m_nSize = nSize;
	}
	else if(nRest)
	{
		if(nCount > nRest)
			nCount = nRest;
		nRest -= nCount;
		for(i=nIdx,j=0; j<nRest; ++j,++i)
			m_pData[i+nCount] = m_pData[i];
		for(i=nIdx,j=0; j<nCount; ++j,++i)
			m_pData[i] = pStr[j];
	}
	return *this;
}

CString& CString::Replace(uint32 nIdx, const CString& oStr)
{
	return Replace(nIdx, oStr.m_pData, oStr.m_nSize);
}

CString& CString::Replace(uint32 nIdx, char nCh, uint32 nCount)
{
	if(nCount)
	{
		CString o(nCh, nCount);
		Replace(nIdx, o.m_pData, o.m_nSize);
	}
	return *this;
}

CString& CString::Replace(uint32 nIdx, const char* pStr, uint32 nCount)
{
	char* pData;
	uint32 i, nSize;
	if(nCount == 0)
		nCount = StringLength(pStr);
	if(nCount == 0)
		return *this;
	if(nIdx > m_nSize)
		nIdx = m_nSize;
	pData = m_pData;
	nSize = nCount + nIdx;
	if(nSize > m_nSize)
	{
		if(m_bOwned)
		{
			pData = new char[nSize+1];
			pData[nSize] = 0;
			for(i=0; i<nIdx; ++i)
				pData[i] = m_pData[i];
			m_nSize = nSize;
		}
		else
			nCount = m_nSize - nIdx;
	}
	if(nCount)
		CBinary::MemoryCopy(pData+nIdx, pStr, nCount);
	if(pData != m_pData)
	{
		if(m_pData)
			delete[] m_pData;
		m_pData = pData;
	}
	return *this;
}

void CString::TrimDetail(uint32 nMode)
{
	const char* sLeft;
	const char* sEnd;
	if(nMode == 0 || !m_pData)
		return;
	sLeft = m_pData;
	if(nMode & 1)
	{
		while(*sLeft && isspace(*sLeft))
			++sLeft;
	}
	sEnd = sLeft;
	while(*sEnd)
		++sEnd;
	if(sEnd != sLeft && (nMode & 2))
	{
		--sEnd;
		while(isspace(*sEnd))
			--sEnd;
		++sEnd;
	}
	uint32 nLen = sEnd - sLeft;
	if(sLeft != m_pData && nLen)
		CBinary::MemoryCopy(m_pData, sLeft, nLen);
	m_pData[nLen] = 0;
}

CString& CString::Remove(uint32 nIdx, uint32 nCount)
{
	uint32 nLen, nRest;
	nLen = m_nSize;
	if(nIdx > nLen)
		nIdx = nLen;
	nRest = nLen - nIdx;
	if(nCount > nRest)
		nCount = nRest;
	if(nIdx == 0 && nCount >= nLen)
		Clear();
	else
	{
		nRest -= nCount;
		if(nRest)
			CBinary::MemoryCopy(m_pData+nIdx, m_pData+nIdx+nCount, nRest);
		m_pData[nIdx+nRest] = 0;
	}
	return *this;
}

bool CString::Equal(const CString& oStr, bool bSensitive) const
{
	if(this == &oStr)
		return true;
	return Compare(oStr.m_pData, bSensitive, oStr.m_nSize)==0;
}

bool CString::Equal(const char* pStr, bool bSensitive, uint32 nCount) const
{
	return Compare(pStr, bSensitive, nCount)==0;
}

int32 CString::Compare(const CString& oStr, bool bSensitive) const
{
	if(this == &oStr)
		return 0;
	return Compare(oStr.m_pData, bSensitive, oStr.m_nSize);
}

int32 CString::Compare(const char* pStr, bool bSensitive, uint32 nCount) const
{
	return StringCompare(m_pData, pStr, bSensitive, nCount);
}

uint32 CString::Find(char nCh, uint32 nIdx, bool bSensitive) const
{
	return Find(&nCh, 1, nIdx, bSensitive);
}

uint32 CString::Find(const char* pStr, uint32 nIdx, bool bSensitive) const
{
	return Find(pStr, 0, nIdx, bSensitive);
}

uint32 CString::Find(const CString& oStr, uint32 nIdx, bool bSensitive) const
{
	return Find(oStr.m_pData, oStr.m_nSize, nIdx, bSensitive);
}

uint32 CString::Find(const char* pStr, uint32 nCount, uint32 nIdx, bool bSensitive) const
{
	uint32 i, j, nLen;
	nCount = StringLength(pStr, nCount);
	if(nCount == 0)
		return (uint32)(-1);
	nLen = m_nSize;
	if(nIdx >= nLen)
		return (uint32)(-1);
	uint32 nRest = nLen - nIdx;
	for(i=nIdx; i<nLen; ++i, --nRest)
	{
		if(nRest < nCount)
			break;
		nIdx = i;
		for(j=0; j<nCount; ++j, ++nIdx)
		{
			if(bSensitive)
			{
				if(m_pData[nIdx] != pStr[j])
					break;
			}
			else
			{
				char c1 = m_pData[nIdx];
				char c2 = pStr[j];
				if(c1 >= 'a' && c1 <= 'z')
					c1 -= 'a' - 'A';
				if(c2 >= 'a' && c2 <= 'z')
					c2 -= 'a' - 'A';
				if(c1 != c2)
					break;
			}
		}
		if(j>=nCount)
			return i;
	}
	return (uint32)(-1);
}

uint32 CString::GetToken(CString& oToken, uint32 nIdx, const CString& oDelimiters) const
{
	const char* sLocal = " \t\r\n\v\f";
	uint32 i, j, nLen = oDelimiters.m_nSize;
	if(nLen)
		sLocal = oDelimiters.m_pData;
	else
		nLen = 6;
	if(nIdx >= m_nSize)
	{
		if(this != &oToken)
			oToken.Clear();
		return (uint32)(-1);
	}

	const char* sStart = m_pData + nIdx;
	for(i = nIdx; (*sStart); ++i)
	{
		for(j=0; j<nLen; ++j)
		{
			if(*sStart == sLocal[j])
				break;
		}
		if(j == nLen)
			break;
		++sStart;
	}

	const char* sEnd = sStart;
	for(; (*sEnd); ++i)
	{
		for(j=0; j<nLen; ++j)
		{
			if(*sEnd == sLocal[j])
				break;
		}
		if(j < nLen)
			break;
	}

	nLen = sEnd - sStart;
	if(nLen == 0)
	{
		if(this != &oToken)
			oToken.Clear();
		return (uint32)(-1);
	}

	CString oTmp(sStart, sEnd - sStart);
	oToken.Swap(oTmp);
	if(this == &oToken)
		return (uint32)(0);

	return i;
}

const char* CString::SkipSpace(const char* sStr)
{
	while((*sStr) && IsSpace(*sStr))
		++sStr;
	return sStr;
}

bool CString::IsIdentifierOfC() const
{
	if(m_pData == NULL || !m_pData[0])
		return false;
	char* pData = m_pData;
	if((pData[0] != '_') && !IsAlpha(pData[0]))
		return false;
	++pData;
	while(pData[0])
	{
		if((pData[0] != '_') && !IsAlnum(pData[0]))
			return false;
		++pData;
	}
	return true;
}

void CString::GetCIdentifier(CString& oIdentifier, uint32 &nIdx) const
{
	oIdentifier.Clear();
	if(nIdx >= m_nSize)
		return;
	const char* pData = m_pData+nIdx;
	CString::GetCIdentifier(oIdentifier, pData);
	nIdx = pData - m_pData;
}

void CString::GetCIdentifier(CString& oIdentifier, const char* &sStr)
{
	oIdentifier.Clear();
	if(sStr == NULL)
		return;
	sStr = SkipSpace(sStr);
	if((sStr[0] != '_') && !IsAlpha(sStr[0]))
		return;
	oIdentifier += sStr[0];
	++sStr;
	while(sStr[0])
	{
		if((sStr[0] != '_') && !IsAlnum(sStr[0]))
			break;
		oIdentifier += sStr[0];
		++sStr;
	}
}

CString CString::GetCString(const char* sStr, bool bWithoutQuote, uint32 nCount)
{
	if(nCount == 0)
		nCount = StringLength(sStr);
	CString oRet;
	if(!bWithoutQuote)
		oRet += "\"";
	char* pData = (char*)sStr;
	for(uint32 i=0; i<nCount; ++pData,++i)switch(*pData)
		{
		case '\a':
			oRet += "\\a";
			break;
		case '\b':
			oRet += "\\b";
			break;
		case '\f':
			oRet += "\\f";
			break;
		case '\n':
			oRet += "\\n";
			break;
		case '\r':
			oRet += "\\r";
			break;
		case '\t':
			oRet += "\\t";
			break;
		case '\v':
			oRet += "\\v";
			break;
		case '\'':
			oRet += "\\\'";
			break;
		case '\"':
			oRet += "\\\"";
			break;
		case '\\':
			oRet += "\\\\";
			break;
		default:
			if(pData[0] < '\0' || IsPrint(pData[0]))
				oRet += pData[0];
			else
			{
				uchar c = pData[0];
				oRet += "\\X";
				uchar s[3];
				s[2] = 0;
				s[0] = (c>>4)+'0';
				if(s[0] > '9')
					s[0] = s[0] - '0' + 'A';
				s[1] = (c&0x0F) + '0';
				if(s[1] > '9')
					s[1] = s[1] - '0' + 'A';
				oRet += (const char*)s;
			}
		}
	if(!bWithoutQuote)
		oRet += "\"";
	return oRet;
}

CString& CString::ToCString(bool bWithoutQuote)
{
	*this = GetCString(m_pData, bWithoutQuote);
	return *this;
}

void CString::Swap(CString &oSrc)
{
	if(this != &oSrc)
	{
		::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
		::FOCP_NAME::Swap(m_pData, oSrc.m_pData);
	}
}

bool CString::IsAlnum(char c)
{
	return isalnum(c)?true:false;
}

bool CString::IsAlpha(char c)
{
	return isalpha(c)?true:false;
}

bool CString::IsControl(char c)
{
	return iscntrl(c)?true:false;
}

bool CString::IsDigit(char c)
{
	return isdigit(c)?true:false;
}

bool CString::IsGraph(char c)
{
	return isgraph(c)?true:false;
}

bool CString::IsLower(char c)
{
	return islower(c)?true:false;
}

bool CString::IsUpper(char c)
{
	return isupper(c)?true:false;
}

bool CString::IsPrint(char c)
{
	return isprint(c)?true:false;
}

bool CString::IsPunct(char c)
{
	return ispunct(c)?true:false;
}

bool CString::IsSpace(char c)
{
	return isspace(c)?true:false;
}

bool CString::IsXdigit(char c)
{
	return isxdigit(c)?true:false;
}

char CString::ToLower(char c)
{
	return tolower(c);
}

char CString::ToUpper(char c)
{
	return toupper(c);
}

CString& CString::ToUpper()
{
	for(uint32 i=0; i<m_nSize; ++i)
		m_pData[i] = CString::ToUpper(m_pData[i]);
	return *this;
}

CString& CString::ToLower()
{
	for(uint32 i=0; i<m_nSize; ++i)
		m_pData[i] = CString::ToLower(m_pData[i]);
	return *this;
}

int32 CString::Atoi(const char* s, const char** sEnd)
{
	int32 nRet=0;
	CFormatString oString;
	s = SkipSpace(s);
	oString.Bind((char*)s);
	oString.Scan("%i", &nRet);
	if(sEnd)
	{
		s += oString.GetAlignPos();
		sEnd[0] = SkipSpace(s);
	}
	return nRet;
}

int64 CString::Atoi64(const char* s, const char** sEnd)
{
	int64 nRet=0;
	CFormatString oString;
	s = SkipSpace(s);
	oString.Bind((char*)s);
	oString.Scan("%i64", &nRet);
	if(sEnd)
	{
		s += oString.GetAlignPos();
		sEnd[0] = SkipSpace(s);
	}
	return nRet;
}

double CString::Atof(const char* s, const char** sEnd)
{
	double nRet=0.0;
	CFormatString oString;
	s = SkipSpace(s);
	oString.Bind((char*)s);
	oString.Scan("%f", &nRet);
	if(sEnd)
	{
		s += oString.GetAlignPos();
		sEnd[0] = SkipSpace(s);
	}
	return nRet;
}

FOCP_END();
