
#include "RdbTab.hpp"

FOCP_BEGIN();
CRedoLog* g_pRedo = NULL;

//-----------------------------------------------------
// CRdbTable
//-----------------------------------------------------
CRdbTable::CRdbTable()
{
	m_nThis = 0;
	m_pTabDef = NULL;
	m_pTableSpace = NULL;
	m_pDataTable = NULL;
	m_pIndexTable = NULL;
	m_pRowIndex = NULL;
	m_nIndexCount = 0;
}

CRdbTable::~CRdbTable()
{
	if(m_nThis)
	{
		Distruction();
		m_nThis = 0;
	}
}

bool CRdbTable::IsEmptyTable()
{
	return (m_pDataTable->Begin() == m_pDataTable->End());
}

void CRdbTable::Construction(CTableDefine* pTabDef)
{
	m_pTabDef = pTabDef;
	m_pTabDef->SetInstance(this);
	m_pTableSpace = new CTableSpace(m_pTabDef);
	m_pRowIndex = new CRdbRowIndex(m_pTabDef, m_pTableSpace);
	m_pDataTable = m_pRowIndex->m_pDataTable;
	m_nIndexCount = pTabDef->m_nIndexCount;
	m_pIndexTable = NULL;
	if(m_nIndexCount)
	{
		m_pIndexTable = new CAbstractIndex*[m_nIndexCount];
		for(uint32 i=0; i<m_nIndexCount; ++i)
		{
			CIndexDefine* pIndexDefine = m_pTabDef->m_pIndexDefineSet[i];
			if(!pIndexDefine)
				m_pIndexTable[i] = NULL;
			else
			{
				CRdbIndexDef* pIdxAttr = pIndexDefine->m_pBaseAttr;
				switch(pIdxAttr->nArithmetic)
				{
				case RDB_RBTREE_INDEX:
					m_pIndexTable[i] = new CRdbRbTreeIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
					break;
				case RDB_NTREE_INDEX:
					m_pIndexTable[i] = new CRdbPrefixIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
					break;
				case RDB_HASH_INDEX:
					m_pIndexTable[i] = new CRdbHashIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
					break;
				}
			}
		}
	}
}

void CRdbTable::Distruction()
{
	m_pTabDef->SetInstance(NULL);
	uint32 nIndexCount = m_nIndexCount;
	if(nIndexCount)
	{
		for(uint32 i=0; i<nIndexCount; ++i)
			if(m_pIndexTable[i])
				delete m_pIndexTable[i];
		delete[] m_pIndexTable;
	}
	delete m_pRowIndex;
	delete m_pTableSpace;
	m_pTabDef = NULL;
	m_pTableSpace = NULL;
	m_pDataTable = NULL;
	m_pIndexTable = NULL;
	m_pRowIndex = NULL;
	m_nIndexCount = 0;
}

uint64 CRdbTable::GetThis()
{
	return m_nThis;
}

CRdbTable& CRdbTable::SetThis(CTableDefine* pTabDef)
{
	if(m_nThis)
		Distruction();
	m_pTabDef = pTabDef;
	m_nThis = pTabDef->m_pBaseAttr->nStorageAddr;
	if(m_nThis)
	{
		Construction(pTabDef);
		uint64 nAddrTable[2];
		vquery(m_nThis, (char*)nAddrTable, 16);
		m_pTableSpace->SetThis(nAddrTable[0]);
		m_pRowIndex->SetThis(nAddrTable[1]);
		uint32 nIndexCount = m_nIndexCount;
		if(nIndexCount)
		{
			for(uint32 i=0; i<nIndexCount; ++i)
			{
				if(m_pIndexTable[i])
				{
					CIndexDefine * pIndex = m_pTabDef->m_pIndexDefineSet[i];
					m_pIndexTable[i]->SetThis(pIndex->m_pBaseAttr->nStorageAddr);
				}
			}
		}
	}
	return *this;
}

bool CRdbTable::CreateObject(CTableDefine* pTabDef)
{
	if(m_nThis)
		Distruction();
	m_pTabDef = pTabDef;
	int32 bInMemory = (m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE);
	m_nThis = vmalloc(16, bInMemory);
	if(!m_nThis)
		return false;
	Construction(pTabDef);
	uint64 nAddrTable[2];
	if(!m_pTableSpace->CreateObject())
	{
		Distruction();
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	nAddrTable[0] = m_pTableSpace->GetThis();
	if(!m_pRowIndex->CreateObject())
	{
		m_pTableSpace->DestroyObject();
		Distruction();
		vfree(m_nThis);
		m_nThis = 0;
		return false;
	}
	nAddrTable[1] = m_pRowIndex->GetThis();
	uint32 nIndexCount = m_pTabDef->m_nIndexCount;
	for(uint32 i=0; i<nIndexCount; ++i)
	{
		if(m_pIndexTable[i])
		{
			CIndexDefine * pIndex = m_pTabDef->m_pIndexDefineSet[i];
			if(!m_pIndexTable[i]->CreateObject())
			{
				for(uint32 j=0; j<i; ++j)
				{
					m_pIndexTable[i]->DestroyObject();
					pIndex->SetStorageAddr(0);
				}
				m_pRowIndex->DestroyObject();
				m_pTableSpace->DestroyObject();
				Distruction();
				vfree(m_nThis);
				m_nThis = 0;
				return false;
			}
			pIndex->SetStorageAddr(m_pIndexTable[i]->GetThis());
		}
	}
	vcommit(m_nThis, (char*)nAddrTable, 16);
	pTabDef->SetStorageAddr(m_nThis);

	return true;
}

void CRdbTable::DestroyObject()
{
	if(m_nThis)
	{
		m_pTabDef->SetStorageAddr(0);
		Truncate();
		uint32 nIndexCount = m_nIndexCount;
		if(nIndexCount)
		{
			for(uint32 i=0; i<nIndexCount; ++i)
			{
				if(m_pIndexTable[i])
				{
					CIndexDefine * pIndex = m_pTabDef->m_pIndexDefineSet[i];
					m_pIndexTable[i]->DestroyObject();
					pIndex->SetStorageAddr(0);
				}
			}
		}
		m_pRowIndex->DestroyObject();
		m_pTableSpace->DestroyObject();
		Distruction();
		vfree(m_nThis);
		m_nThis = 0;
	}
}

void CRdbTable::FillIndexFilter(CSqlFilter& oFilter)
{
	for(register uint32 i=0; i<m_nIndexCount; ++i)
	{
		CIndexDefine* pIdxDef = m_pTabDef->m_pIndexDefineSet[i];
		register uint32 nFieldCount = pIdxDef->m_nFieldCount;
		for(register uint32 j=0; j<nFieldCount; ++j)
			oFilter.SetField(pIdxDef->m_pFields[j]);
	}
}

uint32 CRdbTable::GetIndexCount()
{
	if(!m_nThis)
		return 0;
	return m_nIndexCount;
}

CAbstractIndex* CRdbTable::GetIndex(uint32 nIndexNo)
{
	if(!m_nThis || nIndexNo >= m_nIndexCount)
		return NULL;
	return m_pIndexTable[nIndexNo];
}

void CRdbTable::DropIndex(uint32 nIndexNo)
{
	uint32 nIndexCount = m_nIndexCount;
	if(nIndexNo < nIndexCount)
	{
		CAbstractIndex* pIndex = m_pIndexTable[nIndexNo];
		if(pIndex)
		{
			CIndexDefine * pIndexDefine = m_pTabDef->m_pIndexDefineSet[nIndexNo];
			pIndex->Truncate();
			pIndex->DestroyObject();
			m_pIndexTable[nIndexNo] = NULL;
			pIndexDefine->SetStorageAddr(0);
		}
	}
}

uint32 CRdbTable::CreateNewIndex(uint32 nIndexNo)
{
	if(nIndexNo >= m_nIndexCount)
	{
		uint32 i, nTotalIndexCount = m_pTabDef->m_nIndexCount;
		CAbstractIndex** pNewIndexTable = new CAbstractIndex*[nTotalIndexCount];
		if(!pNewIndexTable)
			return RDB_LACK_MEMORY;
		for(i=0; i<m_nIndexCount; ++i)
			pNewIndexTable[i] = m_pIndexTable[i];
		for(; i<nTotalIndexCount; ++i)
			pNewIndexTable[i] = NULL;
		delete[] m_pIndexTable;
		m_pIndexTable = pNewIndexTable;
		m_nIndexCount = nTotalIndexCount;
	}
	CIndexDefine* pIndexDefine = m_pTabDef->m_pIndexDefineSet[nIndexNo];
	CRdbIndexDef* pIdxAttr = pIndexDefine->m_pBaseAttr;
	CAbstractIndex* pIndex;
	switch(pIdxAttr->nArithmetic)
	{
	case RDB_RBTREE_INDEX:
		pIndex = new CRdbRbTreeIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
		break;
	case RDB_NTREE_INDEX:
		pIndex = new CRdbPrefixIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
		break;
	case RDB_HASH_INDEX:
		pIndex = new CRdbHashIndex(m_pTabDef, pIndexDefine, m_pTableSpace, m_pDataTable);
		break;
	}
	CIndexDefine* pPriIndexDef = pIndexDefine->m_pPrimaryIndex;
	if(pPriIndexDef)
	{
		CAbstractIndex* pPriIndex = (CAbstractIndex*)pPriIndexDef->m_pInstance;
		pIndex->SetPrimaryIndex(pPriIndex);
	}
	if(!pIndex->CreateObject())
	{
		delete pIndex;
		return RDB_LACK_STORAGE;
	}
	uint32 nRet = pIndex->LoadAllData();
	if(nRet)
	{
		pIndex->DestroyObject();
		delete pIndex;
		return nRet;
	}
	m_pIndexTable[nIndexNo] = pIndex;
	pIndexDefine->SetStorageAddr(pIndex->GetThis());
	return RDB_SUCCESS;
}

void CRdbTable::Truncate()
{
	if(m_nThis)
	{
		m_oLock.EnterWrite();
		uint32 nIndexCount = m_nIndexCount;
		for(uint32 i=0; i<nIndexCount; ++i)
		{
			if(m_pIndexTable[i])
				m_pIndexTable[i]->Truncate();
		}
		m_pRowIndex->Truncate();
		m_pTableSpace->Truncate();
		m_oLock.LeaveWrite();
	}
}

typedef struct CIndexChose
{
	CAbstractIndex* pIndex;
	uint32 nFieldCount;
	uint32 nMatchFieldCount;
	uint32 nArithmetic;
	uint32 nQualifier;
	bool Matched[RDB_MAX_INDEX_FIELD_NUM];
}CIndexChose;

CAbstractIndex* CRdbTable::IndexChoice(CSqlParameterSet* pCondition)
{
	uint32 i;
	uint32 nFlagCount = m_pTabDef->GetFieldFlagCount();
	uint32 pFlag[RDB_FLG_L(RDB_MAX_FIELD_NUM-1)+1];// = new uint32[nFlagCount];
	uint32 pIsRange[RDB_MAX_FIELD_NUM];// = new uint32[nFieldCount];
	for(i=0; i<nFlagCount; ++i)
		pFlag[i] = 0;
	if(!GetFieldAttrTable(pFlag, pIsRange, pCondition))
		return m_pRowIndex;
	uint32 j,nIndexCount = 0;
	CIndexChose pIndexChoseTable[RDB_MAX_INDEX_NUM];// = new CIndexChose[m_nIndexCount];
	for(i=0; i<m_nIndexCount; ++i)
	{
		if(!m_pIndexTable[i])
			continue;
		CIndexChose& oIndexChose = pIndexChoseTable[nIndexCount];
		CIndexDefine* pIndexDefine = m_pTabDef->m_pIndexDefineSet[i];
		oIndexChose.pIndex = m_pIndexTable[i];
		oIndexChose.nFieldCount = pIndexDefine->m_nFieldCount;
		oIndexChose.nMatchFieldCount = 0;
		oIndexChose.nArithmetic = pIndexDefine->m_pBaseAttr->nArithmetic;
		oIndexChose.nQualifier = pIndexDefine->m_pBaseAttr->nQualifier;
		bool bSupportRange = (oIndexChose.nArithmetic == RDB_RBTREE_INDEX);
		for(j=0; j<oIndexChose.nFieldCount; ++j)
		{
			uint32 nFieldNo = pIndexDefine->m_pFields[j];
			uint32 nFlag = RDB_FLG_L(nFieldNo);
			uint32 nBit = RDB_FLG_B(nFieldNo);
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
		if(oIndexChose.nArithmetic == RDB_NTREE_INDEX)
			return oIndexChose.pIndex;
		if(oIndexChose.nArithmetic == RDB_HASH_INDEX)
		{
			if(oIndexChose.nMatchFieldCount != oIndexChose.nFieldCount)
				continue;
		}
		++nIndexCount;
	}
	if(!nIndexCount)
		return m_pRowIndex;

	uint32 nChoice[2], nMaxFieldCount[2];
	nMaxFieldCount[0] = nMaxFieldCount[1] = RDB_MAX_INDEX_FIELD_NUM+1;
	for(i=0; i<nIndexCount; ++i)
	{
		CIndexChose &oIndexChose = pIndexChoseTable[i];
		if(oIndexChose.nQualifier == RDB_UNIQUE_INDEX)
		{
			if(oIndexChose.nArithmetic == RDB_HASH_INDEX)
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
		if(nMaxFieldCount[i] <= RDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[i]].pIndex;
	}

	nMaxFieldCount[0] = nMaxFieldCount[1] = RDB_MAX_INDEX_FIELD_NUM+1;
	for(i=0; i<nIndexCount; ++i)
	{
		CIndexChose &oIndexChose = pIndexChoseTable[i];
		if(oIndexChose.nFieldCount == oIndexChose.nMatchFieldCount)
		{
			if(oIndexChose.nArithmetic == RDB_HASH_INDEX)
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
		if(nMaxFieldCount[i] <= RDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[i]].pIndex;
	}

	for(i=0; i<3; ++i)
	{
		nMaxFieldCount[1] = RDB_MAX_INDEX_FIELD_NUM+1;
		for(j=0; j<nIndexCount; ++j)
		{
			CIndexChose &oIndexChose = pIndexChoseTable[j];
			if(oIndexChose.nArithmetic == RDB_RBTREE_INDEX)
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
		if(nMaxFieldCount[1] <= RDB_MAX_INDEX_FIELD_NUM)
			return pIndexChoseTable[nChoice[1]].pIndex;
	}

	return m_pRowIndex;
}

uint32 CRdbTable::GetFieldAttrTable(uint32* pFlag, uint32* pIsRange, CSqlParameterSet* pCondition)
{
	uint32 nCount = 0;
	uint32 nParameterCount = pCondition->m_nParaCount;
	for(uint32 i=0; i<nParameterCount; ++i)
	{
		bool bParaSet;
		void * pPara = pCondition->GetPara(i, bParaSet);
		if(bParaSet)
			nCount += GetFieldAttrTable(pFlag, pIsRange, (CSqlParameterSet*)pPara);
		else
		{
			CSqlParameter* pParameter = (CSqlParameter*)pPara;
			uint32 nParaCount = pParameter->m_nParaCount;
			for(uint32 j=0; j<nParaCount; ++j)
			{
				uint32 nOperator, nFieldNo;
				pParameter->GetPara(j, &nOperator, &nFieldNo);
				uint32 nFlag = RDB_FLG_L(nFieldNo);
				uint32 nBit = RDB_FLG_B(nFieldNo);
				if(pFlag[nFlag] & nBit)
				{
					uint32 &bIsRange = pIsRange[nFieldNo];
					if(!bIsRange)
						bIsRange = (nOperator!=RDB_SQLPARA_OPERATOR_EQUAL);
				}
				else
				{
					pIsRange[nFieldNo] = (nOperator!=RDB_SQLPARA_OPERATOR_EQUAL);
					pFlag[nFlag] |= nBit;
					++nCount;
				}
			}
		}
	}
	return nCount;
}

uint32 CRdbTable::Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter,
						CRecordSet &oResult, uint32 nSkipCount)
{
	CAbstractIndex* pIndex = IndexChoice(pCondition);
	m_oLock.EnterRead();
	uint32 nRet = pIndex->Query(pCondition, oFilter, oResult, nSkipCount);
	m_oLock.LeaveRead();
	return nRet;
}

uint32 CRdbTable::Query(uint64 nRowId, CSqlFilter& oFilter, CRecordSet &oResult)
{
	CRecord* pRecord = NULL;
	if(!oFilter.m_nParaCount)
		oFilter.SetAllField();
	m_oLock.EnterRead();
	CDataTable::CIterator it = m_pDataTable->Find(nRowId);
	if(it != m_pDataTable->End())
	{
		CRdbRecord oRdbRecord(m_pTableSpace, m_pTabDef, it.GetValue());
		pRecord = oResult.AllocRecord();
		if(!pRecord)
		{
			m_oLock.LeaveRead();
			return RDB_LACK_MEMORY;
		}
		oRdbRecord.GetRecord(pRecord, oFilter);
	}
	m_oLock.LeaveRead();
	return RDB_SUCCESS;
}

uint32 CRdbTable::QueryCount(CSqlParameterSet * pCondition, uint32 &nCount)
{
	CAbstractIndex* pIndex = IndexChoice(pCondition);
	m_oLock.EnterRead();
	uint32 nRet = pIndex->QueryCount(pCondition, nCount);
	m_oLock.LeaveRead();
	return nRet;
}

uint32 CRdbTable::Exist(CSqlParameterSet * pCondition, uint32& bExist)
{
	CAbstractIndex* pIndex = IndexChoice(pCondition);
	m_oLock.EnterRead();
	uint32 nRet = pIndex->Exist(pCondition, bExist);
	m_oLock.LeaveRead();
	return nRet;
}

uint32 CRdbTable::Insert(CSqlParameter* pInsertAttr)
{
	CRecord& oRecord = pInsertAttr->m_oRecord;
	uint32 i, nFieldCount = m_pTabDef->m_nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		if(!IS_SET_FIELD(pInsertAttr, i))
		{
			CRdbFieldDef* pFieldDefine = m_pTabDef->m_pFieldDefineSet[i]->m_pBaseAttr;
			if(pFieldDefine->nJob >= 0)
			{
				if(pFieldDefine->sDefault && pFieldDefine->sDefault[0])
				{
					oRecord.GetField(i)->SetFromString(pFieldDefine->sDefault);
					pInsertAttr->SetField(i, RDB_SQLPARA_OPERATOR_EQUAL);
				}
				else if(pFieldDefine->bNotNull)
					return RDB_FIELD_ISNULL;
			}
		}
	}

	m_oLock.EnterWrite();
	CDataTable::CIterator idx = m_pDataTable->Insert();
	if(idx == m_pDataTable->End())
	{
		m_oLock.LeaveWrite();
		return RDB_LACK_STORAGE;
	}

	uint32 nRet = RDB_SUCCESS;
	CKernelIndexItem oItem = *idx.GetValue();
	{// let oDbRecord can call ~CRdbRecord before WriteLeaveSection
		CRdbRecord oDbRecord(m_pTableSpace, m_pTabDef, &oItem);
		i = oDbRecord.CopyRecord(oRecord, pInsertAttr);
		if(i < pInsertAttr->m_nParaCount)
		{
			oDbRecord.Clear(pInsertAttr, i);
			m_pDataTable->Erase(idx);
			nRet = RDB_LACK_STORAGE;
		}
		if(m_nIndexCount)
		{
			oRecord.SetRowId(oItem.nRowId);
			for(i=0; i<m_nIndexCount; ++i)
			{
				if(!m_pIndexTable[i])
					continue;
				nRet = m_pIndexTable[i]->AttachRecord(&oRecord);
				if(nRet)
				{
					for(uint32 j=0; j<i; ++j)
						if(m_pIndexTable[i])
							m_pIndexTable[i]->DetachRecord(&oRecord);
					oDbRecord.Clear();
					m_pDataTable->Erase(idx);
					break;
				}
			}
		}
	}
	idx.Detach();
	m_oLock.LeaveWrite();
	return RDB_SUCCESS;
}

uint32 DeleteRdbRecord(CAbstractIndex* pIndex, CDataTable::CIterator& oIt, CRecord* pRecord, void*)
{
	CRdbTable* pTable = (CRdbTable*)pIndex->m_pTabDef->m_pInstance;
	pTable->Delete(oIt, pRecord);
	return RDB_SUCCESS;
}

uint32 CRdbTable::Delete(CSqlParameterSet* pCondition, uint32 &nRemovedCount)
{
	CSqlFilter oFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	FillIndexFilter(oFilter);
	CAbstractIndex* pIndex = IndexChoice(pCondition);
	m_oLock.EnterWrite();
	uint32 nRet = pIndex->Travel(pCondition, oFilter, false, DeleteRdbRecord, nRemovedCount);
	m_oLock.LeaveWrite();
	return nRet;
}

void CRdbTable::Delete(CDataTable::CIterator& oIt, CRecord* pRecord)
{
	for(uint32 i=0; i<m_nIndexCount; ++i)
	{
		if(!m_pIndexTable[i])
			continue;
		CAbstractIndex* pIndex = m_pIndexTable[i];
		CIndexDefine* pIndexDefine = pIndex->m_pIdxDef;
		uint32 nDetailIndexCount = pIndex->m_nDetailIndexCount;
		uint32 nFieldCount = pIndexDefine->m_nFieldCount;
		for(uint32 j=0; j<nDetailIndexCount; ++j)
		{
			uint32 nCount;
			CAbstractIndex* pDetailIndex = pIndex->m_pDetailIndexes[j];
			CTableDefine* pDetailTabDef = pDetailIndex->m_pTabDef;
			CRdbTable* pDetailTable = (CRdbTable*)pDetailTabDef->m_pInstance;
			CSqlParameterSet oNewCondition(pDetailTabDef);
			CSqlParameter* pPara = (CSqlParameter*)oNewCondition.AddPara();
			CRecord& oLRecord = pPara->m_oRecord;
			for(uint32 k=0; k<nFieldCount; ++k)
			{
				uint32 nRightFieldNo = pIndexDefine->m_pFields[k];
				char* sFieldName = m_pTabDef->m_pFieldDefineSet[nRightFieldNo]->m_pBaseAttr->sFieldName;
				uint32 nLeftFieldNo = pDetailTabDef->GetFieldNo(sFieldName);
				oLRecord.GetField(nLeftFieldNo)->SetFromField(pRecord->GetField(nRightFieldNo));
				pPara->SetField(nLeftFieldNo, RDB_SQLPARA_OPERATOR_EQUAL);
			}
			pDetailTable->Delete(&oNewCondition, nCount);
		}
		pIndex->DetachRecord(pRecord);
	}
	if(1)
	{
		CRdbRecord oDbRecord(m_pTableSpace, m_pTabDef, oIt.GetValue());
		oDbRecord.Clear();
	}
	oIt = m_pDataTable->Erase(oIt);
}

struct CRdbUpdateParameter
{
	CRecordSet* pRollBack;
	CSqlParameter* pSetAttr;
	uint32* pModifiedCount;
	bool bNeedDetach;
	CSqlFilter* pFilter;
};

uint32 UpdateRdbRecord(CAbstractIndex* pIndex, CDataTable::CIterator& oIt, CRecord* pRecord, void* pPara)
{
	CRdbUpdateParameter* pUpdatePara = (CRdbUpdateParameter*)pPara;
	CRdbTable* pTable = (CRdbTable*)pIndex->m_pTabDef->m_pInstance;
	return pTable->Update(oIt, pRecord, pUpdatePara->pSetAttr, pUpdatePara->pRollBack,
		pUpdatePara->pModifiedCount, pUpdatePara->bNeedDetach, *pUpdatePara->pFilter);
}

uint32 CRdbTable::Update(CSqlParameterSet* pCondition, CSqlParameter* pSetAttr,
						 CRecordSet &oRollBack, uint32 &nModifiedCount)
{
	uint32 nOperator, nFieldNo;

	CRecord& oSetRecord = pSetAttr->m_oRecord;
	uint32 i, nSetCount = pSetAttr->m_nParaCount;
	for(i=0; i<nSetCount; ++i)
	{
		pSetAttr->GetPara(i, &nOperator, &nFieldNo);
		if(m_pTabDef->m_pFieldDefineSet[nFieldNo]->m_pBaseAttr->bNotNull &&
			oSetRecord.IsNull(nFieldNo))
			return RDB_FIELD_ISNULL;
	}

	CSqlFilter oFilter(m_pTabDef), oIndexFilter(m_pTabDef);
	pCondition->GetFilter(oFilter);
	CRdbUpdateParameter oUpdateParameter = {NULL, pSetAttr, &nModifiedCount, false, &oIndexFilter};
	for(i=0; i<m_nIndexCount; ++i)
	{
		CAbstractIndex* pIndex = m_pIndexTable[i];
		if(!pIndex)
			continue;
		CIndexDefine* pIndexDefine = pIndex->m_pIdxDef;
		uint32 nQualifier = pIndexDefine->m_pBaseAttr->nQualifier;
		uint32 nFieldCount = pIndexDefine->m_nFieldCount;
		if(nQualifier == RDB_UNIQUE_INDEX)
		{
			for(uint32 j=0; j<nFieldCount; ++j)
			{
				nFieldNo = pIndexDefine->m_pFields[j];
				oFilter.SetField(nFieldNo);
				oIndexFilter.SetField(nFieldNo);
				if(IS_SET_FIELD(pSetAttr, nFieldNo))
				{
					oUpdateParameter.pRollBack = &oRollBack;
					oUpdateParameter.bNeedDetach = true;
					break;
				}
			}
		}
		else
		{
			for(uint32 j=0; j<nFieldCount; ++j)
			{
				nFieldNo = pIndexDefine->m_pFields[j];
				oFilter.SetField(nFieldNo);
				oIndexFilter.SetField(nFieldNo);
				if(IS_SET_FIELD(pSetAttr, nFieldNo))
				{
					oUpdateParameter.bNeedDetach = true;
					break;
				}
			}
		}
		if(oUpdateParameter.pRollBack)
			break;
	}
	nModifiedCount = 0;
	CAbstractIndex* pIndex = IndexChoice(pCondition);
	if(oUpdateParameter.bNeedDetach)
	{
		FillIndexFilter(oFilter);
		m_oLock.EnterWrite();
	}
	else
		m_oLock.EnterRead();
	uint32 nRet = pIndex->Travel(pCondition, oFilter, true, UpdateRdbRecord, nModifiedCount, &oUpdateParameter);
	if(oUpdateParameter.bNeedDetach)
		m_oLock.LeaveWrite();
	else
		m_oLock.LeaveRead();
	if(nRet)
		nModifiedCount = 0;
	return nRet;
}

uint32 CRdbTable::Update(CDataTable::CIterator& oIt, CRecord* pRecord, CSqlParameter* pSetAttr,
						 CRecordSet* pRollBack, uint32* pModifiedCount,
						 bool bNeedDetach, CSqlFilter& oFilter)
{
	if(pRollBack && pModifiedCount[0] >= pRollBack->GetRecordCount())
		return 1;//Roll back lack;
	CKernelIndexItem& oKernelItem = *oIt.GetValue();
	CRdbRecord oDbRecord(m_pTableSpace, m_pTabDef, &oKernelItem);
	CRecord& oSetRecord = pSetAttr->m_oRecord;
	uint32 nSetCount = pSetAttr->m_nParaCount;
	CRecord* pRollRecord = NULL;
	if(pRollBack)
	{
		pRollRecord = pRollBack->GetRecord(pModifiedCount[0]);
		pRollRecord->SetRowId(oKernelItem.nRowId);
	}
	if(bNeedDetach)
	{
		for(uint32 i=0; i<m_nIndexCount; ++i)
		{
			CAbstractIndex* pIndex = m_pIndexTable[i];
			if(!pIndex)
				continue;
			CIndexDefine* pIndexDefine = pIndex->m_pIdxDef;
			uint32 nQualifier = pIndexDefine->m_pBaseAttr->nQualifier;
			uint32 j, nFieldCount = pIndexDefine->m_nFieldCount;
			CSqlParameterSet* pCond = NULL;
			CSqlParameter* pCondPara = NULL;
			CRecord* pCondRecord = NULL;
			if(nQualifier == RDB_UNIQUE_INDEX)
			{
				pCond = new CSqlParameterSet(m_pTabDef);
				pCondPara = (CSqlParameter*)pCond->AddPara();
				pCondRecord = &pCondPara->m_oRecord;
			}
			pIndex->DetachRecord(pRecord);
			for(j=0; j<nFieldCount; ++j)
			{
				uint32 nFieldNo = pIndexDefine->m_pFields[j];
				CField* pOrigValue = pRecord->GetField(nFieldNo);

				if(pRollRecord)
					pRollRecord->GetField(nFieldNo)->SetFromField(pOrigValue);

				if(IS_SET_FIELD(pSetAttr, nFieldNo))
				{
					CField* pSetValue = oSetRecord.GetField(nFieldNo);
					if(pCond)
					{
						pCondPara->SetField(nFieldNo, RDB_SQLPARA_OPERATOR_EQUAL);
						pCondRecord->GetField(nFieldNo)->SetFromField(pSetValue);
					}
					pOrigValue->SetFromField(pSetValue);
				}
				else if(pCond)
				{
					pCondPara->SetField(nFieldNo, RDB_SQLPARA_OPERATOR_EQUAL);
					pCondRecord->GetField(nFieldNo)->SetFromField(pOrigValue);
				}
			}
			if(pCond)
			{
				uint32 bExist = 0;
				pIndex->Exist(pCond, bExist);
				if(bExist)
				{
					pIndex->AttachRecord(pRollRecord);
					RollBack(oDbRecord, pRollBack, pModifiedCount[0], i, oFilter, pSetAttr);
					return 1;
				}
			}
			pIndex->AttachRecord(pRecord);
		}
	}
	for(uint32 i=0; i<nSetCount; ++i)
	{
		uint32 nOperator, nFieldNo;
		pSetAttr->GetPara(i, &nOperator, &nFieldNo);
		CRdbField* pRdbField = oDbRecord.GetField(nFieldNo);
		if(pRollRecord && !IS_SET_FIELD0(oFilter, nFieldNo))
			pRdbField->GetField(pRollRecord->GetField(nFieldNo));
		pRdbField->SetField(oSetRecord.GetField(nFieldNo));
	}
	return 0;
}

void CRdbTable::RollBack(CRdbRecord& oCurDbRecord, CRecordSet* pRollBack, uint32 nLastRecordNo, uint32 nLastIndexNo, CSqlFilter& oFilter, CSqlParameter* pSetAttr)
{
	uint32 i, j;
	CRecord oNewRecord(m_pTabDef);
	for(i=0; i<nLastRecordNo; ++i)
	{
		CRecord* pOldRecord = pRollBack->GetRecord(i);
		uint64 nRowId = pOldRecord->m_nRowId;
		CDataTable::CIterator oIt = m_pDataTable->Find(nRowId);
		CKernelIndexItem& oKernelItem = *oIt.GetValue();
		CRdbRecord oDbRecord(m_pTableSpace, m_pTabDef, &oKernelItem);
		oDbRecord.GetRecord(&oNewRecord, oFilter);
		for(j=0; j<m_nIndexCount; ++j)
		{
			CAbstractIndex* pIndex = m_pIndexTable[j];
			if(!pIndex)
				continue;
			pIndex->DetachRecord(&oNewRecord);
			pIndex->AttachRecord(pOldRecord);
		}
		uint32 nSetCount = pSetAttr->m_nParaCount;
		for(j=0; j<nSetCount; ++j)
		{
			uint32 nOperator, nFieldNo;
			pSetAttr->GetPara(j, &nOperator, &nFieldNo);
			oDbRecord.GetField(nFieldNo)->SetField(pOldRecord->GetField(nFieldNo));
		}
	}
	CRecord* pOldRecord = pRollBack->GetRecord(i);
	oCurDbRecord.GetRecord(&oNewRecord, oFilter);
	for(j=0; j<nLastIndexNo; ++j)
	{
		CAbstractIndex* pIndex = m_pIndexTable[j];
		if(!pIndex)
			continue;
		pIndex->DetachRecord(&oNewRecord);
		pIndex->AttachRecord(pOldRecord);
	}
}

//-----------------------------------------------------
// CRdbTableAccess
//-----------------------------------------------------
CRdbTableAccess::CRdbTableAccess(void* pDb, CTableDefine* pTabDef, CRwMutex* pLock):
m_oIdxAttr(pTabDef),
m_oInsertAttr(pTabDef),
m_oSetAttr(pTabDef),
m_oGetAttr(pTabDef),
m_oRollBack(pTabDef),
m_oFilter(pTabDef)
{
	m_pDb = pDb;
	m_pLock = pLock;
	m_pTabDef = pTabDef;
	m_pTable = (CRdbTable*)pTabDef->m_pInstance;
	m_nLocked = 0;
}

CRdbTableAccess::~CRdbTableAccess()
{
}

uint32 CRdbTableAccess::GetFieldCount()
{
	return m_pTabDef->m_nFieldCount;
}

bool CRdbTableAccess::IsValidField(uint32 nFieldNo)
{
	return m_pTabDef->IsValidField(nFieldNo);
}

uint32 CRdbTableAccess::GetFieldNo(const char* sFieldName)
{
	return m_pTabDef->GetFieldNo((char*)sFieldName);
}

CRdbFilter* CRdbTableAccess::GetResultFilter()
{
	m_oFilter.Clear();
	return &m_oFilter;
}

CRdbResultSet* CRdbTableAccess::GetResultSet()
{
	return &m_oGetAttr;
}

CRdbParaSet* CRdbTableAccess::GetQueryPara()
{
	m_oIdxAttr.Clear();
	m_oFilter.Clear();
	return &m_oIdxAttr;
}

CRdbPara* CRdbTableAccess::GetUpdatePara()
{
	m_oSetAttr.Clear();
	return &m_oSetAttr;
}

CRdbPara* CRdbTableAccess::GetInsertPara()
{
	m_oInsertAttr.Clear();
	return &m_oInsertAttr;
}

void CRdbTableAccess::Truncate()
{
	EnterSystemRead();
	if(WriteRedoForTruncate())
	{
		FocpLogEx(m_pTabDef->GetLogName(), FOCP_LOG_ERROR, ("WriteRedoForTruncate '%s' failure", m_pTabDef->m_pBaseAttr->sTableName));
		LeaveSystemRead();
		return;
	}
	m_pTable->Truncate();
	LeaveSystemRead();
}

uint32 CRdbTableAccess::Query(uint32 nMaxRecordQuantity, uint32 nSkipCount)
{
	m_oGetAttr.SetRecordSetSize(nMaxRecordQuantity);
	return m_pTable->Query(&m_oIdxAttr, m_oFilter, m_oGetAttr, nSkipCount);
}

uint32 CRdbTableAccess::Query(uint64 nRowId)
{
	m_oGetAttr.SetRecordSetSize(1);
	return m_pTable->Query(nRowId, m_oFilter, m_oGetAttr);
}

uint32 CRdbTableAccess::QueryCount(uint32 &nCount)
{
	return m_pTable->QueryCount(&m_oIdxAttr, nCount);
}

uint32 CRdbTableAccess::Exist(uint32& bExist)
{
	return m_pTable->Exist(&m_oIdxAttr, bExist);
}

uint32 CRdbTableAccess::Insert()
{
	EnterSystemRead();
	if(WriteRedoForInsert())
	{
		FocpLogEx(m_pTabDef->GetLogName(), FOCP_LOG_ERROR, ("WriteRedoForInsert '%s' failure", m_pTabDef->m_pBaseAttr->sTableName));
		LeaveSystemRead();
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = m_pTable->Insert(&m_oInsertAttr);
	LeaveSystemRead();
	return nRet;
}

uint32 CRdbTableAccess::Delete()
{
	uint32 nCount;
	EnterSystemRead();
	if(WriteRedoForDelete())
	{
		FocpLogEx(m_pTabDef->GetLogName(), FOCP_LOG_ERROR, ("WriteRedoForDelete '%s' failure", m_pTabDef->m_pBaseAttr->sTableName));
		LeaveSystemRead();
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = m_pTable->Delete(&m_oIdxAttr, nCount);
	LeaveSystemRead();
	return nRet;
}

uint32 CRdbTableAccess::Update(uint32 nMaxRollBackRecordQuantity)
{
	uint32 nCount;
	EnterSystemRead();
	m_oRollBack.SetRecordSetSize(nMaxRollBackRecordQuantity);
	m_oRollBack.PreAlloc();
	if(WriteRedoForUpdate(nMaxRollBackRecordQuantity))
	{
		FocpLogEx(m_pTabDef->GetLogName(), FOCP_LOG_ERROR, ("WriteRedoForUpdate '%s' failure", m_pTabDef->m_pBaseAttr->sTableName));
		LeaveSystemRead();
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = m_pTable->Update(&m_oIdxAttr, &m_oSetAttr, m_oRollBack, nCount);
	LeaveSystemRead();
	return nRet;
}

void CRdbTableAccess::Release()
{
	if(m_nLocked)
	{
		m_pLock->LeaveRead();
		m_nLocked = 0;
	}
}

uint32 CRdbTableAccess::WriteRedoForTruncate()
{
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE)
		return 0;
	char sRedo[256], *pRedo = sRedo;
	const char* sLogName = m_pTabDef->GetLogName();
	const char* sTableName = (const char*)m_pTabDef->m_pBaseAttr->sTableName;
	*(uint32*)pRedo = RDO_TRUNCATE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sLogName);
	pRedo += CString::StringLength(sLogName) + 1;
	CString::StringCopy(pRedo, sTableName);
	pRedo += CString::StringLength(sTableName) + 1;
	return g_pRedo->WriteRedo(sRedo, pRedo-sRedo);
}

void CRdbTableAccess::ReTruncate(char*, uint32)
{
	m_pTable->Truncate();
}

uint32 CRdbTableAccess::WriteRedoForInsert()
{
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE)
		return 0;

	char sRedo[256], *pRedo = sRedo;
	const char* sLogName = m_pTabDef->GetLogName();
	const char* sTableName = (const char*)m_pTabDef->m_pBaseAttr->sTableName;
	*(uint32*)pRedo = RDO_INSERT;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sLogName);
	pRedo += CString::StringLength(sLogName) + 1;
	CString::StringCopy(pRedo, sTableName);
	pRedo += CString::StringLength(sTableName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
		return 1;
	if(!m_oInsertAttr.Write(oStream))
		return 1;
	return g_pRedo->WriteRedoStream(&oStream);
}

void CRdbTableAccess::ReInsert(char* sRedo, uint32 nSize)
{
	CMemoryStream oStream(sRedo, nSize);
	oStream.SetLocalCode(true);
	if(!m_oInsertAttr.Read(oStream))
		return;
	m_pTable->Insert(&m_oInsertAttr);
}

uint32 CRdbTableAccess::WriteRedoForDelete()
{
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE)
		return 0;

	char sRedo[256], *pRedo = sRedo;
	const char* sLogName = m_pTabDef->GetLogName();
	const char* sTableName = (const char*)m_pTabDef->m_pBaseAttr->sTableName;
	*(uint32*)pRedo = RDO_DELETE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sLogName);
	pRedo += CString::StringLength(sLogName) + 1;
	CString::StringCopy(pRedo, sTableName);
	pRedo += CString::StringLength(sTableName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
		return 1;
	if(!m_oIdxAttr.Write(oStream))
		return 1;

	return g_pRedo->WriteRedoStream(&oStream);
}

void CRdbTableAccess::ReDelete(char* sRedo, uint32 nSize)
{
	uint32 nCount;
	CMemoryStream oStream(sRedo, nSize);
	oStream.SetLocalCode(true);
	if(!m_oIdxAttr.Read(oStream))
		return;
	m_pTable->Delete(&m_oIdxAttr, nCount);
}

uint32 CRdbTableAccess::WriteRedoForUpdate(uint32 nMaxRollBackRecordQuantity)
{
	if(m_pTabDef->m_pBaseAttr->nStorage == RDB_MEMORY_TABLE)
		return 0;

	char sRedo[256], *pRedo = sRedo;
	const char* sLogName = m_pTabDef->GetLogName();
	const char* sTableName = (const char*)m_pTabDef->m_pBaseAttr->sTableName;
	*(uint32*)pRedo = RDO_UPDATE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sLogName);
	pRedo += CString::StringLength(sLogName) + 1;
	CString::StringCopy(pRedo, sTableName);
	pRedo += CString::StringLength(sTableName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
		return 1;
	if(!oStream.Write(nMaxRollBackRecordQuantity))
		return 1;
	if(!m_oIdxAttr.Write(oStream))
		return 1;
	if(!m_oSetAttr.Write(oStream))
		return 1;
	return g_pRedo->WriteRedoStream(&oStream);
}

void CRdbTableAccess::ReUpdate(char* sRedo, uint32 nSize)
{
	uint32 nMaxRollBackRecordQuantity;
	CMemoryStream oStream(sRedo, nSize);
	oStream.SetLocalCode(true);
	if(!oStream.Read(nMaxRollBackRecordQuantity))
		return;
	if(!m_oIdxAttr.Read(oStream))
		return;
	if(!m_oSetAttr.Read(oStream))
		return;
	uint32 nCount;
	m_oRollBack.SetRecordSetSize(nMaxRollBackRecordQuantity);
	m_oRollBack.PreAlloc();
	m_pTable->Update(&m_oIdxAttr, &m_oSetAttr, m_oRollBack, nCount);
}

FOCP_END();
