
#include "RdbPara.hpp"
#include "RdbSpace.hpp"

FOCP_BEGIN();

//-----------------------------------------------------------------------------
// CRdbField
//----------------------------------------------------------------------------
CRdbField::CRdbField()
{
	m_pData = NULL;
}

CRdbField::~CRdbField()
{
}

void CRdbField::Initialize(CRdbRecord* pRecord, CFieldDefine* pFieldDef, uint32 nRecHeadSize)
{
	m_pTableSpace = pRecord->m_pTableSpace;
	m_pRecord = pRecord;
	m_pFlag = (uint32*)pRecord->m_pRecordData;
	m_pData = (char*)m_pFlag + nRecHeadSize + pFieldDef->m_nDiskOffset;
	m_nBit = RDB_FLG_B(pFieldDef->m_nFieldNo);
	m_sLogName = pFieldDef->GetLogName();
	m_pBaseAttr = pFieldDef->m_pBaseAttr;
}

const char* CRdbField::GetLogName()
{
	return m_sLogName;
}

bool CRdbField::IsNull()
{
	return ((*m_pFlag) & m_nBit)?false:false;
}

void CRdbField::GetField(CField* pFieldValue)
{
	char* s;
	register uint32 nStrLen;
	CVarFieldStruct *pVar;

	pFieldValue->SetNull();

	if((*m_pFlag) & m_nBit)switch(pFieldValue->m_nType)
	{
	case RDB_UINT8_FIELD:
	case RDB_INT8_FIELD:
		GetAtomX(int8, pFieldValue) = *(int8*)m_pData;
		FillFieldA(pFieldValue);
		break;
	case RDB_UINT16_FIELD:
	case RDB_INT16_FIELD:
		GetAtomX(int16, pFieldValue) = *(int16*)m_pData;
		FillFieldA(pFieldValue);
		break;
	case RDB_FLOAT_FIELD:
	case RDB_UINT32_FIELD:
	case RDB_INT32_FIELD:
		GetAtomX(int32, pFieldValue) = *(int32*)m_pData;
		FillFieldA(pFieldValue);
		break;
	case RDB_DOUBLE_FIELD:
	case RDB_UINT64_FIELD:
	case RDB_INT64_FIELD:
		GetAtomX(int64, pFieldValue) = *(int64*)m_pData;
		FillFieldA(pFieldValue);
		break;
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		pFieldValue->SetString(m_pData);
		break;
	case RDB_RAW_FIELD:
		pFieldValue->SetRaw(m_pData+4, *(uint32*)m_pData);
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD: case RDB_VARRAW_FIELD:
		pVar = (CVarFieldStruct*)m_pData;
		nStrLen = pVar->nSize;
		s = new char[nStrLen+1];
		s[nStrLen] = 0;
		m_pTableSpace->ReadVarField(*pVar, m_pBaseAttr->nRecSize, s);
		if(pFieldValue->m_nType == RDB_VARRAW_FIELD)
			pFieldValue->SetRaw(s, nStrLen);
		else
			pFieldValue->SetString(s);
		delete[] s;
		break;
	}
}

void CRdbField::SetNull()
{
	if((*m_pFlag) & m_nBit)
	{
		(*m_pFlag) &= ~m_nBit;
		switch(m_pBaseAttr->nType)
		{
		case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD: case RDB_VARRAW_FIELD:
			m_pTableSpace->RemoveVarField(*(CVarFieldStruct*)m_pData, m_pBaseAttr->nRecSize);
			break;
		}
		m_pRecord->m_nDirty = 1;
	}
}

uint32 CRdbField::SetField(CField* pFieldValue)
{
	register uint32 nStrLen;
	register char* s;
	CVarFieldStruct* pVar;

	SetNull();

	if(!IS_NULL_A(pFieldValue))
	{
		(*m_pFlag) |= m_nBit;
		switch(pFieldValue->m_nType)
		{
		case RDB_UINT8_FIELD:
		case RDB_INT8_FIELD:
			*(int8*)m_pData = GetAtomX(int8, pFieldValue);
			break;
		case RDB_UINT16_FIELD:
		case RDB_INT16_FIELD:
			*(int16*)m_pData = GetAtomX(int16, pFieldValue);
			break;	
		case RDB_FLOAT_FIELD:
		case RDB_UINT32_FIELD:
		case RDB_INT32_FIELD:
			*(int32*)m_pData = GetAtomX(int32, pFieldValue);
			break;
		case RDB_DOUBLE_FIELD:
		case RDB_UINT64_FIELD:
		case RDB_INT64_FIELD:
			*(int64*)m_pData = GetAtomX(int64, pFieldValue);
			break;
		case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
			CBinary::MemoryCopy(m_pData, pFieldValue->GetString(NULL), m_pBaseAttr->nLen);
			break;
		case RDB_RAW_FIELD:
			s = (char*)pFieldValue->GetRaw(nStrLen);
			*(uint32*)m_pData = nStrLen;
			CBinary::MemoryCopy(m_pData+4, s, nStrLen);
			break;
		default:
			pVar = (CVarFieldStruct*)m_pData;
			switch(pFieldValue->m_nType)
			{
			case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
				s = (char*)pFieldValue->GetString(&nStrLen);
				break;
			case RDB_VARRAW_FIELD:
				s = (char*)pFieldValue->GetRaw(nStrLen);
				break;
			}
			if(!m_pTableSpace->WriteVarField(*pVar, m_pBaseAttr->nRecSize, s, nStrLen))
				return RDB_LACK_STORAGE;
			break;
		}
		m_pRecord->m_nDirty = 1;
	}

	return RDB_SUCCESS;
}

uint32 CRdbField::SetField(char* sValue)
{
	register uint32 nRet = RDB_SUCCESS;
	if(!sValue || !sValue[0])
		SetNull();
	else
	{
		uint32 nType = m_pBaseAttr->nType;
		uint32 nSize = CTableDefine::ComputeFieldRecSize(m_pBaseAttr, 1);
		char* pRecordData = new char[nSize+8];
		*(uint64*)pRecordData = 0;
		CField oField;
		oField.Initialize(nType, nSize, pRecordData, 0, 8, NULL);
		oField.SetFromString(sValue);
		nRet = SetField(&oField);
		oField.SetNull();
		delete[] pRecordData;
	}
	return nRet;
}

void CRdbField::CopyField(CRdbField* pField, uint32 nFieldSize)
{
	if((*pField->m_pFlag) & pField->m_nBit)
	{
		*m_pFlag |= m_nBit;
		CBinary::MemoryCopy(m_pData, pField->m_pData, nFieldSize);
		m_pRecord->m_nDirty = 1;
	}
}

//-----------------------------------------------------------------------------
// CRdbRecord
//----------------------------------------------------------------------------
CRdbRecord::CRdbRecord(CTableSpace* pTableSpace, CTableDefine * pTabDef, CKernelIndexItem* pKernelItem)
{
	m_pTabDef = pTabDef;
	m_pTableSpace = pTableSpace;
	m_pKernelItem = pKernelItem;
	m_nRecHeadSize = m_pTabDef->GetFileRecordHeadSize(m_pKernelItem->nJobId);
	if(IS_MEMORY_ADDR(pKernelItem->nRecord))
		m_pRecordData = GET_MEMORY_OBJECT(char, pKernelItem->nRecord);
	else
		m_pRecordData = (char*)vmap(pKernelItem->nRecord);
	m_nFieldCount = m_pTabDef->GetFileFieldCount(m_pKernelItem->nJobId);
	m_pFieldTable = NULL;//new CRdbField[m_nFieldCount];
	m_nDirty = 0;
}

CRdbRecord::~CRdbRecord()
{
	if(m_pFieldTable)
		delete[] m_pFieldTable;
	m_pFieldTable = NULL;
	if(!IS_MEMORY_ADDR(m_pKernelItem->nRecord))
	{
		if(m_nDirty)
			vflush(m_pRecordData);
		unvmap(m_pRecordData);
	}
	m_pRecordData = NULL;
	m_nDirty = 0;
}

uint32 CRdbRecord::CopyRecord(CRecord& oRecord, CSqlParameter* pInsertAttr)
{
	m_nDirty = 1;
	register uint32 i, nFlagCount = pInsertAttr->m_nFlgCount;
	register uint32* pSrcFlag = (uint32*)oRecord.m_pRecordData;
	register uint32* pDstFlag = (uint32*)m_pRecordData;
	for(i=0; i<nFlagCount; ++i)
		pDstFlag[i] = pSrcFlag[i];
	register uint32 nFieldCount = pInsertAttr->m_nParaCount;
	register CSqlParameter::CParaItem* pParaTable = pInsertAttr->m_pParaTable;
	for(i=0; i<nFieldCount; ++i)
	{
		register uint32 nStrLen;
		register char* s;
		register CVarFieldStruct* pVar;
		register uint32 nFieldNo = pParaTable[i].nFieldNo;
		register CField* pFieldValue = oRecord.m_pFieldTable+nFieldNo;
		register CFieldDefine* pFieldDefine = (CFieldDefine*)m_pTabDef->m_pFieldDefineSet[nFieldNo];
		register void* m_pData = m_pRecordData + m_nRecHeadSize + pFieldDefine->m_nDiskOffset;
		switch(pFieldValue->m_nType)
		{
		case RDB_UINT8_FIELD:
		case RDB_INT8_FIELD:
			GetAtomX0(int8) = GetAtomX(int8, pFieldValue);
			break;
		case RDB_UINT16_FIELD:
		case RDB_INT16_FIELD:
			GetAtomX0(int16) = GetAtomX(int16, pFieldValue);
			break;	
		case RDB_FLOAT_FIELD:
		case RDB_UINT32_FIELD:
		case RDB_INT32_FIELD:
			GetAtomX0(int32) = GetAtomX(int32, pFieldValue);
			break;
		case RDB_DOUBLE_FIELD:
		case RDB_UINT64_FIELD:
		case RDB_INT64_FIELD:
			GetAtomX0(int64) = GetAtomX(int64, pFieldValue);
			break;
		case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
			CBinary::MemoryCopy(m_pData, pFieldValue->GetString(NULL), pFieldDefine->m_pBaseAttr->nLen);
			break;
		case RDB_RAW_FIELD:
			s = (char*)pFieldValue->GetRaw(nStrLen);
			*(uint32*)m_pData = nStrLen;
			CBinary::MemoryCopy((char*)m_pData+4, s, nStrLen);
			break;
		default:
			pVar = (CVarFieldStruct*)m_pData;
			switch(pFieldValue->m_nType)
			{
			case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD:
				s = (char*)pFieldValue->GetString(&nStrLen);
				break;
			case RDB_VARRAW_FIELD:
				s = (char*)pFieldValue->GetRaw(nStrLen);
				break;
			}
			if(!m_pTableSpace->WriteVarField(*pVar, pFieldDefine->m_pBaseAttr->nRecSize, s, nStrLen))
				return i;
			break;
		}
	}
	return i;
}

const char* CRdbRecord::GetLogName()
{
	return m_pTabDef->GetLogName();
}

CRdbField* CRdbRecord::GetField(uint32 nFieldNo)
{
	if(!m_pFieldTable)
		m_pFieldTable = new CRdbField[m_nFieldCount];
	CRdbField* pField = m_pFieldTable + nFieldNo;
	if(!pField->m_pData)
		pField->Initialize(this, (CFieldDefine*)m_pTabDef->m_pFieldDefineSet[nFieldNo], m_nRecHeadSize);
	return pField;
}

void CRdbRecord::Clear()
{
	register uint32* pFlag = (uint32*)m_pRecordData;
	for(register uint32 i=0; i<m_nFieldCount; ++i)
		if(pFlag[RDB_FLG_L(i)] & RDB_FLG_B(i))
			GetField(i)->SetNull();
}

void CRdbRecord::Clear(CSqlParameter* pInsertAttr, uint32 nCount)
{
	register CSqlParameter::CParaItem* pParaTable = pInsertAttr->m_pParaTable;
	for(register uint32 i=0; i<nCount; ++i)
		GetField(pParaTable[i].nFieldNo)->SetNull();
}

uint64 CRdbRecord::GetRowId()
{
	return m_pKernelItem->nRowId;
}

void CRdbRecord::GetRecord(CRecord* pRecord, CSqlFilter & oFilter)
{
	pRecord->Clear();
	pRecord->m_nRowId = m_pKernelItem->nRowId;
	register uint32 nFilterCount = oFilter.m_nParaCount;
	register uint32 nCount = nFilterCount?nFilterCount:m_pTabDef->m_nFieldCount;
	for(register uint32 i=0; i<nCount; ++i)
	{
		register uint32 nFieldNo = (nFilterCount?oFilter.m_pParaTable[i]:i);
		register CRdbFieldDef* pAttr = m_pTabDef->m_pFieldDefineSet[nFieldNo]->m_pBaseAttr;
		register int32 nJob = pAttr->nJob;
		if(nJob > 0)
		{
			CField* pFieldValue = pRecord->GetField(nFieldNo);
			if(nJob <= (int32)m_pKernelItem->nJobId)
				GetField(nFieldNo)->GetField(pFieldValue);
			else if(pAttr->sDefault)
				pFieldValue->SetFromString(pAttr->sDefault);
		}
	}
}

void CRdbRecord::ClearDirty()
{
	m_nDirty = 0;
}

//------------------------------------------------------------------------
// CDataBaseSpace
//------------------------------------------------------------------------
CDataBaseSpace::CDataBaseSpace()
{
	m_pSpace = NewVirtualSpace();
}

CDataBaseSpace::~CDataBaseSpace()
{
	DeleteVirtualSpace(m_pSpace);
}

int32 CDataBaseSpace::InMemory()
{
	return VirtualSpaceInMemory(m_pSpace);
}

bool CDataBaseSpace::CreateObject(int32 bInMemory)
{
	return CreateVirtualSpace(m_pSpace, bInMemory);
}

void CDataBaseSpace::DestroyObject()
{
	DestroyVirtualSpace(m_pSpace);
}

uint64 CDataBaseSpace::GetThis()
{
	return GetVirtualSpaceAddress(m_pSpace);
}

CDataBaseSpace& CDataBaseSpace::SetThis(uint64 nThis)
{
	SetVirtualSpaceAddress(m_pSpace, nThis);
	return *this;
}

uint64 CDataBaseSpace::Allocate(uint32 nSize, int32)
{
	return AllocateFromVirtualSpace(m_pSpace, nSize);
}

void CDataBaseSpace::DeAllocate(uint64 nAddr)
{
	DeAllocateIntoVirtualSpace(m_pSpace, nAddr);
}

void CDataBaseSpace::DeAllocateAll()
{
	ClearVirtualSpace(m_pSpace);
}

//------------------------------------------------------------------------
// CTableSpace
//------------------------------------------------------------------------
CTableSpace::~CTableSpace()
{
}

CTableSpace::CTableSpace(CTableDefine* pTabDef)
{
	m_pTabDef = pTabDef;
	m_nThis = 0;
}

CDataBaseSpace& CTableSpace::GetDiskSpace()
{
	return m_oDiskSpace;
}

CTableSpace& CTableSpace::SetThis(uint64 nThis)
{
	m_nThis = nThis;
	if(m_nThis)
	{
		uint64 pAddrs[2];
		vquery(m_nThis, (char*)pAddrs, 16);
		m_nMaxRowId = pAddrs[0];
		m_oDiskSpace.SetThis(pAddrs[1]);
	}
	return *this;
}

uint64 CTableSpace::GetThis()
{
	return m_nThis;
}

bool CTableSpace::CreateObject()
{
	uint64 pAddr[2] = {0, 0};
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;
	m_nThis = vmalloc(16, bMemory);
	if(!m_nThis)
		return false;
	m_nMaxRowId = 1;
	pAddr[0] = m_nMaxRowId;
	if(!m_oDiskSpace.CreateObject(bMemory))
	{
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	pAddr[1] = m_oDiskSpace.GetThis();
	vcommit(m_nThis, (char*)pAddr, 16);
	return true;
}

void CTableSpace::DestroyObject()
{
	if(m_nThis)
	{
		m_oDiskSpace.DestroyObject();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

void CTableSpace::Truncate()
{
	if(m_nThis)
		m_oDiskSpace.DeAllocateAll();
}

uint64 CTableSpace::AllocateRecord()
{
	if(!m_nThis)
		return 0;

	return m_oDiskSpace.Allocate(m_pTabDef->m_nFileRecordSize, 0);
}

void CTableSpace::DeAllocateRecord(uint64 nRecord)
{
	if(m_nThis)
		m_oDiskSpace.DeAllocate(nRecord);
}

void CTableSpace::ReadVarField(CVarFieldStruct& oVar, uint32 nRecSize, char* pBuf)
{
	char* p;
	uint64 nAddr = oVar.nAddr;
	uint32 nSize = oVar.nSize;
	int32 bInMemory = m_oDiskSpace.InMemory();
	while(nSize && nAddr)
	{
		if(bInMemory)
			p = GET_MEMORY_OBJECT(char, nAddr);
		else
			p = (char*)vmap(nAddr);
		uint32 nReadSize = nSize;
		if(nReadSize > nRecSize)
			nReadSize = nRecSize;
		CBinary::MemoryCopy(pBuf, p, nReadSize);
		pBuf += nReadSize;
		nSize -= nReadSize;
		if(nSize)
			CBinary::MemoryCopy(&nAddr, p+nRecSize, 8);
		if(!bInMemory)
			unvmap(p);
	}
}

bool CTableSpace::RemoveVarField(CVarFieldStruct& oVar, uint32 nRecSize)
{
	if(oVar.nSize && oVar.nAddr)
	{
		while(oVar.nSize && oVar.nAddr)
		{
			uint64 nAddr = 0;
			uint32 nReadSize = oVar.nSize;
			if(oVar.nSize > nRecSize)
			{
				nReadSize = nRecSize;
				vquery(oVar.nAddr + nRecSize, (char*)&nAddr, sizeof(uint64));
			}
			m_oDiskSpace.DeAllocate(oVar.nAddr);
			oVar.nAddr = nAddr;
			oVar.nSize -= nReadSize;
		}
		oVar.nAddr = 0;
		oVar.nSize = 0;
		return true;
	}
	return false;
}

bool CTableSpace::WriteVarField(CVarFieldStruct& oVar, uint32 nRecSize, char* pBuf, uint32 nBufSize)
{
	uint64 nPrev = 0;
	char* s1=NULL, *s2;
	CVarFieldStruct oTmp = {0, 0};
	int32 bInMemory = m_oDiskSpace.InMemory();
	while(nBufSize)
	{
		uint32 nWriteSize = nBufSize;
		if(nWriteSize > nRecSize)
			nWriteSize = nRecSize;
		uint64 nRecord = m_oDiskSpace.Allocate(nRecSize+8, 0);
		if(!nRecord)
		{
			if(nPrev && !bInMemory)
				unvmap(s1);
			RemoveVarField(oTmp, nRecSize);
			return false;
		}
		if(bInMemory)
			s2 = GET_MEMORY_OBJECT(char, nRecord);
		else
			s2 = (char*)vmap(nRecord);
		if(nPrev)
		{
			CBinary::MemoryCopy(s1+nRecSize, &nRecord, sizeof(nRecord));
			if(!bInMemory)
			{
				vflush(s1);
				unvmap(s1);
			}
		}
		else
			oTmp.nAddr = nRecord;
		oTmp.nSize += nWriteSize;
		CBinary::MemoryCopy(s2, pBuf, nWriteSize);
		nBufSize -= nWriteSize;
		pBuf += nWriteSize;
		s1 = s2;
		nPrev = nRecord;
	}
	if(!bInMemory)
	{
		vflush(s1);
		unvmap(s1);
	}
	oVar.nAddr = oTmp.nAddr;
	oVar.nSize = oTmp.nSize;
	return true;
}

uint64 CTableSpace::ApplyRowid()
{
	m_oMutex.Enter();
	uint64 nRow = m_nMaxRowId++;
	vcommit(m_nThis, (char*)&m_nMaxRowId, 8);
	m_oMutex.Leave();
	return nRow;
}

//------------------------------------------------------------------------
// CKernelIndexItem
//------------------------------------------------------------------------
CKernelIndexItem::operator uint32()
{
	if(!nRowId)
		return 0;
	return 1;
}

CKernelIndexItem& CKernelIndexItem::operator=(uint32)
{
	nRowId = 0;
	return *this;
}

CKernelIndexItem& CKernelIndexItem::operator=(const CKernelIndexItem& oSrc)
{
	if(this != &oSrc)
	{
		nRowId = oSrc.nRowId;
		nRecord = oSrc.nRecord;
		nJobId = oSrc.nJobId;
	}
	return *this;
}

//------------------------------------------------------------------------
// CDataTable
//------------------------------------------------------------------------
CDataTable::CIterator::~CIterator()
{
	Detach();
}

void CDataTable::CIterator::UnLock()
{
	if(m_bLocked)
	{
		m_bLocked = false;
		m_pLock[m_pValue->nRowId%RDB_MAX_RECORD_LOCK].Leave();
	}
}

CDataTable::CIterator::CIterator()
{
	m_pKernelIndex = NULL;
	m_pLock = NULL;
	m_bLocked = false;
	m_oIt.pBlock = NULL;
	m_pValue = NULL;
}

CDataTable::CIterator::CIterator(CDataTable::THash::CHashIterator& oIt, 
									CDataTable::THash* pKernelIndex, 
									CMutex* pLock)
{
	m_pKernelIndex = pKernelIndex;
	m_pLock = pLock;
	m_oIt = oIt;
	m_bLocked = false;
	m_pValue = NULL;
	if(m_oIt.pBlock)
	{
		m_pValue = &m_oIt.pBlock->oNode[m_oIt.nSub].oObject;
		m_pLock[m_pValue->nRowId%RDB_MAX_RECORD_LOCK].Leave();
		m_bLocked = true;
	}
}


CDataTable::CIterator::CIterator(const CDataTable::CIterator& oIt)
{
	m_pKernelIndex = oIt.m_pKernelIndex;
	m_pLock = oIt.m_pLock;
	m_oIt = oIt.m_oIt;
	m_bLocked = oIt.m_bLocked;
	m_pValue = oIt.m_pValue;
	if(m_bLocked)
		((CIterator&)oIt).m_bLocked = false;
	if(m_oIt.pBlock)
		++m_oIt.pBlock->m_nCounter;
}

CDataTable::CIterator& CDataTable::CIterator::operator=(const CDataTable::CIterator& oIt)
{
	if(this != &oIt)
	{
		Detach();
		m_pKernelIndex = oIt.m_pKernelIndex;
		m_pLock = oIt.m_pLock;
		m_oIt = oIt.m_oIt;
		m_bLocked = oIt.m_bLocked;
		m_pValue = oIt.m_pValue;
		if(m_bLocked)
			((CIterator&)oIt).m_bLocked = false;
		if(m_oIt.pBlock)
			++m_oIt.pBlock->m_nCounter;
	}
	return *this;
}

void CDataTable::CIterator::Detach()
{
	UnLock();
	if(m_oIt.pBlock)
	{
		m_pKernelIndex->ReleaseBlock(m_oIt.nBlock, m_oIt.pBlock);
		m_oIt.pBlock = NULL;
		m_pValue = NULL;
	}
}

bool CDataTable::CIterator::operator==(const CDataTable::CIterator& oIt)
{
	if(m_pLock != oIt.m_pLock)
		return false;
	if(m_oIt.nBlock != oIt.m_oIt.nBlock)
		return false;
	if(m_oIt.pBlock != oIt.m_oIt.pBlock)
		return false;
	if(m_oIt.nSub != oIt.m_oIt.nSub)
		return false;
	return true;
}

bool CDataTable::CIterator::operator!=(const CDataTable::CIterator& oIt)
{
	if(m_pLock != oIt.m_pLock)
		return true;
	if(m_oIt.nBlock != oIt.m_oIt.nBlock)
		return true;
	if(m_oIt.pBlock != oIt.m_oIt.pBlock)
		return true;
	if(m_oIt.nSub != oIt.m_oIt.nSub)
		return true;
	return false;
}

CDataTable::CIterator& CDataTable::CIterator::operator++ ()
{
	UnLock();
	m_pValue = NULL;

	if(m_oIt.pBlock)
		++m_oIt.nSub;
	else
		m_oIt.m = 0;

	do
	{
		if(!m_oIt.pBlock)
		{
			m_oIt.nSub = 0;
			while(m_oIt.m < m_oIt.pHashInfo->n)
			{
				m_oIt.pBlock = m_pKernelIndex->GetBlock(m_oIt.m, m_oIt.nBlock);
				if(m_oIt.pBlock)
					break;
				++m_oIt.m;
			}
			if(!m_oIt.pBlock)
				break;
		}
		for(; m_oIt.nSub < RDB_HASH_BLOCKSIZE; ++m_oIt.nSub)
		{
			if(m_oIt.pBlock->oNode[m_oIt.nSub].oObject.nRowId)
				break;
		}
		if(m_oIt.nSub == RDB_HASH_BLOCKSIZE)
		{
			m_pKernelIndex->ReleaseBlock(m_oIt.nBlock, m_oIt.pBlock);
			m_oIt.pBlock = NULL;
			++m_oIt.m;			
		}
	}while(m_oIt.nSub == RDB_HASH_BLOCKSIZE);

	if(m_oIt.pBlock)
	{
		m_pValue = &m_oIt.pBlock->oNode[m_oIt.nSub].oObject;
		m_pLock[m_pValue->nRowId%RDB_MAX_RECORD_LOCK].Enter();
		m_bLocked = true;
	}
	else
	{
		m_oIt.nBlock = 0;
		m_oIt.nSub = 0;
		m_oIt.m = 0;
	}

	return *this;
}

CDataTable::CIterator CDataTable::CIterator::operator++ (int)
{
	CIterator oIt(*this);
	operator++();
	return oIt;
}

void CDataTable::CIterator::KillSelf()
{
	if(m_pValue)
	{
		THash::CHashIterator oIt = m_oIt;

		if(m_oIt.pBlock)
			++m_oIt.nSub;
		else
			m_oIt.m = 0;

		do
		{
			if(!m_oIt.pBlock)
			{
				m_oIt.nSub = 0;
				while(m_oIt.m < m_oIt.pHashInfo->n)
				{
					m_oIt.pBlock = m_pKernelIndex->GetBlock(m_oIt.m, m_oIt.nBlock);
					if(m_oIt.pBlock)
						break;
					++m_oIt.m;
				}
				if(!m_oIt.pBlock)
					break;
			}
			for(; m_oIt.nSub < RDB_HASH_BLOCKSIZE; ++m_oIt.nSub)
			{
				if(m_oIt.pBlock->oNode[m_oIt.nSub].oObject.nRowId)
					break;
			}
			if(m_oIt.nSub == RDB_HASH_BLOCKSIZE)
			{
				m_pKernelIndex->ReleaseBlock(m_oIt.nBlock, m_oIt.pBlock);
				m_oIt.pBlock = NULL;
				++m_oIt.m;			
			}
		}while(m_oIt.nSub == RDB_HASH_BLOCKSIZE);

		m_pKernelIndex->RemoveNode(oIt.m, oIt.nBlock, oIt.nSub);

		UnLock();

		if(m_oIt.pBlock)
		{
			m_pValue = &m_oIt.pBlock->oNode[m_oIt.nSub].oObject;
			m_pLock[m_pValue->nRowId%RDB_MAX_RECORD_LOCK].Enter();
			m_bLocked = true;
		}
		else
		{
			m_oIt.nBlock = 0;
			m_oIt.nSub = 0;
			m_oIt.m = 0;
			m_pValue = NULL;
		}
	}
}

CKernelIndexItem* CDataTable::CIterator::GetValue()
{
	return m_pValue;
}

CDataTable::THash::CHashIterator& CDataTable::CIterator::GetIterator()
{
	return m_oIt;
}

CDataTable::CDataTable(CTableDefine* pTabDef, CTableSpace* pTableSpace)
:m_oKernelIndex(&pTableSpace->GetDiskSpace(), 0, 0.0625)
{
	m_nThis = 0;
	m_pTabDef = pTabDef;
	m_pTableSpace = pTableSpace;
}

CDataTable::~CDataTable()
{
}

uint64 CDataTable::GetThis()
{
	return m_nThis;
}

CDataTable& CDataTable::SetThis(uint64 nThis)
{
	CRdbHashInfo oInfo;
	m_nThis = nThis;
	if(nThis)
	{
		vquery(nThis, (char*)&oInfo, sizeof(oInfo));
		m_oKernelIndex.SetThis(oInfo.nAddr, oInfo.oInfo);
	}
	return *this;
}

bool CDataTable::CreateObject()
{
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;
	static CRdbHashInfo oInfo = {0, {1, 2, 0, 1}};
	m_nThis = vmalloc(sizeof(oInfo), bMemory);
	if(!m_nThis)
		return false;
	m_oKernelIndex.SetThis(0, oInfo.oInfo);
	vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	return true;
}

void CDataTable::DestroyObject()
{
	if(m_nThis)
	{
		static CVmmHashInfo oInfo = {1,2,0,1};
		m_oKernelIndex.SetThis(0, oInfo);
		m_pTableSpace->GetDiskSpace().DeAllocateAll();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

void CDataTable::Truncate()
{
	if(m_nThis)
	{
		static CRdbHashInfo oInfo = {0, {1, 2, 0, 1}};
		m_oKernelIndex.SetThis(0, oInfo.oInfo);
		vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	}
}

CDataTable::CIterator CDataTable::Begin()
{
	CIterator oIt(End());
	return (++oIt);
}

CDataTable::CIterator CDataTable::End()
{
	CVmmHashInfo& oHashInfo = m_oKernelIndex.GetHashInfo();
	THash::CHashIterator oIt = {0, NULL, 0, 0, &oHashInfo};
	return CIterator(oIt, &m_oKernelIndex, m_pLock);
}

CDataTable::CIterator CDataTable::Find(uint64 nRowId)
{
	uint32 i;
	CVmmHashInfo& oHashInfo = m_oKernelIndex.GetHashInfo();
	THash::CHashIterator oIt = {0, NULL, 0, 0, &oHashInfo};

	uint64 nNext;
	if(nRowId)
	{
		oIt.pBlock = m_oKernelIndex.GetNode((uint32)nRowId, oIt.nBlock, oIt.m);
		while(oIt.pBlock)
		{
			uint32 nCount=0, nSize = oIt.pBlock->nSize;
			for(i=0; nCount<nSize; ++i)
			{
				uint64 id = oIt.pBlock->oNode[i].oObject.nRowId;
				if(id)
				{
					if(id == nRowId)
					{
						oIt.nSub = i;
						break;
					}
					++nCount;
				}
			}
			if(nCount<nSize)
				break;
			THash::THashBlock* pBlock = m_oKernelIndex.GetNextNode(oIt.pBlock, nNext);
			m_oKernelIndex.ReleaseBlock(oIt.nBlock, oIt.pBlock);
			oIt.pBlock = pBlock;
			oIt.nBlock = nNext;
		}
	}

	return CIterator(oIt, &m_oKernelIndex, m_pLock);
}

CDataTable::CIterator& CDataTable::Erase(CDataTable::CIterator& oIt)
{
	CKernelIndexItem* pItem = oIt.GetValue();
	if(pItem)
	{
		m_pTableSpace->DeAllocateRecord(pItem->nRecord);
		oIt.KillSelf();
		CVmmHashInfo& oHashInfo = m_oKernelIndex.GetHashInfo();
		CRdbHashInfo oInfo = {m_oKernelIndex.GetThis(), {oHashInfo.i, oHashInfo.n, oHashInfo.r, oHashInfo.d}};
		vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	}
	return oIt;
}

CDataTable::CIterator CDataTable::Insert()
{
	bool bConflict;
	uint64 nRecordId = m_pTableSpace->AllocateRecord();
	if(!nRecordId)
		return End();
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;
	CVmmHashInfo& oHashInfo = m_oKernelIndex.GetHashInfo();
	CKernelIndexItem oItem = {m_pTableSpace->ApplyRowid(), nRecordId, m_pTabDef->m_pBaseAttr->nMaxJob};
	THash::CHashIterator oIt = {0, NULL, 0, 0, &oHashInfo};
	oIt.pBlock = m_oKernelIndex.InsertNode((uint32)oItem.nRowId, oItem, NULL, NULL, bMemory, oIt.nBlock, oIt.nSub, bConflict);
	CRdbHashInfo oInfo = {m_oKernelIndex.GetThis(), {oHashInfo.i, oHashInfo.n, oHashInfo.r, oHashInfo.d}};
	vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	return CIterator(oIt, &m_oKernelIndex, m_pLock);
}

CDataTable::CIterator& CDataTable::ActivateRecord(CDataTable::CIterator& it, bool &bUpgradeFailure)
{
	bUpgradeFailure = false;
	CKernelIndexItem* pValue = it.GetValue();
	if(pValue)
	{
		CKernelIndexItem& oItem = *pValue;
		uint32 nMaxJob = m_pTabDef->GetBaseAttr()->nMaxJob;
		if(oItem.nJobId != nMaxJob)
		{
			uint64 nOldRecord = oItem.nRecord;
			uint64 nRecord = m_pTableSpace->AllocateRecord();
			if(!nRecord)
				bUpgradeFailure = true;
			else
			{
				CKernelIndexItem oNewItem = oItem;
				oNewItem.nRecord = nRecord;
				oNewItem.nJobId = nMaxJob;
				CRdbRecord oOldRecord(m_pTableSpace, m_pTabDef, &oItem);
				CRdbRecord oNewRecord(m_pTableSpace, m_pTabDef, &oNewItem);
				uint32 nFieldCount = m_pTabDef->m_nFieldCount;
				for(uint32 i=0; i<nFieldCount; ++i)
				{
					CFieldDefine* pField = (CFieldDefine*)m_pTabDef->m_pFieldDefineSet[i];
					CRdbFieldDef* pAttr = pField->m_pBaseAttr;
					int32 nJob = pAttr->nJob;
					if(nJob < 0)
						nJob = -nJob;
					if(nJob <= (int32)oItem.nJobId)
					{						
						if(pAttr->nJob > 0)
							oNewRecord.GetField(i)->CopyField(oOldRecord.GetField(i), pField->m_nDiskSize);
						else switch(pAttr->nType)
						{
						case RDB_VARCHAR_FIELD:
						case RDB_VARRAW_FIELD:
						case RDB_VARLCHAR_FIELD:
							oOldRecord.GetField(i)->SetNull();
							break;
						}
					}
					else if(pAttr->nJob > 0 && pAttr->sDefault)
					{
						if(oNewRecord.GetField(i)->SetField(pAttr->sDefault))
						{
							for(uint32 j=0; j<i; ++j)
							{
								pField = (CFieldDefine*)m_pTabDef->m_pFieldDefineSet[j];
								pAttr = pField->m_pBaseAttr;
								if(pAttr->nJob > (int32)oItem.nJobId && pAttr->sDefault)
									oNewRecord.GetField(j)->SetNull();
							}
							bUpgradeFailure = true;
							oNewRecord.ClearDirty();
							break;
						}
					}
				}
			}
			if(bUpgradeFailure == false)
			{
				oItem.nRecord = nRecord;
				oItem.nJobId = nMaxJob;
				it.GetIterator().pBlock->m_nDirty = 1;
				m_pTableSpace->DeAllocateRecord(nOldRecord);
			}
			else if(nRecord)
				m_pTableSpace->DeAllocateRecord(nRecord);
		}
	}
	return it;
}

FOCP_END();
