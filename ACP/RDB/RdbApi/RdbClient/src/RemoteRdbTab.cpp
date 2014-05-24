
//-----------------------------------------------------
// CRemoteRdbAccess
//-----------------------------------------------------
CRemoteRdbAccess::CRemoteRdbAccess(void* pDb, CRemoteTableDefine* pTabDef, CRwMutex* pLock):
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

CRemoteRdbAccess::~CRemoteRdbAccess()
{
}

uint32 CRemoteRdbAccess::GetFieldCount()
{
	return m_pTabDef->m_nFieldCount;
}

bool CRemoteRdbAccess::IsValidField(uint32 nFieldNo)
{
	return m_pTabDef->IsValidField(nFieldNo);
}

uint32 CRemoteRdbAccess::GetFieldNo(const char* sFieldName)
{
	return m_pTabDef->GetFieldNo((char*)sFieldName);
}

CRdbFilter* CRemoteRdbAccess::GetResultFilter()
{
	m_oFilter.Clear();
	return &m_oFilter;
}

CRdbResultSet* CRemoteRdbAccess::GetResultSet()
{
	return &m_oGetAttr;
}

CRdbParaSet* CRemoteRdbAccess::GetQueryPara()
{
	m_oIdxAttr.Clear();
	m_oFilter.Clear();
	return &m_oIdxAttr;
}

CRdbPara* CRemoteRdbAccess::GetUpdatePara()
{
	m_oSetAttr.Clear();
	return &m_oSetAttr;
}

CRdbPara* CRemoteRdbAccess::GetInsertPara()
{
	m_oInsertAttr.Clear();
	return &m_oInsertAttr;
}

void CRemoteRdbAccess::Truncate()
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

uint32 CRemoteRdbAccess::Query(uint32 nMaxRecordQuantity, uint32 nSkipCount)
{
	m_oGetAttr.SetRecordSetSize(nMaxRecordQuantity);
	return m_pTable->Query(&m_oIdxAttr, m_oFilter, m_oGetAttr, nSkipCount);
}

uint32 CRemoteRdbAccess::Query(uint64 nRowId)
{
	m_oGetAttr.SetRecordSetSize(1);
	return m_pTable->Query(nRowId, m_oFilter, m_oGetAttr);
}

uint32 CRemoteRdbAccess::QueryCount(uint32 &nCount)
{
	return m_pTable->QueryCount(&m_oIdxAttr, nCount);
}

uint32 CRemoteRdbAccess::Exist(uint32& bExist)
{
	return m_pTable->Exist(&m_oIdxAttr, bExist);
}

uint32 CRemoteRdbAccess::Insert()
{
	return m_pTable->Insert(&m_oInsertAttr);
}

uint32 CRemoteRdbAccess::Delete()
{
	uint32 nCount;
	return m_pTable->Delete(&m_oIdxAttr, nCount);
}

uint32 CRemoteRdbAccess::Update(uint32 nMaxRollBackRecordQuantity)
{
	uint32 nCount;
	m_oRollBack.SetRecordSetSize(nMaxRollBackRecordQuantity);
	m_oRollBack.PreAlloc();
	return m_pTable->Update(&m_oIdxAttr, &m_oSetAttr, m_oRollBack, nCount);
}

void CRemoteRdbAccess::Release()
{
	if(m_nLocked)
	{
		m_pLock->LeaveRead();
		m_nLocked = 0;
	}
}

