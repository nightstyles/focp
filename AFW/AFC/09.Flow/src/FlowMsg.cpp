
#include "FlowMsg.hpp"

#include <stdlib.h>
#include <errno.h>

FOCP_BEGIN();

static uint8 g_nLocalCode = IsSmallEndian()?1:0;

#define FLW_FLG_L(x) ((x)>>5)
#define FLW_FLG_B(x) (1<<((x)&31))

#define GetAtom(type) (*(type*)m_pData)
#define FillField() ((*m_pFlag) |= m_nBit)
#define IS_NULL() (((*m_pFlag) & m_nBit)?false:true)

CMsgFieldType::CMsgFieldType(CMsgSystem* pMsgSystem)
{
	if(pMsgSystem)
		Register(pMsgSystem);
}

CMsgFieldType::~CMsgFieldType()
{
}

uint32 CMsgFieldType::GetType() const
{
	return FLW_NULL;
}

const char* CMsgFieldType::GetTypeName(uint32 nType)
{
	const char* sRet;
	switch(nType)
	{
	default:
		sRet = "unknown";
		break;
	case FLW_INT8:
		sRet = "int8";
		break;
	case FLW_UINT8:
		sRet = "uint8";
		break;
	case FLW_INT16:
		sRet = "int16";
		break;
	case FLW_UINT16:
		sRet = "uint16";
		break;
	case FLW_INT32:
		sRet = "int32";
		break;
	case FLW_UINT32:
		sRet = "uint32";
		break;
	case FLW_INT64:
		sRet = "int64";
		break;
	case FLW_UINT64:
		sRet = "uint64";
		break;
	case FLW_FLOAT32:
		sRet = "float";
		break;
	case FLW_FLOAT64:
		sRet = "double";
		break;
	case FLW_DATE:
		sRet = "date";
		break;
	case FLW_TIME:
		sRet = "time";
		break;
	case FLW_DATETIME:
		sRet = "datetime";
		break;
	case FLW_STRING:
		sRet = "string";
		break;
	case FLW_VSTRING:
		sRet = "vstring";
		break;
	case FLW_STRUCT:
		sRet = "struct";
		break;
	}
	return sRet;
}

uint32 CMsgFieldType::GetSize() const
{
	return 0;
}

const char* CMsgFieldType::GetName() const
{
	return NULL;
}

void CMsgFieldType::Register(CMsgSystem* pMsgSystem)
{
	pMsgSystem->RegType(this);
}

template<typename TData, uint32 nType> class CBaseType: public CMsgFieldType
{
private:
	CString m_oName;

public:
	inline CBaseType(CMsgSystem* pMsgSystem, const char* sName):CMsgFieldType(NULL), m_oName(sName)
	{
		CMsgFieldType::Register(pMsgSystem);
	}

	inline virtual uint32 GetType() const
	{
		return nType;
	}

	inline virtual uint32 GetSize() const
	{
		return sizeof(TData);
	}

	inline virtual const char* GetName() const
	{
		return m_oName.GetStr();
	}
};

CMsgFieldDef::CMsgFieldDef(CMsgFieldType* pType, const char* sFieldName):
	m_oName(sFieldName), m_pType(pType)
{
	m_nOffset = 0;
	m_nMaxLen = 0;//仅对string&vstring字段有意义
	m_nUnits = 0;
	m_pStruct = NULL;
	m_bMandatory = false;
}

CMsgFieldDef::~CMsgFieldDef()
{
}

void CMsgFieldDef::Missing(const char* pAddInfo) const
{
	FocpLog(FOCP_LOG_WARNING, ("%s missing the mandatory field %s.%s", pAddInfo, m_pStruct->GetName(), GetName()));
}

const char* CMsgFieldDef::GetStructName()const
{
	return m_pStruct->GetName();
}

const CMsgFieldType* CMsgFieldDef::GetType() const
{
	return m_pType;
}

const char* CMsgFieldDef::GetName() const
{
	return m_oName.GetStr();
}

uint32 CMsgFieldDef::GetOffset() const
{
	return m_nOffset;
}

void CMsgFieldDef::SetDefault(const char* sDefault)
{
	m_bMandatory = false;
	m_nUnits = 0;
	m_oDefault = sDefault;
}

const char* CMsgFieldDef::GetDefault() const
{
	return m_oDefault.GetStr();
}

void CMsgFieldDef::SetMandatory(bool bMandatory)
{
	if(bMandatory)
	{
		m_oDefault.Clear();
		m_nUnits = 0;
	}
	m_bMandatory = bMandatory;
}

bool CMsgFieldDef::IsMandatory() const
{
	return m_bMandatory;
}

void CMsgFieldDef::SetVector(uint32 nSize)
{
	if(nSize)
	{
		m_bMandatory = false;
		m_oDefault.Clear();
	}
	m_nUnits = nSize;
}

uint32 CMsgFieldDef::GetVector() const
{
	return m_nUnits;
}

void CMsgFieldDef::SetMaxLen(uint32 nLen)
{
	uint32 nType = m_pType->GetType();
	if(nType == FLW_STRING || nType == FLW_VSTRING)
		m_nMaxLen = nLen;
}

uint32 CMsgFieldDef::GetMaxLen() const
{
	return m_nMaxLen;
}

void CMsgFieldDef::GetFieldAttr(const char* sStructName, CString& oAttr) const
{
	if(IsMandatory())
		oAttr += "mandatory ";
	oAttr += m_pType->GetName();
	uint32 nType = m_pType->GetType();
	if(nType == FLW_STRING || nType == FLW_VSTRING)
	{
		oAttr += "(";
		CStringFormatter oFmt(&oAttr, oAttr.GetSize());
		oFmt.Print("%u", m_nMaxLen);
		oAttr += ")";
	}
	oAttr += " ";
	oAttr += sStructName;
	oAttr += "::";
	oAttr += GetName();
	if(m_nUnits)
	{
		oAttr += "[";
		CStringFormatter oFmt(&oAttr, oAttr.GetSize());
		oFmt.Print("%u", m_nUnits);
		oAttr += "]";
	}
	else if(!m_oDefault.Empty())
	{
		oAttr += " = ";
		CString oDef(m_oDefault);
		switch(nType)
		{
		case FLW_STRING:
		case FLW_VSTRING:
		case FLW_DATE:
		case FLW_TIME:
		case FLW_DATETIME:
			oDef.ToCString(false);
		}
		oAttr += oDef;
	}
	oAttr += ";";
}

CMsgStruct::CMsgStruct(CMsgSystem* pMsgSystem, const char* sName, bool bUnion):
	CMsgFieldType(NULL), m_oName(sName), m_nSize(0), m_bUnion(bUnion)
{
	if(pMsgSystem)
		CMsgFieldType::Register(pMsgSystem);
	m_bImplemented = false;
}

CMsgStruct::~CMsgStruct()
{
}

uint32 CMsgStruct::GetType() const
{
	return FLW_STRUCT;
}

uint32 CMsgStruct::GetSize() const
{
	if(!m_bImplemented)
		return 0;
	return m_nSize;
}

const char* CMsgStruct::GetName() const
{
	return m_oName.GetStr();
}

uint32 CMsgStruct::GetFieldCount() const
{
	return m_oFields.GetSize();
}

const CMsgFieldDef* CMsgStruct::GetField(uint32 nIdx) const
{
	if(nIdx < m_oFields.GetSize())
		return m_oFields[nIdx];
	return NULL;
}

const CMsgFieldDef* CMsgStruct::FindField(const char* sName) const
{
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		const CMsgFieldDef* pField = m_oFields[i];
		if(!pField->m_oName.Compare(sName, false))
			return pField;
	}
	return NULL;
}

uint32 CMsgStruct::GetFieldNo(const char* sName) const
{
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		const CMsgFieldDef* pField = m_oFields[i];
		if(!pField->m_oName.Compare(sName, false))
			return i;
	}
	return (uint32)(-1);
}

bool CMsgStruct::AddField(CMsgFieldDef* pField)
{
	if(FindField(pField->GetName()))
		return false;
	m_oFields.Insert((uint32)(-1), CAutoPointer<CMsgFieldDef>(pField));
	m_bImplemented = false;
	pField->m_pStruct = this;
	return true;
}

bool CMsgStruct::Implemented() const
{
	return m_bImplemented;
}

namespace
{
struct CMsgUnionData
{
	double nData;
	uint32 nIdx;
};
};

bool CMsgStruct::IsUnion() const
{
	return m_bUnion;
}

void CMsgStruct::FinishDefine()
{
	m_nSize = 0;
	m_bImplemented = true;

	if(m_bUnion)
	{
		m_nSize = sizeof(CMsgUnionData);
		return;
	}

	uint32 nMod, nSize = m_oFields.GetSize();
	m_nSize = FLW_FLG_L(nSize-1) + 1;
	if(m_nSize&1)
		m_nSize+=1;
	m_nSize <<= 2;
	uint32 nAlign = 4;
	for(uint32 i=0; i<nSize; ++i)
	{
		CMsgFieldDef* pField = m_oFields[i];
		const CMsgFieldType* pType = pField->GetType();
		uint32 nFieldType = pType->GetType();
		uint32 nFieldSize = pType->GetSize();
		uint32 nVector = pField->GetVector();
		if(nVector)
			nFieldSize = sizeof(void*);
		else switch(nFieldType)
			{
			case FLW_STRING:
				nFieldSize = pField->GetMaxLen();
				break;
			case FLW_VSTRING:
			case FLW_STRUCT:
				nFieldSize = sizeof(void*);
				break;
			}
		nMod = m_nSize % nFieldSize;
		if(nMod)
			m_nSize += nFieldSize - nMod;
		pField->m_nOffset = m_nSize;
		m_nSize += nFieldSize;
		if(nFieldSize > nAlign)
			nAlign = nFieldSize;
	}
	nMod = m_nSize % nAlign;
	if(nMod)
		m_nSize += nAlign - nMod;
}

static CString GetFieldTypeName(const CMsgFieldType* pType)
{
	CString oType;
	const char* sType = pType->GetName();
	if(pType->GetType() == FLW_STRUCT)
		oType = "C";
	else
	{
		if(!CString::StringCompare(sType, "string", false) || !CString::StringCompare(sType, "vstring", false))
			sType = "CString";
		else if(!CString::StringCompare(sType, "date", false))
			sType = "CDate";
		else if(!CString::StringCompare(sType, "time", false))
			sType = "CTime";
		else if(!CString::StringCompare(sType, "datetime", false))
			sType = "CDateTime";
	}
	oType += sType;
	return oType;
}

void CMsgStruct::CreateCppStructCode(CFileFormatter& oFmt, bool bHpp) const
{
	const char* sStructName = m_oName.GetStr();
	uint32 i, nSize = m_oFields.GetSize();
	oFmt.Print("\n");
	oFmt.Print("/**********************************************\n");
	oFmt.Print(" * C%s for %s %s\n", sStructName, IsUnion()?"union":"struct", sStructName);
	oFmt.Print(" **********************************************/\n");
	if(bHpp)
	{
		oFmt.Print("class C%s: public CMsgStructInst\n", sStructName);
		oFmt.Print("{\n");
		oFmt.Print("private:\n");
		oFmt.Print("	C%s& operator=(const C%s &oSrc);\n", sStructName, sStructName);
		oFmt.Print("\n");
		oFmt.Print("public:\n");
		oFmt.Print("	C%s(CMsgRecord* pStruct);\n", sStructName);
		oFmt.Print("	C%s(const C%s &oSrc);\n", sStructName, sStructName);
		oFmt.Print("\n");
		oFmt.Print("	//Field list(can be direct accessed):\n");
		oFmt.Print("	//CMsgValue<TValueType, nCol, nMaxLen, bOption, nCapacity>\n");
		for(i=0; i<nSize; ++i)
		{
			CString oFieldAttr;
			const CMsgFieldDef* pField = GetField(i);
			pField->GetFieldAttr(sStructName, oFieldAttr);
			const char* sFieldName = pField->GetName();
			CString oType = GetFieldTypeName(pField->GetType());
			oFmt.Print("	CMsgValue<%s, %u, %u, %s, %u>::TValue %s;//%s\n",
					   oType.GetStr(), i, pField->GetMaxLen(), pField->IsMandatory()?"false":"true",
					   pField->GetVector(), sFieldName, oFieldAttr.GetStr());
		}
		oFmt.Print("};\n");
	}
	else
	{
		oFmt.Print("C%s::C%s(CMsgRecord* pStruct):\n", sStructName, sStructName);
		oFmt.Print("	CMsgStructInst(pStruct)\n");
		oFmt.Print("{\n");
		oFmt.Print("}\n");
		oFmt.Print("\n");
		oFmt.Print("C%s::C%s(const C%s &oSrc)\n", sStructName, sStructName, sStructName);
		oFmt.Print("	:CMsgStructInst(oSrc)\n");
		oFmt.Print("{\n");
		oFmt.Print("}\n");
	}
}

void CMsgStruct::CreateCppMsgCode(CFileFormatter& oFmt, uint32 nMsgId, bool bHpp) const
{
	const char* sStructName = m_oName.GetStr();
	oFmt.Print("/**********************************************\n");
	oFmt.Print(" * C%sMsg\n", sStructName);
	oFmt.Print(" **********************************************/\n");
	if(bHpp)
	{
		oFmt.Print("class C%sMsg: public CMessage\n", sStructName);
		oFmt.Print("{//msgid = %u;\n", nMsgId);
		oFmt.Print("public:\n");
		oFmt.Print("//create message(new):\n");
		oFmt.Print("	C%sMsg(CMsgSystem* pMsgSystem = NULL);\n", sStructName);
		oFmt.Print("\n");
		oFmt.Print("//parse message(decode):\n");
		oFmt.Print("//	you can direct use 'CMessage(CMemoryStream& oStream, CMsgSystem* pMsgSystem=NULL)' to parse any message,\n");
		oFmt.Print("//	then call Valid() & GetMsgId(), and by the msgid, you can forced convert it into the related message class.\n");
		oFmt.Print("//	such as:\n");
		oFmt.Print("//		CMessage x(oStream);\n");
		oFmt.Print("//		CAaMsg *pAaMsg = NULL;\n");
		oFmt.Print("//		if(x.Valid() && x.GetMsgId() == 101)\n");
		oFmt.Print("//			pAaMsg = (CAaMsg*)&x;\n");
		oFmt.Print("	C%sMsg(CMemoryStream& oStream, CMsgSystem* pMsgSystem = NULL);\n", sStructName);
		oFmt.Print("	C%sMsg(uint32 nMsgId, CMemoryStream& oStream, CMsgSystem* pMsgSystem = NULL);\n", sStructName);
		oFmt.Print("\n");
		oFmt.Print("//get message struct for any reading, writing or updating):\n");
		oFmt.Print("	C%s& GetStruct();\n", sStructName);
		oFmt.Print("\n");
		oFmt.Print("//other interface(defined in the base class 'CMessage'):\n");
		oFmt.Print("	//bool Check(); //check the message's availability because of some writing or updating\n");
		oFmt.Print("	//bool Pack(CMemoryStream& oStream); //encode the message into stream\n");
		oFmt.Print("	//void Dump(CFormatter& oFmt); //dump the message into the formatter as text\n");
		oFmt.Print("	//bool Valid() const; //to only check decode's availability\n");
		oFmt.Print("	//uint32 GetMsgId(); //get the message id\n");
		oFmt.Print("};\n");
	}
	else
	{
		oFmt.Print("C%sMsg::C%sMsg(CMsgSystem* pMsgSystem):\n", sStructName, sStructName);
		oFmt.Print("	CMessage(%u, pMsgSystem)\n", nMsgId);
		oFmt.Print("{\n");
		oFmt.Print("}\n");
		oFmt.Print("\n");
		oFmt.Print("C%sMsg::C%sMsg(CMemoryStream& oStream, CMsgSystem* pMsgSystem):CMessage(oStream, pMsgSystem)\n", sStructName, sStructName);
		oFmt.Print("{\n");
		oFmt.Print("	if(m_oInst.Valid() && m_nMsgId != %u)\n", nMsgId);
		oFmt.Print("	{\n");
		oFmt.Print("		m_nMsgId = %u;\n", nMsgId);
		oFmt.Print("		Clear();\n");
		oFmt.Print("	}\n");
		oFmt.Print("}\n");
		oFmt.Print("\n");
		oFmt.Print("C%sMsg::C%sMsg(uint32 nMsgId, CMemoryStream& oStream, CMsgSystem* pMsgSystem):CMessage(nMsgId, oStream, pMsgSystem)\n", sStructName, sStructName);
		oFmt.Print("{\n");
		oFmt.Print("	if(m_oInst.Valid() && m_nMsgId != %u)\n", nMsgId);
		oFmt.Print("	{\n");
		oFmt.Print("		m_nMsgId = %u;\n", nMsgId);
		oFmt.Print("		Clear();\n");
		oFmt.Print("	}\n");
		oFmt.Print("}\n");
		oFmt.Print("\n");
		oFmt.Print("C%s& C%sMsg::GetStruct()\n", sStructName, sStructName);
		oFmt.Print("{\n");
		oFmt.Print("	return (C%s&)m_oInst;\n", sStructName);
		oFmt.Print("}\n");
	}
}

CMsgField::CMsgField()
{
	m_pFieldDef = NULL;
	m_nSize = 0;
	m_pFlag = NULL;
	m_nBit = 0;
	m_pData = NULL;
	m_pRecord = NULL;
	m_pVector = NULL;
}

CMsgField::~CMsgField()
{
	Clear();
}

bool CMsgField::IsVector() const
{
	if(m_pData == NULL || m_pFlag)
		return (0 != m_pFieldDef->GetVector());
	return false;
}

bool CMsgField::IsMandatory() const
{
	if(m_pData == NULL || m_pFlag)
		return m_pFieldDef->IsMandatory();
	return true;
}

const CMsgFieldType* CMsgField::GetType() const
{
	return m_pFieldDef->GetType();
}

void CMsgField::Initialize(const CMsgFieldDef* pFieldDef, uint32 nSize,
						   void* pRecordData, uint32 nFieldNo, uint32 nOffset)
{
	m_pFieldDef = pFieldDef;
	m_nSize = nSize;
	m_pFlag = (uint32*)pRecordData+FLW_FLG_L(nFieldNo);
	m_pData = (char*)pRecordData+nOffset;
	m_nBit = FLW_FLG_B(nFieldNo);
}

void CMsgField::Initialize(const CMsgFieldDef* pFieldDef, uint32 nSize, void* pData, bool bInVector)
{
	m_pFieldDef = pFieldDef;
	m_nSize = nSize;
	m_pFlag = NULL;
	m_pData = pData;
	m_nBit = bInVector?0:1;
}

bool CMsgField::IsNull() const
{
	return(m_pFlag && IS_NULL());
}

int8 CMsgField::GetInt8() const
{
	return GetAtom(int8);
}

int16 CMsgField::GetInt16() const
{
	return GetAtom(int16);
}

int32 CMsgField::GetInt32() const
{
	return GetAtom(int32);
}

int64 CMsgField::GetInt64() const
{
	return GetAtom(int64);
}

uint8 CMsgField::GetUInt8() const
{
	return GetAtom(uint8);
}

uint16 CMsgField::GetUInt16() const
{
	return GetAtom(uint16);
}

uint32 CMsgField::GetUInt32() const
{
	return GetAtom(uint32);
}

uint64 CMsgField::GetUInt64() const
{
	return GetAtom(uint64);
}

float CMsgField::GetFloat() const
{
	return GetAtom(float);
}

double CMsgField::GetDouble() const
{
	return GetAtom(double);
}

char* CMsgField::GetString(uint32 * pStrLen) const
{
	char* sRet = NULL;
	if(pStrLen)
		pStrLen[0] = 0;
	if(!IsNull())switch(m_pFieldDef->GetType()->GetType())
		{
		default:
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::GetString() Failure"));
			break;
		case FLW_STRING:
			sRet = (char*)m_pData;
			if(pStrLen)
				pStrLen[0] = CString::StringLength(sRet, m_nSize);
			break;
		case FLW_VSTRING:
			sRet = *(char**)m_pData;
			if(sRet && pStrLen)
				pStrLen[0] = CString::StringLength(sRet);
			break;
		}
	return sRet;
}

CDate CMsgField::GetDate() const
{
	return GetAtom(int32);
}

CTime CMsgField::GetTime() const
{
	return GetAtom(int32);
}

CDateTime CMsgField::GetDateTime() const
{
	return GetAtom(double);
}

uint32 CMsgField::GetStringSize() const
{
	uint32 nLen;
	const char* sRet;
	if(IsVector() || IsNull())
		return 0;
	switch(m_pFieldDef->GetType()->GetType())
	{
	case FLW_INT8:
	case FLW_UINT8:
		return 4;
	case FLW_INT16:
	case FLW_UINT16:
		return 8;
	case FLW_INT32:
	case FLW_UINT32:
		return 12;
	case FLW_INT64:
	case FLW_UINT64:
		return 24;
	case FLW_FLOAT32:
	case FLW_FLOAT64:
		return 64;
	case FLW_DATE:
		return 12;
	case FLW_TIME:
		return 16;
	case FLW_DATETIME:
		return 24;
	case FLW_STRING:
		return m_nSize+1;
	case FLW_VSTRING:
		sRet = (const char*)m_pData;
		sRet = *(const char**)sRet;
		nLen = 0;
		if(sRet)
			nLen = 1 + CString::StringLength(sRet);
		return nLen;
	}
	return 0;
}

void CMsgField::GetAsString(char * pString) const
{
	pString[0] = 0;
	if(!IsNull())switch(m_pFieldDef->GetType()->GetType())
		{
		default:
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::GetAsString() Failure"));
			break;
		case FLW_UINT8:
			StringPrint(pString, "%u8", GetAtom(uint8));
			break;
		case FLW_INT8:
			StringPrint(pString, "%d8", GetAtom(int8));
			break;
		case FLW_UINT16:
			StringPrint(pString, "%u16", GetAtom(uint16));
			break;
		case FLW_INT16:
			StringPrint(pString, "%d16", GetAtom(int16));
			break;
		case FLW_UINT32:
			StringPrint(pString, "%u32", GetAtom(uint32));
			break;
		case FLW_INT32:
			StringPrint(pString, "%d32", GetAtom(int32));
			break;
		case FLW_UINT64:
			StringPrint(pString, "%u64", GetAtom(uint64));
			break;
		case FLW_INT64:
			StringPrint(pString, "%d64", GetAtom(int64));
			break;
		case FLW_FLOAT32:
			StringPrint(pString, "%g", (double)GetAtom(float));
			break;
		case FLW_FLOAT64:
			StringPrint(pString, "%g", GetAtom(double));
			break;
		case FLW_DATE:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CDate(GetAtom(int32)).Print(oFmt);
			break;
		}
		case FLW_TIME:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CTime(GetAtom(int32)).Print(oFmt);
			break;
		}
		case FLW_DATETIME:
		{
			CFormatBinary oFmt((uint8*)pString, 0x7FFFFFFF);
			CDateTime(GetAtom(double)).Print(oFmt);
			break;
		}
		case FLW_STRING:
			CBinary::MemoryCopy(pString, (char*)GetString(NULL), m_nSize);
			pString[m_nSize] = 0;
			break;
		case FLW_VSTRING:
			CString::StringCopy(pString, GetString(NULL));
			break;
		}
}

void CMsgField::SetNull()
{
	if(!m_pFlag || IsNull())
		return;
	Clear();
}

void CMsgField::SetInt8(int8 v)
{
	GetAtom(int8) = v;
	if(m_pFlag)
		FillField();
}

int8& CMsgField::RefInt8()
{
	int8 &v = GetAtom(int8);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetInt16(int16 v)
{
	GetAtom(int16) = v;
	if(m_pFlag)
		FillField();
}

int16& CMsgField::RefInt16()
{
	int16 &v = GetAtom(int16);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetInt32(int32 v)
{
	GetAtom(int32) = v;
	if(m_pFlag)
		FillField();
}

int32& CMsgField::RefInt32()
{
	int32 &v = GetAtom(int32);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetInt64(int64 v)
{
	GetAtom(int64) = v;
	if(m_pFlag)
		FillField();
}

int64& CMsgField::RefInt64()
{
	int64 &v = GetAtom(int64);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetUInt8(uint8 v)
{
	GetAtom(uint8) = v;
	if(m_pFlag)
		FillField();
}

uint8& CMsgField::RefUInt8()
{
	uint8 &v = GetAtom(uint8);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetUInt16(uint16 v)
{
	GetAtom(uint16) = v;
	if(m_pFlag)
		FillField();
}

uint16& CMsgField::RefUInt16()
{
	uint16 &v = GetAtom(uint16);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetUInt32(uint32 v)
{
	GetAtom(uint32) = v;
	if(m_pFlag)
		FillField();
}

uint32& CMsgField::RefUInt32()
{
	uint32 &v = GetAtom(uint32);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetUInt64(uint64 v)
{
	GetAtom(uint64) = v;
	if(m_pFlag)
		FillField();
}

uint64& CMsgField::RefUInt64()
{
	uint64 &v = GetAtom(uint64);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetFloat(float v)
{
	GetAtom(float) = v;
	if(m_pFlag)
		FillField();
}

float& CMsgField::RefFloat()
{
	float &v = GetAtom(float);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetDouble(double v)
{
	GetAtom(double) = v;
	if(m_pFlag)
		FillField();
}

double& CMsgField::RefDouble()
{
	double &v = GetAtom(double);
	if(m_pFlag)
		FillField();
	return v;
}

void CMsgField::SetString(const char* v)
{
	uint32 nSize;
	switch(m_pFieldDef->GetType()->GetType())
	{
	default:
		FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetString() Failure"));
		break;
	case FLW_STRING:
		SetNull();
		if(v && v[0])
		{
			CString::StringCopy((char*)m_pData, v, m_nSize);
			if(m_pFlag)
				FillField();
		}
		break;
	case FLW_VSTRING:
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
			if(m_pFlag)
				FillField();
		}
		break;
	}
}

void CMsgField::SetDate(const CDate& oDate)
{
	GetAtom(int32) = oDate.GetValue();
	if(m_pFlag)
		FillField();
}

void CMsgField::SetTime(const CTime& oTime)
{
	GetAtom(int32) = oTime.GetValue();
	if(m_pFlag)
		FillField();
}

void CMsgField::SetDateTime(const CDateTime& oDateTime)
{
	GetAtom(double) = oDateTime.GetValue();
	if(m_pFlag)
		FillField();
}

void CMsgField::SetFromString(const char* v)
{
	int8 i8 = 0;
	int16 i16 = 0;
	int32 i32 = 0;
	int64 i64 = 0;
	uint8 u8 = 0;
	uint16 u16 = 0;
	uint32 u32 = 0;
	uint64 u64 = 0;
	float f32 = 0.0;
	double f64 = 0.0;

	Clear();
	if(!v || !v[0])
		return;

	switch(m_pFieldDef->GetType()->GetType())
	{
	case FLW_INT8:
		if(StringScan(v, "%i8", &i8) == 1)
		{
			GetAtom(int8) = i8;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(i8) Failure"));
		break;
	case FLW_INT16:
		if(StringScan(v, "%i16", &i16) == 1)
		{
			GetAtom(int16) = i16;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(i16) Failure"));
		break;
	case FLW_INT32:
		if(StringScan(v, "%i32", &i32) == 1)
		{
			GetAtom(int32) = i32;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(i32) Failure"));
		break;
	case FLW_INT64:
		if(StringScan(v, "%i64", &i64) == 1)
		{
			GetAtom(int64) = i64;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(i64) Failure"));
		break;
	case FLW_UINT8:
		if(StringScan(v, "%u8", &u8) == 1)
		{
			GetAtom(uint8) = u8;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(u8) Failure"));
		break;
	case FLW_UINT16:
		if(StringScan(v, "%u16", &u16) == 1)
		{
			GetAtom(uint16) = u16;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(u16) Failure"));
		break;
	case FLW_UINT32:
		if(StringScan(v, "%u32", &u32) == 1)
		{
			GetAtom(uint32) = u32;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(u32) Failure"));
		break;
	case FLW_UINT64:
		if(StringScan(v, "%u64", &u64) == 1)
		{
			GetAtom(uint64) = u64;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(u64) Failure"));
		break;
	case FLW_FLOAT32:
		if(StringScan(v, "%f32", &f32) == 1)
		{
			GetAtom(float) = (float)f32;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(f32) Failure"));
		break;
	case FLW_FLOAT64:
		if(StringScan(v, "%f64", &f64) == 1)
		{
			GetAtom(double) = f64;
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(f64) Failure"));
		break;
	case FLW_DATE:
	{
		CDate oDate;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oDate.Scan(oFmt))
		{
			GetAtom(int32) = oDate.GetValue();
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(date) Failure"));
		break;
	}
	case FLW_TIME:
	{
		CTime oTime;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oTime.Scan(oFmt))
		{
			GetAtom(int32) = oTime.GetValue();
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(time) Failure"));
		break;
	}
	case FLW_DATETIME:
	{
		CDateTime oDateTime;
		CFormatBinary oFmt((uint8*)v, 0x7FFFFFFF);
		if(oDateTime.Scan(oFmt))
		{
			GetAtom(double) = oDateTime.GetValue();
			if(m_pFlag)
				FillField();
		}
		else
			FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(datetime) Failure"));
		break;
	}
	case FLW_STRING:
	case FLW_VSTRING:
		SetString(v);
		break;
	default:
		FocpLog(FOCP_LOG_WARNING, ("CMsgField::SetFromString(struct) Failure"));
		break;
	}
}

CMsgRecord* CMsgField::GetRecord()
{
	if(!m_pRecord && !IsVector())
	{
		const CMsgFieldType* pType = m_pFieldDef->GetType();
		if(pType->GetType() == FLW_STRUCT)
		{
			m_pRecord = *(CMsgRecord**)m_pData;
			if(!m_pRecord)
			{
				m_pRecord = new CMsgRecord((CMsgStruct*)pType);
				*(CMsgRecord**)m_pData = m_pRecord;
				if(m_pFlag)
					FillField();
			}
		}
	}
	return m_pRecord;
}

CMsgVector* CMsgField::GetVector()
{
	if(!m_pVector)
	{
		uint32 nVector = m_pFieldDef->GetVector();
		if(nVector)
		{
			m_pVector = *(CMsgVector**)m_pData;
			if(!m_pVector)
			{
				m_pVector = new CMsgVector(m_pFieldDef, nVector);
				*(CMsgVector**)m_pData = m_pVector;
				if(m_pFlag)
					FillField();
			}
		}
	}
	return m_pVector;
}

void CMsgField::SetNumber(void* pVal)
{
	switch(GetType()->GetType())
	{
	case FLW_INT8:
		SetInt8(*(int8*)pVal);
		break;
	case FLW_INT16:
		SetInt16(*(int16*)pVal);
		break;
	case FLW_INT32:
		SetInt32(*(int32*)pVal);
		break;
	case FLW_INT64:
		SetInt64(*(int64*)pVal);
		break;
	case FLW_UINT8:
		SetUInt8(*(uint8*)pVal);
		break;
	case FLW_UINT16:
		SetUInt16(*(uint16*)pVal);
		break;
	case FLW_UINT32:
		SetUInt32(*(uint32*)pVal);
		break;
	case FLW_UINT64:
		SetUInt64(*(uint64*)pVal);
		break;
	case FLW_FLOAT32:
		SetFloat(*(float*)pVal);
		break;
	case FLW_FLOAT64:
		SetDouble(*(double*)pVal);
		break;
	case FLW_DATE:
		SetDate(*(CDate*)pVal);
		break;
	case FLW_TIME:
		SetTime(*(CTime*)pVal);
		break;
	case FLW_DATETIME:
		SetDateTime(*(CDateTime*)pVal);
		break;
	}
}

void CMsgField::GetNumber(void* pVal)
{
	switch(GetType()->GetType())
	{
	case FLW_INT8:
		*(int8*)pVal = GetInt8();
		break;
	case FLW_INT16:
		*(int16*)pVal = GetInt16();
		break;
	case FLW_INT32:
		*(int32*)pVal = GetInt32();
		break;
	case FLW_INT64:
		*(int64*)pVal = GetInt64();
		break;
	case FLW_UINT8:
		*(uint8*)pVal = GetUInt8();
		break;
	case FLW_UINT16:
		*(uint16*)pVal = GetUInt16();
		break;
	case FLW_UINT32:
		*(uint32*)pVal = GetUInt32();
		break;
	case FLW_UINT64:
		*(uint64*)pVal = GetUInt64();
		break;
	case FLW_FLOAT32:
		*(float*)pVal = GetFloat();
		break;
	case FLW_FLOAT64:
		*(double*)pVal = GetDouble();
		break;
	case FLW_DATE:
		*(int32*)pVal = GetInt32();
		break;
	case FLW_TIME:
		*(int32*)pVal = GetInt32();
		break;
	case FLW_DATETIME:
		*(double*)pVal = GetDouble();
		break;
	}
}

void* CMsgField::RefNumber()
{
	void* pVal = NULL;
	switch(GetType()->GetType())
	{
	case FLW_INT8:
		pVal = &RefInt8();
		break;
	case FLW_INT16:
		pVal = &RefInt16();
		break;
	case FLW_INT32:
		pVal = &RefInt32();
		break;
	case FLW_INT64:
		pVal = &RefInt64();
		break;
	case FLW_UINT8:
		pVal = &RefUInt8();
		break;
	case FLW_UINT16:
		pVal = &RefUInt16();
		break;
	case FLW_UINT32:
		pVal = &RefUInt32();
		break;
	case FLW_UINT64:
		pVal = &RefUInt64();
		break;
	case FLW_FLOAT32:
		pVal = &RefFloat();
		break;
	case FLW_FLOAT64:
		pVal = &RefDouble();
		break;
	case FLW_DATE:
		pVal = &RefInt32();
		break;
	case FLW_TIME:
		pVal = &RefInt32();
		break;
	case FLW_DATETIME:
		pVal = &RefDouble();
		break;
	}
	return pVal;
}

void CMsgField::Clear()
{
	if(m_pData)
	{
		if(IsVector())
		{
			m_pVector = *(CMsgVector**)m_pData;
			if(m_pVector)
			{
				delete m_pVector;
				*(CMsgVector**)m_pData = NULL;
			}
		}
		else
		{
			const CMsgFieldType* pType = m_pFieldDef->GetType();
			uint32 nType = pType->GetType();
			if(nType == FLW_VSTRING)
			{
				char* pData = *(char**)m_pData;
				if(pData)
				{
					delete[] pData;
					*(char**)m_pData = NULL;
				}
			}
			else if(nType == FLW_STRUCT)
			{
				m_pRecord = *(CMsgRecord**)m_pData;
				if(m_pRecord)
				{
					delete m_pRecord;
					*(CMsgRecord**)m_pData = NULL;
				}
			}
		}
		if(m_pVector)
			m_pVector = NULL;
		else if(m_pRecord)
			m_pRecord = NULL;
		if(m_pFlag)
			(*m_pFlag) &= ~m_nBit;
	}
}

void CMsgField::DumpLevel(CFormatter& oFmt, uint32 nLevel)
{
	for(uint32 i=0; i<nLevel; ++i)
		oFmt.Print("	");
}

void CMsgField::Dump(CFormatter& oFmt, uint32 nLevel)
{
	const CMsgFieldType* pType = m_pFieldDef->GetType();
	uint32 nType = pType->GetType();
	if(m_pFlag || m_nBit)
	{
		DumpLevel(oFmt, nLevel);
		oFmt.Print("%s =", m_pFieldDef->GetName());
	}
	if(IsVector())
	{
		GetVector();
		if(nType == FLW_STRUCT)
			oFmt.Print(" %s::\n", pType->GetName());
		else
			oFmt.Print(" ");
		m_pVector->Dump(oFmt, nLevel);
		oFmt.Print(";\n");
	}
	else
	{
		if(nType < FLW_STRUCT)
		{
			uint32 nSize = GetStringSize();
			char* pStr = new char[nSize];
			GetAsString(pStr);
			CString oStr(pStr);
			delete[] pStr;
			switch(nType)
			{
			case FLW_STRING:
			case FLW_VSTRING:
			case FLW_DATE:
			case FLW_TIME:
			case FLW_DATETIME:
				oStr.ToCString(false);
				break;
			}
			oFmt.Print(" %s", oStr.GetStr());
			if(m_pFlag || m_nBit)
				oFmt.Print(";\n");
		}
		else
		{
			GetRecord();
			if(m_pFlag || m_nBit)
				oFmt.Print(" %s::\n", pType->GetName());
			m_pRecord->Dump(oFmt, nLevel);
			oFmt.Print(";\n");
		}
	}
}

bool CMsgField::Check(bool bAll)
{
	uint8 bIsNull = (uint8)IsNull();
	if(bIsNull)
	{
		if(m_pFieldDef->IsMandatory())
		{
			m_pFieldDef->Missing("CMsgField::Check()");
			return false;
		}
	}
	else if(IsVector())
	{
		GetVector();
		uint32 nSize = m_pVector->GetSize();
		if(!nSize && m_pFieldDef->IsMandatory())
		{
			m_pFieldDef->Missing("CMsgField::Check()");
			return false;
		}
		if(bAll)
			return m_pVector->Check(true);
	}
	else
	{
		uint32 nType = m_pFieldDef->GetType()->GetType();
		if(nType == FLW_STRING || nType == FLW_VSTRING)
		{
			uint32 nLen;
			char* s = GetString(&nLen);
			if(!s || !nLen)
			{
				if(m_pFlag && m_pFieldDef->IsMandatory())
				{
					m_pFieldDef->Missing("CMsgField::Check()");
					return false;
				}
			}
		}
		else if(nType == FLW_STRUCT && bAll)
		{
			GetRecord();
			return m_pRecord->Check(true);
		}
	}
	return true;
}

bool CMsgField::Write(CMemoryStream & oStream)
{
	uint32 nLen;
	char* s;

	if(!Check())
		return false;

	uint8 bIsNull = (uint8)IsNull();

	uint32 nPos1 = oStream.GetPosition();

	bool bRet = true;

	if(m_pFlag || m_nBit)
	{
		bRet = oStream.Write((uint32)0);
		if(bRet)
			bRet = oStream.Write(bIsNull);
	}

	if(bRet && !bIsNull)
	{
		if(IsVector())
		{
			GetVector();
			bRet = m_pVector->Write(oStream);
			if(bRet && (m_pFlag || m_nBit))
			{
				uint32 nPos2 = oStream.GetPosition();
				uint32 nFieldSize = nPos2 - nPos1;
				oStream.SetPosition(nPos1);
				oStream.Write(nFieldSize);
				oStream.SetPosition(nPos2);
			}
			return bRet;
		}

		uint32 nType = m_pFieldDef->GetType()->GetType();

		if(m_pFlag || m_nBit)
			bRet = oStream.Write(nType);

		if(bRet)switch(nType)
			{
			case FLW_INT8:
				bRet = oStream.Write(GetAtom(int8));
				break;
			case FLW_INT16:
				bRet = oStream.Write(GetAtom(int16));
				break;
			case FLW_INT32:
			case FLW_DATE:
			case FLW_TIME:
				bRet = oStream.Write(GetAtom(int32));
				break;
			case FLW_INT64:
				bRet = oStream.Write(GetAtom(int64));
				break;
			case FLW_UINT8:
				bRet = oStream.Write(GetAtom(uint8));
				break;
			case FLW_UINT16:
				bRet = oStream.Write(GetAtom(uint16));
				break;
			case FLW_UINT32:
				bRet = oStream.Write(GetAtom(uint32));
				break;
			case FLW_UINT64:
				bRet = oStream.Write(GetAtom(uint64));
				break;
			case FLW_FLOAT32:
				bRet = oStream.Write(GetAtom(float));
				break;
			case FLW_FLOAT64:
			case FLW_DATETIME:
				bRet = oStream.Write(GetAtom(double));
				break;
			case FLW_STRING:
			case FLW_VSTRING:
				s = GetString(&nLen);
				if(!s || !nLen)
					nLen = 0;
				bRet = oStream.Write(nLen);
				if(bRet && nLen)
					bRet = (nLen == oStream.Write(s, nLen));
				break;
			case FLW_STRUCT:
				GetRecord();
				bRet = m_pRecord->Write(oStream);
				break;
			}
	}
	if(bRet && (m_pFlag || m_nBit))
	{
		uint32 nPos2 = oStream.GetPosition();
		uint32 nFieldSize = nPos2 - nPos1;
		oStream.SetPosition(nPos1);
		oStream.Write(nFieldSize);
		oStream.SetPosition(nPos2);
	}
	return bRet;
}

bool CMsgField::Read(CMemoryStream & oStream)
{
	uint32 nLen;
	char* s;

	uint8 bIsNull = false;
	bool bRet = true;
	uint32 nFieldSize;
	uint32 nPos1 = oStream.GetPosition();
	if(m_pFlag || m_nBit)
	{
		bRet = oStream.Read(nFieldSize);
		if(bRet)
			bRet = oStream.Read(bIsNull);
		if(!bRet)
			return bRet;
		SetNull();
	}

	if(bIsNull)
		return Check();

	if(IsVector())
	{
		GetVector();
		bRet = m_pVector->Read(oStream);
	}
	else
	{
		uint32 nType;
		if(m_pFlag || m_nBit)
		{
			bRet = oStream.Read(nType);
			uint32 nSelfType = m_pFieldDef->GetType()->GetType();
			if(bRet && nType != nSelfType)
			{
				FocpLog(FOCP_LOG_WARNING, ("CMsgField::Read(%s.%s) failure: type should be %s, but is %s by received", m_pFieldDef->GetStructName(), m_pFieldDef->GetName(),
										   CMsgFieldType::GetTypeName(nSelfType), CMsgFieldType::GetTypeName(nType)));
				bRet = false;
			}
		}
		else
			nType = m_pFieldDef->GetType()->GetType();
		if(bRet) switch(nType)
			{
			default:
				bRet = false;
				break;
			case FLW_INT8:
				bRet = oStream.Read(GetAtom(int8));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_INT16:
				bRet = oStream.Read(GetAtom(int16));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_INT32:
			case FLW_DATE:
			case FLW_TIME:
				bRet = oStream.Read(GetAtom(int32));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_INT64:
				bRet = oStream.Read(GetAtom(int64));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_UINT8:
				bRet = oStream.Read(GetAtom(uint8));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_UINT16:
				bRet = oStream.Read(GetAtom(uint16));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_UINT32:
				bRet = oStream.Read(GetAtom(uint32));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_UINT64:
				bRet = oStream.Read(GetAtom(uint64));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_FLOAT32:
				bRet = oStream.Read(GetAtom(float));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_FLOAT64:
			case FLW_DATETIME:
				bRet = oStream.Read(GetAtom(double));
				if(bRet && m_pFlag)
					FillField();
				break;
			case FLW_STRING:
			case FLW_VSTRING:
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
					else if(m_pFlag && m_pFieldDef->IsMandatory())
					{
						m_pFieldDef->Missing("CMsgField::Read()");
						return false;
					}
				}
				break;
			case FLW_STRUCT:
				GetRecord();
				bRet = m_pRecord->Read(oStream);
				break;
			}
	}
	if(bRet && (m_pFlag || m_nBit) )
	{
		uint32 nPos2 = oStream.GetPosition();
		if(nPos2 - nPos1 != nFieldSize)
			bRet = false;
	}
	if(bRet)
		bRet = Check(false);

	return bRet;
}

CMsgRecord::CMsgRecord(const CMsgStruct* pType)
{
	m_pType = pType;
	m_nSize = pType->GetSize();
	m_pData = (char*)CBufferManager::GetInstance()->AllocateBuffer(m_nSize);
	if(pType->IsUnion())
	{
		CMsgUnionData* pUnionData = (CMsgUnionData*)m_pData;
		pUnionData->nIdx = (uint32)(-1);
		pUnionData->nData = 0.0;
	}
	else
		CBinary::MemorySet(m_pData, 0, m_nSize);
	m_nCount = pType->GetFieldCount();
	m_pFieldTable = (CMsgField*)CBufferManager::GetInstance()->AllocateBuffer(m_nCount*sizeof(CMsgField));
	for(uint32 i=0; i<m_nCount; ++i)
		new(m_pFieldTable+i) CMsgField();
}

CMsgRecord::~CMsgRecord()
{
	Clear();
	CBufferManager::GetInstance()->DeAllocateBuffer(m_pData);
	m_pData = NULL;
	for(uint32 i=0; i<m_nCount; ++i)
		m_pFieldTable[i].~CMsgField();
	CBufferManager::GetInstance()->DeAllocateBuffer(m_pFieldTable);
	m_pFieldTable = NULL;
}

const CMsgStruct* CMsgRecord::GetStructType() const
{
	return m_pType;
}

CMsgField* CMsgRecord::GetField(uint32 nFieldNo)
{
	if(nFieldNo >= m_nCount)
		return NULL;
	CMsgField* pField = m_pFieldTable + nFieldNo;
	const CMsgFieldDef* pFieldDef = m_pType->GetField(nFieldNo);
	if(m_pType->IsUnion())
	{
		CMsgUnionData* pUnionData = (CMsgUnionData*)m_pData;
		if(pUnionData->nIdx != (uint32)(-1))
		{
			if(pUnionData->nIdx == nFieldNo)
				return pField;
			CMsgField* pOldField = m_pFieldTable + pUnionData->nIdx;
			pOldField->Clear();
			pOldField->m_pData = NULL;
		}
		pUnionData->nIdx = nFieldNo;
		pUnionData->nData = 0.0;
		pField->Initialize(pFieldDef, pFieldDef->GetMaxLen(), (void*)&pUnionData->nData, false);
	}
	else if(!pField->m_pData)
	{
		pField->Initialize(pFieldDef, pFieldDef->GetMaxLen(), (void*)m_pData, nFieldNo, pFieldDef->GetOffset());
		const char* sDefault = pFieldDef->GetDefault();
		if(sDefault && sDefault[0])
			pField->SetFromString(sDefault);
	}
	return pField;
}

CMsgField* CMsgRecord::GetField(const char* sFieldName)
{
	return GetField(m_pType->GetFieldNo(sFieldName));
}

bool CMsgRecord::IsUnion() const
{
	return m_pType->IsUnion();
}

uint32 CMsgRecord::GetUnionField() const
{
	if(m_pType->IsUnion())
	{
		CMsgUnionData* pUnionData = (CMsgUnionData*)m_pData;
		return pUnionData->nIdx;
	}
	return (uint32)(-1);
}

void CMsgRecord::Clear()
{
	for(uint32 i=0; i<m_nCount; ++i)
	{
		CMsgField* pField = m_pFieldTable + i;
		if(pField->m_pData)
			pField->Clear();
	}
	if(m_pType->IsUnion())
	{
		CMsgUnionData* pUnionData = (CMsgUnionData*)m_pData;
		pUnionData->nIdx = (uint32)(-1);
	}
}

void CMsgRecord::Dump(CFormatter& oFmt, uint32 nLevel)
{
	CMsgField::DumpLevel(oFmt, nLevel);
	oFmt.Print("{\n");
	if(IsUnion())
	{
		uint32 nIdx = GetUnionField();
		if(nIdx != (uint32)(-1))
		{
			CMsgField* pField = GetField(nIdx);
			pField->Dump(oFmt, nLevel+1);
		}
	}
	else for(uint32 i=0; i<m_nCount; ++i)
	{
		CMsgField* pField = GetField(i);
		if(!pField->IsNull())
			pField->Dump(oFmt, nLevel+1);
	}
	CMsgField::DumpLevel(oFmt, nLevel);
	oFmt.Print("}");
}

bool CMsgRecord::Check(bool bAll)
{
	bool bRet = true;
	if(IsUnion())
	{
		uint32 nIdx = GetUnionField();
		if(nIdx != (uint32)(-1))
		{
			CMsgField* pField = GetField(nIdx);
			if(!pField->Check(bAll))
				bRet = false;
		}
	}
	else for(uint32 i=0; i<m_nCount; ++i)
		{
			CMsgField* pField = GetField(i);
			if(!pField->Check(bAll))
				bRet = false;
		}
	return bRet;
}

bool CMsgRecord::Write(CMemoryStream & oStream)
{
	uint32 nCount = 0;

	if(!oStream.Write(g_nLocalCode))
		return false;

	bool bLocal = oStream.IsLocalCode();
	oStream.SetLocalCode(true);

	uint32 nIsUnion = m_pType->IsUnion()?1:0;
	if(!oStream.Write(nIsUnion))
	{
		oStream.SetLocalCode(bLocal);
		return false;
	}

	if(nIsUnion)
	{
		CMsgUnionData* pUnionData = (CMsgUnionData*)m_pData;
		if(pUnionData->nIdx != (uint32)(-1))
			nCount = 1;
		if(!oStream.Write(nCount))
		{
			oStream.SetLocalCode(bLocal);
			return false;
		}
		if(nCount)
		{
			if(!oStream.Write(pUnionData->nIdx))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
			if(!GetField(pUnionData->nIdx)->Write(oStream))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
		}
		oStream.SetLocalCode(bLocal);
		return true;
	}
	uint32 i, nPos1 = oStream.GetPosition();
	if(!oStream.Write(nCount))
	{
		oStream.SetLocalCode(bLocal);
		return false;
	}
	for(i=0; i<m_nCount; ++i)
	{
		CMsgField* pField = GetField(i);
		if(pField->IsNull())
		{
			if(pField->IsMandatory())
			{
				oStream.SetLocalCode(bLocal);
				pField->m_pFieldDef->Missing("CMsgRecord::Write()");
				return false;
			}
		}
		else
		{
			if(!oStream.Write(i))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
			if(!pField->Write(oStream))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
			++nCount;
		}
	}
	if(nCount)
	{
		uint32 nPos2 = oStream.GetPosition();
		oStream.SetPosition(nPos1);
		oStream.Write(nCount);
		oStream.SetPosition(nPos2);
	}
	oStream.SetLocalCode(bLocal);
	return true;
}

bool CMsgRecord::Read(CMemoryStream & oStream)
{
	uint32 i, nCount, nIsUnion;

	Clear();

	bool bLocal = oStream.IsLocalCode();

	uint8 nLocalCode;
	if(!oStream.Read(nLocalCode))
		return false;

	oStream.SetLocalCode((nLocalCode == g_nLocalCode));

	if(!oStream.Read(nIsUnion))
	{
		oStream.SetLocalCode(bLocal);
		return false;
	}

	if(nIsUnion != (IsUnion()?1:0))
	{
		oStream.SetLocalCode(bLocal);
		return false;
	}

	if(!oStream.Read(nCount))
	{
		oStream.SetLocalCode(bLocal);
		return false;
	}

	if(nIsUnion)
	{
		if(nCount > 1)
		{
			oStream.SetLocalCode(bLocal);
			return false;
		}
	}

	bool bFailure = false;
	for(i=0; i<nCount; ++i)
	{
		uint32 nCol;
		if(!oStream.Read(nCol))
		{
			oStream.SetLocalCode(bLocal);
			return false;
		}
		CMsgField* pField = GetField(nCol);
		if(pField)
		{
			uint32 nPos = oStream.GetPosition();
			if(!pField->Read(oStream))
			{
				bFailure = true;
				oStream.SetPosition(nPos);
				uint32 nFieldSize;
				if(!oStream.Read(nFieldSize))
				{
					oStream.SetLocalCode(bLocal);
					return false;
				}
				if(nFieldSize <= sizeof(nFieldSize))
				{
					oStream.SetLocalCode(bLocal);
					return false;
				}
				nFieldSize -= sizeof(nFieldSize);
				if(!oStream.Seek(nFieldSize))
				{
					oStream.SetLocalCode(bLocal);
					return false;
				}
			}
		}
		else
		{
			uint32 nFieldSize;
			if(!oStream.Read(nFieldSize))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
			if(nFieldSize <= sizeof(nFieldSize))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
			nFieldSize -= sizeof(nFieldSize);
			if(!oStream.Seek(nFieldSize))
			{
				oStream.SetLocalCode(bLocal);
				return false;
			}
		}
	}

	oStream.SetLocalCode(bLocal);

	if(!Check(false))
		bFailure = true;

	return !bFailure;
}

CMsgVector::CMsgVector(const CMsgFieldDef* pFieldDef, uint32 nCapacity)
{
	m_pFieldDef = pFieldDef;
	m_nCapacity = nCapacity;
	m_nCount = 0;
	m_pData = NULL;
	m_pFieldTable = NULL;
	const CMsgFieldType* pType = pFieldDef->GetType();
	m_nUnitSize = pType->GetSize();
	switch(pType->GetType())
	{
	case FLW_STRING:
		m_nUnitSize = pFieldDef->GetMaxLen();
		break;
	case FLW_VSTRING:
	case FLW_STRUCT:
		m_nUnitSize = sizeof(void*);
		break;
	}
}

CMsgVector::~CMsgVector()
{
	SetSize(0);
}

const CMsgFieldType* CMsgVector::GetType() const
{
	return m_pFieldDef->GetType();
}

uint32 CMsgVector::GetCapacity() const
{
	return m_nCapacity;
}

void CMsgVector::SetSize(uint32 nSize)
{
	if(nSize > m_nCapacity)
		nSize = m_nCapacity;
	if(nSize != m_nCount)
	{
		char* pData = NULL;
		CMsgField* pFieldTable = NULL;
		if(nSize)
		{
			pData = new char[nSize*m_nUnitSize];
			uint32 nCopySize = nSize;
			if(nCopySize > m_nCount)
				nCopySize = m_nCount;
			if(nCopySize)
				CBinary::MemoryCopy(pData, m_pData, nCopySize*m_nUnitSize);
			uint32 nSetSize = nSize - nCopySize;
			if(nSetSize)
				CBinary::MemorySet(pData+nCopySize*m_nUnitSize, 0, nSetSize*m_nUnitSize);
			pFieldTable = new CMsgField[nSize];
			for(uint32 i=0; i<nSize; ++i)
			{
				pFieldTable[i].Initialize(m_pFieldDef, m_pFieldDef->GetMaxLen(), pData+i*m_nUnitSize, true);
				if(i < m_nCount)
					m_pFieldTable[i].m_pData = NULL;
			}
		}
		if(m_pFieldTable)
			delete[] m_pFieldTable;
		if(m_pData)
			delete[] m_pData;
		m_pData = pData;
		m_pFieldTable = pFieldTable;
		m_nCount = nSize;
	}
}

uint32 CMsgVector::GetSize() const
{
	return m_nCount;
}

CMsgField* CMsgVector::GetItem(uint32 nIdx)
{
	if(nIdx >= m_nCount)
		return NULL;
	return m_pFieldTable + nIdx;
}

void CMsgVector::Dump(CFormatter& oFmt, uint32 nLevel)
{
	uint32 nType = m_pFieldDef->GetType()->GetType();
	if(nType == FLW_STRUCT)
	{
		CMsgField::DumpLevel(oFmt, nLevel);
		oFmt.Print("[\n");
	}
	else
		oFmt.Print("[");
	for(uint32 i=0; i<m_nCount; ++i)
	{
		if(nType != FLW_STRUCT && i)
			oFmt.Print(", ");
		CMsgField* pField = GetItem(i);
		pField->Dump(oFmt, nLevel+1);
	}
	if(nType == FLW_STRUCT)
	{
		CMsgField::DumpLevel(oFmt, nLevel);
		oFmt.Print("]");
	}
	else
		oFmt.Print(" ]");
}

bool CMsgVector::Check(bool bAll)
{
	bool bRet = true;
	uint32 i, nCount = GetSize();
	for(i=0; i<nCount; ++i)
	{
		CMsgField* pField = GetItem(i);
		if(!pField->Check(bAll))
			bRet = false;
	}
	return bRet;
}

bool CMsgVector::Write(CMemoryStream & oStream)
{
	uint32 i, nCount = GetSize();
	if(!oStream.Write(nCount))
		return false;
	if(nCount)
	{
		const CMsgFieldType* pType = m_pFieldDef->GetType();
		uint32 nType = pType->GetType();
		if(!oStream.Write(nType))
			return false;
		if(nType < FLW_STRING)
		{
			uint32 nSize = pType->GetSize() * nCount;
			if(nSize != oStream.Write((void*)m_pData, nSize))
				return false;
		}
		else for(i=0; i<nCount; ++i)
			{
				CMsgField* pField = GetItem(i);
				if(!pField->Write(oStream))
					return false;
			}
	}
	return true;
}

bool CMsgVector::Read(CMemoryStream & oStream)
{
	uint32 i, nCount;
	SetSize(0);
	if(!oStream.Read(nCount))
		return false;
	if(nCount)
	{
		uint32 nType;
		if(!oStream.Read(nType))
			return false;
		const CMsgFieldType* pType = m_pFieldDef->GetType();
		if(nType != pType->GetType())
			return false;
		SetSize(nCount);
		if(nType < FLW_STRING)
		{
			uint32 nUnitSize = pType->GetSize();
			uint32 i, nSize = nUnitSize * nCount;
			if(nSize != oStream.Read((void*)m_pData, nSize))
				return false;
			if(nUnitSize > 1 && !oStream.IsLocalCode())
			{
				char* pData = m_pData;
				for(i=0; i<nCount; ++i)
				{
					switch(nUnitSize)
					{
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
					pData += nUnitSize;
				}
			}
		}
		else for(i=0; i<nCount; ++i)
			{
				CMsgField* pField = GetItem(i);
				if(!pField->Read(oStream))
					return false;
			}
	}
	return true;
}

CMsgStructInst::CMsgStructInst(CMsgRecord* pStruct)
{
	m_pStruct = pStruct;
}

CMsgStructInst::CMsgStructInst(const CMsgStructInst& oInst)
{
	m_pStruct = oInst.m_pStruct;
}

CMsgStructInst::~CMsgStructInst()
{
}

bool CMsgStructInst::Valid() const
{
	return m_pStruct!=NULL;
}

CMessage::~CMessage()
{
	Clear();
}

CMessage::CMessage(uint32 nMsgId, CMsgSystem* pMsgSystem):
	m_pMsgSystem(pMsgSystem?pMsgSystem:CMsgSystem::GetInstance()),
	m_nMsgId(nMsgId),
	m_oInst(m_pMsgSystem->AllocMsg(nMsgId))
{
}

CMessage::CMessage(CMemoryStream& oStream, CMsgSystem* pMsgSystem):
	m_pMsgSystem(pMsgSystem?pMsgSystem:CMsgSystem::GetInstance()),
	m_oInst(m_pMsgSystem->Parse(oStream, m_nMsgId))
{
}

CMessage::CMessage(uint32 nMsgId, CMemoryStream& oStream, CMsgSystem* pMsgSystem):
	m_pMsgSystem(pMsgSystem?pMsgSystem:CMsgSystem::GetInstance()),
	m_oInst(m_pMsgSystem->Parse(oStream, nMsgId, false))
{
	m_nMsgId = nMsgId;
}

uint32 CMessage::GetMsgId() const
{
	return m_nMsgId;
}

bool CMessage::Valid() const
{
	return m_oInst.Valid();
}

bool CMessage::Check()
{
	if(m_oInst.m_pStruct == NULL)
		return false;
	return m_oInst.m_pStruct->Check(true);
}

void CMessage::Clear()
{
	if(m_oInst.m_pStruct)
	{
		delete m_oInst.m_pStruct;
		m_oInst.m_pStruct = NULL;
	}
}

bool CMessage::Pack(CMemoryStream& oStream, bool bBuildHead)
{
	return m_pMsgSystem->Pack(oStream, m_oInst.m_pStruct, m_nMsgId, bBuildHead);
}

void CMessage::Dump(CFormatter& oFmt)
{
	if(m_oInst.m_pStruct)
	{
		oFmt.Print("%s =\n", m_oInst.m_pStruct->m_pType->GetName());
		m_oInst.m_pStruct->Dump(oFmt, 0);
		oFmt.Print(";\n");
	}
}

FLW_API uint32 CompileMsg(CFile& oErrFile, CFile& oLangSyntax, CFile &oMsgSyntax, uint32 &nWarning, CMsgSystem* pSystem)
{
	if(pSystem == NULL)
		pSystem = CMsgSystem::GetInstance();
	return pSystem->Compile(oErrFile, oLangSyntax, oMsgSyntax, nWarning);
}

FLW_API void CreateMsgCode(CFile& oHppFile, CFile& oCppFile, const char* sProtocolName, CMsgSystem* pSystem)
{
	if(pSystem == NULL)
		pSystem = CMsgSystem::GetInstance();
	pSystem->CreateCppCode(oHppFile, oCppFile, sProtocolName);
}

CMsgSystem::CMsgSystem():
	m_oAliasTable(NULL, "", false)
{
	new CBaseType<int8, FLW_INT8>(this, "int8");
	new CBaseType<int16, FLW_INT16>(this, "int16");
	new CBaseType<int32, FLW_INT32>(this, "int32");
	new CBaseType<int64, FLW_INT64>(this, "int64");

	new CBaseType<uint8, FLW_UINT8>(this, "uint8");
	new CBaseType<uint16, FLW_UINT16>(this, "uint16");
	new CBaseType<uint32, FLW_UINT32>(this, "uint32");
	new CBaseType<uint64, FLW_UINT64>(this, "uint64");

	new CBaseType<float, FLW_FLOAT32>(this, "float");
	new CBaseType<double, FLW_FLOAT64>(this, "double");

	new CBaseType<int32, FLW_DATE>(this, "date");
	new CBaseType<int32, FLW_TIME>(this, "time");
	new CBaseType<double, FLW_DATETIME>(this, "datetime");

	new CBaseType<char, FLW_STRING>(this, "string");
	new CBaseType<char, FLW_VSTRING>(this, "vstring");
}

CMsgSystem::~CMsgSystem()
{
}

void CMsgSystem::RegType(CMsgFieldType* pType)
{
	const char* sName = pType->GetName();
	if(GetType(sName))
		FocpAbort(("CMsgSystem::RegType('%s') failure", pType->GetName()));
	m_oSystemTypes[sName] = pType;
}

CMsgFieldType* CMsgSystem::GetType(const char* sName) const
{
	CRbTreeNode* pIt = m_oSystemTypes.Find(sName);
	if(pIt == m_oSystemTypes.End())
		return NULL;
	return (CMsgFieldType*)(const CMsgFieldType*)m_oSystemTypes.GetItem(pIt);
}

bool CMsgSystem::Export(uint32 nMsgId, const char* sMsgType)
{
	CMsgFieldType* pType = GetType(sMsgType);
	if(pType == NULL)
		return false;
	if(pType->GetType() != FLW_STRUCT)
		return false;
	if(GetMsgType(nMsgId))
		return false;
	m_oMsgTypes[nMsgId] = (CMsgStruct*)pType;
	return true;
}

CMsgStruct* CMsgSystem::GetMsgType(uint32 nMsgId) const
{
	CRbTreeNode* pIt = m_oMsgTypes.Find(nMsgId);
	if(pIt == m_oMsgTypes.End())
		return NULL;
	return (CMsgStruct*)(const CMsgStruct*)m_oMsgTypes.GetItem(pIt);
}

CMsgRecord* CMsgSystem::AllocMsg(uint32 nMsgId)
{
	CMsgStruct* pStruct = GetMsgType(nMsgId);
	if(!pStruct)
		return NULL;
	return new CMsgRecord(pStruct);
}

CMsgRecord* CMsgSystem::Parse(CMemoryStream& oStream, uint32 &nMsgId, bool bIncHead)
{
	uint32 nMsgSize;
	if(bIncHead)
	{
		oStream.SetLocalCode(false);
		if(!oStream.Read(nMsgId))
			return NULL;
		if(!oStream.Read(nMsgSize))
			return NULL;
	}
	uint32 nPos1 = oStream.GetPosition();
	CMsgRecord* pMsg = AllocMsg(nMsgId);
	if(pMsg && !pMsg->Read(oStream))
	{
		delete pMsg;
		pMsg = NULL;
	}
	if(bIncHead)
	{
		uint32 nPos2 = oStream.GetPosition();
		uint32 nSize = nPos2 - nPos1;
		if(nMsgSize != nSize)
		{
			oStream.SetPosition(nPos1 + nMsgSize);
			if(pMsg)
			{
				delete pMsg;
				pMsg = NULL;
			}
		}
	}
	return pMsg;
}

bool CMsgSystem::Pack(CMemoryStream& oStream, CMsgRecord* pMsg, uint32 nMsgId, bool bBuildHead)
{
	uint32 nMsgSize = 0;
	CMsgStruct* pStruct = (CMsgStruct*)pMsg->GetStructType();
	if(pStruct != GetMsgType(nMsgId))
		return false;
	uint32 nPos1;
	if(bBuildHead)
	{
		oStream.SetLocalCode(false);
		if(!oStream.Write(nMsgId))
			return false;
		nPos1 = oStream.GetPosition();
		if(!oStream.Write(nMsgSize))
			return NULL;
	}
	bool bRet = pMsg->Write(oStream);
	if(bRet && bBuildHead)
	{
		uint32 nPos2 = oStream.GetPosition();
		nMsgSize = nPos2 - nPos1 - sizeof(uint32);
		oStream.SetPosition(nPos1);
		oStream.Write(nMsgSize);
		oStream.SetPosition(nPos2);
	}
	return bRet;
}

CMsgSystem* CMsgSystem::GetInstance()
{
	return CSingleInstance<CMsgSystem>::GetInstance();
}

void CMsgSystem::CreateCppCode(CFile& oHppFile, CFile& oCppFile, const char* sProtocolName)
{
	CFileFormatter oHppFmt(&oHppFile);
	CFileFormatter oCppFmt(&oCppFile);

	oHppFmt.SetLineBuf(false);
	oCppFmt.SetLineBuf(false);

	CString oProName(sProtocolName);
	oProName.ToUpper();

	oHppFmt.Print("\n");
	oHppFmt.Print("#include \"AFC.hpp\"\n");
	oHppFmt.Print("\n");
	oHppFmt.Print("#ifndef _FLW_%s_HPP_\n", oProName.GetStr());
	oHppFmt.Print("#define _FLW_%s_HPP_\n", oProName.GetStr());

	oHppFmt.Print("\n");
	oHppFmt.Print("FOCP_BEGIN();\n");
	oHppFmt.Print("\n");

	CRbTreeNode* pEnd = m_oSystemTypes.End();
	CRbTreeNode* pIt = m_oSystemTypes.First();

	oHppFmt.Print("/**********************************************\n");
	oHppFmt.Print(" * uint32 CompileMsg(CFile& oErrFile, CFile& oLangSyntax, CFile &oMsgSyntax, uint32 &nWarning, CMsgSystem* pSystem=NULL);\n");
	oHppFmt.Print(" *	the function is defined in AFC, it can compile message define script into the message system,\n");
	oHppFmt.Print(" *	return-value: 0=success, other=error information quantity.\n");
	oHppFmt.Print(" *	parameters:\n");
	oHppFmt.Print(" *		oErrFile, the error information is recorded into the file\n");
	oHppFmt.Print(" *		oLangSyntax, the msg script's grammar file\n");
	oHppFmt.Print(" *		oMsgSyntax, the msg script file\n");
	oHppFmt.Print(" *		nWarning, return warning information quantity\n");
	oHppFmt.Print(" *		pSystem, the message system, default use CMsgSystem::GetInstance();\n");
	oHppFmt.Print(" *			the default mode is also suitable for CMessage's construction interface.\n");
	oHppFmt.Print(" *			If you need to support multi-protocol, you can define multi-objects of CMsgSystem.\n");
	oHppFmt.Print(" *			suce as: CMsgSystem oSms, o7Signal, oStockExchange, oGoldExchange;\n");
	oHppFmt.Print(" *\n");
	oHppFmt.Print(" * Datatypes in CMsgSystem are devided into 5 kinds:\n");
	oHppFmt.Print(" *	(1)number type, such as: int8~int64, uint8~uint64, float, double;\n");
	oHppFmt.Print(" *	(2)datetime type, such as: CDate, CTime, CDateTime;\n");
	oHppFmt.Print(" *	(3)string type, such as: CString, map into string or vstring;\n");
	oHppFmt.Print(" *	(4)struct type, map into struct or union, they are created by this code-generater.\n");
	oHppFmt.Print(" *	(5)vector type, its base type may be any number~struct type;\n");
	oHppFmt.Print(" *\n");
	oHppFmt.Print(" * The mantatory field types:\n");
	oHppFmt.Print(" *	class CMsgBaseField(The base value type):\n");
	oHppFmt.Print(" *		CString GetAsString();\n");
	oHppFmt.Print(" *		void SetFromString(const char* v);\n");
	oHppFmt.Print(" *		void SetFromString(const CString& v);\n");
	oHppFmt.Print(" *	class CMsgNumber<TData>: public CMsgBaseField\n");
	oHppFmt.Print(" *		CMsgNumber<TData>& operator=(const CMsgNumber<TData>& v);\n");
	oHppFmt.Print(" *		CMsgNumber<TData>& operator=(TData v);\n");
	oHppFmt.Print(" *		TData Get();\n");
	oHppFmt.Print(" *		void Set(TData v);\n");
	oHppFmt.Print(" *		operator TData();\n");
	oHppFmt.Print(" * 	class CMsgString: public CMsgBaseField\n");
	oHppFmt.Print(" *		CMsgString& operator=(const CMsgString& v);\n");
	oHppFmt.Print(" *		CMsgString& operator=(const char* v);\n");
	oHppFmt.Print(" *		CMsgString& operator=(const CString& v);\n");
	oHppFmt.Print(" *		const char* Get(uint32 &nLen);\n");
	oHppFmt.Print(" *		void Set(const char* v);\n");
	oHppFmt.Print(" *		void Set(const CString& v);\n");
	oHppFmt.Print(" *		operator CString();\n");
	oHppFmt.Print(" *		uint32 GetMaxLen();\n");
	oHppFmt.Print(" * 	class CMsgTime<TData>: public CMsgBaseField\n");
	oHppFmt.Print(" *		CMsgTime<TData>& operator=(const CMsgTime<TData>& v);\n");
	oHppFmt.Print(" *		CMsgTime<TData>& operator=(const TData& v);\n");
	oHppFmt.Print(" *		TData Get();\n");
	oHppFmt.Print(" *		void Set(const TData v);\n");
	oHppFmt.Print(" *		operator TData();\n");
	oHppFmt.Print(" * 	class CMsgStructVal<TStruct>:\n");
	oHppFmt.Print(" *		TStruct Get();\n");
	oHppFmt.Print(" *		operator TStruct();\n");
	oHppFmt.Print(" *	The mantatory field types are unified into CMsgMandatoryHelper<TData>::TValue\n");
	oHppFmt.Print(" *\n");
	oHppFmt.Print(" * The option field type:\n");
	oHppFmt.Print(" * 	class CMsgOptionValue<TData>: public CMsgMandatoryHelper::TValue\n");
	oHppFmt.Print(" *		bool IsNull();\n");
	oHppFmt.Print(" *		void SetNull();\n");
	oHppFmt.Print(" *\n");
	oHppFmt.Print(" * The vector field type:\n");
	oHppFmt.Print(" *	class CMsgBaseVectorField(the base vector TValue type):\n");
	oHppFmt.Print(" *		CString GetAsString(uint32 nIdx);\n");
	oHppFmt.Print(" *		void SetFromString(uint32 nIdx, const char* v);\n");
	oHppFmt.Print(" *		void SetFromString(uint32 nIdx, const CString& v);\n");
	oHppFmt.Print(" *		uint32 GetCapacity();\n");
	oHppFmt.Print(" *		uint32 GetVectorSize();\n");
	oHppFmt.Print(" *		void SetVectorSize(uint32 nSize);\n");
	oHppFmt.Print(" * 	class CMsgNumberVec<TData>: public CMsgBaseVectorField\n");
	oHppFmt.Print(" *		TData Get(uint32 nIdx);\n");
	oHppFmt.Print(" *		void Set(uint32 nIdx, TData v);\n");
	oHppFmt.Print(" *		TData& operator[](uint32 nIdx);\n");
	oHppFmt.Print(" * 	class CMsgStringVec: public CMsgBaseVectorField\n");
	oHppFmt.Print(" *		const char* Get(uint32 nIdx, uint32 &nLen);\n");
	oHppFmt.Print(" *		void void Set(uint32 nIdx, const char* v);\n");
	oHppFmt.Print(" *		void void Set(uint32 nIdx, const CString& v);\n");
	oHppFmt.Print(" *		CString operator[](uint32 nIdx);\n");
	oHppFmt.Print(" *		uint32 GetMaxLen();\n");
	oHppFmt.Print(" * 	CMsgTimeVec<TData>: public CMsgBaseVectorField:\n");
	oHppFmt.Print(" *		TData Get(uint32 nIdx);\n");
	oHppFmt.Print(" *		void Set(uint32 nIdx, const TData& v);\n");
	oHppFmt.Print(" *		TData& operator[](uint32 nIdx);\n");
	oHppFmt.Print(" * 	CMsgStructValVec<TStruct>:\n");
	oHppFmt.Print(" *		TStruct Get(uint32 nIdx);\n");
	oHppFmt.Print(" *		TStruct operator[](uint32 nIdx);\n");
	oHppFmt.Print(" *		uint32 GetCapacity();\n");
	oHppFmt.Print(" *		uint32 GetVectorSize();\n");
	oHppFmt.Print(" *		void SetVectorSize(uint32 nSize);\n");
	oHppFmt.Print(" *	The vector field types are unified into CMsgVectorHelper<TData>::TValue\n");
	oHppFmt.Print(" *\n");
	oHppFmt.Print(" * All these types are unified into CMsgValue<type, tag, width, optional, vector>::TValue\n");
	oHppFmt.Print(" **********************************************/\n");
	oHppFmt.Print("\n");

	oHppFmt.Print("/**********************************************\n");
	oHppFmt.Print(" * declare area:\n");
	oHppFmt.Print(" **********************************************/\n");
	for(; pIt!=pEnd; pIt=m_oSystemTypes.GetNext(pIt))
	{
		uint32 nMsgId;
		CMsgFieldType* pType = m_oSystemTypes.GetItem(pIt);
		if(pType->GetType() != FLW_STRUCT)
			continue;
		CMsgStruct* pStruct = (CMsgStruct*)pType;
		bool bIsMsg = GetMsgId(pStruct, nMsgId);
		const char* sStructName = pStruct->GetName();
		oHppFmt.Print("class C%s;\n", sStructName);
		if(bIsMsg)
			oHppFmt.Print("class C%sMsg;\n", sStructName);
	};
	oHppFmt.Print("\n");

	pIt = m_oSystemTypes.First();
	for(; pIt!=pEnd; pIt=m_oSystemTypes.GetNext(pIt))
	{
		uint32 nMsgId;
		CMsgFieldType* pType = m_oSystemTypes.GetItem(pIt);
		if(pType->GetType() != FLW_STRUCT)
			continue;
		CMsgStruct* pStruct = (CMsgStruct*)pType;
		bool bIsMsg = GetMsgId(pStruct, nMsgId);
		pStruct->CreateCppStructCode(oHppFmt, true);
		if(bIsMsg)
		{
			oHppFmt.Print("\n");
			pStruct->CreateCppMsgCode(oHppFmt, nMsgId, true);
			oHppFmt.Print("\n");
		}
	}

	oHppFmt.Print("\n");
	oHppFmt.Print("FOCP_END();\n");
	oHppFmt.Print("\n");

	oHppFmt.Print("#endif\n");
	oHppFmt.Print("\n");

	oCppFmt.Print("\n");
	oCppFmt.Print("#include \"%s.hpp\"\n", sProtocolName);

	oCppFmt.Print("\n");
	oCppFmt.Print("FOCP_BEGIN();\n");
	oCppFmt.Print("\n");

	pIt = m_oSystemTypes.First();
	for(; pIt!=pEnd; pIt=m_oSystemTypes.GetNext(pIt))
	{
		uint32 nMsgId;
		CMsgFieldType* pType = m_oSystemTypes.GetItem(pIt);
		if(pType->GetType() != FLW_STRUCT)
			continue;
		CMsgStruct* pStruct = (CMsgStruct*)pType;
		bool bIsMsg = GetMsgId(pStruct, nMsgId);
		pStruct->CreateCppStructCode(oCppFmt, false);
		if(bIsMsg)
		{
			oCppFmt.Print("\n");
			pStruct->CreateCppMsgCode(oCppFmt, nMsgId, false);
			oCppFmt.Print("\n");
		}
	}

	oCppFmt.Print("\n");
	oCppFmt.Print("FOCP_END();\n");
	oCppFmt.Print("\n");
}

bool CMsgSystem::GetMsgId(CMsgStruct* pStruct, uint32 &nMsgId)
{
	CRbTreeNode* pEnd = m_oMsgTypes.End();
	CRbTreeNode* pIt = m_oMsgTypes.First();
	for(; pIt!=pEnd; pIt=m_oMsgTypes.GetNext(pIt))
	{
		if(pStruct == m_oMsgTypes.GetItem(pIt))
		{
			nMsgId = m_oMsgTypes.GetKey(pIt);
			return true;
		}
	}
	return false;
}

FOCP_PRIVATE_BEGIN();

class CMsgCompileModule: public CCompileModule
{
private:
	CMsgSystem* m_pMsgSystem;

public:
	CMsgCompileModule(CFile &oErrorFile, CCompileSystem &oCompileSystem, CMsgSystem* pMsgSystem):
		CCompileModule(oErrorFile, oCompileSystem)
	{
		m_pMsgSystem = pMsgSystem;
	}

	CMsgSystem* GetMsgSystem()
	{
		return m_pMsgSystem;
	}
};
FOCP_PRIVATE_END();

void CMsgSystem::CreateStringConst(CRuleStack &oStack, CToken* pConst, CString& oStr)
{
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	const char* sConst = pConst->GetToken();

	uchar c, d;
	CVector<char> oDst;
	char* val = (char*)sConst, *sSrc;

	sSrc = val+1;
	while(sSrc[0])
	{
		if(sSrc[0] == '\"')
			break;
		if(sSrc[0] != '\\')
			oDst.Insert((uint32)(-1), sSrc[0]);
		else
		{
			++sSrc;
			switch(sSrc[0])
			{
			case 'a':
				oDst.Insert((uint32)(-1), '\a');
				break;
			case 'b':
				oDst.Insert((uint32)(-1), '\b');
				break;
			case 'f':
				oDst.Insert((uint32)(-1), '\f');
				break;
			case 'n':
				oDst.Insert((uint32)(-1), '\n');
				break;
			case 'r':
				oDst.Insert((uint32)(-1), '\r');
				break;
			case 't':
				oDst.Insert((uint32)(-1), '\t');
				break;
			case 'v':
				oDst.Insert((uint32)(-1), '\v');
				break;
			case '\'':
				oDst.Insert((uint32)(-1), '\'');
				break;
			case '\"':
				oDst.Insert((uint32)(-1), '\"');
				break;
			case '\\':
				oDst.Insert((uint32)(-1), '\\');
				break;
			case '?':
				oDst.Insert((uint32)(-1), '?');
				break;
			default:
				if(sSrc[0] == 'x' || sSrc[0] == 'X')
				{
					++sSrc;
					c = sSrc[0];
					if(c >= '0' && c <= '9')
						c -= '0';
					else if(c >= 'A' && c <= 'F')
						c = c - 'A' + 10;
					else
						c = c - 'a' + 10;
					sSrc++;
					if(sSrc[0] != '\"')
					{
						c <<= 4;
						d = sSrc[0];
						if(d >= '0' && d <= '9')
							d -= '0';
						else if(d >= 'A' && d <= 'F')
							d = d - 'A' + 10;
						else
							d = d - 'a' + 10;
						c |= d;
					}
				}
				else
				{
					c = sSrc[0] - '0';
					++sSrc;
					if(sSrc[0] != '\"')
					{
						c <<= 3;
						d = sSrc[0] - '0';
						c |= d;
						++sSrc;
						if(sSrc[0] != '\"')
						{
							d = sSrc[0] - '0';
							if(c>(d|(c<<3)))
								pLexModule->OnError(*pConst, "octal char:%u overflow", (((uint32)c)<<3)|d);
							c <<= 3;
							c |= d;
						}
					}
				}
				oDst.Insert((uint32)(-1), (char)c);
			}
		}
		++sSrc;
	}
	oDst.Insert((uint32)(-1), (char)0);
	oStr = oDst.At(0);
}

void CMsgSystem::CreateIntConst(CRuleStack &oStack, CToken* pConst, uint64 &ul, uint32 &nConstBits, bool bSigned)
{
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	const char* sConst = pConst->GetToken();

	enum
	{
		FLW_SUFFIX_NON = 0,
		FLW_SUFFIX_U = 1,
		FLW_SUFFIX_L = 2,
		FLW_SUFFIX_I = 4
	};
	enum
	{
		FLW_INT_CONST_0 = 0,
		FLW_INT_CONST_8 = 8,
		FLW_INT_CONST_16 = 16,
		FLW_INT_CONST_32 = 32,
		FLW_INT_CONST_64 = 64
	};
	enum
	{
		FLW_INT_CONST_SIGNED = 1,
		FLW_INT_CONST_UNSIGNED = 2
	};

	int32 nSys = 10;
	ul = 0;
	char* val = (char*)sConst;
	uchar* s = (uchar*)val;
	uint32 nSuffixType = FLW_SUFFIX_NON;
	nConstBits = FLW_INT_CONST_0;
	uint32 nSigned = FLW_INT_CONST_0;
	uint64 c, d;
	if(s[0] == '0')
	{
		++s;
		nSys = 8;
		if(s[0] == 'x' || s[0] == 'X')
		{
			nSys = 16;
			++s;
		}
	}
	while(s[0])
	{
		if(s[0] == 'u' || s[0] =='U')
		{
			nSuffixType = FLW_SUFFIX_U;
			nSigned = FLW_INT_CONST_UNSIGNED;
			++s;
			break;
		}
		if(s[0] == 'l' || s[0] =='L')
		{
			nSuffixType = FLW_SUFFIX_L;
			nSigned = FLW_INT_CONST_SIGNED;
			++s;
			break;
		}
		if(s[0] == 'i' || s[0] =='I')
		{
			nSuffixType = FLW_SUFFIX_I;
			nSigned = FLW_INT_CONST_SIGNED;
			++s;
			break;
		}
		d = ul * nSys;
		if(d < ul)
			pLexModule->OnError(*pConst, "integer constant overflow");
		ul = d;
		c = s[0];
		if(c >= '0' && c <= '9')
			c -= '0';
		else if(c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if(c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		d += c;
		if(d < ul)
			pLexModule->OnError(*pConst, "integer constant overflow");
		ul = d;
		++s;
	}
	if(nSuffixType == FLW_SUFFIX_U)
	{
		nConstBits = FLW_INT_CONST_32;
		if(s[0] == 'l' || s[0] == 'L')
			nConstBits = FLW_INT_CONST_64;
		else
		{
			if(!CString::StringCompare((const char*)s, "8"))
				nConstBits = FLW_INT_CONST_8;
			else if(!CString::StringCompare((const char*)s, "16"))
				nConstBits = FLW_INT_CONST_16;
			else if(!CString::StringCompare((const char*)s, "32"))
				nConstBits = FLW_INT_CONST_32;
			else if(!CString::StringCompare((const char*)s, "64"))
				nConstBits = FLW_INT_CONST_64;
		}
	}
	else if(nSuffixType == FLW_SUFFIX_L)
	{
		nConstBits = FLW_INT_CONST_64;
		if(s[0] == 'u' || s[0] == 'U')
			nSigned = FLW_INT_CONST_UNSIGNED;
	}
	else if(nSuffixType == FLW_SUFFIX_I)
	{
		if(!CString::StringCompare((const char*)s, "8"))
			nConstBits = FLW_INT_CONST_8;
		else if(!CString::StringCompare((const char*)s, "16"))
			nConstBits = FLW_INT_CONST_16;
		else if(!CString::StringCompare((const char*)s, "32"))
			nConstBits = FLW_INT_CONST_32;
		else if(!CString::StringCompare((const char*)s, "64"))
			nConstBits = FLW_INT_CONST_64;
	}
	else
	{
		nSigned = FLW_INT_CONST_SIGNED;
		if(ul >> 32)
		{
			nConstBits = FLW_INT_CONST_64;
			if(0 > (int64)ul)
				nSigned = FLW_INT_CONST_UNSIGNED;
		}
		else
		{
			nConstBits = FLW_INT_CONST_32;
			if(0 > (int32)ul)
				nSigned = FLW_INT_CONST_UNSIGNED;
		}
	}
	bSigned = (nSigned == FLW_INT_CONST_SIGNED);
	switch(nConstBits)
	{
	case FLW_INT_CONST_8:
		if( (bSigned && (ul > 0x7F)) || (ul > 0xFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		break;
	case FLW_INT_CONST_16:
		if( (bSigned && (ul > 0x7FFF)) || (ul > 0xFFFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		break;
	case FLW_INT_CONST_32:
		if( (bSigned && (ul > 0x7FFFFFFF)) || (ul > 0xFFFFFFFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		break;
	case FLW_INT_CONST_64:
		if( bSigned && (ul > FOCP_UINT64_CONST(0x7FFFFFFFFFFFFFFF)))
			pLexModule->OnError(*pConst, "integer constant overflow");
		break;
	}
}

void CMsgSystem::CreateFloatConst(CRuleStack &oStack, double& d, CToken* pConst)
{
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	const char* sConst = pConst->GetToken();

	char *end, *val;
	float f;

	val = (char*)sConst;

	errno = 0;
	d = strtod(val, &end);
	int32 nError = errno;
	if(nError)
	{
		if(!d)
			pLexModule->OnError(*pConst, "float constant downflow");
		else
			pLexModule->OnError(*pConst, "float constant overflow");
	}
	if(end && (end[0] == 'F' || end[0] == 'f'))
	{
		f = (float)d;
		if(!nError)
		{
			if(*(uint32*)&f == 0x7F800000)
				pLexModule->OnError(*pConst, "float constant overflow");
			if(!f && d)
				pLexModule->OnError(*pConst, "float constant downflow");
		}
		d = f;
	}
}

static CMsgFieldType*& GetFieldType(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(CMsgFieldType*);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	CMsgFieldType* &pRet = *(CMsgFieldType**)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return pRet;
}

void CMsgSystem::CreateStruct(CRuleStack &oStack)
{
	//type CreateStruct(token oKey, token oName);
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMsgFieldType* &pType = GetRuleArgObject<CMsgFieldType*>(oArgv);
	CToken* pKey = GetRuleArgToken(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	CMsgSystem* pMsgSystem = pModule->GetMsgSystem();
	bool bUnion = !CString::StringCompare(pKey->GetToken(), "union", false);
	pType = pMsgSystem->GetType(sTypeName);
	if(pType == NULL)
	{
		pType = new CMsgStruct(pMsgSystem, sTypeName, bUnion);
		pType->SetFile(*pTypeName);
	}
	else if(pType->GetType() != FLW_STRUCT)
	{
		pType = NULL;
		if(bUnion)
			pLexModule->OnError(*pTypeName, "Redefine the base type into union type");
		else
			pLexModule->OnError(*pTypeName, "Redefine the base type into struct type");
	}
	else
	{
		CMsgStruct* pStruct = (CMsgStruct*)pType;
		if(pStruct->IsUnion() != bUnion)
		{
			if(bUnion)
				pLexModule->OnError(*pTypeName, "Redefine the struct type into union type");
			else
				pLexModule->OnError(*pTypeName, "Redefine the union type into struct type");
			pType = NULL;
		}
	}
}

//int CreateMessage(token oName, token nMsgId)
void CMsgSystem::CreateMessage(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();
	CToken* pMsgId = GetRuleArgToken(oArgv);

	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CMsgSystem* pMsgSystem = pModule->GetMsgSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	nRet = 0;
	CMsgFieldType* pType = pMsgSystem->GetType(sTypeName);
	if(pType == NULL)
		nRet = 1;
	else if(pType->GetType() != FLW_STRUCT || ((CMsgStruct*)pType)->IsUnion())
		nRet = 2;
	else
	{
		bool bSigned=false;
		uint64 nTmp=0;
		uint32 nMsgId, nBits;
		CreateIntConst(oStack, pMsgId, nTmp, nBits, bSigned);
		nMsgId = (uint32)nTmp;
		if(nTmp != (uint64)nMsgId)
			pLexModule->OnError(*pMsgId, "should be uint32 const");
		CMsgStruct* pStruct = pMsgSystem->GetMsgType(nMsgId);
		if(pStruct == NULL)
		{
			if(!pMsgSystem->GetMsgId((CMsgStruct*)pType, nBits))
				pMsgSystem->Export(nMsgId, sTypeName);
			else if(nBits != nMsgId)
				nRet = 4;
		}
		else if(pStruct != (CMsgStruct*)pType)
			nRet = 3;
	}
}

//int CloseStruct(type pType)
void CMsgSystem::CloseStruct(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CMsgStruct* pType = (CMsgStruct*)GetFieldType(oArgv);
	if(pType)
	{
		pType->FinishDefine();
		nRet = 0;
	}
	else
		nRet = 1;
}

//	int CreateField(type pType, token oMandatory, token oTypeName, token oFieldName, token oVector, token oSign, token oDefault);
void CMsgSystem::CreateField(CRuleStack &oStack)
{
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CMsgSystem* pMsgSystem = pModule->GetMsgSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);

	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CMsgStruct* pType = (CMsgStruct*)GetFieldType(oArgv);
	CToken* pMandatory = GetRuleArgToken(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();
	CToken* pMaxLen = GetRuleArgToken(oArgv);
	CToken* pFieldName = GetRuleArgToken(oArgv);
	const char* sFieldName = pFieldName->GetToken();
	CToken* pVector = GetRuleArgToken(oArgv);
	CToken* pSign = GetRuleArgToken(oArgv);
	CToken* pDefault = GetRuleArgToken(oArgv);

	CMsgFieldDef* pAlias = (CMsgFieldDef*)pMsgSystem->m_oAliasTable.FindField(sTypeName);

	nRet = 0;
	bool bMandatory = (pMandatory || (pAlias && pAlias->IsMandatory()));

	if(bMandatory)
	{
		if(pVector || (pAlias && pAlias->GetVector()))
		{
			nRet = 1;
			return;
		}
		if(pDefault)
		{
			nRet = 2;
			return;
		}
		if(pType->IsUnion())
		{
			nRet = 18;
			return;
		}
	}
	if(pDefault)
	{
		if(pVector || (pAlias && pAlias->GetVector()))
		{
			nRet = 3;
			return;
		}
		if(pType->IsUnion())
		{
			nRet = 19;
			return;
		}
	}
	CMsgFieldType* pFieldType;
	if(pAlias)
		pFieldType = (CMsgFieldType*)pAlias->GetType();
	else
		pFieldType = pMsgSystem->GetType(sTypeName);
	if(pFieldType == NULL)
	{
		nRet = 4;
		return;
	}
	uint32 nType = pFieldType->GetType();
	if(pMaxLen)
	{
		if((nType != FLW_STRING) && (nType != FLW_VSTRING))
		{
			nRet = 16;
			return;
		}
		if(pAlias && pAlias->GetMaxLen())
		{
			nRet = 21;
			return;
		}
	}
	else if(pAlias == NULL && (nType == FLW_STRING || nType == FLW_VSTRING))
	{
		nRet = 17;
		return;
	}
	bool bSigned;
	uint64 nTmp=0;
	uint32 nBits;
	uint32 nMaxVectorSize = 0, nMaxLen = 0;
	if(pVector)
	{
		if(pAlias && pAlias->GetVector())
		{
			nRet = 20;
			return;
		}
		bSigned = false;
		CreateIntConst(oStack, pVector, nTmp, nBits, bSigned);
		nMaxVectorSize = (uint32)nTmp;
		if(nTmp != (uint64)nMaxVectorSize)
			pLexModule->OnError(*pVector, "should be uint32 const");
	}
	else if(pAlias)
		nMaxVectorSize = pAlias->GetVector();

	if(pMaxLen)
	{
		bSigned = false;
		CreateIntConst(oStack, pMaxLen, nTmp, nBits, bSigned);
		nMaxLen = (uint32)nTmp;
		if(nTmp != (uint64)nMaxLen)
			pLexModule->OnError(*pMaxLen, "should be uint32 const");
		if(nMaxLen == 0)
		{
			nRet = 17;
			return;
		}
	}
	else if(pAlias)
		nMaxLen = pAlias->GetMaxLen();

	CString oDefaultStr;
	double nDefaultFloat = 0.0;
	uint32 nDefault = 0;
	if(pDefault)
	{
		if(nType == FLW_STRUCT)
		{
			nRet = 5;
			return;
		}
		bool bNeg = false;
		if(pSign && pSign->GetToken()[0] == '-')
			bNeg = true;
		const char* sKind = pDefault->GetKind();
		if(!CString::StringCompare(sKind, "ConstInteger", false))
			nDefault = 1;
		else if(!CString::StringCompare(sKind, "ConstFloat", false))
			nDefault = 2;
		else if(!CString::StringCompare(sKind, "ConstString", false))
			nDefault = 3;
		if(nDefault == 3)
		{
			CreateStringConst(oStack, pDefault, oDefaultStr);
			switch(nType)
			{
			default:
				nRet = 6;
				break;
			case FLW_STRING:
			case FLW_VSTRING:
				break;
			case FLW_FLOAT32:
			case FLW_FLOAT64:
				nRet = 7;
				break;
			case FLW_DATE:
			{
				CDate oDate;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oDate.Scan(oFmt))
					nRet = 8;
			}
			break;
			case FLW_TIME:
			{
				CTime oTime;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oTime.Scan(oFmt))
					nRet = 9;
			}
			break;
			case FLW_DATETIME:
			{
				CDateTime oDateTime;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oDateTime.Scan(oFmt))
					nRet = 10;
			}
			}
			if(nRet)
				return;
		}
		else if(nDefault == 2)
		{
			CreateFloatConst(oStack, nDefaultFloat, pDefault);
			if(bNeg)
				nDefaultFloat = -nDefaultFloat;
			switch(nType)
			{
			case FLW_DATE:
			case FLW_TIME:
			case FLW_DATETIME:
			case FLW_STRING:
			case FLW_VSTRING:
				nRet = 11;
				break;
			case FLW_FLOAT32:
			{
				float x = (float)nDefaultFloat;
				if(nDefaultFloat != (double)x)
				{
					nRet = 12;
					break;
				}
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%g", nDefaultFloat);
			}
			break;
			case FLW_FLOAT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%g", nDefaultFloat);
			}
			break;
			default:
				nRet = 6;
				break;
			}
			if(nRet)
				return;
		}
		else if(nDefault == 1)
		{
			bSigned = false;
			CreateIntConst(oStack, pDefault, nTmp, nBits, bSigned);
			switch(nBits)
			{
			case 8:
				if(bSigned)
				{
					int8 x = (int8)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint8 x = (uint8)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int8)x;
					}
					else
						nTmp = x;
				}
				break;
			case 16:
				if(bSigned)
				{
					int16 x = (int16)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint16 x = (uint16)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int16)x;
					}
					else
						nTmp = x;
				}
				break;
			case 32:
				if(bSigned)
				{
					int32 x = (int32)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint32 x = (uint32)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int32)x;
					}
					else
						nTmp = x;
				}
				break;
			case 64:
				if(bSigned)
					nTmp = -(int64)nTmp;
				break;
			}
			switch(nType)
			{
			case FLW_DATE:
			case FLW_TIME:
			case FLW_DATETIME:
			case FLW_STRING:
			case FLW_VSTRING:
				nRet = 11;
				break;
			case FLW_FLOAT32:
			case FLW_FLOAT64:
				switch(nBits)
				{
				case 8:
					if(bSigned)
					{
						int8 x = (int8)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d8", x);
					}
					else
					{
						uint8 x = (uint8)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u8", x);
					}
					break;
				case 16:
					if(bSigned)
					{
						int16 x = (int16)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d16", x);
					}
					else
					{
						uint16 x = (uint16)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u16", x);
					}
					break;
				case 32:
					if(bSigned)
					{
						int32 x = (int32)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d32", x);
					}
					else
					{
						uint32 x = (uint32)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u32", x);
					}
					break;
				case 64:
					if(bSigned)
					{
						int64 x = (int64)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d64", x);
					}
					else
					{
						uint64 x = (uint64)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u64", x);
					}
					break;
				}
				break;
			case FLW_INT8:
			{
				int8 x = (int8)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d8", x);
				}
			}
			break;
			case FLW_UINT8:
			{
				uint8 x = (uint8)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u8", x);
				}
			}
			break;
			case FLW_INT16:
			{
				int16 x = (int16)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d16", x);
				}
			}
			break;
			case FLW_UINT16:
			{
				uint16 x = (uint16)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u16", x);
				}
			}
			break;
			case FLW_INT32:
			{
				int32 x = (int32)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d32", x);
				}
			}
			break;
			case FLW_UINT32:
			{
				uint32 x = (uint32)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u32", x);
				}
			}
			break;
			case FLW_INT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%d64", nTmp);
			}
			break;
			case FLW_UINT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%u64", nTmp);
			}
			break;
			}
		}
		if(nRet)
			return;
	}
	else if(pAlias)
		oDefaultStr = pAlias->GetDefault();

	nRet = 0;
	if(pType == NULL)
		return ;
	else if(pType->Implemented())
		nRet = 14;
	else if(pType->FindField(sFieldName))
		nRet = 15;

	if(nRet)
		return;

	CMsgFieldDef* pField = new CMsgFieldDef(pFieldType, sFieldName);

	if(bMandatory)
		pField->SetMandatory();

	if(nMaxVectorSize)
		pField->SetVector(nMaxVectorSize);

	if(nMaxLen)
		pField->SetMaxLen(nMaxLen);

	if(!oDefaultStr.Empty() && !bMandatory && !nMaxVectorSize)
		pField->SetDefault(oDefaultStr.GetStr());

	pType->AddField(pField);
}

//	int CreateAlias(token oMandatory, token oTypeName, token oMaxLen, token oAliasName, token oVector, token oSign, token oDefault);
void CMsgSystem::CreateAlias(CRuleStack &oStack)
{
	CMsgCompileModule* pModule = (CMsgCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CMsgSystem* pMsgSystem = pModule->GetMsgSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);

	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CMsgStruct* pType = &pMsgSystem->m_oAliasTable;
	CToken* pMandatory = GetRuleArgToken(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();
	CToken* pMaxLen = GetRuleArgToken(oArgv);
	CToken* pAliasName = GetRuleArgToken(oArgv);
	const char* sAliasName = pAliasName->GetToken();
	CToken* pVector = GetRuleArgToken(oArgv);
	CToken* pSign = GetRuleArgToken(oArgv);
	CToken* pDefault = GetRuleArgToken(oArgv);

	nRet = 0;
	if(pMandatory)
	{
		if(pVector)
		{
			nRet = 1;
			return;
		}
		if(pDefault)
		{
			nRet = 2;
			return;
		}
 		if(pType->IsUnion())
 		{
 			nRet = 18;
 			return;
 		}
	}
	if(pDefault)
	{
		if(pVector)
		{
			nRet = 3;
			return;
		}
		if(pType->IsUnion())
		{
			nRet = 19;
			return;
		}
	}
	CMsgFieldType* pFieldType = pMsgSystem->GetType(sTypeName);
	if(pFieldType == NULL)
	{
		nRet = 4;
		return;
	}
	uint32 nType = pFieldType->GetType();
	if(pMaxLen)
	{
		if((nType != FLW_STRING) && (nType != FLW_VSTRING))
		{
			nRet = 16;
			return;
		}
	}
	else if(nType == FLW_STRING || nType == FLW_VSTRING)
	{
		nRet = 17;
		return;
	}
	bool bSigned;
	uint64 nTmp=0;
	uint32 nBits;
	uint32 nMaxVectorSize = 0, nMaxLen = 0;
	if(pVector)
	{
		bSigned = false;
		CreateIntConst(oStack, pVector, nTmp, nBits, bSigned);
		nMaxVectorSize = (uint32)nTmp;
		if(nTmp != (uint64)nMaxVectorSize)
			pLexModule->OnError(*pVector, "should be uint32 const");
	}
	if(pMaxLen)
	{
		bSigned = false;
		CreateIntConst(oStack, pMaxLen, nTmp, nBits, bSigned);
		nMaxLen = (uint32)nTmp;
		if(nTmp != (uint64)nMaxLen)
			pLexModule->OnError(*pMaxLen, "should be uint32 const");
		if(nMaxLen == 0)
		{
			nRet = 17;
			return;
		}
	}
	CString oDefaultStr;
	double nDefaultFloat = 0.0;
	uint32 nDefault = 0;
	if(pDefault)
	{
		if(nType == FLW_STRUCT)
		{
			nRet = 5;
			return;
		}
		bool bNeg = false;
		if(pSign && pSign->GetToken()[0] == '-')
			bNeg = true;
		const char* sKind = pDefault->GetKind();
		if(!CString::StringCompare(sKind, "ConstInteger", false))
			nDefault = 1;
		else if(!CString::StringCompare(sKind, "ConstFloat", false))
			nDefault = 2;
		else if(!CString::StringCompare(sKind, "ConstString", false))
			nDefault = 3;
		if(nDefault == 3)
		{
			CreateStringConst(oStack, pDefault, oDefaultStr);
			switch(nType)
			{
			default:
				nRet = 6;
				break;
			case FLW_STRING:
			case FLW_VSTRING:
				break;
			case FLW_FLOAT32:
			case FLW_FLOAT64:
				nRet = 7;
				break;
			case FLW_DATE:
			{
				CDate oDate;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oDate.Scan(oFmt))
					nRet = 8;
			}
			break;
			case FLW_TIME:
			{
				CTime oTime;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oTime.Scan(oFmt))
					nRet = 9;
			}
			break;
			case FLW_DATETIME:
			{
				CDateTime oDateTime;
				CFormatBinary oFmt((uint8*)oDefaultStr.GetStr(), 0x7FFFFFFF);
				if(!oDateTime.Scan(oFmt))
					nRet = 10;
			}
			}
			if(nRet)
				return;
		}
		else if(nDefault == 2)
		{
			CreateFloatConst(oStack, nDefaultFloat, pDefault);
			if(bNeg)
				nDefaultFloat = -nDefaultFloat;
			switch(nType)
			{
			case FLW_DATE:
			case FLW_TIME:
			case FLW_DATETIME:
			case FLW_STRING:
			case FLW_VSTRING:
				nRet = 11;
				break;
			case FLW_FLOAT32:
			{
				float x = (float)nDefaultFloat;
				if(nDefaultFloat != (double)x)
				{
					nRet = 12;
					break;
				}
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%g", nDefaultFloat);
			}
			break;
			case FLW_FLOAT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%g", nDefaultFloat);
			}
			break;
			default:
				nRet = 6;
				break;
			}
			if(nRet)
				return;
		}
		else if(nDefault == 1)
		{
			bSigned = false;
			CreateIntConst(oStack, pDefault, nTmp, nBits, bSigned);
			switch(nBits)
			{
			case 8:
				if(bSigned)
				{
					int8 x = (int8)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint8 x = (uint8)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int8)x;
					}
					else
						nTmp = x;
				}
				break;
			case 16:
				if(bSigned)
				{
					int16 x = (int16)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint16 x = (uint16)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int16)x;
					}
					else
						nTmp = x;
				}
				break;
			case 32:
				if(bSigned)
				{
					int32 x = (int32)nTmp;
					if(bNeg)
						x = -x;
					nTmp = x;
				}
				else
				{
					uint32 x = (uint32)nTmp;
					if(bNeg)
					{
						bSigned = true;
						nTmp = -(int32)x;
					}
					else
						nTmp = x;
				}
				break;
			case 64:
				if(bSigned)
					nTmp = -(int64)nTmp;
				break;
			}
			switch(nType)
			{
			case FLW_DATE:
			case FLW_TIME:
			case FLW_DATETIME:
			case FLW_STRING:
			case FLW_VSTRING:
				nRet = 11;
				break;
			case FLW_FLOAT32:
			case FLW_FLOAT64:
				switch(nBits)
				{
				case 8:
					if(bSigned)
					{
						int8 x = (int8)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d8", x);
					}
					else
					{
						uint8 x = (uint8)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u8", x);
					}
					break;
				case 16:
					if(bSigned)
					{
						int16 x = (int16)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d16", x);
					}
					else
					{
						uint16 x = (uint16)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u16", x);
					}
					break;
				case 32:
					if(bSigned)
					{
						int32 x = (int32)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d32", x);
					}
					else
					{
						uint32 x = (uint32)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u32", x);
					}
					break;
				case 64:
					if(bSigned)
					{
						int64 x = (int64)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%d64", x);
					}
					else
					{
						uint64 x = (uint64)nTmp;
						CStringFormatter oFmt(&oDefaultStr);
						oFmt.Print("%u64", x);
					}
					break;
				}
				break;
			case FLW_INT8:
			{
				int8 x = (int8)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d8", x);
				}
			}
			break;
			case FLW_UINT8:
			{
				uint8 x = (uint8)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u8", x);
				}
			}
			break;
			case FLW_INT16:
			{
				int16 x = (int16)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d16", x);
				}
			}
			break;
			case FLW_UINT16:
			{
				uint16 x = (uint16)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u16", x);
				}
			}
			break;
			case FLW_INT32:
			{
				int32 x = (int32)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%d32", x);
				}
			}
			break;
			case FLW_UINT32:
			{
				uint32 x = (uint32)nTmp;
				if(nTmp != (uint64)x)
					nRet = 13;
				else
				{
					CStringFormatter oFmt(&oDefaultStr);
					oFmt.Print("%u32", x);
				}
			}
			break;
			case FLW_INT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%d64", nTmp);
			}
			break;
			case FLW_UINT64:
			{
				CStringFormatter oFmt(&oDefaultStr);
				oFmt.Print("%u64", nTmp);
			}
			break;
			}
		}
		if(nRet)
			return;
	}

	nRet = 0;

	if(pMsgSystem->GetType(sAliasName))
		nRet = 14;
	else if(pType->FindField(sAliasName))
		nRet = 15;

	if(nRet)
		return;

	CMsgFieldDef* pField = new CMsgFieldDef(pFieldType, sAliasName);

	if(pDefault)
		pField->SetDefault(oDefaultStr.GetStr());

	if(pMandatory)
		pField->SetMandatory();

	if(pVector)
		pField->SetVector(nMaxVectorSize);

	if(nMaxLen)
		pField->SetMaxLen(nMaxLen);

	pType->AddField(pField);
}

uint32 CMsgSystem::Compile(CFile& oErrFile, CFile& oLangSyntax, CFile &oMsgSyntax, uint32 &nWarning)
{
	CCompileSystem oCompilerSystem;
	CSyntaxSystem* pSyntaxSystem = oCompilerSystem.GetSyntaxSystem();

	if(sizeof(void*) == sizeof(uint32))
		new CCommonRuleType<CMsgFieldType*, ARF_UINT32>(pSyntaxSystem, "type");
	else
		new CCommonRuleType<CMsgFieldType*, ARF_UINT64>(pSyntaxSystem, "type");
	pSyntaxSystem->RegHost("CreateStruct", CreateStruct);
	pSyntaxSystem->RegHost("CreateMessage", CreateMessage);
	pSyntaxSystem->RegHost("CreateField", CreateField);
	pSyntaxSystem->RegHost("CloseStruct", CloseStruct);
	pSyntaxSystem->RegHost("CreateAlias", CreateAlias);

	uint32 nErr = MetaCompile(oErrFile, oLangSyntax, &oCompilerSystem, nWarning);
	if(nErr)
		return nErr;

	CMsgCompileModule oCompileModule(oErrFile, oCompilerSystem, this);
	oCompileModule.GetSyntaxModule()->InitData(pSyntaxSystem);
	oCompileModule.Compile(oMsgSyntax);
	oCompileModule.GetSyntaxModule()->ClearData(pSyntaxSystem);
	nWarning = oCompileModule.GetWarningCount();
	nErr = oCompileModule.GetErrorCount();

	CLexicalModule* pLexModule = oCompileModule.GetLexicalModule();
	CRbTreeNode* pIt = m_oSystemTypes.First();
	CRbTreeNode* pEnd = m_oSystemTypes.End();
	for(; pIt!=pEnd; pIt=m_oSystemTypes.GetNext(pIt))
	{
		CMsgFieldType* pType = m_oSystemTypes.GetItem(pIt);
		if(pType->GetType() == FLW_STRUCT)
		{
			CMsgStruct* pStruct = (CMsgStruct*)pType;
			if(!pStruct->Implemented())
			{
				++nErr;
				pLexModule->OnError(*pStruct, "Undefined struct body for %s", pStruct->GetName());
			}
		}
	}

	return nErr;
}

FOCP_END();
