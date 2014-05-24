
#include "RuleSystem.hpp"

#include <math.h>

#ifdef MSVC
#if _MSC_VER >= 1300
#pragma warning( disable : 4244 )
#pragma warning( disable : 4146 )
#endif
#endif

FOCP_BEGIN();

// ---------------------------------------------------
// CRuleExpress
// ---------------------------------------------------
CRuleExpress::CRuleExpress()
{
}

CRuleExpress::~CRuleExpress()
{
}

CRuleType* CRuleExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return NULL;
}

void* CRuleExpress::GetExpressValue(CRuleStack* pStack) const
{
	return NULL;
}

CRuleExpress* CRuleExpress::Clone() const
{
	CRuleExpress* pExp = new CRuleExpress;
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleExpress::IsVariable() const
{
	return false;
}

bool CRuleExpress::IsLiteral() const
{
	return false;
}

bool CRuleExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	return true;
}

void CRuleExpress::Dump(CString &oDump) const
{
}

bool CRuleExpress::Same(const CRuleExpress &oSrc) const
{
	return true;
}

// ---------------------------------------------------
// CRuleDataExpress
// ---------------------------------------------------
CRuleDataExpress::CRuleDataExpress(CRuleType* pType, void* pData)
{
	m_pType = pType;
	if(pData)
	{
		m_bOwned = false;
		m_pData = (uint8*)pData;
	}
	else
	{
		uint32 nSize = m_pType->GetSize();
		m_bOwned = true;
		m_pData = new uint8[nSize];
		m_pType->InitObject(m_pData);
	}
}

CRuleDataExpress::~CRuleDataExpress()
{
	if(m_bOwned)
	{
		m_pType->ClearObject(m_pData);
		delete[] m_pData;
	}
}

CRuleType* CRuleDataExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pType;
}

void* CRuleDataExpress::GetExpressValue(CRuleStack* pStack) const
{
	if(m_bOwned)
		return m_pData;
	void* pData = m_pData;
	switch(m_pType->TypeCode())
	{
//	case ARF_STRUCT:
	case ARF_STRING:
	case ARF_OBJECT:
		pData = *(void**)pData;
		if(pData == NULL)
		{
			pData = new char[m_pType->GetSize()];
			m_pType->InitObject(pData);
			*(void**)m_pData = pData;
		}
		break;
	}
	return pData;
}

CRuleExpress* CRuleDataExpress::Clone() const
{
	CRuleDataExpress* pExp = new CRuleDataExpress(m_pType, m_bOwned?((void*)NULL):m_pData);
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleDataExpress::IsVariable() const
{
	return !m_bOwned;
}

// ---------------------------------------------------
// CRuleConstExpress
// ---------------------------------------------------
CRuleConstExpress::CRuleConstExpress(uint32 nType, const CRuleVal& oVal):
	m_nType(nType), m_oVal(oVal), m_pType(NULL)
{
	if(m_nType == ARF_STRING)
	{
		if(m_oVal.s.s && m_oVal.s.len)
		{
			m_oVal.s.s = new char[m_oVal.s.len];
			CBinary::MemoryCopy(m_oVal.s.s, oVal.s.s, m_oVal.s.len);
		}
		else
		{
			m_oVal.s.len = 0;
			m_oVal.s.s = NULL;
		}
	}
}

CRuleConstExpress::CRuleConstExpress(bool b):
	m_pType(NULL)
{
	m_nType = ARF_BOOL;
	m_oVal.b = b;
}

CRuleConstExpress::CRuleConstExpress(int8 i8, bool bChar):
	m_pType(NULL)
{
	if(bChar)
		m_nType = ARF_CHAR;
	else
		m_nType = ARF_INT8;
	m_oVal.i8 = i8;
}

CRuleConstExpress::CRuleConstExpress(uint8 u8):
	m_pType(NULL)
{
	m_nType = ARF_UINT8;
	m_oVal.i8 = (int8)u8;
}

CRuleConstExpress::CRuleConstExpress(int16 i16):
	m_pType(NULL)
{
	m_nType = ARF_INT16;
	m_oVal.i16 = i16;
}

CRuleConstExpress::CRuleConstExpress(uint16 u16):
	m_pType(NULL)
{
	m_nType = ARF_UINT16;
	m_oVal.i16 = (int16)u16;
}

CRuleConstExpress::CRuleConstExpress(int32 i32):
	m_pType(NULL)
{
	m_nType = ARF_INT32;
	m_oVal.i32 = i32;
}

CRuleConstExpress::CRuleConstExpress(uint32 u32):
	m_pType(NULL)
{
	m_nType = ARF_UINT32;
	m_oVal.i32 = (int32)u32;
}

CRuleConstExpress::CRuleConstExpress(int64 i64):
	m_pType(NULL)
{
	m_nType = ARF_INT64;
	m_oVal.i64 = i64;
}

CRuleConstExpress::CRuleConstExpress(uint64 u64):
	m_pType(NULL)
{
	m_nType = ARF_UINT64;
	m_oVal.i64 = (int64)u64;
}

CRuleConstExpress::CRuleConstExpress(float f):
	m_pType(NULL)
{
	m_nType = ARF_FLOAT;
	m_oVal.f32 = f;
}

CRuleConstExpress::CRuleConstExpress(double d):
	m_pType(NULL)
{
	m_nType = ARF_DOUBLE;
	m_oVal.f64 = d;
}

CRuleConstExpress::CRuleConstExpress(const char* s, uint32 nSize):
	m_pType(NULL)
{
	m_nType = ARF_STRING;
	if(nSize && s)
	{
		m_oVal.s.s = new char[nSize];
		CBinary::MemoryCopy(m_oVal.s.s, s, nSize);
		m_oVal.s.len = nSize;
	}
	else
	{
		m_oVal.s.s = NULL;
		m_oVal.s.len = 0;
	}
}

CRuleConstExpress::~CRuleConstExpress()
{
	if(m_nType == ARF_STRING)
	{
		if(m_oVal.s.s && m_oVal.s.len)
			delete[] m_oVal.s.s;
		m_oVal.s.s = NULL;
		m_oVal.s.len = 0;
	}
}

CRuleType* CRuleConstExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pType;
}

void* CRuleConstExpress::GetExpressValue(CRuleStack* pStack) const
{
	if(m_pType == NULL)
		return NULL;
	if(m_nType == ARF_STRING)
		return m_oVal.s.s;
	return (void*)&m_oVal;
}

const CRuleVal* CRuleConstExpress::GetValue() const
{
	return &m_oVal;
}

bool CRuleConstExpress::IsLiteral() const
{
	return true;
}

CRuleExpress* CRuleConstExpress::Clone() const
{
	CRuleConstExpress * pExp = new CRuleConstExpress(m_nType, m_oVal);
	pExp->m_pType = m_pType;
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleConstExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	m_pType = NULL;
	switch(m_nType)
	{
	case ARF_BOOL:
		m_pType = oSystem.GetType("bool");
		break;
	case ARF_CHAR:
		m_pType = oSystem.GetType("char");
		break;
	case ARF_INT8:
		m_pType = oSystem.GetType("tiny");
		break;
	case ARF_INT16:
		m_pType = oSystem.GetType("short");
		break;
	case ARF_INT32:
		m_pType = oSystem.GetType("int");
		break;
	case ARF_INT64:
		m_pType = oSystem.GetType("long");
		break;
	case ARF_UINT8:
		m_pType = oSystem.GetType("utiny");
		break;
	case ARF_UINT16:
		m_pType = oSystem.GetType("ushort");
		break;
	case ARF_UINT32:
		m_pType = oSystem.GetType("uint");
		break;
	case ARF_UINT64:
		m_pType = oSystem.GetType("ulong");
		break;
	case ARF_FLOAT:
		m_pType = oSystem.GetType("float");
		break;
	case ARF_DOUBLE:
		m_pType = oSystem.GetType("double");
		break;
	case ARF_STRING:
		m_pType = oSystem.GetType("string");
		break;
	}
	if(!m_pType)
	{
		pChecker->OnError(*this, "invalid const type '%u'", m_nType);
		return false;
	}
	return true;
}

void CRuleConstExpress::Dump(CString &oDump) const
{
	CFormatString oStr;
	switch(m_nType)
	{
	case ARF_BOOL:
		if(m_oVal.b)
			oStr += "true";
		else
			oStr += "false";
		break;
	case ARF_CHAR:
		if(m_oVal.i8)
		{
			CString x;
			x += (char)m_oVal.i8;
			x.ToCString();
			oStr.Print("'%s'", x.GetStr());
		}
		else
			oStr += "'\x00'";
		break;
	case ARF_INT8:
		oStr.Print("%d8i8", m_oVal.i8);
		break;
	case ARF_INT16:
		oStr.Print("%d16i16", m_oVal.i16);
		break;
	case ARF_INT32:
		oStr.Print("%d32", m_oVal.i32);
		break;
	case ARF_INT64:
		oStr.Print("%d64i64", m_oVal.i64);
		break;
	case ARF_UINT8:
		oStr.Print("%u8u8", m_oVal.i8);
		break;
	case ARF_UINT16:
		oStr.Print("%u16u16", m_oVal.i16);
		break;
	case ARF_UINT32:
		oStr.Print("%u32u", m_oVal.i32);
		break;
	case ARF_UINT64:
		oStr.Print("%u64u64", m_oVal.i64);
		break;
	case ARF_FLOAT:
		oStr.Print("%gf", (double)m_oVal.f32);
		break;
	case ARF_DOUBLE:
		oStr.Print("%g", m_oVal.f64);
		break;
	case ARF_STRING:
		if(m_oVal.s.s)
			oStr += CString::GetCString(m_oVal.s.s, false, m_oVal.s.len);
		else
			oStr.ToCString(false);
		break;
	}
	oDump += oStr.GetStr();
}

bool CRuleConstExpress::Same(const CRuleExpress &oSrc) const
{
	const CRuleConstExpress* pExp = dynamic_cast<const CRuleConstExpress*>(&oSrc);
	if(!pExp)
		return NULL;
	if(m_nType != pExp->m_nType)
		return false;
	switch(m_nType)
	{
	case ARF_BOOL:
		if(m_oVal.b != pExp->m_oVal.b)
			return false;
		break;
	case ARF_CHAR:
	case ARF_INT8:
	case ARF_UINT8:
		if(m_oVal.i8 != pExp->m_oVal.i8)
			return false;
		break;
	case ARF_INT16:
	case ARF_UINT16:
		if(m_oVal.i16 != pExp->m_oVal.i16)
			return false;
		break;
	case ARF_INT32:
	case ARF_UINT32:
		if(m_oVal.i32 != pExp->m_oVal.i32)
			return false;
		break;
	case ARF_INT64:
	case ARF_UINT64:
		if(m_oVal.i32 != pExp->m_oVal.i32)
			return false;
		break;
	case ARF_FLOAT:
		if(m_oVal.f32 != pExp->m_oVal.f32)
			return false;
		break;
	case ARF_DOUBLE:
		if(m_oVal.f64 != pExp->m_oVal.f64)
			return false;
		break;
	case ARF_STRING:
		if(m_oVal.s.s == pExp->m_oVal.s.s)
			return true;
		if(m_oVal.s.s == NULL || pExp->m_oVal.s.s == NULL)
			return false;
		if(m_oVal.s.len != pExp->m_oVal.s.len ||
				m_oVal.s.len == 0 || pExp->m_oVal.s.len == 0)
			return false;
		if(CBinary::MemoryCompare(m_oVal.s.s, pExp->m_oVal.s.s, m_oVal.s.len))
			return false;
		break;
	default:
		return false;
		break;
	}
	return true;
}

// ---------------------------------------------------
// CRuleVariableExpress
// ---------------------------------------------------
CRuleVariableExpress::CRuleVariableExpress(const char* sName):
	m_oName(sName), m_pType(NULL)
{
	m_bVariable = true;
}

CRuleVariableExpress::~CRuleVariableExpress()
{
}

CRuleType* CRuleVariableExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pType;
}

void* CRuleVariableExpress::GetExpressValue(CRuleStack* pStack) const
{
	if(m_pType == NULL)
		return NULL;
	return FindVarData(*pStack, m_oName.GetStr());
}

CRuleExpress* CRuleVariableExpress::Clone() const
{
	CRuleVariableExpress* pExp = new CRuleVariableExpress(m_oName.GetStr());
	pExp->m_pType = m_pType;
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleVariableExpress::IsVariable() const
{
	return m_bVariable;
}

bool CRuleVariableExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	CRuleStruct* pVariables = oSystem.GetCheckVariables();
	CRuleStruct* pParameters = oSystem.GetCheckParameters();
	CRuleStruct* pGlobalVariables = oSystem.GetVariables();
	m_pType = FindVarType(oSystem, pVariables, pParameters, pGlobalVariables, pChecker, m_oName.GetStr());
	if(m_pType == NULL)
		return false;
	return true;
}

void* CRuleVariableExpress::FindVarData(CRuleStack &oStack, const char* sName) const
{
	char* sDot;
	CString oName(sName);
	sName = oName.GetStr();
	sDot = CString::CharOfString(sName, '.');
	if(sDot)
		*sDot = '\0';
	void* pData = NULL;
	CRuleStruct* pVar;
	CRuleType* pType;
	if(oStack.pVarv)
	{
		pVar = oStack.pProc->GetVariables();
		pData = FindVarData(oStack, pVar, sName, oStack.pVarv, &pType);
	}
	if(!pData && oStack.pArgv)
	{
		pVar = oStack.pProc->GetParameters();
		pData = FindVarData(oStack, pVar, sName, oStack.pArgv, &pType);
	}
	if(!pData && oStack.pData)
	{
		pVar = oStack.pSystem->GetVariables();
		pData = FindVarData(oStack, pVar, sName, oStack.pData, &pType);
	}
	if(!pData || !sDot)
		return pData;
	sName = sDot + 1;
	if(pType->TypeCode() == ARF_VECTOR)
	{//返回向量大小,sName是size
		CRuleVectorData* pVec = (CRuleVectorData*)pData;
		return &pVec->nCount;
	}
	pVar = (CRuleStruct*)pType;
	if(!(*(void**)pData))
	{
		char* s = new char[pType->GetSize()];
		pType->InitObject(s);
		*(void**)pData = s;
	}
	pData = *(void**)pData;
	while(sName)
	{
		sDot = CString::CharOfString(sName, '.');
		if(sDot)
			*sDot = '\0';
		pData = FindVarData(oStack, pVar, sName, pData, &pType);
		if(!pData || !sDot)
			return pData;
		sName = sDot+1;
		if(pType->TypeCode() == ARF_VECTOR)
		{//返回向量大小,sName是size
			CRuleVectorData* pVec = (CRuleVectorData*)pData;
			return &pVec->nCount;
		}
		pVar = (CRuleStruct*)pType;
		if(!(*(void**)pData))
		{
			char* s = new char[pType->GetSize()];
			pType->InitObject(s);
			*(void**)pData = s;
		}
		pData = *(void**)pData;
	}
	return pData;
}

void* CRuleVariableExpress::FindVarData(CRuleStack &oStack, CRuleStruct* pStruct, const char* sName, void* pVarv, CRuleType** pType) const
{
	void* pOld;
	uint32 nFieldCount = pStruct->GetFieldCount();
	uint32 nFieldNo = pStruct->GetFieldNo(sName);
	if(nFieldNo >= nFieldCount)
		return NULL;
	CRuleVariable* pField = pStruct->GetField(nFieldNo);
	if(pStruct->IsUnion())
	{
		uint32& nCol = *((uint32*)pVarv+1);
		if(nCol != nFieldNo)
		{
			if(nCol < nFieldCount)
				pStruct->ClearObject(pVarv);
			nCol = nFieldNo;
		}
	}
	void* pRet = (char*)pVarv + pField->GetOffset();
	CRuleParameter* pPara = dynamic_cast<CRuleParameter*>(pField);
	*pType = pField->GetType();
	uint32 nFieldType = (*pType)->TypeCode();
	if(pPara && pPara->IsOut())
		pRet = *(void**)pRet;//引用存储指针
	else switch(nFieldType)
	{
	case ARF_STRUCT:
		if(pStruct->IsUnion())
		{
			pOld = pRet;
			pRet = *(void**)pRet;
			if(pRet == NULL)
			{
				pRet = new char[(*pType)->GetSize()];
				(*pType)->InitObject(pRet);
				*(void**)pOld = pRet;
			}
			pRet = pOld;
		}
		break;
	case ARF_STRING:
	case ARF_OBJECT:
		pOld = pRet;
		pRet = *(void**)pRet;
		if(pRet == NULL)
		{
			pRet = new char[(*pType)->GetSize()];
			(*pType)->InitObject(pRet);
			*(void**)pOld = pRet;
		}
	}
	return pRet;
}

CRuleType* CRuleVariableExpress::FindVarType(CRuleSystem &oSystem, CRuleStruct* pVariables, CRuleStruct* pParameters, CRuleStruct* pGlobalVariables, CRuleChecker* pChecker, const char* sName)
{
	char* sDot;
	CString oName(sName);
	sName = oName.GetStr();
	sDot = CString::CharOfString(sName, '.');
	if(sDot)
		*sDot = '\0';
	CRuleVariable* pVar = pVariables->FindField(sName);
	if(!pVar)
		pVar = pParameters->FindField(sName);
	if(!pVar)
		pVar = pGlobalVariables->FindField(sName);
	if(!pVar)
	{
		pChecker->OnError(*this, "the variable or field(%s) is undefined", sName);
		return NULL;
	}
	CRuleType* pType;
	pType = pVar->GetType();
	if(sDot == NULL)
		return pType;
	if(pType->TypeCode() != ARF_STRUCT)
	{
		pChecker->OnError(*this, "'%s' isn't struct type variable or field", sName);
		return NULL;
	}
	sName = sDot + 1;
	while(true)
	{
		sDot = CString::CharOfString(sName, '.');
		if(sDot)
			*sDot = '\0';
		pVar = ((CRuleStruct*)pType)->FindField(sName);
		if(!pVar)
		{
			pChecker->OnError(*this, "the variable or field(%s) is undefined", sName);
			return NULL;
		}
		pType = pVar->GetType();
		if(sDot == NULL)
			break;
		uint32 nTypeCode = pType->TypeCode();
		if(nTypeCode == ARF_VECTOR)
		{
			sName = sDot + 1;
			if(CString::StringCompare(sDot + 1, "size", false))
			{
				pChecker->OnError(*this, "invalid vector method '%s'", sName);
				return NULL;
			}
			return oSystem.GetType("uint");
		}
		if(pType->TypeCode() != ARF_STRUCT)
		{
			pChecker->OnError(*this, "'%s' isn't struct type variable or field", sName);
			return NULL;
		}
		sName = sDot + 1;
	}
	return pType;
}

void CRuleVariableExpress::Dump(CString &oDump) const
{
	oDump += m_oName;
}

bool CRuleVariableExpress::Same(const CRuleExpress &oSrc) const
{
	const CRuleVariableExpress* pExp = dynamic_cast<const CRuleVariableExpress*>(&oSrc);
	if(!pExp)
		return false;
	if(m_oName.Compare(pExp->m_oName, false))
		return false;
	if(m_pType != pExp->m_pType)
		return false;
	return true;
}

// ---------------------------------------------------
// CRuleCallExpress
// ---------------------------------------------------
CRuleCallExpress::CRuleCallExpress(const char* sName, ...)
{
	if(!sName || !sName[0])
		FocpAbort(("CRuleCallExpress::CRuleCallExpress(), invalid rule/func name"));
	m_oName = sName;
	CRuleExpress* exp;
	CVaList args;
	VaStart(args, sName);
	while((exp=VaArg<CRuleExpress*>(args)))
		m_oArgv.Insert((uint32)(-1), exp);
	VaEnd(args);
	m_pFunction = NULL;
	m_pRetExp = NULL;
}

CRuleCallExpress::CRuleCallExpress(const char* sName, const CVector< CAutoPointer<CRuleExpress> > &oArgv)
{
	if(!sName || !sName[0])
		FocpAbort(("CRuleCallExpress::CRuleCallExpress(), invalid rule/func name"));
	m_oName = sName;
	m_oArgv = oArgv;
	m_pFunction = NULL;
	m_pRetExp = NULL;
}

CRuleCallExpress::~CRuleCallExpress()
{
	if(m_pRetExp)
	{
		if(!m_oArgv.GetSize() || m_pRetExp != (CRuleExpress*)m_oArgv[0])
			delete m_pRetExp;
		m_pRetExp = NULL;
	}
}

void CRuleCallExpress::AddExpress(CRuleExpress* pExp)
{
	m_oArgv.Insert((uint32)(-1), pExp);
}

CRuleType* CRuleCallExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pFunction->GetRetType();
}

static const char* g_sDebugFun = GetEnvVar("DebugRuleFunc");
void* CRuleCallExpress::GetExpressValue(CRuleStack* pStack) const
{
	void * pArgv = NULL;
	if(g_sDebugFun && !CString::StringCompare(m_pFunction->GetName(), g_sDebugFun, false))
		Print("Debug CRuleCallExpress::GetExpressValue(%s)\n", m_pFunction->GetName());
	void* pRet = m_pRetExp->GetExpressValue(pStack);
	CRuleStruct* pParaInfo = m_pFunction->GetParameters();
	uint32 nArgvSize = pParaInfo->GetSize();
	if(nArgvSize > sizeof(uint32))
	{
		pArgv = new char[nArgvSize];
		pParaInfo->InitObject(pArgv);
		BuildArgv(pStack, pArgv, pParaInfo, m_oArgv);
	}
	void* pOldArgv = pStack->pArgv;
	pStack->pArgv = pArgv;
	uint32 nArgOff = pStack->nArgOff;
	pStack->nArgOff = 0;
	if(pArgv)
	{
		CRuleParameter* pField = (CRuleParameter*)pParaInfo->GetField((uint32)0);
		pStack->nArgOff = pField->GetOffset();
	}
	m_pFunction->Call(*pStack);
	pStack->pArgv = pOldArgv;
	if(nArgvSize)
	{
		pParaInfo->ClearObject(pArgv);
		delete[] (char*)pArgv;
	}
	pStack->nArgOff = nArgOff;
	return pRet;
}

void CRuleCallExpress::BuildArgv(CRuleStack *pStack, void* pVarv, CRuleStruct *pType, const CVector< CAutoPointer<CRuleExpress> >& oArgv)
{
	uint32 nArgc = pType->GetFieldCount();
	for(uint32 i=0; i<nArgc; ++i)
	{
		CRuleParameter* pField = (CRuleParameter*)pType->GetField(i);
		char* pArg = (char*)pVarv+pField->GetOffset();
		if(pField->IsOut())
		{
			const CRuleExpress* pRight = oArgv[i];
			void* pVal = pRight->GetExpressValue(pStack);
			*(void**)pArg = pVal;
		}
		else
		{
			CRuleDataExpress oLeft(pField->GetType(), pArg);
			const CRuleExpress* pRight = oArgv[i];
			CRuleBinaryExpress oExp(oLeft, ARF_ASIGN_OP, *pRight);
			oExp.SetFile(*pRight);
			oExp.GetExpressValue(pStack);
		}
	}
}

CRuleExpress* CRuleCallExpress::Clone() const
{
	CRuleCallExpress* pExp = new CRuleCallExpress(m_oName.GetStr(), NULL);
	uint32 i=0, nSize = m_oArgv.GetSize();
	if(nSize && m_pRetExp && m_pRetExp == (const CRuleExpress*)m_oArgv[0])
	{
		i = 1;
		pExp->m_pRetExp = m_pRetExp->Clone();
		pExp->m_oArgv.Insert((uint32)(-1), pExp->m_pRetExp);
	}
	for(; i<nSize; ++i)
		pExp->m_oArgv.Insert((uint32)(-1), m_oArgv[i]->Clone());
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleCallExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	uint32 i;
	m_pFunction = oSystem.GetFunc(m_oName.GetStr());
	if(m_pFunction == NULL)
	{
		pChecker->OnError(*this, "the function '%s' is undefined", m_oName.GetStr());
		return false;
	}
	if(m_pRetExp == NULL)
	{
		CRuleType* pType = m_pFunction->GetRetType();
		CRuleStruct* pVariables = oSystem.GetCheckVariables();
		CRuleTmpVar* pVar = pVariables->AllocVar(pType);
		const char* sName = pVar->GetName();
		m_pRetExp = new CRuleVariableExpress(sName);
		m_pRetExp->Check(oSystem, pChecker);
		m_oArgv.Insert(0, m_pRetExp);
	}
	CRuleStruct* pStruct = m_pFunction->GetParameters();
	uint32 nArgc = pStruct->GetFieldCount();
	if(nArgc != m_oArgv.GetSize())
	{
		pChecker->OnError(*this, "the rule '%s' argc(%u) isn't the fact value(%u)", m_oName.GetStr(), nArgc-1, m_oArgv.GetSize()-1);
		return false;
	}
	for(i=0; i<nArgc; ++i)
	{
		CRuleExpress *pExp = (CRuleExpress*)m_oArgv[i];
		if(!pExp->Check(oSystem, pChecker))
			return false;
		CRuleParameter* pVar = (CRuleParameter*)pStruct->GetField(i);
		if(pVar->IsOut())
		{
			if(!pExp->IsVariable())
			{
				pChecker->OnError(*this, "the rule '%s' arg(%u) can't be constant data", m_oName.GetStr(), i);
				return false;
			}
			if(pExp->GetExpressType(&oSystem) != pVar->GetType())
			{
				bool bRet = false;
				uint32 nType1 = pVar->GetType()->TypeCode();
				uint32 nType2 = pExp->GetExpressType(&oSystem)->TypeCode();
				switch(nType1)
				{
				case ARF_CHAR:
					if(nType2 == ARF_INT8 || nType2 == ARF_UINT8)
						bRet = true;
					break;
				case ARF_INT8:
					if(nType2 == ARF_CHAR || nType2 == ARF_UINT8)
						bRet = true;
					break;
				case ARF_UINT8:
					if(nType2 == ARF_CHAR || nType2 == ARF_INT8)
						bRet = true;
					break;
				case ARF_INT16:
					if(nType2 == ARF_UINT16)
						bRet = true;
					break;
				case ARF_UINT16:
					if(nType2 == ARF_INT16)
						bRet = true;
					break;
				case ARF_INT32:
					if(nType2 == ARF_UINT32)
						bRet = true;
					break;
				case ARF_UINT32:
					if(nType2 == ARF_INT32)
						bRet = true;
					break;
				case ARF_INT64:
					if(nType2 == ARF_UINT64)
						bRet = true;
					break;
				case ARF_UINT64:
					if(nType2 == ARF_INT64)
						bRet = true;
					break;
				}
				if(bRet == false)
				{
					pChecker->OnError(*this, "the rule '%s' arg(%u) type isn't consistent", m_oName.GetStr(), i);
					return false;
				}
			}
		}
		else
		{
			if(pExp->GetExpressType(&oSystem) != pVar->GetType())
			{
				bool bRet = false;
				uint32 nType1 = pVar->GetType()->TypeCode();
				uint32 nType2 = pExp->GetExpressType(&oSystem)->TypeCode();
				switch(nType1)
				{
				case ARF_BOOL:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_INT64:
					case ARF_UINT64:
					case ARF_FLOAT:
					case ARF_DOUBLE:
						bRet = true;
						break;
					}
					break;
				case ARF_CHAR:
					switch(nType2)
					{
					case ARF_INT8:
					case ARF_UINT8:
						bRet = true;
						break;
					}
					break;
				case ARF_INT8:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_UINT8:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT8:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
						bRet = true;
						break;
					}
					break;
				case ARF_INT16:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_UINT16:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT16:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
						bRet = true;
						break;
					}
					break;
				case ARF_INT32:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_UINT32:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT32:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
						bRet = true;
						break;
					}
					break;
				case ARF_INT64:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_UINT64:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT64:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_INT64:
						bRet = true;
						break;
					}
					break;
				case ARF_FLOAT:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
						bRet = true;
						break;
					}
					break;
				case ARF_DOUBLE:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_FLOAT:
						bRet = true;
						break;
					}
					break;
				}
				if(bRet == false)
				{
					pChecker->OnError(*this, "the rule '%s' arg(%u) type isn't consistent", m_oName.GetStr(), i);
					return false;
				}
			}
		}
	}
	return true;
}

void CRuleCallExpress::Dump(CString &oDump) const
{
	oDump += m_oName;
	oDump += "(";
	uint32 i=0, nSize = m_oArgv.GetSize();
	if(m_pRetExp && nSize && m_pRetExp==m_oArgv[0])
		i=1;
	for(uint32 j=i; j<nSize; ++j)
	{
		if(j>i)
			oDump += ",";
		m_oArgv[j]->Dump(oDump);
	}
	oDump += ")";
}

bool CRuleCallExpress::Same(const CRuleExpress &oSrc) const
{
	const CRuleCallExpress* pExp = dynamic_cast<const CRuleCallExpress*>(&oSrc);
	if(m_pFunction != pExp->m_pFunction)
		return false;
	uint32 nSize = m_oArgv.GetSize();
	if(nSize != pExp->m_oArgv.GetSize())
		return false;
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!m_oArgv[i]->Same(pExp->m_oArgv[i]))
			return false;
	}
	return true;
}

// ---------------------------------------------------
// CRuleUnaryExpress
// ---------------------------------------------------
CRuleUnaryExpress::CRuleUnaryExpress(uint32 nOpr, const CRuleExpress& oExp)
{
	m_nOperator = nOpr;
	m_pExp = oExp.Clone();
	m_pRet = NULL;
}

CRuleUnaryExpress::~CRuleUnaryExpress()
{
	if(m_pExp)
	{
		delete m_pExp;
		m_pExp = NULL;
	}
	if(m_pRet)
	{
		delete m_pRet;
		m_pRet = NULL;
	}
}

CRuleType* CRuleUnaryExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	if(m_pRet)
		return m_pRet->GetExpressType(pSystem);
	return m_pExp->GetExpressType(pSystem);
}

void* CRuleUnaryExpress::GetExpressValue(CRuleStack* pStack) const
{
	void* pSrc = m_pExp->GetExpressValue(pStack);
	CRuleType* pSrcType = m_pExp->GetExpressType(pStack->pSystem);
	void* pDst = NULL;
	if(m_pRet)
		pDst = m_pRet->GetExpressValue(pStack);
	uint32 nType = pSrcType->TypeCode();
	switch(m_nOperator)
	{
	case ARF_INC_OP: // a++ 	n
		switch(nType)
		{
		case ARF_CHAR:
			*(char*)pDst = (*(char*)pSrc)++;
			break;
		case ARF_INT8:
			*(int8*)pDst = (*(int8*)pSrc)++;
			break;
		case ARF_UINT8:
			*(uint8*)pDst = (*(uint8*)pSrc)++;
			break;
		case ARF_INT16:
			*(int16*)pDst = (*(int16*)pSrc)++;
			break;
		case ARF_UINT16:
			*(uint16*)pDst = (*(uint16*)pSrc)++;
			break;
		case ARF_INT32:
			*(int32*)pDst = (*(int32*)pSrc)++;
			break;
		case ARF_UINT32:
			*(uint32*)pDst = (*(uint32*)pSrc)++;
			break;
		case ARF_INT64:
			*(int64*)pDst = (*(int64*)pSrc)++;
			break;
		case ARF_UINT64:
			*(uint64*)pDst = (*(uint64*)pSrc)++;
			break;
		case ARF_FLOAT:
			*(float*)pDst = (*(float*)pSrc)++;
			break;
		case ARF_DOUBLE:
			*(double*)pDst = (*(double*)pSrc)++;
			break;
		}
		break;
	case ARF_DEC_OP: // a--		n
		switch(nType)
		{
		case ARF_CHAR:
			*(char*)pDst = (*(char*)pSrc)--;
			break;
		case ARF_INT8:
			*(int8*)pDst = (*(int8*)pSrc)--;
			break;
		case ARF_UINT8:
			*(uint8*)pDst = (*(uint8*)pSrc)--;
			break;
		case ARF_INT16:
			*(int16*)pDst = (*(int16*)pSrc)--;
			break;
		case ARF_UINT16:
			*(uint16*)pDst = (*(uint16*)pSrc)--;
			break;
		case ARF_INT32:
			*(int32*)pDst = (*(int32*)pSrc)--;
			break;
		case ARF_UINT32:
			*(uint32*)pDst = (*(uint32*)pSrc)--;
			break;
		case ARF_INT64:
			*(int64*)pDst = (*(int64*)pSrc)--;
			break;
		case ARF_UINT64:
			*(uint64*)pDst = (*(uint64*)pSrc)--;
			break;
		case ARF_FLOAT:
			*(float*)pDst = (*(float*)pSrc)--;
			break;
		case ARF_DOUBLE:
			*(double*)pDst = (*(double*)pSrc)--;
			break;
		}
		break;
	case ARF_SINC_OP: // ++a	n&
		switch(nType)
		{
		case ARF_CHAR:
			(*(char*)pSrc)++;
			break;
		case ARF_INT8:
			(*(int8*)pSrc)++;
			break;
		case ARF_UINT8:
			(*(uint8*)pSrc)++;
			break;
		case ARF_INT16:
			(*(int16*)pSrc)++;
			break;
		case ARF_UINT16:
			(*(uint16*)pSrc)++;
			break;
		case ARF_INT32:
			(*(int32*)pSrc)++;
			break;
		case ARF_UINT32:
			(*(uint32*)pSrc)++;
			break;
		case ARF_INT64:
			(*(int64*)pSrc)++;
			break;
		case ARF_UINT64:
			(*(uint64*)pSrc)++;
			break;
		case ARF_FLOAT:
			(*(float*)pSrc)++;
			break;
		case ARF_DOUBLE:
			(*(double*)pSrc)++;
			break;
		}
		break;
	case ARF_SDEC_OP: // --a	n&
		switch(nType)
		{
		case ARF_CHAR:
			(*(char*)pSrc)--;
			break;
		case ARF_INT8:
			(*(int8*)pSrc)--;
			break;
		case ARF_UINT8:
			(*(uint8*)pSrc)--;
			break;
		case ARF_INT16:
			(*(int16*)pSrc)--;
			break;
		case ARF_UINT16:
			(*(uint16*)pSrc)--;
			break;
		case ARF_INT32:
			(*(int32*)pSrc)--;
			break;
		case ARF_UINT32:
			(*(uint32*)pSrc)--;
			break;
		case ARF_INT64:
			(*(int64*)pSrc)--;
			break;
		case ARF_UINT64:
			(*(uint64*)pSrc)--;
			break;
		case ARF_FLOAT:
			(*(float*)pSrc)--;
			break;
		case ARF_DOUBLE:
			(*(double*)pSrc)--;
			break;
		}
		break;
	case ARF_NON_OP: // ~a		n
		switch(nType)
		{
		case ARF_CHAR:
			*(char*)pDst = ~(*(char*)pSrc);
			break;
		case ARF_INT8:
			*(int8*)pDst = ~(*(int8*)pSrc);
			break;
		case ARF_UINT8:
			*(uint8*)pDst = ~(*(uint8*)pSrc);
			break;
		case ARF_INT16:
			*(int16*)pDst = ~(*(int16*)pSrc);
			break;
		case ARF_UINT16:
			*(uint16*)pDst = ~(*(uint16*)pSrc);
			break;
		case ARF_INT32:
			*(int32*)pDst = ~(*(int32*)pSrc);
			break;
		case ARF_UINT32:
			*(uint32*)pDst = ~(*(uint32*)pSrc);
			break;
		case ARF_INT64:
			*(int64*)pDst = ~(*(int64*)pSrc);
			break;
		case ARF_UINT64:
			*(uint64*)pDst = ~(*(uint64*)pSrc);
			break;
		}
		break;
	case ARF_NEG_OP: // -a		n
		switch(nType)
		{
		case ARF_CHAR:
			*(char*)pDst = -(*(char*)pSrc);
			break;
		case ARF_INT8:
			*(int8*)pDst = -(*(int8*)pSrc);
			break;
		case ARF_UINT8:
			*(uint8*)pDst = -(*(uint8*)pSrc);
			break;
		case ARF_INT16:
			*(int16*)pDst = -(*(int16*)pSrc);
			break;
		case ARF_UINT16:
			*(uint16*)pDst = -(*(uint16*)pSrc);
			break;
		case ARF_INT32:
			*(int32*)pDst = -(*(int32*)pSrc);
			break;
		case ARF_UINT32:
			*(uint32*)pDst = -(*(uint32*)pSrc);
			break;
		case ARF_INT64:
			*(int64*)pDst = -(*(int64*)pSrc);
			break;
		case ARF_UINT64:
			*(uint64*)pDst = -(*(uint64*)pSrc);
			break;
		case ARF_FLOAT:
			*(float*)pDst = -(*(float*)pSrc);
			break;
		case ARF_DOUBLE:
			*(double*)pDst = -(*(double*)pSrc);
			break;
		}
		break;
	case ARF_POST_OP: // +a		n
		switch(nType)
		{
		case ARF_CHAR:
			*(char*)pDst = (*(char*)pSrc);
			break;
		case ARF_INT8:
			*(int8*)pDst = (*(int8*)pSrc);
			break;
		case ARF_UINT8:
			*(uint8*)pDst = (*(uint8*)pSrc);
			break;
		case ARF_INT16:
			*(int16*)pDst = (*(int16*)pSrc);
			break;
		case ARF_UINT16:
			*(uint16*)pDst = (*(uint16*)pSrc);
			break;
		case ARF_INT32:
			*(int32*)pDst = (*(int32*)pSrc);
			break;
		case ARF_UINT32:
			*(uint32*)pDst = (*(uint32*)pSrc);
			break;
		case ARF_INT64:
			*(int64*)pDst = (*(int64*)pSrc);
			break;
		case ARF_UINT64:
			*(uint64*)pDst = (*(uint64*)pSrc);
			break;
		case ARF_FLOAT:
			*(float*)pDst = (*(float*)pSrc);
			break;
		case ARF_DOUBLE:
			*(double*)pDst = (*(double*)pSrc);
			break;
		}
		break;
	case ARF_LNON_OP: // !a		b
		switch(nType)
		{
		case ARF_STRUCT:
			*(bool*)pDst = (*(void**)pSrc)?false:true;
			break;
		case ARF_BOOL:
			*(bool*)pDst = !(*(bool*)pSrc);
			break;
		case ARF_CHAR:
			*(bool*)pDst = !(*(char*)pSrc);
			break;
		case ARF_INT8:
			*(bool*)pDst = !(*(int8*)pSrc);
			break;
		case ARF_UINT8:
			*(bool*)pDst = !(*(uint8*)pSrc);
			break;
		case ARF_INT16:
			*(bool*)pDst = !(*(int16*)pSrc);
			break;
		case ARF_UINT16:
			*(bool*)pDst = !(*(uint16*)pSrc);
			break;
		case ARF_INT32:
			*(bool*)pDst = !(*(int32*)pSrc);
			break;
		case ARF_UINT32:
			*(bool*)pDst = !(*(uint32*)pSrc);
			break;
		case ARF_INT64:
			*(bool*)pDst = !(*(int64*)pSrc);
			break;
		case ARF_UINT64:
			*(bool*)pDst = !(*(uint64*)pSrc);
			break;
		case ARF_FLOAT:
			*(bool*)pDst = !(*(float*)pSrc);
			break;
		case ARF_DOUBLE:
			*(bool*)pDst = !(*(double*)pSrc);
			break;
		}
		break;
	}
	if(pDst)
		return pDst;
	return pSrc;
}

CRuleExpress* CRuleUnaryExpress::Clone() const
{
	CRuleUnaryExpress* pExp = new CRuleUnaryExpress(m_nOperator, *m_pExp);
	if(m_pRet)
		pExp->m_pRet = (CRuleVariableExpress*)m_pRet->Clone();
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleUnaryExpress::IsVariable() const
{
	if(m_pRet)
		return false;
	return true;
}

bool CRuleUnaryExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pExp->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pSrcType = m_pExp->GetExpressType(&oSystem);
	CRuleType* pDstType = NULL;//m_pRet->GetExpressType(pStack->pSystem);
	switch(m_nOperator)
	{
	default:
		bRet = false;
		pChecker->OnError(*this, "invalid unary operator '%u'", m_nOperator);
		break;
	case ARF_INC_OP: // a++ 	n
	case ARF_DEC_OP: // a--		n
	case ARF_SINC_OP: // ++a	n&
	case ARF_SDEC_OP: // --a	n&
	case ARF_NEG_OP: // -a		n
	case ARF_POST_OP: // +a		n
		switch(pSrcType->TypeCode())
		{
		default:
			bRet = false;
			pChecker->OnError(*this, "invalid data type '%s' for unary operator", pSrcType->GetName());
			break;
		case ARF_CHAR:
		case ARF_INT8:
		case ARF_UINT8:
		case ARF_INT16:
		case ARF_UINT16:
		case ARF_INT32:
		case ARF_UINT32:
		case ARF_INT64:
		case ARF_UINT64:
		case ARF_FLOAT:
		case ARF_DOUBLE:
			if(m_nOperator == ARF_SINC_OP || m_nOperator == ARF_SDEC_OP)
			{
				if(!m_pExp->IsVariable())
				{
					pChecker->OnError(*this, "invalid l-value");
					bRet = false;
				}
			}
			else
				pDstType = pSrcType;
			break;
		}
		break;
	case ARF_NON_OP: // ~a		n
		switch(pSrcType->TypeCode())
		{
		default:
			bRet = false;
			pChecker->OnError(*this, "invalid data type '%s' for unary operator", pSrcType->GetName());
			break;
		case ARF_CHAR:
		case ARF_INT8:
		case ARF_UINT8:
		case ARF_INT16:
		case ARF_UINT16:
		case ARF_INT32:
		case ARF_UINT32:
		case ARF_INT64:
		case ARF_UINT64:
			pDstType = pSrcType;
			break;
		}
		break;
	case ARF_LNON_OP: // !a		b
		switch(pSrcType->TypeCode())
		{
		default:
			bRet = false;
			pChecker->OnError(*this, "invalid data type '%s' for unary operator", pSrcType->GetName());
			break;
		case ARF_STRUCT:
		case ARF_BOOL:
		case ARF_CHAR:
		case ARF_INT8:
		case ARF_UINT8:
		case ARF_INT16:
		case ARF_UINT16:
		case ARF_INT32:
		case ARF_UINT32:
		case ARF_INT64:
		case ARF_UINT64:
		case ARF_FLOAT:
		case ARF_DOUBLE:
			pDstType = oSystem.GetType("bool");
			break;
		}
	}
	if(bRet && pDstType)
	{
		CRuleStruct* pVariables = oSystem.GetCheckVariables();
		CRuleTmpVar* pVar = pVariables->AllocVar(pDstType);
		const char* sName = pVar->GetName();
		m_pRet = new CRuleVariableExpress(sName);
		m_pRet->Check(oSystem, pChecker);
	}
	return bRet;
}

// ---------------------------------------------------
// CRuleBinaryExpress
// ---------------------------------------------------
CRuleBinaryExpress::CRuleBinaryExpress(const CRuleExpress& oLeft, uint32 nOpr, const CRuleExpress& oRight)
{
	m_nOperator = nOpr;
	m_pLeft = oLeft.Clone();
	m_pRight = oRight.Clone();
	m_pRet = NULL;
}

CRuleBinaryExpress::~CRuleBinaryExpress()
{
	if(m_pLeft)
	{
		delete m_pLeft;
		m_pLeft = NULL;
	}
	if(m_pRight)
	{
		delete m_pRight;
		m_pRight = NULL;
	}
	if(m_pRet)
	{
		delete m_pRet;
		m_pRet = NULL;
	}
}

CRuleType* CRuleBinaryExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	if(m_pRet)
		return m_pRet->GetExpressType(pSystem);
	if(m_nOperator == ARF_COMMA_OP)
		return m_pRight->GetExpressType(pSystem);
	return m_pLeft->GetExpressType(pSystem);
}

static bool IsInteger(uint32 nType)
{
	bool bRet;
	switch(nType)
	{
	case ARF_CHAR:
	case ARF_INT8:
	case ARF_INT16:
	case ARF_INT32:
	case ARF_INT64:
	case ARF_UINT8:
	case ARF_UINT16:
	case ARF_UINT32:
	case ARF_UINT64:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}
	return bRet;
}

static bool IsFloat(uint32 nType)
{
	bool bRet;
	switch(nType)
	{
	case ARF_FLOAT:
	case ARF_DOUBLE:
		bRet = true;
		break;
	default:
		bRet = false;
		break;
	}
	return bRet;
}

static bool IsNumber(uint32 nType)
{
	return (IsInteger(nType) || IsFloat(nType));
}

#if defined(MSVC) && (_MSC_VER < 1300)

static double UInt64ToDouble(uint64 n)
{
	int64 x = (int64)n;
	if(x >= 0)
		return (double)x;
	double y = (x&FOCP_INT64_MAX);
	return (y+FOCP_INT64_MAX)+1;
}

#define Rule3Tuple_LeftUInt64(TLeft, nOpr, TRight) (UInt64ToDouble(*(TLeft*)pLeftValue) nOpr (*(TRight*)pRightValue))
#define Rule3Tuple_RightUInt64(TLeft, nOpr, TRight) ((*(TLeft*)pLeftValue) nOpr UInt64ToDouble(*(TRight*)pRightValue))
#define Rule4Tuple_LeftUInt64(TRet, TLeft, nOpr, TRight) ((*(TRet*)pRetValue) = Rule3Tuple_LeftUInt64(TLeft, nOpr, TRight))
#define Rule4Tuple_RightUInt64(TRet, TLeft, nOpr, TRight) ((*(TRet*)pRetValue) = Rule3Tuple_RightUInt64(TLeft, nOpr, TRight))
#define RuleFloatMod2_LeftUInt64(TLeft, TRight) ((*(TLeft*)pLeftValue) = (TLeft)fmod(UInt64ToDouble(*(TLeft*)pLeftValue), (double)(*(TRight*)pRightValue)))
#define RuleFloatMod2_RightUInt64(TLeft, TRight) ((*(TLeft*)pLeftValue) = (TLeft)fmod((double)(*(TLeft*)pLeftValue), UInt64ToDouble(*(TRight*)pRightValue)))
#define RuleFloatMod_LeftUInt64(TRet, TLeft, TRight) ((*(TRet*)pRetValue) = (TRet)fmod(UInt64ToDouble(*(TLeft*)pLeftValue), (double)(*(TRight*)pRightValue)))
#define RuleFloatMod_RightUInt64(TRet, TLeft, TRight) ((*(TRet*)pRetValue) = (TRet)fmod((double)(*(TLeft*)pLeftValue), UInt64ToDouble(*(TRight*)pRightValue)))

#endif

#define Rule3Tuple(TLeft, nOpr, TRight) ((*(TLeft*)pLeftValue) nOpr (*(TRight*)pRightValue))
#define Rule4Tuple(TRet, TLeft, nOpr, TRight) ((*(TRet*)pRetValue) = Rule3Tuple(TLeft, nOpr, TRight))
#define RuleFloatMod2(TLeft, TRight) ((*(TLeft*)pLeftValue) = (TLeft)fmod((double)(*(TLeft*)pLeftValue), (double)(*(TRight*)pRightValue)))
#define RuleFloatMod(TRet, TLeft, TRight) ((*(TRet*)pRetValue) = (TRet)fmod((double)(*(TLeft*)pLeftValue), (double)(*(TRight*)pRightValue)))

#define RuleMath32Operator(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule4Tuple(int32, TLeft, nOpr, char); break; \
	case ARF_INT8: Rule4Tuple(int32, TLeft, nOpr, int8); break; \
	case ARF_INT16:Rule4Tuple(int32, TLeft, nOpr, int16); break; \
	case ARF_INT32:Rule4Tuple(int32, TLeft, nOpr, int32); break; \
	case ARF_UINT8:Rule4Tuple(int32, TLeft, nOpr, uint8); break; \
	case ARF_UINT16:Rule4Tuple(int32, TLeft, nOpr, uint16); break; \
	}}while(0)

#define RuleMathIntOperator(nOpr) \
	do{switch(nRetType) \
	{ \
	case ARF_INT32: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleMath32Operator(char, nOpr); break; \
		case ARF_INT8: RuleMath32Operator(int8, nOpr); break; \
		case ARF_INT16: RuleMath32Operator(int16, nOpr); break; \
		case ARF_INT32: RuleMath32Operator(int32, nOpr); break; \
		case ARF_UINT8: RuleMath32Operator(uint8, nOpr); break; \
		case ARF_UINT16: RuleMath32Operator(uint16, nOpr); break; \
		} \
		break; \
	case ARF_UINT32: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(uint32, char, nOpr, uint32); break; \
		case ARF_INT8: Rule4Tuple(uint32, int8, nOpr, uint32); break; \
		case ARF_INT16:Rule4Tuple(uint32, int16, nOpr, uint32); break; \
		case ARF_INT32:Rule4Tuple(uint32, int32, nOpr, uint32); break; \
		case ARF_UINT8:Rule4Tuple(uint32, uint8, nOpr, uint32); break; \
		case ARF_UINT16:Rule4Tuple(uint32, uint16, nOpr, uint32); break; \
		case ARF_UINT32: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(uint32, uint32, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(uint32, uint32, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(uint32, uint32, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(uint32, uint32, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(uint32, uint32, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(uint32, uint32, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(uint32, uint32, nOpr, uint32); break; \
			} \
			break; \
		} \
		break; \
	case ARF_INT64: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(int64, char, nOpr, int64); break; \
		case ARF_INT8: Rule4Tuple(int64, int8, nOpr, int64); break; \
		case ARF_INT16:Rule4Tuple(int64, int16, nOpr, int64); break; \
		case ARF_INT32:Rule4Tuple(int64, int32, nOpr, int64); break; \
		case ARF_UINT8:Rule4Tuple(int64, uint8, nOpr, int64); break; \
		case ARF_UINT16:Rule4Tuple(int64, uint16, nOpr, int64); break; \
		case ARF_UINT32:Rule4Tuple(int64, uint32, nOpr, int64); break; \
		case ARF_INT64: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(int64, int64, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(int64, int64, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(int64, int64, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(int64, int64, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(int64, int64, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(int64, int64, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(int64, int64, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(int64, int64, nOpr, int64); break; \
			} \
			break; \
		} \
		break; \
	case ARF_UINT64: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(uint64, char, nOpr, uint64); break; \
		case ARF_INT8: Rule4Tuple(uint64, int8, nOpr, uint64); break; \
		case ARF_INT16:Rule4Tuple(uint64, int16, nOpr, uint64); break; \
		case ARF_INT32:Rule4Tuple(uint64, int32, nOpr, uint64); break; \
		case ARF_UINT8:Rule4Tuple(uint64, uint8, nOpr, uint64); break; \
		case ARF_UINT16:Rule4Tuple(uint64, uint16, nOpr, uint64); break; \
		case ARF_UINT32:Rule4Tuple(uint64, uint32, nOpr, uint64); break; \
		case ARF_INT64:Rule4Tuple(uint64, int64, nOpr, uint64); break; \
		case ARF_UINT64: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(uint64, uint64, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(uint64, uint64, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(uint64, uint64, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(uint64, uint64, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(uint64, uint64, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(uint64, uint64, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(uint64, uint64, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(uint64, uint64, nOpr, int64); break; \
			case ARF_UINT64:Rule4Tuple(uint64, uint64, nOpr, uint64); break; \
			} \
			break; \
		} \
		break; \
	}}while(0)

#if defined(MSVC) && (_MSC_VER < 1300)

#define RuleMathFloatOperator(nOpr) \
	do{switch(nRetType) \
	{ \
	case ARF_FLOAT: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(float, char, nOpr, float); break; \
		case ARF_INT8: Rule4Tuple(float, int8, nOpr, float); break; \
		case ARF_INT16:Rule4Tuple(float, int16, nOpr, float); break; \
		case ARF_INT32:Rule4Tuple(float, int32, nOpr, float); break; \
		case ARF_UINT8:Rule4Tuple(float, uint8, nOpr, float); break; \
		case ARF_UINT16:Rule4Tuple(float, uint16, nOpr, float); break; \
		case ARF_UINT32:Rule4Tuple(float, uint32, nOpr, float); break; \
		case ARF_INT64:Rule4Tuple(float, int64, nOpr, float); break; \
		case ARF_UINT64:Rule4Tuple_LeftUInt64(float, uint64, nOpr, float); break; \
		case ARF_FLOAT: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(float, float, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(float, float, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(float, float, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(float, float, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(float, float, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(float, float, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(float, float, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(float, float, nOpr, int64); break; \
			case ARF_UINT64:Rule4Tuple_RightUInt64(float, float, nOpr, uint64); break; \
			case ARF_FLOAT:Rule4Tuple(float, float, nOpr, float); break; \
			} \
			break; \
		} \
		break; \
	case ARF_DOUBLE: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(double, char, nOpr, double); break; \
		case ARF_INT8: Rule4Tuple(double, int8, nOpr, double); break; \
		case ARF_INT16:Rule4Tuple(double, int16, nOpr, double); break; \
		case ARF_INT32:Rule4Tuple(double, int32, nOpr, double); break; \
		case ARF_UINT8:Rule4Tuple(double, uint8, nOpr, double); break; \
		case ARF_UINT16:Rule4Tuple(double, uint16, nOpr, double); break; \
		case ARF_UINT32:Rule4Tuple(double, uint32, nOpr, double); break; \
		case ARF_INT64:Rule4Tuple(double, int64, nOpr, double); break; \
		case ARF_UINT64:Rule4Tuple_LeftUInt64(double, uint64, nOpr, double); break; \
		case ARF_FLOAT:Rule4Tuple(double, float, nOpr, double); break; \
		case ARF_DOUBLE: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(double, double, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(double, double, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(double, double, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(double, double, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(double, double, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(double, double, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(double, double, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(double, double, nOpr, int64); break; \
			case ARF_UINT64:Rule4Tuple_RightUInt64(double, double, nOpr, uint64); break; \
			case ARF_FLOAT:Rule4Tuple(double, double, nOpr, float); break; \
			case ARF_DOUBLE:Rule4Tuple(double, double, nOpr, double); break; \
			} \
			break; \
		} \
		break; \
	}}while(0)

#else

#define RuleMathFloatOperator(nOpr) \
	do{switch(nRetType) \
	{ \
	case ARF_FLOAT: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(float, char, nOpr, float); break; \
		case ARF_INT8: Rule4Tuple(float, int8, nOpr, float); break; \
		case ARF_INT16:Rule4Tuple(float, int16, nOpr, float); break; \
		case ARF_INT32:Rule4Tuple(float, int32, nOpr, float); break; \
		case ARF_UINT8:Rule4Tuple(float, uint8, nOpr, float); break; \
		case ARF_UINT16:Rule4Tuple(float, uint16, nOpr, float); break; \
		case ARF_UINT32:Rule4Tuple(float, uint32, nOpr, float); break; \
		case ARF_INT64:Rule4Tuple(float, int64, nOpr, float); break; \
		case ARF_UINT64:Rule4Tuple(float, uint64, nOpr, float); break; \
		case ARF_FLOAT: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(float, float, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(float, float, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(float, float, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(float, float, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(float, float, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(float, float, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(float, float, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(float, float, nOpr, int64); break; \
			case ARF_UINT64:Rule4Tuple(float, float, nOpr, uint64); break; \
			case ARF_FLOAT:Rule4Tuple(float, float, nOpr, float); break; \
			} \
			break; \
		} \
		break; \
	case ARF_DOUBLE: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(double, char, nOpr, double); break; \
		case ARF_INT8: Rule4Tuple(double, int8, nOpr, double); break; \
		case ARF_INT16:Rule4Tuple(double, int16, nOpr, double); break; \
		case ARF_INT32:Rule4Tuple(double, int32, nOpr, double); break; \
		case ARF_UINT8:Rule4Tuple(double, uint8, nOpr, double); break; \
		case ARF_UINT16:Rule4Tuple(double, uint16, nOpr, double); break; \
		case ARF_UINT32:Rule4Tuple(double, uint32, nOpr, double); break; \
		case ARF_INT64:Rule4Tuple(double, int64, nOpr, double); break; \
		case ARF_UINT64:Rule4Tuple(double, uint64, nOpr, double); break; \
		case ARF_FLOAT:Rule4Tuple(double, float, nOpr, double); break; \
		case ARF_DOUBLE: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: Rule4Tuple(double, double, nOpr, char); break; \
			case ARF_INT8: Rule4Tuple(double, double, nOpr, int8); break; \
			case ARF_INT16:Rule4Tuple(double, double, nOpr, int16); break; \
			case ARF_INT32:Rule4Tuple(double, double, nOpr, int32); break; \
			case ARF_UINT8:Rule4Tuple(double, double, nOpr, uint8); break; \
			case ARF_UINT16:Rule4Tuple(double, double, nOpr, uint16); break; \
			case ARF_UINT32:Rule4Tuple(double, double, nOpr, uint32); break; \
			case ARF_INT64:Rule4Tuple(double, double, nOpr, int64); break; \
			case ARF_UINT64:Rule4Tuple(double, double, nOpr, uint64); break; \
			case ARF_FLOAT:Rule4Tuple(double, double, nOpr, float); break; \
			case ARF_DOUBLE:Rule4Tuple(double, double, nOpr, double); break; \
			} \
			break; \
		} \
		break; \
	}}while(0)

#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)

#define RuleMathFloatMod() \
	do{switch(nRetType) \
	{ \
	case ARF_FLOAT: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleFloatMod(float, char, float); break; \
		case ARF_INT8: RuleFloatMod(float, int8, float); break; \
		case ARF_INT16:RuleFloatMod(float, int16, float); break; \
		case ARF_INT32:RuleFloatMod(float, int32, float); break; \
		case ARF_UINT8:RuleFloatMod(float, uint8, float); break; \
		case ARF_UINT16:RuleFloatMod(float, uint16, float); break; \
		case ARF_UINT32:RuleFloatMod(float, uint32, float); break; \
		case ARF_INT64:RuleFloatMod(float, int64, float); break; \
		case ARF_UINT64:RuleFloatMod_LeftUInt64(float, uint64, float); break; \
		case ARF_FLOAT: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: RuleFloatMod(float, float, char); break; \
			case ARF_INT8: RuleFloatMod(float, float, int8); break; \
			case ARF_INT16:RuleFloatMod(float, float, int16); break; \
			case ARF_INT32:RuleFloatMod(float, float, int32); break; \
			case ARF_UINT8:RuleFloatMod(float, float, uint8); break; \
			case ARF_UINT16:RuleFloatMod(float, float, uint16); break; \
			case ARF_UINT32:RuleFloatMod(float, float, uint32); break; \
			case ARF_INT64:RuleFloatMod(float, float, int64); break; \
			case ARF_UINT64:RuleFloatMod_RightUInt64(float, float, uint64); break; \
			case ARF_FLOAT:RuleFloatMod(float, float, float); break; \
			} \
			break; \
		} \
		break; \
	case ARF_DOUBLE: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleFloatMod(double, char, double); break; \
		case ARF_INT8: RuleFloatMod(double, int8, double); break; \
		case ARF_INT16:RuleFloatMod(double, int16, double); break; \
		case ARF_INT32:RuleFloatMod(double, int32, double); break; \
		case ARF_UINT8:RuleFloatMod(double, uint8, double); break; \
		case ARF_UINT16:RuleFloatMod(double, uint16, double); break; \
		case ARF_UINT32:RuleFloatMod(double, uint32, double); break; \
		case ARF_INT64:RuleFloatMod(double, int64, double); break; \
		case ARF_UINT64:RuleFloatMod_LeftUInt64(double, uint64, double); break; \
		case ARF_FLOAT:RuleFloatMod(double, float, double); break; \
		case ARF_DOUBLE: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: RuleFloatMod(double, double, char); break; \
			case ARF_INT8: RuleFloatMod(double, double, int8); break; \
			case ARF_INT16:RuleFloatMod(double, double, int16); break; \
			case ARF_INT32:RuleFloatMod(double, double, int32); break; \
			case ARF_UINT8:RuleFloatMod(double, double, uint8); break; \
			case ARF_UINT16:RuleFloatMod(double, double, uint16); break; \
			case ARF_UINT32:RuleFloatMod(double, double, uint32); break; \
			case ARF_INT64:RuleFloatMod(double, double, int64); break; \
			case ARF_UINT64:RuleFloatMod_RightUInt64(double, double, uint64); break; \
			case ARF_FLOAT:RuleFloatMod(double, double, float); break; \
			case ARF_DOUBLE:RuleFloatMod(double, double, double); break; \
			} \
			break; \
		} \
		break; \
	}}while(0)

#else

#define RuleMathFloatMod() \
	do{switch(nRetType) \
	{ \
	case ARF_FLOAT: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleFloatMod(float, char, float); break; \
		case ARF_INT8: RuleFloatMod(float, int8, float); break; \
		case ARF_INT16:RuleFloatMod(float, int16, float); break; \
		case ARF_INT32:RuleFloatMod(float, int32, float); break; \
		case ARF_UINT8:RuleFloatMod(float, uint8, float); break; \
		case ARF_UINT16:RuleFloatMod(float, uint16, float); break; \
		case ARF_UINT32:RuleFloatMod(float, uint32, float); break; \
		case ARF_INT64:RuleFloatMod(float, int64, float); break; \
		case ARF_UINT64:RuleFloatMod(float, uint64, float); break; \
		case ARF_FLOAT: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: RuleFloatMod(float, float, char); break; \
			case ARF_INT8: RuleFloatMod(float, float, int8); break; \
			case ARF_INT16:RuleFloatMod(float, float, int16); break; \
			case ARF_INT32:RuleFloatMod(float, float, int32); break; \
			case ARF_UINT8:RuleFloatMod(float, float, uint8); break; \
			case ARF_UINT16:RuleFloatMod(float, float, uint16); break; \
			case ARF_UINT32:RuleFloatMod(float, float, uint32); break; \
			case ARF_INT64:RuleFloatMod(float, float, int64); break; \
			case ARF_UINT64:RuleFloatMod(float, float, uint64); break; \
			case ARF_FLOAT:RuleFloatMod(float, float, float); break; \
			} \
			break; \
		} \
		break; \
	case ARF_DOUBLE: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleFloatMod(double, char, double); break; \
		case ARF_INT8: RuleFloatMod(double, int8, double); break; \
		case ARF_INT16:RuleFloatMod(double, int16, double); break; \
		case ARF_INT32:RuleFloatMod(double, int32, double); break; \
		case ARF_UINT8:RuleFloatMod(double, uint8, double); break; \
		case ARF_UINT16:RuleFloatMod(double, uint16, double); break; \
		case ARF_UINT32:RuleFloatMod(double, uint32, double); break; \
		case ARF_INT64:RuleFloatMod(double, int64, double); break; \
		case ARF_UINT64:RuleFloatMod(double, uint64, double); break; \
		case ARF_FLOAT:RuleFloatMod(double, float, double); break; \
		case ARF_DOUBLE: \
			switch(nRightType) \
			{ \
			case ARF_CHAR: RuleFloatMod(double, double, char); break; \
			case ARF_INT8: RuleFloatMod(double, double, int8); break; \
			case ARF_INT16:RuleFloatMod(double, double, int16); break; \
			case ARF_INT32:RuleFloatMod(double, double, int32); break; \
			case ARF_UINT8:RuleFloatMod(double, double, uint8); break; \
			case ARF_UINT16:RuleFloatMod(double, double, uint16); break; \
			case ARF_UINT32:RuleFloatMod(double, double, uint32); break; \
			case ARF_INT64:RuleFloatMod(double, double, int64); break; \
			case ARF_UINT64:RuleFloatMod(double, double, uint64); break; \
			case ARF_FLOAT:RuleFloatMod(double, double, float); break; \
			case ARF_DOUBLE:RuleFloatMod(double, double, double); break; \
			} \
			break; \
		} \
		break; \
	}}while(0)

#endif

#define RuleShiftOpr(TRet, TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule4Tuple(TRet, TLeft, nOpr, char); break; \
	case ARF_INT8: Rule4Tuple(TRet, TLeft, nOpr, int8); break; \
	case ARF_INT16:Rule4Tuple(TRet, TLeft, nOpr, int16); break; \
	case ARF_INT32:Rule4Tuple(TRet, TLeft, nOpr, int32); break; \
	case ARF_INT64:Rule4Tuple(TRet, TLeft, nOpr, int64); break; \
	case ARF_UINT8:Rule4Tuple(TRet, TLeft, nOpr, uint8); break; \
	case ARF_UINT16:Rule4Tuple(TRet, TLeft, nOpr, uint16); break; \
	case ARF_UINT32:Rule4Tuple(TRet, TLeft, nOpr, uint32); break; \
	case ARF_UINT64:Rule4Tuple(TRet, TLeft, nOpr, uint64); break; \
	}}while(0)

#define RuleShiftOperator(nOpr) \
	do{switch(nRetType) \
	{ \
	case ARF_INT32: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: RuleShiftOpr(int32, char, nOpr); break; \
		case ARF_INT8: RuleShiftOpr(int32, int8, nOpr); break; \
		case ARF_INT16: RuleShiftOpr(int32, int16, nOpr); break; \
		case ARF_INT32: RuleShiftOpr(int32, int32, nOpr); break; \
		case ARF_UINT8: RuleShiftOpr(int32, uint8, nOpr); break; \
		case ARF_UINT16: RuleShiftOpr(int32, uint16, nOpr); break; \
		} \
		break; \
	case ARF_UINT32: RuleShiftOpr(uint32, uint32, nOpr); break; \
	case ARF_INT64: RuleShiftOpr(int64, int64, nOpr); break; \
	case ARF_UINT64:RuleShiftOpr(uint64, uint64, nOpr); break; \
	}}while(0)

#define RuleBitOperator(nOpr) \
	do{switch(nRetType) \
	{ \
	case ARF_INT32: \
		switch(nLeftType) \
		{ \
		case ARF_CHAR: Rule4Tuple(int32, char, nOpr, uint8); break; \
		case ARF_INT8: Rule4Tuple(int32, int8, nOpr, uint8); break; \
		case ARF_INT16: Rule4Tuple(int32, int16, nOpr, uint16); break; \
		case ARF_INT32: Rule4Tuple(int32, int32, nOpr, uint32); break; \
		case ARF_UINT8: Rule4Tuple(int32, uint8, nOpr, uint8); break; \
		case ARF_UINT16: Rule4Tuple(int32, uint16, nOpr, uint16); break; \
		} \
		break; \
	case ARF_UINT32: \
		switch(nLeftType) \
		{ \
		case ARF_INT32: Rule4Tuple(uint32, int32, nOpr, uint32); break; \
		case ARF_UINT32: Rule4Tuple(uint32, uint32, nOpr, uint32); break; \
		} \
		break; \
	case ARF_INT64: Rule4Tuple(int64, int64, nOpr, uint64); break; \
	case ARF_UINT64:Rule4Tuple(int64, uint64, nOpr, uint64); break; \
	}}while(0)

#define RuleNumCmpOperator3(TLeft, nOpr, TRight) \
	do{ \
		if(nLeftType > nRightType) \
		{ \
			TLeft x = (TLeft)*(TRight*)pRightValue; \
			void* pVal = pRightValue; \
			pRightValue = &x; \
			Rule4Tuple(bool, TLeft, nOpr, TLeft); \
			pRightValue = pVal; \
		} \
		else if(nLeftType < nRightType) \
		{ \
			TRight x = (TRight)*(TLeft*)pLeftValue; \
			void* pVal = pLeftValue; \
			pLeftValue = &x; \
			Rule4Tuple(bool, TRight, nOpr, TRight); \
			pLeftValue = pVal; \
		} \
		else Rule4Tuple(bool, TRight, nOpr, TRight); \
	}while(0)

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define RuleNumCmpOperator3_LeftUInt64(TLeft, nOpr, TRight) \
	do{ \
		double d = UInt64ToDouble(*(TLeft*)pLeftValue); \
		void* pVal = pLeftValue; \
		pLeftValue = &d; \
		Rule4Tuple(bool, double, nOpr, TRight); \
		pLeftValue = pVal; \
	}while(0)
#define RuleNumCmpOperator3_RightUInt64(TLeft, nOpr, TRight) \
	do{ \
		double d = UInt64ToDouble(*(TRight*)pRightValue); \
		void* pVal = pRightValue; \
		pRightValue = &d; \
		Rule4Tuple(bool, TLeft, nOpr, double); \
		pRightValue = pVal; \
	}while(0)
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define RuleNumCmpOperator2(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator3(TLeft, nOpr, char); break; \
	case ARF_INT8: RuleNumCmpOperator3(TLeft, nOpr, int8); break; \
	case ARF_UINT8: RuleNumCmpOperator3(TLeft, nOpr, uint8); break; \
	case ARF_INT16: RuleNumCmpOperator3(TLeft, nOpr, int16); break; \
	case ARF_UINT16: RuleNumCmpOperator3(TLeft, nOpr, uint16); break; \
	case ARF_INT32: RuleNumCmpOperator3(TLeft, nOpr, int32); break; \
	case ARF_UINT32: RuleNumCmpOperator3(TLeft, nOpr, uint32); break; \
	case ARF_INT64: RuleNumCmpOperator3(TLeft, nOpr, int64); break; \
	case ARF_UINT64: RuleNumCmpOperator3(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: RuleNumCmpOperator3(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: RuleNumCmpOperator3(TLeft, nOpr, double); break; \
	}}while(0)

#define RuleNumCmpOperator2_1(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator3(TLeft, nOpr, char); break; \
	case ARF_INT8: RuleNumCmpOperator3(TLeft, nOpr, int8); break; \
	case ARF_UINT8: RuleNumCmpOperator3(TLeft, nOpr, uint8); break; \
	case ARF_INT16: RuleNumCmpOperator3(TLeft, nOpr, int16); break; \
	case ARF_UINT16: RuleNumCmpOperator3(TLeft, nOpr, uint16); break; \
	case ARF_INT32: RuleNumCmpOperator3(TLeft, nOpr, int32); break; \
	case ARF_UINT32: RuleNumCmpOperator3(TLeft, nOpr, uint32); break; \
	case ARF_INT64: RuleNumCmpOperator3(TLeft, nOpr, int64); break; \
	case ARF_UINT64: RuleNumCmpOperator3(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: RuleNumCmpOperator3_LeftUInt64(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: RuleNumCmpOperator3_LeftUInt64(TLeft, nOpr, double); break; \
	}}while(0)

#define RuleNumCmpOperator2_2(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator3(TLeft, nOpr, char); break; \
	case ARF_INT8: RuleNumCmpOperator3(TLeft, nOpr, int8); break; \
	case ARF_UINT8: RuleNumCmpOperator3(TLeft, nOpr, uint8); break; \
	case ARF_INT16: RuleNumCmpOperator3(TLeft, nOpr, int16); break; \
	case ARF_UINT16: RuleNumCmpOperator3(TLeft, nOpr, uint16); break; \
	case ARF_INT32: RuleNumCmpOperator3(TLeft, nOpr, int32); break; \
	case ARF_UINT32: RuleNumCmpOperator3(TLeft, nOpr, uint32); break; \
	case ARF_INT64: RuleNumCmpOperator3(TLeft, nOpr, int64); break; \
	case ARF_UINT64: RuleNumCmpOperator3_RightUInt64(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: RuleNumCmpOperator3(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: RuleNumCmpOperator3(TLeft, nOpr, double); break; \
	}}while(0)

#define RuleNumCmpOperator(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator2(char, nOpr); break; \
	case ARF_INT8: RuleNumCmpOperator2(int8, nOpr); break; \
	case ARF_UINT8: RuleNumCmpOperator2(uint8, nOpr); break; \
	case ARF_INT16: RuleNumCmpOperator2(int16, nOpr); break; \
	case ARF_UINT16: RuleNumCmpOperator2(uint16, nOpr); break; \
	case ARF_INT32: RuleNumCmpOperator2(int32, nOpr); break; \
	case ARF_UINT32: RuleNumCmpOperator2(uint32, nOpr); break; \
	case ARF_INT64: RuleNumCmpOperator2(int64, nOpr); break; \
	case ARF_UINT64: RuleNumCmpOperator2_1(uint64, nOpr); break; \
	case ARF_FLOAT: RuleNumCmpOperator2_2(float, nOpr); break; \
	case ARF_DOUBLE: RuleNumCmpOperator2_2(double, nOpr); break; \
	}}while(0)
#else
#define RuleNumCmpOperator2(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator3(TLeft, nOpr, char); break; \
	case ARF_INT8: RuleNumCmpOperator3(TLeft, nOpr, int8); break; \
	case ARF_UINT8: RuleNumCmpOperator3(TLeft, nOpr, uint8); break; \
	case ARF_INT16: RuleNumCmpOperator3(TLeft, nOpr, int16); break; \
	case ARF_UINT16: RuleNumCmpOperator3(TLeft, nOpr, uint16); break; \
	case ARF_INT32: RuleNumCmpOperator3(TLeft, nOpr, int32); break; \
	case ARF_UINT32: RuleNumCmpOperator3(TLeft, nOpr, uint32); break; \
	case ARF_INT64: RuleNumCmpOperator3(TLeft, nOpr, int64); break; \
	case ARF_UINT64: RuleNumCmpOperator3(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: RuleNumCmpOperator3(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: RuleNumCmpOperator3(TLeft, nOpr, double); break; \
	}}while(0)

#define RuleNumCmpOperator(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_CHAR: RuleNumCmpOperator2(char, nOpr); break; \
	case ARF_INT8: RuleNumCmpOperator2(int8, nOpr); break; \
	case ARF_UINT8: RuleNumCmpOperator2(uint8, nOpr); break; \
	case ARF_INT16: RuleNumCmpOperator2(int16, nOpr); break; \
	case ARF_UINT16: RuleNumCmpOperator2(uint16, nOpr); break; \
	case ARF_INT32: RuleNumCmpOperator2(int32, nOpr); break; \
	case ARF_UINT32: RuleNumCmpOperator2(uint32, nOpr); break; \
	case ARF_INT64: RuleNumCmpOperator2(int64, nOpr); break; \
	case ARF_UINT64: RuleNumCmpOperator2(uint64, nOpr); break; \
	case ARF_FLOAT: RuleNumCmpOperator2(float, nOpr); break; \
	case ARF_DOUBLE: RuleNumCmpOperator2(double, nOpr); break; \
	}}while(0)
#endif

#define RuleBoolCmpOperator(nOpr) Rule4Tuple(bool, bool, nOpr, bool)

#define RuleStringCmpOperator(bLeftConst, nOpr, bRightConst) \
	do{if(bLeftConst) \
	{ \
		CString oLeft((char*)pLeftValue); \
		if(bRightConst) \
		{ \
			int32 nRet = oLeft.Compare((const char*)pRightValue); \
			(*(bool*)pRetValue) = (nRet nOpr 0); \
		} \
		else \
		{ \
			int32 nRet = oLeft.Compare(((CString*)pRightValue)->GetStr()); \
			(*(bool*)pRetValue) = (nRet nOpr 0); \
		} \
	} \
	else if(bRightConst) \
	{ \
		CString &oLeft = (*(CString*)pLeftValue); \
		int32 nRet = oLeft.Compare((const char*)pRightValue); \
		(*(bool*)pRetValue) = (nRet nOpr 0); \
	} \
	else \
	{ \
		CString &oLeft = (*(CString*)pLeftValue); \
		int32 nRet = oLeft.Compare(((CString*)pRightValue)->GetStr()); \
		(*(bool*)pRetValue) = (nRet nOpr 0); \
	}}while(0)

#define RuleNumAssignOperator2(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule3Tuple(TLeft, nOpr, char); break; \
	case ARF_INT8: Rule3Tuple(TLeft, nOpr, int8); break; \
	case ARF_INT16: Rule3Tuple(TLeft, nOpr, int16); break; \
	case ARF_INT32: Rule3Tuple(TLeft, nOpr, int32); break; \
	case ARF_INT64: Rule3Tuple(TLeft, nOpr, int64); break; \
	case ARF_UINT8: Rule3Tuple(TLeft, nOpr, uint8); break; \
	case ARF_UINT16: Rule3Tuple(TLeft, nOpr, uint16); break; \
	case ARF_UINT32: Rule3Tuple(TLeft, nOpr, uint32); break; \
	case ARF_UINT64: Rule3Tuple(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: Rule3Tuple(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: Rule3Tuple(TLeft, nOpr, double); break; \
	}}while(0)

#if defined(_MSC_VER) && (_MSC_VER < 1300)

#define RuleNumAssignOperator3(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule3Tuple(TLeft, nOpr, char); break; \
	case ARF_INT8: Rule3Tuple(TLeft, nOpr, int8); break; \
	case ARF_INT16: Rule3Tuple(TLeft, nOpr, int16); break; \
	case ARF_INT32: Rule3Tuple(TLeft, nOpr, int32); break; \
	case ARF_INT64: Rule3Tuple(TLeft, nOpr, int64); break; \
	case ARF_UINT8: Rule3Tuple(TLeft, nOpr, uint8); break; \
	case ARF_UINT16: Rule3Tuple(TLeft, nOpr, uint16); break; \
	case ARF_UINT32: Rule3Tuple(TLeft, nOpr, uint32); break; \
	case ARF_UINT64: Rule3Tuple_RightUInt64(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: Rule3Tuple(TLeft, nOpr, float); break; \
	case ARF_DOUBLE: Rule3Tuple(TLeft, nOpr, double); break; \
	}}while(0)
#define RuleNumAssignOperator6(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule3Tuple(TLeft, nOpr, char); break; \
	case ARF_INT8: Rule3Tuple(TLeft, nOpr, int8); break; \
	case ARF_INT16: Rule3Tuple(TLeft, nOpr, int16); break; \
	case ARF_INT32: Rule3Tuple(TLeft, nOpr, int32); break; \
	case ARF_INT64: Rule3Tuple(TLeft, nOpr, int64); break; \
	case ARF_UINT8: Rule3Tuple(TLeft, nOpr, uint8); break; \
	case ARF_UINT16: Rule3Tuple(TLeft, nOpr, uint16); break; \
	case ARF_UINT32: Rule3Tuple(TLeft, nOpr, uint32); break; \
	case ARF_UINT64: Rule3Tuple(TLeft, nOpr, uint64); break; \
	case ARF_FLOAT: \
		{ \
			double x = UInt64ToDouble(*(TLeft*)pLeftValue); \
			void* pLeft = pLeftValue; \
			pLeftValue = &x; \
			Rule3Tuple(double, nOpr, float); \
			pLeftValue = pLeft; \
			*(TLeft*)pLeftValue = x; \
			break; \
		} \
	case ARF_DOUBLE: \
		{ \
			double x = UInt64ToDouble(*(TLeft*)pLeftValue); \
			void* pLeft = pLeftValue; \
			pLeftValue = &x; \
			Rule3Tuple(double, nOpr, double); \
			pLeftValue = pLeft; \
			*(TLeft*)pLeftValue = x; \
			break; \
		} \
	}}while(0)
#define RuleNumAssignOperator4(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_CHAR: RuleNumAssignOperator2(char, nOpr); break; \
	case ARF_INT8: RuleNumAssignOperator2(int8, nOpr); break; \
	case ARF_INT16: RuleNumAssignOperator2(int16, nOpr); break; \
	case ARF_INT32: RuleNumAssignOperator2(int32, nOpr); break; \
	case ARF_INT64: RuleNumAssignOperator2(int64, nOpr); break; \
	case ARF_UINT8: RuleNumAssignOperator2(uint8, nOpr); break; \
	case ARF_UINT16: RuleNumAssignOperator2(uint16, nOpr); break; \
	case ARF_UINT32: RuleNumAssignOperator2(uint32, nOpr); break; \
	case ARF_UINT64: RuleNumAssignOperator6(uint64, nOpr); break; \
	}}while(0)
#define RuleNumAssignOperator5(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_FLOAT: RuleNumAssignOperator3(float, nOpr); break; \
	case ARF_DOUBLE: RuleNumAssignOperator3(double, nOpr); break; \
	}}while(0)
#define RuleNumAssignOperator(nOpr) \
	do{ \
		RuleNumAssignOperator4(nOpr); \
		RuleNumAssignOperator5(nOpr); \
	}while(0)
#else
#define RuleNumAssignOperator(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_CHAR: RuleNumAssignOperator2(char, nOpr); break; \
	case ARF_INT8: RuleNumAssignOperator2(int8, nOpr); break; \
	case ARF_INT16: RuleNumAssignOperator2(int16, nOpr); break; \
	case ARF_INT32: RuleNumAssignOperator2(int32, nOpr); break; \
	case ARF_INT64: RuleNumAssignOperator2(int64, nOpr); break; \
	case ARF_UINT8: RuleNumAssignOperator2(uint8, nOpr); break; \
	case ARF_UINT16: RuleNumAssignOperator2(uint16, nOpr); break; \
	case ARF_UINT32: RuleNumAssignOperator2(uint32, nOpr); break; \
	case ARF_UINT64: RuleNumAssignOperator2(uint64, nOpr); break; \
	case ARF_FLOAT: RuleNumAssignOperator2(float, nOpr); break; \
	case ARF_DOUBLE: RuleNumAssignOperator2(double, nOpr); break; \
	}}while(0)
#endif

#define RuleBoolAssignOperator2(TRight) ((*(bool*)pLeftValue) = ((*(TRight*)pRightValue)?true:false))
#define RuleBoolAssignOperator() \
	do{if(nLeftType == ARF_BOOL)switch(nRightType) \
	{ \
	case ARF_CHAR: RuleBoolAssignOperator2(char); \
	case ARF_INT8: RuleBoolAssignOperator2(int8); break; \
	case ARF_INT16: RuleBoolAssignOperator2(int16); break; \
	case ARF_INT32: RuleBoolAssignOperator2(int32); break; \
	case ARF_INT64: RuleBoolAssignOperator2(int64); break; \
	case ARF_UINT8: RuleBoolAssignOperator2(uint8); break; \
	case ARF_UINT16: RuleBoolAssignOperator2(uint16); break; \
	case ARF_UINT32: RuleBoolAssignOperator2(uint32); break; \
	case ARF_UINT64: RuleBoolAssignOperator2(uint64); break; \
	case ARF_FLOAT: RuleBoolAssignOperator2(float); break; \
	case ARF_DOUBLE: RuleBoolAssignOperator2(double); break; \
	case ARF_BOOL: Rule3Tuple(bool, =, bool); break; \
	}}while(0)

#define RuleIntAssignOperator2(TLeft, nOpr) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: Rule3Tuple(TLeft, nOpr, char); break; \
	case ARF_INT8: Rule3Tuple(TLeft, nOpr, int8); break; \
	case ARF_INT16: Rule3Tuple(TLeft, nOpr, int16); break; \
	case ARF_INT32: Rule3Tuple(TLeft, nOpr, int32); break; \
	case ARF_INT64: Rule3Tuple(TLeft, nOpr, int64); break; \
	case ARF_UINT8: Rule3Tuple(TLeft, nOpr, uint8); break; \
	case ARF_UINT16: Rule3Tuple(TLeft, nOpr, uint16); break; \
	case ARF_UINT32: Rule3Tuple(TLeft, nOpr, uint32); break; \
	case ARF_UINT64: Rule3Tuple(TLeft, nOpr, uint64); break; \
	}}while(0)

#define RuleIntAssignOperator(nOpr) \
	do{switch(nLeftType) \
	{ \
	case ARF_CHAR: RuleIntAssignOperator2(char, nOpr); break; \
	case ARF_INT8: RuleIntAssignOperator2(int8, nOpr); break; \
	case ARF_INT16: RuleIntAssignOperator2(int16, nOpr); break; \
	case ARF_INT32: RuleIntAssignOperator2(int32, nOpr); break; \
	case ARF_INT64: RuleIntAssignOperator2(int64, nOpr); break; \
	case ARF_UINT8: RuleIntAssignOperator2(uint8, nOpr); break; \
	case ARF_UINT16: RuleIntAssignOperator2(uint16, nOpr); break; \
	case ARF_UINT32: RuleIntAssignOperator2(uint32, nOpr); break; \
	case ARF_UINT64: RuleIntAssignOperator2(uint64, nOpr); break; \
	}}while(0)

#if defined(_MSC_VER) && (_MSC_VER < 1300)

#define RuleFloatAssignMod2(TLeft) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleFloatMod2(TLeft, char); break; \
	case ARF_INT8: RuleFloatMod2(TLeft, int8); break; \
	case ARF_INT16: RuleFloatMod2(TLeft, int16); break; \
	case ARF_INT32: RuleFloatMod2(TLeft, int32); break; \
	case ARF_INT64: RuleFloatMod2(TLeft, int64); break; \
	case ARF_UINT8: RuleFloatMod2(TLeft, uint8); break; \
	case ARF_UINT16: RuleFloatMod2(TLeft, uint16); break; \
	case ARF_UINT32: RuleFloatMod2(TLeft, uint32); break; \
	case ARF_UINT64: RuleFloatMod2_RightUInt64(TLeft, uint64); break; \
	case ARF_FLOAT: RuleFloatMod2(TLeft, float); break; \
	case ARF_DOUBLE: RuleFloatMod2(TLeft, double); break; \
	}}while(0)

#else

#define RuleFloatAssignMod2(TLeft) \
	do{switch(nRightType) \
	{ \
	case ARF_CHAR: RuleFloatMod2(TLeft, char); break; \
	case ARF_INT8: RuleFloatMod2(TLeft, int8); break; \
	case ARF_INT16: RuleFloatMod2(TLeft, int16); break; \
	case ARF_INT32: RuleFloatMod2(TLeft, int32); break; \
	case ARF_INT64: RuleFloatMod2(TLeft, int64); break; \
	case ARF_UINT8: RuleFloatMod2(TLeft, uint8); break; \
	case ARF_UINT16: RuleFloatMod2(TLeft, uint16); break; \
	case ARF_UINT32: RuleFloatMod2(TLeft, uint32); break; \
	case ARF_UINT64: RuleFloatMod2(TLeft, uint64); break; \
	case ARF_FLOAT: RuleFloatMod2(TLeft, float); break; \
	case ARF_DOUBLE: RuleFloatMod2(TLeft, double); break; \
	}}while(0)

#endif

#define RuleFloatAssignMod() \
	do{switch(nLeftType) \
	{ \
	case ARF_FLOAT: RuleFloatAssignMod2(float); break; \
	case ARF_DOUBLE: RuleFloatAssignMod2(double); break; \
	}}while(0)

#define RuleStringAssignOperator(nOpr, bRightConst) \
	do{if(bRightConst) \
	{ \
		CString &oLeft = (*(CString*)pLeftValue); \
		CString oRight((char*)pRightValue); \
		oLeft nOpr oRight; \
	} \
	else \
	{ \
		CString &oLeft = (*(CString*)pLeftValue); \
		CString &oRight = (*(CString*)pRightValue); \
		oLeft nOpr oRight; \
	}}while(0)

bool CRuleExpress::GetCond(uint32 nCondType, void* pCond)
{
	bool bRet = false;
	switch(nCondType)
	{
	case ARF_STRUCT:
		bRet = (*(void**)pCond)?true:false;
		break;
	case ARF_BOOL:
		bRet = (*(bool*)pCond)?true:false;
		break;
	case ARF_CHAR:
		bRet = (*(char*)pCond)?true:false;
		break;
	case ARF_INT8:
		bRet = (*(int8*)pCond)?true:false;
		break;
	case ARF_UINT8:
		bRet = (*(uint8*)pCond)?true:false;
		break;
	case ARF_INT16:
		bRet = (*(int16*)pCond)?true:false;
		break;
	case ARF_UINT16:
		bRet = (*(uint16*)pCond)?true:false;
		break;
	case ARF_INT32:
		bRet = (*(int32*)pCond)?true:false;
		break;
	case ARF_UINT32:
		bRet = (*(uint32*)pCond)?true:false;
		break;
	case ARF_INT64:
		bRet = (*(int64*)pCond)?true:false;
		break;
	case ARF_UINT64:
		bRet = (*(uint64*)pCond)?true:false;
		break;
	case ARF_FLOAT:
		bRet = (*(float*)pCond)?true:false;
		break;
	case ARF_DOUBLE:
		bRet = (*(double*)pCond)?true:false;
		break;
	}
	return bRet;
}

void* CRuleBinaryExpress::GetExpressValue(CRuleStack* pStack) const
{
	CRuleType* pLeftType = m_pLeft->GetExpressType(pStack->pSystem);
	CRuleType* pRightType = m_pRight->GetExpressType(pStack->pSystem);
	CRuleType* pRetType = NULL;
	uint32 nLeftType = pLeftType->TypeCode();
	uint32 nRightType = pRightType->TypeCode();
	uint32 nRetType = 0;

	void* pLeftValue = m_pLeft->GetExpressValue(pStack);
	void* pRightValue = m_pRight->GetExpressValue(pStack);
	void* pRetValue = NULL;
	if(m_pRet)
	{
		pRetValue = m_pRet->GetExpressValue(pStack);
		pRetType = m_pRight->GetExpressType(pStack->pSystem);
		nRetType = pRetType->TypeCode();
	}
	switch(m_nOperator)
	{
	case ARF_ADD_OP:// n = n1 + n2
		RuleMathIntOperator(+);
		RuleMathFloatOperator(+);
		break;
	case ARF_SUB_OP:// n = n1 - n2
		RuleMathIntOperator(-);
		RuleMathFloatOperator(-);
		break;
	case ARF_MUL_OP:// n = n1 * n2
		RuleMathIntOperator(*);
		RuleMathFloatOperator(*);
		break;
	case ARF_DIV_OP:// n = n1 / n2
		RuleMathIntOperator(/);
		RuleMathFloatOperator(/);
		break;
	case ARF_MOD_OP:// n = n1 % n2
		RuleMathIntOperator(%);
		RuleMathFloatMod();
		break;
	case ARF_LSH_OP:// i = i1 << i2
		RuleShiftOperator(<<);
		break;
	case ARF_RSH_OP:// i = i1 >> i2
		RuleShiftOperator(>>);
		break;
	case ARF_AND_OP:// i = i1 & i2
		RuleBitOperator(&);
		break;
	case ARF_OR_OP:// i = i1 | i2
		RuleBitOperator(|);
		break;
	case ARF_XOR_OP:// i = i1 ^ i2
		RuleBitOperator(^);
		break;
	case ARF_LAND_OP:// b = (b1 or n1) && (b2 or n2)
		{
			bool bLeft = CRuleExpress::GetCond(nLeftType, pLeftValue);
			bool bRight = CRuleExpress::GetCond(nRightType, pRightValue);
			(*(bool*)pRetValue) = (bLeft && bRight);
		}
		break;
	case ARF_LOR_OP:// b = (b1 or n1) || (b2 or n2)
		{
			bool bLeft = CRuleExpress::GetCond(nLeftType, pLeftValue);
			bool bRight = CRuleExpress::GetCond(nRightType, pRightValue);
			(*(bool*)pRetValue) = (bLeft || bRight);
		}
		break;
	case ARF_LT_OP:// b = (n1 or s1) < (n2 or s2)
		if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), <, m_pRight->IsLiteral());
		else
			RuleNumCmpOperator(<);
		break;
	case ARF_GT_OP:// b = (n1 or s1) > (n2 or s2)
		if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), >, m_pRight->IsLiteral());
		else
			RuleNumCmpOperator(>);
		break;
	case ARF_LE_OP:// b = (n1 or s1) <= (n2 or s2)
		if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), <=, m_pRight->IsLiteral());
		else
			RuleNumCmpOperator(<=);
		break;
	case ARF_GE_OP:// b = (n1 or s1) >= (n2 or s2)
		if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), >=, m_pRight->IsLiteral());
		else
			RuleNumCmpOperator(>=);
		break;
	case ARF_EQ_OP:// b = (n1 or s1 or b) == (n2 or s2 or b)
		if(nLeftType == ARF_STRUCT)
			(*(bool*)pRetValue) = (*(void**)pLeftValue == *(void**)pRightValue);
		else if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), ==, m_pRight->IsLiteral());
		else if(nLeftType == ARF_BOOL)
			RuleBoolCmpOperator(==);
		else
			RuleNumCmpOperator(==);
		break;
	case ARF_NE_OP:// b = (n1 or s1 or b) != (n2 or s2 or b)
		if(nLeftType == ARF_STRUCT)
			(*(bool*)pRetValue) = (*(void**)pLeftValue != *(void**)pRightValue);
		else if(nLeftType == ARF_STRING)
			RuleStringCmpOperator(m_pLeft->IsLiteral(), !=, m_pRight->IsLiteral());
		else if(nLeftType == ARF_BOOL)
			RuleBoolCmpOperator(!=);
		else
			RuleNumCmpOperator(!=);
		break;
	case ARF_ASIGN_OP:// a& = a1
		if(nLeftType == ARF_STRUCT)
		{
			uint32* &pLeftCounter = *(uint32**)pLeftValue;
			uint32* &pRightCounter = *(uint32**)pRightValue;
			if(pLeftCounter != pRightCounter)
			{
				if(pLeftCounter)
				{
					--pLeftCounter[0];
					if(!pLeftCounter[0])
						pLeftType->ClearObject(pLeftCounter);
				}
				pLeftCounter = pRightCounter;
				if(pRightCounter)
					pRightCounter[0]++;
			}
		}
		else if(nLeftType == ARF_OBJECT)
			pLeftType->AssignObject(pLeftValue, pRightValue);
		else if(nLeftType == ARF_STRING)
			RuleStringAssignOperator(=, m_pRight->IsLiteral());
		else if(nLeftType == ARF_BOOL)
			RuleBoolAssignOperator();
		else
			RuleNumAssignOperator(=);
		break;
	case ARF_ADDEQ_OP:// n& += n1
		RuleNumAssignOperator(+=);
		break;
	case ARF_SUBEQ_OP:// n& -= n1
		RuleNumAssignOperator(-=);
		break;
	case ARF_MULEQ_OP:// n& *= n1
		RuleNumAssignOperator(*=);
		break;
	case ARF_DIVEQ_OP:// n& /= n1
		RuleNumAssignOperator(/=);
		break;
	case ARF_MODEQ_OP:// n& %= n1
		RuleIntAssignOperator(%=);
		RuleFloatAssignMod();
		break;
	case ARF_ANDEQ_OP:// i& &= i1
		RuleIntAssignOperator(&=);
		break;
	case ARF_OREQ_OP:// i& |= i1
		RuleIntAssignOperator(|=);
		break;
	case ARF_XOREQ_OP:// i& ^= i1
		RuleIntAssignOperator(^=);
		break;
	case ARF_COMMA_OP:
		break;
	}
	if(m_pRet)
		return pRetValue;
	if(m_nOperator == ARF_COMMA_OP)
		return pRightValue;
	return pLeftValue;
}

bool CRuleBinaryExpress::IsVariable() const
{
	if(m_pRet)
		return false;
	if(m_nOperator == ARF_COMMA_OP)
		return m_pRight->IsVariable();
	return true;
}

CRuleExpress* CRuleBinaryExpress::Clone() const
{
	CRuleBinaryExpress* pExp = new CRuleBinaryExpress(*m_pLeft, m_nOperator, *m_pRight);
	if(m_pRet)
		pExp->m_pRet = (CRuleVariableExpress*)m_pRet->Clone();
	pExp->SetFile(*this);
	return pExp;
}

bool CRuleBinaryExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pRight->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pLeft->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pLeftType = m_pLeft->GetExpressType(&oSystem);
	CRuleType* pRightType = m_pRight->GetExpressType(&oSystem);
	uint32 nLeftType = pLeftType->TypeCode();
	uint32 nRightType = pRightType->TypeCode();
	CRuleType* pRetType = NULL;
	switch(m_nOperator)
	{
	default:
		bRet = false;
		pChecker->OnError(*this, "invalid binary operator '%u'", m_nOperator);
		break;

	case ARF_ADD_OP:// n = n1 + n2
	case ARF_SUB_OP:// n = n1 - n2
	case ARF_MUL_OP:// n = n1 * n2
	case ARF_DIV_OP:// n = n1 / n2
	case ARF_MOD_OP:// n = n1 % n2
		if(!IsNumber(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't number type data");
			break;
		}
		if(!IsNumber(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't number type data");
			break;
		}
		if(nLeftType < ARF_INT32)
			nLeftType = ARF_INT32;
		if(nRightType < ARF_INT32)
			nRightType = ARF_INT32;
		if(nLeftType > nRightType)
			pRetType = pLeftType;
		else
			pRetType = pRightType;
		break;

	case ARF_LSH_OP:// i = i1 << i2
	case ARF_RSH_OP:// i = i1 >> i2
		if(!IsInteger(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't integer type data");
			break;
		}
		if(!IsInteger(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't integer type data");
			break;
		}
		if(nLeftType < ARF_INT32)
			pRetType = oSystem.GetType("int");
		else
			pRetType = pLeftType;
		break;

	case ARF_AND_OP:// i = i1 & i2
	case ARF_OR_OP:// i = i1 | i2
	case ARF_XOR_OP:// i = i1 ^ i2
		if(!IsInteger(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't integer type data");
			break;
		}
		if(!IsInteger(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't integer type data");
			break;
		}
		if(pLeftType->GetSize() != pRightType->GetSize())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't consistent with the righ operand");
			break;
		}
		if(nLeftType < ARF_INT32)
			nLeftType = ARF_INT32;
		if(nRightType < ARF_INT32)
			nRightType = ARF_INT32;
		if(nLeftType > nRightType)
			pRetType = pLeftType;
		else
			pRetType = pRightType;
		break;

	case ARF_LAND_OP:// b = (b1 or n1) && (b2 or n2)
	case ARF_LOR_OP:// b = (b1 or n1) || (b2 or n2)
		if(!IsNumber(nLeftType) && nLeftType != ARF_BOOL && nLeftType != ARF_STRUCT)
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the number,bool,struct type data");
			break;
		}
		if(!IsNumber(nRightType) && nRightType != ARF_BOOL && nRightType != ARF_STRUCT)
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand must be the number,bool,struct type data");
			break;
		}
		pRetType = oSystem.GetType("bool");
		break;

	case ARF_LT_OP:// b = (n1 or s1) < (n2 or s2)
	case ARF_GT_OP:// b = (n1 or s1) > (n2 or s2)
	case ARF_LE_OP:// b = (n1 or s1) <= (n2 or s2)
	case ARF_GE_OP:// b = (n1 or s1) >= (n2 or s2)
		if(nLeftType == ARF_STRING)
		{
			if(nRightType != ARF_STRING)
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the string type data");
				break;
			}
		}
		else if(IsNumber(nLeftType))
		{
			if(!IsNumber(nRightType))
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the number type data");
				break;
			}
		}
		else
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the number or string type data");
			break;
		}
		pRetType = oSystem.GetType("bool");
		break;
	case ARF_EQ_OP:// b = (n1 or s1 or b) == (n2 or s2 or b)
	case ARF_NE_OP:// b = (n1 or s1 or b) != (n2 or s2 or b)
		if(nLeftType == ARF_STRING)
		{
			if(nRightType != ARF_STRING)
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the string type data");
				break;
			}
		}
		else if(IsNumber(nLeftType))
		{
			if(!IsNumber(nRightType))
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the number type data");
				break;
			}
		}
		else if(nLeftType == ARF_BOOL)
		{
			if(nRightType != ARF_BOOL)
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the bool type data");
				break;
			}
		}
		else if(nLeftType == ARF_STRUCT)
		{
			if(pLeftType != pRightType)
			{
				bRet = false;
				pChecker->OnError(*this, "the righ operand must be the same struct type data with the left operand");
				break;
			}
		}
		else
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the number or string type data");
			break;
		}
		pRetType = oSystem.GetType("bool");
		break;

	case ARF_ASIGN_OP:// a& = a1
		if(!m_pLeft->IsVariable())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the variable type data");
			break;
		}
		if(IsNumber(nRightType))
		{
			if(!IsNumber(nLeftType) && nLeftType != ARF_BOOL)
			{
				bRet = false;
				pChecker->OnError(*this, "'%s' can't be cast into '%s'", pRightType->GetName(), pLeftType->GetName());
				break;
			}
		}
		else if(nRightType==ARF_BOOL)
		{
			if(nLeftType != ARF_BOOL)
			{
				bRet = false;
				pChecker->OnError(*this, "'%s' can't be cast into '%s'", pRightType->GetName(), pLeftType->GetName());
				break;
			}
		}
		else if(CString::StringCompare(pLeftType->GetName(), pRightType->GetName(), false))
		{
			bRet = false;
			pChecker->OnError(*this, "'%s' can't be cast into '%s'", pRightType->GetName(), pLeftType->GetName());
			break;
		}
		break;

	case ARF_ADDEQ_OP:// n& += n1
	case ARF_SUBEQ_OP:// n& -= n1
	case ARF_MULEQ_OP:// n& *= n1
	case ARF_DIVEQ_OP:// n& /= n1
	case ARF_MODEQ_OP:// n& %= n1
		if(!m_pLeft->IsVariable())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the variable type data");
			break;
		}
		if(!IsNumber(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't number type data");
			break;
		}
		if(!IsNumber(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't number type data");
			break;
		}
		break;

	case ARF_LSHEQ_OP:// i& <<= i1
	case ARF_RSHEQ_OP:// i& >>= i1
		if(!m_pLeft->IsVariable())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the variable type data");
			break;
		}
		if(!IsInteger(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't integer type data");
			break;
		}
		if(!IsInteger(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't integer type data");
			break;
		}
		break;

	case ARF_ANDEQ_OP:// i& &= i1
	case ARF_OREQ_OP:// i& |= i1
	case ARF_XOREQ_OP:// i& ^= i1
		if(!m_pLeft->IsVariable())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand must be the variable type data");
			break;
		}
		if(!IsInteger(nLeftType))
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't integer type data");
			break;
		}
		if(!IsInteger(nRightType))
		{
			bRet = false;
			pChecker->OnError(*this, "the righ operand isn't integer type data");
			break;
		}
		if(pLeftType->GetSize() != pRightType->GetSize())
		{
			bRet = false;
			pChecker->OnError(*this, "the left operand isn't consistent with the righ operand");
			break;
		}
		break;
	case ARF_COMMA_OP:// a1,b2
		break;
	}

	if(bRet == false)
		return false;
	CRuleStruct* pVariables = oSystem.GetCheckVariables();
	if(pRetType)
	{
		CRuleTmpVar* pVar = pVariables->AllocVar(pRetType);
		const char* sName = pVar->GetName();
		m_pRet = new CRuleVariableExpress(sName);
		m_pRet->Check(oSystem, pChecker);
	}
	return true;
}

// ---------------------------------------------------
// CRuleVectorExpress
// ---------------------------------------------------
CRuleVectorExpress::CRuleVectorExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp)
{
	m_pVec = oVecExp.Clone();
	m_pIdx = oIdxExp.Clone();
}

CRuleVectorExpress::~CRuleVectorExpress()
{
	if(m_pVec)
	{
		delete m_pVec;
		m_pVec = NULL;
	}
	if(m_pIdx)
	{
		delete m_pIdx;
		m_pIdx = NULL;
	}
}

CRuleType* CRuleVectorExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	CRuleVector* pType = (CRuleVector*)m_pVec->GetExpressType(pSystem);
	return pType->GetBaseType();
}

void* CRuleVectorExpress::GetExpressValue(CRuleStack* pStack)const
{
	CRuleVectorData* pVec = (CRuleVectorData*)m_pVec->GetExpressValue(pStack);
	uint32 nIdx = *(uint32*)m_pIdx->GetExpressValue(pStack);
	if(nIdx >= pVec->nCount)
		return NULL;
	CRuleType* pType = GetExpressType(pStack->pSystem);
	uint32 nTypeCode = pType->TypeCode();
	uint32 nSize = pType->GetSize();
	if(nTypeCode == ARF_STRUCT)
		nSize = sizeof(void*);
	return pVec->pData+nSize*nIdx;
}

bool CRuleVectorExpress::IsVariable() const
{
	return true;
}

CRuleExpress* CRuleVectorExpress::Clone() const
{
	CRuleExpress* pRet = new CRuleVectorExpress(*m_pVec, *m_pIdx);
	pRet->SetFile(*this);
	return pRet;
}

bool CRuleVectorExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pVec->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pIdx->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pType = m_pVec->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_VECTOR)
	{
		pChecker->OnError(*m_pVec, "isn't vector type");
		return false;
	}
	pType = m_pIdx->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_UINT32)
	{
		pChecker->OnError(*m_pIdx, "isn't uint type");
		return false;
	}
	return true;
}

// ---------------------------------------------------
// CRuleVectorInsertExpress
// ---------------------------------------------------
CRuleVectorInsertExpress::CRuleVectorInsertExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp, const CRuleExpress& oValExp)
{
	m_pVec = oVecExp.Clone();
	m_pIdx = oIdxExp.Clone();
	m_pVal = oValExp.Clone();
}

CRuleVectorInsertExpress::~CRuleVectorInsertExpress()
{
	if(m_pVec)
	{
		delete m_pVec;
		m_pVec = NULL;
	}
	if(m_pIdx)
	{
		delete m_pIdx;
		m_pIdx = NULL;
	}
	if(m_pVal)
	{
		delete m_pVal;
		m_pVal = NULL;
	}
}

CRuleType* CRuleVectorInsertExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pVec->GetExpressType(pSystem);
}

void* CRuleVectorInsertExpress::GetExpressValue(CRuleStack* pStack) const
{
	CRuleVector* pType = (CRuleVector*)m_pVec->GetExpressType(pStack->pSystem);
	CRuleVectorData* pVec = (CRuleVectorData*)m_pVec->GetExpressValue(pStack);
	uint32 nIdx = *(uint32*)m_pIdx->GetExpressValue(pStack);
	if(nIdx >= pVec->nCount)
		nIdx = pVec->nCount;
	void* pVal = m_pVal->GetExpressValue(pStack);
	uint32 nNewSize = pVec->nCount+1;
	if(nNewSize)
	{
		CRuleType* pBase = pType->GetBaseType();
		uint32 nBaseType = pBase->TypeCode();
		uint32 nSize = pBase->GetSize();
		if(nBaseType == ARF_STRUCT)
			nSize = sizeof(void*);
		pType->SetVectorSize(pVec, nNewSize);
		uint32 nEnd = pVec->nCount-1;
		uint8* pMem = pVec->pData + nEnd*nSize;
		for(; nEnd > nIdx; --nEnd)
		{
			if(nBaseType == ARF_STRUCT)
				*(void**)pMem = *(void**)(pMem-nSize);
			else
				pBase->AssignObject(pMem, pMem-nSize);
			pMem -= nSize;
		}
		if(nBaseType == ARF_STRUCT)
		{
			*(void**)pMem = *(void**)pVal;
			uint32* pCounter = *(uint32**)pMem;
			pCounter[0]++;
		}
		else
			pBase->AssignObject(pMem, pVal);
	}
	return pVec;
}

bool CRuleVectorInsertExpress::IsVariable() const
{
	return m_pVec->IsVariable();
}

CRuleExpress* CRuleVectorInsertExpress::Clone() const
{
	CRuleExpress* pRet = new CRuleVectorInsertExpress(*m_pVec, *m_pIdx, *m_pVal);
	pRet->SetFile(*this);
	return pRet;
}

bool CRuleVectorInsertExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pVec->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pIdx->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pVal->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pType = m_pVec->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_VECTOR)
	{
		pChecker->OnError(*m_pVec, "isn't vector type");
		return false;
	}
	CRuleType* pBase = ((CRuleVector*)pType)->GetBaseType();
	pType = m_pIdx->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_UINT32)
	{
		pChecker->OnError(*m_pIdx, "isn't uint type");
		return false;
	}
	pType = m_pVal->GetExpressType(&oSystem);
	if(pType != pBase)
	{
		pChecker->OnError(*m_pVal, "invalid type '%s' for the vector<%s>", pType->GetName(), pBase->GetName());
		return false;
	}
	return true;
}

// ---------------------------------------------------
// CRuleVectorRemoveExpress
// ---------------------------------------------------
CRuleVectorRemoveExpress::CRuleVectorRemoveExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp)
{
	m_pVec = oVecExp.Clone();
	m_pIdx = oIdxExp.Clone();
}

CRuleVectorRemoveExpress::~CRuleVectorRemoveExpress()
{
	if(m_pVec)
	{
		delete m_pVec;
		m_pVec = NULL;
	}
	if(m_pIdx)
	{
		delete m_pIdx;
		m_pIdx = NULL;
	}
}

CRuleType* CRuleVectorRemoveExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pVec->GetExpressType(pSystem);
}

void* CRuleVectorRemoveExpress::GetExpressValue(CRuleStack* pStack) const
{
	CRuleVector* pType = (CRuleVector*)m_pVec->GetExpressType(pStack->pSystem);
	CRuleVectorData* pVec = (CRuleVectorData*)m_pVec->GetExpressValue(pStack);
	uint32 nIdx = *(uint32*)m_pIdx->GetExpressValue(pStack);
	if(nIdx < pVec->nCount)
	{
		uint32 nNewSize = pVec->nCount-1;
		uint8* pData = NULL;
		if(nNewSize)
		{
			CRuleType* pBase = pType->GetBaseType();
			uint32 nBaseType = pBase->TypeCode();
			uint32 i, nSize = pBase->GetSize();
			if(nBaseType == ARF_STRUCT)
				nSize = sizeof(void*);
			pData = new uint8[nNewSize*nSize];
			uint8* pMem = pData;
			if(nBaseType > ARF_DOUBLE)for(i=0; i<nNewSize; ++i)
			{
				if(nBaseType == ARF_STRUCT)
					*(uint32**)pMem = NULL;
				else
					pBase->InitObject(pMem);
				pMem += nSize;
			}
			pMem = pData;
			uint8* pRight = pVec->pData;
			for(i=0; i<nNewSize; ++i)
			{
				if(nBaseType != ARF_STRUCT)
					pBase->AssignObject(pMem, pRight);
				else
				{
					uint32* pCounter = *(uint32**)pRight;
					if(pCounter)
					{
						*(uint32**)pMem = pCounter;
						pCounter[0]++;
					}
				}
				pMem += nSize;
				pRight += nSize;
				if(i == nIdx)
					pRight += nSize;
			}
		}
		pType->ClearObject(pVec);
		pVec->nCount = nNewSize;
		pVec->pData = pData;
	}
	return pVec;
}

bool CRuleVectorRemoveExpress::IsVariable() const
{
	return m_pVec->IsVariable();
}

CRuleExpress* CRuleVectorRemoveExpress::Clone() const
{
	CRuleExpress* pRet = new CRuleVectorRemoveExpress(*m_pVec, *m_pIdx);
	pRet->SetFile(*this);
	return pRet;
}

bool CRuleVectorRemoveExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pVec->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pIdx->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pType = m_pVec->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_VECTOR)
	{
		pChecker->OnError(*m_pVec, "isn't vector type");
		return false;
	}
	pType = m_pIdx->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_UINT32)
	{
		pChecker->OnError(*m_pIdx, "isn't uint type");
		return false;
	}
	return true;
}

// ---------------------------------------------------
// CRuleVectorSetSizeExpress
// ---------------------------------------------------
CRuleVectorSetSizeExpress::CRuleVectorSetSizeExpress(const CRuleExpress& oVecExp, const CRuleExpress& oSizeExp)
{
	m_pVec = oVecExp.Clone();
	m_pSize = oSizeExp.Clone();
}

CRuleVectorSetSizeExpress::~CRuleVectorSetSizeExpress()
{
	if(m_pVec)
	{
		delete m_pVec;
		m_pVec = NULL;
	}
	if(m_pSize)
	{
		delete m_pSize;
		m_pSize = NULL;
	}
}

CRuleType* CRuleVectorSetSizeExpress::GetExpressType(const CRuleSystem* pSystem) const
{
	return m_pVec->GetExpressType(pSystem);
}

void* CRuleVectorSetSizeExpress::GetExpressValue(CRuleStack* pStack) const
{
	CRuleVector* pType = (CRuleVector*)m_pVec->GetExpressType(pStack->pSystem);
	CRuleVectorData* pVec = (CRuleVectorData*)m_pVec->GetExpressValue(pStack);
	uint32 nSize = *(uint32*)m_pSize->GetExpressValue(pStack);
	pType->SetVectorSize(pVec, nSize);
	return pVec;
}

bool CRuleVectorSetSizeExpress::IsVariable() const
{
	return m_pVec->IsVariable();
}

CRuleExpress* CRuleVectorSetSizeExpress::Clone() const
{
	CRuleExpress* pRet = new CRuleVectorSetSizeExpress(*m_pVec, *m_pSize);
	pRet->SetFile(*this);
	return pRet;
}

bool CRuleVectorSetSizeExpress::Check(CRuleSystem &oSystem, CRuleChecker* pChecker)
{
	bool bRet = m_pVec->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	bRet = m_pSize->Check(oSystem, pChecker);
	if(bRet == false)
		return false;
	CRuleType* pType = m_pVec->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_VECTOR)
	{
		pChecker->OnError(*m_pVec, "isn't vector type");
		return false;
	}
	pType = m_pSize->GetExpressType(&oSystem);
	if(pType->TypeCode() != ARF_UINT32)
	{
		pChecker->OnError(*m_pSize, "isn't uint type");
		return false;
	}
	return true;
}

FOCP_END();
