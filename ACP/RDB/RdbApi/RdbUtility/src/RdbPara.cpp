
#include "RdbPara.hpp"

FOCP_BEGIN();

//-----------------------------------------------------------------------------
// CField
//----------------------------------------------------------------------------
CField::CField()
{
	m_pFlag = NULL;
	m_pData = NULL;
}

CField::~CField()
{
}

CField::CField(const CField& oSrc)
{
	m_nType = oSrc.m_nType;
	m_nSize = oSrc.m_nSize;
	m_pFlag = oSrc.m_pFlag;
	m_nBit = oSrc.m_nBit;
	m_pData = oSrc.m_pData;
	m_pTabDef = oSrc.m_pTabDef;
}

CField& CField::operator=(const CField& oSrc)
{
	if(this != &oSrc)
	{
		m_nType = oSrc.m_nType;
		m_nSize = oSrc.m_nSize;
		m_pFlag = oSrc.m_pFlag;
		m_nBit = oSrc.m_nBit;
		m_pData = oSrc.m_pData;
		m_pTabDef = oSrc.m_pTabDef;
	}
	return *this;
}

void CField::Initialize(uint32 nType, uint32 nSize, 
						 void* pRecordData, uint32 nFieldNo, uint32 nOffset, CBaseTableDefine* pTabDef)
{
	m_nType = nType;
	m_nSize = nSize;
	m_pFlag = (uint32*)pRecordData+RDB_FLG_L(nFieldNo);
	m_pData = (char*)pRecordData+nOffset;
	m_nBit = RDB_FLG_B(nFieldNo);
	m_pTabDef = pTabDef;
}

const char* CField::GetLogName()
{
	return m_pTabDef->GetLogName();
}

uint32 CField::GetStringSize()
{
	register uint32 nLen;
	register const char* sRet;
	register CRawData* pRaw;
	if(IS_NULL())
		return 0;
	switch(m_nType)
	{
	case RDB_INT8_FIELD: case RDB_UINT8_FIELD:
		return 4;
	case RDB_INT16_FIELD: case RDB_UINT16_FIELD:
		return 6;
	case RDB_INT32_FIELD: case RDB_UINT32_FIELD:
		return 12;
	case RDB_INT64_FIELD: case RDB_UINT64_FIELD:
		return 21;
	case RDB_FLOAT_FIELD: case RDB_DOUBLE_FIELD:
		return 64;
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		return m_nSize+1;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		sRet = (const char*)m_pData;
		sRet = *(const char**)sRet;
		nLen = 0;
		if(sRet)
			nLen = 1 + CString::StringLength(sRet);
		return nLen;
	case RDB_RAW_FIELD: 
		pRaw = (CRawData*)m_pData;
		return pRaw->nSize*2;
	case RDB_VARRAW_FIELD:
		pRaw = *(CRawData**)m_pData;
		return pRaw->nSize*2;
	}
	return 0;
}

void CField::SetNull()
{
	if(IS_NULL())
		return;
	register char * pData;
	switch(m_nType)
	{
	case RDB_VARCHAR_FIELD: 
	case RDB_VARLCHAR_FIELD: 
	case RDB_VARRAW_FIELD:
		pData = *(char**)m_pData;
		if(pData)
		{
			delete[] pData;
			*(char**)m_pData = NULL;
		}
		break;
	}
	(*m_pFlag) &= ~m_nBit;
}

char* CField::GetString(register uint32 * pStrLen)
{
	register char* sRet = NULL;
	if(pStrLen)
		pStrLen[0] = 0;
	if(!IS_NULL())switch(m_nType)
	{
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		sRet = (char*)m_pData;
		if(pStrLen)for(pStrLen[0] = 0; pStrLen[0]<m_nSize; ++pStrLen[0])
			if(!sRet[pStrLen[0]])
				break;
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		sRet = (char*)m_pData;
		sRet = *(char**)sRet;
		if(sRet && pStrLen)
			pStrLen[0] = CString::StringLength(sRet);
		break;
	default:
		FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::GetString() Failure: m_nType is %u", m_nType));
		break;
	}
	return sRet;
}

void* CField::GetRaw(register uint32 &nRawSize)
{
	nRawSize = 0;
	register void * pRet = NULL;
	register CRawData* pRaw = NULL;
	if(!IS_NULL())switch(m_nType)
	{
	case RDB_RAW_FIELD:
		pRaw = (CRawData*)m_pData;
		break;
	case RDB_VARRAW_FIELD:
		pRaw = *(CRawData**)m_pData;
		break;
	default:
		FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::GetRaw() Failure: m_nType is %u", m_nType));
		break;
	}
	if(pRaw)
	{
		nRawSize = pRaw->nSize;
		pRet = pRaw->sData;
	}
	return pRet;
}

void CField::GetAsString(char * pString)
{
	register uint8* pRaw;
	register uint32 i, nRawSize;
	register char *shift;

	pString[0] = 0;
	if(!IS_NULL())switch(m_nType)
	{
	case RDB_UINT8_FIELD:
		StringPrint(pString, "%u8", GetAtomX0(uint8));
		break;
	case RDB_INT8_FIELD:
		StringPrint(pString, "%i8", GetAtomX0(int8));
		break;
	case RDB_UINT16_FIELD:
		StringPrint(pString, "%u16", GetAtomX0(uint16));
		break;
	case RDB_INT16_FIELD:
		StringPrint(pString, "%i16", GetAtomX0(int16));
		break;
	case RDB_UINT32_FIELD:
		StringPrint(pString, "%u32", GetAtomX0(uint32));
		break;
	case RDB_INT32_FIELD:
		StringPrint(pString, "%i32", GetAtomX0(int32));
		break;
	case RDB_UINT64_FIELD:
		StringPrint(pString, "%u64", GetAtomX0(uint64));
		break;
	case RDB_INT64_FIELD:
		StringPrint(pString, "%ui64", GetAtomX0(int64));
		break;
	case RDB_FLOAT_FIELD:
		StringPrint(pString, "%f32", GetAtomX0(float));
		break;
	case RDB_DOUBLE_FIELD:
		StringPrint(pString, "%f64", GetAtomX0(double));
		break;
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		CBinary::MemoryCopy(pString, (char*)GetString(NULL), m_nSize);
		pString[m_nSize] = 0;
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		CString::StringCopy(pString, GetString(NULL));
		break;
	case RDB_RAW_FIELD: case RDB_VARRAW_FIELD:
		pRaw = (uint8*)GetRaw(nRawSize);
		shift = pString;
		for(i=0; i<nRawSize; ++i)
		{
			uint8 l, h;
			l = (pRaw[i] & 0x0F);
			h = ( (pRaw[i] & 0xF0) >> 4 );
			if(l > 10) l = 'A' + l - 10;
			if(h > 10) h = 'A' + h - 10;
			shift[0] = h;
			shift[1] = l;
			shift += 2;
		}
		shift[0] = 0;
		break;
	default:
		FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::GetAsString() Failure: m_nType is %u", m_nType));
		break;
	}
}

void CField::SetString(register const char * v)
{
	register uint32 nSize;
	switch(m_nType)
	{
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		SetNull();
		if(v && v[0])
		{
			CString::StringCopy((char*)m_pData, v, m_nSize);
			FillField();
		}
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		SetNull();
		if(v && v[0])
		{
			nSize = CString::StringLength(v);
			if(nSize > m_nSize)
				nSize = m_nSize;
			char* s = new char[nSize+1];
			CString::StringCopy(s, v, nSize);
			s[nSize] = 0;
			*(char**)m_pData = s;
			FillField();
		}
		break;
	default:
		FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetString() Failure: m_nType is %u", m_nType));
		break;
	}
}

void CField::SetRaw(register const void * v, register uint32 nLen)
{
	switch(m_nType)
	{
	case RDB_RAW_FIELD:
		SetNull();
		if(v && nLen)
		{
			CRawData* pRaw = (CRawData*)m_pData;
			if(nLen > m_nSize)
				nLen = m_nSize;
			pRaw->nSize = nLen;
			CBinary::MemoryCopy(pRaw->sData, v, nLen);
			FillField();
		}
		break;
	case RDB_VARRAW_FIELD:
		SetNull();
		if(v && nLen)
		{
			CRawData* &pRaw = *(CRawData**)m_pData;
			if(nLen > m_nSize)
				nLen = m_nSize;	
			pRaw = (CRawData*) (new char[sizeof(uint32) + nLen]);
			pRaw->nSize = nLen;
			CBinary::MemoryCopy(pRaw->sData, v, nLen);
			FillField();
		}
		break;
	default:
		FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetRaw() Failure: m_nType is %u", m_nType));
		break;
	}
}

static bool StringToRaw(register const char* s, register char * raw)
{
	if(!s || !raw)
		return false;
	while(s[0])
	{
		register uint8 c;
		if(s[0] >= '0' && s[0] <= '9')
			c = (uint8)(s[0] - '0');
		else if(s[0] >= 'A' && s[0] <= 'F')
			c = (uint8)(s[0] - 'A' + 10);
		else if(s[0] >= 'a' && s[0] <= 'f')
			c = (uint8)(s[0] - 'a' + 10);
		else return false;
		++s;
		c <<= 4;
		if(s[0] >= '0' && s[0] <= '9')
			c |= (uint8)(s[0] - '0');
		else if(s[0] >= 'A' && s[0] <= 'F')
			c |= (uint8)(s[0] - 'A' + 10);
		else if(s[0] >= 'a' && s[0] <= 'f')
			c |= (uint8)(s[0] - 'a' + 10);
		else return false;
		raw[0] = (char)c;
		++s;
		++raw;
	}
	return true;
}

void CField::SetFromString(register const char* v)
{
	register int8 i8;
	register int16 i16;
	register int32 i32;
	register int64 i64;
	register uint8 u8;
	register uint16 u16;
	register uint32 u32;
	register uint64 u64;
	register float f32;
	register double f64;
	register uint8* pRaw;
	register uint32 nRawSize;
	register uint32 nStrLen;

	SetNull();
	if(!v || !v[0])
		return;

	switch(m_nType)
	{
	case RDB_INT8_FIELD:
		if(StringScan(v, "%s8", &i8) == 1)
		{
			GetAtomX0(int8) = i8;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(i8) Failure"));
		break;
	case RDB_INT16_FIELD:
		if(StringScan(v, "%s16", &i16) == 1)
		{
			GetAtomX0(int16) = i16;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(i16) Failure"));
		break;
	case RDB_INT32_FIELD:
		if(StringScan(v, "%s32", &i32) == 1)
		{
			GetAtomX0(int32) = i32;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(i32) Failure"));
		break;
	case RDB_INT64_FIELD:
		if(StringScan(v, "%s64", &i64) == 1)
		{
			GetAtomX0(int64) = i64;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(i64) Failure"));
		break;
	case RDB_UINT8_FIELD:
		if(StringScan(v, "%u8", &u8) == 1)
		{
			GetAtomX0(uint8) = u8;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(u8) Failure"));
		break;
	case RDB_UINT16_FIELD:
		if(StringScan(v, "%u16", &u16) == 1)
		{
			GetAtomX0(uint16) = u16;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(u16) Failure"));
		break;
	case RDB_UINT32_FIELD:
		if(StringScan(v, "%u32", &u32) == 1)
		{
			GetAtomX0(uint32) = u32;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(u32) Failure"));
		break;
	case RDB_UINT64_FIELD:
		if(StringScan(v, "%u64", &u64) == 1)
		{
			GetAtomX0(uint64) = u64;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(u64) Failure"));
		break;
	case RDB_FLOAT_FIELD:
		if(StringScan(v, "%f32", &f32) == 1)
		{
			GetAtomX0(float) = (float)f32;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(f32) Failure"));
		break;
	case RDB_DOUBLE_FIELD:
		if(StringScan(v, "%f64", &f64) == 1)
		{
			GetAtomX0(double) = f64;
			FillField();
		}
		else
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(f64) Failure"));
		break;
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		SetString(v);
		break;
	case RDB_RAW_FIELD: case RDB_VARRAW_FIELD:
		nStrLen = CString::StringLength(v);
		if(nStrLen&1)
			FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(raw) Failure"));
		else
		{
			nRawSize = (nStrLen>>1);
			pRaw = new uint8[nRawSize];
			if(StringToRaw(v, (char*)pRaw))
				SetRaw(pRaw, nRawSize);
			else
				FocpLogEx(GetLogName(), FOCP_LOG_WARNING, ("CField::SetFromString(raw) Failure"));
			delete[] pRaw;
		}
		break;
	}
}

void CField::SetFromField(register CField* pValue)
{
	register uint8* pRaw;
	register char * sNewStr;
	register uint32 nLen;

	SetNull();
	if(!pValue || IS_NULL_A(pValue))
		return;

	if(m_nType != pValue->m_nType)
		return;

	switch(m_nType)
	{
	case RDB_UINT8_FIELD:
	case RDB_INT8_FIELD:
		GetAtomX0(int8) = GetAtomX(int8, pValue);
		FillField();
		break;
	case RDB_UINT16_FIELD:
	case RDB_INT16_FIELD:
		GetAtomX0(int16) = GetAtomX(int16, pValue);
		FillField();
		break;
	case RDB_FLOAT_FIELD:
	case RDB_UINT32_FIELD:
	case RDB_INT32_FIELD:
		GetAtomX0(int32) = GetAtomX(int32, pValue);
		FillField();
		break;
	case RDB_DOUBLE_FIELD:
	case RDB_UINT64_FIELD:
	case RDB_INT64_FIELD:
		GetAtomX0(int64) = GetAtomX(int64, pValue);
		FillField();
		break;
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		nLen = pValue->m_nSize;
		sNewStr = new char[nLen+1];
		CBinary::MemoryCopy(sNewStr, (char*)pValue->GetString(NULL), nLen);
		sNewStr[nLen] = 0;
		SetString(sNewStr);
		delete[] sNewStr;
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		SetString(pValue->GetString(NULL));
		break;
	case RDB_RAW_FIELD: case RDB_VARRAW_FIELD:
		pRaw = (uint8*)pValue->GetRaw(nLen);
		SetRaw(pRaw, nLen);
		break;
	}
}

template<typename T> bool IsCoincident(const T& t1, const T& t2, register uint32 nOpr)
{
	switch(nOpr)
	{
	case RDB_SQLPARA_OPERATOR_LESS:
		return (t1 < t2);
	case RDB_SQLPARA_OPERATOR_EQUAL:
		return (t1 == t2);
	case RDB_SQLPARA_OPERATOR_MORE:
		return (t1 > t2);
	case RDB_SQLPARA_OPERATOR_LESSEQUAL:
		return (t1 <= t2);
	case RDB_SQLPARA_OPERATOR_MOREEQUAL:
		return (t1 >= t2);
	case RDB_SQLPARA_OPERATOR_NOTEQUAL:
		return (t1 != t2);
	}
	return false;
}

template<typename T> int32 Compare(const T& t1, const T& t2)
{
	if(t1 < t2)
		return -1;
	if(t1 > t2)
		return 1;
	return 0;
}

int32 CField::Compare(register CField* pValue)
{
	register const char* s1, *s2;
	register uint8 *pRaw1, *pRaw2;
	register uint32 nRawSize1, nRawSize2;
	register uint32 nStrLen1, nStrLen2, nCmpLen;

	register bool bIsNull_1 = IS_NULL();
	register bool bIsNull_2 = IS_NULL_A(pValue);

	if(bIsNull_1 != bIsNull_2)
	{
		if(!bIsNull_1)
			return -1;
		return 1;
	}
	if(bIsNull_1)
		return 0;

	if(m_nType != pValue->m_nType)
		return -1;
	
	register int32 nRet = 0;
	switch(m_nType)
	{
	case RDB_INT8_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int8), GetAtomX(int8, pValue));
		break;
	case RDB_INT16_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int16), GetAtomX(int16, pValue));
		break;
	case RDB_INT32_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int32), GetAtomX(int32, pValue));
		break;
	case RDB_INT64_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int64), GetAtomX(int64, pValue));
		break;
	case RDB_UINT8_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint8), GetAtomX(uint8, pValue));
		break;
	case RDB_UINT16_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint16), GetAtomX(uint16, pValue));
		break;
	case RDB_UINT32_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint32), GetAtomX(uint32, pValue));
		break;
	case RDB_UINT64_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint64), GetAtomX(uint64, pValue));
		break;
	case RDB_FLOAT_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(float), GetAtomX(float, pValue));
		break;
	case RDB_DOUBLE_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(double), GetAtomX(double, pValue));
		break;
	case RDB_CHAR_FIELD: case RDB_VARCHAR_FIELD:
		s1 = GetString(&nStrLen1);
		s2 = pValue->GetString(&nStrLen2);
		nCmpLen = nStrLen1;
		if(nStrLen1 > nStrLen2)
			nCmpLen = nStrLen2;
		nRet = (int32)CBinary::MemoryCompare(s1, s2, nCmpLen);
		if(!nRet && nStrLen1 != nStrLen2)
			nRet = (nStrLen1>nStrLen2)?1:(-1);
		break;
	case RDB_LCHAR_FIELD: case RDB_VARLCHAR_FIELD:
		s1 = GetString(&nStrLen1);
		s2 = pValue->GetString(&nStrLen2);
		nCmpLen = nStrLen1;
		if(nStrLen1 > nStrLen2)
			nCmpLen = nStrLen2;
		nRet = CBinary::MemoryCompare((void*)s1, (void*)s2, nCmpLen, false);
		if(!nRet && nStrLen1 != nStrLen2)
			nRet = (nStrLen1>nStrLen2)?1:(-1);
		break;
	case RDB_RAW_FIELD: case RDB_VARRAW_FIELD:
		pRaw1 = (uint8*)GetRaw(nRawSize1);
		pRaw2 = (uint8*)pValue->GetRaw(nRawSize2);
		nCmpLen = nRawSize1;
		if(nRawSize1 > nRawSize2)
			nCmpLen = nRawSize2;
		nRet = (int32)CBinary::MemoryCompare(pRaw1, pRaw2, nCmpLen);
		if(!nRet && nRawSize1 != nRawSize2)
			nRet = (nRawSize1>nRawSize2)?1:(-1);
		break;
	}
	return nRet;
}

bool CField::IsCoincident(register CField* pValue, register uint32 nOpr)
{
	register int32 nCmp =  Compare(pValue);
	return FOCP_NAME::IsCoincident(nCmp, (int32)0, nOpr)?true:false;
}

uint32 CField::GetHashValue()
{
	register int64 i64;
	register uint8 *pRaw;
	register uint32 nRawSize;
	register uint32 nRet;
	register uint32 bCaseSentive;

	if(IS_NULL())
		return 0;

	switch(m_nType)
	{
	case RDB_INT8_FIELD:
	case RDB_UINT8_FIELD:
		nRet = (uint32)GetAtomX0(uint8);
		break;
	case RDB_UINT16_FIELD:
	case RDB_INT16_FIELD:
		nRet = (uint32)GetAtomX0(uint16);
		break;
	case RDB_FLOAT_FIELD:
	case RDB_INT32_FIELD:
	case RDB_UINT32_FIELD:
		nRet = GetAtomX0(uint32);
		break;
	default:
		bCaseSentive = 1;
		switch(m_nType)
		{
		case RDB_INT64_FIELD:
		case RDB_UINT64_FIELD:
		case RDB_DOUBLE_FIELD:
			i64 = GetAtomX0(int64);
			pRaw = (uint8*)&i64;
			nRawSize = 8;
			break;
		case RDB_CHAR_FIELD: case RDB_VARCHAR_FIELD:
			pRaw = (uint8*)GetString(&nRawSize);
			break;
		case RDB_LCHAR_FIELD: case RDB_VARLCHAR_FIELD:
			pRaw = (uint8*)GetString(&nRawSize);
			bCaseSentive = 0;
		case RDB_RAW_FIELD: case RDB_VARRAW_FIELD:
			pRaw = (uint8*)GetRaw(nRawSize);
			break;
		}
		nRet = GetCrc32((const uint8*)pRaw, nRawSize, bCaseSentive);
		break;
	}
	return nRet;
}

bool CField::Write(CMemoryStream & oStream)
{
	register uint32 nLen;
	CRdbValue oVal;

	register bool bRet = true;
	register uint8 bIsNull = (uint8)IS_NULL();
	if(bRet)bRet = oStream.Write(bIsNull);
	if(bRet && !bIsNull)
	{
		bRet = oStream.Write(m_nType);
		if(bRet)switch(m_nType)
		{
		case RDB_INT8_FIELD:
			bRet = oStream.Write(GetAtomX0(int8));
			break;
		case RDB_INT16_FIELD:
			bRet = oStream.Write(GetAtomX0(int16));
			break;
		case RDB_INT32_FIELD:
			bRet = oStream.Write(GetAtomX0(int32));
			break;
		case RDB_INT64_FIELD:
			bRet = oStream.Write(GetAtomX0(int64));
			break;
		case RDB_UINT8_FIELD:
			bRet = oStream.Write(GetAtomX0(uint8));
			break;
		case RDB_UINT16_FIELD:
			bRet = oStream.Write(GetAtomX0(uint16));
			break;
		case RDB_UINT32_FIELD:
			bRet = oStream.Write(GetAtomX0(uint32));
			break;
		case RDB_UINT64_FIELD:
			bRet = oStream.Write(GetAtomX0(uint64));
			break;
		case RDB_FLOAT_FIELD:
			bRet = oStream.Write(GetAtomX0(float));
			break;
		case RDB_DOUBLE_FIELD:
			bRet = oStream.Write(GetAtomX0(double));
			break;
		case RDB_CHAR_FIELD:
		case RDB_LCHAR_FIELD:
		case RDB_VARCHAR_FIELD:
		case RDB_VARLCHAR_FIELD:
			oVal.s = GetString(&nLen);
			bRet = oStream.Write(nLen);
			if(bRet)bRet = (nLen == oStream.Write(oVal.s, nLen));
			break;
		case RDB_RAW_FIELD:
		case RDB_VARRAW_FIELD:
			oVal.s = (char*)GetRaw(nLen);
			bRet = oStream.Write(nLen);
			if(bRet)bRet = (nLen == oStream.Write(oVal.s, nLen));
			break;
		}
	}
	return bRet;
}

bool CField::Read(CMemoryStream & oStream)
{
	register uint32 nLen;
	CRdbValue oVal;
	register uint8 bIsNull;

	bool bRet = oStream.Read(bIsNull);
	if(bRet)
	{
		SetNull();
		if(!bIsNull)
		{
			uint32 nType;
			bRet = oStream.Read(nType);
			if(bRet)
			{
				if(nType != m_nType)
					bRet = false;
				else switch(nType)
				{
				case RDB_INT8_FIELD:
					bRet = oStream.Read(oVal.i8);
					if(bRet) 
					{
						GetAtomX0(int8) = oVal.i8;
						FillField();
					}
					break;
				case RDB_INT16_FIELD:
					bRet = oStream.Read(oVal.i16);
					if(bRet) 
					{
						GetAtomX0(int16) = oVal.i16;
						FillField();
					}
					break;
				case RDB_INT32_FIELD:
					bRet = oStream.Read(oVal.i32);
					if(bRet) 
					{
						GetAtomX0(int32) = oVal.i32;
						FillField();
					}
					break;
				case RDB_INT64_FIELD:
					bRet = oStream.Read(oVal.i64);
					if(bRet) 
					{
						GetAtomX0(int64) = oVal.i64;
						FillField();
					}
					break;
				case RDB_UINT8_FIELD:
					bRet = oStream.Read(oVal.u8);
					if(bRet) 
					{
						GetAtomX0(uint8) = oVal.u8;
						FillField();
					}
					break;
				case RDB_UINT16_FIELD:
					bRet = oStream.Read(oVal.u16);
					if(bRet) 
					{
						GetAtomX0(uint16) = oVal.u16;
						FillField();
					}
					break;
				case RDB_UINT32_FIELD:
					bRet = oStream.Read(oVal.u32);
					if(bRet) 
					{
						GetAtomX0(uint32) = oVal.u32;
						FillField();
					}
					break;
				case RDB_UINT64_FIELD:
					bRet = oStream.Read(oVal.u64);
					if(bRet) 
					{
						GetAtomX0(uint64) = oVal.u64;
						FillField();
					}
					break;
				case RDB_FLOAT_FIELD:
					bRet = oStream.Read(oVal.f32);
					if(bRet) 
					{
						GetAtomX0(float) = oVal.f32;
						FillField();
					}
					break;
				case RDB_DOUBLE_FIELD:
					bRet = oStream.Read(oVal.f64);
					if(bRet) 
					{
						GetAtomX0(double) = oVal.f64;
						FillField();
					}
					break;
				case RDB_CHAR_FIELD:
				case RDB_LCHAR_FIELD:
				case RDB_VARCHAR_FIELD:
				case RDB_VARLCHAR_FIELD:
					bRet = oStream.Read(nLen);
					if(bRet)
					{
						oVal.s = new char[nLen+1];
						bRet = (nLen == oStream.Read(oVal.s, nLen));
						if(bRet)
						{
							oVal.s[nLen] = 0;
							SetString(oVal.s);
						}
						delete[] oVal.s;
					}
					break;
				case RDB_RAW_FIELD:
				case RDB_VARRAW_FIELD:
					bRet = oStream.Read(nLen);
					if(bRet)
					{
						oVal.s = new char[nLen];
						bRet = (nLen == oStream.Read(oVal.s, nLen));
						if(bRet)
							SetRaw(oVal.s, nLen);
						delete[] oVal.s;
					}
					break;
				}
			}
		}
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// CRecord
//----------------------------------------------------------------------------
CRecord::CRecord(CBaseTableDefine * pTabDef)
{
	m_nRowId = 0;
	m_pTabDef = pTabDef;
	m_pRecordData = new uint8[m_pTabDef->m_nRecordSize];
	CBinary::MemorySet(m_pRecordData, 0, m_pTabDef->m_nRecordSize);
	m_nFieldCount = m_pTabDef->m_nFieldCount;
	m_pFieldTable = NULL;//new CField[m_nFieldCount];
}

CRecord::~CRecord()
{
	Clear();
	delete[] m_pRecordData;
	m_pRecordData = NULL;
	if(m_pFieldTable)
		delete[] m_pFieldTable;
	m_pFieldTable = NULL;
}

const char* CRecord::GetLogName()
{
	return m_pTabDef->GetLogName();
}

CField* CRecord::GetField(register uint32 nFieldNo)
{
	if(!m_pFieldTable)
		m_pFieldTable = new CField[m_nFieldCount];
	CField* pField = m_pFieldTable + nFieldNo;
	if(!pField->m_pData)
	{
		CBaseFieldDefine* pFieldDef = (CBaseFieldDefine*)m_pTabDef->m_pFieldDefineSet[nFieldNo];
		pField->Initialize(pFieldDef->m_pBaseAttr->nType, pFieldDef->m_nSize, m_pRecordData, nFieldNo, pFieldDef->m_nOffset, m_pTabDef);
	}
	return pField;
}

uint64 CRecord::GetRowId()
{
	return m_nRowId;
}

void CRecord::SetRowId(uint64 nRowId)
{
	m_nRowId = nRowId;
}

void CRecord::Clear()
{
	if(m_pRecordData)
	{
		register uint32 nVarFieldCount = m_pTabDef->m_nVarFieldCount;
		if(nVarFieldCount)
		{
			register char* pData = (char*)m_pRecordData;
			register uint32* pVarFieldList = m_pTabDef->m_pVarFieldList;
			for(register uint32 i=0; i<nVarFieldCount; ++i)
			{
				register char* &pFieldData = *(char**)(pData + m_pTabDef->m_pFieldDefineSet[pVarFieldList[i]]->m_nOffset);
				if(pFieldData)
				{
					delete[] pFieldData;
					pFieldData = NULL;
				}
			}
		}
		CBinary::MemorySet(m_pRecordData, 0, m_pTabDef->GetFieldFlagCount()<<2);
	}
}

bool CRecord::Write(CMemoryStream & oStream, CSqlFilter &oFilter)
{
	register uint32 nCount = oFilter.m_nParaCount;
	if(!nCount)
		nCount = m_pTabDef->m_nFieldCount;
	if(!oStream.Write(nCount))
		return false;
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint32 nCol = oFilter.m_pParaTable[i];
		register CField* pField = GetField(nCol);
		if(!oStream.Write(nCol))
			return false;
		if(!pField->Write(oStream))
			return false;
	}
	return true;
}

bool CRecord::Read(CMemoryStream & oStream)
{
	register uint32 nCount;
	if(!oStream.Read(nCount))
		return false;
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint32 nCol;
		if(!oStream.Read(nCol))
			return false;
		if(!GetField(nCol)->Read(oStream))
			return false;
	}
	return true;
}

uint32 CRecord::GetType(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->m_nType;
}

bool CRecord::IsNull(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return IS_NULL_A(pField);
}

void CRecord::SetNull(register uint32 nFieldNo)
{
	GetField(nFieldNo)->SetNull();
}

int8 CRecord::GetInt8(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(int8, pField);
}

int16 CRecord::GetInt16(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(int16, pField);
}

int32 CRecord::GetInt32(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(int32, pField);
}

int64 CRecord::GetInt64(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(int64, pField);
}

uint8 CRecord::GetUInt8(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(uint8, pField);
}

uint16 CRecord::GetUInt16(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(uint16, pField);
}

uint32 CRecord::GetUInt32(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(uint32, pField);
}

uint64 CRecord::GetUInt64(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(uint64, pField);
}

float CRecord::GetFloat(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(float, pField);
}

double CRecord::GetDouble(register uint32 nFieldNo)
{
	register CField* pField = GetField(nFieldNo);
	return GetAtomX(double, pField);
}

char* CRecord::GetString(register uint32 nFieldNo, register uint32 * pStrLen)
{
	return GetField(nFieldNo)->GetString(pStrLen);
}

void* CRecord::GetRaw(register uint32 nFieldNo, register uint32 &nRawSize)
{
	return GetField(nFieldNo)->GetRaw(nRawSize);
}

uint32 CRecord::GetStringSize(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->GetStringSize();
}

void CRecord::GetAsString(register uint32 nFieldNo, register char * pString)
{
	GetField(nFieldNo)->GetAsString(pString);
}

void CRecord::SetInt8(register uint32 nFieldNo, register int8 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(int8, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetInt16(register uint32 nFieldNo, register int16 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(int16, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetInt32(register uint32 nFieldNo, register int32 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(int32, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetInt64(register uint32 nFieldNo, register int64 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(int64, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetUInt8(register uint32 nFieldNo, register uint8 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(uint8, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetUInt16(register uint32 nFieldNo, register uint16 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(uint16, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetUInt32(register uint32 nFieldNo, register uint32 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(uint32, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetUInt64(register uint32 nFieldNo, register uint64 v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(uint64, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetFloat(register uint32 nFieldNo, register float v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(float, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetDouble(register uint32 nFieldNo, register double v)
{
	register CField* pField = GetField(nFieldNo);
	GetAtomX(double, pField) = v;
	FillFieldA(pField);
}

void CRecord::SetString(register uint32 nFieldNo, register const char * v)
{
	GetField(nFieldNo)->SetString(v);
}

void CRecord::SetRaw(register uint32 nFieldNo, register const void * v, uint32 nLen)
{
	GetField(nFieldNo)->SetRaw(v, nLen);
}

void CRecord::SetFromString(register uint32 nFieldNo, register const char* v)
{
	GetField(nFieldNo)->SetFromString(v);
}

void CRecord::SetFromField(register uint32 nFieldNo, register CRecord* pRecord)
{
	GetField(nFieldNo)->SetFromField(pRecord->GetField(nFieldNo));
}

//-----------------------------------------------------------------------------
// CRecordSet
//----------------------------------------------------------------------------
CRecordSet::CRecordSet(CBaseTableDefine * pTabDef)
{
	m_pTabDef = pTabDef;
	m_nSetSize = 0;
	m_pRecordSet = NULL;
	m_nRecordCount = 0;
}

CRecordSet::~CRecordSet()
{
	Clear();
}

const char* CRecordSet::GetLogName()
{
	return m_pTabDef->GetLogName();
}

void CRecordSet::PreAlloc()
{
	m_nRecordCount = m_nSetSize;
}

CRecord* CRecordSet::AllocRecord()
{
	register CRecord* pRecord = NULL;
	if(m_nRecordCount < m_nSetSize)
	{
		pRecord = m_pRecordSet[m_nRecordCount++];
		pRecord->Clear();
	}
	return pRecord;
}

void CRecordSet::PopRecord()
{
	if(m_nRecordCount)
		--m_nRecordCount;
}

uint32 CRecordSet::GetRecordSetSize()
{
	return m_nSetSize;
}

void CRecordSet::SetRecordSetSize(uint32 nSize)
{
	if(nSize != m_nSetSize)
	{
		register CRecord** pRecSet = new CRecord*[nSize];
		register uint32 i, nRest = nSize;
		if(nRest > m_nSetSize)
			nRest = m_nSetSize;
		for(i=0; i<nRest; ++i)
			pRecSet[i] = m_pRecordSet[i];
		for(; i<nSize; ++i)
			pRecSet[i] = new CRecord(m_pTabDef);
		for(; i<m_nSetSize; ++i)
			delete m_pRecordSet[i];
		if(m_pRecordSet)
			delete[] m_pRecordSet;
		m_pRecordSet = pRecSet;
		m_nSetSize = nSize;
	}
	m_nRecordCount = 0;
}

uint32 CRecordSet::GetRecordCount()
{
	return m_nRecordCount;
}

CRecord* CRecordSet::GetRecord(uint32 nRecNo)
{
	if(nRecNo < m_nRecordCount)
		return m_pRecordSet[nRecNo];
	return NULL;
}

uint32 CRecordSet::GetResultCount()
{
	return m_nRecordCount;
}

CRdbResult* CRecordSet::GetResult(uint32 nRecordNo)
{
	if(nRecordNo < m_nRecordCount)
		return m_pRecordSet[nRecordNo];
	return NULL;
}

void CRecordSet::Clear()
{
	if(m_pRecordSet)
	{
		for(register uint32 i=0; i<m_nSetSize; ++i)
			delete m_pRecordSet[i];
		delete[] m_pRecordSet;
		m_pRecordSet = NULL;
		m_nSetSize = 0;
	}
	m_nRecordCount = 0;
}

//-----------------------------------------------------------------------------
// CSqlFilter
//----------------------------------------------------------------------------
CSqlFilter::CSqlFilter(CBaseTableDefine* pTabDef)
{
	m_pTabDef = pTabDef;
	m_nFieldCount = m_pTabDef->m_nFieldCount;
	m_nFlgCount = m_pTabDef->GetFieldFlagCount();
	m_nParaCount = 0;
	m_pParaTable = new uint32[m_nFieldCount];
	m_pFlag = new uint32[m_nFlgCount];
	for(uint32 i=0; i<m_nFlgCount; ++i)
		m_pFlag[i] = 0;
}

CSqlFilter::~CSqlFilter()
{
	delete[] m_pParaTable;
	delete[] m_pFlag;
}

const char* CSqlFilter::GetLogName()
{
	return m_pTabDef->GetLogName();
}

void CSqlFilter::SetField(uint32 nFieldNo)
{
	if(m_pTabDef->IsValidField(nFieldNo))
	{
		register uint32 nFlg = RDB_FLG_L(nFieldNo);
		register uint32 nBit = RDB_FLG_B(nFieldNo);
		if(!(m_pFlag[nFlg] & nBit))
		{
			m_pParaTable[m_nParaCount++] = nFieldNo;
			m_pFlag[nFlg] |= nBit;
		}
	}
}

void CSqlFilter::Clear()
{
	if(m_nParaCount)
	{
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			m_pFlag[i] = 0;
		m_nParaCount = 0;
	}
}

void CSqlFilter::SetAllField()
{
	if(m_nParaCount != m_nFieldCount)
	{
		for(register uint32 i=0; i<m_nFieldCount; ++i)
		{
			m_pParaTable[i] = i;
			m_pFlag[RDB_FLG_L(i)] |= RDB_FLG_B(i);
		}
		m_nParaCount = m_nFieldCount;
	}
}

bool CSqlFilter::IsSetField(uint32 nFieldNo)
{
	if(nFieldNo >= m_nFieldCount)
		return false;
	if(m_pFlag[RDB_FLG_L(nFieldNo)] & RDB_FLG_B(nFieldNo))
		return true;
	return false;
}

uint32 CSqlFilter::GetParaCount()
{
	return m_nParaCount;
}

uint32 CSqlFilter::GetPara(uint32 nParaIdx)
{
	if(nParaIdx >= m_nParaCount)
		return 0xFFFFFFFF;
	return m_pParaTable[nParaIdx];
}

bool CSqlFilter::Write(CMemoryStream & oStream)
{
	if(!oStream.Write(m_nParaCount))
		return false;
	if(m_nParaCount)
	{
		if(!oStream.Write(m_nFieldCount))
			return false;
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			if(!oStream.Write(m_pFlag[i]))
				return false;
	}
	return true;
}

bool CSqlFilter::Read(CMemoryStream & oStream)
{
	register uint32 nCount;
	if(!oStream.Read(nCount))
		return false;
	if(nCount >= m_nFieldCount)
		return false;
	Clear();
	if(nCount)
	{
		m_nParaCount = nCount;
		if(!oStream.Read(nCount))
			return false;
		if(nCount != m_nFieldCount)
			return false;
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			if(!oStream.Read(m_pFlag[i]))
				return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// CSqlParameter
//----------------------------------------------------------------------------
CSqlParameter::CSqlParameter(CBaseTableDefine * pTabDef)
:m_oRecord(pTabDef)
{
	m_pTabDef = pTabDef;
	m_nFieldCount = m_pTabDef->m_nFieldCount;
	m_nFlgCount = m_pTabDef->GetFieldFlagCount();
	m_nParaCount = 0;
	m_pParaTable = new CParaItem[m_nFieldCount];
	m_pFlag = new uint32[m_nFlgCount];
	for(register uint32 i=0; i<m_nFlgCount; ++i)
		m_pFlag[i] = 0;
}

CSqlParameter::~CSqlParameter()
{
	delete[] m_pParaTable;
	delete[] m_pFlag;
}

const char* CSqlParameter::GetLogName()
{
	return m_pTabDef->GetLogName();
}

CRecord& CSqlParameter::GetRecord()
{
	return m_oRecord;
}

uint32 CSqlParameter::SetField(uint32 nFieldNo, uint32 nOperator)
{
	if(m_pTabDef->IsValidField(nFieldNo))
	{
		register uint32 nFlg = RDB_FLG_L(nFieldNo);
		register uint32 nBit = RDB_FLG_B(nFieldNo);
		if(!(m_pFlag[nFlg] & nBit))
		{
			CParaItem &oItem = m_pParaTable[m_nParaCount++];
			oItem.nFieldNo = nFieldNo;
			oItem.nOperator = nOperator;
			m_pFlag[nFlg] |= nBit;
		}
		return RDB_SUCCESS;
	}
	return RDB_FIELD_NOTEXIST;
}

void CSqlParameter::Clear()
{
	if(m_nParaCount)
	{
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			m_pFlag[i] = 0;
		m_nParaCount = 0;
		m_oRecord.Clear();
	}
}

bool CSqlParameter::IsSetField(uint32 nFieldNo)
{
	if(nFieldNo >= m_nFieldCount)
		return false;
	if(m_pFlag[RDB_FLG_L(nFieldNo)] & RDB_FLG_B(nFieldNo))
		return true;
	return false;
}

uint32 CSqlParameter::GetParaCount()
{
	return m_nParaCount;
}

uint32 CSqlParameter::GetPara(uint32 nParaIdx, uint32 *pOperator, uint32 * pFieldNo)
{
	if(nParaIdx >= m_nParaCount)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CSqlParameter::GetPara(%u) failure: m_nParaCount=%u", 
			nParaIdx, m_nParaCount));
		return RDB_INVALID_INPUT;
	}
	CParaItem& oItem = m_pParaTable[nParaIdx];
	pFieldNo[0] = oItem.nFieldNo;
	pOperator[0] = oItem.nOperator;
	return RDB_SUCCESS;
}

void CSqlParameter::GetFilter(CSqlFilter &oFilter)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)
		oFilter.SetField(m_pParaTable[i].nFieldNo);
}

bool CSqlParameter::IsCoincident(CRecord& oRecord)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		CParaItem& oItem = m_pParaTable[i];
		register uint32 nFieldNo = oItem.nFieldNo;
		bRet = oRecord.GetField(nFieldNo)->IsCoincident(m_oRecord.GetField(nFieldNo), oItem.nOperator);
		if(!bRet)
			break;
	}
	return bRet;
}

bool CSqlParameter::IsCoincident(uint32 nFieldNo, CField* pField)
{
	register bool bRet = true;
	if(m_pFlag[RDB_FLG_L(nFieldNo)] & RDB_FLG_B(nFieldNo))
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			CParaItem& oItem = m_pParaTable[i];
			if(nFieldNo == oItem.nFieldNo)
			{
				bRet = pField->IsCoincident(m_oRecord.GetField(nFieldNo), oItem.nOperator);
				if(!bRet)
					break;
			}
		}
	}
	return bRet;
}

uint32 CSqlParameter::GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount)
{
	if(m_pFlag[RDB_FLG_L(nFieldNo)] & RDB_FLG_B(nFieldNo))
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			CParaItem& oItem = m_pParaTable[i];
			if(nFieldNo == oItem.nFieldNo)
			{
				if(oItem.nOperator != RDB_SQLPARA_OPERATOR_EQUAL)
					return 1;
				nHashValue =m_oRecord.GetField(nFieldNo)->GetHashValue();
				++nHashCount;
			}
		}
	}
	return 0;
}

CField* CSqlParameter::GetCondField(uint32 nFieldNo, uint32& nOperator)
{
	if(m_pFlag[RDB_FLG_L(nFieldNo)] & RDB_FLG_B(nFieldNo))
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			if(m_pParaTable[i].nFieldNo == nFieldNo)
			{
				nOperator = m_pParaTable[i].nOperator;
				return m_oRecord.GetField(nFieldNo);
			}
		}
	}
	return NULL;
}

bool CSqlParameter::GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen)
{
	register uint32 nFlg = RDB_FLG_L(nFieldNo), nBit = RDB_FLG_B(nFieldNo);
	if(m_pFlag[nFlg] & nBit)
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			CParaItem& oItem = m_pParaTable[i];
			if(oItem.nFieldNo == nFieldNo)
			{
				if(sStr)
					return false;
				if(oItem.nOperator != RDB_SQLPARA_OPERATOR_EQUAL)
					return false;
				sStr = m_oRecord.GetField(nFieldNo)->GetString(pStrLen);
				if(!sStr || !pStrLen[0])
					return false;
				--m_nParaCount;
				for(; i<m_nParaCount; ++i)
					m_pParaTable[i] = m_pParaTable[i+1];
				break;
			}
		}
		m_pFlag[nFlg] &= ~nBit;
	}
	return true;
}

bool CSqlParameter::Write(CMemoryStream & oStream)
{
	if(!oStream.Write(m_nParaCount))
		return false;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		CParaItem & oItem = m_pParaTable[i];
		register uint32 nCol = oItem.nFieldNo;
		register uint32 nOp = oItem.nOperator;
		if(!oStream.Write(nCol))
			return false;
		if(!oStream.Write(nOp))
			return false;
	}
	CSqlFilter oFilter(m_pTabDef);
	GetFilter(oFilter);
	return m_oRecord.Write(oStream, oFilter);
}

bool CSqlParameter::Read(CMemoryStream & oStream)
{
	register uint32 nCount;
	if(!oStream.Read(nCount))
		return false;
	if(nCount > m_nFieldCount)
		return false;
	Clear();
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint32 nCol, nOp;
		if(!oStream.Read(nCol))
			return false;
		if(!oStream.Read(nOp))
			return false;
		if(SetField(nCol, nOp))
			return false;
	}
	return m_oRecord.Read(oStream);
}

uint32 CSqlParameter::GetType(register uint32 nFieldNo)
{
	return m_oRecord.GetType(nFieldNo);
}

void CSqlParameter::SetNull(register uint32 nFieldNo, uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetNull(nFieldNo);
}

void CSqlParameter::SetInt8(register uint32 nFieldNo, register int8 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt8(nFieldNo, v);
}

void CSqlParameter::SetInt16(register uint32 nFieldNo, register int16 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt16(nFieldNo, v);
}

void CSqlParameter::SetInt32(register uint32 nFieldNo, register int32 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt32(nFieldNo, v);
}

void CSqlParameter::SetInt64(register uint32 nFieldNo, register int64 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt64(nFieldNo, v);
}

void CSqlParameter::SetUInt8(register uint32 nFieldNo, register uint8 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt8(nFieldNo, v);
}

void CSqlParameter::SetUInt16(register uint32 nFieldNo, register uint16 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt16(nFieldNo, v);
}

void CSqlParameter::SetUInt32(register uint32 nFieldNo, register uint32 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt32(nFieldNo, v);
}

void CSqlParameter::SetUInt64(register uint32 nFieldNo, register uint64 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt64(nFieldNo, v);
}

void CSqlParameter::SetFloat(register uint32 nFieldNo, register float v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetFloat(nFieldNo, v);
}

void CSqlParameter::SetDouble(register uint32 nFieldNo, register double v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetDouble(nFieldNo, v);
}

void CSqlParameter::SetString(register uint32 nFieldNo, register const char * v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetString(nFieldNo, v);
}

void CSqlParameter::SetRaw(register uint32 nFieldNo, register const void * v, register uint32 nLen, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetRaw(nFieldNo, v, nLen);
}

void CSqlParameter::SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetFromString(nFieldNo, v);
}

//-----------------------------------------------------------------------------
// CSqlParameterSet
//----------------------------------------------------------------------------
CSqlParameterSet::CSqlParameterSet(CBaseTableDefine* pTabDef)
{
	m_pTabDef = pTabDef;
	m_nParaCount = 0;
	m_pPataSet = NULL;
}

CSqlParameterSet::~CSqlParameterSet()
{
	Clear();
	m_pTabDef = NULL;
}

const char* CSqlParameterSet::GetLogName()
{
	return m_pTabDef->GetLogName();
}

CRdbPara* CSqlParameterSet::AddPara()
{
	register CSqlParameter* pSqlPara = NULL;
	register CParaItem* pNewSet = (CParaItem*)CMalloc::Realloc(m_pPataSet, (m_nParaCount+1)*sizeof(CParaItem));
	if(pNewSet)
	{
		m_pPataSet = pNewSet;
		pNewSet[m_nParaCount].bAtom = true;
		pNewSet[m_nParaCount].pPara = new CSqlParameter(m_pTabDef);
		if(!pNewSet[m_nParaCount].pPara)
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CSqlParameterSet::AddPara() failure: RDB_LACK_MEMORY"));
		else
		{
			pSqlPara = (CSqlParameter*)pNewSet[m_nParaCount].pPara;
			++m_nParaCount;
		}
	}
	else
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CSqlParameterSet::AddPara() failure: RDB_LACK_MEMORY"));
	return pSqlPara;
}

CRdbParaSet* CSqlParameterSet::AddParaSet()
{
	register CSqlParameterSet* pSqlPara = NULL;
	register CParaItem* pNewSet = (CParaItem*)CMalloc::Realloc(m_pPataSet, (m_nParaCount+1)*sizeof(CParaItem));
	if(pNewSet)
	{
		m_pPataSet = pNewSet;
		pNewSet[m_nParaCount].bAtom = false;
		pNewSet[m_nParaCount].pPara = new CSqlParameterSet(m_pTabDef);
		if(!pNewSet[m_nParaCount].pPara)
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CSqlParameterSet::AddParaSet() failure: RDB_LACK_MEMORY"));
		else
		{
			pSqlPara = (CSqlParameterSet*)pNewSet[m_nParaCount].pPara;
			++m_nParaCount;
		}
	}
	else
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CSqlParameterSet::AddParaSet() failure: RDB_LACK_MEMORY"));
	return pSqlPara;
}

uint32 CSqlParameterSet::GetParaCount()
{
	return m_nParaCount;
}

void* CSqlParameterSet::GetPara(uint32 nParaIdx, bool &bParaSet)
{
	if(nParaIdx >= m_nParaCount)
		return NULL;
	bParaSet = !m_pPataSet[nParaIdx].bAtom;
	return m_pPataSet[nParaIdx].pPara;
}

void CSqlParameterSet::Clear()
{
	if(m_pTabDef)
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			if(m_pPataSet[i].bAtom)
				delete (CSqlParameter*)m_pPataSet[i].pPara;
			else
				delete (CSqlParameterSet*)m_pPataSet[i].pPara;
		}
		if(m_pPataSet)
			CMalloc::Free(m_pPataSet);
	}
	m_nParaCount = 0;
	m_pPataSet = NULL;
}

void CSqlParameterSet::GetFilter(CSqlFilter &oFilter)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			pSqlPara->GetFilter(oFilter);
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			pSqlParaSet->GetFilter(oFilter);
		}
	}
}

bool CSqlParameterSet::IsCoincident(CRecord& oRecord)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			bRet = pSqlPara->IsCoincident(oRecord);
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			bRet = pSqlParaSet->IsCoincident(oRecord);
		}
		if(!bRet)
			break;
	}
	return bRet;
}

bool CSqlParameterSet::IsCoincident(uint32 nFieldNo, CField* pField)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			bRet = pSqlPara->IsCoincident(nFieldNo, pField);
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			bRet = pSqlParaSet->IsCoincident(nFieldNo, pField);
		}
		if(!bRet)
			break;
	}
	return bRet;
}

uint32 CSqlParameterSet::GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount)
{
	register uint32 nRet = 0;
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			nRet = pSqlPara->GetHashValue(nFieldNo, nHashValue, nHashCount);
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			nRet = pSqlParaSet->GetHashValue(nFieldNo, nHashValue, nHashCount);
		}
		if(nRet)
			break;
	}
	return nRet;
}

bool CSqlParameterSet::GetCondField(uint32 nFieldNo, CCondField& oMinField, CCondField& oMaxField)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			CCondField oCondField;
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			oCondField.pField = pSqlPara->GetCondField(nFieldNo, oCondField.nOperator);
			if(oCondField.pField && !GetCondField(oCondField, oMinField, oMaxField))
				return false;
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			if(!pSqlParaSet->GetCondField(nFieldNo, oMinField, oMaxField))
				return false;
		}
	}
	return true;
}

bool CSqlParameterSet::GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)	
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			if(!pSqlPara->GetPrefixCondField(nFieldNo, sStr, pStrLen))
				return false;
		}
		else
		{
			register CSqlParameterSet* pSqlParaSet = (CSqlParameterSet*)pPara->pPara;
			if(!pSqlParaSet->GetPrefixCondField(nFieldNo, sStr, pStrLen))
				return false;
		}
	}
	return true;
}

bool CSqlParameterSet::GetCondField(CCondField &oCondField, CCondField& oMinField, CCondField& oMaxField)
{
	register uint32 nOperator;
	if(oCondField.nOperator < RDB_SQLPARA_OPERATOR_LESS || 
		oCondField.nOperator > RDB_SQLPARA_OPERATOR_MOREEQUAL)
		return false;
	if(oCondField.nOperator == RDB_SQLPARA_OPERATOR_EQUAL)
	{
		oCondField.nOperator = RDB_SQLPARA_OPERATOR_MOREEQUAL;
		if(!GetCondField(oCondField, oMinField, oMaxField))
			return false;
		oCondField.nOperator = RDB_SQLPARA_OPERATOR_LESSEQUAL;
		if(!GetCondField(oCondField, oMinField, oMaxField))
			return false;
	}
	else if(oCondField.nOperator & RDB_SQLPARA_OPERATOR_LESS)
	{
		if(oCondField.nOperator & RDB_SQLPARA_OPERATOR_MORE)
			return false;		
		if(oMinField.pField)
		{
			nOperator = RDB_SQLPARA_OPERATOR_MOREEQUAL;
			if(oCondField.nOperator & RDB_SQLPARA_OPERATOR_EQUAL)
			{
				if(oMinField.nOperator == nOperator)
					nOperator = RDB_SQLPARA_OPERATOR_MORE;
			}
			if(oMinField.pField->IsCoincident(oCondField.pField, nOperator))
				return false;
		}
		if(!oMaxField.pField)
			oMaxField = oCondField;
		else if(oCondField.pField->IsCoincident(oMaxField.pField, oMaxField.nOperator))
			oMaxField = oCondField;
	}
	else
	{		
		if(oMaxField.pField)
		{
			nOperator = RDB_SQLPARA_OPERATOR_LESSEQUAL;
			if(oCondField.nOperator & RDB_SQLPARA_OPERATOR_EQUAL)
			{
				if(oMaxField.nOperator == nOperator)
					nOperator = RDB_SQLPARA_OPERATOR_LESS;
			}
			if(oMaxField.pField->IsCoincident(oCondField.pField, nOperator))
				return false;
		}
		if(!oMinField.pField)
			oMinField = oCondField;
		else if(oCondField.pField->IsCoincident(oMinField.pField, oMinField.nOperator))
			oMaxField = oCondField;
	}
	return true;
}

bool CSqlParameterSet::Write(CMemoryStream & oStream)
{
	if(!oStream.Write(m_nParaCount))
		return false;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(!oStream.Write((uint8)pPara->bAtom))
			return false;
		if(pPara->bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)pPara->pPara;
			if(!pSqlPara->Write(oStream))
				return false;
		}
		else
		{
			register CSqlParameterSet* pSqlPara = (CSqlParameterSet*)pPara->pPara;
			if(!pSqlPara->Write(oStream))
				return false;
		}
	}
	return true;
}

bool CSqlParameterSet::Read(CMemoryStream & oStream)
{
	register uint32 nCount;

	Clear();
	if(!oStream.Read(nCount))
		return false;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register uint8 bAtom;
		if(!oStream.Read(bAtom))
			return false;
		if(bAtom)
		{
			register CSqlParameter* pSqlPara = (CSqlParameter*)AddPara();
			if(!pSqlPara)
				return false;
			if(!pSqlPara->Read(oStream))
				return false;
		}
		else
		{
			register CSqlParameterSet * pSqlPara = (CSqlParameterSet*)AddParaSet();
			if(!pSqlPara)
				return false;
			if(!pSqlPara->Read(oStream))
				return false;
		}
	}
	return true;
}

FOCP_END();
