
#include "RdbDb.hpp"

FOCP_BEGIN();

//-----------------------------------------------------------------------------
// CRdbDataBase
//----------------------------------------------------------------------------
CRdbDataBase::CRdbDataBase(CDataBaseDefine* pDefine, uint32 nDbNo)
{
	m_nDbNo = nDbNo;
	m_pDataBaseDefine = pDefine;
	m_pDataBaseDefine->SetInstance(this);
}

CRdbDataBase::~CRdbDataBase()
{
	UnMount();
	m_pDataBaseDefine->SetInstance(NULL);
}

const char* CRdbDataBase::GetLogName()
{
	return m_pDataBaseDefine->GetLogName();
}

CRdbAccess* CRdbDataBase::QueryAccess(const char* sTableName)
{
	CRdbTableAccess* pAccess = NULL;
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo((char*)sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return NULL;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterRead();
	ulong tid = CCooperator::GetCurrentThreadId();
	CRbTreeNode* idx = m_oAccessTable.Find(nTableNo);
	if(idx != m_oAccessTable.End())
	{
		CRbMap<ulong, CRdbTableAccess*> &oAccessTable = m_oAccessTable.GetItem(idx);
		CRbTreeNode* idx2 = oAccessTable.Find(tid);
		if(idx2 != oAccessTable.End())
			pAccess = oAccessTable.GetItem(idx2);
		else
		{
			CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
			pAccess = new CRdbTableAccess(this, pTableDefine, pMutex);
			if(pAccess)
				oAccessTable[tid] = pAccess;
		}
	}
	else
	{
		CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
		if(pTableDefine)
		{
			pAccess = new CRdbTableAccess(this, pTableDefine, pMutex);
			if(pAccess)
				m_oAccessTable[nTableNo][tid] = pAccess;
		}
	}
	if(pAccess)
		pAccess->m_nLocked = 1;
	else
		pMutex->LeaveRead();
	m_oMutex.Leave();
	return pAccess;
}

void CRdbDataBase::DestroyTableAccess(uint32 nTableNo)
{
	CRbTreeNode* idx = m_oAccessTable.Find(nTableNo);
	if(idx != m_oAccessTable.End())
	{
		CRbMap<ulong, CRdbTableAccess*>& oAccessTable = m_oAccessTable.GetItem(idx);
		CRbTreeNode* idx2 = oAccessTable.First();
		CRbTreeNode* end2 = oAccessTable.End();
		for(; idx2!=end2; idx2 = oAccessTable.GetNext(idx2))
		{
			CRdbTableAccess* pAccess = oAccessTable.GetItem(idx2);
			delete pAccess;
		}
		m_oAccessTable.Remove(idx);
	}
}

void CRdbDataBase::DestroyTableAccess()
{
	m_oMutex.Enter();
	CRbTreeNode* idx = m_oAccessTable.First();
	CRbTreeNode* end = m_oAccessTable.End();
	for(; idx!=end; idx=m_oAccessTable.GetNext(idx))
	{
		CRbMap<ulong, CRdbTableAccess*>& oAccessTable = m_oAccessTable.GetItem(idx);
		CRbTreeNode* idx2 = oAccessTable.First();
		CRbTreeNode* end2 = oAccessTable.End();
		for(; idx2!=end2; idx2=oAccessTable.GetNext(idx2))
		{
			CRdbTableAccess* pAccess = oAccessTable.GetItem(idx2);
			delete pAccess;
		}
	}
	m_oMutex.Leave();
}

uint32 CRdbDataBase::Mount()
{
	CTableDefineSet * pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	uint32 i, nTableCount = pTableDefineSet->GetTableCount();
	for(i=0; i<nTableCount; ++i)
	{
		CTableDefine* pTabDef = pTableDefineSet->GetTableDefine(i);
		if(pTabDef)
		{
			CRdbTable* pTable = new CRdbTable;
			if(!pTable)
			{
				for(uint32 j=0; j<i; ++j)
				{
					pTabDef = pTableDefineSet->GetTableDefine(j);
					if(pTabDef)
					{
						pTable = (CRdbTable*)pTabDef->GetInstance();
						delete pTable;
					}
				}
				return RDB_LACK_MEMORY;
			}
			pTable->SetThis(pTabDef);
		}
	}
	CIndexDefineSet* pIndexDefineSet = m_pDataBaseDefine->GetIndexDefineSet();
	uint32 nIndexCount = pIndexDefineSet->GetIndexCount();
	for(i=0; i<nIndexCount; ++i)
	{
		CIndexDefine* pIdxDef = pIndexDefineSet->GetIndexDefine(i);
		if(pIdxDef)
		{
			CIndexDefine* pPrimaryDef = pIdxDef->GetPrimaryIndex();
			if(pPrimaryDef)
			{
				CAbstractIndex* pIdxInstance = (CAbstractIndex*)pIdxDef->GetInstance();
				CAbstractIndex* pPriIdxInstance = (CAbstractIndex*)pPrimaryDef->GetInstance();
				pIdxInstance->SetPrimaryIndex(pPriIdxInstance);
			}
		}
	}
	return RDB_SUCCESS;
}

void CRdbDataBase::UnMount(bool bDestroy)
{
	DestroyTableAccess();
	CTableDefineSet * pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	uint32 i, nTableCount = pTableDefineSet->GetTableCount();
	for(i=0; i<nTableCount; ++i)
	{
		CTableDefine* pTabDef = pTableDefineSet->GetTableDefine(i);
		if(pTabDef)
		{
			CRdbTable* pTable = (CRdbTable*)pTabDef->GetInstance();
			if(pTable)
			{
				if(bDestroy)
					pTable->DestroyObject();
				delete pTable;
			}
		}
	}
}

uint32 CRdbDataBase::CreateTable2(CRdbTableDef* pDefine)
{
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nRet = pTableDefineSet->CreateTable(pDefine);
	if(!nRet)
	{
		uint32 nTableNo = pTableDefineSet->GetTableNo(pDefine->sTableName);
		CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
		CRdbTable* pTable = new CRdbTable();
		if(!pTable)
		{
			pTableDefineSet->RemoveTable(pDefine->sTableName);
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateTable(%s): RDB_LACK_MEMORY", pDefine->sTableName));
			nRet = RDB_LACK_MEMORY;
		}
		else if(!pTable->CreateObject(pTableDefine))
		{
			delete pTable;
			pTableDefineSet->RemoveTable(pDefine->sTableName);
			nRet = RDB_LACK_STORAGE;
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateTable(%s): RDB_LACK_STORAGE", pDefine->sTableName));
		}
	}
	else
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateTable(%s): create table dict failure", pDefine->sTableName));
	m_oMutex.Leave();
	return nRet;
}

uint32 CRdbDataBase::CreateTable(CRdbTableDef* pDefine)
{
	uint32 nRet;

	char sRedo[256], *pRedo = sRedo;
	const char* sDbName = m_pDataBaseDefine->GetDataBaseName();

#if defined(RDB_SUPPORT_MEMORY_DB) && !defined(RDB_SUPPORT_DISK_DB)
	pDefine->nStorage = RDB_MEMORY_TABLE;
#endif
#if !defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
	pDefine->nStorage = RDB_FILE_TABLE;
#endif

	*(uint32*)pRedo = RDO_CREATE_TABLE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);
	pRedo += CString::StringLength(sDbName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CreateTable %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}
	if(!PackTableDefine(oStream, *pDefine))
	{
		FocpLogEx(sDbName, FOCP_LOG_ERROR, ("PackTableDefine %s.%s failure", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}

	EnterSystemRead();
	nRet = g_pRedo->WriteRedoStream(&oStream);
	if(nRet)
	{
		LeaveSystemRead();
		FocpLogEx(sDbName, FOCP_LOG_ERROR, ("CreateTable %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}
	nRet = CreateTable2(pDefine);
	LeaveSystemRead();
	return nRet;
}

uint32 CRdbDataBase::ModifyTable2(CRdbTableDef* pDefine)
{
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo(pDefine->sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::ModifyTable(%s): RDB_TABLE_NOTEXIST", pDefine->sTableName));
		return RDB_TABLE_NOTEXIST;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterWrite();
	DestroyTableAccess(nTableNo);
	CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
	CRdbTable* pTable = (CRdbTable*)pTableDefine->GetInstance();
	bool bAllowModify = (pTable->IsEmptyTable()?true:false);
	uint32 nRet = pTableDefineSet->ModifyTable(pDefine, bAllowModify);
//	if(!nRet)
//		pTable->m_pTabDef = pTableDefineSet->GetTableDefine(nTableNo);
	pMutex->LeaveWrite();
	m_oMutex.Leave();
	if(nRet)
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::ModifyTable(%s): modify table dict failure", pDefine->sTableName));
	return nRet;
}

uint32 CRdbDataBase::ModifyTable(CRdbTableDef* pDefine)
{
	uint32 nRet;

	char sRedo[256], *pRedo = sRedo;
	const char* sDbName = m_pDataBaseDefine->GetDataBaseName();

	*(uint32*)pRedo = RDO_MODIFY_TABLE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);
	pRedo += CString::StringLength(sDbName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("ModifyTable %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}
	if(!PackTableDefine(oStream, *pDefine))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("PackTableDefine %s.%s failure", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}

	EnterSystemRead();
	nRet = g_pRedo->WriteRedoStream(&oStream);
	if(nRet)
	{
		LeaveSystemRead();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("ModifyTable %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sTableName));
		return RDB_WRITERDO_FAILURE;
	}
	nRet = ModifyTable2(pDefine);
	LeaveSystemRead();
	return nRet;
}

uint32 CRdbDataBase::RemoveTable2(char* sTableName)
{
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo(sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::RemoveTable(%s): RDB_TABLE_NOTEXIST", sTableName));
		return RDB_TABLE_NOTEXIST;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterWrite();
	DestroyTableAccess(nTableNo);
	CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
	CRdbTable* pTable = (CRdbTable*)pTableDefine->GetInstance();
	pTable->DestroyObject();
	delete pTable;
	pTableDefineSet->RemoveTable(sTableName);
	pMutex->LeaveWrite();
	m_oMutex.Leave();
	return RDB_SUCCESS;
}

uint32 CRdbDataBase::DropTable(const char* sTableName)
{
	char sRedo[256], *pRedo = sRedo;
	const char* sDbName = m_pDataBaseDefine->GetDataBaseName();
	*(uint32*)pRedo = RDO_DROP_TABLE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);
	pRedo += CString::StringLength(sDbName) + 1;
	CString::StringCopy(pRedo, sTableName);
	pRedo += CString::StringLength(sTableName) + 1;

	EnterSystemRead();
	if(g_pRedo->WriteRedo(sRedo, pRedo-sRedo))
	{
		LeaveSystemRead();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("DropTable %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, sTableName));
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = RemoveTable2((char*)sTableName);
	LeaveSystemRead();

	return nRet;
}

CRdbTableDef* CRdbDataBase::GetTableDefine(const char* sTableName)
{
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo((char*)sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return NULL;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterRead();
	m_oMutex.Leave();
	CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
	return pTableDefine->GetBaseAttr();
}

void CRdbDataBase::ReleaseTableDefine(const char* sTableName)
{
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo((char*)sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->LeaveRead();
	m_oMutex.Leave();
}

uint32 CRdbDataBase::CreateIndex2(CRdbIndexDef* pDefine)
{
	uint32 nIndexNo;
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	CIndexDefineSet* pIndexDefineSet = m_pDataBaseDefine->GetIndexDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo(pDefine->sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateIndex(%s): RDB_TABLE_NOTEXIST", pDefine->sTableName));
		return RDB_TABLE_NOTEXIST;
	}
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterWrite();
	uint32 nRet = pIndexDefineSet->CreateIndex(pDefine, pTableDefineSet);
	if(nRet)
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateIndex(%s): create index dict failure", pDefine->sIndexName));
	if(!nRet)
	{
		CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
		nIndexNo = pTableDefine->GetIndexCount()-1;
		CRdbTable* pTable = (CRdbTable*)pTableDefine->GetInstance();
		nRet = pTable->CreateNewIndex(nIndexNo);
		if(nRet)
		{
			pIndexDefineSet->RemoveIndex(pDefine->sIndexName);
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::CreateIndex(%s): create index space failure", pDefine->sIndexName));
		}
	}
	pMutex->LeaveWrite();
	m_oMutex.Leave();
	return nRet;
}

uint32 CRdbDataBase::CreateIndex(CRdbIndexDef* pDefine)
{
	uint32 nRet;
	char sRedo[256], *pRedo = sRedo;
	const char* sDbName = m_pDataBaseDefine->GetDataBaseName();
	*(uint32*)pRedo = RDO_CREATE_INDEX;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);
	pRedo += CString::StringLength(sDbName) + 1;

	CMemoryStream oStream;
	oStream.SetLocalCode(true);
	if(!oStream.Write(sRedo, pRedo-sRedo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CreateIndex %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sIndexName));
		return RDB_WRITERDO_FAILURE;
	}
	if(!PackIndexDefine(oStream, *pDefine))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("PackIndexDefine %s.%s failure", sDbName, pDefine->sIndexName));
		return RDB_WRITERDO_FAILURE;
	}

	EnterSystemRead();
	if(g_pRedo->WriteRedoStream(&oStream))
	{
		LeaveSystemRead();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CreateIndex %s.%s failure: RDB_WRITERDO_FAILURE", sDbName, pDefine->sIndexName));
		return RDB_WRITERDO_FAILURE;
	}
	nRet = CreateIndex2(pDefine);
	LeaveSystemRead();

	return nRet;
}

uint32 CRdbDataBase::RemoveIndex2(char* sIndexName)
{
	CIndexDefineSet* pIndexDefineSet = m_pDataBaseDefine->GetIndexDefineSet();
	m_oMutex.Enter();
	uint32 nIndexNo = pIndexDefineSet->GetIndexNo(sIndexName);
	if(nIndexNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CRdbDataBase::RemoveIndex(%s): RDB_INDEX_NOTEXIST", sIndexName));
		return RDB_INDEX_NOTEXIST;
	}
	CIndexDefine* pIndexDefine = pIndexDefineSet->GetIndexDefine(nIndexNo);
	uint32 nTableNo = pIndexDefine->GetTableDefine()->GetBaseAttr()->nTableNo;
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterWrite();
	CAbstractIndex* pIndex = (CAbstractIndex*)pIndexDefine->GetInstance();
	pIndex->DestroyObject();
	delete pIndex;
	pIndexDefineSet->RemoveIndex(sIndexName);
	pMutex->LeaveWrite();
	m_oMutex.Leave();
	return RDB_SUCCESS;
}

uint32 CRdbDataBase::DropIndex(const char* sIndexName)
{
	char sRedo[256], *pRedo = sRedo;
	const char* sDbName = m_pDataBaseDefine->GetDataBaseName();
	*(uint32*)pRedo = RDO_DROP_INDEX;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);
	pRedo += CString::StringLength(sDbName) + 1;
	CString::StringCopy(pRedo, sIndexName);
	pRedo += CString::StringLength(sIndexName) + 1;

	EnterSystemRead();
	if(g_pRedo->WriteRedo(sRedo, pRedo-sRedo))
	{
		LeaveSystemRead();
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("DropIndex '%s.%s' failure: RDB_WRITERDO_FAILURE", sDbName, sIndexName));
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = RemoveIndex2((char*)sIndexName);
	LeaveSystemRead();

	return nRet;
}

CRdbIndexDef* CRdbDataBase::GetIndexDefine(const char* sIndexName)
{
	CIndexDefineSet* pIndexDefineSet = m_pDataBaseDefine->GetIndexDefineSet();
	m_oMutex.Enter();
	uint32 nIndexNo = pIndexDefineSet->GetIndexNo((char*)sIndexName);
	if(nIndexNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return NULL;
	}
	CIndexDefine* pIndexDefine = pIndexDefineSet->GetIndexDefine(nIndexNo);
	uint32 nTableNo = pIndexDefine->GetTableDefine()->GetBaseAttr()->nTableNo;
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->EnterRead();
	m_oMutex.Leave();
	return pIndexDefine->GetBaseAttr();
}

void CRdbDataBase::ReleaseIndexDefine(const char* sIndexName)
{
	CIndexDefineSet* pIndexDefineSet = m_pDataBaseDefine->GetIndexDefineSet();
	m_oMutex.Enter();
	uint32 nIndexNo = pIndexDefineSet->GetIndexNo((char*)sIndexName);
	if(nIndexNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return;
	}
	CIndexDefine* pIndexDefine = pIndexDefineSet->GetIndexDefine(nIndexNo);
	uint32 nTableNo = pIndexDefine->GetTableDefine()->GetBaseAttr()->nTableNo;
	CRwMutex* pMutex = m_oTableLockTable.FindLock(nTableNo);
	pMutex->LeaveRead();
	m_oMutex.Leave();
}

uint32 CRdbDataBase::GetIndexCount(const char* sTableName, uint32 & nCount)
{
	nCount = 0;
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo((char*)sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return RDB_TABLE_NOTEXIST;
	}
	CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
	nCount = pTableDefine->GetIndexCount();
	m_oMutex.Leave();
	return RDB_SUCCESS;
}

uint32 CRdbDataBase::GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName)
{
	uint32 nCount;
	pIndexName[0] = 0;
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	m_oMutex.Enter();
	uint32 nTableNo = pTableDefineSet->GetTableNo((char*)sTableName);
	if(nTableNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return RDB_TABLE_NOTEXIST;
	}
	CTableDefine * pTableDefine = pTableDefineSet->GetTableDefine(nTableNo);
	nCount = pTableDefine->GetIndexCount();
	if(nIdx >= nCount)
	{
		m_oMutex.Leave();
		return RDB_INDEX_NOTEXIST;
	}
	CIndexDefine* pIndex = pTableDefine->GetIndexDefine(nIdx);
	CString::StringCopy(pIndexName, pIndex->GetBaseAttr()->sIndexName);
	m_oMutex.Leave();
	return RDB_SUCCESS;
}

void CRdbDataBase::QueryTableList(CString &oList)
{
	uint32 nCount;
	CTableDefineSet* pTableDefineSet = m_pDataBaseDefine->GetTableDefineSet();
	oList.Clear();
	m_oMutex.Enter();

	nCount = pTableDefineSet->GetTableCount();
	for(uint32 i=0; i<nCount; ++i)
	{
		CTableDefine* pTableDefine = pTableDefineSet->GetTableDefine(i);
		if(pTableDefine)
		{
			if(i)
				oList += ",";
			oList += pTableDefine->GetBaseAttr()->sTableName;
		}
	}

	m_oMutex.Leave();
}

void CRdbDataBase::Release()
{
	((CRdbDataBaseSystem*)CRdbSystem::GetInstance())->m_oDbLockTable.FindLock(m_nDbNo)->LeaveRead();
}

//-----------------------------------------------------------------------------
// CRdbDataBaseSystem
//----------------------------------------------------------------------------
static void OnRdbRedo(char* sRedo, uint32 nSize)
{
	CRdbDataBaseSystem* pRdbSys = (CRdbDataBaseSystem*)CRdbSystem::GetInstance();

	uint32 nCode = *(uint32*)sRedo;
	sRedo += sizeof(uint32);
	if(nCode == RDO_CREATE_DATABASE)
	{
		char* sDbName = sRedo;
		pRdbSys->CreateDataBase2((char*)sDbName);
	}
	else if(nCode == RDO_DROP_DATABASE)
	{
		char* sDbName = sRedo;
		pRdbSys->RemoveDataBase2((char*)sDbName);
	}
	else if(nCode == RDO_INSERT)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sTabName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbAccess* pTab = pDb->QueryAccess((const char*)sTabName);
			if(pTab)
			{
				((CRdbTableAccess*)pTab)->ReInsert(sRedo, nSize);
				pTab->Release();
			}
			pDb->Release();
		}
	}
	else if(nCode == RDO_UPDATE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sTabName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbAccess* pTab = pDb->QueryAccess((const char*)sTabName);
			if(pTab)
			{
				((CRdbTableAccess*)pTab)->ReUpdate(sRedo, nSize);
				pTab->Release();
			}
			pDb->Release();
		}
	}
	else if(nCode == RDO_DELETE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sTabName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbAccess* pTab = pDb->QueryAccess((const char*)sTabName);
			if(pTab)
			{
				((CRdbTableAccess*)pTab)->ReDelete(sRedo, nSize);
				pTab->Release();
			}
			pDb->Release();
		}
	}
	else if(nCode == RDO_TRUNCATE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sTabName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbAccess* pTab = pDb->QueryAccess((const char*)sTabName);
			if(pTab)
			{
				((CRdbTableAccess*)pTab)->ReTruncate(sRedo, nSize);
				pTab->Release();
			}
			pDb->Release();
		}
	}
	else if(nCode == RDO_CREATE_TABLE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbTableDef oDefine;
			CMemoryStream oStream(sRedo, nSize);
			oStream.SetLocalCode(true);
			if(UnPackTableDefine(oStream, oDefine))
				pDb->CreateTable2(&oDefine);
			FreeTableDefine(oDefine);
			pDb->Release();
		}
	}
	else if(nCode == RDO_MODIFY_TABLE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbTableDef oDefine;
			CMemoryStream oStream(sRedo, nSize);
			oStream.SetLocalCode(true);
			if(UnPackTableDefine(oStream, oDefine))
				pDb->ModifyTable2(&oDefine);
			FreeTableDefine(oDefine);
			pDb->Release();
		}
	}
	else if(nCode == RDO_DROP_TABLE)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sTabName = sRedo;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			pDb->RemoveTable2(sTabName);
			pDb->Release();
		}
	}
	else if(nCode == RDO_CREATE_INDEX)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			CRdbIndexDef oDefine;
			CMemoryStream oStream(sRedo, nSize);
			oStream.SetLocalCode(true);
			if(UnPackIndexDefine(oStream, oDefine))
				pDb->CreateIndex2(&oDefine);
			FreeIndexDefine(oDefine);
			pDb->Release();
		}
	}
	else if(nCode == RDO_DROP_INDEX)
	{
		uint32 nLen;
		char* sDbName = sRedo;
		nLen = CString::StringLength(sRedo)+1;
		sRedo += nLen;
		nSize -= nLen;
		char* sIdxName = sRedo;
		CRdbDataBase* pDb = (CRdbDataBase*)pRdbSys->QueryDataBase((const char*)sDbName);
		if(pDb)
		{
			pDb->RemoveIndex2(sIdxName);
			pDb->Release();
		}
	}
}

CRdbDataBaseSystem::CRdbDataBaseSystem()
{
}

CRdbDataBaseSystem::~CRdbDataBaseSystem()
{
}

CRdbSystem* CRdbSystem::GetInstance()
{
	return CStaticInstance<CRdbDataBaseSystem>::GetInstance();
}

CRdb* CRdbDataBaseSystem::QueryDataBase(const char* sDbName)
{
	m_oMutex.Enter();
	uint32 nDbNo = m_oDbSysDef.GetDataBaseNo((char*)sDbName);
	if(nDbNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		return NULL;
	}
	m_oMutex.Leave();
	m_oDbLockTable.FindLock(nDbNo)->EnterRead();
	return (CRdb*)m_oDbSysDef.GetDataBaseDefine(nDbNo)->GetInstance();
}

uint32 CRdbDataBaseSystem::CreateDataBaseSystemObject()
{
	uint64 nSystemPage;
	if(!GetSystemPage(&nSystemPage))
		return RDB_LACK_STORAGE;
	uint64 nDbsAddr = m_oDbSysDef.CreateDataBaseSystem();
	if(!nDbsAddr)
		return RDB_LACK_STORAGE;
	DirectWriteVmm(nSystemPage, (char*)&nDbsAddr, sizeof(nDbsAddr));
	return RDB_SUCCESS;
}

uint32 CRdbDataBaseSystem::CreateDataBase2(const char* sDbName)
{
	uint32 nRet;
	m_oMutex.Enter();
	if(!m_oDbSysDef.GetDbsAddr())
	{
		nRet = 	CreateDataBaseSystemObject();
		if(nRet)
		{
			m_oMutex.Leave();
			return nRet;
		}
	}

	nRet = m_oDbSysDef.CreateDataBase((char*)sDbName);
	if(!nRet)
	{
		uint32 nDbNo = m_oDbSysDef.GetDataBaseNo((char*)sDbName);
		CDataBaseDefine* pDefine = m_oDbSysDef.GetDataBaseDefine(nDbNo);
		CRdbDataBase* pDb = new CRdbDataBase(pDefine, nDbNo);
		if(!pDb)
		{
			m_oDbSysDef.RemoveDataBase((char*)sDbName);
			FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::CreateDataBase(%s): allocate CRdbDataBase failure", sDbName));
			nRet = RDB_LACK_MEMORY;
		}
		else
		{
			nRet = pDb->Mount();
			if(nRet)
			{
				delete pDb;
				m_oDbSysDef.RemoveDataBase((char*)sDbName);
				FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::CreateDataBase(%s): mount failure", sDbName));
			}
		}
	}
	else
		FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::CreateDataBase(%s): create database dict failure", sDbName));
	m_oMutex.Leave();
	return nRet;
}

uint32 CRdbDataBaseSystem::CreateDataBase(const char* sDbName)
{
	if(!sDbName || !sDbName[0] || CString::StringLength(sDbName) > RDB_NAME_MAXLEN)
		return RDB_INVALID_NAME;

	char sRedo[128], *pRedo = sRedo;
	*(uint32*)pRedo = RDO_CREATE_DATABASE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);

	EnterSystemRead();
	if(g_pRedo->WriteRedo(sRedo, 4+CString::StringLength(sDbName)+1))
	{
		LeaveSystemRead();
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = CreateDataBase2(sDbName);
	LeaveSystemRead();

	return nRet;
}

uint32 CRdbDataBaseSystem::RemoveDataBase2(const char* sDbName)
{
	m_oMutex.Enter();
	uint32 nDbNo = m_oDbSysDef.GetDataBaseNo((char*)sDbName);
	if(nDbNo == 0xFFFFFFFF)
	{
		m_oMutex.Leave();
		FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::RemoveDataBase(%s): RDB_DB_NOTEXIST", sDbName));
		return RDB_DB_NOTEXIST;
	}
	m_oMutex.Leave();
	CRwMutex* pLock = m_oDbLockTable.FindLock(nDbNo);
	pLock->EnterWrite();
	m_oMutex.Enter();
	CRdbDataBase* pDb = (CRdbDataBase*)m_oDbSysDef.GetDataBaseDefine(nDbNo)->GetInstance();
	pDb->UnMount(true);
	delete pDb;
	m_oDbSysDef.RemoveDataBase((char*)sDbName);
	m_oMutex.Leave();
	pLock->LeaveWrite();
	return RDB_SUCCESS;
}

uint32 CRdbDataBaseSystem::RemoveDataBase(const char* sDbName)
{
	if(!sDbName || !sDbName[0] || CString::StringLength(sDbName) > RDB_NAME_MAXLEN)
		return RDB_INVALID_NAME;

	char sRedo[128], *pRedo = sRedo;
	*(uint32*)pRedo = RDO_DROP_DATABASE;
	pRedo += sizeof(uint32);
	CString::StringCopy(pRedo, sDbName);

	EnterSystemRead();
	if(g_pRedo->WriteRedo(sRedo, 4+CString::StringLength(sDbName)+1))
	{
		LeaveSystemRead();
		return RDB_WRITERDO_FAILURE;
	}
	uint32 nRet = RemoveDataBase2(sDbName);
	LeaveSystemRead();

	return nRet;
}

void CRdbDataBaseSystem::QueryDataBaseList(CString &oDbList)
{
	m_oMutex.Enter();
	oDbList = m_oDbSysDef.GetDataBaseList();
	m_oMutex.Leave();
}

uint32 CRdbDataBaseSystem::Startup()
{
	uint64 nSystemPage;
	if(!GetSystemPage(&nSystemPage))
		return RDB_SUCCESS;
	uint64 nDbsAddr;
	DirectReadVmm(nSystemPage, (char*)&nDbsAddr, sizeof(nDbsAddr));
	if(!nDbsAddr)
		return RDB_SUCCESS;
	uint32 nRet = m_oDbSysDef.LoadFromDataDict(nDbsAddr);
	if(nRet)
	{
		FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::Startup(): LoadFromDataDict failure"));
		return nRet;
	}
	uint32 nDbCount = m_oDbSysDef.GetDataBaseCount();
	for(uint32 i=0; i<nDbCount; ++i)
	{
		CDataBaseDefine* pDefine = m_oDbSysDef.GetDataBaseDefine(i);
		if(!pDefine)
			continue;
		CRdbDataBase* pDb = new CRdbDataBase(pDefine, i);
		if(!pDb)
		{
			FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::Startup(): allocate CRdbDataBase failure"));
			nRet = RDB_LACK_MEMORY;
		}
		else
		{
			nRet = pDb->Mount();
			if(nRet)
			{
				FocpLogEx("Rdb", FOCP_LOG_ERROR, ("CRdbDataBaseSystem::Startup(): mount(%s) failure", pDefine->GetBaseAttr()->sDbName));
				delete pDb;
			}
		}
		if(nRet)
		{
			for(uint32 j=0; j<i; ++j)
			{
				pDefine = m_oDbSysDef.GetDataBaseDefine(j);
				if(!pDefine)
					continue;
				pDb = (CRdbDataBase*)pDefine->GetInstance();
				if(pDb)
					delete pDb;
			}
			m_oDbSysDef.Clear();
			break;
		}
	}
	return nRet;
}

void CRdbDataBaseSystem::Cleanup()
{
	uint32 nDbCount = m_oDbSysDef.GetDataBaseCount();
	for(uint32 i=0; i<nDbCount; ++i)
	{
		CDataBaseDefine* pDefine = m_oDbSysDef.GetDataBaseDefine(i);
		if(!pDefine)
			continue;
		CRdbDataBase* pDb = (CRdbDataBase*)pDefine->GetInstance();
		if(pDb)
			delete pDb;
	}
	m_oDbSysDef.Clear();
	m_oDbLockTable.DeleteLock();
	if(g_pRedo)
    {
        delete g_pRedo;
        g_pRedo = NULL;
    }
}

uint32 CRdbDataBaseSystem::Startup(const char* sConfigPath, const char* sRdoPath, uint32 nLogFileSize, uint32 nLogFileNum, bool bRedo)
{
	char sRdoFile[FOCP_MAX_PATH+1];
	CString::StringCopy(sRdoFile, sConfigPath);
	CString::StringCatenate(sRdoFile, "/rdo.ctr");
	g_pRedo = new CRedoLog;
	if(g_pRedo->Create(sRdoFile, (char*)sRdoPath, OnRdbRedo, nLogFileSize, nLogFileNum))
		return RDB_CREATE_REDO_FAILURE;
	uint32 nRet = Startup();
	if(nRet)
		return nRet;
	if(bRedo)
		g_pRedo->Redo();
	return nRet;
}

uint32 CRdbDataBaseSystem::Backup(const char* sDataBakPath, const char* sDataBakPath2)
{
	EnterSystemWrite();
	if(GetVmmControlStatus(VMM_CONTROL_DIRTY))
	{
		SetVmmControlStatus(VMM_CONTROL_BACKUP, 0);
		if(BackupVmm(sDataBakPath2))
			return RDB_BACKUP_FAILURE;
		SetVmmControlStatus(VMM_CONTROL_BACKUP, 1);
		Reset(sDataBakPath, sDataBakPath2);
		SetVmmControlStatus(VMM_CONTROL_BACKUP, 0);
	}
	LeaveSystemWrite();
	return RDB_SUCCESS;
}

static const char* GetShellPath(char* sShellPath)
{
	CString::StringCopy(sShellPath, CFilePathInfo::GetInstance()->GetDir());
	CDiskFileSystem::GetInstance()->GetOsPathName(sShellPath);
	return sShellPath;
}

void CRdbDataBaseSystem::Reset(const char* sDataBakPath, const char* sDataBakPath2)
{
	g_pRedo->Reset();

	char* sShellPath = new char[FOCP_MAX_PATH];
	char* sDataPath = new char[FOCP_MAX_PATH];
	char* sDataPath2 = new char[FOCP_MAX_PATH];

	GetShellPath(sShellPath);

	CString::StringCopy(sDataPath, sDataBakPath);
	CDiskFileSystem::GetInstance()->GetOsPathName(sDataPath);
	CString::StringCopy(sDataPath2, sDataBakPath2);
	CDiskFileSystem::GetInstance()->GetOsPathName(sDataPath2);

	CFormatString oCommand;

#ifdef WINDOWS
	oCommand.Print("%s/rm.exe -rf %s", sShellPath, sDataPath);
#else
	oCommand.Print("rm -rf %s", sDataPath);
#endif
	System(oCommand.GetStr());

#ifdef WINDOWS
	oCommand.Print("%s/mv.exe -f %s %s", sShellPath, sDataPath2, sDataPath);
#else
	oCommand.Print("mv -f %s %s", sShellPath, sDataPath2, sDataPath);
#endif
	System(oCommand.GetStr());

	delete[] sDataPath;
	delete[] sDataPath2;
#ifdef WINDOWS
	delete[] sShellPath;
#endif
}

FOCP_END();
