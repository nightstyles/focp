
#include "MdbAccess.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();

MCI_API uint32 GetFieldFlagCount(CMdbTableDef* pTabDef)
{
	uint32 nSize = MDB_FLG_L(pTabDef->nFieldCount-1) + 1;
	if(nSize & 1)
		nSize += 1;
	return nSize;
}

//-----------------------------------------------------------------------------
// CMdbField
//----------------------------------------------------------------------------
CMdbField::CMdbField()
{
	m_pFlag = NULL;
	m_pData = NULL;
}

CMdbField::~CMdbField()
{
}

CMdbField::CMdbField(const CMdbField& oSrc)
{
	m_nType = oSrc.m_nType;
	m_nSize = oSrc.m_nSize;
	m_pFlag = oSrc.m_pFlag;
	m_nBit = oSrc.m_nBit;
	m_pData = oSrc.m_pData;
}

CMdbField& CMdbField::operator=(const CMdbField& oSrc)
{
	if(this != &oSrc)
	{
		m_nType = oSrc.m_nType;
		m_nSize = oSrc.m_nSize;
		m_pFlag = oSrc.m_pFlag;
		m_nBit = oSrc.m_nBit;
		m_pData = oSrc.m_pData;
	}
	return *this;
}

void CMdbField::Initialize(uint32 nType, uint32 nSize,
						   void* pRecordData, uint32 nFieldNo, uint32 nOffset)
{
	m_nType = nType;
	m_nSize = nSize;
	m_pFlag = (uint32*)pRecordData+MDB_FLG_L(nFieldNo);
	m_pData = (char*)pRecordData+nOffset;
	m_nBit = MDB_FLG_B(nFieldNo);
}

uint32 CMdbField::GetStringSize()
{
	register uint32 nLen;
	register const char* sRet;
	register CMdbRaw* pRaw;
	if(IS_NULL())
		return 0;
	switch(m_nType)
	{
	case MDB_INT8_FIELD:
	case MDB_UINT8_FIELD:
		return 4;
	case MDB_INT16_FIELD:
	case MDB_UINT16_FIELD:
		return 8;
	case MDB_INT32_FIELD:
	case MDB_UINT32_FIELD:
		return 12;
	case MDB_INT64_FIELD:
	case MDB_UINT64_FIELD:
		return 24;
	case MDB_FLOAT_FIELD:
	case MDB_DOUBLE_FIELD:
		return 64;
	case MDB_DATE_FIELD:
		return 12;
	case MDB_TIME_FIELD:
		return 16;
	case MDB_DATETIME_FIELD:
		return 24;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
		return m_nSize+1;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		sRet = (const char*)m_pData;
		sRet = *(const char**)sRet;
		nLen = 0;
		if(sRet)
			nLen = 1 + CString::StringLength(sRet);
		return nLen;
	case MDB_RAW_FIELD:
		pRaw = (CMdbRaw*)m_pData;
		return pRaw->nSize*2;
	case MDB_VARRAW_FIELD:
		pRaw = *(CMdbRaw**)m_pData;
		return pRaw->nSize*2;
	}
	return 0;
}

void CMdbField::SetNull()
{
	if(IS_NULL())
		return;
	register char * pData;
	switch(m_nType)
	{
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
	case MDB_VARRAW_FIELD:
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

char* CMdbField::GetString(register uint32 * pStrLen)
{
	register char* sRet = NULL;
	if(pStrLen)
		pStrLen[0] = 0;
	if(!IS_NULL())switch(m_nType)
		{
		case MDB_CHAR_FIELD:
		case MDB_LCHAR_FIELD:
			sRet = (char*)m_pData;
			if(pStrLen)
				pStrLen[0] = CString::StringLength(sRet, m_nSize);
			break;
		case MDB_VARCHAR_FIELD:
		case MDB_VARLCHAR_FIELD:
			sRet = *(char**)m_pData;
			if(sRet && pStrLen)
				pStrLen[0] = CString::StringLength(sRet);
			break;
		default:
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::GetString() Failure: m_nType is %u", m_nType));
			break;
		}
	return sRet;
}

void* CMdbField::GetRaw(register uint32 &nRawSize)
{
	nRawSize = 0;
	register void * pRet = NULL;
	register CMdbRaw* pRaw = NULL;
	if(!IS_NULL())switch(m_nType)
		{
		case MDB_RAW_FIELD:
			pRaw = (CMdbRaw*)m_pData;
			break;
		case MDB_VARRAW_FIELD:
			pRaw = *(CMdbRaw**)m_pData;
			break;
		default:
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::GetRaw() Failure: m_nType is %u", m_nType));
			break;
		}
	if(pRaw)
	{
		nRawSize = pRaw->nSize;
		pRet = pRaw->sData;
	}
	return pRet;
}

CDate CMdbField::GetDate()
{
	return GetAtomX(int32, this);
}

CTime CMdbField::GetTime()
{
	return GetAtomX(int32, this);
}

CDateTime CMdbField::GetDateTime()
{
	return GetAtomX(double, this);
}

void CMdbField::GetAsString(char * pString)
{
	register uint8* pRaw;
	register uint32 i, nRawSize;
	register char *shift;

	pString[0] = 0;
	if(!IS_NULL())switch(m_nType)
		{
		case MDB_UINT8_FIELD:
			StringPrint(pString, "%u8", GetAtomX0(uint8));
			break;
		case MDB_INT8_FIELD:
			StringPrint(pString, "%d8", GetAtomX0(int8));
			break;
		case MDB_UINT16_FIELD:
			StringPrint(pString, "%u16", GetAtomX0(uint16));
			break;
		case MDB_INT16_FIELD:
			StringPrint(pString, "%d16", GetAtomX0(int16));
			break;
		case MDB_UINT32_FIELD:
			StringPrint(pString, "%u32", GetAtomX0(uint32));
			break;
		case MDB_INT32_FIELD:
			StringPrint(pString, "%d32", GetAtomX0(int32));
			break;
		case MDB_UINT64_FIELD:
			StringPrint(pString, "%u64", GetAtomX0(uint64));
			break;
		case MDB_INT64_FIELD:
			StringPrint(pString, "%d64", GetAtomX0(int64));
			break;
		case MDB_FLOAT_FIELD:
			StringPrint(pString, "%g", GetAtomX0(float));
			break;
		case MDB_DOUBLE_FIELD:
			StringPrint(pString, "%g", GetAtomX0(double));
			break;
		case MDB_DATE_FIELD:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CDate(GetAtomX0(int32)).Print(oFmt);
			break;
		}
		case MDB_TIME_FIELD:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CTime(GetAtomX0(int32)).Print(oFmt);
			break;
		}
		case MDB_DATETIME_FIELD:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CDateTime(GetAtomX0(double)).Print(oFmt);
			break;
		}
		case MDB_CHAR_FIELD:
		case MDB_LCHAR_FIELD:
			CBinary::MemoryCopy(pString, (char*)GetString(NULL), m_nSize);
			pString[m_nSize] = 0;
			break;
		case MDB_VARCHAR_FIELD:
		case MDB_VARLCHAR_FIELD:
			CString::StringCopy(pString, GetString(NULL));
			break;
		case MDB_RAW_FIELD:
		case MDB_VARRAW_FIELD:
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
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::GetAsString() Failure: m_nType is %u", m_nType));
			break;
		}
}

//NULL和""是等同的。
void CMdbField::SetString(register const char * v)
{
	register uint32 nSize;
	switch(m_nType)
	{
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
		SetNull();
		if(v && v[0])
		{
			CString::StringCopy((char*)m_pData, v, m_nSize);
			FillField();
		}
		break;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
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
		FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetString() Failure: m_nType is %u", m_nType));
		break;
	}
}

//NULL和长度为0的二进制数据是等同的。
void CMdbField::SetRaw(register const void * v, register uint32 nLen)
{
	switch(m_nType)
	{
	case MDB_RAW_FIELD:
		SetNull();
		if(v && nLen)
		{
			CMdbRaw* pRaw = (CMdbRaw*)m_pData;
			if(nLen > m_nSize)
				nLen = m_nSize;
			pRaw->nSize = nLen;
			CBinary::MemoryCopy(pRaw->sData, v, nLen);
			FillField();
		}
		break;
	case MDB_VARRAW_FIELD:
		SetNull();
		if(v && nLen)
		{
			CMdbRaw* &pRaw = *(CMdbRaw**)m_pData;
			if(nLen > m_nSize)
				nLen = m_nSize;
			pRaw = (CMdbRaw*) (new char[sizeof(uint32) + nLen]);
			pRaw->nSize = nLen;
			CBinary::MemoryCopy(pRaw->sData, v, nLen);
			FillField();
		}
		break;
	default:
		FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetRaw() Failure: m_nType is %u", m_nType));
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

void CMdbField::SetFromString(register const char* v)
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
	case MDB_INT8_FIELD:
		if(StringScan(v, "%i8", &i8) == 1)
		{
			GetAtomX0(int8) = i8;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(i8) Failure"));
		break;
	case MDB_INT16_FIELD:
		if(StringScan(v, "%i16", &i16) == 1)
		{
			GetAtomX0(int16) = i16;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(i16) Failure"));
		break;
	case MDB_INT32_FIELD:
		if(StringScan(v, "%i32", &i32) == 1)
		{
			GetAtomX0(int32) = i32;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(i32) Failure"));
		break;
	case MDB_INT64_FIELD:
		if(StringScan(v, "%i64", &i64) == 1)
		{
			GetAtomX0(int64) = i64;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(i64) Failure"));
		break;
	case MDB_UINT8_FIELD:
		if(StringScan(v, "%u8", &u8) == 1)
		{
			GetAtomX0(uint8) = u8;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(u8) Failure"));
		break;
	case MDB_UINT16_FIELD:
		if(StringScan(v, "%u16", &u16) == 1)
		{
			GetAtomX0(uint16) = u16;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(u16) Failure"));
		break;
	case MDB_UINT32_FIELD:
		if(StringScan(v, "%u32", &u32) == 1)
		{
			GetAtomX0(uint32) = u32;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(u32) Failure"));
		break;
	case MDB_UINT64_FIELD:
		if(StringScan(v, "%u64", &u64) == 1)
		{
			GetAtomX0(uint64) = u64;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(u64) Failure"));
		break;
	case MDB_FLOAT_FIELD:
		if(StringScan(v, "%f32", &f32) == 1)
		{
			GetAtomX0(float) = (float)f32;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(f32) Failure"));
		break;
	case MDB_DOUBLE_FIELD:
		if(StringScan(v, "%f64", &f64) == 1)
		{
			GetAtomX0(double) = f64;
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(f64) Failure"));
		break;
	case MDB_DATE_FIELD:
	{
		CDate oDate;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oDate.Scan(oFmt))
		{
			GetAtomX0(int32) = oDate.GetValue();
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(date) Failure"));
		break;
	}
	case MDB_TIME_FIELD:
	{
		CTime oTime;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oTime.Scan(oFmt))
		{
			GetAtomX0(int32) = oTime.GetValue();
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(time) Failure"));
		break;
	}
	case MDB_DATETIME_FIELD:
	{
		CDateTime oDateTime;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oDateTime.Scan(oFmt))
		{
			GetAtomX0(double) = oDateTime.GetValue();
			FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(datetime) Failure"));
		break;
	}
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		SetString(v);
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		nStrLen = CString::StringLength(v);
		if(nStrLen&1)
			FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(raw) Failure"));
		else
		{
			nRawSize = (nStrLen>>1);
			pRaw = new uint8[nRawSize];
			if(StringToRaw(v, (char*)pRaw))
				SetRaw(pRaw, nRawSize);
			else
				FocpLog(FOCP_LOG_WARNING, ("CMdbField::SetFromString(raw) Failure"));
			delete[] pRaw;
		}
		break;
	}
}

template<typename T> void MdbFieldIntOp(T& oLeft, const T& oRight, uint32 nOp)
{
	switch(nOp)
	{
	case MDB_SQLPARA_OPERATOR_EQUAL:
		oLeft = oRight;
		break;
	case MDB_SQLPARA_OPERATOR_ADD:
		oLeft += oRight;
		break;
	case MDB_SQLPARA_OPERATOR_SUB:
		oLeft -= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_MUL:
		oLeft *= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_DIV:
		oLeft /= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_MOD:
		oLeft %= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_BITAND:
		oLeft &= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_BITOR:
		oLeft |= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_BITNOT:
		oLeft = ~oLeft;
		break;
	case MDB_SQLPARA_OPERATOR_BITXOR:
		oLeft ^= oRight;
		break;
	}
}

template<typename T> void MdbFieldFloatOp(T& oLeft, const T& oRight, uint32 nOp)
{
	switch(nOp)
	{
	case MDB_SQLPARA_OPERATOR_EQUAL:
		oLeft = oRight;
		break;
	case MDB_SQLPARA_OPERATOR_ADD:
		oLeft += oRight;
		break;
	case MDB_SQLPARA_OPERATOR_SUB:
		oLeft -= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_MUL:
		oLeft *= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_DIV:
		oLeft /= oRight;
		break;
	case MDB_SQLPARA_OPERATOR_MOD:
		oLeft = oLeft - (oLeft/oRight)*oRight;
		break;
	}
}

template<typename TDateTime, typename TVal> struct CMdbFieldDateTimeOp
{
	inline static void Call(TVal& oLeft, const TVal& oRight, uint32 nOp)
	{
		switch(nOp)
		{
		case MDB_SQLPARA_OPERATOR_EQUAL:
			oLeft = oRight;
			break;
		case MDB_SQLPARA_OPERATOR_ADD:
			oLeft = TDateTime(oLeft+oRight).GetValue();
			break;
		case MDB_SQLPARA_OPERATOR_SUB:
			oLeft = TDateTime(oLeft-oRight).GetValue();
			break;
		}
	}
};

void CMdbField::SetFromField(register CMdbField* pValue, uint32 nOp)
{
	if(m_nType != pValue->m_nType)
		return;

	if(nOp == MDB_SQLPARA_OPERATOR_EQUAL)
		SetNull();

	if(!pValue || IS_NULL_A(pValue))
	{
		if(nOp != MDB_SQLPARA_OPERATOR_BITNOT)
			return;
	}

	if(IsNull())
	{
		if(nOp == MDB_SQLPARA_OPERATOR_ADD)
		{
			switch(m_nType)
			{
			default:
				return;
			case MDB_CHAR_FIELD:
			case MDB_LCHAR_FIELD:
			case MDB_RAW_FIELD:
			case MDB_VARCHAR_FIELD:
			case MDB_VARLCHAR_FIELD:
			case MDB_VARRAW_FIELD:
				nOp = MDB_SQLPARA_OPERATOR_EQUAL;
				break;
			}
		}
		if(nOp != MDB_SQLPARA_OPERATOR_EQUAL)
			return;
	}

	switch(m_nType)
	{
	case MDB_UINT8_FIELD:
		MdbFieldIntOp(GetAtomX0(int8) , GetAtomX(int8, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_INT8_FIELD:
		MdbFieldIntOp(GetAtomX0(uint8) , GetAtomX(uint8, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_UINT16_FIELD:
		MdbFieldIntOp(GetAtomX0(uint16) , GetAtomX(uint16, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_INT16_FIELD:
		MdbFieldIntOp(GetAtomX0(int16) , GetAtomX(int16, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_FLOAT_FIELD:
		MdbFieldFloatOp(GetAtomX0(float) , GetAtomX(float, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_UINT32_FIELD:
		MdbFieldIntOp(GetAtomX0(uint32) , GetAtomX(uint32, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_INT32_FIELD:
		MdbFieldIntOp(GetAtomX0(int32) , GetAtomX(int32, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_DOUBLE_FIELD:
		MdbFieldFloatOp(GetAtomX0(double) , GetAtomX(double, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_DATE_FIELD:
		CMdbFieldDateTimeOp<CDate, int32>::Call(GetAtomX0(int32), GetAtomX(int32, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_TIME_FIELD:
		CMdbFieldDateTimeOp<CTime, int32>::Call(GetAtomX0(int32), GetAtomX(int32, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_DATETIME_FIELD:
		CMdbFieldDateTimeOp<CDateTime, double>::Call(GetAtomX0(double), GetAtomX(double, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_UINT64_FIELD:
		MdbFieldIntOp(GetAtomX0(uint64) , GetAtomX(uint64, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_INT64_FIELD:
		MdbFieldIntOp(GetAtomX0(int64) , GetAtomX(int64, pValue), nOp);
		if(IsNull()) FillField();
		break;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
		if(nOp == MDB_SQLPARA_OPERATOR_EQUAL)
		{
			char* sRight = (char*)pValue->GetString(NULL);
			CString oStr2(pValue->m_nSize?sRight:"", pValue->m_nSize);
			SetString(oStr2.GetStr());
		}
		else if(nOp == MDB_SQLPARA_OPERATOR_ADD)
		{
			register const char* sLeft = (const char*)GetString(NULL);
			register const char* sRight = (const char*)pValue->GetString(NULL);
			if(!sLeft) sLeft = "";
			if(!sRight) sRight = "";
			register uint32 nSize0 = CString::StringLength(sLeft, m_nSize);
			register uint32 nSize1 = CString::StringLength(sRight, pValue->m_nSize);
			CString oStr1(sLeft, nSize0);
			CString oStr2(sRight, nSize1);
			oStr1 += oStr2;
			SetString(oStr1.GetStr());
		}
		break;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		if(nOp == MDB_SQLPARA_OPERATOR_EQUAL)
			SetString(pValue->GetString(NULL));
		else if(nOp == MDB_SQLPARA_OPERATOR_ADD)
		{
			register const char* sLeft = (const char*)GetString(NULL);
			register const char* sRight = (const char*)pValue->GetString(NULL);
			if(!sLeft) sLeft = "";
			if(!sRight) sRight = "";
			register uint32 nSize0 = CString::StringLength(sLeft);
			register uint32 nSize1 = CString::StringLength(sRight);
			CString oStr1(sLeft, nSize0);
			CString oStr2(sRight, nSize1);
			oStr1 += oStr2;
			SetString(oStr1.GetStr());
		}
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		if(nOp == MDB_SQLPARA_OPERATOR_EQUAL)
		{
			register uint8* pRaw;
			register uint32 nLen;
			pRaw = (uint8*)pValue->GetRaw(nLen);
			SetRaw(pRaw, nLen);
		}
		else if(nOp == MDB_SQLPARA_OPERATOR_ADD)
		{
			register uint8* pRaw1;
			register uint32 nLen1;
			pRaw1 = (uint8*)pValue->GetRaw(nLen1);
			if(nLen1)
			{
				register uint8* pRaw0;
				register uint32 nLen0;
				pRaw0 = (uint8*)GetRaw(nLen0);
				if(nLen0)
				{
					CBinary oBin;
					register uint32 nOff=0;
					oBin.Write(nOff, pRaw0, nLen0);
					nOff += nLen0;
					oBin.Write(nOff, pRaw1, nLen1);
					SetRaw((const void*)oBin.GetData(), oBin.GetSize());
				}
				else
					SetRaw(pRaw1, nLen1);
			}
		}
		break;
	}
}

void* CMdbField::GetData(uint32 &nSize)
{
	switch(m_nType)
	{
	case MDB_UINT8_FIELD:
	case MDB_INT8_FIELD:
		nSize = sizeof(uint8);
		break;
	case MDB_UINT16_FIELD:
	case MDB_INT16_FIELD:
		nSize = sizeof(uint16);
		break;
	case MDB_UINT32_FIELD:
	case MDB_INT32_FIELD:
	case MDB_DATE_FIELD:
	case MDB_TIME_FIELD:
		nSize = sizeof(uint32);
		break;
	case MDB_UINT64_FIELD:
	case MDB_INT64_FIELD:
		nSize = sizeof(uint64);
		break;
	case MDB_FLOAT_FIELD:
		nSize = sizeof(float);
		break;
	case MDB_DOUBLE_FIELD:
	case MDB_DATETIME_FIELD:
		nSize = sizeof(double);
		break;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		return GetString(&nSize);
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		return GetRaw(nSize);
		break;
	}
	return m_pData;
}

template<typename T> bool IsCoincident(const T& t1, const T& t2, register uint32 nOpr)
{
	switch(nOpr)
	{
	case MDB_SQLPARA_OPERATOR_LESS:
		return (t1 < t2);
	case MDB_SQLPARA_OPERATOR_EQUAL:
		return (t1 == t2);
	case MDB_SQLPARA_OPERATOR_MORE:
		return (t1 > t2);
	case MDB_SQLPARA_OPERATOR_LESSEQUAL:
		return (t1 <= t2);
	case MDB_SQLPARA_OPERATOR_MOREEQUAL:
		return (t1 >= t2);
	case MDB_SQLPARA_OPERATOR_NOTEQUAL:
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

int32 CMdbField::Compare(register CMdbField* pValue)
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
	case MDB_INT8_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int8), GetAtomX(int8, pValue));
		break;
	case MDB_INT16_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int16), GetAtomX(int16, pValue));
		break;
	case MDB_INT32_FIELD:
	case MDB_DATE_FIELD:
	case MDB_TIME_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int32), GetAtomX(int32, pValue));
		break;
	case MDB_INT64_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(int64), GetAtomX(int64, pValue));
		break;
	case MDB_UINT8_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint8), GetAtomX(uint8, pValue));
		break;
	case MDB_UINT16_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint16), GetAtomX(uint16, pValue));
		break;
	case MDB_UINT32_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint32), GetAtomX(uint32, pValue));
		break;
	case MDB_UINT64_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(uint64), GetAtomX(uint64, pValue));
		break;
	case MDB_FLOAT_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(float), GetAtomX(float, pValue));
		break;
	case MDB_DOUBLE_FIELD:
	case MDB_DATETIME_FIELD:
		nRet = FOCP_NAME::Compare(GetAtomX0(double), GetAtomX(double, pValue));
		break;
	case MDB_CHAR_FIELD:
	case MDB_VARCHAR_FIELD:
		s1 = GetString(&nStrLen1);
		s2 = pValue->GetString(&nStrLen2);
		nCmpLen = nStrLen1;
		if(nStrLen1 > nStrLen2)
			nCmpLen = nStrLen2;
		nRet = (int32)CBinary::MemoryCompare(s1, s2, nCmpLen);
		if(!nRet && nStrLen1 != nStrLen2)
			nRet = (nStrLen1>nStrLen2)?1:(-1);
		break;
	case MDB_LCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		s1 = GetString(&nStrLen1);
		s2 = pValue->GetString(&nStrLen2);
		nCmpLen = nStrLen1;
		if(nStrLen1 > nStrLen2)
			nCmpLen = nStrLen2;
		nRet = CBinary::MemoryCompare((void*)s1, (void*)s2, nCmpLen, false);
		if(!nRet && nStrLen1 != nStrLen2)
			nRet = (nStrLen1>nStrLen2)?1:(-1);
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
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

bool CMdbField::IsCoincident(register CMdbField* pValue, register uint32 nOpr)
{
	register int32 nCmp =  Compare(pValue);
	return FOCP_NAME::IsCoincident(nCmp, (int32)0, nOpr)?true:false;
}

uint32 CMdbField::GetHashValue()
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
	case MDB_INT8_FIELD:
	case MDB_UINT8_FIELD:
		nRet = (uint32)GetAtomX0(uint8);
		break;
	case MDB_UINT16_FIELD:
	case MDB_INT16_FIELD:
		nRet = (uint32)GetAtomX0(uint16);
		break;
	case MDB_FLOAT_FIELD:
	case MDB_INT32_FIELD:
	case MDB_UINT32_FIELD:
	case MDB_DATE_FIELD:
	case MDB_TIME_FIELD:
		nRet = GetAtomX0(uint32);
		break;
	default:
		bCaseSentive = 1;
		switch(m_nType)
		{
		case MDB_INT64_FIELD:
		case MDB_UINT64_FIELD:
		case MDB_DOUBLE_FIELD:
		case MDB_DATETIME_FIELD:
			i64 = GetAtomX0(int64);
			pRaw = (uint8*)&i64;
			nRawSize = 8;
			break;
		case MDB_CHAR_FIELD:
		case MDB_VARCHAR_FIELD:
			pRaw = (uint8*)GetString(&nRawSize);
			break;
		case MDB_LCHAR_FIELD:
		case MDB_VARLCHAR_FIELD:
			pRaw = (uint8*)GetString(&nRawSize);
			bCaseSentive = 0;
		case MDB_RAW_FIELD:
		case MDB_VARRAW_FIELD:
			pRaw = (uint8*)GetRaw(nRawSize);
			break;
		}
		nRet = GetCrc32((const uint8*)pRaw, nRawSize, bCaseSentive);
		break;
	}
	return nRet;
}

bool CMdbField::Write(CMemoryStream & oStream)
{
	register uint32 nLen;
	register char* s;

	register uint8 bIsNull = (uint8)IS_NULL();
	uint32 nPos1 = oStream.GetPosition();
	//为支持平滑升级，需要存储该字段大小，以便跳跃该字段解析。
	register bool bRet = oStream.Write((uint32)0);
	if(bRet)bRet = oStream.Write(bIsNull);
	if(bRet && !bIsNull)
	{
		bRet = oStream.Write(m_nType);
		if(bRet)switch(m_nType)
		{
		case MDB_INT8_FIELD:
			bRet = oStream.Write(GetAtomX0(int8));
			break;
		case MDB_INT16_FIELD:
			bRet = oStream.Write(GetAtomX0(int16));
			break;
		case MDB_INT32_FIELD:
		case MDB_DATE_FIELD:
		case MDB_TIME_FIELD:
			bRet = oStream.Write(GetAtomX0(int32));
			break;
		case MDB_INT64_FIELD:
			bRet = oStream.Write(GetAtomX0(int64));
			break;
		case MDB_UINT8_FIELD:
			bRet = oStream.Write(GetAtomX0(uint8));
			break;
		case MDB_UINT16_FIELD:
			bRet = oStream.Write(GetAtomX0(uint16));
			break;
		case MDB_UINT32_FIELD:
			bRet = oStream.Write(GetAtomX0(uint32));
			break;
		case MDB_UINT64_FIELD:
			bRet = oStream.Write(GetAtomX0(uint64));
			break;
		case MDB_FLOAT_FIELD:
			bRet = oStream.Write(GetAtomX0(float));
			break;
		case MDB_DOUBLE_FIELD:
		case MDB_DATETIME_FIELD:
			bRet = oStream.Write(GetAtomX0(double));
			break;
		case MDB_CHAR_FIELD:
		case MDB_LCHAR_FIELD:
		case MDB_VARCHAR_FIELD:
		case MDB_VARLCHAR_FIELD:
			s = GetString(&nLen);
			bRet = oStream.Write(nLen);
			if(bRet && nLen)
				bRet = (nLen == oStream.Write(s, nLen));
			break;
		case MDB_RAW_FIELD:
		case MDB_VARRAW_FIELD:
			s = (char*)GetRaw(nLen);
			bRet = oStream.Write(nLen);
			if(bRet && nLen)
				bRet = (nLen == oStream.Write(s, nLen));
			break;
		}
	}
	if(bRet)
	{
		//为支持平滑升级，需要存储该字段大小，以便跳跃该字段解析。
		uint32 nPos2 = oStream.GetPosition();
		uint32 nFieldSize = nPos2 - nPos1;
		oStream.SetPosition(nPos1);
		oStream.Write(nFieldSize);
		oStream.SetPosition(nPos2);
	}
	return bRet;
}

bool CMdbField::Read(CMemoryStream & oStream)
{
	register uint32 nLen;
	register char* s;
	register uint8 bIsNull;

	uint32 nFieldSize;
	uint32 nPos1 = oStream.GetPosition();
	register bool bRet = oStream.Read(nFieldSize);
	if(bRet)bRet = oStream.Read(bIsNull);
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
				default:
					bRet = false;
					break;
				case MDB_INT8_FIELD:
					bRet = oStream.Read(GetAtomX0(int8));
					if(bRet)FillField();
					break;
				case MDB_INT16_FIELD:
					bRet = oStream.Read(GetAtomX0(int16));
					if(bRet)FillField();
					break;
				case MDB_INT32_FIELD:
				case MDB_DATE_FIELD:
				case MDB_TIME_FIELD:
					bRet = oStream.Read(GetAtomX0(int32));
					if(bRet)FillField();
					break;
				case MDB_INT64_FIELD:
					bRet = oStream.Read(GetAtomX0(int64));
					if(bRet)FillField();
					break;
				case MDB_UINT8_FIELD:
					bRet = oStream.Read(GetAtomX0(uint8));
					if(bRet)FillField();
					break;
				case MDB_UINT16_FIELD:
					bRet = oStream.Read(GetAtomX0(uint16));
					if(bRet)FillField();
					break;
				case MDB_UINT32_FIELD:
					bRet = oStream.Read(GetAtomX0(uint32));
					if(bRet)FillField();
					break;
				case MDB_UINT64_FIELD:
					bRet = oStream.Read(GetAtomX0(uint64));
					if(bRet)FillField();
					break;
				case MDB_FLOAT_FIELD:
					bRet = oStream.Read(GetAtomX0(float));
					if(bRet)FillField();
					break;
				case MDB_DOUBLE_FIELD:
				case MDB_DATETIME_FIELD:
					bRet = oStream.Read(GetAtomX0(double));
					if(bRet)FillField();
					break;
				case MDB_CHAR_FIELD:
				case MDB_LCHAR_FIELD:
				case MDB_VARCHAR_FIELD:
				case MDB_VARLCHAR_FIELD:
					bRet = oStream.Read(nLen);
					if(bRet)
					{
						if(nLen)
						{
							s = new char[nLen+1];
							bRet = (nLen == oStream.Read(s, nLen));
							if(bRet)
							{
								s[nLen] = 0;
								SetString(s);
							}
							delete[] s;
						}
						else
							SetString("");
					}
					break;
				case MDB_RAW_FIELD:
				case MDB_VARRAW_FIELD:
					bRet = oStream.Read(nLen);
					if(bRet && nLen)
					{
						s = new char[nLen];
						bRet = (nLen == oStream.Read(s, nLen));
						if(bRet)
							SetRaw(s, nLen);
						delete[] s;
					}
					break;
				}
			}
		}
	}
	if(bRet)
	{
		uint32 nPos2 = oStream.GetPosition();
		if(nPos2 - nPos1 != nFieldSize)
			bRet = false;
	}
	return bRet;
}

uint32 CMdbField::GetType()
{
	return m_nType;
}

bool CMdbField::IsNull()
{
	return IS_NULL_A(this);
}

int8 CMdbField::GetInt8()
{
	return GetAtomX(int8, this);
}

int16 CMdbField::GetInt16()
{
	return GetAtomX(int16, this);
}

int32 CMdbField::GetInt32()
{
	return GetAtomX(int32, this);
}

int64 CMdbField::GetInt64()
{
	return GetAtomX(int64, this);
}

uint8 CMdbField::GetUInt8()
{
	return GetAtomX(uint8, this);
}

uint16 CMdbField::GetUInt16()
{
	return GetAtomX(uint16, this);
}

uint32 CMdbField::GetUInt32()
{
	return GetAtomX(uint32, this);
}

uint64 CMdbField::GetUInt64()
{
	return GetAtomX(uint64, this);
}

float CMdbField::GetFloat()
{
	return GetAtomX(float, this);
}

double CMdbField::GetDouble()
{
	return GetAtomX(double, this);
}

//-----------------------------------------------------------------------------
// CMdbRecord
//----------------------------------------------------------------------------
CMdbRecord::CMdbRecord(CMdbTableDef * pTabDef, uint8* pRecordData)
{
	m_pTabDef = pTabDef;
	if(pRecordData)
	{
		m_bOwner = false;
		m_pRecordData = pRecordData;
	}
	else
	{
		register uint32 nRecordSize = m_pTabDef->pExtendAttr->nRecordSize;
		m_pRecordData = new uint8[nRecordSize];
		CBinary::MemorySet(m_pRecordData, 0, nRecordSize);
		m_bOwner = true;
	}
	m_pFieldTable = NULL;
}

CMdbRecord::~CMdbRecord()
{
	if(m_bOwner)
	{
		Clear();
		delete[] m_pRecordData;
	}
	m_pRecordData = NULL;
	if(m_pFieldTable)
		delete[] m_pFieldTable;
	m_pFieldTable = NULL;
}

CMdbField* CMdbRecord::GetField(register uint32 nFieldNo)
{
	if(!m_pFieldTable)
		m_pFieldTable = new CMdbField[m_pTabDef->nFieldCount];
	register CMdbField* pField = m_pFieldTable + nFieldNo;
	if(!pField->m_pData)
	{
		register CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + nFieldNo;
		register CMdbFieldAttr* pFieldAttr = pFieldDef->pExtendAttr;
		pField->Initialize(pFieldDef->nType, pFieldDef->nLen, //数据大小
						   m_pRecordData, nFieldNo, pFieldAttr->nOffset);
	}
	return pField;
}

void CMdbRecord::GetField(register uint32 nFieldNo, CMdbField& oField)
{
	register CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + nFieldNo;
	register CMdbFieldAttr* pFieldAttr = pFieldDef->pExtendAttr;
	oField.Initialize(pFieldDef->nType, pFieldDef->nLen,
					  m_pRecordData, nFieldNo, pFieldAttr->nOffset);
}

void CMdbRecord::Clear()
{
	if(m_pRecordData)
	{
		register CMdbTableAttr* pTableAttr = m_pTabDef->pExtendAttr;
		register uint32 nVarFieldCount = pTableAttr->nVarFieldCount;
		if(nVarFieldCount)
		{
			register char* pData = (char*)m_pRecordData;
			register uint32* pVarFieldList = pTableAttr->pVarFieldList;
			for(register uint32 i=0; i<nVarFieldCount; ++i)
			{
				register char* &pFieldData = *(char**)(pData + m_pTabDef->pFieldDefines[pVarFieldList[i]].pExtendAttr->nOffset);
				if(pFieldData)
				{
					delete[] pFieldData;
					pFieldData = NULL;
				}
			}
		}
		CBinary::MemorySet(m_pRecordData, 0, GetFieldFlagCount(m_pTabDef)<<2);
	}
}

bool CMdbRecord::Write(CMemoryStream & oStream, CMdbSqlFilter &oFilter)
{
	register uint32 nCol, i, nTotal, nCount = oFilter.m_nParaCount;
	if(!nCount)
		nTotal = m_pTabDef->nFieldCount;
	else
		nTotal = nCount;
	if(!oStream.Write(nTotal))
		return false;
	for(i=0; i<nTotal; ++i)
	{
		if(nCount)
			nCol = oFilter.m_pParaTable[i];
		else
			nCol = i;
		register CMdbField* pField = GetField(nCol);
		if(!oStream.Write(nCol))
			return false;
		if(!pField->Write(oStream))
			return false;
	}
	return true;
}

bool CMdbRecord::Read(CMemoryStream & oStream)
{
	register uint32 nCount;

	Clear();
	if(!oStream.Read(nCount))
		return false;
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint32 nCol;
		if(!oStream.Read(nCol))
			return false;
		register CMdbField* pField = GetField(nCol);
		if(pField)
		{
			if(!pField->Read(oStream))
				return false;
		}
		else
		{
			//考虑平滑升级
			uint32 nFieldSize;
			if(!oStream.Read(nFieldSize))
				return false;
			if(nFieldSize <= sizeof(nFieldSize))
				return false;
			nFieldSize -= sizeof(nFieldSize);
			if(!oStream.Seek(nFieldSize))
				return false;
		}
	}
	return true;
}

uint32 CMdbRecord::GetType(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->m_nType;
}

bool CMdbRecord::IsNull(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return IS_NULL_A(pField);
}

void CMdbRecord::SetNull(register uint32 nFieldNo)
{
	GetField(nFieldNo)->SetNull();
}

int8 CMdbRecord::GetInt8(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(int8, pField);
}

int16 CMdbRecord::GetInt16(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(int16, pField);
}

int32 CMdbRecord::GetInt32(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(int32, pField);
}

int64 CMdbRecord::GetInt64(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(int64, pField);
}

uint8 CMdbRecord::GetUInt8(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(uint8, pField);
}

uint16 CMdbRecord::GetUInt16(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(uint16, pField);
}

uint32 CMdbRecord::GetUInt32(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(uint32, pField);
}

uint64 CMdbRecord::GetUInt64(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(uint64, pField);
}

float CMdbRecord::GetFloat(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(float, pField);
}

double CMdbRecord::GetDouble(register uint32 nFieldNo)
{
	register CMdbField* pField = GetField(nFieldNo);
	return GetAtomX(double, pField);
}

char* CMdbRecord::GetString(register uint32 nFieldNo, register uint32 * pStrLen)
{
	return GetField(nFieldNo)->GetString(pStrLen);
}

void* CMdbRecord::GetRaw(register uint32 nFieldNo, register uint32 &nRawSize)
{
	return GetField(nFieldNo)->GetRaw(nRawSize);
}

CDate CMdbRecord::GetDate(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->GetDate();
}

CTime CMdbRecord::GetTime(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->GetTime();
}

CDateTime CMdbRecord::GetDateTime(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->GetDateTime();
}

uint32 CMdbRecord::GetStringSize(register uint32 nFieldNo)
{
	return GetField(nFieldNo)->GetStringSize();
}

void CMdbRecord::GetAsString(register uint32 nFieldNo, register char * pString)
{
	GetField(nFieldNo)->GetAsString(pString);
}

void CMdbRecord::SetInt8(register uint32 nFieldNo, register int8 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int8, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetInt16(register uint32 nFieldNo, register int16 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int16, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetInt32(register uint32 nFieldNo, register int32 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int32, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetInt64(register uint32 nFieldNo, register int64 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int64, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetUInt8(register uint32 nFieldNo, register uint8 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(uint8, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetUInt16(register uint32 nFieldNo, register uint16 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(uint16, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetUInt32(register uint32 nFieldNo, register uint32 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(uint32, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetUInt64(register uint32 nFieldNo, register uint64 v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(uint64, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetFloat(register uint32 nFieldNo, register float v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(float, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetDouble(register uint32 nFieldNo, register double v)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(double, pField) = v;
	FillFieldA(pField);
}

void CMdbRecord::SetString(register uint32 nFieldNo, register const char * v)
{
	GetField(nFieldNo)->SetString(v);
}

void CMdbRecord::SetRaw(register uint32 nFieldNo, register const void * v, uint32 nLen)
{
	GetField(nFieldNo)->SetRaw(v, nLen);
}

void CMdbRecord::SetDate(register uint32 nFieldNo, const CDate& oDate)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int32, pField) = oDate.GetValue();
	FillFieldA(pField);
}

void CMdbRecord::SetTime(register uint32 nFieldNo, const CTime& oTime)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(int32, pField) = oTime.GetValue();
	FillFieldA(pField);
}

void CMdbRecord::SetDateTime(register uint32 nFieldNo, const CDateTime& oDateTime)
{
	register CMdbField* pField = GetField(nFieldNo);
	GetAtomX(double, pField) = oDateTime.GetValue();
	FillFieldA(pField);
}

void CMdbRecord::SetFromString(register uint32 nFieldNo, register const char* v)
{
	GetField(nFieldNo)->SetFromString(v);
}

void CMdbRecord::CopyFrom(CMdbSqlFilter &oFilter, register CMdbRecord* pRecord)
{
	CMdbField oSrcField, oDstField;
	register CMdbField *pSrcField, *pDstField;
	register uint32 nCol, i, nTotal, nCount = oFilter.m_nParaCount;
	if(!nCount)
		nTotal = m_pTabDef->nFieldCount;
	else
		nTotal = nCount;
	for(i=0; i<nTotal; ++i)
	{
		if(nCount)
			nCol = oFilter.m_pParaTable[i];
		else
			nCol = i;
		if(m_bOwner)
			pDstField = GetField(nCol);
		else
		{
			GetField(nCol, oDstField);
			pDstField = &oDstField;
		}
		if(pRecord->m_bOwner)
			pSrcField = pRecord->GetField(nCol);
		else
		{
			pRecord->GetField(nCol, oSrcField);
			pSrcField = &oSrcField;
		}
		pDstField->SetFromField(pSrcField);
	}
}

int32 CMdbRecord::Compare(register CMdbRecord* pRecord, uint32 nFieldCount, uint32 *pFields)
{
	uint32 i;
	int32 nRet = 0;
	for(i=0; i<nFieldCount; ++i)
	{
		nRet = GetField(pFields[i])->Compare(pRecord->GetField(pFields[i]));
		if(nRet)
			break;
	}
	return nRet;
}

//-----------------------------------------------------------------------------
// CMdbRecordSet
//----------------------------------------------------------------------------
CMdbRecordSet::CMdbRecordSet(CMdbTableDef * pTabDef)
{
	m_pTabDef = pTabDef;
	m_nSetSize = 0;
	m_pRecordSet = NULL;
	m_nRecordCount = 0;
}

CMdbRecordSet::~CMdbRecordSet()
{
	Clear();
}

void CMdbRecordSet::PreAlloc()
{
	m_nRecordCount = m_nSetSize;
}

CMdbRecord* CMdbRecordSet::AllocRecord()
{
	register CMdbRecord* pRecord = NULL;
	if(m_nRecordCount < m_nSetSize)
		pRecord = m_pRecordSet[m_nRecordCount++];
	return pRecord;
}

void CMdbRecordSet::PopRecord()
{
	if(m_nRecordCount)
	{
		--m_nRecordCount;
		m_pRecordSet[m_nRecordCount]->Clear();
	}
}

uint32 CMdbRecordSet::GetRecordSetSize()
{
	return m_nSetSize;
}

void CMdbRecordSet::SetRecordSetSize(uint32 nSize)
{
	if(nSize != m_nSetSize)
	{
		register CMdbRecord** pRecSet = new CMdbRecord*[nSize];
		register uint32 i, nRest = nSize;
		if(nRest > m_nSetSize)
			nRest = m_nSetSize;
		for(i=0; i<nRest; ++i)
			pRecSet[i] = m_pRecordSet[i];
		for(; i<nSize; ++i)
			pRecSet[i] = new CMdbRecord(m_pTabDef);
		for(; i<m_nSetSize; ++i)
			delete m_pRecordSet[i];
		if(m_pRecordSet)
			delete[] m_pRecordSet;
		m_pRecordSet = pRecSet;
		m_nSetSize = nSize;
	}
	m_nRecordCount = 0;
}

uint32 CMdbRecordSet::GetRecordCount()
{
	return m_nRecordCount;
}

CMdbRecord* CMdbRecordSet::GetRecord(uint32 nRecNo)
{
	if(nRecNo < m_nRecordCount)
		return m_pRecordSet[nRecNo];
	return NULL;
}

uint32 CMdbRecordSet::GetResultCount()
{
	return m_nRecordCount;
}

CMdbResult* CMdbRecordSet::GetResult(uint32 nRecordNo)
{
	if(nRecordNo < m_nRecordCount)
		return m_pRecordSet[nRecordNo];
	return NULL;
}

static void ReverseBlock(CMdbRecord** pBlock, const uint32 nBlockSize)
{
	if(nBlockSize < 2)
		return;
	CMdbRecord** pEnd = pBlock + nBlockSize - 1;
	for(; pBlock < pEnd; ++pBlock, --pEnd)
	{
		CMdbRecord* pTmp = *pBlock;
		*pBlock = *pEnd;
		*pEnd = pTmp;
	}
}

void CMdbRecordSet::Sort(bool bAsc, uint32 nFieldCount, uint32* pFields, uint32 nSegCount, uint32* pSeg)
{
	if(pSeg)
	{
		if(m_nRecordCount > 1 && nSegCount > 1)for(uint32 i=0; i<nSegCount; ++i)
			{
				for(uint32 j=nSegCount; j; --j)
				{
					uint32 nIdx1=pSeg[j-1], nIdx2=pSeg[j-2];
					CMdbRecord* &pRec1 = m_pRecordSet[nIdx1];
					CMdbRecord* &pRec2 = m_pRecordSet[nIdx2];
					int32 nCmp = pRec1->Compare(pRec2, nFieldCount, pFields);
					if((nCmp<0 && bAsc) || (nCmp>0 && !bAsc))
					{
						uint32 nHeadSize = nIdx1 - nIdx2;
						ReverseBlock(&pRec2, nHeadSize);
						ReverseBlock(&pRec2 + nHeadSize, pSeg[j] - nIdx1);
						ReverseBlock(&pRec2, pSeg[j] - nIdx2);
					}
				}
			}
	}
	else if(m_nRecordCount > 1)for(uint32 i=0; i<m_nRecordCount; ++i)
		{
			for(uint32 j=m_nRecordCount; j; --j)
			{
				CMdbRecord* &pRec1 = m_pRecordSet[j-1];
				CMdbRecord* &pRec2 = m_pRecordSet[j-2];
				int32 nCmp = pRec1->Compare(pRec2, nFieldCount, pFields);
				if((nCmp<0 && bAsc) || (nCmp>0 && !bAsc))
				{
					CMdbRecord* pTmp = pRec2;
					pRec2 = pRec1;
					pRec1 = pTmp;
				}
			}
		}
}

void CMdbRecordSet::Clear()
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

bool CMdbRecordSet::Write(CMemoryStream & oStream, CMdbSqlFilter &oFilter)
{
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write(m_nRecordCount);
	for(uint32 i=0; bRet && i<m_nRecordCount; ++i)
		bRet = m_pRecordSet[i]->Write(oStream, oFilter);
	return bRet;
}

bool CMdbRecordSet::Read(CMemoryStream & oStream)
{
	uint32 nCount;
	Clear();
	bool bRet = oStream.Read(nCount);
	if(bRet && nCount)
	{
		SetRecordSetSize(nCount);
		for(uint32 i=0; bRet && i<nCount; ++i)
			bRet = AllocRecord()->Read(oStream);
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// CMdbSqlFilter
//----------------------------------------------------------------------------
CMdbSqlFilter::CMdbSqlFilter(CMdbTableDef* pTabDef)
{
	m_pTabDef = pTabDef;
	m_nFlgCount = GetFieldFlagCount(m_pTabDef);
	m_nParaCount = 0;
	m_pParaTable = new uint32[m_pTabDef->nFieldCount];
	m_pFlag = new uint32[m_nFlgCount];
	for(uint32 i=0; i<m_nFlgCount; ++i)
		m_pFlag[i] = 0;
}

CMdbSqlFilter::~CMdbSqlFilter()
{
	delete[] m_pParaTable;
	delete[] m_pFlag;
}

void CMdbSqlFilter::SetField(uint32 nFieldNo)
{
	if(nFieldNo < m_pTabDef->nFieldCount)
	{
		register uint32 nFlg = MDB_FLG_L(nFieldNo);
		register uint32 nBit = MDB_FLG_B(nFieldNo);
		if(!(m_pFlag[nFlg] & nBit))
		{
			m_pParaTable[m_nParaCount++] = nFieldNo;
			m_pFlag[nFlg] |= nBit;
		}
	}
}

void CMdbSqlFilter::Clear()
{
	if(m_nParaCount)
	{
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			m_pFlag[i] = 0;
		m_nParaCount = 0;
	}
}

void CMdbSqlFilter::SetAllField()
{
	register uint32 nFieldCount = m_pTabDef->nFieldCount;
	if(m_nParaCount != nFieldCount)
	{
		register uint32 *pParaTable = m_pParaTable;
		register uint32 *pFlag = m_pFlag;
		for(register uint32 i=0; i<nFieldCount; ++i)
		{
			pParaTable[i] = i;
			pFlag[MDB_FLG_L(i)] |= MDB_FLG_B(i);
		}
		m_nParaCount = nFieldCount;
	}
}

bool CMdbSqlFilter::IsSetField(uint32 nFieldNo)
{
	if(nFieldNo >= m_pTabDef->nFieldCount)
		return false;
	if(m_pFlag[MDB_FLG_L(nFieldNo)] & MDB_FLG_B(nFieldNo))
		return true;
	return false;
}

uint32 CMdbSqlFilter::GetParaCount()
{
	return m_nParaCount;
}

uint32 CMdbSqlFilter::GetPara(uint32 nParaIdx)
{
	if(nParaIdx >= m_nParaCount)
		return 0xFFFFFFFF;
	return m_pParaTable[nParaIdx];
}

bool CMdbSqlFilter::Write(CMemoryStream & oStream)
{
	if(!oStream.Write(m_nParaCount))
		return false;
	if(m_nParaCount)
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
			if(!oStream.Write(m_pParaTable[i]))
				return false;
	}
	return true;
}

bool CMdbSqlFilter::Read(CMemoryStream & oStream)
{
	register uint32 nCount;
	if(!oStream.Read(nCount))
		return false;
	Clear();
	if(nCount)
	{
		//为支持平滑升级，故不对nCount做检查
		for(uint32 i=0; i<nCount; ++i)
		{
			uint32 nCol;
			if(!oStream.Read(nCol))
				return false;
			SetField(nCol);
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// CMdbSqlPara
//----------------------------------------------------------------------------
CMdbSqlPara::CMdbSqlPara(CMdbTableDef * pTabDef)
	:m_oRecord(pTabDef)
{
	m_pTabDef = pTabDef;
	m_nFlgCount = GetFieldFlagCount(m_pTabDef);
	m_nParaCount = 0;
	m_pParaTable = new CParaItem[m_pTabDef->nFieldCount];
	m_pFlag = new uint32[m_nFlgCount];
	for(register uint32 i=0; i<m_nFlgCount; ++i)
		m_pFlag[i] = 0;
	m_bError = false;
}

CMdbSqlPara::~CMdbSqlPara()
{
	delete[] m_pParaTable;
	delete[] m_pFlag;
}

CMdbRecord& CMdbSqlPara::GetRecord()
{
	return m_oRecord;
}

static bool CheckOperator(uint32 nType, uint32 nOperator)
{
	switch(nOperator)
	{
	case MDB_SQLPARA_OPERATOR_LESS:
	case MDB_SQLPARA_OPERATOR_EQUAL:
	case MDB_SQLPARA_OPERATOR_MORE:
	case MDB_SQLPARA_OPERATOR_LESSEQUAL:
	case MDB_SQLPARA_OPERATOR_MOREEQUAL:
	case MDB_SQLPARA_OPERATOR_NOTEQUAL:
	case MDB_SQLPARA_OPERATOR_ADD:
		return true;
	case MDB_SQLPARA_OPERATOR_SUB:
		switch(nType)
		{
		case MDB_DATE_FIELD:
		case MDB_TIME_FIELD:
		case MDB_DATETIME_FIELD:
			return true;
		}
	case MDB_SQLPARA_OPERATOR_MUL:
	case MDB_SQLPARA_OPERATOR_DIV:
	case MDB_SQLPARA_OPERATOR_MOD:
		switch(nType)
		{
		case MDB_INT8_FIELD:
		case MDB_INT16_FIELD:
		case MDB_INT32_FIELD:
		case MDB_INT64_FIELD:
		case MDB_UINT8_FIELD:
		case MDB_UINT16_FIELD:
		case MDB_UINT32_FIELD:
		case MDB_UINT64_FIELD:
		case MDB_FLOAT_FIELD:
		case MDB_DOUBLE_FIELD:
			return true;
		}
		break;
	case MDB_SQLPARA_OPERATOR_BITAND:
	case MDB_SQLPARA_OPERATOR_BITOR:
	case MDB_SQLPARA_OPERATOR_BITNOT:
	case MDB_SQLPARA_OPERATOR_BITXOR:
		switch(nType)
		{
		case MDB_INT8_FIELD:
		case MDB_INT16_FIELD:
		case MDB_INT32_FIELD:
		case MDB_INT64_FIELD:
		case MDB_UINT8_FIELD:
		case MDB_UINT16_FIELD:
		case MDB_UINT32_FIELD:
		case MDB_UINT64_FIELD:
			return true;
		}
		break;
	}
	return false;
}

uint32 CMdbSqlPara::SetField(uint32 nFieldNo, uint32 nOperator)
{
	if(nFieldNo < m_pTabDef->nFieldCount)
	{
		if(!CheckOperator(m_pTabDef->pFieldDefines[nFieldNo].nType, nOperator))
			return MDB_INVALID_INPUT;
		register uint32 nFlg = MDB_FLG_L(nFieldNo);
		register uint32 nBit = MDB_FLG_B(nFieldNo);
		if(!(m_pFlag[nFlg] & nBit))
		{
			CParaItem &oItem = m_pParaTable[m_nParaCount++];
			oItem.nFieldNo = nFieldNo;
			oItem.nOperator = nOperator;
			m_pFlag[nFlg] |= nBit;
		}
		return MDB_SUCCESS;
	}
	m_bError = true;
	return MDB_FIELD_NOTEXIST;
}

void CMdbSqlPara::SetFrom(CMdbSqlPara* pPara)
{
	Clear();
	CMdbSqlFilter oFilter(m_pTabDef);
	uint32 nParaCount = pPara->GetParaCount();
	for(uint32 i=0; i<nParaCount; ++i)
	{
		uint32 nOp, nCol;
		pPara->GetPara(i, &nOp, &nCol);
		SetField(nCol, nOp);
		oFilter.SetField(nCol);
	}
	m_oRecord.CopyFrom(oFilter, &pPara->m_oRecord);
}

void CMdbSqlPara::SetAllField(uint32 nOperator)
{
	for(uint32 i=0; i<m_pTabDef->nFieldCount; ++i)
	{
		register uint32 nFlg = MDB_FLG_L(i);
		register uint32 nBit = MDB_FLG_B(i);
		if(!(m_pFlag[nFlg] & nBit))
		{
			CParaItem &oItem = m_pParaTable[m_nParaCount++];
			oItem.nFieldNo = i;
			oItem.nOperator = nOperator;
			m_pFlag[nFlg] |= nBit;
		}
	}
}

void CMdbSqlPara::Clear()
{
	if(m_nParaCount)
	{
		for(register uint32 i=0; i<m_nFlgCount; ++i)
			m_pFlag[i] = 0;
		m_nParaCount = 0;
		m_oRecord.Clear();
	}
	m_bError = false;
}

bool CMdbSqlPara::HaveError()
{
	return m_bError;
}

bool CMdbSqlPara::IsSetField(uint32 nFieldNo)
{
	if(nFieldNo >= m_pTabDef->nFieldCount)
		return false;
	if(m_pFlag[MDB_FLG_L(nFieldNo)] & MDB_FLG_B(nFieldNo))
		return true;
	return false;
}

uint32 CMdbSqlPara::GetParaCount()
{
	return m_nParaCount;
}

uint32 CMdbSqlPara::GetPara(uint32 nParaIdx, uint32 *pOperator, uint32 * pFieldNo)
{
	if(nParaIdx >= m_nParaCount)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbSqlPara::GetPara(%u) failure: m_nParaCount=%u",
								 nParaIdx, m_nParaCount));
		return MDB_INVALID_INPUT;
	}
	CParaItem& oItem = m_pParaTable[nParaIdx];
	pFieldNo[0] = oItem.nFieldNo;
	pOperator[0] = oItem.nOperator;
	return MDB_SUCCESS;
}

void CMdbSqlPara::GetFilter(CMdbSqlFilter &oFilter)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)
		oFilter.SetField(m_pParaTable[i].nFieldNo);
}

bool CMdbSqlPara::IsCoincident(CMdbRecord& oRecord)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		CParaItem& oItem = m_pParaTable[i];
		register uint32 nFieldNo = oItem.nFieldNo;
		if(oRecord.m_bOwner)
			bRet = oRecord.GetField(nFieldNo)->IsCoincident(m_oRecord.GetField(nFieldNo), oItem.nOperator);
		else
		{
			CMdbField oField;
			oRecord.GetField(nFieldNo, oField);
			bRet = oField.IsCoincident(m_oRecord.GetField(nFieldNo), oItem.nOperator);
		}
		if(!bRet)
			break;
	}
	return bRet;
}

bool CMdbSqlPara::IsCoincident(uint32 nFieldNo, CMdbField* pField)
{
	register bool bRet = true;
	if(m_pFlag[MDB_FLG_L(nFieldNo)] & MDB_FLG_B(nFieldNo))
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

uint32 CMdbSqlPara::GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount)
{
	if(m_pFlag[MDB_FLG_L(nFieldNo)] & MDB_FLG_B(nFieldNo))
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			CParaItem& oItem = m_pParaTable[i];
			if(nFieldNo == oItem.nFieldNo)
			{
				if(oItem.nOperator != MDB_SQLPARA_OPERATOR_EQUAL)
					return 1;
				nHashValue =m_oRecord.GetField(nFieldNo)->GetHashValue();
				++nHashCount;
			}
		}
	}
	return 0;
}

CMdbField* CMdbSqlPara::GetCondField(uint32 nFieldNo, uint32& nOperator)
{
	if(m_pFlag[MDB_FLG_L(nFieldNo)] & MDB_FLG_B(nFieldNo))
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

bool CMdbSqlPara::GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen)
{
	register uint32 nFlg = MDB_FLG_L(nFieldNo), nBit = MDB_FLG_B(nFieldNo);
	if(m_pFlag[nFlg] & nBit)
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			CParaItem& oItem = m_pParaTable[i];
			if(oItem.nFieldNo == nFieldNo)
			{
				if(sStr)
					return false;
				if(oItem.nOperator != MDB_SQLPARA_OPERATOR_EQUAL)
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

bool CMdbSqlPara::WriteForRep(CMemoryStream & oStream, uint32 &nParaCount)
{
	nParaCount = 0;
	uint32 nPos1 = oStream.GetPosition();
	if(!oStream.Write(nParaCount))
		return false;
	CMdbSqlFilter oFilter(m_pTabDef);
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		CParaItem & oItem = m_pParaTable[i];
		register uint32 nCol = oItem.nFieldNo;
		void* pReplicative = m_pTabDef->pFieldDefines[nCol].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_REPLICATOR];
		if(pReplicative)
		{
			++nParaCount;
			register uint32 nOp = oItem.nOperator;
			if(!oStream.Write(nCol))
				return false;
			if(!oStream.Write(nOp))
				return false;
			oFilter.SetField(nCol);
		}
	}
	bool bRet = m_oRecord.Write(oStream, oFilter);
	if(bRet)
	{
		uint32 nPos2 = oStream.GetPosition();
		oStream.SetPosition(nPos1);
		oStream.Write(nParaCount);
		oStream.SetPosition(nPos2);
	}
	return bRet;
}

bool CMdbSqlPara::Write(CMemoryStream & oStream)
{
	CMdbSqlFilter oFilter(m_pTabDef);
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
		oFilter.SetField(nCol);
	}
	return m_oRecord.Write(oStream, oFilter);
}

bool CMdbSqlPara::Read(CMemoryStream & oStream)
{
	register uint32 nCount;
	if(!oStream.Read(nCount))
		return false;
	Clear();
	for(register uint32 i=0; i<nCount; ++i)
	{
		//为支持平滑升级，故不对nCount做检查
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

bool CMdbSqlPara::ReadFilterAndRecord(CMemoryStream & oStream)
{
	CMdbSqlFilter oFilter(m_pTabDef);
	if(!oFilter.Read(oStream))
		return false;
	register uint32 i, nCount = oFilter.GetParaCount();
	for(i=0; i<nCount; ++i)
	{
		register uint32 nCol = oFilter.GetPara(i);
		SetField(nCol, MDB_SQLPARA_OPERATOR_EQUAL);
	}
	return m_oRecord.Read(oStream);
}

uint32 CMdbSqlPara::GetType(register uint32 nFieldNo)
{
	return m_oRecord.GetType(nFieldNo);
}

void CMdbSqlPara::SetFrom(register uint32 nFieldNo, register CMdbField* pField, register uint32 nOperator)
{
	register CMdbField* pDstField = m_oRecord.GetField(nFieldNo);
	if(pDstField && pDstField->GetType() == pField->GetType())
	{
		SetField(nFieldNo, nOperator);
		m_oRecord.GetField(nFieldNo)->SetFromField(pField);
	}
}

void CMdbSqlPara::SetFrom(CMdbIndexDef* pIdxDef, CMdbRecord* pRecord)
{
	uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
	for(uint32 i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxDef->pExtendAttr->pFields[i];
		SetFrom(nFieldNo, pRecord->GetField(nFieldNo), MDB_SQLPARA_OPERATOR_EQUAL);
	}
}

bool CMdbSqlPara::IncludeIndexField(CMdbIndexDef* pIdxDef)
{
	uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
	for(uint32 i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxDef->pExtendAttr->pFields[i];
		if(IsSetField(nFieldNo))
			return true;
	}
	return false;
}

void CMdbSqlPara::SetNull(register uint32 nFieldNo, uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetNull(nFieldNo);
}

void CMdbSqlPara::SetInt8(register uint32 nFieldNo, register int8 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt8(nFieldNo, v);
}

void CMdbSqlPara::SetInt16(register uint32 nFieldNo, register int16 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt16(nFieldNo, v);
}

void CMdbSqlPara::SetInt32(register uint32 nFieldNo, register int32 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt32(nFieldNo, v);
}

void CMdbSqlPara::SetInt64(register uint32 nFieldNo, register int64 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetInt64(nFieldNo, v);
}

void CMdbSqlPara::SetUInt8(register uint32 nFieldNo, register uint8 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt8(nFieldNo, v);
}

void CMdbSqlPara::SetUInt16(register uint32 nFieldNo, register uint16 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt16(nFieldNo, v);
}

void CMdbSqlPara::SetUInt32(register uint32 nFieldNo, register uint32 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt32(nFieldNo, v);
}

void CMdbSqlPara::SetUInt64(register uint32 nFieldNo, register uint64 v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetUInt64(nFieldNo, v);
}

void CMdbSqlPara::SetFloat(register uint32 nFieldNo, register float v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetFloat(nFieldNo, v);
}

void CMdbSqlPara::SetDouble(register uint32 nFieldNo, register double v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetDouble(nFieldNo, v);
}

void CMdbSqlPara::SetString(register uint32 nFieldNo, register const char * v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetString(nFieldNo, v);
}

void CMdbSqlPara::SetRaw(register uint32 nFieldNo, register const void * v, register uint32 nLen, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetRaw(nFieldNo, v, nLen);
}

void CMdbSqlPara::SetDate(register uint32 nFieldNo, const CDate& oDate, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetDate(nFieldNo, oDate);
}

void CMdbSqlPara::SetTime(register uint32 nFieldNo, const CTime& oTime, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetTime(nFieldNo, oTime);
}

void CMdbSqlPara::SetDateTime(register uint32 nFieldNo, const CDateTime& oDateTime, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetDateTime(nFieldNo, oDateTime);
}

void CMdbSqlPara::SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator)
{
	SetField(nFieldNo, nOperator);
	m_oRecord.SetFromString(nFieldNo, v);
}

//-----------------------------------------------------------------------------
// CMdbSqlParaSet
//----------------------------------------------------------------------------
CMdbSqlParaSet::CMdbSqlParaSet(CMdbTableDef* pTabDef, bool bAnd)
{
	m_pTabDef = pTabDef;
	m_nParaCount = 0;
	m_pPataSet = NULL;
	m_bAnd = bAnd;
}

CMdbSqlParaSet::~CMdbSqlParaSet()
{
	Clear();
	m_pTabDef = NULL;
}

CMdbPara* CMdbSqlParaSet::AddPara()
{
	if(!m_bAnd)
		return NULL;
	m_pPataSet = (CParaItem*)CMalloc::Realloc(m_pPataSet, (m_nParaCount+1)*sizeof(CParaItem));
	m_pPataSet[m_nParaCount].bAtom = true;
	m_pPataSet[m_nParaCount].pPara = new CMdbSqlPara(m_pTabDef);
	CMdbSqlPara* pSqlPara = (CMdbSqlPara*)m_pPataSet[m_nParaCount].pPara;
	++m_nParaCount;
	return pSqlPara;
}

CMdbParaSet* CMdbSqlParaSet::AddParaSet()
{
	if(m_bAnd)
		return NULL;
	m_pPataSet = (CParaItem*)CMalloc::Realloc(m_pPataSet, (m_nParaCount+1)*sizeof(CParaItem));
	m_pPataSet[m_nParaCount].bAtom = false;
	m_pPataSet[m_nParaCount].pPara = new CMdbSqlParaSet(m_pTabDef, true);
	CMdbSqlParaSet* pSqlPara = (CMdbSqlParaSet*)m_pPataSet[m_nParaCount].pPara;
	++m_nParaCount;
	return pSqlPara;
}

uint32 CMdbSqlParaSet::GetParaCount()
{
	return m_nParaCount;
}

void* CMdbSqlParaSet::GetPara(uint32 nParaIdx, bool &bParaSet)
{
	if(nParaIdx >= m_nParaCount)
		return NULL;
	bParaSet = !m_pPataSet[nParaIdx].bAtom;
	return m_pPataSet[nParaIdx].pPara;
}

void CMdbSqlParaSet::Clear()
{
	if(m_pTabDef)
	{
		for(register uint32 i=0; i<m_nParaCount; ++i)
		{
			if(m_pPataSet[i].bAtom)
				delete (CMdbSqlPara*)m_pPataSet[i].pPara;
			else
				delete (CMdbSqlParaSet*)m_pPataSet[i].pPara;
		}
		if(m_pPataSet)
			CMalloc::Free(m_pPataSet);
	}
	m_nParaCount = 0;
	m_pPataSet = NULL;
}

bool CMdbSqlParaSet::IsEmpty()
{
	if(!m_nParaCount)
		return true;
	register uint32 i;
	for(i=0; i<m_nParaCount; ++i)
	{
		if(m_pPataSet[i].bAtom)
		{
			CMdbSqlPara* pPara = (CMdbSqlPara*)m_pPataSet[i].pPara;
			if(pPara->m_nParaCount)
				break;
		}
		else
		{
			CMdbSqlParaSet* pPara = (CMdbSqlParaSet*)m_pPataSet[i].pPara;
			if(!pPara->IsEmpty())
				break;
		}
	}
	return (i==m_nParaCount);
}

bool CMdbSqlParaSet::HaveError()
{
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			if(pSqlPara->HaveError())
				return true;
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			if(pSqlParaSet->HaveError())
				return true;
		}
	}
	return false;
}

void CMdbSqlParaSet::GetFilter(CMdbSqlFilter &oFilter)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			pSqlPara->GetFilter(oFilter);
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			pSqlParaSet->GetFilter(oFilter);
		}
	}
}

bool CMdbSqlParaSet::IsCoincident(CMdbRecord& oRecord)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			bRet = pSqlPara->IsCoincident(oRecord);
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			bRet = pSqlParaSet->IsCoincident(oRecord);
		}
		if(m_bAnd)
		{
			if(!bRet)
				break;
		}
		else if(bRet)
			break;
	}
	return bRet;
}

bool CMdbSqlParaSet::IsCoincident(uint32 nFieldNo, CMdbField* pField)
{
	register bool bRet = true;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			bRet = pSqlPara->IsCoincident(nFieldNo, pField);
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			bRet = pSqlParaSet->IsCoincident(nFieldNo, pField);
		}
		if(m_bAnd)
		{
			if(!bRet)
				break;
		}
		else if(bRet)
			break;
	}
	return bRet;
}

uint32 CMdbSqlParaSet::GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount)
{
	if(!m_bAnd)
		return 1;
	register uint32 nRet = 0;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			nRet = pSqlPara->GetHashValue(nFieldNo, nHashValue, nHashCount);
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			nRet = pSqlParaSet->GetHashValue(nFieldNo, nHashValue, nHashCount);
		}
		if(nRet)
			break;
	}
	return nRet;
}

bool CMdbSqlParaSet::GetCondField(uint32 nFieldNo, CMdbCondField& oMinField, CMdbCondField& oMaxField)
{
	if(!m_bAnd)
	{
		if(m_nParaCount)
			return false;
		oMinField.nOperator = MDB_SQLPARA_OPERATOR_MOREEQUAL;
		oMaxField.nOperator = MDB_SQLPARA_OPERATOR_LESSEQUAL;
	}
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			CMdbCondField oCondField;
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			oCondField.pField = pSqlPara->GetCondField(nFieldNo, oCondField.nOperator);
			if(oCondField.pField && !GetCondField(oCondField, oMinField, oMaxField))
				return false;
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			if(!pSqlParaSet->GetCondField(nFieldNo, oMinField, oMaxField))
				return false;
		}
	}
	return true;
}

bool CMdbSqlParaSet::GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen)
{
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			if(!pSqlPara->GetPrefixCondField(nFieldNo, sStr, pStrLen))
				return false;
		}
		else
		{
			register CMdbSqlParaSet* pSqlParaSet = (CMdbSqlParaSet*)pPara->pPara;
			if(!pSqlParaSet->GetPrefixCondField(nFieldNo, sStr, pStrLen))
				return false;
		}
	}
	return true;
}

bool CMdbSqlParaSet::GetCondField(CMdbCondField &oCondField, CMdbCondField& oMinField, CMdbCondField& oMaxField)
{
	register uint32 nOperator;
	if(oCondField.nOperator < MDB_SQLPARA_OPERATOR_LESS ||
			oCondField.nOperator > MDB_SQLPARA_OPERATOR_MOREEQUAL)
		return false;
	if(oCondField.nOperator == MDB_SQLPARA_OPERATOR_EQUAL)
	{
		oCondField.nOperator = MDB_SQLPARA_OPERATOR_MOREEQUAL;
		if(!GetCondField(oCondField, oMinField, oMaxField))
			return false;
		oCondField.nOperator = MDB_SQLPARA_OPERATOR_LESSEQUAL;
		if(!GetCondField(oCondField, oMinField, oMaxField))
			return false;
	}
	else if(oCondField.nOperator & MDB_SQLPARA_OPERATOR_LESS)
	{
		if(oCondField.nOperator & MDB_SQLPARA_OPERATOR_MORE)
			return false;
		if(oMinField.pField)
		{
			nOperator = MDB_SQLPARA_OPERATOR_MOREEQUAL;
			if(oCondField.nOperator & MDB_SQLPARA_OPERATOR_EQUAL)
			{
				if(oMinField.nOperator == nOperator)
					nOperator = MDB_SQLPARA_OPERATOR_MORE;
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
			nOperator = MDB_SQLPARA_OPERATOR_LESSEQUAL;
			if(oCondField.nOperator & MDB_SQLPARA_OPERATOR_EQUAL)
			{
				if(oMaxField.nOperator == nOperator)
					nOperator = MDB_SQLPARA_OPERATOR_LESS;
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

bool CMdbSqlParaSet::Write(CMemoryStream & oStream)
{
	if(!oStream.Write((uint8)m_bAnd))
		return false;
	if(!oStream.Write(m_nParaCount))
		return false;
	for(register uint32 i=0; i<m_nParaCount; ++i)
	{
		register CParaItem* pPara = m_pPataSet + i;
		if(!oStream.Write((uint8)pPara->bAtom))
			return false;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)pPara->pPara;
			if(!pSqlPara->Write(oStream))
				return false;
		}
		else
		{
			register CMdbSqlParaSet* pSqlPara = (CMdbSqlParaSet*)pPara->pPara;
			if(!pSqlPara->Write(oStream))
				return false;
		}
	}
	return true;
}

void CMdbSqlParaSet::SetFrom(CMdbSqlParaSet* pParaSet)
{
	Clear();
	m_bAnd = pParaSet->m_bAnd;
	for(register uint32 i=0; i<pParaSet->m_nParaCount; ++i)
	{
		register CParaItem* pPara = pParaSet->m_pPataSet + i;
		if(pPara->bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)AddPara();
			pSqlPara->SetFrom((CMdbSqlPara*)pPara->pPara);
		}
		else
		{
			register CMdbSqlParaSet* pSqlPara = (CMdbSqlParaSet*)AddParaSet();
			pSqlPara->SetFrom((CMdbSqlParaSet*)pPara->pPara);
		}
	}
}

bool CMdbSqlParaSet::Read(CMemoryStream & oStream)
{
	register uint32 nCount;

	Clear();
	uint8 nAnd;
	if(!oStream.Read(nAnd))
		return false;
	m_bAnd = nAnd?true:false;
	if(!oStream.Read(nCount))
		return false;
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint8 bAtom;
		if(!oStream.Read(bAtom))
			return false;
		if(bAtom)
		{
			register CMdbSqlPara* pSqlPara = (CMdbSqlPara*)AddPara();
			if(!pSqlPara)
				return false;
			if(!pSqlPara->Read(oStream))
				return false;
		}
		else
		{
			register CMdbSqlParaSet * pSqlPara = (CMdbSqlParaSet*)AddParaSet();
			if(!pSqlPara)
				return false;
			if(!pSqlPara->Read(oStream))
				return false;
		}
	}
	return true;
}

bool CMdbSqlParaSet::Insert2Update(CMdbSqlPara* pInsert, CMdbSqlPara* pSet)
{
	uint32 i;
	CMdbIndexDef* pUniqueIdx = NULL;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	for(i=0; i<pTabAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIdx = pTabAttr->pIndexDefineSet[i];
		if(pIdx->nQualifier == MDB_UNIQUE_INDEX)
		{
			pUniqueIdx = pIdx;
			break;
		}
	}
	if(!pUniqueIdx)
		return false;
	Clear();
	CMdbSqlPara* pIdx = (CMdbSqlPara*)AddParaSet()->AddPara();
	uint32 nIdxParaCount = 0;
	CMdbIndexAttr* pIdxAttr = pUniqueIdx->pExtendAttr;
	uint32 nParaCount = pInsert->GetParaCount();
	CMdbSqlFilter oIdxFilter(m_pTabDef), oSetFilter(m_pTabDef);
	for(i=0; i<nParaCount; ++i)
	{
		uint32 nOp, nCol;
		pInsert->GetPara(i, &nOp, &nCol);
		if(nIdxParaCount < pIdxAttr->nFieldCount)
		{
			uint32 j;
			for(j=0; j<pIdxAttr->nFieldCount; ++j)
			{
				if(pIdxAttr->pFields[j] == nCol)
				{
					++nIdxParaCount;
					break;
				}
			}
			if(j < pIdxAttr->nFieldCount)
			{
				pIdx->SetField(nCol, MDB_SQLPARA_OPERATOR_EQUAL);
				oIdxFilter.SetField(nCol);
				continue;
			}
		}
		pSet->SetField(nCol, MDB_SQLPARA_OPERATOR_EQUAL);
		oSetFilter.SetField(nCol);
	}
	if(nIdxParaCount < pIdxAttr->nFieldCount)
		return false;
	pIdx->m_oRecord.CopyFrom(oIdxFilter, &pInsert->m_oRecord);
	pSet->m_oRecord.CopyFrom(oSetFilter, &pInsert->m_oRecord);
	return true;
}

bool CMdbSqlParaSet::SetFromWhere(const char* sCond)
{
	Clear();
	bool bFirst = true;
	const char* sWhere = sCond;
	if(sCond)while(sCond[0])
		{
			uint32 nRet, nOp;
			CString oName, oValue;
			if(!sCond[0])
				break;
			if(GetIdentifier(sCond, oName))
			{
				FocpError(("CMdbParaSet::SetFromWhere(%u:%s): get fieldname failure", sCond-sWhere, sWhere));
				return false;
			}
			if(!bFirst)
			{
				if(!oName.Compare("and", false))
					continue;
				if(!oName.Compare("or", false))
				{
					AddParaSet();
					continue;
				}
			}
			bFirst = false;
			uint32 nCol = GetFieldNo(oName.GetStr());
			if(nCol == (uint32)(-1))
			{
				FocpError(("CMdbParaSet::SetFromWhere(%u:%s): the field '%s' isn't existed", sCond-sWhere, sWhere, oName.GetStr()));
				return false;
			}
			nRet = GetOperator(sCond, nOp);
			if(nRet)
			{
				FocpError(("CMdbParaSet::SetFromWhere(%u:%s): get oprand for WHERE failure", sCond-sWhere, sWhere));
				return false;
			}
			nRet = GetValue(sCond, oValue);
			if(nRet)
			{
				FocpError(("CMdbParaSet::SetFromWhere(%u:%s): get value for WHERE failure", sCond-sWhere, sWhere));
				return false;
			}
			AddCond(nCol, nOp, oValue);
		}
	return true;
}

void CMdbSqlParaSet::SkipWhiteSpace(const char* &pStr)
{
	while(pStr[0] == ' ' || pStr[0] == '\t')
		++pStr;
}

uint32 CMdbSqlParaSet::GetIdentifier(const char* &pStr, CString &oIdentifier)
{
	SkipWhiteSpace(pStr);
	if( (pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z') || pStr[0] == '_')
	{
		oIdentifier += pStr[0];
		++pStr;
		while( (pStr[0] >= 'a' && pStr[0] <= 'z') ||
				(pStr[0] >= 'A' && pStr[0] <= 'Z') ||
				pStr[0] == '_' ||
				(pStr[0] >= '0' && pStr[0] <= '9') )
		{
			oIdentifier += pStr[0];
			++pStr;
		}
		return 0;
	}
	return 1;
}

uint32 CMdbSqlParaSet::GetOperator(const char* &pStr, uint32 &nOp)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] == '>')
	{
		nOp = MDB_SQLPARA_OPERATOR_MORE;
		++pStr;
		if(pStr[0] == '=')
		{
			nOp = MDB_SQLPARA_OPERATOR_MOREEQUAL;
			++pStr;
		}
	}
	else if(pStr[0] == '<')
	{
		nOp = MDB_SQLPARA_OPERATOR_LESS;
		++pStr;
		if(pStr[0] == '=')
		{
			nOp = MDB_SQLPARA_OPERATOR_LESSEQUAL;
			++pStr;
		}
	}
	else if(pStr[0] == '=')
	{
		nOp = MDB_SQLPARA_OPERATOR_EQUAL;
		++pStr;
	}
	else
		return 1;
	return 0;
}

uint32 CMdbSqlParaSet::GetValue(const char* &pStr, CString &oValue)
{
	SkipWhiteSpace(pStr);
	if(!CString::StringCompare(pStr, "NULL", false, 4))
	{
		oValue = "";
		pStr += 4;
		return 0;
	}
	if(pStr[0] =='\'')
	{
		++pStr;

		while(pStr[0])
		{
			if(pStr[0] == '\'' && pStr[1] == '\'')
			{
				pStr += 2;
				oValue += '\'';
			}
			else if(pStr[0] == '\'' || pStr[0] == '\n' || pStr[0] == '\r')
				break;
			oValue += pStr[0];
			++pStr;
		}
		if(pStr[0] == '\'')
		{
			++pStr;
			return 0;
		}
		return 1;
	}
	if(pStr[0] =='+' || pStr[0] == '-')
	{
		++pStr;
		oValue = pStr[0];
	}
	while(pStr[0] >= '0' && pStr[0] <= '9')
	{
		oValue += pStr[0];
		++pStr;
	}
	if(pStr[0] == '.')
	{
		oValue += pStr[0];
		++pStr;
		while(pStr[0] >= '0' && pStr[0] <= '9')
		{
			oValue += pStr[0];
			++pStr;
		}
	}
	if(oValue.GetSize())
		return 0;
	return 1;
}

void CMdbSqlParaSet::AddCond(uint32 nFieldNo, uint32 nOp, const CString& oVal)
{
	bool bSet;
	CMdbSqlParaSet* pParaSet = this;
	uint32 i, nCount = pParaSet->GetParaCount();
	if(nCount)
		pParaSet = (CMdbSqlParaSet*)pParaSet->GetPara(nCount - 1, bSet);
	else
		pParaSet = (CMdbSqlParaSet*)pParaSet->AddParaSet();
	nCount = pParaSet->GetParaCount();
	for(i=0; i<nCount; ++i)
	{
		CMdbSqlPara* pPara = (CMdbSqlPara*)pParaSet->GetPara(i, bSet);
		if(!pPara->IsSetField(nFieldNo))
		{
			pPara->SetFromString(nFieldNo, oVal.GetStr(), nOp);
			return;
		}
	}
	CMdbSqlPara* pPara = (CMdbSqlPara*)pParaSet->AddPara();
	pPara->SetFromString(nFieldNo, oVal.GetStr(), nOp);
}

uint32 CMdbSqlParaSet::GetFieldNo(const char* sFieldName)
{
	register uint32 nFieldCount = m_pTabDef->nFieldCount;
	for(register uint32 i=0; i<nFieldCount; ++i)
	{
		register CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + i;
		if(!CString::StringCompare(pFieldDef->sFieldName, sFieldName, false))
			return i;
	}
	FocpLog(FOCP_LOG_WARNING, ("CMdbTableAccess::GetFieldNo(%s.%s) failure", m_pTabDef->sTableName, sFieldName));
	return (uint32)(-1);
}

static CMutex g_oAccessMutex;
static CRbMap<CString, CMdbTableAccess*, CNameCompare> g_oAccessTable;

void RemoveAccessTable()
{
	CRbTreeNode* pEnd = g_oAccessTable.End();
	g_oAccessMutex.Enter();
	CRbTreeNode* pIt = g_oAccessTable.First();
	for(; pIt!=pEnd; pIt=g_oAccessTable.GetNext(pIt))
	{
		CMdbTableAccess* pAccess = g_oAccessTable.GetItem(pIt);
		while(pAccess)
		{
			CMdbTableAccess* pNext = pAccess->m_pNext;
			delete pAccess;
			pAccess = pNext;
		}
	}
	g_oAccessTable.Clear();
	g_oAccessMutex.Leave();
}

CMdbTableAccess::CMdbTableAccess(CMdb* pDb, CMdbTableDef* pTabDef):
	m_pTabDef(pTabDef),
	m_oIdxAttr(pTabDef),
	m_oInsertAttr(pTabDef),
	m_oSetAttr(pTabDef),
	m_oGetAttr(pTabDef),
	m_oFilter(pTabDef),
	m_pDb(pDb)
{
	m_pOrderByIndex = NULL;
	m_bAscOrder = true;
	m_pNext = NULL;
}

CMdbTableAccess::~CMdbTableAccess()
{
}

uint32 CMdbTableAccess::GetFieldNo(const char* sFieldName)
{
	register uint32 nFieldCount = m_pTabDef->nFieldCount;
	for(register uint32 i=0; i<nFieldCount; ++i)
	{
		register CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + i;
		if(!CString::StringCompare(pFieldDef->sFieldName, sFieldName, false))
			return i;
	}
	FocpLog(FOCP_LOG_WARNING, ("CMdbTableAccess::GetFieldNo(%s.%s.%s) failure", GetMdb()->GetDbName(), m_pTabDef->sTableName, sFieldName));
	return (uint32)(-1);
}

uint32 CMdbTableAccess::GetFieldCount()
{
	return m_pTabDef->nFieldCount;
}

CMdbFilter* CMdbTableAccess::GetResultFilter()
{
	m_oFilter.Clear();
	return &m_oFilter;
}

CMdbResultSet* CMdbTableAccess::GetResultSet()
{
	return &m_oGetAttr;
}

CMdbParaSet* CMdbTableAccess::GetQueryPara()
{
	m_oIdxAttr.Clear();
	m_oFilter.Clear();
	m_pOrderByIndex = NULL;
	return &m_oIdxAttr;
}

CMdbPara* CMdbTableAccess::GetUpdatePara()
{
	m_oSetAttr.Clear();
	return &m_oSetAttr;
}

CMdbPara* CMdbTableAccess::GetInsertPara()
{
	m_oInsertAttr.Clear();
	return &m_oInsertAttr;
}

bool CMdbTableAccess::SetOrderBy(const char* sIndexName, bool bAsc)
{
	m_bAscOrder = bAsc;
	m_pOrderByIndex = NULL;
	CMdbTableAttr* pTableAttr = m_pTabDef->pExtendAttr;
	if(sIndexName)for(uint32 i=0; i<pTableAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIndexDef = pTableAttr->pIndexDefineSet[i];
		if(!CString::StringCompare(pIndexDef->sIndexName, sIndexName, false))
		{
			if(pIndexDef->nArithmetic & MDB_RANGE_INDEX)
			{
				m_pOrderByIndex = pIndexDef;
				return true;
			}
		}
	}
	return false;
}

void CMdbTableAccess::Clear()
{
	m_oFilter.Clear();
	m_oGetAttr.Clear();
	m_oIdxAttr.Clear();
	m_oFilter.Clear();
	m_pOrderByIndex = NULL;
	m_oSetAttr.Clear();
	m_oInsertAttr.Clear();
}

const char* CMdbTableAccess::GetTableName()
{
	return m_pTabDef->sTableName;
}

CMdb* CMdbTableAccess::GetMdb()
{
	return m_pDb;
}

CMdbAccess* CMdb::QueryAccess(const char* sTableName)
{
	CMdbTableAccess* pAccess = NULL;
	CRbTreeNode* pEnd = g_oAccessTable.End();
	g_oAccessMutex.Enter();
	CRbTreeNode* pIt = g_oAccessTable.Find(sTableName);
	if(pIt != pEnd)
	{
		CMdbTableAccess* &pAccess2 = g_oAccessTable.GetItem(pIt);
		if(pAccess2)
		{
			pAccess = pAccess2;
			CMdbTableAccess* pPrev = NULL;
			while(pAccess)
			{
				if(pAccess->GetMdb() == this)
					break;
				pPrev = pAccess;
				pAccess = pAccess->m_pNext;
			}
			if(pAccess)
			{
				if(pPrev)
					pPrev->m_pNext = pAccess->m_pNext;
				else
					pAccess2 = pAccess->m_pNext;
			}
		}
	}
	g_oAccessMutex.Leave();
	if(pAccess == NULL)
		pAccess = (CMdbTableAccess*)CreateAccess(sTableName);
	pAccess->m_pNext = NULL;
	return pAccess;
}

void CMdbTableAccess::Release()
{
	m_oIdxAttr.Clear();
	m_oInsertAttr.Clear();
	m_oSetAttr.Clear();
	m_oGetAttr.Clear();
	m_oFilter.Clear();

	CRbTreeNode* pEnd = g_oAccessTable.End();
	g_oAccessMutex.Enter();
	CRbTreeNode* pIt = g_oAccessTable.Find(m_pTabDef->sTableName);
	if(pIt != pEnd)
	{
		CMdbTableAccess* &pAccess2 = g_oAccessTable.GetItem(pIt);
		m_pNext = pAccess2;
		pAccess2 = this;
	}
	else
	{
		m_pNext = NULL;
		g_oAccessTable[m_pTabDef->sTableName] = this;
	}
	g_oAccessMutex.Leave();
}

CMdbTableDef* CMdbTableAccess::GetTableDefine()
{
	return m_pTabDef;
}

static CMdbExtPlugIn* g_pPlugInTable[MDB_ATTR_EXTSIZE] = {NULL};

CMdbExtPlugIn::CMdbExtPlugIn(uint32 nPlugInType)
{
	if(nPlugInType >= MDB_ATTR_EXTSIZE)
		FocpAbort(("CMdbExtPlugIn::CMdbExtPlugIn(%u), the argument is invalid)", nPlugInType));
	if(g_pPlugInTable[nPlugInType])
		FocpAbort(("CMdbExtPlugIn::CMdbExtPlugIn(%u), the plugin is registered)", nPlugInType));

	g_pPlugInTable[nPlugInType] = this;
	m_nPlugIn = nPlugInType;
}

CMdbExtPlugIn::~CMdbExtPlugIn()
{
	g_pPlugInTable[m_nPlugIn] = NULL;
}

CMdbExtPlugIn* CMdbExtPlugIn::GetPlugIn(uint32 nPlugInType)
{
	if(nPlugInType >= MDB_ATTR_EXTSIZE)
		return NULL;
	return g_pPlugInTable[nPlugInType];
}

static CMdbFirstNode* g_pFirstNode = NULL;

CMdbFirstNode::CMdbFirstNode()
{
	if(g_pFirstNode)
		FocpAbort(("g_pFirstNode isn't NULL)"));
	g_pFirstNode = this;
}

CMdbFirstNode::~CMdbFirstNode()
{
	g_pFirstNode = NULL;
}

CMdbFirstNode* CMdbFirstNode::GetInstance()
{
	if(g_pFirstNode)
		return g_pFirstNode;
	return CSingleInstance<CMdbFirstNode>::GetInstance();
}

bool CMdbFirstNode::OnFirstNode(uint32 nDomain)
{
	return true;
}

void CMdbFirstNode::OnOtherNode(uint32 nDomain)
{
}

FOCP_END();
