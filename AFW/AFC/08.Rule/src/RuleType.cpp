
#include "RuleSystem.hpp"

FOCP_BEGIN();

// ---------------------------------------------------
// CRuleType
// ---------------------------------------------------
CRuleType::CRuleType(CRuleSystem *pRuleSystem, const char* sName):
	m_oName(sName)
{
	if(pRuleSystem)
		pRuleSystem->RegType(this);
}

CRuleType::~CRuleType()
{
}

uint32 CRuleType::GetSize() const
{
	return 0;
}

uint32 CRuleType::TypeCode() const
{
	return ARF_OBJECT;
}

void CRuleType::InitObject(void* pData) const
{
}

void CRuleType::ClearObject(void* pData) const
{
}

void CRuleType::AssignObject(void* pLeft, void* pRight) const
{
}

bool CRuleType::Check(CRuleChecker* pChecker)
{
	return true;
}

void CRuleType::DumpLevel(CString & oDump, uint32 nLevel)
{
	for(uint32 i=0; i<nLevel; ++i)
		oDump += "    ";
}

void CRuleType::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(TypeCode() == ARF_OBJECT)
		oDump += "object ";
	oDump += m_oName;
	oDump += ";\n";
}

const char* CRuleType::GetName() const
{
	return m_oName.GetStr();
}

RULE_API CRuleType*& GetRuleArgType(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(CRuleType*);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	CRuleType* &pRet = *(CRuleType**)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return pRet;
}

// ---------------------------------------------------
// CRuleVariable
// ---------------------------------------------------
CRuleVariable::CRuleVariable(CRuleType* pType, const char* sVarName):
	m_oName(sVarName), m_pType(pType), m_nOffset(0)
{
}

CRuleVariable::~CRuleVariable()
{
	if(m_pType->TypeCode() == ARF_VECTOR)
	{
		delete m_pType;
		m_pType = NULL;
	}
}

CRuleType* CRuleVariable::GetType() const
{
	return m_pType;
}

uint32 CRuleVariable::GetOffset() const
{
	return m_nOffset;
}

uint32 CRuleVariable::GetSize() const
{
	uint32 nSize = m_pType->GetSize();
	switch(m_pType->TypeCode())
	{
	case ARF_STRING:
	case ARF_STRUCT:
	case ARF_OBJECT:
		nSize = sizeof(void*);
		break;
	}
	return nSize;
}

uint32 CRuleVariable::GetAlign() const
{
	uint32 nAlign = GetSize();
	if(m_pType->TypeCode() == ARF_VECTOR)
		nAlign = sizeof(void*);
	return nAlign;
}

const char* CRuleVariable::GetName() const
{
	return m_oName.GetStr();
}

void CRuleVariable::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += m_pType->GetName();
	oDump += " ";
	oDump += m_oName;
}

// ---------------------------------------------------
// CRuleVariable
// ---------------------------------------------------
CRuleTmpVar::CRuleTmpVar(CRuleType* pType, const char* sVarName): CRuleVariable(pType, sVarName)
{
}

CRuleTmpVar::CRuleTmpVar(CRuleType* pType, uint32 nId): CRuleVariable(pType, NULL)
{
	m_oName.Clear();
	CStringFormatter oFmt(&m_oName);
	oFmt.Print("$%u", nId);
}

CRuleTmpVar::~CRuleTmpVar()
{
}

// ---------------------------------------------------
// CRuleParameter
// ---------------------------------------------------
CRuleParameter::CRuleParameter(CRuleType* pType, const char* sVarName, bool bOut):
	CRuleVariable(pType, sVarName), m_bOut(bOut)
{
}

CRuleParameter::~CRuleParameter()
{
}

bool CRuleParameter::IsOut() const
{
	return m_bOut;
}

uint32 CRuleParameter::GetSize() const
{
	if(m_bOut)
		return sizeof(void*);
	return CRuleVariable::GetSize();
}

uint32 CRuleParameter::GetAlign() const
{
	if(m_bOut)
		return sizeof(void*);
	return CRuleVariable::GetAlign();
}

void CRuleParameter::Dump(CString & oDump, uint32 nLevel) const
{
	if(!m_bOut)
		CRuleVariable::Dump(oDump, nLevel);
	else
	{
		oDump += m_pType->GetName();
		oDump += " &";
		oDump += m_oName;
	}
}

// ---------------------------------------------------
// CRuleStruct
// ---------------------------------------------------
CRuleStruct::CRuleStruct(CRuleSystem *pRuleSystem, const char* sName, bool bUnion):
	CRuleType(pRuleSystem, sName), m_nSize(0), m_bImplemented(false), m_bUnion(bUnion)
{
}

CRuleStruct::~CRuleStruct()
{
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		delete pVar;
	}
	m_oFields.Clear();
}

bool CRuleStruct::IsUnion() const
{
	return m_bUnion;
}

uint32 CRuleStruct::GetFieldCount() const
{
	return m_oFields.GetSize();
}

CRuleVariable* CRuleStruct::GetField(uint32 nIdx) const
{
	if(nIdx < m_oFields.GetSize())
		return m_oFields[nIdx];
	return NULL;
}

uint32 CRuleStruct::GetFieldNo(const char* sName) const
{
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		if(!pVar->m_oName.Compare(sName, false))
			return i;
	}
	return 0xFFFFFFFF;
}

CRuleVariable* CRuleStruct::FindField(const char* sName) const
{
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		if(!pVar->m_oName.Compare(sName, false))
			return pVar;
	}
	return NULL;
}

bool CRuleStruct::AddField(CRuleVariable* pField)
{
	if(FindField(pField->m_oName.GetStr()))
		return false;
	m_oFields.Insert((uint32)(-1), pField);
	m_bImplemented = false;
	return true;
}

bool CRuleStruct::Implemented() const
{
	return m_bImplemented;
}

void CRuleStruct::FinishDefine()
{
	m_bImplemented = true;
	m_nSize = sizeof(uint32);//counter
	uint32 i, nMod, nSize = m_oFields.GetSize();
	uint32 nMaxAlign = sizeof(uint32);

	for(i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		uint32 nFieldSize = pVar->GetSize();
		uint32 nFieldAlign = pVar->GetAlign();
		if(m_bUnion)
		{
			nMod = sizeof(uint32) % nFieldAlign;
			pVar->m_nOffset = 2*sizeof(uint32);//counter+idx
			pVar->m_nOffset += (nMod?(nFieldAlign - nMod):0);
			if(nFieldAlign > nMaxAlign)
				nMaxAlign = nFieldAlign;
			nFieldSize += pVar->m_nOffset;
			if(nFieldSize > m_nSize)
				m_nSize = nFieldSize;
		}
		else
		{
			nMod = m_nSize % nFieldAlign;
			if(nMod)
				m_nSize += nFieldAlign - nMod;
			pVar->m_nOffset = m_nSize;
			m_nSize += nFieldSize;
			if(nFieldAlign > nMaxAlign)
				nMaxAlign = nFieldAlign;
		}
	}

	if(m_nSize)
	{
		nMod = m_nSize % nMaxAlign;
		if(nMod)
			m_nSize += nMaxAlign - nMod;
	}
}

uint32 CRuleStruct::GetSize() const
{
	if(!m_bImplemented)
		return 0;
	return m_nSize;
}

uint32 CRuleStruct::TypeCode() const
{
	return ARF_STRUCT;
}

void CRuleStruct::InitObject(void* pData) const
{
	uint32 nSize = GetSize();
	if(nSize)
	{
		CBinary::MemorySet(pData, 0, nSize);
		*(uint32*)pData = 1; //初始计数器为1
		if(m_bUnion)
			*((uint32*)pData + 1) = 0xFFFFFFFF;//字段号初始为无效字段
	}
}

void CRuleStruct::ClearObject(void* pData) const
{
	void** pData2;
	uint32 i, nSize = m_oFields.GetSize();
	if(nSize == 0)
		return;
	if(m_bUnion)
	{
		i = *((uint32*)pData+1);
		if(i < nSize)
		{
			CRuleVariable* pVar = m_oFields[i];
			CRuleType* pType = pVar->GetType();
			CRuleParameter* pPara = dynamic_cast<CRuleParameter*>(pVar);			
			if(pPara == NULL || !pPara->IsOut()) switch(pType->TypeCode())
			{
			case ARF_STRUCT:
				pData2 = (void**)((char*)pData+pVar->GetOffset());
				if(pData2[0])
				{
					uint32 *pCounter = (uint32*)pData2[0];
					--pCounter[0];
					if(pCounter[0] == 0)
					{
						pType->ClearObject(pCounter);
						delete[] (char*)pCounter;
					}
					pData2[0] = NULL;
				}
				break;
			case ARF_STRING:
			case ARF_OBJECT:
				pData2 = (void**)((char*)pData+pVar->GetOffset());
				if(pData2[0])
				{
					pType->ClearObject(pData2[0]);
					delete[] (char*)(pData2[0]);
					pData2[0] = NULL;
				}
				break;
			}
			else
			{
				pData2 = (void**)((char*)pData+pVar->GetOffset());
				pData2[0] = NULL;
			}
		}
		*((uint32*)pData+1) = 0xFFFFFFFF;
	}
	else for(i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		CRuleType* pType = pVar->GetType();
		CRuleParameter* pPara = dynamic_cast<CRuleParameter*>(pVar);
		if(pPara == NULL || !pPara->IsOut()) switch(pType->TypeCode())
		{
		case ARF_STRUCT:
			pData2 = (void**)((char*)pData+pVar->GetOffset());
			if(pData2[0])
			{
				uint32 *pCounter = (uint32*)pData2[0];
				--pCounter[0];
				if(pCounter[0] == 0)
				{
					pType->ClearObject(pCounter);
					delete[] (char*)pCounter;
				}
				pData2[0] = NULL;
			}
			break;
		case ARF_STRING:
		case ARF_OBJECT:
			pData2 = (void**)((char*)pData+pVar->GetOffset());
			if(pData2[0])
			{
				pType->ClearObject(pData2[0]);
				delete[] (char*)(pData2[0]);
				pData2[0] = NULL;
			}
			break;
		}
		else
		{
			pData2 = (void**)((char*)pData+pVar->GetOffset());
			pData2[0] = NULL;
		}
	}
}

void CRuleStruct::AssignObject(void* pLeft, void* pRight) const
{
	void* pOld;
	uint32 i, j, nSize = m_oFields.GetSize();
	if(nSize == 0)
		return;
	if(m_bUnion)
	{
		i = *((uint32*)pLeft+1);
		j = *((uint32*)pRight+1);
		if((i != j) && (i<nSize))
		{
			ClearObject(pLeft);
			i = 0xFFFFFFFF;
		}
		if(j < nSize)
		{
			CRuleVariable* pVar = m_oFields[j];
			CRuleType* pType = pVar->GetType();
			uint32 nOffset = pVar->GetOffset();
			void* pLeft1 = (char*)pLeft + nOffset;
			void* pRight1 = (char*)pRight + nOffset;
			switch(pType->TypeCode())
			{
			case ARF_STRUCT:
				pOld = pLeft1;
				pLeft1 = *(void**)pLeft1;
				pRight1 = *(void**)pRight1;
				if(i == j && pLeft1 != pRight1)
				{
					ClearObject(pLeft);
					pLeft1 = NULL;
				}
				if(pLeft1 != pRight1)
				{
					*(void**)pOld = pRight1;
					((uint32*)pRight1)[0]++;
				}
				break;
			case ARF_STRING:
			case ARF_OBJECT:
				pOld = pLeft1;
				pLeft1 = *(void**)pLeft1;
				pRight1 = *(void**)pRight1;
				if(i != j)//i==0xFFFFFFFF
				{
					pLeft1 = new char[pType->GetSize()];
					pType->InitObject(pLeft1);
					*(void**)pOld = pLeft1;
				}
				pType->AssignObject(pLeft1, pRight1);
				break;
			default:
				pType->AssignObject(pLeft1, pRight1);
				break;
			}
		}
		*((uint32*)pLeft+1) = j;
	}
	else for(i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		CRuleType* pType = pVar->GetType();
		uint32 nOffset = pVar->GetOffset();
		void* pLeft1 = (char*)pLeft + nOffset;
		void* pRight1 = (char*)pRight + nOffset;
		switch(pType->TypeCode())
		{
		case ARF_STRUCT:
			pOld = pLeft1;
			pLeft1 = *(void**)pLeft1;
			pRight1 = *(void**)pRight1;
			if(pLeft1 != pRight1)
			{
				if(pLeft1)
				{
					uint32* pCounter = (uint32*)pLeft1;
					--pCounter[0];
					if(!pCounter[0])
					{
						pType->ClearObject(pLeft1);
						delete[] (char*)pLeft1;
					}
				}
				*(void**)pOld = pRight1;
				if(pRight1)
					((uint32*)pRight1)[0]++;
			}
			break;
		case ARF_STRING:
		case ARF_OBJECT:
			pOld = pLeft1;
			pLeft1 = *(void**)pLeft1;
			pRight1 = *(void**)pRight1;
			if(pRight1)
			{
				if(!pLeft1)
				{
					pLeft1 = new char[pType->GetSize()];
					pType->InitObject(pLeft1);
					*(void**)pOld = pLeft1;
				}
				pType->AssignObject(pLeft1, pRight1);
			}
			else if(pLeft1)
			{
				pType->ClearObject(pLeft1);
				delete[] (char*)pLeft1;
				*(void**)pOld = NULL;
			}
			break;
		default:
			pType->AssignObject(pLeft1, pRight1);
			break;
		}
	}
}

bool CRuleStruct::Check(CRuleChecker* pChecker)
{
	if(!m_bImplemented)
	{
		pChecker->OnError(*this, "undefined struct '%s'", m_oName.GetStr());
		return false;
	}
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		CRuleType* pType = pVar->GetType();
		if(!pType->Check(pChecker))
			return false;
	}
	return true;
}

void CRuleStruct::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(GetName()[0])
	{
		if(m_bUnion)
			oDump += "union ";
		else
			oDump += "struct ";
		oDump += GetName();
		oDump += "{\n";
	}
	bool bDone = false;
	uint32 nSize = m_oFields.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		if(pVar->m_oName.GetStr()[0] == '$')
			continue;
		if(bDone)
			oDump += ";\n";
		bDone = true;
		pVar->Dump(oDump, nLevel+1);
	}
	if(GetName()[0])
	{
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "};\n";
	}
}

void CRuleStruct::Dump(CString & oDump, bool bForRule) const
{
	if(bForRule)
		oDump+= "<";
	else
		oDump+= "(";
	uint32 nSize = m_oFields.GetSize();
	bool bDone = false;
	for(uint32 i=0; i<nSize; ++i)
	{
		CRuleVariable* pVar = m_oFields[i];
		if(pVar->m_oName.GetStr()[0] == '$')
			continue;
		if(bDone)
			oDump += ", ";
		bDone = true;
		pVar->Dump(oDump, 0);
	}
	if(bForRule)
		oDump+= ">";
	else
		oDump+= ")";
}

CRuleTmpVar* CRuleStruct::AllocVar(CRuleType* pType)
{
	uint32 nSize = m_oFields.GetSize();
	CRuleTmpVar* pVar = new CRuleTmpVar(pType, nSize);
	AddField(pVar);
	return pVar;
}

CRuleVector::CRuleVector(CRuleSystem *pRuleSystem, const char* sName):
	CRuleType(NULL, sName)
{
	m_pBaseType = pRuleSystem->GetType(sName);
	m_oName = "vector<";
	m_oName += sName;
	m_oName += ">";
}

CRuleVector::~CRuleVector()
{
}

uint32 CRuleVector::GetSize() const
{
	return sizeof(CRuleVectorData);
}

uint32 CRuleVector::TypeCode() const
{
	return ARF_VECTOR;
}

CRuleType* CRuleVector::GetBaseType() const
{
	return m_pBaseType;
}

void CRuleVector::InitObject(void* pData) const
{
	CRuleVectorData* pVec = (CRuleVectorData*)pData;
	pVec->pData = NULL;
	pVec->nCount = 0;
}

void CRuleVector::ClearObject(void* pData) const
{
	CRuleVectorData* pVec = (CRuleVectorData*)pData;
	if(pVec->pData)
	{
		uint32 nBaseType = m_pBaseType->TypeCode();
		if(nBaseType > ARF_DOUBLE)
		{
			uint8* pMem = pVec->pData;
			uint32 nSize = m_pBaseType->GetSize();
			if(nBaseType == ARF_STRUCT)
				nSize = sizeof(void*);
			for(uintptr i=0; i<pVec->nCount; ++i)
			{
				if(nBaseType != ARF_STRUCT)
					m_pBaseType->ClearObject(pMem);
				else
				{
					uint32 * pCounter = *(uint32**)pMem;
					if(pCounter)
					{
						--pCounter[0];
						if(!pCounter[0])
							m_pBaseType->ClearObject(pCounter);
					}
				}
				pMem += nSize;
			}
		}
		delete[] pVec->pData;
		pVec->pData = NULL;
		pVec->nCount = 0;
	}
}

void CRuleVector::AssignObject(void* pLeft, void* pRight) const
{
	CRuleVectorData* pLeftVec = (CRuleVectorData*)pLeft;
	CRuleVectorData* pRightVec = (CRuleVectorData*)pRight;
	if(pLeftVec->nCount != pRightVec->nCount)
	{
		ClearObject(pLeft);
		if(pRightVec->nCount)
		{
			uint32 nBaseType = m_pBaseType->TypeCode();
			uint32 nSize = m_pBaseType->GetSize();
			if(nBaseType == ARF_STRUCT)
				nSize = sizeof(void*);
			pLeftVec->nCount = pRightVec->nCount;
			pLeftVec->pData = new uint8[nSize*pLeftVec->nCount];
			if(nBaseType > ARF_DOUBLE)
			{
				uint8* pMem = pLeftVec->pData;
				for(uintptr i=0; i<pLeftVec->nCount; ++i)
				{
					if(nBaseType == ARF_STRUCT)
						*(uint32**)pMem = NULL;
					else
						m_pBaseType->InitObject(pMem);
					pMem += nSize;
				}
			}
		}
	}
	if(pLeftVec->nCount)
	{
		uint32 nBaseType = m_pBaseType->TypeCode();
		uint32 nSize = m_pBaseType->GetSize();
		if(nBaseType == ARF_STRUCT)
			nSize = sizeof(void*);
		uint8* pLeftMem = pLeftVec->pData;
		uint8* pRightMem = pRightVec->pData;
		for(uintptr i=0; i<pLeftVec->nCount; ++i)
		{
			if(nBaseType != ARF_STRUCT)
				m_pBaseType->AssignObject(pLeftMem, pRightMem);
			else
			{
				uint32* pCounter1 = *(uint32**)pLeftMem;
				uint32* pCounter2 = *(uint32**)pRightMem;
				if(pCounter1 != pCounter2)
				{
					if(pCounter1)
					{
						--pCounter1[0];
						if(!pCounter1[0])
							m_pBaseType->ClearObject(pCounter1);
					}
					*(uint32**)pLeftMem = pCounter2;
					if(pCounter2)
						pCounter2[0]++;
				}
			}
			pLeftMem += nSize;
			pRightMem += nSize;
		}
	}
}

uint32 CRuleVector::GetVectorSize(void* pData) const
{
	CRuleVectorData* pVec = (CRuleVectorData*)pData;
	return pVec->nCount;
}

void CRuleVector::SetVectorSize(void* pData, uint32 nNewSize) const
{
	CRuleVectorData* pVec = (CRuleVectorData*)pData;
	if(nNewSize != pVec->nCount)
	{
		uint8* pData2 = NULL;
		if(nNewSize)
		{
			uint32 nBaseType = m_pBaseType->TypeCode();
			uint32 i, nSize = m_pBaseType->GetSize();
			if(nBaseType == ARF_STRUCT)
				nSize = sizeof(void*);
			uint8* pLeftMem = pData2;
			pData2 = new uint8[nSize*nNewSize];
			for(i=0; i<nNewSize; ++i)
			{
				if(nBaseType == ARF_STRUCT)
					*(uint32**)pLeftMem = NULL;
				else
					m_pBaseType->InitObject(pLeftMem);
				pLeftMem += nSize;
			}
			uint32 nCount = nNewSize;
			if(nCount > pVec->nCount)
				nCount = pVec->nCount;
			pLeftMem = pData2;
			uint8* pRightMem = pVec->pData;
			for(i=0; i<nCount; ++i)
			{
				if(nBaseType != ARF_STRUCT)
					m_pBaseType->AssignObject(pLeftMem, pRightMem);
				else
				{
					uint32* pCounter = *(uint32**)pRightMem;
					if(pCounter)
					{
						*(uint32**)pLeftMem = pCounter;
						pCounter[0]++;
					}
				}
				pLeftMem += nSize;
				pRightMem += nSize;
			}
		}
		ClearObject(pData);
		pVec->nCount = nNewSize;
		pVec->pData = pData2;
	}
}

FOCP_END();
