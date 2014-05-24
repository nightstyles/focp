
#include "MdbIdx.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();
//----------------------------------------------------------
// CMdbAbstractIndex
//----------------------------------------------------------
CMdbAbstractIndex::CMdbAbstractIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex)
{
	m_pTabDef = pTabDef;
	m_pIdxDef = pIdxDef;
	m_pRowIndex = pRowIndex;
	m_pPrimaryIndex = NULL;
	m_nDetailIndexCount = 0;
	m_pDetailIndexes = NULL;
	if(pIdxDef)
		pIdxDef->pExtendAttr->pInstance = this;
}

CMdbAbstractIndex::~CMdbAbstractIndex()
{
	if(m_pDetailIndexes)
		CMalloc::Free(m_pDetailIndexes);
}

void CMdbAbstractIndex::SetPrimaryIndex(CMdbAbstractIndex* pPrimaryIndex)
{
	m_pPrimaryIndex = pPrimaryIndex;
	pPrimaryIndex->m_pDetailIndexes = (CMdbAbstractIndex**)CMalloc::Realloc(pPrimaryIndex->m_pDetailIndexes,
									  (pPrimaryIndex->m_nDetailIndexCount+1)*sizeof(void*));
	pPrimaryIndex->m_pDetailIndexes[pPrimaryIndex->m_nDetailIndexCount] = this;
	++pPrimaryIndex->m_nDetailIndexCount;
}

uint32 CMdbAbstractIndex::AttachRecord(CMdbDataRecord*)
{
	return MDB_SUCCESS;
}

uint32 CMdbAbstractIndex::DetachRecord(CMdbDataRecord*)
{
	return MDB_SUCCESS;
}

CMdbRecord* CMdbAbstractIndex::AllocRecord(CMdbRecordSet *pResult)
{
	CMdbRecord* pRecord = NULL;
	if(!pResult)
		pRecord = new CMdbRecord(m_pTabDef);
	return pRecord;
}

bool CMdbAbstractIndex::ExistInPrimaryTable(CMdbRecord* pRecord)
{
	uint32 bExist;
	CMdbField oSrcField, oDstField;
	CMdbSqlParaSet oCond(m_pPrimaryIndex->m_pTabDef);
	CMdbSqlPara* pPara = (CMdbSqlPara*)oCond.AddPara();
	CMdbRecord* pDstRecord = &pPara->m_oRecord;
	CMdbIndexAttr *pIdxAttr = m_pPrimaryIndex->m_pIdxDef->pExtendAttr;
	uint32 nCount = pIdxAttr->nFieldCount;
	uint32* pDstFields = pIdxAttr->pFields;
	uint32* pSrcFields = m_pIdxDef->pExtendAttr->pFields;
	for(uint32 i=0; i<nCount; ++i)
	{
		uint32 nDstFieldNo = pDstFields[i];
		uint32 nSrcFieldNo = pSrcFields[i];
		pDstRecord->GetField(nDstFieldNo, oDstField);
		pRecord->GetField(nSrcFieldNo, oSrcField);
		oDstField.SetFromField(&oSrcField);
		pPara->SetField(nDstFieldNo, MDB_SQLPARA_OPERATOR_EQUAL);
	}
	return(!m_pPrimaryIndex->Exist(&oCond, bExist) && bExist);
}

void CMdbAbstractIndex::Truncate()
{
	Clear();
}

uint32 CMdbAbstractIndex::IsCoincident(
	CMdbDataRecord* pDataRec,
	CMdbSqlParaSet* pCondition,
	CMdbSqlFilter& oFilter,
	CMdbRecordSet *pResult,
	uint32* pSkipCount,
	uint32* pCount,
	uint32* pMaxCount,
	uint32& nFull,
	bool &bCoincident,
	bool &bDeleted,
	FOnTravelIndex OnTravel,
	void* pTravelPara)
{
	nFull = 0;
	bDeleted = false;
	CMdbRecord oRecord(m_pTabDef, pDataRec->pRecord);
	bCoincident = pCondition->IsCoincident(oRecord);
	if(bCoincident)
	{
		if(pSkipCount && pSkipCount[0])
		{
			--pSkipCount[0];
			return MDB_SUCCESS;
		}
		if(OnTravel && OnTravel(this, pDataRec, pTravelPara, bDeleted))
			return MDB_TRAVEL_FAILURE;
		if(pResult)
		{
			CMdbRecord* pRecord = pResult->AllocRecord();
			pRecord->CopyFrom(oFilter, &oRecord);
			if(pResult->GetRecordCount() == pResult->GetRecordSetSize())
				nFull = 1;
		}
		if(pCount)
		{
			++pCount[0];
			if(pMaxCount && (pCount[0] == pMaxCount[0]))
				nFull = 1;
		}
	}
	return MDB_SUCCESS;
}

//----------------------------------------------------------
// CMdbRowIndex
//----------------------------------------------------------
CMdbRowIndex::CMdbRowIndex(CMdbTableDef* pTabDef):
	CMdbAbstractIndex(pTabDef, NULL, this)
{
	m_nRecordCount = 0;
	m_pHead = NULL;
	m_pTail = NULL;
	m_bWriteLock = false;
}

CMdbRowIndex::~CMdbRowIndex()
{
	Clear();
}

void CMdbRowIndex::Clear()
{
	while(m_pHead)
	{
		m_pTail = m_pHead->pNext;
		delete[] (uint8*)(m_pHead);
		m_pHead = m_pTail;
	}
	m_nRecordCount = 0;
}

CMdbDataRecord* CMdbRowIndex::NewDataRecord()
{
	uint32 nSize = m_pTabDef->pExtendAttr->nRecordSize + 2*sizeof(void*);
	uint8* pRecord = new uint8[nSize];
	CBinary::MemorySet(pRecord, 0, nSize);
	return (CMdbDataRecord*)pRecord;
}

void CMdbRowIndex::DelDataRecord(CMdbDataRecord* pRec)
{
	CMdbRecord oRecord(m_pTabDef, pRec->pRecord);
	oRecord.Clear();
	delete[] (uint8*)pRec;
}

CMdbDataRecord* CMdbRowIndex::CloneRecord(CMdbDataRecord* pOld)
{
	CMdbDataRecord* pNew = NewDataRecord();
	CMdbRecord oOld(m_pTabDef, pOld->pRecord), oNew(m_pTabDef, pNew->pRecord);
	CMdbSqlFilter oFilter(m_pTabDef);
	oNew.CopyFrom(oFilter, &oOld);
	return pNew;
}

void CMdbRowIndex::LockRecord(CMdbDataRecord* pDataRecord)
{
	if(!m_bWriteLock)
	{
		ulong nId = (ulong)pDataRecord;
		ulong nLockNum = sizeof(m_oRecordLock)/sizeof(CMutex);
		m_oRecordLock[nId % nLockNum].Enter();
	}
}

void CMdbRowIndex::UnLockRecord(CMdbDataRecord* pDataRecord)
{
	if(!m_bWriteLock)
	{
		ulong nId = (ulong)pDataRecord;
		ulong nLockNum = sizeof(m_oRecordLock)/sizeof(CMutex);
		m_oRecordLock[nId % nLockNum].Leave();
	}
}

void CMdbRowIndex::LockTable(bool bRead)
{
	if(bRead)
		m_oTableLock.EnterRead();
	else
	{
		m_oTableLock.EnterWrite();
		m_bWriteLock = true;
	}
}

void CMdbRowIndex::UnLockTable(bool bRead)
{
	if(bRead)
		m_oTableLock.LeaveRead();
	else
	{
		m_bWriteLock = false;
		m_oTableLock.LeaveWrite();
	}
}

uint32 CMdbRowIndex::AttachRecord(CMdbDataRecord* pDataRecord)
{
	pDataRecord->pNext = NULL;
	pDataRecord->pPrev = m_pTail;
	if(m_pTail)
		m_pTail->pNext = pDataRecord;
	else
		m_pHead = pDataRecord;
	m_pTail = pDataRecord;
	++m_nRecordCount;
	return MDB_SUCCESS;
}

uint32 CMdbRowIndex::DetachRecord(CMdbDataRecord* pDataRecord)
{
	--m_nRecordCount;
	CMdbDataRecord* pPrev = pDataRecord->pPrev;
	CMdbDataRecord* pNext = pDataRecord->pNext;
	if(pPrev)
		pPrev->pNext = pNext;
	else
		m_pHead = pNext;
	if(pNext)
		pNext->pPrev = pPrev;
	else
		m_pTail = pPrev;
	return MDB_SUCCESS;
}

CMdbDataRecord* CMdbRowIndex::GetFirst()
{
	CMdbDataRecord* pRec = m_pHead;
	if(pRec)
		LockRecord(pRec);
	return pRec;
}

CMdbDataRecord* CMdbRowIndex::GetNext(CMdbDataRecord* pRec, bool Unlock)
{
	CMdbDataRecord* pNext = pRec->pNext;
	if(Unlock)
		UnLockRecord(pRec);
	if(pNext)
		LockRecord(pNext);
	return pNext;
}

uint32 CMdbRowIndex::Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	for(CMdbDataRecord* pHead = GetFirst(); pHead; pHead=GetNext(pHead))
	{
		CMdbRecord oRecord(m_pTabDef, pHead->pRecord);
		if(pCondition->IsCoincident(oRecord))
		{
			if(nSkipCount)
			{
				--nSkipCount;
				continue;
			}
			else
			{
				CMdbRecord* pRecord = oResult.AllocRecord();
				pRecord->CopyFrom(oFilter, &oRecord);
				if(oResult.GetRecordCount() == oResult.GetRecordSetSize())
				{
					UnLockRecord(pHead);
					break;
				}
			}
		}
	}
	return MDB_SUCCESS;
}

uint32 CMdbRowIndex::Query(CMdbSqlParaSet * pCondition, uint32 &nCount)
{
	if(pCondition->m_nParaCount == 0)
		nCount = m_nRecordCount;
	else
	{
		nCount = 0;
		for(CMdbDataRecord* pHead = GetFirst(); pHead; pHead=GetNext(pHead))
		{
			CMdbRecord oRecord(m_pTabDef, pHead->pRecord);
			if(pCondition->IsCoincident(oRecord))
				++nCount;
		}
	}
	return MDB_SUCCESS;
}

uint32 CMdbRowIndex::Exist(CMdbSqlParaSet * pCondition, uint32& bExist)
{
	bExist = 0;
	if(pCondition->m_nParaCount == 0)
	{
		if(m_nRecordCount)
			bExist = 1;
	}
	else for(CMdbDataRecord* pHead = GetFirst(); pHead; pHead=GetNext(pHead))
		{
			CMdbRecord oRecord(m_pTabDef, pHead->pRecord);
			if(pCondition->IsCoincident(oRecord))
			{
				bExist = 1;
				UnLockRecord(pHead);
				break;
			}
		}
	return MDB_SUCCESS;
}

uint32 CMdbRowIndex::Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
							FOnTravelIndex OnTravel, uint32& nCount, void* pPara)
{
	nCount = 0;
	CMdbDataRecord* pHead = GetFirst();
	while(pHead)
	{
		CMdbRecord oRecord(m_pTabDef, pHead->pRecord);
		if(pCondition->IsCoincident(oRecord))
		{
			bool bDeleted;
			CMdbDataRecord* pIdx = GetNext(pHead, false);
			if(OnTravel(this, pHead, pPara, bDeleted))
			{
				if(pIdx)
					UnLockRecord(pIdx);
				return MDB_TRAVEL_FAILURE;
			}
			if(!bDeleted)
				UnLockRecord(pHead);
			pHead = pIdx;
			++nCount;
		}
		else
			pHead = GetNext(pHead);
	}
	return MDB_SUCCESS;
}

//----------------------------------------------------------
// CMdbHashIndex
//----------------------------------------------------------
CMdbHashIndex::CMdbHashIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex):
	CMdbAbstractIndex(pTabDef, pIdxDef, pRowIndex),
	m_oHash((pIdxDef->nQualifier&MDB_UNIQUE_INDEX)?1:0, pIdxDef->nHashRate/1000.0)
{
}

CMdbHashIndex::~CMdbHashIndex()
{
}

void CMdbHashIndex::Clear()
{
	m_oHash.Clear();
}

uint32 CMdbHashIndex::Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, &oResult, &nSkipCount, NULL, NULL);
}

uint32 CMdbHashIndex::Query(CMdbSqlParaSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL);
}

uint32 CMdbHashIndex::Exist(CMdbSqlParaSet * pCondition, uint32& bExist)
{
	bExist = 0;
	uint32 nCount = 0;
	uint32 nMaxCount = 1;
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	uint32 nRet = QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, &nMaxCount);
	if(nCount)
		bExist = 1;
	return nRet;
}

uint32 CMdbHashIndex::Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
							 FOnTravelIndex OnTravel, uint32& nCount, void* pPara)
{
	nCount = 0;
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL, OnTravel, pPara);
}

uint32 CMdbHashIndex::GetHashValue(CMdbSqlParaSet* pCondition, uint32 &nHashValue)
{
	uint32 nIdxFieldCount = m_pIdxDef->pExtendAttr->nFieldCount;
	InitCrc32(&nHashValue);
	for(uint32 i=0; i<nIdxFieldCount; ++i)
	{
		uint32 nTmp, nHashCount = 0;
		uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[i];
		pCondition->GetHashValue(nFieldNo, nTmp, nHashCount);
		if(nHashCount != 1)
			return MDB_INVALID_COND;
		ComputeCrc32(&nHashValue, (const uint8*)&nTmp, sizeof(uint32), 1);
	}
	EndCrc32(&nHashValue);
	return MDB_SUCCESS;
}

uint32 CMdbHashIndex::QueryHelp(CMdbSqlParaSet* pCondition,
								CMdbSqlFilter& oFilter,
								CMdbRecordSet *pResult,
								uint32* pSkipCount,
								uint32* pCount,
								uint32* pMaxCount,
								FOnTravelIndex OnTravel,
								void* pPara)
{
	uint32 nHashValue;

	if(GetHashValue(pCondition, nHashValue))
		return MDB_INVALID_COND;

	CFreeHash<CMdbDataRecord*>::CHashNode* pNode = m_oHash.GetNode(nHashValue);
	if(pNode == NULL)
		return MDB_SUCCESS;

	uint32 nFull;
	uint32 nRet = MDB_SUCCESS;
	while(pNode)
	{
		CFreeHash<CMdbDataRecord*>::CHashNode* pNext = pNode->pNextNode;
		if(pNode->nHashValue == nHashValue)
		{
			bool bCoincident, bDeleted;
			CMdbDataRecord* pDataRec = pNode->oObject;
			m_pRowIndex->LockRecord(pDataRec);
			nRet = IsCoincident(pDataRec, pCondition,
								oFilter, pResult, pSkipCount, pCount,
								pMaxCount, nFull, bCoincident, bDeleted, OnTravel, pPara);
			if(!bDeleted)
				m_pRowIndex->UnLockRecord(pDataRec);
			if(nRet)
				break;
			if(nFull || (bCoincident && m_oHash.IsUnique()))
				break;
		}
		pNode = pNext;
	}

	return nRet;
}

bool CMdbHashIndex::RecordIsEqual(CMdbDataRecord* a, CMdbDataRecord* b)
{
	CMdbField oAField, oBField;
	CMdbRecord oARecord(m_pTabDef, a->pRecord);
	CMdbRecord oBRecord(m_pTabDef, b->pRecord);
	uint32 i, nIdxFieldCount = m_pIdxDef->pExtendAttr->nFieldCount;
	for(i=0; i<nIdxFieldCount; ++i)
	{
		uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[i];
		oARecord.GetField(nFieldNo, oAField);
		oBRecord.GetField(nFieldNo, oBField);
		if(oAField.Compare(&oBField))
			return false;
	}
	return true;
}

static bool MdbHashRecordIsEqual(CMdbDataRecord* const &a, CMdbDataRecord* const &b, void* pPara)
{
	CMdbHashIndex* pIndex = (CMdbHashIndex*)pPara;
	return pIndex->RecordIsEqual((CMdbDataRecord*)a, (CMdbDataRecord*)b);
}

uint32 CMdbHashIndex::GetHashValue(CMdbRecord &oRecord)
{
	uint32 nHashValue;
	CMdbField oField;
	uint32 nIdxFieldCount = m_pIdxDef->pExtendAttr->nFieldCount;
	InitCrc32(&nHashValue);
	for(uint32 i=0; i<nIdxFieldCount; ++i)
	{
		uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[i];
		oRecord.GetField(nFieldNo, oField);
		uint32 nTmp = oField.GetHashValue();
		ComputeCrc32(&nHashValue, (const uint8*)&nTmp, sizeof(uint32), 1);
	}
	EndCrc32(&nHashValue);
	return nHashValue;
}

uint32 CMdbHashIndex::AttachRecord(CMdbDataRecord* pDataRecord)
{
	bool bConflict;
	CMdbRecord oRecord(m_pTabDef, pDataRecord->pRecord);
	uint32 nHashValue = GetHashValue(oRecord);

	if(m_pPrimaryIndex && !ExistInPrimaryTable(&oRecord))
		return MDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE;

	CFreeHash<CMdbDataRecord*>::CHashNode* pNode = m_oHash.InsertNode(nHashValue, pDataRecord,
			FOCP_NAME::MdbHashRecordIsEqual, this, bConflict);

	if(pNode == NULL)
		return MDB_UNIQUE_INDEX_CONFLICT;

	return MDB_SUCCESS;
}

static bool MdbHashRecordIsEqual2(void* pCond, CMdbDataRecord* const &pRecord)
{
	return pRecord == (CMdbDataRecord*)pCond;
}

uint32 CMdbHashIndex::DetachRecord(CMdbDataRecord* pDataRecord)
{
	CMdbRecord oRecord(m_pTabDef, pDataRecord->pRecord);
	uint32 nHashValue = GetHashValue(oRecord);

	if(m_oHash.RemoveNode(nHashValue, FOCP_NAME::MdbHashRecordIsEqual2, pDataRecord))
		return MDB_SUCCESS;

	return MDB_RECORD_NOT_EXIST;
}

//----------------------------------------------------------
// CMdbRbTreeIndex
//----------------------------------------------------------
CMdbRbTreeIndexField::~CMdbRbTreeIndexField()
{
	if(m_pFlag)
	{
		delete[] (uint8*)m_pFlag;
		m_pFlag = NULL;
	}
}

CMdbRbTreeIndexField::CMdbRbTreeIndexField(uint32 nType, uint32 nSize)
{
	uint32 nRecSize;
	if(nType == MDB_VARCHAR_FIELD || nType == MDB_VARLCHAR_FIELD || nType == MDB_VARCHAR_FIELD)
		nRecSize = sizeof(void*) + sizeof(uint32);
	else
		nRecSize = nSize + sizeof(uint32);
	uint8* pRecordData = new uint8[nRecSize];
	Initialize(nType, nSize, pRecordData, 0, sizeof(uint32));
	*(uint32*)pRecordData = 0;
}

int32 CMdbRbCmpKey::Compare(CMdbRbTreeIndexField* const * pLeft, CMdbRbTreeIndexField* const * pRight)
{
	return ((CMdbRbTreeIndexField*)*pLeft)->Compare((CMdbRbTreeIndexField*)*pRight);
}

CMdbRbTreeIndex::CMdbRbTreeIndex(CMdbTableDef* pTabDef, CMdbIndexDef* pIdxDef, CMdbRowIndex* pRowIndex, uint32 nLevel):
	CMdbAbstractIndex(pTabDef, pIdxDef, pRowIndex)//,m_oTree((nLevel<(pIdxDef->pExtendAttr->nFieldCount-1))||(pIdxDef->nQualifier&MDB_UNIQUE_INDEX))
{
	m_nCurLevel = nLevel;
	uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[m_nCurLevel];
	CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + nFieldNo;
	m_nType = pFieldDef->nType;
	m_nSize = pFieldDef->nLen;//数据大小
}

CMdbRbTreeIndex::~CMdbRbTreeIndex()
{
	Clear();
}

void CMdbRbTreeIndex::Clear()
{
	CRbTreeNode* pIt = m_oTree.First();
	CRbTreeNode* pEnd = m_oTree.End();
	if(m_nCurLevel < m_pIdxDef->pExtendAttr->nFieldCount-1)
	{
		for(; pIt!=pEnd; pIt=m_oTree.GetNext(pIt))
		{
			CMdbRbTreeIndex* pIndex = (CMdbRbTreeIndex*)m_oTree.GetItem(pIt);
			CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(pIt);
			delete pIndex;
			delete pKey;
		}
	}
	else if(m_pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
	{
		for(; pIt!=pEnd; pIt=m_oTree.GetNext(pIt))
		{
			CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(pIt);
			delete pKey;
		}
	}
	else
	{
		for(; pIt!=pEnd; pIt=m_oTree.GetNext(pIt))
		{
			CDoubleList<CMdbDataRecord*>* pList = (CDoubleList<CMdbDataRecord*>*)m_oTree.GetItem(pIt);
			CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(pIt);
			delete pList;
			delete pKey;
		}
	}
	m_oTree.Clear();
}

uint32 CMdbRbTreeIndex::Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, bool bAsc)
{
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	else
		pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, &oResult, &nSkipCount, NULL, NULL, bAsc);
}

uint32 CMdbRbTreeIndex::Query(CMdbSqlParaSet * pCondition, uint32 &nCount)
{
	nCount = 0;
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL, true);
}

uint32 CMdbRbTreeIndex::Exist(CMdbSqlParaSet * pCondition, uint32& bExist)
{
	bExist = 0;
	uint32 nCount = 0;
	uint32 nMaxCount = 1;
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	uint32 nRet = QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, &nMaxCount, true);
	if(nCount)
		bExist = 1;
	return nRet;
}

uint32 CMdbRbTreeIndex::Travel(CMdbSqlParaSet * pCondition, CMdbSqlFilter &oFilter,
							   FOnTravelIndex OnTravel, uint32& nCount, void* pPara)
{
	nCount = 0;
	return QueryHelp(pCondition, oFilter, NULL, NULL, &nCount, NULL, true, OnTravel, pPara);
}

uint32 CMdbRbTreeIndex::QueryHelp(CMdbSqlParaSet* pCondition,
								  CMdbSqlFilter& oFilter,
								  CMdbRecordSet *pResult,
								  uint32* pSkipCount,
								  uint32* pCount,
								  uint32* pMaxCount,
								  bool bAsc,
								  FOnTravelIndex OnTravel,
								  void* pPara)
{
	CRbTreeNode* idx = m_oTree.First();
	CRbTreeNode* end = m_oTree.End();

	if( idx == end)
		return MDB_SUCCESS;

	CMdbCondField oMinField, oMaxField;
	uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[m_nCurLevel];
	oMinField.pField = NULL;
	oMaxField.pField = NULL;

	if(!pCondition->GetCondField(nFieldNo, oMinField, oMaxField))
		return MDB_SUCCESS;

	CMdbRbTreeIndexField oKey(m_nType, m_nSize);
	if(oMinField.pField)
	{
		oKey.SetFromField(oMinField.pField);
		if(oMinField.nOperator == MDB_SQLPARA_OPERATOR_MOREEQUAL)
			idx = m_oTree.LowerBound(&oKey);
		else
			idx = m_oTree.UpperBound(&oKey);
		if(idx == end)
			return MDB_SUCCESS;
	}

	if(oMaxField.pField)
	{
		oKey.SetFromField(oMaxField.pField);
		if(oMaxField.nOperator == MDB_SQLPARA_OPERATOR_LESSEQUAL)
			end = m_oTree.UpperBound(&oKey);
		else
			end = m_oTree.LowerBound(&oKey);
	}

	uint32 nRet = MDB_SUCCESS;

	if(m_nCurLevel<(m_pIdxDef->pExtendAttr->nFieldCount-1))
	{
		if(bAsc)
		{
			while(idx!=end)
			{
				CMdbRbTreeIndex* pSubIdx = (CMdbRbTreeIndex*)m_oTree.GetItem(idx);
				idx = m_oTree.GetNext(idx);
				nRet = pSubIdx->QueryHelp(pCondition, oFilter, pResult, pSkipCount,
										  pCount, pMaxCount, bAsc, OnTravel, pPara);
				if(nRet || (pResult && (pResult->GetRecordCount() == pResult->GetRecordSetSize())) ||
						(pMaxCount && pMaxCount[0] == pCount[0]))
					return nRet;
			}
		}
		else
		{
			CRbTreeNode * pIt = m_oTree.GetPrev(end), *pPrev;
			while(true)
			{
				CMdbRbTreeIndex* pSubIdx = (CMdbRbTreeIndex*)m_oTree.GetItem(pIt);
				pPrev = m_oTree.GetPrev(pIt);
				nRet = pSubIdx->QueryHelp(pCondition, oFilter, pResult, pSkipCount,
										  pCount, pMaxCount, bAsc, OnTravel, pPara);
				if(nRet || (pResult && (pResult->GetRecordCount() == pResult->GetRecordSetSize())) ||
						(pMaxCount && pMaxCount[0] == pCount[0]))
					return nRet;
				if(pIt == idx)
					break;
				pIt = pPrev;
			}
		}
	}
	else if(m_pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
	{
		uint32 nFull;
		if(bAsc)
		{
			while(idx!=end)
			{
				bool bCoincident, bDeleted;
				CMdbDataRecord* pDataRecord = m_oTree.GetItem(idx);
				idx = m_oTree.GetNext(idx);
				m_pRowIndex->LockRecord(pDataRecord);
				nRet = IsCoincident(pDataRecord, pCondition, oFilter,
									pResult, pSkipCount, pCount, pMaxCount, nFull, bCoincident, bDeleted, OnTravel, pPara);
				if(!bDeleted)
					m_pRowIndex->UnLockRecord(pDataRecord);
				if(nRet || nFull)
					break;
			}
		}
		else
		{
			CRbTreeNode * pIt = m_oTree.GetPrev(end), *pPrev;
			while(true)
			{
				bool bCoincident, bDeleted;
				CMdbDataRecord* pDataRecord = m_oTree.GetItem(pIt);
				pPrev = m_oTree.GetPrev(pIt);
				m_pRowIndex->LockRecord(pDataRecord);
				nRet = IsCoincident(pDataRecord, pCondition, oFilter,
									pResult, pSkipCount, pCount, pMaxCount, nFull, bCoincident, bDeleted, OnTravel, pPara);
				if(!bDeleted)
					m_pRowIndex->UnLockRecord(pDataRecord);
				if(nRet || nFull)
					break;
				if(pIt == idx)
					break;
				pIt = pPrev;
			}
		}
	}
	else
	{
		uint32 nFull;
		if(bAsc)
		{
			while(idx!=end)
			{
				CDoubleList<CMdbDataRecord*> *pList = (CDoubleList<CMdbDataRecord*>*)m_oTree.GetItem(idx);
				idx = m_oTree.GetNext(idx);
				void* pIdx = pList->First();
				while(pIdx)
				{
					bool bCoincident, bDeleted;
					CMdbDataRecord* pDataRecord = pList->GetItem(pIdx);
					pIdx = pList->GetNext(pIdx);
					m_pRowIndex->LockRecord(pDataRecord);
					nRet = IsCoincident(pDataRecord, pCondition, oFilter,
										pResult, pSkipCount, pCount, pMaxCount, nFull, bCoincident, bDeleted, OnTravel, pPara);
					if(!bDeleted)
						m_pRowIndex->UnLockRecord(pDataRecord);
					if(nRet || nFull)
						break;
				}
				if(nRet || nFull)
					break;
			}
		}
		else
		{
			CRbTreeNode * pIt = m_oTree.GetPrev(end), *pPrev;
			while(true)
			{
				CDoubleList<CMdbDataRecord*> *pList = (CDoubleList<CMdbDataRecord*>*)m_oTree.GetItem(idx);
				pPrev = m_oTree.GetPrev(pIt);
				void* pIdx = pList->Last();
				while(pIdx)
				{
					bool bCoincident, bDeleted;
					CMdbDataRecord* pDataRecord = pList->GetItem(pIdx);
					pIdx = pList->GetPrev(pIdx);
					m_pRowIndex->LockRecord(pDataRecord);
					nRet = IsCoincident(pDataRecord, pCondition, oFilter,
										pResult, pSkipCount, pCount, pMaxCount, nFull, bCoincident, bDeleted, OnTravel, pPara);
					if(!bDeleted)
						m_pRowIndex->UnLockRecord(pDataRecord);
					if(nRet || nFull)
						break;
				}
				if(nRet || nFull)
					break;
				if(pIt == idx)
					break;
				pIt = pPrev;
			}
		}
	}
	return nRet;
}

uint32 CMdbRbTreeIndex::AttachRecord(CMdbDataRecord* pRecord)
{
	if(m_nCurLevel == 0 && m_pPrimaryIndex)
	{
		CMdbRecord oRecord(m_pTabDef, pRecord->pRecord);
		if(!ExistInPrimaryTable(&oRecord))
			return MDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE;
	}
	return AttachRecordHelp(pRecord);
}

uint32 CMdbRbTreeIndex::AttachRecordHelp(CMdbDataRecord* pRecord)
{
	CMdbField oField;
	CMdbRecord oRecord(m_pTabDef, pRecord->pRecord);
	uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[m_nCurLevel];
	oRecord.GetField(nFieldNo, oField);
	CMdbRbTreeIndexField *pKey = new CMdbRbTreeIndexField(m_nType, m_nSize);
	pKey->SetFromField(&oField);
	if(m_nCurLevel<(m_pIdxDef->pExtendAttr->nFieldCount-1))
	{
		CMdbRbTreeIndex* pSubIndex;
		CRbTreeNode* idx = m_oTree.Find(pKey);
		CRbTreeNode* end = m_oTree.End();
		if(idx == end)
		{
			pSubIndex = new CMdbRbTreeIndex(m_pTabDef, m_pIdxDef, m_pRowIndex, m_nCurLevel+1);
			m_oTree[pKey] = (CMdbDataRecord*)pSubIndex;
		}
		else
		{
			delete pKey;
			pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(idx);
			pSubIndex = (CMdbRbTreeIndex*)m_oTree.GetItem(idx);
		}
		uint32 nRet = pSubIndex->AttachRecordHelp(pRecord);
		if(nRet && idx == end)
		{
			m_oTree.Remove(pKey);
			delete pSubIndex;
			delete pKey;
		}
		return nRet;
	}
	else if(m_pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
	{
		CRbTreeNode* pNode = m_oTree.Insert(pKey, pRecord);
		if(pNode == m_oTree.End())
		{
			delete pKey;
			return MDB_UNIQUE_INDEX_CONFLICT;
		}
	}
	else
	{
		CDoubleList<CMdbDataRecord*> *pList;
		CRbTreeNode* idx = m_oTree.Find(pKey);
		CRbTreeNode* end = m_oTree.End();
		if(idx == end)
		{
			pList = new CDoubleList<CMdbDataRecord*>;
			m_oTree[pKey] = (CMdbDataRecord*)pList;
		}
		else
		{
			delete pKey;
			pList = (CDoubleList<CMdbDataRecord*>*)m_oTree.GetItem(idx);
		}
		pList->Push(pRecord);
	}

	return MDB_SUCCESS;
}

uint32 CMdbRbTreeIndex::DetachRecord(CMdbDataRecord* pDataRecord)
{
	CMdbField oField;
	CMdbRecord oRecord(m_pTabDef, pDataRecord->pRecord);
	uint32 nFieldNo = m_pIdxDef->pExtendAttr->pFields[m_nCurLevel];
	oRecord.GetField(nFieldNo, oField);
	CMdbRbTreeIndexField oKey(m_nType, m_nSize);
	oKey.SetFromField(&oField);

	CRbTreeNode* idx = m_oTree.Find(&oKey);
	CRbTreeNode* end = m_oTree.End();
	if(idx == end)
		return MDB_RECORD_NOT_EXIST;

	if(m_nCurLevel<(m_pIdxDef->pExtendAttr->nFieldCount-1))
	{
		CMdbRbTreeIndex* pSubIndex = (CMdbRbTreeIndex*)m_oTree.GetItem(idx);
		uint32 nRet = pSubIndex->DetachRecord(pDataRecord);
		if(!pSubIndex->m_oTree.GetSize())
		{
			CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(idx);
			delete pKey;
			delete pSubIndex;
			m_oTree.Remove(idx);
		}
		return nRet;
	}
	else if(m_pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
	{
		CMdbDataRecord* pRecord = m_oTree.GetItem(idx);
		if(pDataRecord == pRecord)
		{
			CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(idx);
			delete pKey;
			m_oTree.Remove(idx);
			return MDB_SUCCESS;
		}
	}
	else
	{
		CDoubleList<CMdbDataRecord*> *pList = (CDoubleList<CMdbDataRecord*>*)m_oTree.GetItem(idx);
		void* pIt = pList->First();
		while(pIt)
		{
			CMdbDataRecord* pRecord = pList->GetItem(pIt);
			if(pDataRecord == pRecord)
			{
				pList->Remove(pIt);
				if(!pList->GetSize())
				{
					CMdbRbTreeIndexField* pKey = (CMdbRbTreeIndexField*)m_oTree.GetKey(idx);
					delete pKey;
					delete pList;
					m_oTree.Remove(idx);
					return MDB_SUCCESS;
				}
			}
			pIt = pList->GetNext(pIt);
		}
	}
	return MDB_RECORD_NOT_EXIST;
}

FOCP_END();
