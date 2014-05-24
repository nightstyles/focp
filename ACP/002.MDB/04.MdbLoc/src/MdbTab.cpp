
#include "MdbTab.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();

//-----------------------------------------------------
// CMdbTable
//-----------------------------------------------------
CMdbTable::CMdbTable(CMdbTableDef* pTabDef)
	:m_oRowIndex(pTabDef)
{
	m_pTabDef = pTabDef;
	m_pIndexTable = NULL;
	m_nIndexCount = 0;
	m_pIndexTable = NULL;
	pTabDef->pExtendAttr->pInstance = this;
}

CMdbTable::~CMdbTable()
{
	if(m_pIndexTable)
		CMalloc::Free(m_pIndexTable);
}

void CMdbTable::AddIndex(CMdbAbstractIndex* pIndex)
{
	m_pIndexTable = (CMdbAbstractIndex**)CMalloc::Realloc(m_pIndexTable, (m_nIndexCount+1)*sizeof(void*));
	m_pIndexTable[m_nIndexCount] = pIndex;
	++m_nIndexCount;
}

uint32 CMdbTable::Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, CMdbAbstractIndex* pIndex, bool bAsc)
{
	uint32 nRet = 0;
	uint32 nNewSetSize = 0;
	uint32 nCondCount = pCondition->GetParaCount();
	if(nCondCount)
	{
		uint32 i;
		if(pIndex && oFilter.GetParaCount())
		{
			CMdbIndexDef* pIdxDef = pIndex->m_pIdxDef;
			uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
			for(uint32 i=0; i<nFieldCount; ++i)
				oFilter.SetField(pIdxDef->pExtendAttr->pFields[i]);
		}
		for(i=0; i<nCondCount; ++i)
		{
			bool bParaSet;
			CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
			if(pCond->IsEmpty())
				continue;
			++nNewSetSize;
		}
		if(nNewSetSize)
		{
			uint32 *pSegument = NULL;
			uint32 nTimes = 0, nRecCount = 0;
			uint32 nRet3 = 0, nRet4 = 1, nNewSetSize = 0;
			uint32 nOldSetSize = oResult.GetRecordSetSize();
			if(nNewSetSize > 1 && pIndex)
			{
				pSegument = new uint32[nCondCount+1];
				oResult.SetRecordSetSize(nNewSetSize*nOldSetSize);
			}
			m_oRowIndex.LockTable(true);
			for(i=0; i<nCondCount; ++i)
			{
				bool bParaSet;
				CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
				if(pCond->IsEmpty())
					continue;
				CMdbAbstractIndex* pIdx = pIndex;
				if(!pIdx)
					pIdx = IndexChoice(pCond);
				uint32 nRet2 = pIdx->Query(pCond, oFilter, oResult, nSkipCount, bAsc);
				if(nRet2)
					nRet3 = nRet2;
				else
				{
					nRet4 = 0;
					uint32 nCount = oResult.GetRecordCount();
					if(nCount > nRecCount)
					{
						if(pIndex)
							pSegument[nTimes++] = nRecCount;
						nRecCount = nCount;
						if(nCount == oResult.GetRecordSetSize())
							break;//满了
					}
				}
			}
			m_oRowIndex.UnLockTable(true);
			if(nRet4)
				nRet = nRet3;
			if(pIndex)
			{
				if(nTimes > 1)
				{
					pSegument[nTimes] = nRecCount;
					uint32 nFieldCount = pIndex->m_pIdxDef->pExtendAttr->nFieldCount;
					uint32* pFields = pIndex->m_pIdxDef->pExtendAttr->pFields;
					oResult.Sort(bAsc, nFieldCount, pFields, nTimes, pSegument);
				}
				if(nNewSetSize > 1)
				{
					uint32 nCount = oResult.GetRecordCount();
					oResult.SetRecordSetSize(nOldSetSize);
					oResult.m_nRecordCount = nCount;
					delete[] pSegument;
				}
			}
		}
	}
	if(nNewSetSize == 0)
	{
		pCondition->Clear();
		CMdbAbstractIndex* pIdx = pIndex;
		if(!pIdx)
			pIdx = IndexChoice(pCondition);
		m_oRowIndex.LockTable(true);
		nRet = pIdx->Query(pCondition, oFilter, oResult, nSkipCount, bAsc);
		m_oRowIndex.UnLockTable(true);
	}
	return nRet;
}

uint32 CMdbTable::Query(CMdbSqlParaSet * pCondition, uint32 &nCount)
{
	uint32 nRet = 0;
	bool bDone = false;
	uint32 nCondCount = pCondition->GetParaCount();
	if(nCondCount)
	{
		nCount = 0;
		uint32 nRet3 = 0, nRet4 = 1;
		for(uint32 i=0; i<nCondCount; ++i)
		{
			bool bParaSet;
			uint32 nCount2 = 0;
			CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
			if(pCond->IsEmpty())
				continue;
			bDone = true;
			CMdbAbstractIndex* pIndex = IndexChoice(pCond);
			m_oRowIndex.LockTable(true);
			uint32 nRet2 = pIndex->Query(pCond, nCount2);
			m_oRowIndex.UnLockTable(true);
			if(nRet2)
				nRet3 = nRet2;
			else
			{
				nRet4 = 0;
				nCount += nCount2;
			}
		}
		if(nRet4)
			nRet = nRet3;
	}
	if(!bDone)
	{
		pCondition->Clear();
		CMdbAbstractIndex* pIndex = IndexChoice(pCondition);
		m_oRowIndex.LockTable(true);
		nRet = pIndex->Query(pCondition, nCount);
		m_oRowIndex.UnLockTable(true);
	}
	return nRet;
}

uint32 CMdbTable::Exist(CMdbSqlParaSet * pCondition, uint32& bExist)
{
	uint32 nRet = 0;
	bool bDone = false;
	uint32 nCondCount = pCondition->GetParaCount();
	if(nCondCount)
	{
		bExist = 0;
		uint32 nRet3 = 0, nRet4 = 1;
		for(uint32 i=0; i<nCondCount; ++i)
		{
			bool bParaSet;
			uint32 bExist2 = 0;
			CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
			if(pCond->IsEmpty())
				continue;
			bDone = true;
			CMdbAbstractIndex* pIndex = IndexChoice(pCond);
			m_oRowIndex.LockTable(true);
			uint32 nRet2 = pIndex->Exist(pCond, bExist2);
			m_oRowIndex.UnLockTable(true);
			if(nRet2)
				nRet3 = nRet2;
			else
			{
				nRet4 = 0;
				bExist = bExist2;
				break;
			}
		}
		if(nRet4)
			nRet = nRet3;
	}
	if(!bDone)
	{
		pCondition->Clear();
		CMdbAbstractIndex* pIndex = IndexChoice(pCondition);
		m_oRowIndex.LockTable(true);
		nRet = pIndex->Exist(pCondition, bExist);
		m_oRowIndex.UnLockTable(true);
	}
	return nRet;
}

uint32 CMdbTable::Insert(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlPara* pInsertAttr)
{
	CMdbRecord& oRecord = pInsertAttr->m_oRecord;
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbFieldDef* pFieldDef = m_pTabDef->pFieldDefines + i;
		if(IS_SET_FIELD(pInsertAttr, i))
		{
			if(oRecord.IsNull(i) && pFieldDef->bNotNull)
				return MDB_FIELD_ISNULL;
		}
		else if(pFieldDef->sDefault && pFieldDef->sDefault[0])
		{
			oRecord.GetField(i)->SetFromString(pFieldDef->sDefault);
			pInsertAttr->SetField(i, MDB_SQLPARA_OPERATOR_EQUAL);
		}
		else if(pFieldDef->bNotNull)
			return MDB_FIELD_ISNULL;
	}
	CMdbSqlFilter oFilter(m_pTabDef);
	pInsertAttr->GetFilter(oFilter);
	CMdbDataRecord* pDataRecord = m_oRowIndex.NewDataRecord();
	{
		CMdbRecord oRecord2(m_pTabDef, pDataRecord->pRecord);
		oRecord2.CopyFrom(oFilter, &oRecord);
	}

	CLocalMdb* pDb = (CLocalMdb*)pAccess->GetMdb();
	pDb->ProcessTrigger(pAccess, nCaller, &pDb->m_oInsertTriggerList);

	CMdbTableAccess* pTableAccess = (CMdbTableAccess*)pAccess;
	if(m_oInsertBeforeTriggerList.GetSize() || m_oInsertAfterTriggerList.GetSize())
	{
		pTableAccess->m_oGetAttr.Clear();
		pTableAccess->m_oGetAttr.SetRecordSetSize(1);
		CMdbRecord* pRecord = pTableAccess->m_oGetAttr.AllocRecord();
		pRecord->CopyFrom(oFilter, &oRecord);
	}

	m_oRowIndex.LockTable(false);

	if(!ProcessBeforeTrigger(pAccess, nCaller, &m_oInsertBeforeTriggerList))
	{
		m_oRowIndex.UnLockTable(false);
		m_oRowIndex.DelDataRecord(pDataRecord);
		return MDB_TRIGGER_CHECKFAILURE;
	}

	if(m_oRowIndex.m_nRecordCount >= m_pTabDef->nMaxRecordNum)
	{
		m_oRowIndex.UnLockTable(false);
		m_oRowIndex.DelDataRecord(pDataRecord);
		return MDB_RECORD_TOOMANY;
	}

	for(i=0; i<m_nIndexCount; ++i)
	{
		uint32 nRet = m_pIndexTable[i]->AttachRecord(pDataRecord);
		if(nRet)
		{
			for(uint32 j=0; j<i; ++j)
				m_pIndexTable[j]->DetachRecord(pDataRecord);
			m_oRowIndex.UnLockTable(false);
			m_oRowIndex.DelDataRecord(pDataRecord);
			return nRet;
		}
	}
	m_oRowIndex.AttachRecord(pDataRecord);

	ProcessAfterTrigger(pAccess, nCaller, &m_oInsertAfterTriggerList);

	m_oRowIndex.UnLockTable(false);
	return MDB_SUCCESS;
}

struct CMdbDeleteParameter
{
	CMdbTableAccess* pAccess;
	uint32 nCaller;
	CMdbTable* pTable;
};

uint32 DeleteMdbRecord(CMdbAbstractIndex* pIndex, CMdbDataRecord* pDataRecord, void* pPara, bool &bDeleted)
{
	bDeleted = true;
	CMdbDeleteParameter* pContext = (CMdbDeleteParameter*)pPara;

	CMdbTable* pTable = pContext->pTable;
	CMdbTableAccess* pAccess = pContext->pAccess;

	if(pTable->m_oDeleteBeforeTriggerList.GetSize() || pTable->m_oDeleteAfterTriggerList.GetSize())
	{
		pAccess->m_oGetAttr.Clear();
		pAccess->m_oGetAttr.SetRecordSetSize(1);
		CMdbRecord* pRecord = pAccess->m_oGetAttr.AllocRecord();
		CMdbSqlFilter oFilter(pIndex->m_pTabDef);
		CMdbRecord oRecord(pIndex->m_pTabDef, pDataRecord->pRecord);
		pRecord->CopyFrom(oFilter, &oRecord);
	}

	if(!pTable->ProcessBeforeTrigger(pAccess, pContext->nCaller, &pTable->m_oDeleteBeforeTriggerList))
	{
		bDeleted = false;
		return MDB_SUCCESS;
	}

	pContext->pTable->Delete(pAccess, pContext->nCaller, pDataRecord);

	pTable->ProcessAfterTrigger(pAccess, pContext->nCaller, &pTable->m_oDeleteAfterTriggerList);

	return MDB_SUCCESS;
}

uint32 CMdbTable::Delete(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, uint32 &nRemovedCount)
{
	uint32 nRet = 0;
	bool bDone = false;
	uint32 nCondCount = pCondition->GetParaCount();
	if(nCondCount)
	{
		uint32 nRet3 = 0, nRet4 = 1;
		for(uint32 i=0; i<nCondCount; ++i)
		{
			bool bParaSet;
			uint32 nCount2 = 0;
			CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
			if(pCond->IsEmpty())
				continue;
			bDone = true;
			uint32 nRet2 = Delete2(pAccess, nCaller, pCond, nCount2);
			if(nRet2)
				nRet3 = nRet2;
			else
			{
				nRet4 = 0;
				nRemovedCount += nCount2;
			}
		}
		if(nRet4)
			nRet = nRet3;
	}
	if(!bDone)
	{
		pCondition->Clear();
		nRet = Delete2(pAccess, nCaller, pCondition, nRemovedCount);
	}
	return nRet;
}

uint32 CMdbTable::Delete2(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, uint32 &nRemovedCount)
{
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	FillIndexFilter(oFilter);
	CMdbAbstractIndex* pIndex = IndexChoice(pCondition);
	nRemovedCount = 0;

	CLocalMdb* pDb = (CLocalMdb*)pAccess->GetMdb();

	pDb->ProcessTrigger(pAccess, nCaller, &pDb->m_oDeleteTriggerList);

	if(pCondition->m_nParaCount ||
			m_oDeleteBeforeTriggerList.GetSize() ||
			m_oDeleteAfterTriggerList.GetSize())
	{
		CMdbDeleteParameter oContext = {(CMdbTableAccess*)pAccess, nCaller, this};

		m_oRowIndex.LockTable(false);
		uint32 nRet = pIndex->Travel(pCondition, oFilter, DeleteMdbRecord, nRemovedCount, &oContext);
		m_oRowIndex.UnLockTable(false);

		return nRet;
	}

	m_oRowIndex.LockTable(false);
	nRemovedCount = m_oRowIndex.m_nRecordCount;
	if(nRemovedCount)
	{
		for(register uint32 i=0; i<m_nIndexCount; ++i)
			m_pIndexTable[i]->Truncate();
		m_oRowIndex.Truncate();
		m_oRowIndex.UnLockTable(false);
	}

	return MDB_SUCCESS;
}

void CMdbTable::Delete(CMdbAccess* pAccess, uint32 nCaller, CMdbDataRecord* pDataRecord)
{
	uint32 nCount;
	CMdbField oLField, oRField;
	CMdbRecord oRRecord(m_pTabDef, pDataRecord->pRecord);
	for(uint32 i=0; i<m_nIndexCount; ++i)
	{
		CMdbAbstractIndex* pIndex = m_pIndexTable[i];
		CMdbIndexDef* pIdxDef = pIndex->m_pIdxDef;
		uint32 nDetailIndexCount = pIndex->m_nDetailIndexCount;
		uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
		for(uint32 j=0; j<nDetailIndexCount; ++j)
		{
			CMdbAbstractIndex* pDetailIndex = pIndex->m_pDetailIndexes[j];
			CMdbTableDef* pDetailTabDef = pDetailIndex->m_pTabDef;
			CMdbTable* pDetailTable = (CMdbTable*)pDetailTabDef->pExtendAttr->pInstance;
			CMdbSqlParaSet oNewCondition(pDetailTabDef);
			CMdbSqlPara* pPara = (CMdbSqlPara*)oNewCondition.AddPara();
			CMdbRecord& oLRecord = pPara->m_oRecord;
			for(uint32 k=0; k<nFieldCount; ++k)
			{
				uint32 nRightFieldNo = pIdxDef->pExtendAttr->pFields[k];
				uint32 nLeftFieldNo = pDetailIndex->m_pIdxDef->pExtendAttr->pFields[k];
				oLRecord.GetField(nLeftFieldNo, oLField);
				oRRecord.GetField(nRightFieldNo, oRField);
				oLField.SetFromField(&oRField);
				pPara->SetField(nLeftFieldNo, MDB_SQLPARA_OPERATOR_EQUAL);
			}
			pDetailTable->Delete(pAccess, nCaller, &oNewCondition, nCount);
		}
		pIndex->DetachRecord(pDataRecord);
	}
	m_oRowIndex.DetachRecord(pDataRecord);
	m_oRowIndex.DelDataRecord(pDataRecord);
}

struct CMdbUpdateParameter
{
	CMdbDataRecord* pRollBack;//pNext用以维护拉链表，pPrev用以保存新记录。
	CMdbSqlPara* pSetAttr;
	uint32* pModifiedCount;
	CMdbTableAccess* pAccess;
	uint32 nCaller;
	bool bNeedDetach;
	bool bNeedRollBack;
};

uint32 UpdateMdbRecord(CMdbAbstractIndex* pIndex, CMdbDataRecord* pDataRecord, void* pPara, bool &bDeleted)
{
	bDeleted = false;
	CMdbUpdateParameter* pContext = (CMdbUpdateParameter*)pPara;
	CMdbTable* pTable = (CMdbTable*)pIndex->m_pTabDef->pExtendAttr->pInstance;
	CMdbTableAccess* pAccess = pContext->pAccess;

	if(pTable->m_oUpdateBeforeTriggerList.GetSize() || pTable->m_oUpdateAfterTriggerList.GetSize())
	{
		CMdbSqlFilter oFilter(pIndex->m_pTabDef);
		pAccess->m_oGetAttr.Clear();
		pAccess->m_oGetAttr.SetRecordSetSize(2);
		CMdbRecord* pRecord[2] = {pAccess->m_oGetAttr.AllocRecord(), pAccess->m_oGetAttr.AllocRecord()};
		CMdbRecord oRecord(pIndex->m_pTabDef, pDataRecord->pRecord);
		pRecord[0]->CopyFrom(oFilter, &oRecord);//new
		pRecord[1]->CopyFrom(oFilter, &oRecord);//old
		pContext->pSetAttr->GetFilter(oFilter);
		pRecord[0]->CopyFrom(oFilter, &pContext->pSetAttr->m_oRecord);
	}

	if(!pTable->ProcessBeforeTrigger(pAccess, pContext->nCaller, &pTable->m_oUpdateBeforeTriggerList))
		return MDB_SUCCESS;

	uint32 nRet = pTable->Update(pDataRecord, pContext->pSetAttr, pContext->pRollBack,
								 pContext->pModifiedCount, pContext->bNeedDetach, pContext->bNeedRollBack);

	if(!nRet)
		pTable->ProcessAfterTrigger(pAccess, pContext->nCaller, &pTable->m_oUpdateAfterTriggerList);

	return nRet;
}

uint32 CMdbTable::Update(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, CMdbSqlPara* pSetAttr, uint32 &nModifiedCount)
{
	uint32 nRet = 0;
	if(pSetAttr->m_nParaCount == 0)
		return MDB_SUCCESS;
	bool bDone = false;
	uint32 nCondCount = pCondition->GetParaCount();
	if(nCondCount)
	{
		uint32 nRet3 = 0, nRet4 = 1;
		for(uint32 i=0; i<nCondCount; ++i)
		{
			bool bParaSet;
			uint32 nCount2 = 0;
			CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pCondition->GetPara(i, bParaSet);
			if(pCond->IsEmpty())
				continue;
			bDone = true;
			uint32 nRet2 = Update2(pAccess, nCaller, pCond, pSetAttr, nCount2);
			if(nRet2)
				nRet3 = nRet2;
			else
			{
				nRet4 = 0;
				nModifiedCount += nCount2;
			}
		}
		if(nRet4)
			nRet = nRet3;
	}
	if(!bDone)
	{
		pCondition->Clear();
		nRet = Update2(pAccess, nCaller, pCondition, pSetAttr, nModifiedCount);
	}
	return nRet;
}

uint32 CMdbTable::Update2(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, CMdbSqlPara* pSetAttr, uint32 &nModifiedCount)
{
	uint32 nOperator, nFieldNo;
	nModifiedCount = 0;
	CMdbRecord& oSetRecord = pSetAttr->m_oRecord;
	uint32 i, nSetCount = pSetAttr->m_nParaCount;
	for(i=0; i<nSetCount; ++i)
	{
		pSetAttr->GetPara(i, &nOperator, &nFieldNo);
		if(m_pTabDef->pFieldDefines[nFieldNo].bNotNull && oSetRecord.IsNull(nFieldNo))
			return MDB_FIELD_ISNULL;
	}
	CMdbSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	CMdbUpdateParameter oUpdateParameter = {NULL, pSetAttr, &nModifiedCount, (CMdbTableAccess*)pAccess, nCaller, false, false};
	for(i=0; i<m_nIndexCount; ++i)
	{
		CMdbAbstractIndex* pIndex = m_pIndexTable[i];
		CMdbIndexDef* pIdxDef = pIndex->m_pIdxDef;
		uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
		if(pIdxDef->nQualifier == MDB_UNIQUE_INDEX)
		{
			for(uint32 j=0; j<nFieldCount; ++j)
			{
				nFieldNo = pIdxDef->pExtendAttr->pFields[j];
				oFilter.SetField(nFieldNo);
				if(IS_SET_FIELD(pSetAttr, nFieldNo))
				{
					if(pIndex->m_nDetailIndexCount)
						return MDB_TRY_UPDATE_PRIMARYKEY;
					if(pIdxDef->pExtendAttr->pPrimaryIndex && j<pIdxDef->pExtendAttr->pPrimaryIndex->pExtendAttr->nFieldCount)
						return MDB_TRY_UPDATE_PRIMARYKEY;
					oUpdateParameter.bNeedDetach = true;
					oUpdateParameter.bNeedRollBack = true;
					break;
				}
			}
		}
		else
		{
			for(uint32 j=0; j<nFieldCount; ++j)
			{
				nFieldNo = pIdxDef->pExtendAttr->pFields[j];
				oFilter.SetField(nFieldNo);
				if(IS_SET_FIELD(pSetAttr, nFieldNo))
				{
					if(pIdxDef->pExtendAttr->pPrimaryIndex && j<pIdxDef->pExtendAttr->pPrimaryIndex->pExtendAttr->nFieldCount)
						return MDB_TRY_UPDATE_PRIMARYKEY;
					oUpdateParameter.bNeedDetach = true;
					break;
				}
			}
		}
	}

	CLocalMdb* pDb = (CLocalMdb*)pAccess->GetMdb();
	pDb->ProcessTrigger(pAccess, nCaller, &pDb->m_oUpdateTriggerList);

	CMdbAbstractIndex* pIndex = IndexChoice(pCondition);
	if(oUpdateParameter.bNeedDetach)
	{
		FillIndexFilter(oFilter);
		m_oRowIndex.LockTable(false);
	}
	else
		m_oRowIndex.LockTable(true);
	uint32 nRet = pIndex->Travel(pCondition, oFilter, UpdateMdbRecord, nModifiedCount, &oUpdateParameter);
	if(oUpdateParameter.bNeedDetach)
		m_oRowIndex.UnLockTable(false);
	else
		m_oRowIndex.UnLockTable(true);
	if(nRet)
		nModifiedCount = 0;
	return nRet;
}

uint32 CMdbTable::Update(CMdbDataRecord* pDataRecord, CMdbSqlPara* pSetAttr,
						 CMdbDataRecord *&pRollBack, uint32* pModifiedCount, bool bNeedDetach, bool bNeedRollBack)
{
	uint32 nSetCount = pSetAttr->m_nParaCount;
	uint32 i, nOperator, nFieldNo;
	if(bNeedDetach)
	{
		for(i=0; i<m_nIndexCount; ++i)
			m_pIndexTable[i]->DetachRecord(pDataRecord);
		m_oRowIndex.DetachRecord(pDataRecord);
		if(bNeedRollBack)
		{
			pDataRecord->pNext = pRollBack;
			pRollBack = pDataRecord;
			pDataRecord = m_oRowIndex.CloneRecord(pDataRecord);
			pRollBack->pPrev = pDataRecord;
		}
	}
	CMdbField oDbField, *pSetField;
	CMdbRecord& oSetRecord = pSetAttr->m_oRecord;
	CMdbRecord oDbRecord(m_pTabDef, pDataRecord->pRecord);
	for(i=0; i<nSetCount; ++i)
	{
		pSetAttr->GetPara(i, &nOperator, &nFieldNo);
		pSetField = oSetRecord.GetField(nFieldNo);
		oDbRecord.GetField(nFieldNo, oDbField);
		oDbField.SetFromField(pSetField, nOperator);
	}
	if(bNeedDetach)
	{
		m_oRowIndex.AttachRecord(pDataRecord);
		for(i=0; i<m_nIndexCount; ++i)
		{
			uint32 nRet = m_pIndexTable[i]->AttachRecord(pDataRecord);
			if(nRet)
			{
				RollBack(pRollBack);
				return nRet;
			}
		}
	}
	return MDB_SUCCESS;
}

void CMdbTable::RollBack(CMdbDataRecord *pRollBack)
{
	uint32 i;
	while(pRollBack)
	{
		CMdbDataRecord* pTmp = pRollBack->pPrev;
		for(i=0; i<m_nIndexCount; ++i)
			m_pIndexTable[i]->DetachRecord(pTmp);
		m_oRowIndex.DetachRecord(pTmp);
		m_oRowIndex.DelDataRecord(pTmp);
		pTmp = pRollBack->pNext;
		m_oRowIndex.AttachRecord(pRollBack);
		for(i=0; i<m_nIndexCount; ++i)
			m_pIndexTable[i]->AttachRecord(pRollBack);
		pRollBack = pTmp;
	}
}

void CMdbTable::Truncate(CMdbAccess* pAccess, uint32 nCaller)
{
	CMdb* pDb = pAccess->GetMdb();

	((CLocalMdb*)pDb)->ProcessTrigger(pAccess, nCaller, &((CLocalMdb*)pDb)->m_oTruncateTriggerList);

	m_oRowIndex.LockTable(false);

	if(!ProcessBeforeTrigger(pAccess, nCaller, &m_oTruncateBeforeTriggerList))
	{
		m_oRowIndex.UnLockTable(false);
		return;
	}

	for(register uint32 i=0; i<m_nIndexCount; ++i)
		m_pIndexTable[i]->Truncate();
	m_oRowIndex.Truncate();

	ProcessAfterTrigger(pAccess, nCaller, &m_oTruncateAfterTriggerList);

	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::FillIndexFilter(CMdbSqlFilter& oFilter)
{
	for(register uint32 i=0; i<m_nIndexCount; ++i)
	{
		CMdbIndexDef* pIdxDef = m_pTabDef->pExtendAttr->pIndexDefineSet[i];
		register uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
		for(register uint32 j=0; j<nFieldCount; ++j)
			oFilter.SetField(pIdxDef->pExtendAttr->pFields[j]);
	}
}

struct CMdbIndexChose
{
	CMdbAbstractIndex* pIndex;
	uint32 nFieldCount;
	uint32 nMatchFieldCount;
	uint32 nArithmetic;
	uint32 nQualifier;
	bool Matched[MDB_MAX_INDEX_FIELD_NUM];
};

CMdbAbstractIndex* CMdbTable::IndexChoice(CMdbSqlParaSet* pCondition)
{
	uint32 i;
	uint32 nFlagCount = GetFieldFlagCount(m_pTabDef);
	uint32 pFlag[MDB_FLG_L(MDB_MAX_FIELD_NUM-1)+1];// = new uint32[nFlagCount];
	uint32 pIsRange[MDB_MAX_FIELD_NUM];// = new uint32[nFieldCount];
	if(!pCondition->GetParaCount())
		return &m_oRowIndex;
	for(i=0; i<nFlagCount; ++i)
		pFlag[i] = 0;
	if(!GetFieldAttrTable(pFlag, pIsRange, pCondition))
		return &m_oRowIndex;
	uint32 j,nIndexCount = 0;
	CMdbIndexChose pIndexChoseTable[MDB_MAX_INDEX_NUM];// = new CIndexChose[m_nIndexCount];
	for(i=0; i<m_nIndexCount; ++i)
	{
		CMdbIndexChose& oIndexChose = pIndexChoseTable[nIndexCount];
		CMdbIndexDef* pIndexDef = m_pTabDef->pExtendAttr->pIndexDefineSet[i];
		oIndexChose.pIndex = m_pIndexTable[i];
		oIndexChose.nFieldCount = pIndexDef->pExtendAttr->nFieldCount;
		oIndexChose.nMatchFieldCount = 0;
		oIndexChose.nArithmetic = pIndexDef->nArithmetic;
		oIndexChose.nQualifier = pIndexDef->nQualifier;
		bool bSupportRange = (oIndexChose.nArithmetic & MDB_RANGE_INDEX);
		for(j=0; j<oIndexChose.nFieldCount; ++j)
		{
			uint32 nFieldNo = pIndexDef->pExtendAttr->pFields[j];
			uint32 nFlag = MDB_FLG_L(nFieldNo);
			uint32 nBit = MDB_FLG_B(nFieldNo);
			if(pFlag[nFlag] & nBit)
			{
				if(pIsRange[nFieldNo] && !bSupportRange)
					break;
				++oIndexChose.nMatchFieldCount;
				oIndexChose.Matched[j] = true;
			}
			else
				oIndexChose.Matched[j] = false;
		}
		if(j < oIndexChose.nFieldCount || !oIndexChose.nMatchFieldCount)
			continue;
		if(oIndexChose.nArithmetic == MDB_HASH_INDEX)
		{
			if(oIndexChose.nMatchFieldCount != oIndexChose.nFieldCount)
				continue;
		}
		++nIndexCount;
	}
	if(!nIndexCount)
		return &m_oRowIndex;

	uint32 nChoice[2], nMaxFieldCount[2];
	nMaxFieldCount[0] = nMaxFieldCount[1] = MDB_MAX_INDEX_FIELD_NUM+1;
	for(i=0; i<nIndexCount; ++i)
	{
		CMdbIndexChose &oIndexChose = pIndexChoseTable[i];
		if(oIndexChose.nQualifier == MDB_UNIQUE_INDEX)
		{
			if(oIndexChose.nArithmetic == MDB_HASH_INDEX)
			{
				if(nMaxFieldCount[0] > oIndexChose.nFieldCount)
				{
					nMaxFieldCount[0] = oIndexChose.nFieldCount;
					nChoice[0] = i;
				}
			}
			else if(oIndexChose.nFieldCount == oIndexChose.nMatchFieldCount)
			{
				if(nMaxFieldCount[1] > oIndexChose.nFieldCount)
				{
					nMaxFieldCount[1] = oIndexChose.nFieldCount;
					nChoice[1] = i;
				}
			}
		}
	}
	for(i=0; i<2; ++i)
	{
		if(nMaxFieldCount[i] <= MDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[i]].pIndex;
	}

	nMaxFieldCount[0] = nMaxFieldCount[1] = MDB_MAX_INDEX_FIELD_NUM+1;
	for(i=0; i<nIndexCount; ++i)
	{
		CMdbIndexChose &oIndexChose = pIndexChoseTable[i];
		if(oIndexChose.nFieldCount == oIndexChose.nMatchFieldCount)
		{
			if(oIndexChose.nArithmetic == MDB_HASH_INDEX)
			{
				if(nMaxFieldCount[0] > oIndexChose.nFieldCount)
				{
					nMaxFieldCount[0] = oIndexChose.nFieldCount;
					nChoice[0] = i;
				}
			}
			else
			{
				if(nMaxFieldCount[1] > oIndexChose.nFieldCount)
				{
					nMaxFieldCount[1] = oIndexChose.nFieldCount;
					nChoice[1] = i;
				}
			}
		}
	}
	for(i=0; i<2; ++i)
	{
		if(nMaxFieldCount[i] <= MDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[i]].pIndex;
	}

	for(i=0; i<3; ++i)
	{
		nMaxFieldCount[1] = MDB_MAX_INDEX_FIELD_NUM+1;
		for(j=0; j<nIndexCount; ++j)
		{
			CMdbIndexChose &oIndexChose = pIndexChoseTable[j];
			if(oIndexChose.nArithmetic == MDB_RBTREE_INDEX)
			{
				if(oIndexChose.nFieldCount>i && oIndexChose.Matched[i])
				{
					if(nMaxFieldCount[1] > oIndexChose.nFieldCount)
					{
						nMaxFieldCount[1] = oIndexChose.nFieldCount;
						nChoice[1] = j;
					}
				}
			}
		}
		if(nMaxFieldCount[1] <= MDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[1]].pIndex;
	}

	return &m_oRowIndex;
}

uint32 CMdbTable::GetFieldAttrTable(uint32* pFlag, uint32* pIsRange, CMdbSqlParaSet* pCondition)
{
	uint32 nCount = 0;
	uint32 nParameterCount = pCondition->m_nParaCount;
	for(uint32 i=0; i<nParameterCount; ++i)
	{
		bool bParaSet;
		void * pPara = pCondition->GetPara(i, bParaSet);
		if(bParaSet)
			nCount += GetFieldAttrTable(pFlag, pIsRange, (CMdbSqlParaSet*)pPara);
		else
		{
			CMdbSqlPara* pParameter = (CMdbSqlPara*)pPara;
			uint32 nParaCount = pParameter->m_nParaCount;
			for(uint32 j=0; j<nParaCount; ++j)
			{
				uint32 nOperator, nFieldNo;
				pParameter->GetPara(j, &nOperator, &nFieldNo);
				uint32 nFlag = MDB_FLG_L(nFieldNo);
				uint32 nBit = MDB_FLG_B(nFieldNo);
				if(pFlag[nFlag] & nBit)
				{
					uint32 &bIsRange = pIsRange[nFieldNo];
					if(!bIsRange)
						bIsRange = (nOperator!=MDB_SQLPARA_OPERATOR_EQUAL);
				}
				else
				{
					pIsRange[nFieldNo] = (nOperator!=MDB_SQLPARA_OPERATOR_EQUAL);
					pFlag[nFlag] |= nBit;
					++nCount;
				}
			}
		}
	}
	return nCount;
}

void CMdbTable::RegInsertBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CBeforeTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oInsertBeforeTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegInsertAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CAfterTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oInsertAfterTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegDeleteBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CBeforeTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oDeleteBeforeTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegDeleteAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CAfterTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oDeleteAfterTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegUpdateBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CBeforeTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oUpdateBeforeTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegUpdateAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CAfterTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oUpdateAfterTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegTruncateBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CBeforeTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oTruncateBeforeTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegTruncateAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CAfterTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oTruncateAfterTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

void CMdbTable::RegQueryTrigger(FQueryTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CQueryTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oRowIndex.LockTable(false);
	m_oQueryTriggerList.Push(oItem);
	m_oRowIndex.UnLockTable(false);
}

bool CMdbTable::ProcessQueryTrigger(CMdbAccess* pAccess, uint32 nCaller, uint32 nMode, uint32 nPageSize, uint32 &nSkipCount)
{
	bool bRet = false;
	m_oRowIndex.LockTable(true);
	void* pIt = m_oQueryTriggerList.First();
	while(pIt)
	{
		CQueryTriggerItem& oItem = m_oQueryTriggerList.GetItem(pIt);
		if(oItem.nCallerMask & nCaller)
		{
			bRet = oItem.Trigger(pAccess, nCaller, oItem.pContext, nMode, nPageSize, nSkipCount);
			if(bRet)
				break;
		}
		pIt = m_oQueryTriggerList.GetNext(pIt);
	}
	m_oRowIndex.UnLockTable(true);
	return bRet;
}

bool CMdbTable::ProcessBeforeTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CBeforeTriggerItem>* pTriggerList)
{
	bool bRet = true;
	m_oRowIndex.LockTable(true);
	void* pIt = pTriggerList->First();
	while(pIt)
	{
		CBeforeTriggerItem& oItem = pTriggerList->GetItem(pIt);
		if(oItem.nCallerMask & nCaller)
		{
			bRet = oItem.Trigger(pAccess, nCaller, oItem.pContext);
			if(!bRet)
				break;
		}
		pIt = pTriggerList->GetNext(pIt);
	}
	m_oRowIndex.UnLockTable(true);
	return bRet;
}

void CMdbTable::ProcessAfterTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CAfterTriggerItem>* pTriggerList)
{
	m_oRowIndex.LockTable(true);
	void* pIt = pTriggerList->First();
	while(pIt)
	{
		CAfterTriggerItem& oItem = pTriggerList->GetItem(pIt);
		if(oItem.nCallerMask & nCaller)
			oItem.Trigger(pAccess, nCaller, oItem.pContext);
		pIt = pTriggerList->GetNext(pIt);
	}
	m_oRowIndex.UnLockTable(true);
}

//-----------------------------------------------------
// CMdbDataAccess
//-----------------------------------------------------
CMdbDataAccess::CMdbDataAccess(CMdb* pDb, CMdbTableDef* pTabDef, CMdbTable* pTable):
	CMdbTableAccess(pDb, pTabDef)
{
	m_pTable = pTable;
}

CMdbDataAccess::~CMdbDataAccess()
{
}

uint32 CMdbDataAccess::Insert(uint32 nCaller)
{
	if(m_oInsertAttr.HaveError())
		return MDB_INVALID_INPUT;
	return m_pTable->Insert(this, nCaller, &m_oInsertAttr);
}

uint32 CMdbDataAccess::Update(uint32* pModifiedCount, uint32 nCaller)
{
	uint32 nCount = 0;
	if(pModifiedCount)
		*pModifiedCount = 0;
	if(m_oIdxAttr.HaveError() || m_oSetAttr.HaveError())
		return MDB_INVALID_INPUT;
	uint32 nRet = m_pTable->Update(this, nCaller, &m_oIdxAttr, &m_oSetAttr, nCount);
	if(pModifiedCount)
		*pModifiedCount = nCount;
	return nRet;
}

uint32 CMdbDataAccess::Delete(uint32* pDeletedCount, uint32 nCaller)
{
	uint32 nCount = 0;
	if(pDeletedCount)
		*pDeletedCount = 0;
	if(m_oIdxAttr.HaveError())
		return MDB_INVALID_INPUT;
	uint32 nRet = m_pTable->Delete(this, nCaller, &m_oIdxAttr, nCount);
	if(pDeletedCount)
		*pDeletedCount = nCount;
	return nRet;
}

uint32 CMdbDataAccess::Truncate(uint32 nCaller)
{
	m_pTable->Truncate(this, nCaller);
	return MDB_SUCCESS;
}

uint32 CMdbDataAccess::Query(uint32 nPageSize, uint32 nSkipCount, uint32 nCaller)
{
	m_oGetAttr.Clear();
	if(m_oIdxAttr.HaveError())
		return MDB_INVALID_INPUT;
	if(nPageSize == 0)
		return MDB_SUCCESS;
	if(m_pTable->m_oQueryTriggerList.GetSize())
	{
		if(m_pTable->ProcessQueryTrigger(this, nCaller, 0, nPageSize, nSkipCount))
			return MDB_SUCCESS;
	}
	bool bAsc = true;
	CMdbAbstractIndex* pIndex = NULL;
	m_oGetAttr.SetRecordSetSize(nPageSize);
	if(m_pOrderByIndex)
	{
		bAsc = m_bAscOrder;
		pIndex = (CMdbAbstractIndex*)m_pOrderByIndex->pExtendAttr->pInstance;
	}
	return m_pTable->Query(&m_oIdxAttr, m_oFilter, m_oGetAttr, nSkipCount, pIndex, bAsc);
}

uint32 CMdbDataAccess::Count(uint32 &nCount, uint32 nCaller)
{
	nCount = 0;
	if(m_oIdxAttr.HaveError())
		return MDB_INVALID_INPUT;
	if(m_pTable->m_oQueryTriggerList.GetSize())
	{
		if(m_pTable->ProcessQueryTrigger(this, nCaller, 1, 0, nCount))
			return MDB_SUCCESS;
	}
	return m_pTable->Query(&m_oIdxAttr, nCount);
}

uint32 CMdbDataAccess::Exist(uint32& bExist, uint32 nCaller)
{
	bExist = 0;
	if(m_oIdxAttr.HaveError())
		return MDB_INVALID_INPUT;
	if(m_pTable->m_oQueryTriggerList.GetSize())
	{
		if(m_pTable->ProcessQueryTrigger(this, nCaller, 2, 0, bExist))
			return MDB_SUCCESS;
	}
	return m_pTable->Exist(&m_oIdxAttr, bExist);
}

//-----------------------------------------------------
// CLocalMdb
//-----------------------------------------------------
CLocalMdb::CLocalMdb(const char* sDbName):CMdb(sDbName),m_oMdbDef(GetDbName())
{
}

CLocalMdb::~CLocalMdb()
{
	CRbTreeNode* pIt, *pEnd;

	pEnd = m_oIndexs.End();
	pIt = m_oIndexs.First();
	for(; pIt!=pEnd; pIt=m_oIndexs.GetNext(pIt))
	{
		CMdbAbstractIndex* pIdx = m_oIndexs.GetItem(pIt);
		delete pIdx;
	}
	m_oIndexs.Clear();

	pEnd = m_oTables.End();
	pIt = m_oTables.First();
	for(; pIt!=pEnd; pIt=m_oTables.GetNext(pIt))
	{
		CMdbTable* pTable = m_oTables.GetItem(pIt);
		delete pTable;
	}
	m_oTables.Clear();
}

CMdbAccess* CLocalMdb::CreateAccess(const char* sTableName)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);

	if(pTableDef == NULL)
		return NULL;

	return new CMdbDataAccess(this, pTableDef, (CMdbTable*)pTableDef->pExtendAttr->pInstance);
}

uint32 CLocalMdb::CreateTable(CMdbTableDef* pTableDefine)
{
	uint32 nRet = m_oMdbDef.CreateTable(pTableDefine);
	if(nRet)
		return nRet;

	m_oTables[pTableDefine->sTableName] = new CMdbTable(m_oMdbDef.GetTableDefine(pTableDefine->sTableName));

	return MDB_SUCCESS;
}

CMdbTableDef* CLocalMdb::GetTableDefine(const char* sTableName)
{
	return m_oMdbDef.GetTableDefine(sTableName);
}

const char* CLocalMdb::GetTableList()
{
	return m_oMdbDef.GetTableList();
}

CMdbLocalInterface* CLocalMdb::GetLocalInterface()
{
	return this;
}

uint32 CLocalMdb::CreateIndex(CMdbIndexDef* pIndexDefine)
{
	CMdbTableDef* pTabDef = m_oMdbDef.GetTableDefine(pIndexDefine->sTableName);
	if(!pTabDef)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_TABLE_NOTEXIST", GetDbName(), pIndexDefine->sIndexName, pIndexDefine->sTableName));
		return MDB_TABLE_NOTEXIST;
	}
	CMdbTable* pTable = (CMdbTable*)pTabDef->pExtendAttr->pInstance;
	if(pTable->m_oRowIndex.m_nRecordCount)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_NONEMPTY_TABLE", GetDbName(), pIndexDefine->sIndexName, pIndexDefine->sTableName));
		return MDB_NONEMPTY_TABLE;
	}
	uint32 nRet = m_oMdbDef.CreateIndex(pIndexDefine);
	if(nRet)
		return nRet;
	pIndexDefine = m_oMdbDef.GetIndexDefine(pIndexDefine->sIndexName);

	CMdbAbstractIndex* pIndex;
	if(pIndexDefine->nArithmetic == MDB_RBTREE_INDEX)
		pIndex = new CMdbRbTreeIndex(pTabDef, pIndexDefine, &pTable->m_oRowIndex);
	else if(pIndexDefine->nArithmetic == MDB_HASH_INDEX)
		pIndex = new CMdbHashIndex(pTabDef, pIndexDefine, &pTable->m_oRowIndex);
	m_oIndexs[pIndexDefine->sIndexName] = pIndex;

	pTable->AddIndex(pIndex);

	return MDB_SUCCESS;
}

CMdbIndexDef* CLocalMdb::GetIndexDefine(const char* sIndexName)
{
	return m_oMdbDef.GetIndexDefine(sIndexName);
}

const char* CLocalMdb::GetIndexList()
{
	return m_oMdbDef.GetIndexList();
}

uint32 CLocalMdb::GetIndexCount(const char* sTableName, uint32 & nCount)
{
	nCount = 0;
	CMdbTableDef* pTabDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTabDef == NULL)
		return MDB_TABLE_NOTEXIST;
	nCount = pTabDef->pExtendAttr->nIndexCount;
	return MDB_SUCCESS;
}

uint32 CLocalMdb::GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName)
{
	CMdbTableDef* pTabDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTabDef == NULL)
		return MDB_TABLE_NOTEXIST;
	if(nIdx >= pTabDef->pExtendAttr->nIndexCount)
		return MDB_INDEX_NOTEXIST;
	CString::StringCopy(pIndexName, pTabDef->pExtendAttr->pIndexDefineSet[nIdx]->sIndexName);
	return MDB_SUCCESS;
}

void CLocalMdb::RegInsertDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oMutex.Enter();
	m_oInsertTriggerList.Push(oItem);
	m_oMutex.Leave();
}

void CLocalMdb::RegDeleteDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oMutex.Enter();
	m_oDeleteTriggerList.Push(oItem);
	m_oMutex.Leave();
}

void CLocalMdb::RegUpdateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oMutex.Enter();
	m_oUpdateTriggerList.Push(oItem);
	m_oMutex.Leave();
}

void CLocalMdb::RegTruncateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTriggerItem oItem = {Trigger, pContext, nCallerMask};
	m_oMutex.Enter();
	m_oTruncateTriggerList.Push(oItem);
	m_oMutex.Leave();
}

void CLocalMdb::RegInsertBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegInsertBeforeTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegInsertAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegInsertAfterTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegDeleteBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegDeleteBeforeTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegDeleteAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegDeleteAfterTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegUpdateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegUpdateBeforeTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegUpdateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegUpdateAfterTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegTruncateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegTruncateBeforeTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegTruncateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegTruncateAfterTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::RegQueryTrigger(const char* sTableName, FQueryTrigger Trigger, void* pContext, uint32 nCallerMask)
{
	CMdbTableDef* pTableDef = m_oMdbDef.GetTableDefine(sTableName);
	if(pTableDef)
	{
		CMdbTable* pTable = (CMdbTable*)pTableDef->pExtendAttr->pInstance;
		pTable->RegQueryTrigger(Trigger, pContext, nCallerMask);
	}
}

void CLocalMdb::ProcessTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CMdbTriggerItem>* pTriggerList)
{
	m_oMutex.Enter();
	void* pIt = pTriggerList->First();
	while(pIt)
	{
		CMdbTriggerItem& oItem = pTriggerList->GetItem(pIt);
		if(oItem.nCallerMask & nCaller)
			oItem.Trigger(pAccess, nCaller, oItem.pContext);
		pIt = pTriggerList->GetNext(pIt);
	}
	m_oMutex.Leave();
}

FOCP_END();
