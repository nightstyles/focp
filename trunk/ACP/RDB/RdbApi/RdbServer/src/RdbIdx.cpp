
#include "RdbIdx.hpp"

FOCP_BEGIN();

//----------------------------------------------------------
// CAbstractIndex
//----------------------------------------------------------
CAbstractIndex::CAbstractIndex(CTableDefine* pTabDef, 
							   CIndexDefine* pIdxDef, CTableSpace* pTableSpace,
							   CDataTable* pDataTable)
{
	m_pTabDef = pTabDef;
	m_pIdxDef = pIdxDef;
	m_pTableSpace = pTableSpace;
	m_pDataTable = pDataTable;

	m_pPrimaryIndex = NULL;
	m_nDetailIndexCount = 0;
	m_pDetailIndexes = NULL;

	if(m_pIdxDef)
		m_pIdxDef->SetInstance(this);
}

CAbstractIndex::~CAbstractIndex()
{
	if(m_pDetailIndexes)
		CMalloc::Free(m_pDetailIndexes);
}

CDataTable* CAbstractIndex::GetDataTable()
{
	return m_pDataTable;
}

CTableDefine* CAbstractIndex::GetTableDefine()
{
	return m_pTabDef;
}

CIndexDefine* CAbstractIndex::GetIndexDefine()
{
	return m_pIdxDef;
}

CTableSpace* CAbstractIndex::GetTableSpace()
{
	return m_pTableSpace;
}

CAbstractIndex* CAbstractIndex::GetPrimaryIndex()
{
	return m_pPrimaryIndex;
}

void CAbstractIndex::AddDetailIndex(CAbstractIndex* pIndex)
{
	m_pDetailIndexes = (CAbstractIndex**)CMalloc::Realloc(m_pDetailIndexes, 
		(m_nDetailIndexCount+1)*sizeof(void*));
	m_pDetailIndexes[m_nDetailIndexCount] = pIndex;
	++m_nDetailIndexCount;
}

void CAbstractIndex::DelDetailIndex(CAbstractIndex* pIndex)
{
	uint32 i;
	for(i=0; i<m_nDetailIndexCount; ++i)
	{
		if(m_pDetailIndexes[i] == pIndex)
			break;
	}
  if(i < m_nDetailIndexCount)
  {
		for(; i<m_nDetailIndexCount-1; ++i)
			m_pDetailIndexes[i] = m_pDetailIndexes[i+1];
		--m_nDetailIndexCount;
  }
}

void CAbstractIndex::SetPrimaryIndex(CAbstractIndex* pPrimaryIndex)
{
	if(m_pPrimaryIndex)
		m_pPrimaryIndex->DelDetailIndex(this);
	m_pPrimaryIndex = pPrimaryIndex;
	if(m_pPrimaryIndex)
		m_pPrimaryIndex->AddDetailIndex(this);
}

uint32 CAbstractIndex::GetDetailIndexCount()
{
	return m_nDetailIndexCount;
}

CAbstractIndex* CAbstractIndex::GetDetailIndex(uint32 nIndexNo)
{
	return m_pDetailIndexes[nIndexNo];
}

void CAbstractIndex::BuildCond(CSqlParameterSet& oCond, CSqlFilter* pFilter, CRecord* pRecord)
{
  CSqlParameter* pPara = (CSqlParameter*)oCond.AddPara();
  CRecord* pDstRecord = &pPara->m_oRecord;
  uint32 nCount = pFilter->m_nParaCount;
  for(uint32 i=0; i<nCount; ++i)
  {
		uint32 nFieldNo = pFilter->m_pParaTable[i];
		pDstRecord->GetField(nFieldNo)->SetFromField(pRecord->GetField(nFieldNo));
		pPara->SetField(nFieldNo, RDB_SQLPARA_OPERATOR_EQUAL);
  }
}

bool CAbstractIndex::ExistInPrimaryTable(CSqlFilter* pFilter, CRecord* pRecord)
{
	uint32 bExist;
	CSqlParameterSet oCond(m_pPrimaryIndex->m_pTabDef);
	BuildCond(oCond, pFilter, pRecord);
	return(!m_pPrimaryIndex->Exist(&oCond, bExist) && bExist);
}

uint32 CAbstractIndex::AttachRecord(CRecord*)
{
	return RDB_SUCCESS;
}

uint32 CAbstractIndex::DetachRecord(CRecord*)
{
	return RDB_SUCCESS;
}

uint32 CAbstractIndex::AllocRecord(CRecordSet *pResult, CRecord* &pRecord)
{
	pRecord = NULL;
	if(!pResult)
	{
		pRecord = new CRecord(m_pTabDef);
		if(!pRecord)
			return RDB_LACK_MEMORY;
	}
	return RDB_SUCCESS;
}

uint32 CAbstractIndex::IsCoincident(CRecord* &pRecord, 
								   uint64 nRowId, 
								   CSqlParameterSet* pCondition, 
								   CSqlFilter& oFilter, 
								   CRecordSet *pResult, 
								   uint32* pSkipCount, 
								   uint32* pCount, 
								   uint32* pMaxCount,
								   uint32& nFull,
								   bool bUpdate,
								   FOnTravelIndex OnTravel,
								   void* pTravelPara)
{
	nFull = 0;
	CDataTable::CIterator oIdx = m_pDataTable->Find(nRowId);
	if(bUpdate)
	{
		bool bUpgradeFailure = false;
		m_pDataTable->ActivateRecord(oIdx, bUpgradeFailure);
		if(bUpgradeFailure)
		{
			if(pRecord)
			{
				if(pResult)
					pResult->PopRecord();
				else
					delete pRecord;
			}
			return RDB_LACK_STORAGE;
		}
	}
	if(!pRecord)
	{
		pRecord = pResult->AllocRecord();
		if(!pRecord)
			return RDB_LACK_MEMORY;
	}
	if(1)
	{
		CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oIdx.GetValue());
		oRdbRecord.GetRecord(pRecord, oFilter);
	}
	if(pCondition->IsCoincident(pRecord[0]))
	{
		if(pCount)
		{
			if(OnTravel)
			{
				if(OnTravel(this, oIdx, pRecord, pTravelPara))
					return RDB_TRAVEL_FAILURE;
			}
			++pCount[0];
			if(pMaxCount && (pCount[0] == pMaxCount[0]))
				nFull = 1;
		}
		else if(pSkipCount[0])
			--pSkipCount[0];
		else
		{
			pRecord = NULL;
			if(pResult->GetRecordCount() == pResult->GetRecordSetSize())
				nFull = 1;
		}
	}
	return RDB_SUCCESS;
}

uint32 CAbstractIndex::LoadAllData()
{
	CDataTable::CIterator idx = m_pDataTable->Begin();
	CDataTable::CIterator end = m_pDataTable->End();
	CRecord oRecord(m_pTabDef);
	CSqlFilter oFilter(m_pTabDef);
	for(; idx!=end; ++idx)
	{
		CRdbRecord oRdbRec(m_pTableSpace, m_pTabDef, idx.GetValue());
		oRdbRec.GetRecord(&oRecord, oFilter);
		uint32 nRet = AttachRecord(&oRecord);
		if(nRet)
			return nRet;
	}
	return RDB_SUCCESS;
}

const char* CAbstractIndex::GetLogName()
{
	return m_pTabDef->GetLogName();
}

//----------------------------------------------------------
// CRdbRowIndex
//----------------------------------------------------------
CRdbRowIndex::CRdbRowIndex(CTableDefine* pTabDef, CTableSpace* pTableSpace)
:CAbstractIndex(pTabDef, NULL, pTableSpace, &m_oDataTable),m_oDataTable(pTabDef, pTableSpace)
{
}

CRdbRowIndex::~CRdbRowIndex()
{
}

bool CRdbRowIndex::CreateObject()
{
	return m_oDataTable.CreateObject();
}

void CRdbRowIndex::DestroyObject()
{
	m_oDataTable.DestroyObject();
}

CAbstractIndex& CRdbRowIndex::SetThis(uint64 nThis)
{
	m_oDataTable.SetThis(nThis);
	return *this;
}

uint64 CRdbRowIndex::GetThis()
{
	return m_oDataTable.GetThis();
}

void CRdbRowIndex::Truncate()
{
	m_oDataTable.Truncate();
}

uint32 CRdbRowIndex::Query(CSqlParameterSet* pCondition,
						   CSqlFilter& oFilter, 
						   CRecordSet &oResult,
						   uint32 nSkipCount)
{
	CRecord* pRecord = NULL;
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	CDataTable::CIterator oBeg(m_oDataTable.Begin());
	CDataTable::CIterator oEnd(m_oDataTable.End());
	for(; oBeg!=oEnd; ++oBeg)
	{
		if(1)
		{
			CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oBeg.GetValue());
			if(!pRecord)
			{
				pRecord = oResult.AllocRecord();
				if(!pRecord)
					return RDB_LACK_MEMORY;
			}
			oRdbRecord.GetRecord(pRecord, oFilter);
		}
		if(pCondition->IsCoincident(pRecord[0]))
		{
			if(nSkipCount)
			{
				--nSkipCount;
				continue;
			}
			pRecord = NULL;
			if(oResult.GetRecordCount() == oResult.GetRecordSetSize())
				break;
		}
	}
	if(pRecord)
		oResult.PopRecord();
	return RDB_SUCCESS;
}

uint32 CRdbRowIndex::QueryCount(CSqlParameterSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	CRecord oRecord(m_pTabDef);
	CSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	CDataTable::CIterator oBeg(m_oDataTable.Begin());
	CDataTable::CIterator oEnd(m_oDataTable.End());
	for(; oBeg!=oEnd; ++oBeg)
	{
		if(1)
		{
			CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oBeg.GetValue());
			oRdbRecord.GetRecord(&oRecord, oFilter);
		}
		if(pCondition->IsCoincident(oRecord))
			++nCount;
	}
	return RDB_SUCCESS;
}

uint32 CRdbRowIndex::Exist(CSqlParameterSet * pCondition, uint32& bExist)
{
	bExist = 0;
	CRecord oRecord(m_pTabDef);
	CSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	CDataTable::CIterator oBeg(m_oDataTable.Begin());
	CDataTable::CIterator oEnd(m_oDataTable.End());
	for(; oBeg!=oEnd; ++oBeg)
	{
		if(1)
		{
			CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oBeg.GetValue());
			oRdbRecord.GetRecord(&oRecord, oFilter);
		}
		if(pCondition->IsCoincident(oRecord))
		{
			bExist = 1;
			break;
		}
	}
	return RDB_SUCCESS;
}

uint32 CRdbRowIndex::Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32& nCount, void* pPara)
{
	nCount = 0;
	CRecord oRecord(m_pTabDef);
	CDataTable::CIterator oBeg(m_oDataTable.Begin());
	CDataTable::CIterator oEnd(m_oDataTable.End());
	CDataTable::CIterator oIdx(oEnd);
	for(; oBeg!=oEnd; )
	{
		if(bUpdate)
		{
			bool bUpgradeFailure;
			m_oDataTable.ActivateRecord(oBeg, bUpgradeFailure);
			if(bUpgradeFailure)
				return RDB_LACK_STORAGE;
		}
		if(1)
		{
			CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oBeg.GetValue());
			oRdbRecord.GetRecord(&oRecord, oFilter);
		}
		oIdx = (oBeg++);
		if(pCondition->IsCoincident(oRecord))
		{
			if(OnTravel(this, oIdx, &oRecord, pPara))
				return RDB_TRAVEL_FAILURE;
			++nCount;
		}
	}
	return RDB_SUCCESS;
}

//----------------------------------------------------------
// CRdbHashIndex
//----------------------------------------------------------
CRdbHashIndex::~CRdbHashIndex()
{
}

CRdbHashIndex::CRdbHashIndex(CTableDefine* pTabDef,
									 CIndexDefine* pIdxDef, 
									 CTableSpace* pTableSpace,
									 CDataTable* pDataTable)
:CAbstractIndex(pTabDef, pIdxDef, pTableSpace, pDataTable),
m_oHash(&m_oAllocator, (pIdxDef->m_pBaseAttr->nQualifier&RDB_UNIQUE_INDEX)?1:0, pIdxDef->m_pBaseAttr->nHashRate/1000.0),
m_oFilter(pTabDef)
{
}

bool CRdbHashIndex::CreateObject()
{
	int32 bMemory = 1;

	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;

	CRdbHashInfo oInfo = {{0, 0}, {1, 2, 0, 1}};
	m_nThis = vmalloc(32, bMemory);
	if(!m_nThis)
		return false;
	if(!m_oAllocator.CreateObject(bMemory))
	{
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	oInfo.nAddr[0] = m_oAllocator.GetThis();
	m_oHash.SetThis(0, oInfo.oInfo);
	vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	return true;
}

void CRdbHashIndex::DestroyObject()
{
	if(m_nThis)
	{
		static CVmmHashInfo oInfo = {1,2,0,1};
		m_oHash.SetThis(0, oInfo);
		m_oAllocator.DestroyObject();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

CAbstractIndex& CRdbHashIndex::SetThis(uint64 nThis)
{
	CRdbHashInfo oInfo;
	m_nThis = nThis;
	if(nThis)
	{
		vquery(nThis, (char*)&oInfo, sizeof(oInfo));
		m_oAllocator.SetThis(oInfo.nAddr[0]);
		m_oHash.SetThis(oInfo.nAddr[1], oInfo.oInfo);
	}
	return *this;
}

uint64 CRdbHashIndex::GetThis()
{
	return m_nThis;
}

void CRdbHashIndex::Truncate()
{
	if(m_nThis)
	{
		CRdbHashInfo oInfo = {{m_oAllocator.GetThis(), 0}, {1, 2, 0, 1}};
		vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
		m_oHash.SetThis(0, oInfo.oInfo);
		m_oAllocator.DeAllocateAll();
	}
}

uint32 CRdbHashIndex::Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, &oResult, &nSkipCount, NULL, NULL);
}

uint32 CRdbHashIndex::QueryCount(CSqlParameterSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	m_oFilter.Clear();
	pCondition->GetFilter(m_oFilter);
	return QueryHelp(pCondition, m_oFilter, NULL, NULL, &nCount, NULL);
}

uint32 CRdbHashIndex::Exist(CSqlParameterSet * pCondition, uint32& bExist)
{
	bExist = 0;
	uint32 nCount = 0;
	uint32 nMaxCount = 1;
	m_oFilter.Clear();
	pCondition->GetFilter(m_oFilter);
	uint32 nRet = QueryHelp(pCondition, m_oFilter, NULL, NULL, &nCount, &nMaxCount);
	if(nCount)
		bExist = 1;
	return nRet;
}

uint32 CRdbHashIndex::Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32 &nCount, void* pPara)
{
	nCount = 0;
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL, bUpdate, OnTravel, pPara);
}

uint32 CRdbHashIndex::QueryHelp(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet *pResult, uint32* pSkipCount, uint32* pCount, uint32* pMaxCount, bool bUpdate, FOnTravelIndex OnTravel, void* pPara)
{
    uint32 nHashValue;
	uint32 nIdxFieldCount = m_pIdxDef->m_nFieldCount;
	if(nIdxFieldCount == 1)
	{
		uint32 nHashCount = 0;
		uint32 nFieldNo = m_pIdxDef->m_pFields[0];
		pCondition->GetHashValue(nFieldNo, nHashValue, nHashCount);
		if(nHashCount != 1)
			return RDB_INVALID_COND;
		nHashValue = nHashValue;
	}
	else
	{
		uint32 * pHashValue = new uint32[nIdxFieldCount];
		if(!pHashValue)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbHashIndex::QueryHelp(%s): RDB_LACK_MEMORY", m_pTabDef->m_pBaseAttr->sTableName));
			return RDB_LACK_MEMORY;
		}
		for(uint32 i=0; i<nIdxFieldCount; ++i)
		{
			uint32 nHashCount = 0;
			uint32 nFieldNo = m_pIdxDef->m_pFields[i];
			pCondition->GetHashValue(nFieldNo, nHashValue, nHashCount);
			if(nHashCount != 1)
				return RDB_INVALID_COND;
			pHashValue[i] = nHashValue;
		}
		nHashValue = GetCrc32((const uint8*)pHashValue, nIdxFieldCount<<2, 1);
		delete[] pHashValue;
	}

	uint64 nList;
	THashBlock * idx = m_oHash.GetNode(nHashValue, nList);
	if(idx == NULL)
		return RDB_SUCCESS;

	uint64 nNext;
	THashBlock oBlock = *idx;
	THashBlock* pNext = m_oHash.GetNextNode(idx, nNext);
	m_oHash.ReleaseBlock(nList, idx);
	CRecord* pRecord = NULL;
	uint32 nRet = AllocRecord(pResult, pRecord);
	if(nRet)
	{
		if(pNext)
			m_oHash.ReleaseBlock(nNext, pNext);
		return nRet;
	}

	uint32 nFull;
	while(true)
	{
		bool Done = false;
		uint32 nCount = 0;
		for(uint32 i=0; nCount<oBlock.nSize; ++i)
		{
			uint64 nRowId = oBlock.oNode[i].oObject;
			if(nRowId)
			{
				++nCount;
				if(oBlock.oNode[i].nHashValue == nHashValue)
				{
					nRet = IsCoincident(pRecord, nRowId, pCondition, 
						oFilter, pResult, pSkipCount, pCount, 
						pMaxCount, nFull, bUpdate, OnTravel, pPara);
					if(nRet)
					{
						if(pNext)
							m_oHash.ReleaseBlock(nNext, pNext);
						return nRet;
					}
					Done = true;
					if(nFull || m_oHash.IsUnique())
						break;
				}
			}
		}

		if(Done && (nFull || m_oHash.IsUnique()))
			break;

		if(!pNext)
			break;

		nList = nNext;
		idx = pNext;
		oBlock = *idx;

		pNext = m_oHash.GetNextNode(idx, nNext);
		m_oHash.ReleaseBlock(nList, idx);
	}

	if(pRecord)
	{
		if(pResult)
			pResult->PopRecord();
		else
			delete pRecord;
	}
	return RDB_SUCCESS;
}

struct CRecordIsEqualContext
{
	CRdbHashIndex * pIndex;
	CRecord* pRecord;
	CSqlFilter* pFilter;
};

static bool RecordIsEqual(const uint64&, const uint64& b, void* pPara)
{
	CRecordIsEqualContext * pContext = (CRecordIsEqualContext*)pPara;
	return pContext->pIndex->RecordIsEqual(pContext->pRecord, b, pContext->pFilter);
}

bool CRdbHashIndex::RecordIsEqual(CRecord* pRecord, uint64 nRowId, CSqlFilter* pFilter)
{
	CDataTable::CIterator oIdx = m_pDataTable->Find(nRowId);
	CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, oIdx.GetValue());
	CRecord oRecord(m_pTabDef);
	oRdbRecord.GetRecord(&oRecord, *pFilter);
	uint32 i, nIdxFieldCount = m_pIdxDef->m_nFieldCount;
	for(i=0; i<nIdxFieldCount; ++i)
	{
		uint32 nFieldNo = m_pIdxDef->m_pFields[i];
		if(pRecord->GetField(nFieldNo)->Compare(oRecord.GetField(nFieldNo)))
			return false;
	}
	return true;
}

uint32 CRdbHashIndex::AttachRecord(CRecord* pRecord)
{
	uint32 nHashValue;
	uint32 i, nIdxFieldCount = m_pIdxDef->m_nFieldCount;
	m_oFilter.Clear();
	if(nIdxFieldCount == 1)
	{
		uint32 nFieldNo = m_pIdxDef->m_pFields[0];
		m_oFilter.SetField(nFieldNo);
		nHashValue = pRecord->GetField(nFieldNo)->GetHashValue();
	}
	else
	{
		uint32 * pHashValue = new uint32[nIdxFieldCount];
		if(!pHashValue)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbHashIndex::AttachRecord(%s): RDB_LACK_MEMORY", m_pTabDef->m_pBaseAttr->sTableName));
			return RDB_LACK_MEMORY;
		}
		for(i=0; i<nIdxFieldCount; ++i)
		{
			uint32 nFieldNo = m_pIdxDef->m_pFields[i];
			m_oFilter.SetField(nFieldNo);	
			pHashValue[i] = pRecord->GetField(nFieldNo)->GetHashValue();
		}
		nHashValue = GetCrc32((const uint8*)pHashValue, nIdxFieldCount<<2, 1);
		delete[] pHashValue;
	}
	if(m_pPrimaryIndex && !ExistInPrimaryTable(&m_oFilter, pRecord))
		return RDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE;
	bool bConflict;
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;

	uint32 nSub;
	CRecordIsEqualContext oContext = {this, pRecord, &m_oFilter};
	uint64 nRowId = pRecord->m_nRowId, nBlock;
	THashBlock* pBlock = m_oHash.InsertNode(nHashValue, nRowId, FOCP_NAME::RecordIsEqual, &oContext, bMemory, nBlock, nSub, bConflict);
	if(!pBlock)
	{
		if(bConflict)
			return RDB_UNIQUE_INDEX_CONFLICT;
		return RDB_LACK_STORAGE;
	}
	m_oHash.ReleaseBlock(nBlock, pBlock);
	CVmmHashInfo& oHashInfo = m_oHash.GetHashInfo();
	CRdbHashInfo oInfo = {{m_oAllocator.GetThis(), m_oHash.GetThis()}, {oHashInfo.i, oHashInfo.n, oHashInfo.r, oHashInfo.d}};
	vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
	return RDB_SUCCESS;
}

static bool IsEqualRowId(void *pCond, const uint64& nRowId)
{
	return nRowId == (*(uint64*)pCond);
}

uint32 CRdbHashIndex::DetachRecord(CRecord* pRecord)
{
    uint32 nHashValue;
	uint32 i, nIdxFieldCount = m_pIdxDef->m_nFieldCount;
	if(nIdxFieldCount == 1)
	{
		uint32 nFieldNo = m_pIdxDef->m_pFields[0];
		nHashValue = pRecord->GetField(nFieldNo)->GetHashValue();
	}
	else
	{
		uint32 * pHashValue = new uint32[nIdxFieldCount];
		if(!pHashValue)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbHashIndex::DetachRecord(%s): RDB_LACK_MEMORY", m_pTabDef->m_pBaseAttr->sTableName));
			return RDB_LACK_MEMORY;
		}
		for(i=0; i<nIdxFieldCount; ++i)
		{
			uint32 nFieldNo = m_pIdxDef->m_pFields[i];
			pHashValue[i] = pRecord->GetField(nFieldNo)->GetHashValue();
		}
		nHashValue = GetCrc32((const uint8*)pHashValue, nIdxFieldCount<<2, 1);
		delete[] pHashValue;
	}

	uint64 nRowId = pRecord->m_nRowId;
	if(m_oHash.RemoveNode(nHashValue, FOCP_NAME::IsEqualRowId, &nRowId))
	{
		CVmmHashInfo& oHashInfo = m_oHash.GetHashInfo();
		CRdbHashInfo oInfo = {{m_oAllocator.GetThis(), m_oHash.GetThis()}, {oHashInfo.i, oHashInfo.n, oHashInfo.r, oHashInfo.d}};
		vcommit(m_nThis, (char*)&oInfo, sizeof(oInfo));
		return RDB_SUCCESS;
	}
	return RDB_RECORD_NOT_EXIST;
}

//-----------------------------------------------------------------------------
// CRdbRbTreeIndex
//-----------------------------------------------------------------------------
CIndexKeyValue::~CIndexKeyValue()
{
	if(m_pData)
	{
		delete[] m_pFlag;
		m_pData = NULL;
	}
}

CIndexKeyValue::CIndexKeyValue()
{
	Initialize(RDB_INT8_FIELD, 1, NULL, 0, 0, NULL);
	m_nRecSize = 0;
}

CIndexKeyValue::CIndexKeyValue(const CIndexKeyValue& oSrc)
:CField(oSrc)
{
	m_nRecSize = oSrc.m_nRecSize;
	if(m_nRecSize)
	{
		m_pFlag = (uint32*)(new char[m_nRecSize]);
		CBinary::MemoryCopy(m_pFlag, oSrc.m_pFlag, m_nRecSize);
		m_pData = m_pFlag + 2;
	}
}

CIndexKeyValue& CIndexKeyValue::operator=(const CIndexKeyValue& oSrc)
{
	if(this != &oSrc)
	{
		if(m_pData)
		{
			delete[] m_pFlag;
			m_pData = NULL;
		}
		CField::operator=(oSrc);
		m_nRecSize = oSrc.m_nRecSize;
		if(m_nRecSize)
		{
			m_pFlag = (uint32*)(new char[m_nRecSize]);
			CBinary::MemoryCopy(m_pFlag, oSrc.m_pFlag, m_nRecSize);
			m_pData = m_pFlag + 2;
		}
	}
	return *this;
}

void CIndexKeyValue::Create(uint32 nType, uint32 nSize, CTableDefine* pTabDef)
{
	m_pTabDef = pTabDef;
	m_nRecSize = 8;
	switch(nType)
	{
	case RDB_INT8_FIELD: case RDB_UINT8_FIELD:
	case RDB_INT16_FIELD: case RDB_UINT16_FIELD:
	case RDB_INT32_FIELD: case RDB_UINT32_FIELD: case RDB_FLOAT_FIELD:
	case RDB_INT64_FIELD: case RDB_UINT64_FIELD: case RDB_DOUBLE_FIELD:
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		m_nRecSize += nSize;
		break;
	case RDB_RAW_FIELD:
		m_nRecSize += nSize + 4;
		break;
	}
	m_nType = nType;
	m_nSize = nSize;
	m_pFlag = (uint32*)(new char[m_nRecSize]);
	m_pFlag[0] = 0;
	m_pData = m_pFlag + 2;
}

void CIndexKeyValue::Load(void* p)
{
	CBinary::MemoryCopy(m_pFlag, p, m_nRecSize);
}

void CIndexKeyValue::Load(uint64 nThis)
{
	vquery(nThis, (char*)m_pFlag, m_nRecSize);//nFlag+nData
}

void CIndexKeyValue::Save(uint64 nThis)
{
	vcommit(nThis, (char*)m_pFlag, m_nRecSize);//nFlag+nData
}

void CBaseRbTreeIndex::Clear(void* pBuf)
{
	CRdbRbTreeItem &oItem = *(CRdbRbTreeItem*)pBuf;
	if(m_nCurLevel != m_nTotalLevel-1)
	{
		uint64 nHandle = oItem.nHandle;
		CBaseRbTreeIndex* pSubIndex = m_pIndexPool->QueryObject(nHandle, this);
		pSubIndex->DestroyObject();
		m_pIndexPool->ReleaseObject(nHandle);
	}
}

CBaseRbTreeIndex* CBaseRbTreeIndex::QueryObject(uint64 nThis)
{
	CBaseRbTreeIndex* pSubIndex = new CBaseRbTreeIndex(m_pTabDef, m_pIdxDef, m_pTableSpace, 
			m_pDataTable, m_pSpace, m_pIndexPool, m_pFilter, m_nCurLevel+1);
	pSubIndex->SetThis(nThis);
	return pSubIndex;
}

CIndexKeyValue CBaseRbTreeIndex::GetKey(CRdbRbTreeItem& oSrc)
{
	char* p = (char*)&oSrc;
	CIndexKeyValue oKey;
	oKey.Create(m_nType, m_nSize, m_pTabDef);
	oKey.Load(p+40);
	return oKey;
}

int32 CBaseRbTreeIndex::Compare(CIndexKeyValue&l, CIndexKeyValue&r)
{
	return l.Compare(&r);
}

CBaseRbTreeIndex::~CBaseRbTreeIndex()
{
}

CBaseRbTreeIndex::CBaseRbTreeIndex(CTableDefine* pTabDef,
										 CIndexDefine* pIdxDef, 
										 CTableSpace* pTableSpace,
										 CDataTable* pDataTable,
										 CDataBaseSpace* pSpace,
										 CVirtualObjectPool<CBaseRbTreeIndex, CBaseRbTreeIndex>* pIndexPool,
										 CSqlFilter* pFilter,
										 uint32 nLevel):
CAbstractIndex(pTabDef, pIdxDef, pTableSpace, pDataTable),
m_oTree((CVirtualAccess*)this, 
		(CVirtualGetKey<CRdbRbTreeItem, CIndexKeyValue>*)this, 
		(CVirtualCompare<CIndexKeyValue>*)this,
		pSpace, nLevel?pSpace:NULL),
m_pSpace(pSpace)
{
	m_pFilter = pFilter;
	m_pIndexPool = pIndexPool;
	m_nTotalLevel = pIdxDef->m_nFieldCount;
	m_nCurLevel = nLevel;
	uint32 nFieldNo = m_pIdxDef->m_pFields[m_nCurLevel];
	CFieldDefine* pField = (CFieldDefine*)m_pTabDef->m_pFieldDefineSet[nFieldNo];
	CRdbFieldDef* pFieldDef = pField->m_pBaseAttr;
	m_nType = pFieldDef->nType;
	m_nSize = pFieldDef->nLen;
	m_nKeySize = 8;//nFlag
	switch(m_nType)
	{
	case RDB_INT8_FIELD: case RDB_UINT8_FIELD:
	case RDB_INT16_FIELD: case RDB_UINT16_FIELD:
	case RDB_INT32_FIELD: case RDB_UINT32_FIELD: case RDB_FLOAT_FIELD:
	case RDB_INT64_FIELD: case RDB_UINT64_FIELD: case RDB_DOUBLE_FIELD:
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		m_nKeySize += m_nSize;//data
		break;
	case RDB_RAW_FIELD:
		m_nKeySize += m_nSize + 4;//size+data
		break;
	}
}

bool CBaseRbTreeIndex::CreateObject()
{
	uint32 bUnique = 0;
	if(m_nCurLevel == m_nTotalLevel - 1)
	{
		uint32 nQualifier = m_pIdxDef->m_pBaseAttr->nQualifier;
		if(nQualifier & RDB_UNIQUE_INDEX)
			bUnique = 1;
	}
	int32 bInMemory = (m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE);
	return m_oTree.CreateObject(bUnique, bInMemory);
}

void CBaseRbTreeIndex::DestroyObject()
{
	m_oTree.DestroyObject();
}

CAbstractIndex& CBaseRbTreeIndex::SetThis(uint64 nThis)
{
	m_oTree.SetThis(nThis);
	return *this;
}

uint64 CBaseRbTreeIndex::GetThis()
{
	return m_oTree.GetThis();
}

void CBaseRbTreeIndex::Truncate()
{
	m_oTree.Truncate();
	m_pSpace->DeAllocateAll();
}

uint32 CBaseRbTreeIndex::Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, &oResult, &nSkipCount, NULL, NULL);
}

uint32 CBaseRbTreeIndex::QueryCount(CSqlParameterSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	m_pFilter->Clear();
	pCondition->GetFilter(*m_pFilter);
	return QueryHelp(pCondition, *m_pFilter, NULL, NULL, &nCount, NULL);
}

uint32 CBaseRbTreeIndex::Exist(CSqlParameterSet * pCondition, uint32& bExist)
{
	bExist = 0;
	uint32 nCount = 0;
	uint32 nMaxCount = 1;
	m_pFilter->Clear();
	pCondition->GetFilter(*m_pFilter);
	uint32 nRet = QueryHelp(pCondition, *m_pFilter, NULL, NULL, &nCount, &nMaxCount);
	if(nCount)
		bExist = 1;
	return nRet;
}

uint32 CBaseRbTreeIndex::Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32 &nCount, void * pPara)
{
	nCount = 0;
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL, bUpdate, OnTravel, pPara);
}

uint32 CBaseRbTreeIndex::QueryHelp(CSqlParameterSet* pCondition, 
									  CSqlFilter& oFilter, 
									  CRecordSet *pResult, 
									  uint32* pSkipCount, 
									  uint32* pCount, 
									  uint32* pMaxCount,
									  bool bUpdate,
									  FOnTravelIndex OnTravel,
									  void * pPara)
{
	CCondField oMinField, oMaxField;
	uint32 nFieldNo = m_pIdxDef->GetFieldNo(m_nCurLevel);
	oMinField.pField = NULL;
	oMaxField.pField = NULL;
	if(!pCondition->GetCondField(nFieldNo, oMinField, oMaxField))
		return RDB_SUCCESS;
	CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue>::CIterator idx = m_oTree.Begin();
	CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue>::CIterator end = m_oTree.End();

	CIndexKeyValue oKey;
	oKey.Create(m_nType, m_nSize, m_pTabDef);
	if(oMinField.pField)
	{
		oKey.SetFromField(oMinField.pField);
		if(oMinField.nOperator == RDB_SQLPARA_OPERATOR_MOREEQUAL)
			idx = m_oTree.LowerBound(oKey);
		else
			idx = m_oTree.UpperBound(oKey);
		if(idx == m_oTree.End())
			return RDB_SUCCESS;
	}
	if(oMaxField.pField)
	{
		oKey.SetFromField(oMaxField.pField);
		if(oMaxField.nOperator == RDB_SQLPARA_OPERATOR_LESSEQUAL)
			end = m_oTree.UpperBound(oKey);
		else
			end = m_oTree.LowerBound(oKey);
		--end;
		if(end == m_oTree.End())
			return RDB_SUCCESS;
	}
	if(end != m_oTree.End())
		++end;
	uint64 nHandle;
	if(m_nCurLevel != m_nTotalLevel-1)
	{
		for(; idx!=end;)
		{
			nHandle = idx.GetValue().nHandle;
			CBaseRbTreeIndex* pSubIndex = m_pIndexPool->QueryObject(nHandle, this);
			++idx;
			uint32 nRet = pSubIndex->QueryHelp(pCondition, oFilter, pResult, pSkipCount, 
				pCount, pMaxCount, bUpdate, OnTravel, pPara);
			m_pIndexPool->ReleaseObject(nHandle);
			if(nRet || (pResult && (pResult->GetRecordCount() == pResult->GetRecordSetSize())) ||
				(pMaxCount && pMaxCount[0] == pCount[0]))
				return nRet;
		}
	}
	else
	{
		CRecord* pRecord = NULL;
		uint32 nFull, nRet = AllocRecord(pResult, pRecord);
		if(nRet)
			return nRet;
		for(; idx!=end;)
		{
			nHandle = idx.GetValue().nHandle;
			++idx;
			nRet = IsCoincident(pRecord, nHandle, pCondition, oFilter, 
				pResult, pSkipCount, pCount, pMaxCount, nFull, bUpdate, OnTravel, pPara);
			if(nRet)
				return nRet;
			if(nFull)
				break;
		}
		if(pRecord)
		{
			if(pResult)
				pResult->PopRecord();
			else
				delete pRecord;
		}
	}
	return RDB_SUCCESS;
}

uint32 CBaseRbTreeIndex::AttachRecord(CRecord* pRecord)
{
	if(m_nCurLevel == 0 && m_pPrimaryIndex)
	{
		m_pFilter->Clear();
		uint32 i, nIdxFieldCount = m_pIdxDef->m_nFieldCount;
		for(i=0; i<nIdxFieldCount; ++i)
			m_pFilter->SetField(m_pIdxDef->m_pFields[i]);
		if(!ExistInPrimaryTable(m_pFilter, pRecord))
			return RDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE;
	}
	return AttachRecordHelp(pRecord);
}

uint32 CBaseRbTreeIndex::AttachRecordHelp(CRecord* pRecord)
{
	CRdbRbTreeItem oItem;
	CIndexKeyValue oKey;
	CField* pField;

	int32 bInMemory = (m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE);

	uint32 nFieldNo = m_pIdxDef->m_pFields[m_nCurLevel];
	pField = pRecord->GetField(nFieldNo);

	oKey.Create(m_nType, m_nSize, m_pTabDef);
	oKey.SetFromField(pField);

	if(m_nCurLevel != m_nTotalLevel-1)
	{
		CBaseRbTreeIndex* pSubIndex;
		CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue>::CIterator idx = m_oTree.Find(oKey);
		if(idx == m_oTree.End())
		{
			CBaseRbTreeIndex oSubIndex(m_pTabDef, m_pIdxDef, m_pTableSpace, 
				m_pDataTable, m_pSpace, m_pIndexPool, m_pFilter, m_nCurLevel+1);

			if(!oSubIndex.CreateObject())
				return RDB_LACK_STORAGE;

			uint64 nNode = m_pSpace->Allocate(40+m_nKeySize, bInMemory);
			if(!nNode)
			{
				oSubIndex.DestroyObject();
				return RDB_LACK_STORAGE;
			}
			oKey.Save(nNode + 40);
			oItem.nHandle = oSubIndex.GetThis();
			idx.SetThis(nNode);
			idx.SetValue(oItem);
			m_oTree.InsertNode(idx.GetNode(), oKey);
		}
		else
			oItem = idx.GetValue();
		pSubIndex =  m_pIndexPool->QueryObject(oItem.nHandle, this);
		uint32 nRet = pSubIndex->AttachRecord(pRecord);
		m_pIndexPool->ReleaseObject(oItem.nHandle);
		return nRet;
	}
	uint64 nNode = m_pSpace->Allocate(40+m_nKeySize, bInMemory);
	if(!nNode)
		return RDB_LACK_STORAGE;
	oKey.Save(nNode + 40);
	oItem.nHandle = pRecord->GetRowId();
	CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue>::CIterator idx = m_oTree.End();
	idx.SetThis(nNode);
	idx.SetValue(oItem);
	if(m_oTree.InsertNode(idx.GetNode(), oKey) == m_oTree.End())
		return RDB_UNIQUE_INDEX_CONFLICT;
	return RDB_SUCCESS;
}

uint32 CBaseRbTreeIndex::DetachRecord(CRecord* pRecord)
{
	CField* pField;
	CIndexKeyValue oKey;

	uint32 nFieldNo = m_pIdxDef->m_pFields[m_nCurLevel];
	pField = pRecord->GetField(nFieldNo);

	oKey.Create(m_nType, m_nSize, m_pTabDef);
	oKey.SetFromField(pField);
	
	CVmmRbTree<CRdbRbTreeItem, CIndexKeyValue>::CIterator idx = m_oTree.Find(oKey);
	if(idx == m_oTree.End())
		return RDB_RECORD_NOT_EXIST;

	uint64 nHandle = idx.GetValue().nHandle;
	if(m_nCurLevel != m_nTotalLevel-1)
	{
		CBaseRbTreeIndex* pSubIndex = m_pIndexPool->QueryObject(nHandle, this);
		uint32 nRet = pSubIndex->DetachRecord(pRecord);
		if(!nRet && !pSubIndex->m_oTree.GetSize())
			m_oTree.Erase(idx);
		m_pIndexPool->ReleaseObject(nHandle);
		return nRet;
	}

	if(nHandle != pRecord->m_nRowId)
		return RDB_RECORD_NOT_EXIST;

	m_oTree.Erase(idx);

	return RDB_SUCCESS;
}

CRdbRbTreeIndex::~CRdbRbTreeIndex()
{
	delete m_pFilter;
}

CRdbRbTreeIndex::CRdbRbTreeIndex(CTableDefine* pTabDef,
								 CIndexDefine* pIdxDef, 
								 CTableSpace* pTableSpace,
								 CDataTable* pDataTable)
:CBaseRbTreeIndex(pTabDef, pIdxDef, pTableSpace, pDataTable, &m_oSpace, &m_oIndexPool, NULL)
{
	m_pFilter = new CSqlFilter(pTabDef);
	m_nThis = 0;
}

bool CRdbRbTreeIndex::CreateObject()
{
	uint64 nHandle[2];
	int32 bInMemory = (m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE);
	m_nThis = vmalloc(16, bInMemory);
	if(!m_nThis)
		return false;
	if(!m_oSpace.CreateObject(bInMemory))
	{
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	nHandle[1] = m_oSpace.GetThis();
	if(!CBaseRbTreeIndex::CreateObject())
	{
		m_oSpace.DestroyObject();
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	nHandle[0] = CBaseRbTreeIndex::GetThis();
	vcommit(m_nThis, (char*)nHandle, 16);
	return true;
}

void CRdbRbTreeIndex::DestroyObject()
{
	if(m_nThis)
	{
		m_oTree.Truncate();
		m_oTree.DestroyObject();
		m_oSpace.DestroyObject();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

CAbstractIndex& CRdbRbTreeIndex::SetThis(uint64 nThis)
{
	m_nThis = nThis;
	if(m_nThis)
	{
		uint64 nHandle[2];
		vquery(nThis, (char*)nHandle, 16);
		m_oSpace.SetThis(nHandle[1]);
		CBaseRbTreeIndex::SetThis(nHandle[0]);
	}
	return *this;
}

uint64 CRdbRbTreeIndex::GetThis()
{
	return m_nThis;
}

//----------------------------------------------------------
// CRdbPrefixIndex
//----------------------------------------------------------
CRdbPrefixIndex::~CRdbPrefixIndex()
{
}

CRdbPrefixIndex::CRdbPrefixIndex(CTableDefine* pTabDef,
										 CIndexDefine* pIdxDef, 
										 CTableSpace* pTableSpace,
										 CDataTable* pDataTable)
:CAbstractIndex(pTabDef, pIdxDef, pTableSpace, pDataTable),
m_oTree(&m_oAllocator, (pIdxDef->m_pBaseAttr->nQualifier&RDB_UNIQUE_INDEX)?1:0),
m_oFilter(pTabDef)
{
}

bool CRdbPrefixIndex::CreateObject()
{
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;

	uint64 pAddr[] = {0, 0};
	m_nThis = vmalloc(16, bMemory);
	if(!m_nThis)
		return false;
	if(!m_oAllocator.CreateObject(bMemory))
	{
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	pAddr[1] = m_oAllocator.GetThis();
	pAddr[0] = 0;
	m_oTree.SetThis(0);
	vcommit(m_nThis, (char*)pAddr, 16);
	return true;
}

void CRdbPrefixIndex::DestroyObject()
{
	if(m_nThis)
	{
		m_oTree.SetThis(0);
		m_oAllocator.DestroyObject();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

CAbstractIndex& CRdbPrefixIndex::SetThis(uint64 nThis)
{
	uint64 pAddr[2];
	m_nThis = nThis;
	if(nThis)
	{
		vquery(nThis, (char*)pAddr, 16);
		m_oAllocator.SetThis(pAddr[1]);
		m_oTree.SetThis(pAddr[0]);
	}
	return *this;
}

uint64 CRdbPrefixIndex::GetThis()
{
	return m_nThis;
}

void CRdbPrefixIndex::Truncate()
{
	if(m_nThis)
	{
		uint64 nAddr = 0;
		vcommit(m_nThis, (char*)&nAddr, 8);
		m_oTree.SetThis(0);
		m_oAllocator.DeAllocateAll();
	}
}

uint32 CRdbPrefixIndex::Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];
	char* sStr = NULL;
	uint32 nStrLen = 0;
	if(!pCondition->GetPrefixCondField(nFieldNo, sStr, &nStrLen) || !sStr || !nStrLen)
		return RDB_SUCCESS;
	return QueryHelp(sStr, nStrLen, pCondition, oFilter, &oResult, &nSkipCount, NULL, NULL);
}

uint32 CRdbPrefixIndex::QueryCount(CSqlParameterSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	m_oFilter.Clear();
	pCondition->GetFilter(m_oFilter);
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];
	char* sStr = NULL;
	uint32 nStrLen = 0;
	if(!pCondition->GetPrefixCondField(nFieldNo, sStr, &nStrLen) || !sStr || !nStrLen)
		return RDB_SUCCESS;
	return QueryHelp(sStr, nStrLen, pCondition, m_oFilter, NULL, NULL, &nCount, NULL);
}

uint32 CRdbPrefixIndex::Exist(CSqlParameterSet * pCondition, uint32& bExist)
{
	bExist = 0;
	uint32 nCount = 0;
	uint32 nMaxCount = 1;
	m_oFilter.Clear();
	pCondition->GetFilter(m_oFilter);
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];
	char* sStr = NULL;
	uint32 nStrLen = 0;
	if(!pCondition->GetPrefixCondField(nFieldNo, sStr, &nStrLen) || !sStr || !nStrLen)
		return RDB_SUCCESS;
	uint32 nRet = QueryHelp(sStr, nStrLen, pCondition, m_oFilter, NULL, NULL, &nCount, &nMaxCount);
	if(nCount)
		bExist = 1;
	return nRet;
}

uint32 CRdbPrefixIndex::Travel(CSqlParameterSet * pCondition, CSqlFilter &oFilter, bool bUpdate, FOnTravelIndex OnTravel, uint32 &nCount, void* pPara)
{
	nCount = 0;
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];
	char* sStr = NULL;
	uint32 nStrLen = 0;
	if(!pCondition->GetPrefixCondField(nFieldNo, sStr, &nStrLen) || !sStr || !nStrLen)
		return RDB_SUCCESS;
	return QueryHelp(sStr, nStrLen, pCondition, oFilter, NULL, NULL, &nCount, NULL, bUpdate, OnTravel, pPara);
}

uint32 CRdbPrefixIndex::QueryHelp(char* sKey, uint32 nStrLen,
								  CSqlParameterSet* pCondition, 
								  CSqlFilter& oFilter, 
								  CRecordSet *pResult, 
								  uint32* pSkipCount, 
								  uint32* pCount, 
								  uint32* pMaxCount,
								  bool bUpdate, 
								  FOnTravelIndex OnTravel,
								  void * pPara)
{
	uint64 nList;
	TNTreeNode* pNode = m_oTree.GetNode((const uint8*)sKey, nStrLen, nList);
	if(pNode == NULL)
		return RDB_SUCCESS;

	CRecord* pRecord = NULL;
	uint32 nRet = AllocRecord(pResult, pRecord);
	if(nRet)
	{
		m_oTree.ReleaseNode(nList, pNode);
		return nRet;
	}

	uint32 nFull;
	while(pNode)
	{
		uint64 nRowId = pNode->oObject, nNext;
		TNTreeNode* pNext = m_oTree.GetNextNode(pNode, nNext);
		m_oTree.ReleaseNode(nList, pNode);
		pNode = pNext;
		nList = nNext;
		nRet = IsCoincident(pRecord, nRowId, pCondition, 
			oFilter, pResult, pSkipCount, pCount, 
			pMaxCount, nFull, bUpdate, OnTravel, pPara);
		if(nRet)
		{
			if(pNode)
				m_oTree.ReleaseNode(nList, pNode);
			return nRet;
		}
		if(nFull)
			break;
	}
	if(pRecord)
	{
		if(pResult)
			pResult->PopRecord();
		else
			delete pRecord;
	}
	if(pNode)
		m_oTree.ReleaseNode(nList, pNode);
	return RDB_SUCCESS;
}

uint32 CRdbPrefixIndex::AttachRecord(CRecord* pRecord)
{
	CField* pField;
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];
	char* sStr = NULL;
	uint32 nStrLen = 0;
	pField = pRecord->GetField(nFieldNo);
	sStr = pField->GetString(&nStrLen);

	bool bConflict;
	int32 bMemory = 1;
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_FILE_TABLE)
		bMemory = 0;

	uint64 nNode;
	uint64 nRowId = pRecord->m_nRowId;
	uint64 nTreeThis = m_oTree.GetThis();
	TNTreeNode* pNode = m_oTree.InsertNode((const uint8*)sStr, nStrLen, nRowId, bMemory, nNode, bConflict);
	if(!pNode)
	{
		if(bConflict)
			return RDB_UNIQUE_INDEX_CONFLICT;
		return RDB_LACK_STORAGE;
	}
	m_oTree.ReleaseNode(nNode, pNode);
	uint64 nTreeThis2 = m_oTree.GetThis();
	if(nTreeThis != nTreeThis2)
	{
		uint64 pAddr[2] = {nTreeThis2, m_oAllocator.GetThis()};
		vcommit(m_nThis, (char*)pAddr, 16);
	}
	return RDB_SUCCESS;
}

uint32 CRdbPrefixIndex::DetachRecord(CRecord* pRecord)
{
	CField* pField;
	uint32 nFieldNo = m_pIdxDef->m_pFields[0];

	char* sStr = NULL;
	uint32 nStrLen = 0;

	pField = pRecord->GetField(nFieldNo);
	sStr = pField->GetString(&nStrLen);

	uint64 nRowId = pRecord->m_nRowId;

	if(m_oTree.RemoveNode((const uint8*)sStr, nStrLen, FOCP_NAME::IsEqualRowId, &nRowId))
	{
		uint64 nTreeThis = m_oTree.GetThis();
		if(!nTreeThis)
		{
			uint64 pAddr[2] = {nTreeThis, m_oAllocator.GetThis()};
			vcommit(m_nThis, (char*)pAddr, 16);
		}
		return RDB_SUCCESS;
	}
	return RDB_RECORD_NOT_EXIST;
}

FOCP_END();
