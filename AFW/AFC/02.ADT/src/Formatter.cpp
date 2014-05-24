
#include "Formatter.hpp"
#include "Vector.hpp"

#include <math.h>
#include <float.h>

#ifdef MSVC
#pragma warning(disable:4355)
#endif

FOCP_BEGIN();

template<typename TFunc> TFunc GetInterface(const char *sFormat, const CRbMap<CString, TFunc> &oMethodTable, uint32 &nWidthLen)
{
	char x[2] = {sFormat[0], '\0'};
	++x[0];
	const CRbTreeNode* pFirst = oMethodTable.First();
	const CRbTreeNode* pEnd = oMethodTable.End();
	const CRbTreeNode* pNode = oMethodTable.LowerBound(x);
	if(pNode == pFirst)
		return NULL;
	if(pNode == pEnd)
		pNode = oMethodTable.Last();
	else
		pNode = oMethodTable.GetPrev(pNode);
	do
	{
		const CString& oKey = oMethodTable.GetKey(pNode);
		if(oKey.GetStr()[0] != sFormat[0])
			break;
		uint32 nLen = oKey.GetSize();
		if(oKey.GetSize() == 1 || !oKey.Compare(sFormat, true, nLen))
		{
			nWidthLen = nLen;
			return oMethodTable.GetItem(pNode);
		}
		pNode = oMethodTable.GetPrev(pNode);
	}while(pNode != pFirst);
	return NULL;
}

//基础格式化读写函数
//0：正常，-1：IO异常，-2：无效数据

//读取字符
static int32 FormatReadChar(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	char* s;
	int32 nRet;

	if(nWidth == 0)
		return -2;

	if(nWidth < 0)
		nWidth = 1;

	if(bIgnore == false)
		s = VaArg<char*>(pArgList);

	for(int32 i=0; i<nWidth; ++i)
	{
		nRet = pFormatter->GetChar(c);
		if(nRet == 0)
		{
			if(c == '\0')
			{
				pFormatter->UnGetChar(c);
				if(i == 0)
					nRet = -2;
				break;
			}
			else if(bIgnore == false)
				s[i] = c;
			continue;
		}
		if(i)
		{
			nRet = 0;
			break;
		}
	}

	return nRet;
}

//读字符串
//跳过非换行空白，读取多个字符序列，遇空白结束
//0：正常，-1：IO异常，-2：无效数据
static int32 FormatReadString(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr = NULL;

	if(nWidth == 0)
		return -2;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}
	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(c == '\n')
			break;
		if(!CString::IsSpace(c))
			break;
	}
	if(nRet || c=='\n')
		return nRet;

	if(bIgnore == false)
		pStr[0] += c;

	nLen = 1;

	while(!pFormatter->GetChar(c))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(c == '\r')
			continue;
		if(c == '\n')
			break;
		if(CString::IsSpace(c))
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(nWidth > 0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(bIgnore == false)
			pStr[0] += c;
		++nLen;
	}

	return 0;
}

static int32 FormatReadLine(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr;

	if(nWidth == 0)
		return -2;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}
	bool bMeetReturn = false;
loop:
	nRet = pFormatter->GetChar(c);
	if(nRet)
	{
		if(bMeetReturn)
			return 0;
		return nRet;
	}
	if(c == '\0')
	{
		pFormatter->UnGetChar(c);
		if(bMeetReturn)
			return 0;
		return -2;
	}
	else if(c == '\r')
	{
		if(bMeetReturn)
		{
			pFormatter->UnGetChar(c);
			return 0;
		}
		bMeetReturn = true;
		goto loop;
	}
	else if(c == '\n')
		return 0;
	if(bMeetReturn)
	{
		pFormatter->UnGetChar(c);
		return 0;
	}

	if(bIgnore == false)
		pStr[0] += c;
	nLen = 1;

	while(!pFormatter->GetChar(c))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(c == '\r')
		{
			if(bMeetReturn)
			{
				pFormatter->UnGetChar(c);
				break;
			}
			bMeetReturn = true;
			continue;
		}
		if(c == '\n')
			break;
		if(bMeetReturn)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(nWidth>0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(bIgnore == false)
			pStr[0] += c;
		++nLen;
	}
	return 0;
}

static int32 FormatReadMultiLine(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr;

	if(nWidth == 0)
		return -2;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}

	nLen = 0;
	bool bMeetBackSlash = false;
	bool bMeetReturn = false;
	while(true)
	{
		nRet = pFormatter->GetChar(c);
		if(nRet)
		{
			if(nLen == 0)
				return nRet;
			return 0;
		}
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			if(bMeetReturn)
				return 0;
			if(nLen == 0)
				return -2;
			return 0;
		}
		else if(c == '\r')
		{
			if(bMeetReturn)
			{
				pFormatter->UnGetChar(c);
				return 0;
			}
			bMeetReturn = true;
			continue;
		}
		if(c == '\n')
		{
			if(bMeetBackSlash)
			{
				bMeetBackSlash = false;
				continue;
			}
			break;
		}
		if(bMeetReturn)
		{
			pFormatter->UnGetChar(c);
			bMeetReturn = false;
			if(bMeetBackSlash)
			{
				bMeetBackSlash = false;
				continue;
			}
			break;
		}
		if(c == '\\')
		{
			if(bMeetBackSlash == false)
				bMeetBackSlash = true;
			else
			{
				if(nWidth>0 && nLen >= nWidth)
				{
					pFormatter->UnGetChar('\\');
					pFormatter->UnGetChar('\\');
					break;
				}
				if(bIgnore == false)
					pStr[0] += c;
				++nLen;
			}
		}
		else
		{
			if(bMeetBackSlash)
			{
				bMeetBackSlash = false;
				if(nWidth>0 && nLen >= nWidth)
				{
					pFormatter->UnGetChar('\\');
					pFormatter->UnGetChar(c);
					break;
				}
				if(bIgnore == false)
					pStr[0] += '\\';
				++nLen;
			}
			if(nWidth>0 && nLen >= nWidth)
			{
				pFormatter->UnGetChar(c);
				break;
			}
			if(bIgnore == false)
				pStr[0] += c;
			++nLen;
		}
	}

	return 0;
}

//读标识符
//跳过空白，读取C标示符
//0：正常，-1：IO异常，-2：无效数据
static int32 FormatReadIdentifier(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr = NULL;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}
	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(!CString::IsSpace(c))
			break;
	}
	if(nRet)
		return nRet;

	if(c != '_' && !CString::IsAlpha(c))
	{
		pFormatter->UnGetChar(c);
		return -2;
	}

	if(bIgnore == false)
		pStr[0] += c;

	nLen = 1;

	while(!pFormatter->GetChar(c))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(CString::IsSpace(c) || (c != '_' && !CString::IsAlnum(c)))
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(bIgnore == false)
			pStr[0] += c;
		++nLen;
	}

	return 0;
}

//读C字面字符串
//跳过空白，读取C标示符
//0：正常，-1：IO异常，-2：无效数据
static int32 FormatReadCString(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr = NULL;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}
	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(!CString::IsSpace(c))
			break;
	}
	if(nRet)
		return nRet;

	if(c != '\"')
	{
		pFormatter->UnGetChar(c);
		return -2;
	}

	if(bIgnore == false)
		pStr[0] += c;

	nLen = 1;

	bool bDash = false;
	while(!pFormatter->GetChar(c))
	{
		if(c == '\r' || c == '\n')
		{
			pFormatter->UnGetChar(c);
			return -2;
		}
		if(bIgnore == false)
			pStr[0] += c;
		++nLen;
		if(c == '\\')
		{
			if(bDash)
				bDash = false;
			else
				bDash = true;
		}
		else if(c == '\"')
		{
			if(bDash == false)
				return 0;
			bDash = false;
		}
		else
			bDash = false;
	}

	return -2;
}

//读C字面字符
//跳过空白，读取C标示符
//0：正常，-1：IO异常，-2：无效数据
static int32 FormatReadCChar(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	char c;
	int32 nRet;
	int32 nLen;
	CString* pStr = NULL;

	if(bIgnore == false)
	{
		pStr = VaArg<CString*>(pArgList);
		pStr->Clear();
	}
	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(!CString::IsSpace(c))
			break;
	}
	if(nRet)
		return nRet;

	if(c != '\'')
	{
		pFormatter->UnGetChar(c);
		return -2;
	}

	if(bIgnore == false)
		pStr[0] += c;

	nLen = 1;

	bool bDash = false;
	while(!pFormatter->GetChar(c))
	{
		if(c == '\r' || c == '\n')
		{
			pFormatter->UnGetChar(c);
			return -2;
		}
		if(bIgnore == false)
			pStr[0] += c;
		++nLen;
		if(c == '\\')
		{
			if(bDash)
				bDash = false;
			else
				bDash = true;
		}
		else if(c == '\'')
		{
			if(bDash == false)
				return 0;
			bDash = false;
		}
		else
			bDash = false;
	}

	return -2;
}

#define FOCP_FORMAT_READ_SYSTEM_DEFAULT 0
#define FOCP_FORMAT_READ_SYSTEM_8 8
#define FOCP_FORMAT_READ_SYSTEM_10 10
#define FOCP_FORMAT_READ_SYSTEM_16 16

static bool FormatGetDigit(uint32 nSystem, char c, uint64 &v)
{
	bool bRet = true;
	switch(nSystem)
	{
	case FOCP_FORMAT_READ_SYSTEM_8:
		if(c < '0' || c > '7')
			bRet = false;
		else
			v = (uint64)(c - '0');
		break;
	case FOCP_FORMAT_READ_SYSTEM_10:
		if(c < '0' || c > '9')
			bRet = false;
		else
			v = (uint64)(c - '0');
		break;
	case FOCP_FORMAT_READ_SYSTEM_16:
		if(c >= '0' && c <= '9')
			v = (uint64)(c - '0');
		else if(c >= 'a' && c <= 'f')
			v = (uint64)(c - 'a' + 10);
		else if(c >= 'A' && c <= 'F')
			v = (uint64)(c - 'A' + 10);
		else
			bRet = false;
		break;
	}
	return bRet;
}

static bool CheckMaxInteger(bool bMeetSign, bool bNeg, uint32 nBits, uint64 v, uint32 nSignMode)
{
	bool bRet = true;
	switch(nBits)
	{
	case 8:
		if(bMeetSign)
		{
			if(bNeg)
			{
				if(v > (uint64)FOCP_INT8_MAX+1)
					bRet = false;
			}
			else
			{
				if(v > (uint64)FOCP_INT8_MAX)
					bRet = false;
			}
		}
		else if(nSignMode == 1)
		{
			if(v > (uint64)FOCP_INT8_MAX)
				bRet = false;
		}
		else
		{
			if(v > (uint64)FOCP_UINT8_MAX)
				bRet = false;
		}
		break;
	case 16:
		if(bMeetSign)
		{
			if(bNeg)
			{
				if(v > (uint64)FOCP_INT16_MAX+1)
					bRet = false;
			}
			else
			{
				if(v > (uint64)FOCP_INT16_MAX)
					bRet = false;
			}
		}
		else if(nSignMode == 1)
		{
			if(v > (uint64)FOCP_INT16_MAX)
				bRet = false;
		}
		else
		{
			if(v > (uint64)FOCP_UINT16_MAX)
				bRet = false;
		}
		break;
	case 32:
		if(bMeetSign)
		{
			if(bNeg)
			{
				if(v > (uint64)FOCP_INT32_MAX+1)
					bRet = false;
			}
			else
			{
				if(v > (uint64)FOCP_INT32_MAX)
					bRet = false;
			}
		}
		else if(nSignMode == 1)
		{
			if(v > (uint64)FOCP_INT32_MAX)
				bRet = false;
		}
		else
		{
			if(v > (uint64)FOCP_UINT32_MAX)
				bRet = false;
		}
		break;
	case 64:
		if(bMeetSign)
		{
			if(bNeg)
			{
				if(v > (uint64)FOCP_INT64_MAX+1)
					bRet = false;
			}
			else
			{
				if(v > (uint64)FOCP_INT64_MAX)
					bRet = false;
			}
		}
		else if(nSignMode == 1)
		{
			if(v > (uint64)FOCP_INT64_MAX)
				bRet = false;
		}
		break;
	}
	return bRet;
}

static int32 FormatReadIntegerX(CFormatter* pFormatter, int32 nWidth, uint64 &nData, uint32 nBits, uint32 nSystem, uint32 nSignMode)
{
	char c;
	int32 nRet;
	int32 nLen;
	bool bMeetSign, bNeg;
	uint64 nDigit, nData2;

	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(c == '\n')
		{
			nRet = -2;
			break;
		}
		if(!CString::IsSpace(c))
			break;
	}

	if(nRet)
		return nRet;

	bNeg = false;
	bMeetSign = false;
	if(nSignMode == 0 || nSignMode == 1)
	{
		if(c == '+' || c == '-')
		{
			bMeetSign = true;
			if(c == '-')
				bNeg = true;
			nRet = pFormatter->GetChar(c);
			if(nRet)
			{
				if(bNeg)
					pFormatter->UnGetChar('-');
				else
					pFormatter->UnGetChar('+');
				return nRet;
			}
			if(c == '\0')
			{
				pFormatter->UnGetChar(c);
				if(bNeg)
					pFormatter->UnGetChar('-');
				else
					pFormatter->UnGetChar('+');
				return -2;
			}
			if(CString::IsSpace(c))
			{
				pFormatter->UnGetChar(c);
				if(bNeg)
					pFormatter->UnGetChar('-');
				else
					pFormatter->UnGetChar('+');
				return -2;
			}
		}
	}

	if(nSystem == FOCP_FORMAT_READ_SYSTEM_DEFAULT)
	{
		if(c >= '1' && c <= '9')
			nSystem = FOCP_FORMAT_READ_SYSTEM_10;
		else if(c == '0')
		{
			nSystem = FOCP_FORMAT_READ_SYSTEM_8;
			if(pFormatter->GetChar(c))
				c = '0';
			else
			{
				if(c == 'x' || c == 'X')
				{
					nSystem = FOCP_FORMAT_READ_SYSTEM_16;
					nRet = pFormatter->GetChar(c);
					if(nRet)
						return nRet;
				}
				else
				{
					pFormatter->UnGetChar(c);
					c = '0';
				}
			}
		}
		else
		{
			pFormatter->UnGetChar(c);
			return -2;
		}
	}
	else if(nSystem == FOCP_FORMAT_READ_SYSTEM_16)
	{
		if(c == '0')
		{
			if(pFormatter->GetChar(c))
				c = '0';
			else if(c == 'x' || c == 'X')
			{
				nRet = pFormatter->GetChar(c);
				if(nRet)
					return nRet;
			}
			else
			{
				pFormatter->UnGetChar(c);
				c = '0';
			}
		}
//		else
//		{
//			pFormatter->UnGetChar(c);
//			return -2;
//		}
	}

	nLen = 0;
	nData = 0;
	do
	{
		if(nWidth > 0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(!FormatGetDigit(nSystem, c, nDigit))
		{
			if(nLen == 0)
				nRet = -2;
			pFormatter->UnGetChar(c);
			break;
		}
		nData2 = nData * nSystem + nDigit;
		if(nData2 < nData)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(!CheckMaxInteger(bMeetSign, bNeg, nBits, nData2, nSignMode))
		{
			pFormatter->UnGetChar(c);
			break;
		}
		nData = nData2;
		++nLen;
	}
	while(!pFormatter->GetChar(c));

	if(bNeg)
		nData = (uint64)(-(int64)nData);

	return nRet;
}

template<uint32 nBits, uint32 nSystem=FOCP_FORMAT_READ_SYSTEM_DEFAULT, uint32 nSignMode=0> struct CFormatReadInteger
{
	static int32 ReadInteger(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
	{
		uint64 nData;

		if(nWidth == 0)
			return -2;

		int32 nRet = FormatReadIntegerX(pFormatter, nWidth, nData, nBits, nSystem, nSignMode);
		if(nRet)
			return nRet;

		if(bIgnore == false)
		{
			typedef typename FOCP_DETAIL_NAME::integer<nBits>::xuint xuint;
			xuint* pData = VaArg<xuint*>(pArgList);
			pData[0] = (xuint)nData;
		}

		return 0;
	}
};

static int32 FormatReadIntegerI8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerI16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerI32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerI64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerD8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8, FOCP_FORMAT_READ_SYSTEM_10>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerD16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16, FOCP_FORMAT_READ_SYSTEM_10>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerD32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32, FOCP_FORMAT_READ_SYSTEM_10>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerD64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64, FOCP_FORMAT_READ_SYSTEM_10>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerO8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8, FOCP_FORMAT_READ_SYSTEM_8>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerO16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16, FOCP_FORMAT_READ_SYSTEM_8>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerO32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32, FOCP_FORMAT_READ_SYSTEM_8>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerO64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64, FOCP_FORMAT_READ_SYSTEM_8>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerX8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8, FOCP_FORMAT_READ_SYSTEM_16>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerX16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16, FOCP_FORMAT_READ_SYSTEM_16>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerX32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32, FOCP_FORMAT_READ_SYSTEM_16>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerX64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64, FOCP_FORMAT_READ_SYSTEM_16>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerU8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 2>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerU16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 2>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerU32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 2>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerU64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 2>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerS8(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<8, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 1>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerS16(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<16, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 1>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerS32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<32, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 1>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadIntegerS64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return CFormatReadInteger<64, FOCP_FORMAT_READ_SYSTEM_DEFAULT, 1>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadExponentSeaial(CFormatter* pFormatter, int32 nWidth, uint64& nData, int32 &nLen)
{
	char c;
	int32 nRet;
	uint64 nDigit, nData2;

	nRet = 0;
	nData = 0;
	while(!pFormatter->GetChar(c))
	{
		if(nWidth > 0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(!FormatGetDigit(10, c, nDigit))
		{
			if(nLen == 0)
				nRet = -2;
			pFormatter->UnGetChar(c);
			break;
		}
		if(nData != FOCP_UINT64_MAX)
		{
			nData2 = nData * 10 + nDigit;
			if(nData2 < nData)
				nData = FOCP_UINT64_MAX;
			else
				nData = nData2;
		}
		++nLen;
	}

	return nRet;
}

static int32 FormatReadDecimalSeaial(CFormatter* pFormatter, int32 nWidth, double& nDecimal, int32 &nLen)
{
	char c;
	int32 nRet;
	uint64 nDigit;
	double nDiv;

	nRet = 0;
	nDecimal = 0.0;
	nDiv = 10.0;
	while(!pFormatter->GetChar(c))
	{
		if(nWidth > 0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(!FormatGetDigit(10, c, nDigit))
		{
			if(nLen == 0)
				nRet = -2;
			pFormatter->UnGetChar(c);
			break;
		}
		if(nDigit)
			nDecimal += ((int64)nDigit)/nDiv;
		nDiv *= 10.0;
		++nLen;
	}
	return nRet;
}

static int32 FormatReadIntegerSeaial(CFormatter* pFormatter, int32 nWidth, double& nInteger, int32 &nLen)
{
	char c;
	int32 nRet;
	uint64 nDigit;

	nRet = 0;
	nInteger = 0.0;
	while(!pFormatter->GetChar(c))
	{
		if(nWidth > 0 && nLen >= nWidth)
		{
			pFormatter->UnGetChar(c);
			break;
		}
		if(!FormatGetDigit(10, c, nDigit))
		{
			if(nLen == 0)
				nRet = -2;
			pFormatter->UnGetChar(c);
			break;
		}
		nInteger = nInteger * 10.0 + (double)(int64)nDigit;
		++nLen;
	}

	return nRet;
}

static int32 FormatReadFloatX(CFormatter* pFormatter, int32 nWidth,
							  double &nInteger, double &nDecimal, uint64 &nExponent, bool &bNeg, bool& bExpNeg, bool &bINF, bool &bNAN)
{
	//浮点数：[+|-](正浮点数|INF|NAN)
	//正浮点数：小数[指数]
	//小数：(数字序列[.[数字序列]])|(.数字序列)
	//指数：(e|E)[+|-]数字序列
	//数字序列：0~9

	char c;
	int32 nRet;
	int32 nLen;

	nInteger = 0.0;
	nDecimal = 0.0;
	nExponent = 0;

	bNeg = false;
	bExpNeg = false;
	bINF = false;
	bNAN = false;

	if(nWidth == 0)
		return -2;

	while(!(nRet = pFormatter->GetChar(c)))
	{
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			nRet = -2;
			break;
		}
		if(c == '\n')
		{
			nRet = -2;
			break;
		}
		if(!CString::IsSpace(c))
			break;
	}

	if(nRet)
		return nRet;

	if(c == '+' || c == '-')
	{
		if(c == '-')
			bNeg = true;
		nRet = pFormatter->GetChar(c);
		if(nRet)
		{
			if(bNeg)
				pFormatter->UnGetChar('-');
			else
				pFormatter->UnGetChar('+');
			return nRet;
		}
		if(c == '\0')
		{
			pFormatter->UnGetChar(c);
			if(bNeg)
				pFormatter->UnGetChar('-');
			else
				pFormatter->UnGetChar('+');
			return -2;
		}
		if(CString::IsSpace(c))
		{
			pFormatter->UnGetChar(c);
			if(bNeg)
				pFormatter->UnGetChar('-');
			else
				pFormatter->UnGetChar('+');
			return -2;
		}
	}

	nLen = 0;
	if(c == '.')
	{
		nRet = FormatReadDecimalSeaial(pFormatter, nWidth, nDecimal, nLen);
		if(nRet)
		{
			pFormatter->UnGetChar('.');
			return nRet;
		}
loop:
		bExpNeg = false;
		nRet = pFormatter->GetChar(c);
		if(nRet)
			return 0;
		if(c == 'e' || c == 'E')
		{
			if(pFormatter->GetChar(c))
				return 0;
			if(c == '+' || c == '-')
			{
				if(c == '-')
					bExpNeg = false;
				if(pFormatter->GetChar(c))
					return 0;
			}
			if(c >= '0' && c <= '9')
			{
				pFormatter->UnGetChar(c);
				if(FormatReadExponentSeaial(pFormatter, nWidth, nExponent, nLen))
					return 0;
			}
			else
				pFormatter->UnGetChar(c);
		}
		else
			pFormatter->UnGetChar(c);
		return 0;
	}
	else if(c >= '0' && c <= '9')
	{
		pFormatter->UnGetChar(c);
		nRet = FormatReadIntegerSeaial(pFormatter, nWidth, nInteger, nLen);
		if(nRet)
			return nRet;
		if(pFormatter->GetChar(c))
			return 0;
		if(c == '.')
		{
			if(FormatReadDecimalSeaial(pFormatter, nWidth, nDecimal, nLen))
			{
				pFormatter->UnGetChar('.');
				return 0;
			}
			goto loop;
		}
		else
			pFormatter->UnGetChar(c);
		return 0;
	}
	else if(c == 'i' || c == 'I')
	{
		int32 i, j;
		char s[4] = {0};
		pFormatter->UnGetChar(c);
		for(i=0; i<3; ++i)
		{
			nRet = pFormatter->GetChar(c);
			if(nRet)
				break;
			s[i] = c;
		}
		if(CString::StringCompare(s, "INF", false))
		{
			for(j=i-1; j>=0; --j)
				pFormatter->UnGetChar(s[j]);
			return -2;
		}
		bINF = true;
	}
	else if(c == 'n' || c == 'N')
	{
		int32 i, j;
		char s[4] = {0};
		pFormatter->UnGetChar(c);
		for(i=0; i<3; ++i)
		{
			nRet = pFormatter->GetChar(c);
			if(nRet)
				break;
			s[i] = c;
		}
		if(CString::StringCompare(s, "NAN", false))
		{
			for(j=i-1; j>=0; --j)
				pFormatter->UnGetChar(s[j]);
			return -2;
		}
		bNAN = true;
	}
	else
	{
		pFormatter->UnGetChar('.');
		return -2;
	}
	return 0;
}

static int32 FormatReadFloatX2(CFormatter* pFormatter, int32 nWidth, double &nFloat)
{
	uint64 nExponent;
	double nInteger, nDecimal;
	bool bNeg, bExpNeg, bINF, bNAN;

	static double g_nPostINF=1.7976931348623157e+308;
	static double g_nNegINF= -g_nPostINF;
	static uint64 g_nNAN=(uint64)(int64)(-1);

	int32 nRet = FormatReadFloatX(pFormatter, nWidth, nInteger, nDecimal, nExponent, bNeg, bExpNeg, bINF, bNAN);
	if(nRet)
		return nRet;

	if(bINF)
	{
		if(bNeg)
			nInteger = g_nNegINF;
		else
			nInteger = g_nPostINF;
	}
	else if(bNAN)
		nInteger = *(double*)&g_nNAN;
	else
	{
		nInteger += nDecimal;
		if(bExpNeg)
			nInteger /= pow(10.0, (double)(int64)nExponent);
		else
			nInteger *= pow(10.0, (double)(int64)nExponent);
	}

	nFloat = nInteger;

	return 0;
}

static int32 FormatReadFloat32(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	double nData;
	int32 nRet = FormatReadFloatX2(pFormatter, nWidth, nData);
	if(nRet)
		return nRet;
	if(bIgnore == false)
	{
		float* pData = VaArg<float*>(pArgList);
		pData[0] = (float)nData;
	}
	return 0;
}

static int32 FormatReadFloat64(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	double nData;
	int32 nRet = FormatReadFloatX2(pFormatter, nWidth, nData);
	if(nRet)
		return nRet;
	if(bIgnore == false)
	{
		double* pData = VaArg<double*>(pArgList);
		pData[0] = nData;
	}
	return 0;
}

static int32 FormatReadAddress(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	enum {nBits = 8*sizeof(void*)};
	return CFormatReadInteger<nBits, FOCP_FORMAT_READ_SYSTEM_16, 2>::ReadInteger(pFormatter, bIgnore, nWidth, pArgList);
}

static int32 FormatReadCounter(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	if(bIgnore == false)
	{
		uint32 * pData = VaArg<uint32*>(pArgList);
		pData[0] = pFormatter->GetReadCount();
	}
	return 0;
}

static int32 FormatReadBinaryX(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList, bool bNeedAlign, bool bNetworkByte)
{
	int32 i, j, nRet;
	char c[8], *pData;
	switch(nWidth)
	{
	default:
		nWidth = 4;
	case 1:
	case 2:
	case 4:
	case 8:
		break;
	}
	if(bNeedAlign)
	{
		int32 nPos = pFormatter->GetAlignPos();
		int32 nMod = nPos % nWidth;
		if(nMod)
			nMod = nWidth - nMod;
		for(i=0; i<nMod; ++i)
		{
			nRet = pFormatter->GetChar(c[i]);
			if(nRet)
			{
				for(j=i-1; j>=0; --j)
					pFormatter->UnGetChar(c[j]);
				return nRet;
			}

		}
	}
	for(i=0; i<nWidth; ++i)
	{
		nRet = pFormatter->GetChar(c[i]);
		if(nRet)
		{
			for(j=i-1; j>=0; --j)
				pFormatter->UnGetChar(c[j]);
			return nRet;
		}
	}
	if(bIgnore == false)
	{
		pData = VaArg<char*>(pArgList);
		for(i=0; i<nWidth; ++i)
			pData[i] = c[i];
		switch(nWidth)
		{
		case 1:
			break;
		case 2:
			*(uint16*)pData = CBinary::U16Code(*(uint16*)pData);
			break;
		case 4:
			*(uint32*)pData = CBinary::U32Code(*(uint32*)pData);
			break;
		case 8:
			*(uint64*)pData = CBinary::U64Code(*(uint64*)pData);
			break;
		}
	}
	return 0;
}

static int32 FormatReadBinary(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return FormatReadBinaryX(pFormatter, bIgnore, nWidth, pArgList, false, false);
}

static int32 FormatReadBinaryNetwork(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return FormatReadBinaryX(pFormatter, bIgnore, nWidth, pArgList, false, true);
}

static int32 FormatReadBinaryAlign(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return FormatReadBinaryX(pFormatter, bIgnore, nWidth, pArgList, true, false);
}

static int32 FormatReadBinaryAlignNetwork(CFormatter* pFormatter, bool bIgnore, int32 nWidth, CVaList& pArgList)
{
	return FormatReadBinaryX(pFormatter, bIgnore, nWidth, pArgList, true, true);
}

template<uint32 nBits> struct CGetWriteDataItem
{
	static uint64 GetData(uint32 nSignMode, CVaList& pArgList, bool &bNeg)
	{
		typedef typename FOCP_DETAIL_NAME::integer<nBits>::xint xint;
		typedef typename FOCP_DETAIL_NAME::integer<nBits>::xuint xuint;
		xuint xData = VaArg<xuint>(pArgList);
		uint64 nData = (uint64)xData;
		if(nSignMode)
		{
			if((xint)0 > (xint)xData)
			{
				nData = (uint64)(xuint)((xint)0-(xint)xData);
				bNeg = true;
			}
		}
		return nData;
	}
};

#ifdef FOCP_GCC
template<> struct CGetWriteDataItem<8>
{
	static uint64 GetData(uint32 nSignMode, CVaList& pArgList, bool &bNeg)
	{
		typedef FOCP_DETAIL_NAME::integer<8>::xint xint;
		typedef FOCP_DETAIL_NAME::integer<8>::xuint xuint;
		uint64 nData = (uint64)VaArg<uint32>(pArgList);
		if(nSignMode)
		{
			xint x = (xint)nData;
			if(x <(xint)0)
			{
				nData = (uint64)(xuint)(0-x);
				bNeg = true;
			}
		}
		return nData;
	}
};
template<> struct CGetWriteDataItem<16>
{
	static uint64 GetData(uint32 nSignMode, CVaList& pArgList, bool &bNeg)
	{
		typedef FOCP_DETAIL_NAME::integer<16>::xint xint;
		typedef FOCP_DETAIL_NAME::integer<16>::xuint xuint;
		uint64 nData = (uint64)VaArg<uint32>(pArgList);
		if(nSignMode)
		{
			xint x = (xint)nData;
			if(x <(xint)0)
			{
				nData = (uint64)(xuint)(0-x);
				bNeg = true;
			}
		}
		return nData;
	}
};
#endif

static int32 FormatWriteHelp(CFormatter* pFormatter, const char* sStr, int32 nWidth)
{
	int32 nRet;
	for(nRet=0; nRet<nWidth; ++nRet)
	{
		int32 i = pFormatter->PutChar(sStr[nRet]);
		if(i)
		{
			if(i == -1 && nRet == 0)
				nRet = -1;
			break;
		}
	}
	return nRet;
}

static int32 FormatWriteIntegerX(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList, uint32 nBits, uint32 nSystem, uint32 nSignMode=0, bool bBigHex=false)
{
	uint64 nData;
	uint8 nDigit;
	int32 nMinWidth;
	bool bNeg = false;
	CVector<char> oOut;
	oOut.SetCapacity(64);
	switch(nBits)
	{
	case 8:
		nData = CGetWriteDataItem<8>::GetData(nSignMode, pArgList, bNeg);
		break;
	case 16:
		nData = CGetWriteDataItem<16>::GetData(nSignMode, pArgList, bNeg);
		break;
	case 32:
		nData = CGetWriteDataItem<32>::GetData(nSignMode, pArgList, bNeg);
		break;
	case 64:
		nData = CGetWriteDataItem<64>::GetData(nSignMode, pArgList, bNeg);
		break;
	}
	if(bNeg)
		oOut.Insert(0, '-');
	switch(nSystem)
	{
	case FOCP_FORMAT_READ_SYSTEM_8:
		if(!nData)
			oOut.Insert(0, '0');
		else while(nData)
			{
				nDigit = (uint8)(nData & ((uint64)7)) + '0';
				oOut.Insert(bNeg?1:0, (char)nDigit);
				nData >>= 3;
			}
		break;
	case FOCP_FORMAT_READ_SYSTEM_16:
		if(!nData)
			oOut.Insert(0, '0');
		else while(nData)
			{
				nDigit = (uint8)(nData & ((uint64)15));
				if(nDigit < 10)
					nDigit += '0';
				else if(bBigHex)
					nDigit += 'A' - 10;
				else
					nDigit += 'a' - 10;
				oOut.Insert(bNeg?1:0, (char)nDigit);
				nData >>= 4;
			}
		break;
	case FOCP_FORMAT_READ_SYSTEM_10:
		if(!nData)
			oOut.Insert(0, '0');
		else while(nData)
			{
				nDigit = (uint8)(nData % ((uint64)10)) + '0';
				oOut.Insert(bNeg?1:0, (char)nDigit);
				nData /= 10;
			}
		break;
	}
	if(nFlags & FOCP_FORMATER_PLUS_FLAG)
	{
		if(nSignMode && (!bNeg))
			oOut.Insert(0, '+');
	}
	if(nFlags & FOCP_FORMATER_SPACE_FLAG)
	{
		if(!bNeg)
			oOut.Insert(0, ' ');
	}
	if(nFlags & FOCP_FORMATER_SHARP_FLAG)
	{
		uint32 nIdx = 0;
		char c = oOut[0];
		if(c == '+' || c == '-' || c == ' ')
			nIdx = 1;
		switch(nSystem)
		{
		case FOCP_FORMAT_READ_SYSTEM_8:
			if(oOut[nIdx] != '0')
				oOut.Insert(nIdx, '0');
			break;
		case FOCP_FORMAT_READ_SYSTEM_16:
			if(bBigHex)
				oOut.Insert(nIdx, "0X", 2);
			else
				oOut.Insert(nIdx, "0x", 2);
			break;
		}
	}
	nMinWidth = oOut.GetSize();
	if(nPrecision > 0 && nWidth > nPrecision)
		nWidth = nPrecision;
	if(nWidth > nMinWidth)
	{
		if(nFlags & FOCP_FORMATER_ZERO_FLAG)
			oOut.Insert(0, '0', nWidth-nMinWidth);
		else if(nFlags & FOCP_FORMATER_MINUS_FLAG)
			oOut.Insert((uint32)(-1), ' ', nWidth-nMinWidth);
		else
			oOut.Insert(0, ' ', nWidth-nMinWidth);
	}
	else
		nWidth = nMinWidth;
	return FormatWriteHelp(pFormatter, oOut.At(0), nWidth);
}

static int32 FormatWriteIntegerD8(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 8, FOCP_FORMAT_READ_SYSTEM_10, 1);
}

static int32 FormatWriteIntegerD16(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 16, FOCP_FORMAT_READ_SYSTEM_10, 1);
}

static int32 FormatWriteIntegerD32(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_10, 1);
}

static int32 FormatWriteIntegerD64(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_10, 1);
}

static int32 FormatWriteIntegerO8(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 8, FOCP_FORMAT_READ_SYSTEM_8);
}

static int32 FormatWriteIntegerO16(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 16, FOCP_FORMAT_READ_SYSTEM_8);
}

static int32 FormatWriteIntegerO32(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_8);
}

static int32 FormatWriteIntegerO64(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_8);
}

static int32 FormatWriteIntegerX8(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 8, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
}

static int32 FormatWriteIntegerX16(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 16, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
}

static int32 FormatWriteIntegerX32(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
}

static int32 FormatWriteIntegerX64(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
}

static int32 FormatWriteIntegerx8(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 8, FOCP_FORMAT_READ_SYSTEM_16);
}

static int32 FormatWriteIntegerx16(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 16, FOCP_FORMAT_READ_SYSTEM_16);
}

static int32 FormatWriteIntegerx32(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_16);
}

static int32 FormatWriteIntegerx64(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_16);
}

static int32 FormatWriteIntegerU8(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 8, FOCP_FORMAT_READ_SYSTEM_10);
}

static int32 FormatWriteIntegerU16(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 16, FOCP_FORMAT_READ_SYSTEM_10);
}

static int32 FormatWriteIntegerU32(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_10);
}

static int32 FormatWriteIntegerU64(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_10);
}

static int32 FormatWriteBigAddress(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	nFlags &= ~(uint32)FOCP_FORMATER_MINUS_FLAG;
	nFlags &= ~(uint32)FOCP_FORMATER_PLUS_FLAG;
	nFlags |= FOCP_FORMATER_ZERO_FLAG;
	if(sizeof(void*) == sizeof(uint32))
		return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
	else
		return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_16, 0, true);
}

static int32 FormatWriteSmallAddress(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	nFlags &= ~(uint32)FOCP_FORMATER_MINUS_FLAG;
	nFlags &= ~(uint32)FOCP_FORMATER_PLUS_FLAG;
	nFlags |= FOCP_FORMATER_ZERO_FLAG;
	if(sizeof(void*) == sizeof(uint32))
		return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 32, FOCP_FORMAT_READ_SYSTEM_16);
	else
		return FormatWriteIntegerX(pFormatter, nFlags, nWidth, nPrecision, pArgList, 64, FOCP_FORMAT_READ_SYSTEM_16);
}

static int32 FormatWriteCounter(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	uint32 * pData = VaArg<uint32*>(pArgList);
	pData[0] = pFormatter->GetWriteCount();
	return 0;
}

static int32 FormatWriteChar(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
#ifdef FOCP_GCC
	char c = (char)VaArg<int32>(pArgList);
#else
	char c = VaArg<char>(pArgList);
#endif
	CVector<char> oOut;
	oOut.SetCapacity(32);
	oOut.Insert(0, c);

	if(nPrecision < 0)
		nPrecision = 0;
	if(nWidth > nPrecision)
		nWidth = nPrecision;
	if(nFlags & FOCP_FORMATER_SPACE_FLAG)
		oOut.Insert(0, ' ');
	int32 nMinWidth = oOut.GetSize();
	if(nMinWidth < nWidth)
	{
		if(nFlags & FOCP_FORMATER_MINUS_FLAG)
			oOut.Insert((uint32)(-1), ' ', nWidth-nMinWidth);
		else
			oOut.Insert(0, ' ', nWidth-nMinWidth);
	}
	else
		nWidth = nMinWidth;
	if(c == '\0')
		++nWidth;
	return FormatWriteHelp(pFormatter, oOut.At(0), nWidth);
}

static int32 FormatWriteString(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	const char* s = VaArg<const char*>(pArgList);
	CVector<char> oOut;
	oOut.SetCapacity(256);
	oOut.Insert(0, s, CString::StringLength(s));

	if(nPrecision < 0)
		nPrecision = 0;
	if(nWidth > nPrecision)
		nWidth = nPrecision;

	if(nFlags & FOCP_FORMATER_SPACE_FLAG)
		oOut.Insert(0, ' ');
	int32 nMinWidth = oOut.GetSize();
	if(nMinWidth < nWidth)
	{
		if(nFlags & FOCP_FORMATER_MINUS_FLAG)
			oOut.Insert((uint32)(-1), ' ', nWidth-nMinWidth);
		else
			oOut.Insert(0, ' ', nWidth-nMinWidth);
	}
	else
		nWidth = nMinWidth;
	return FormatWriteHelp(pFormatter, oOut.At(0), nWidth);
}

#define FOCP_FLOAT_CONVERT_f 0
#define FOCP_FLOAT_CONVERT_e 1
#define FOCP_FLOAT_CONVERT_E 2
#define FOCP_FLOAT_CONVERT_g 4
#define FOCP_FLOAT_CONVERT_G 8

static int32 GetFloatPrecision(double nData)
{
	double nInteger = floor(nData);
	double nDecimal = nData - nInteger;
	int32 nPrecision=0;
	nData = nDecimal;
	while(nData)
	{
		++nPrecision;
		nData *= 10.0;
		nInteger = floor(nData);
		nDecimal = nData - nInteger;
		nData = nDecimal;
	}
	if(nPrecision == 0)
		nPrecision = 1;
	return nPrecision;
}

static void WriteFloatIntegerPart(double nInteger, CVector<char> &oOut)
{
	uint32 nIdx = oOut.GetSize();
	uint64 x = (uint64)(int64)nInteger;
	if(!x)
		oOut.Insert(nIdx, '0');
	else
	{
		uint8 nDigit = (uint8)(x % ((uint64)10)) + '0';
		oOut.Insert(nIdx, (char)nDigit);
		x /= 10;
	}
}

static void WriteFloatDecimalPart(double nDecimal, int32 nPrecision, bool bForceAddPoint, CVector<char> &oOut)
{
	bool AddPoint = true;
	if(nPrecision == 0)
		AddPoint = bForceAddPoint;
	if(AddPoint)
	{
		oOut.Insert((uint32)(-1), '.');
		int32 nLen = 0;
		while(nDecimal != 0.0)
		{
			++nLen;
			nDecimal *= 10.0;
			double nInteger = floor(nDecimal);
			nDecimal -= nInteger;
			uint8 nDigit = (uint8)(uint64)nInteger + '0';
			oOut.Insert((uint32)(-1), (char)nDigit);
			if(nLen >= nPrecision)
				break;
		}
		if(nLen < nPrecision)
		{
			if(!bForceAddPoint)
				oOut.Insert((uint32)(-1), '0', nPrecision - nLen);
			else if(nLen == 0)
				oOut.Insert((uint32)(-1), '0');
		}
	}
}

static void WriteFloatExponentPart(int32 nExponent, char cExp, CVector<char> &oOut)
{
	oOut.Insert((uint32)(-1), cExp);
	if(nExponent < 0)
	{
		oOut.Insert((uint32)(-1), '-');
		nExponent = -nExponent;
	}
	else
		oOut.Insert((uint32)(-1), '+');
	uint32 nIdx = oOut.GetSize();
	while(nExponent)
	{
		uint8 nDigit = (uint8)(nExponent % 10) + '0';
		oOut.Insert(nIdx, (char)nDigit);
		nExponent /= 10;
	}
}

static int32 FormatWriteFloatX(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, double nData, uint32 nExpMode)
{
	CVector<char> oOut;
	oOut.SetCapacity(128);
	int32 nMinWidth;

#ifdef MSVC
	if(_isnan(nData))
#else
	if(isnan(nData))
#endif
		oOut.Insert(0, "NAN", 3);
#ifdef MSVC
	else if(!_finite(nData))
#else
	else if(!finite(nData))
#endif
	{
		if(nData < 0.0)
			oOut.Insert((uint32)(-1), "-INF", 4);
		else
			oOut.Insert((uint32)(-1), "INF", 3);
		if(nFlags & FOCP_FORMATER_PLUS_FLAG)
		{
			if(nData >= 0.0)
				oOut.Insert(0, '+');
		}
	}
	else
	{
		int32 nExponent;
		int32 nPrecision2;
		double nInteger, nDecimal;
		if(nData < 0.0)
		{
			nData = -nData;
			oOut.Insert((uint32)(-1), '-');
		}
		if(nPrecision < 0)
			nPrecision = 6;
		if(nExpMode == FOCP_FLOAT_CONVERT_f)
		{
			if(nPrecision == 0)
			{
				nInteger = ceil(nData);
				nDecimal = nData - nInteger;
				if(nDecimal + 0.5 >= 1.0)
					nInteger += 1.0;
				nData = nInteger;
			}
		}
		else if(nExpMode == FOCP_FLOAT_CONVERT_e ||
				nExpMode == FOCP_FLOAT_CONVERT_E)
		{
			nExponent = 0;
			while(nData >= 10.0)
			{
				++nExponent;
				nData /= 10.0;
			}
			if(nData != 0.0)while(nData < 1.0)
				{
					--nExponent;
					nData *= 10.0;
				}
			if(nPrecision == 0)
			{
				nInteger = ceil(nData);
				nDecimal = nData - nInteger;
				if(nDecimal + 0.5 >= 1.0)
					nInteger += 1.0;
				nData = nInteger;
			}
		}
		else
		{
			if(nPrecision == 0)
				nPrecision = 1;
			double nData2 = nData;
			nExponent = 0;
			while(nData2 > 10.0)
			{
				++nExponent;
				nData2 /= 10.0;
			}
			if(nData2 != 0.0)while(nData2 < 1.0)
				{
					--nExponent;
					nData2 *= 10.0;
				}
			if(nExponent < -4)
			{
				nData = nData2;
				nExpMode = (nExpMode == FOCP_FLOAT_CONVERT_g)?
						   FOCP_FLOAT_CONVERT_e:FOCP_FLOAT_CONVERT_E;
			}
			else
			{
				nPrecision2 = GetFloatPrecision(nData);
				if(nPrecision2 > nPrecision)
				{
					nData = nData2;
					nExpMode = (nExpMode == FOCP_FLOAT_CONVERT_g)?
							   FOCP_FLOAT_CONVERT_e:FOCP_FLOAT_CONVERT_E;
				}
			}
		}
		nInteger = floor(nData);
		nDecimal = nData - nInteger;
		nPrecision2 = GetFloatPrecision(nData);
		if(nPrecision2 > nPrecision)
		{
			int32 i;
			double nDecimal2 = nDecimal;
			for(i=0; i<nPrecision; ++i)
				nDecimal2 *= 10.0;
			double nInteger2 = floor(nDecimal2);
			nDecimal2 -= nInteger2;
			if(nDecimal2 + 0.5 >= 1.0)
				nInteger2 += 1.0;
			for(i=0; i<nPrecision; ++i)
				nInteger2 /= 10.0;
			if(nInteger2 >= 1.0)
			{
				nInteger += 1.0;
				nDecimal = 0.0;
			}
		}
		bool bForceAddPoint = (nFlags&FOCP_FORMATER_SHARP_FLAG)?true:false;
		WriteFloatIntegerPart(nInteger, oOut);
		WriteFloatDecimalPart(nDecimal, nPrecision, bForceAddPoint, oOut);
		if(nExponent)
		{
			char cExp = (nExpMode == FOCP_FLOAT_CONVERT_e)?'e':'E';
			WriteFloatExponentPart(nExponent, cExp, oOut);
		}
		if(nFlags & FOCP_FORMATER_PLUS_FLAG)
		{
			if(nData >= 0.0)
				oOut.Insert(0, '+');
		}
	}
	if(nFlags & FOCP_FORMATER_SPACE_FLAG)
	{
		if(nData >= 0.0)
			oOut.Insert(0, ' ');
	}
	nMinWidth = oOut.GetSize();
	if(nWidth > nMinWidth)
	{
		if(nFlags & FOCP_FORMATER_ZERO_FLAG)
			oOut.Insert(0, '0', nWidth-nMinWidth);
		else if(nFlags & FOCP_FORMATER_MINUS_FLAG)
			oOut.Insert((uint32)(-1), ' ', nWidth-nMinWidth);
		else
			oOut.Insert(0, ' ', nWidth-nMinWidth);
	}
	else
		nWidth = nMinWidth;
	return FormatWriteHelp(pFormatter, oOut.At(0), nWidth);
}

static int32 FormatWriteFloatF(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteFloatX(pFormatter, nFlags, nWidth, nPrecision, VaArg<double>(pArgList), FOCP_FLOAT_CONVERT_f);
}

static int32 FormatWriteFloate(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteFloatX(pFormatter, nFlags, nWidth, nPrecision, VaArg<double>(pArgList), FOCP_FLOAT_CONVERT_e);
}

static int32 FormatWriteFloatE(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteFloatX(pFormatter, nFlags, nWidth, nPrecision, VaArg<double>(pArgList), FOCP_FLOAT_CONVERT_E);
}

static int32 FormatWriteFloatg(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteFloatX(pFormatter, nFlags, nWidth, nPrecision, VaArg<double>(pArgList), FOCP_FLOAT_CONVERT_g);
}

static int32 FormatWriteFloatG(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteFloatX(pFormatter, nFlags, nWidth, nPrecision, VaArg<double>(pArgList), FOCP_FLOAT_CONVERT_G);
}

static int32 FormatWriteBinaryX(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList, bool bNeedAlign, bool bNetworkByte)
{
	uint64 l;
	uint16 x;
	char *s, c;
	int32 i, nRet, d;
	switch(nWidth)
	{
	default:
		nWidth = 4;
	case 4:
	{
		d = VaArg<uint32>(pArgList);
		if(bNetworkByte)
			d = CBinary::U32Code(d);
		s = (char*)&d;
	}
	break;
	case 1:
	{
#ifdef FOCP_GCC
		c = (char)VaArg<int32>(pArgList);
#else
		c = VaArg<char>(pArgList);
#endif
		s = &c;
	}
	break;
	case 2:
	{
#ifdef FOCP_GCC
		x = (uint16)VaArg<uint32>(pArgList);
#else
		x = VaArg<uint16>(pArgList);
#endif
		if(bNetworkByte)
			x = CBinary::U16Code(x);
		s = (char*)&x;
	}
	break;
	case 8:
	{
		l = VaArg<uint64>(pArgList);
		if(bNetworkByte)
			l = CBinary::U64Code(l);
		s = (char*)&l;
	}
	break;
	}
	if(bNeedAlign)
	{
		int32 nPos = pFormatter->GetAlignPos();
		int32 nMod = nPos % nWidth;
		if(nMod)
			nMod = nWidth - nMod;
		for(i=0; i<nMod; ++i)
		{
			char c = 0;
			nRet = pFormatter->PutChar(c);
			if(nRet)
			{
				if(nRet == -2)
					nRet = 0;
				return nRet;
			}
		}
	}
	return FormatWriteHelp(pFormatter, s, nWidth);
}

static int32 FormatWriteBinary(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteBinaryX(pFormatter, nFlags, nWidth, nPrecision, pArgList, false, false);
}

static int32 FormatWriteBinaryNetwork(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteBinaryX(pFormatter, nFlags, nWidth, nPrecision, pArgList, false, true);
}

static int32 FormatWriteBinaryAlign(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteBinaryX(pFormatter, nFlags, nWidth, nPrecision, pArgList, true, false);
}

static int32 FormatWriteBinaryAlignNetwork(CFormatter* pFormatter, uint32 nFlags, int32 nWidth, int32 nPrecision, CVaList& pArgList)
{
	return FormatWriteBinaryX(pFormatter, nFlags, nWidth, nPrecision, pArgList, true, true);
}

//格式化框架
CFormatterMethod::CFormatterMethod(bool bDefault)
{
	if(bDefault)
	{
		SetReader("rr", FormatReadMultiLine);
		SetReader("cc", FormatReadCChar);
		SetReader("cs", FormatReadCString);
		SetReader("i8", FormatReadIntegerI8);
		SetReader("i16", FormatReadIntegerI16);
		SetReader("i32", FormatReadIntegerI32);
		SetReader("i64", FormatReadIntegerI64);
		SetReader("i", FormatReadIntegerI32);
		SetReader("d8", FormatReadIntegerD8);
		SetReader("d16", FormatReadIntegerD16);
		SetReader("d32", FormatReadIntegerD32);
		SetReader("d64", FormatReadIntegerD64);
		SetReader("d", FormatReadIntegerD32);
		SetReader("o8", FormatReadIntegerO8);
		SetReader("o16", FormatReadIntegerO16);
		SetReader("o32", FormatReadIntegerO32);
		SetReader("o64", FormatReadIntegerO64);
		SetReader("o", FormatReadIntegerO32);
		SetReader("x8", FormatReadIntegerX8);
		SetReader("x16", FormatReadIntegerX16);
		SetReader("x32", FormatReadIntegerX32);
		SetReader("x64", FormatReadIntegerX64);
		SetReader("x", FormatReadIntegerX32);
		SetReader("u8", FormatReadIntegerU8);
		SetReader("u16", FormatReadIntegerU16);
		SetReader("u32", FormatReadIntegerU32);
		SetReader("u64", FormatReadIntegerU64);
		SetReader("u", FormatReadIntegerU32);
		SetReader("t8", FormatReadIntegerS8);
		SetReader("t16", FormatReadIntegerS16);
		SetReader("t32", FormatReadIntegerS32);
		SetReader("t64", FormatReadIntegerS64);
		SetReader("t", FormatReadIntegerS32);
		SetReader("f32", FormatReadFloat32);
		SetReader("f64", FormatReadFloat64);
		SetReader("f", FormatReadFloat64);
		SetReader("p", FormatReadAddress);
		SetReader("n", FormatReadCounter);
		SetReader("b", FormatReadBinary);
		SetReader("bn", FormatReadBinaryNetwork);
		SetReader("ba", FormatReadBinaryAlign);
		SetReader("ban", FormatReadBinaryAlignNetwork);
		SetReader("c", FormatReadChar);
		SetReader("s", FormatReadString);
		SetReader("r", FormatReadLine);
		SetReader("k", FormatReadIdentifier);

		SetWriter("d8", FormatWriteIntegerD8);
		SetWriter("d16", FormatWriteIntegerD16);
		SetWriter("d32", FormatWriteIntegerD32);
		SetWriter("d64", FormatWriteIntegerD64);
		SetWriter("d", FormatWriteIntegerD32);
		SetWriter("o8", FormatWriteIntegerO8);
		SetWriter("o16", FormatWriteIntegerO16);
		SetWriter("o32", FormatWriteIntegerO32);
		SetWriter("o64", FormatWriteIntegerO64);
		SetWriter("o", FormatWriteIntegerO32);
		SetWriter("X8", FormatWriteIntegerX8);
		SetWriter("X16", FormatWriteIntegerX16);
		SetWriter("X32", FormatWriteIntegerX32);
		SetWriter("X64", FormatWriteIntegerX64);
		SetWriter("X", FormatWriteIntegerX32);
		SetWriter("x8", FormatWriteIntegerx8);
		SetWriter("x16", FormatWriteIntegerx16);
		SetWriter("x32", FormatWriteIntegerx32);
		SetWriter("x64", FormatWriteIntegerx64);
		SetWriter("x", FormatWriteIntegerx32);
		SetWriter("u8", FormatWriteIntegerU8);
		SetWriter("u16", FormatWriteIntegerU16);
		SetWriter("u32", FormatWriteIntegerU32);
		SetWriter("u64", FormatWriteIntegerU64);
		SetWriter("u", FormatWriteIntegerU32);
		SetWriter("p", FormatWriteSmallAddress);
		SetWriter("P", FormatWriteBigAddress);
		SetWriter("n", FormatWriteCounter);
		SetWriter("c", FormatWriteChar);
		SetWriter("s", FormatWriteString);
		SetWriter("f", FormatWriteFloatF);
		SetWriter("e", FormatWriteFloate);
		SetWriter("E", FormatWriteFloatE);
		SetWriter("g", FormatWriteFloatg);
		SetWriter("G", FormatWriteFloatG);
		SetWriter("b", FormatWriteBinary);
		SetWriter("bn", FormatWriteBinaryNetwork);
		SetWriter("ba", FormatWriteBinaryAlign);
		SetWriter("ban", FormatWriteBinaryAlignNetwork);
	}
}

CFormatterMethod::~CFormatterMethod()
{
}

bool CFormatterMethod::SetReader(const char* sType, FFormatRead fRead)
{
	if(!sType || !sType[0] || !fRead)
		return false;

	CRbTreeNode* pNode = m_oReadTable.Find(sType);
	if(pNode != m_oReadTable.End())
		return false;

	m_oReadTable[sType] = fRead;
	return true;
}

bool CFormatterMethod::SetWriter(const char* sType, FFormatWrite fWrite)
{
	if(!sType || !sType[0] || !fWrite)
		return false;

	CRbTreeNode* pNode = m_oWriteTable.Find(sType);
	if(pNode != m_oWriteTable.End())
		return false;

	m_oWriteTable[sType] = fWrite;
	return true;
}

CFormatter::CFormatter(CFormatterMethod* pMethod)
{
	if(pMethod == NULL)
		pMethod = CSingleInstance<CFormatterMethod>::GetInstance();
	m_pMehod = pMethod;
	m_nAlignPos = 0;
	m_pReadBuffer = NULL;
	m_nReadOffset = 0;
	m_bLineBuf = true;
}

CFormatter::~CFormatter()
{
	while(m_pReadBuffer)
	{
		CReadBuffer* pNext = m_pReadBuffer->pNext;
		delete m_pReadBuffer;
		m_pReadBuffer = pNext;
	}
}

void CFormatter::SetLineBuf(bool bLine)
{
	m_bLineBuf = bLine;
}

void CFormatter::SetAlignPos(int32 nAlignPos)
{
	if(nAlignPos < 0)
		nAlignPos = 0;
	m_nAlignPos = nAlignPos;
}

int32 CFormatter::GetAlignPos()
{
	return m_nAlignPos;
}

int32 CFormatter::GetChar(char &nChar)
{
	if(m_pReadBuffer && m_nReadOffset<128)
	{
		nChar = m_pReadBuffer->pBuffer[m_nReadOffset];
		++m_nReadOffset;
		if(m_nReadOffset == 128)
		{
			CReadBuffer* pNext = m_pReadBuffer->pNext;
			delete m_pReadBuffer;
			m_pReadBuffer = pNext;
			if(m_pReadBuffer)
				m_nReadOffset = 128;
			else
				m_nReadOffset = 0;
		}
		++m_nAlignPos;
		++m_nReadCount;
		return 0;
	}
	int32 nRet = ReadChar(nChar);
	if(nRet == 0)
	{
		++m_nReadCount;
		++m_nAlignPos;
	}
	return nRet;
}

void CFormatter::UnGetChar(char nChar)
{
	if(m_nReadOffset==0)
	{
		CReadBuffer* pBuffer = new CReadBuffer;
		pBuffer->pNext = m_pReadBuffer;
		m_pReadBuffer = pBuffer;
		m_nReadOffset = 128;
	}
	--m_nReadOffset;
	m_pReadBuffer->pBuffer[m_nReadOffset] = nChar;
	--m_nReadCount;
	--m_nAlignPos;
}

int32 CFormatter::PutChar(char nChar)
{
	int32 nRet = WriteChar(nChar);
	if(nRet == 0)
	{
		++m_nWriteCount;
		++m_nAlignPos;
		if(nChar == '\n' && m_bLineBuf)
			Flush();
	}
	return nRet;
}

int32 CFormatter::Scan(const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = ScanV(sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

int32 CFormatter::Print(const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = PrintV(sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

int32 CFormatter::ReadSet(int32 nWidth, const CString &oSet, CString &oStr)
{
	char c;
	int32 nRet;
	int32 nLen;
	const char* sSet = oSet.GetStr();

	if(nWidth == 0)
		return -2;

	nLen = 0;
	while(true)
	{
		nRet = GetChar(c);
		if(nRet)
		{
			if(nLen)
				nRet = 0;
			break;
		}
		else
		{
			if(c == '\0')
			{
				UnGetChar(c);
				if(nLen == 0)
					nRet = -2;
				break;
			}
			if(!CString::CharOfString(sSet, c))
			{
				UnGetChar(c);
				if(nLen == 0)
					nRet = -2;
				break;
			}
			if(nWidth > 0 && nLen >= nWidth)
			{
				UnGetChar(c);
				break;
			}
			oStr += c;
			++nLen;
		}
	}

	return nRet;
}

int32 CFormatter::ScanV(const char* sFormat, CVaList& pArgList)
{
	int32 nRead, nRet = 0;

	m_nReadCount = 0;

	if(sFormat == NULL)
		return -3;

	char c;
	bool bForbidPercent = false;
	bool bReadValid = false;
	for(; sFormat[0]; ++sFormat)
	{
		if(CString::IsSpace(sFormat[0]))
		{
			if(bReadValid == false)
			{
				while(true)
				{
					nRead = GetChar(c);
					if(nRead == 0)
					{
						if(!CString::IsSpace(c))
						{
							bReadValid = true;
							break;
						}
					}
					else if(nRead == -1)
					{
						if(nRet == 0)
							nRet = -1;
						break;
					}
					else
						break;
				}
				if(nRead)
					break;
			}
		}
		else if(sFormat[0] == '%' && bForbidPercent==false)
		{
			bool bIgnore;
			int32 nWidth;
			uint32 nFormatLen;
			CString oSet;
			nWidth=-1;
			FFormatRead ReadMethod = GetReadInterface(sFormat+1, bIgnore, nWidth, nFormatLen, oSet);
			if(ReadMethod)
			{
				sFormat += nFormatLen;
				if(bReadValid)
				{
					bReadValid = false;
					UnGetChar(c);
				}
				int32 nRead2 = ReadMethod(this, bIgnore, nWidth, pArgList);
				if(nRead2 == 0)
				{
					if(bIgnore == false)
						++nRet;
				}
				else if(nRead2 == -1)
				{
					if(nRet == 0)
						nRet = -1;
				}
				else
					break;
			}
			else if(oSet.GetSize())
			{
				CString oStr;
				sFormat += nFormatLen;
				if(bReadValid)
				{
					bReadValid = false;
					UnGetChar(c);
				}
				int32 nRead2 = ReadSet(nWidth, oSet, oStr);
				if(nRead2 == 0)
				{
					if(bIgnore == false)
					{
						CString* pStr = VaArg<CString*>(pArgList);
						pStr->Swap(oStr);
						++nRet;
					}
				}
				else if(nRead2 == -1)
				{
					if(nRet == 0)
						nRet = -1;
				}
				else
					break;
			}
			else
				bForbidPercent = true;
		}
		else
		{
			bForbidPercent = false;
			if(bReadValid == false)
				nRead = GetChar(c);
			if(nRead == 0)
			{
				if(c != sFormat[0])
				{
					bReadValid = true;
					break;
				}
				bReadValid = false;
			}
			else if(nRead == -1)
			{
				if(nRet == 0)
					nRet = -1;
				break;
			}
			else
				break;
		}
	}
	if(bReadValid)
		UnGetChar(c);

	while(m_pReadBuffer && m_nReadOffset<128)
	{
		c = m_pReadBuffer->pBuffer[m_nReadOffset];
		if(c != '\r' && c != '\n')
			break;
		++m_nReadOffset;
		if(m_nReadOffset == 128)
		{
			CReadBuffer* pNext = m_pReadBuffer->pNext;
			delete m_pReadBuffer;
			m_pReadBuffer = pNext;
			if(m_pReadBuffer)
				m_nReadOffset = 128;
			else
				m_nReadOffset = 0;
		}
		++m_nAlignPos;
		++m_nReadCount;
	}

	return nRet;
}

int32 CFormatter::PrintV(const char* sFormat, CVaList& pArgList)
{
	m_nWriteCount = 0;
	m_nFlushCount = 0;
	m_nSuccess = 1;

	if(sFormat == NULL)
		return -3;

	bool bForbidPercent = false;
	for(; sFormat[0]; ++sFormat)
	{
		if(sFormat[0] == '%' && bForbidPercent==false)
		{
			int32 nWidth, nPrecision;
			uint32 nFlags, nFormatLen;
			nWidth = 0;
			nPrecision = -2;//default precision
			FFormatWrite WriteMethod = GetWriteInterface(sFormat+1, nFlags, nWidth, nPrecision, nFormatLen);
			if(WriteMethod)
			{
				sFormat += nFormatLen;
				if(nWidth == -1)
				{
					int32* p = VaArg<int32*>(pArgList);
					nWidth = *p;
					if(nWidth < 0)
						nWidth = 0;
				}
				if(nPrecision == -1)
				{
					int32* p = VaArg<int32*>(pArgList);
					nPrecision = *p;
				}
				if(nPrecision < 0)
					nPrecision = -1;
				WriteMethod(this, nFlags, nWidth, nPrecision, pArgList);
				if(m_nSuccess <= 0)
					break;
			}
			else
				bForbidPercent = true;
		}
		else
		{
			bForbidPercent = false;
			PutChar(sFormat[0]);
			if(m_nSuccess <= 0)
				break;
		}
	}

//	Flush();

	if(m_nFlushCount == 0 && m_nSuccess < 0)
		return -1;

	return m_nFlushCount;
}

//返回被读取的字符数量
uint32 CFormatter::GetReadCount()
{
	return m_nReadCount;
}

//返回被写入的字符数量
uint32 CFormatter::GetWriteCount()
{
	return m_nWriteCount;
}

void CFormatter::GetReadSet(const char* sFormat, uint32 &nFormatLen, CString& oSet)
{
	bool bMeetCaret = false;
	++sFormat;
	nFormatLen = 1;
	while(sFormat[0])
	{
		if(sFormat[0] == '^')
		{
			if(nFormatLen >1 )
				oSet += sFormat[0];
			else
				bMeetCaret = true;
		}
		else if(sFormat[0] == ']')
		{
			if(nFormatLen == 1 || (nFormatLen == 2 && (*(sFormat-1)=='^')))
				oSet += sFormat[0];
			else
			{
				++nFormatLen;
				++sFormat;
				break;
			}
		}
		else
			oSet += sFormat[0];
		++nFormatLen;
		++sFormat;
	}
	if(bMeetCaret && oSet.GetSize())
	{
		CString oSet2;
		const char* sStr = oSet.GetStr();
		for(uint32 i=1; i<256; ++i)
		{
			char c = (char)i;
			if(!CString::CharOfString(sStr, c))
				oSet2 += c;
		}
		oSet.Swap(oSet2);
	}
}

FFormatRead CFormatter::GetReadInterface(const char* sFormat, bool &bIgnore, int32& nWidth, uint32 &nFormatLen, CString& oSet)
{
	//%*[width]type

	uint32 nWidthLen;

	nFormatLen = 0;
	bIgnore = false;
	if(sFormat[0] == '*')
	{
		++sFormat;
		++nFormatLen;
		bIgnore = true;
	}

	ReadWidth(sFormat, nWidth, nWidthLen, false, nWidth);
	nFormatLen += nWidthLen;
	sFormat += nWidthLen;

	if(sFormat[0] == '[')
	{
		GetReadSet(sFormat, nWidthLen, oSet);
		nFormatLen += nWidthLen;
		return NULL;
	}

	FFormatRead ReadMethod = GetInterface(sFormat, m_pMehod->m_oReadTable, nWidthLen);
	if(ReadMethod)
		nFormatLen += nWidthLen;

	return ReadMethod;
}

FFormatWrite CFormatter::GetWriteInterface(const char* sFormat, uint32& nFlags, int32& nWidth, int32& nPrecision, uint32& nFormatLen)
{
	//%[flags] [width] [.precision] type
	uint32 nWidthLen;

	nFormatLen = 0;

	nFlags = 0;
	while(sFormat[0])
	{
		if(sFormat[0] == '+')
			nFlags |= FOCP_FORMATER_PLUS_FLAG;
		else if(sFormat[0] == '-')
			nFlags |= FOCP_FORMATER_MINUS_FLAG;
		else if(sFormat[0] == '0')
			nFlags |= FOCP_FORMATER_ZERO_FLAG;
		else if(sFormat[0] == '#')
			nFlags |= FOCP_FORMATER_SHARP_FLAG;
		else if(sFormat[0] == ' ')
			nFlags |= FOCP_FORMATER_SPACE_FLAG;
		else
			break;
		++sFormat;
		++nFormatLen;
	}
	if(nFlags & FOCP_FORMATER_ZERO_FLAG)
	{
		if(nFlags & FOCP_FORMATER_MINUS_FLAG)
			nFlags &= ~((uint32)FOCP_FORMATER_ZERO_FLAG);
	}
	if(nFlags & FOCP_FORMATER_SPACE_FLAG)
	{
		if(nFlags & FOCP_FORMATER_PLUS_FLAG)
			nFlags &= ~((uint32)FOCP_FORMATER_SPACE_FLAG);
	}

	ReadWidth(sFormat, nWidth, nWidthLen, true, nWidth);
	nFormatLen += nWidthLen;
	sFormat += nWidthLen;

	if(sFormat[0] == '.')
	{
		++sFormat;
		ReadWidth(sFormat, nPrecision, nWidthLen, true, -2);
		nFormatLen += nWidthLen;
		sFormat += nWidthLen;
	}

	FFormatWrite WriteMethod = GetInterface(sFormat, m_pMehod->m_oWriteTable, nWidthLen);
	if(WriteMethod)
		nFormatLen += nWidthLen;

	return WriteMethod;
}

void CFormatter::ReadWidth(const char* sFormat, int32& nWidth, uint32 &nWidthLen, bool bSupportStar, int32 nDefault)
{
	if(bSupportStar && sFormat[0] == '*')
	{
		nWidthLen = 1;
		nWidth = (uint32)-1;
		return;
	}
	if(sFormat[0] == '\0')
	{
		nWidthLen = 0;
		nWidth = nDefault;
		return;
	}
	nWidth = 0;
	nWidthLen = 0;
	while(sFormat[0])
	{
		if(sFormat[0] < '0' || sFormat[0] > '9')
		{
			if(nWidthLen == 0)
				nWidth = nDefault;
			break;
		}
		int32 nValue = nWidth * 10 + (sFormat[0] - '0');
		if(nValue < nWidth)
			break;
		nWidth = nValue;
		++sFormat;
		++nWidthLen;
	}
}

void CFormatter::Flush()
{
}

CStringFormatter::CStringFormatter(CString* pString, int32 nOffset, CFormatterMethod* pMethod)
	:CFormatter(pMethod)
{
	m_pString = pString;
	m_nPos = nOffset;
	int32 nLen = (int32)pString->GetSize();
	if(0 > nLen)
	{
		nLen = 0x7FFFFFFF;
		pString->SetSize(nLen);
	}
	if(m_nPos >= nLen)
		m_nPos = nLen;
	else if(m_nPos <= 0)
		m_nPos = 0;
	m_nAlignPos = m_nPos;
	m_bHaveCr = false;
}

CStringFormatter::~CStringFormatter()
{
}

int32 CStringFormatter::GetPos()
{
	return m_nPos;
}

int32 CStringFormatter::ReadChar(char &nChar)
{
	int32 nLen = (int32)m_pString->GetSize();
	if(m_nPos < nLen)
	{
		nChar = m_pString->GetStr()[m_nPos];
		++m_nPos;
		return 0;
	}
	m_nPos = nLen;
	m_nAlignPos = m_nPos;
	return -2;
}

int32 CStringFormatter::WriteChar(char nChar)
{
#ifdef WINDOWS
	if(nChar == '\n' && !m_bHaveCr)
		WriteChar('\r');
#endif
	if(nChar == '\r')
	{
		if(m_bHaveCr)
			return 0;
		m_bHaveCr = true;
	}
	else
		m_bHaveCr = false;
	int32 nLen = (int32)m_pString->GetSize();
	if(m_nPos < nLen)
	{
		(*m_pString)[m_nPos] = nChar;
		++m_nPos;
		++m_nFlushCount;
		if(m_nPos < nLen)
			(*m_pString)[m_nPos] = '\0';
		return 0;
	}
	m_nPos = nLen;
	m_nAlignPos = m_nPos;
	if(nLen < 0x7FFFFFFF)
	{
		m_pString->Append(nChar, 1);
		++m_nPos;
		++m_nFlushCount;
		return 0;
	}
	m_nSuccess = 0;
	return -2;
}

CBinaryFormatter::CBinaryFormatter(CBinary* pBinary, int32 nOffset, bool bText, CFormatterMethod* pMethod)
	:CFormatter(pMethod)
{
	m_pBinary = pBinary;
	m_nPos = nOffset;
	int32 nLen = (int32)pBinary->GetSize();
	if(0 > nLen)
	{
		nLen = 0x7FFFFFFF;
		pBinary->SetSize(nLen);
	}
	if(m_nPos >= nLen)
		m_nPos = nLen;
	else if(m_nPos <= 0)
		m_nPos = 0;
	m_nAlignPos = m_nPos;
	m_bText = bText;
	m_bHaveCr = false;
}

CBinaryFormatter::~CBinaryFormatter()
{
}

int32 CBinaryFormatter::GetPos()
{
	return m_nPos;
}

int32 CBinaryFormatter::ReadChar(char &nChar)
{
	int32 nLen = (int32)m_pBinary->GetSize();
	if(m_nPos < nLen)
	{
		nChar = (char)m_pBinary->GetData()[m_nPos];
		++m_nPos;
		return 0;
	}
	m_nPos = nLen;
	m_nAlignPos = m_nPos;
	return -2;
}

int32 CBinaryFormatter::WriteChar(char nChar)
{
#ifdef WINDOWS
	if(nChar == '\n' && m_bText && !m_bHaveCr)
		WriteChar('\r');
#endif
	if(m_bText)
	{
		if(nChar == '\r')
		{
			if(m_bHaveCr)
				return 0;
			m_bHaveCr = true;
		}
		else
			m_bHaveCr = false;
	}
	int32 nLen = (int32)m_pBinary->GetSize();
	if(m_nPos < nLen)
	{
		uint8* pData = m_pBinary->GetData();
		pData[m_nPos] = (uint8)nChar;
		++m_nPos;
		++m_nFlushCount;
		if(m_bText)
			m_pBinary->WriteByte(m_nPos, (uint8)'\0');
		return 0;
	}
	m_nPos = nLen;
	m_nAlignPos = m_nPos;
	if(nLen < 0x7FFFFFFF)
	{
		m_pBinary->WriteByte(m_nPos, (uint8)nChar);
		++m_nPos;
		++m_nFlushCount;
		if(m_bText)
			m_pBinary->WriteByte(m_nPos, (uint8)'\0');
		return 0;
	}
	m_nSuccess = 0;
	return -2;
}

CStreamFormatter::CStreamFormatter(CMemoryStream* pStream, int32 nOffset, bool bText, CFormatterMethod* pMethod)
	:CFormatter(pMethod)
{
	m_pStream = pStream;
	int32 nLen = (int32)m_pStream->GetSize();
	if(0 > nLen)
	{
		nLen = 0x7FFFFFFF;
		m_pStream->SetPosition(nLen);
		m_pStream->Truncate();
	}
	if(nOffset >= nLen)
		nOffset = nLen;
	else if(nOffset <= 0)
		nOffset = 0;
	pStream->SetPosition(nOffset);
	m_nAlignPos = m_pStream->GetPosition();
	m_bText = bText;
	m_bHaveCr = false;
}

CStreamFormatter::~CStreamFormatter()
{
}

int32 CStreamFormatter::GetPos()
{
	return m_pStream->GetPosition();
}

int32 CStreamFormatter::ReadChar(char &nChar)
{
	uint32 nRet = m_pStream->Read(&nChar, 1);
	if(nRet == 1)
		return 0;
	m_nAlignPos = m_pStream->GetPosition();
	return -2;
}

int32 CStreamFormatter::WriteChar(char nChar)
{
#ifdef WINDOWS
	if(nChar == '\n' && m_bText && !m_bHaveCr)
		WriteChar('\r');
#endif
	if(m_bText)
	{
		if(nChar == '\r')
		{
			if(m_bHaveCr)
				return 0;
			m_bHaveCr = true;
		}
		else
			m_bHaveCr = false;
	}
	int32 nLen = (int32)m_pStream->GetSize();
	if(nLen < 0x7FFFFFFF)
	{
		uint32 nRet = m_pStream->Write(&nChar, 1);
		if(nRet == 0)
		{
			++m_nFlushCount;
			if(m_bText)
			{
				if(m_pStream->Write((int8)'\0'))
					m_pStream->Seek(-1);
			}
			return 0;
		}
	}
	m_nAlignPos = m_pStream->GetPosition();
	m_nSuccess = 0;
	return -2;
}

CFormatString::CFormatString(CFormatterMethod* pMethod)
	:CString(), CStringFormatter((CString*)this, 0, pMethod)
{
}

CFormatString::CFormatString(char nCh, uint32 nCount, CFormatterMethod* pMethod)
	:CString(nCh, nCount), CStringFormatter((CString*)this, 0, pMethod)
{
}

CFormatString::CFormatString(const char* pStr, uint32 nCount, CFormatterMethod* pMethod)
	:CString(pStr, nCount), CStringFormatter((CString*)this, 0, pMethod)
{
}

CFormatString::CFormatString(const CString& oStr, CFormatterMethod* pMethod)
	:CString(oStr), CStringFormatter((CString*)this, 0, pMethod)
{
}

CFormatString::CFormatString(const CString& oStr, uint32 nIdx, uint32 nCount, CFormatterMethod* pMethod)
	:CString(oStr, nIdx, nCount), CStringFormatter((CString*)this, 0, pMethod)
{
}

CFormatString::~CFormatString()
{
}

CFormatBinary::CFormatBinary(bool bText, CFormatterMethod* pMethod)
	:CBinary(), CBinaryFormatter((CBinary*)this, 0, bText, pMethod)
{
}

CFormatBinary::CFormatBinary(uint32 nBufSize, bool bText, CFormatterMethod* pMethod)
	:CBinary(nBufSize), CBinaryFormatter((CBinary*)this, 0, bText, pMethod)
{
}

CFormatBinary::CFormatBinary(uint8* pData, uint32 nDataLen, bool bText, CFormatterMethod* pMethod)
	:CBinary(pData, nDataLen), CBinaryFormatter((CBinary*)this, 0, bText, pMethod)
{
}

CFormatBinary::CFormatBinary(const CBinary& oSrc, bool bText, CFormatterMethod* pMethod)
	:CBinary(oSrc), CBinaryFormatter((CBinary*)this, 0, bText, pMethod)
{
}

CFormatBinary::CFormatBinary(const CBinary& oSrc, uint32 nOff, uint32 nSize, bool bText, CFormatterMethod* pMethod)
	:CBinary(oSrc, nOff, nSize), CBinaryFormatter((CBinary*)this, 0, bText, pMethod)
{
}

CFormatBinary::~CFormatBinary()
{
}

ADT_API int32 StringScanExV(const char* pBuf, uint32 nBufLen, const char* sFormat, CVaList& pArgList)
{
	return CFormatBinary((uint8*)pBuf, nBufLen).ScanV(sFormat, pArgList);
}

ADT_API int32 StringPrintExV(char* pBuf, uint32 nBufLen, const char* sFormat, CVaList& pArgList)
{
	return CFormatBinary((uint8*)pBuf, nBufLen).PrintV(sFormat, pArgList);
}

ADT_API int32 StringScanEx(const char* pBuf, uint32 nBufLen, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = StringScanExV(pBuf, nBufLen, sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

ADT_API int32 StringPrintEx(char* pBuf, uint32 nBufLen, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = StringPrintExV(pBuf, nBufLen, sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

ADT_API int32 StringScanV(const char* pBuf, const char* sFormat, CVaList& pArgList)
{
	return CFormatBinary((uint8*)pBuf, 0x7FFFFFFF).ScanV(sFormat, pArgList);
}

ADT_API int32 StringPrintV(char* pBuf, const char* sFormat, CVaList& pArgList)
{
	return CFormatBinary((uint8*)pBuf, 0x7FFFFFFF).PrintV(sFormat, pArgList);
}

ADT_API int32 StringScan(const char* pBuf, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = StringScanV(pBuf, sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

ADT_API int32 StringPrint(char* pBuf, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = StringPrintV(pBuf, sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

FOCP_END();
