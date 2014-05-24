
#include "SqlEnv.hpp"

FOCP_BEGIN();

//-----------------------------------------------------------------
// CSqlField
//-----------------------------------------------------------------
CSqlField::CSqlField():
type(0), len(0), recsize(0), null(false)
{
}

const CString* CSqlGetFieldName::GetKey(const CSqlField& oData)
{
	return &oData.name;
}

//-----------------------------------------------------------------
// CSqlTable
//-----------------------------------------------------------------
CSqlTable::CSqlTable()
{
	m_pField = NULL;
}

#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
void CSqlTable::Bind(const CString& oName, uint32 nStorageType)
#else
void CSqlTable::Bind(const CString& oName)
#endif
{
	m_nFldNo = 0;
	m_oName = oName;
#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
	m_nStorage = nStorageType;
#endif
	m_oFieldTable.Clear();
}

CSqlField* CSqlTable::AddField(const CString& oName)
{
	CSqlField a;
	a.name = oName;
	CRbTreeNode* r = m_oFieldTable.Insert(a);
	if(r == m_oFieldTable.End())
		return NULL;
	m_pField = (CSqlField*)&(m_oFieldTable.GetItem(r));
	m_pField->seq = ++m_nFldNo;
	m_pField->name = oName;
	return m_pField;
}

CSqlField* CSqlTable::GetCurrentField()
{
	return m_pField;
}

//-----------------------------------------------------------------
// CSqlOperator
//-----------------------------------------------------------------
CSqlOperator::CSqlOperator()
{
	m_pAccess = NULL;
}

CSqlOperator::~CSqlOperator()
{
	UnBind();
}

void CSqlOperator::Bind(const CString &oName, CRdbAccess * pAccess)
{
	UnBind();
	m_oName = oName;
	m_pAccess = pAccess;
}

void CSqlOperator::UnBind()
{
	if(m_pAccess)
	{
		m_pAccess->Release();
		m_pAccess = NULL;
	}
}

//-----------------------------------------------------------------
// CSqlInsert
//-----------------------------------------------------------------
CSqlInsert::CSqlInsert(CSqlEnv * pEnv)
{
	m_pEnv = pEnv;
}

void CSqlInsert::Bind(const CString &oName, CRdbAccess * pAccess)
{
	CSqlOperator::Bind(oName, pAccess);
	m_pInsertAttr = pAccess->GetInsertPara();
	m_nFldNo = 0;
	m_oFieldTable.Clear();
}

bool CSqlInsert::AddValue(const CString& oVal)
{
	uint32 nFldNo = m_nFldNo++;
	if(!m_oFieldTable.GetSize())
	{
		uint32 nFieldCount = m_pAccess->GetFieldCount();
		while(nFldNo < nFieldCount)
		{
			if(m_pAccess->IsValidField(nFldNo))
			{
				m_pInsertAttr->SetFromString(nFldNo, (char*)oVal.GetStr(), RDB_SQLPARA_OPERATOR_EQUAL);
				break;
			}
			nFldNo = m_nFldNo++;
		}
		if(nFldNo >= nFieldCount)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlInsert::AddValue() there isn't the field no '%u' in the table '%s'", nFldNo, m_oName.GetStr()));
			return false;
		}
	}
	else
	{
		CString& oFieldName = m_oFieldTable[nFldNo];
		uint32 nFieldNo = m_pAccess->GetFieldNo(oFieldName.GetStr());
		if(nFieldNo == (uint32)(-1))
		{
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlInsert::AddValue() there isn't the field no '%u' in the table '%s'", nFldNo, m_oName.GetStr()));
			return false;
		}
		m_pInsertAttr->SetFromString(nFieldNo, oVal.GetStr(), RDB_SQLPARA_OPERATOR_EQUAL);
	}
	return true;
}

void CSqlInsert::AddField(const CString& oName)
{
	m_oFieldTable.Insert((uint32)-1, oName);
}

//-----------------------------------------------------------------
// CSqlWhere
//-----------------------------------------------------------------
CSqlWhere::CSqlWhere(CSqlEnv * pEnv)
{
	m_pWhere = NULL;
	m_pEnv = pEnv;
}

void CSqlWhere::Bind(const CString &oName, CRdbAccess * pAccess)
{
	CSqlOperator::Bind(oName, pAccess);
	m_pWhere = pAccess->GetQueryPara();
}

bool CSqlWhere::AddCond(const CString& oName, uint32 nOp, const CString& oVal)
{
	CRdbParaSet* pSubParaSet = m_pWhere->AddParaSet();
	CRdbPara* pPara = pSubParaSet->AddPara();
	uint32 nFieldNo = m_pAccess->GetFieldNo(oName.GetStr());
	if(nFieldNo == (uint32)(-1))
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlWhere::AddCond() there isn't the field '%s' in the table '%s'", oName.GetStr(), m_oName.GetStr()));
		return false;
	}
	pPara->SetFromString(nFieldNo, oVal.GetStr(), nOp);
	return true;
}

//-----------------------------------------------------------------
// CSqlUpdate
//-----------------------------------------------------------------
CSqlUpdate::CSqlUpdate(CSqlEnv * pEnv)
	:CSqlWhere(pEnv),m_pSetAttr(NULL)
{
}

void CSqlUpdate::Bind(const CString &oName, CRdbAccess * pAccess)
{
	CSqlWhere::Bind(oName, pAccess);
	m_pSetAttr = pAccess->GetUpdatePara();
}

bool CSqlUpdate::AddSetPara(const CString& oName, const CString& oVal)
{
	uint32 nOp = RDB_SQLPARA_OPERATOR_EQUAL;
	uint32 nFieldNo = m_pAccess->GetFieldNo(oName.GetStr());
	if(nFieldNo == (uint32)(-1))
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlUpdate::AddSetPara() there isn't the field '%s' in the table '%s'", oName.GetStr(), m_oName.GetStr()));
		return false;
	}
	m_pSetAttr->SetFromString(nFieldNo, oVal.GetStr(), nOp);
	return true;
}

//-----------------------------------------------------------------
// CSqlSelect
//-----------------------------------------------------------------
CSqlSelect::CSqlSelect(CSqlEnv * pEnv):
	CSqlWhere(pEnv)
{
}

void CSqlSelect::Clear()
{
	m_oFieldTable.Clear();
	m_pRecordSet = NULL;
}

void CSqlSelect::AddField(const CString& oName)
{
	m_oFieldTable.Insert((uint32)(-1),oName);
}

uint32 CSqlSelect::GetResultCount()
{
	return m_pRecordSet->GetResultCount();
}

CRdbResult* CSqlSelect::GetRecord(uint32 nRow)
{
	return m_pRecordSet->GetResult(nRow);
}

//-----------------------------------------------------------------
// CSqlIndex
//-----------------------------------------------------------------
CSqlIndex::CSqlIndex():
	nQualifier(RDB_COMMON_INDEX), nArithmetic(RDB_HASH_INDEX), nHashRate(300)
{
}

//-----------------------------------------------------------------
// CSqlDataBase
//-----------------------------------------------------------------
CSqlDataBase::CSqlDataBase(CSqlEnv* pEnv):
m_pDb(NULL), 
m_oInsert(pEnv),
m_oDelete(pEnv),
m_oUpdate(pEnv),
m_oSelect(pEnv),
m_pWhere(NULL)
{
	m_pEnv = pEnv;
	m_nMaxRollBack = 1024;
	m_nResultSize = 1024;
	m_nRepeat = 0;
}

CSqlDataBase::~CSqlDataBase()
{
	if(m_pDb)
	{
		m_pDb->Release();
		m_pDb = NULL;
	}
}

void CSqlDataBase::Bind(CRdb* pDb)
{
	if(m_pDb)
		m_pDb->Release();
	m_pDb = pDb;
}

void CSqlDataBase::SetRepeat(uint32 nRepeat)
{
	m_nRepeat = nRepeat;
}

#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
uint32 CSqlDataBase::CreateSqlTable(const CString& oName, uint32 nStorageType)
#else
uint32 CSqlDataBase::CreateSqlTable(const CString& oName)
#endif
{
#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
	m_oTable.Bind(oName, nStorageType);
#else
	m_oTable.Bind(oName);
#endif
	return RDB_SUCCESS;
}

uint32 CSqlDataBase::CreateTable(bool bModify)
{
	CRbMap<uint32, CSqlField*> OrderFieldTable;
	CRbTree<CString,CSqlField,CSqlGetFieldName,CNameCompare>& oFieldTable = m_oTable.m_oFieldTable;
	CRbTreeNode* e = oFieldTable.End();
	for(CRbTreeNode* i = oFieldTable.First(); i != e; i=oFieldTable.GetNext(i))
	{
		CSqlField& oField = oFieldTable.GetItem(i);
		OrderFieldTable[oField.seq] = &oField;
	}
	
	CRdbTableDef oTabDef;
	oTabDef.sTableName = (char*)m_oTable.m_oName.GetStr();
	oTabDef.nFieldCount = OrderFieldTable.GetSize();
	oTabDef.pFieldDefines = new CRdbFieldDef[oTabDef.nFieldCount];
	oTabDef.nStorage = m_oTable.m_nStorage;

	if (oTabDef.pFieldDefines == NULL)
	{
		delete [] oTabDef.pFieldDefines;
		if(bModify)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::ModifyTable(%s) failure: RDB_LACK_MEMORY", oTabDef.sTableName));
		else
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateTable(%s) failure: RDB_LACK_MEMORY", oTabDef.sTableName));
		return RDB_LACK_MEMORY;
	}

	uint32 j = 0;
	e = OrderFieldTable.End();
	for(CRbTreeNode* k = OrderFieldTable.First(); 
		k != e; 
		k=OrderFieldTable.GetNext(k), ++j)
	{
		CRdbFieldDef& oField = oTabDef.pFieldDefines[j];
		CSqlField* pField = OrderFieldTable.GetItem(k);
		oField.sFieldName = (char*)pField->name.GetStr();
		oField.sDefault = (char*)pField->defval.GetStr();
		oField.nType = pField->type;
		oField.nLen = pField->len;
		oField.bNotNull = pField->null;
		oField.nRecSize = pField->recsize;
		if(!oField.sDefault[0])
			oField.sDefault = NULL;
	}

	uint32 nRet;
	if(bModify)
		nRet = m_pDb->ModifyTable(&oTabDef);
	else
		nRet = m_pDb->CreateTable(&oTabDef);
	delete [] oTabDef.pFieldDefines;
	if(bModify)
	{
		if(nRet)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::ModifyTable(%s) failure", oTabDef.sTableName));
		else
			FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::ModifyTable(%s) success", oTabDef.sTableName));
	}
	else
	{
		if(nRet)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateTable(%s) failure", oTabDef.sTableName));
		else
			FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::CreateTable(%s) success", oTabDef.sTableName));
	}
	return nRet;
}

uint32 CSqlDataBase::DropTable(const CString& oName)
{
	return m_pDb->DropTable(oName.GetStr());
}

uint32 CSqlDataBase::CreateSqlIndex()
{
	m_oIndex.oIndexFieldTable.Clear();
	return RDB_SUCCESS;
}

uint32 CSqlDataBase::CreateIndex()
{
	CString oFieldName;
	CVector<CString>& oFieldTable = m_oIndex.oIndexFieldTable;
	uint32 nSize = oFieldTable.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!oFieldName.Empty())
			oFieldName += ",";
		oFieldName += oFieldTable[i];
	}

	CRdbIndexDef oIdxDef;
	oIdxDef.sIndexName = (char*)m_oIndex.oIndexName.GetStr();
	oIdxDef.sTableName = (char*)m_oIndex.oTableName.GetStr();
	oIdxDef.sPrimaryIndex = (char*)m_oIndex.oPrimaryIndex.GetStr();
	if(!oIdxDef.sPrimaryIndex[0])
		oIdxDef.sPrimaryIndex = NULL;
	oIdxDef.nQualifier = m_oIndex.nQualifier;
	oIdxDef.nArithmetic = m_oIndex.nArithmetic;
	oIdxDef.nHashRate = m_oIndex.nHashRate;
	oIdxDef.pFieldList = (char*)oFieldName.GetStr();

	return m_pDb->CreateIndex(&oIdxDef);
}

uint32 CSqlDataBase::DropIndex(const CString& oName)
{
	return m_pDb->DropIndex(oName.GetStr());
}

uint32 CSqlDataBase::CreateSqlInsert(const CString& oName)
{
	CRdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlInsert(%s) failure: RDB_TABLE_NOTEXIST", oName.GetStr()));
		return RDB_TABLE_NOTEXIST;
	}
	m_oInsert.Bind(oName, pAccess);
	return RDB_SUCCESS;
}

uint32 CSqlDataBase::InsertRecord()
{
	if(!m_oInsert.m_pAccess)
		return RDB_SUCCESS;
	uint32 nRet;
	if(m_nRepeat)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("begin insert operator:%u", CTimer::GetTickCount()));
		for(uint32 i=0; i<m_nRepeat; ++i)
		{
			nRet = m_oInsert.m_pAccess->Insert();
			if(nRet)
				FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::InsertRecord(%s) failure", m_oInsert.m_oName.GetStr()));
		}
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("end insert operator:%u", CTimer::GetTickCount()));
	}
	else
		nRet = m_oInsert.m_pAccess->Insert();
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::InsertRecord(%s) failure", m_oInsert.m_oName.GetStr()));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::InsertRecord(%s) success", m_oInsert.m_oName.GetStr()));
	m_oInsert.UnBind();
	return nRet;
}

uint32 CSqlDataBase::CreateSqlDelete(const CString& oName)
{
	CRdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlDelete(%s) failure: RDB_TABLE_NOTEXIST", oName.GetStr()));
		return RDB_TABLE_NOTEXIST;
	}
	m_oDelete.Bind(oName, pAccess);
	m_pWhere = &m_oDelete;
	return RDB_SUCCESS;
}

uint32 CSqlDataBase::DeleteRecord()
{
	if(!m_oDelete.m_pAccess)
		return RDB_SUCCESS;
	uint32 nRet;
	if(m_nRepeat)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("begin delete operator:%u", CTimer::GetTickCount()));
		for(uint32 i=0; i<m_nRepeat; ++i)
			nRet = m_oDelete.m_pAccess->Delete();
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("end delete operator:%u", CTimer::GetTickCount()));
	}
	else
		nRet = m_oDelete.m_pAccess->Delete();
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::DeleteRecord(%s) failure", m_oDelete.m_oName.GetStr()));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::DeleteRecord(%s) success", m_oDelete.m_oName.GetStr()));
	m_oDelete.UnBind();
	return nRet;
}

uint32 CSqlDataBase::CreateSqlUpdate(const CString& oName)
{
	CRdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlUpdate(%s) failure: RDB_TABLE_NOTEXIST", oName.GetStr()));
		return RDB_TABLE_NOTEXIST;
	}
	m_oUpdate.Bind(oName, pAccess);
	m_pWhere = &m_oUpdate;
	return RDB_SUCCESS;
}

uint32 CSqlDataBase::UpdateRecord()
{
	if(!m_oUpdate.m_pAccess)
		return RDB_SUCCESS;
	uint32 nRet;
	if(m_nRepeat)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("begin update operator:%u", CTimer::GetTickCount()));
		for(uint32 i=0; i<m_nRepeat; ++i)
		{
			nRet = m_oUpdate.m_pAccess->Update(m_nMaxRollBack);
			if(nRet)
				FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::UpdateRecord(%s) failure", m_oUpdate.m_oName.GetStr()));
		}
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("end update operator:%u", CTimer::GetTickCount()));
	}
	else
		nRet = m_oUpdate.m_pAccess->Update(m_nMaxRollBack);
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::UpdateRecord(%s) failure", m_oUpdate.m_oName.GetStr()));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::UpdateRecord(%s) success", m_oUpdate.m_oName.GetStr()));
	m_oUpdate.UnBind();
	return nRet;
}

void CSqlDataBase::SetMaxRollBack(uint32 nMaxRollBack)
{
	m_nMaxRollBack = nMaxRollBack;
}

uint32 CSqlDataBase::TruncateTable(const CString& oName)
{
	CRdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::TruncateTable(%s) failure: RDB_TABLE_NOTEXIST", oName.GetStr()));
		return RDB_TABLE_NOTEXIST;
	}
	pAccess->Truncate();
	pAccess->Release();
	FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::TruncateTable(%s) success", oName.GetStr()));
	return 0;
}

void CSqlDataBase::PrepareSqlSelect()
{
	m_oSelect.Clear();
}

uint32 CSqlDataBase::CreateSqlSelect(const CString& oName)
{
	CRdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlSelect(%s) failure: RDB_TABLE_NOTEXIST", oName.GetStr()));
		return RDB_TABLE_NOTEXIST;
	}
	m_oSelect.Bind(oName, pAccess);
	m_pWhere = &m_oSelect;
	return RDB_SUCCESS;
}

CString CSqlDataBase::GetFieldList()
{
	CString oList;
	const char* sTableName = m_oSelect.m_oName.GetStr();
	if(!m_oSelect.m_oFieldTable.GetSize())
	{
		CRdbTableDef* pDefine = m_pDb->GetTableDefine(sTableName);
		bool bHave = false;
		for(uint32 i=0; i<pDefine->nFieldCount; ++i)
		{
			CRdbFieldDef& oField = pDefine->pFieldDefines[i];
			if(oField.nJob >= 0)
			{
				if(bHave)
					oList += ", ";
				oList += oField.sFieldName;
				bHave = true;
			}
		}
		m_pDb->ReleaseTableDefine(sTableName);
	}
	else
	{
		uint32 nCount = m_oSelect.m_oFieldTable.GetSize();
		for(uint32 i=0; i<nCount; ++i)
		{
			if(i)
				oList += ", ";
			oList += m_oSelect.m_oFieldTable[i];
		}
	}
	return oList;
}

uint32 CSqlDataBase::QueryTable()
{
	uint32 nSkipCount = 0;
	if(!m_oSelect.m_pAccess)
		return 0;
	CRdbFilter* pFilter = m_oSelect.m_pAccess->GetResultFilter();
	uint32 nSize = m_oSelect.m_oFieldTable.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		pFilter->SetField(m_oSelect.m_pAccess->GetFieldNo(m_oSelect.m_oFieldTable[i].GetStr()));

	uint32 nRet;

	CString oList = GetFieldList();
	FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("%s", oList.GetStr()));
	m_pEnv->Flush(false);
	while(true)
	{
/*		if(m_nRepeat)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("begin query operator:%u", CTimer::GetTickCount()));
			for(uint32 i=0; i<m_nRepeat; ++i)
			{
				nRet = m_oSelect.m_pAccess->Query(m_nResultSize, nSkipCount);
				if(nRet)
					FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::QueryTable(%s) failure", m_oSelect.m_oName.GetStr()));
			}
			FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("end query operator:%u", CTimer::GetTickCount()));
		}
		else
*/
		nRet = m_oSelect.m_pAccess->Query(m_nResultSize, nSkipCount);
		m_oSelect.m_pRecordSet = m_oSelect.m_pAccess->GetResultSet();
		if(nRet)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::QueryTable(%s) failure", m_oSelect.m_oName.GetStr()));
			m_pEnv->Flush(false);
			break;
		}
		else
		{
			uint32 nCount = m_oSelect.GetResultCount();
			for(uint32 i=0; i<nCount; ++i)
			{
				CRdbResult* pRecord = m_oSelect.GetRecord(i);
				FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("%s", GetRecordText(pRecord, oList).GetStr()));
				m_pEnv->Flush(false);
			}
			if(nCount < m_nResultSize)
				break;
			nSkipCount += nCount;
		}
	}
	m_oSelect.UnBind();
	return 0;
}

CString CSqlDataBase::GetRecordText(CRdbResult* pRecord, CString& oFieldList)
{
	char sName[RDB_NAME_MAXLEN+1];
	CString oRet;
	const char* s = oFieldList.GetStr();
	uint32 nCount = 0;
	while(s)
	{
		char* pShift = (char*)CString::CharOfString(s, ',');
		if(pShift)
		{
			uint32 nLen = pShift - s;
			CBinary::MemoryCopy(sName, s, nLen);
			sName[nLen] = 0;
		}
		else
			CString::StringCopy(sName, s);
		s = (const char*)pShift;
		if(s) s += 2;
		if(nCount)
			oRet += ", ";

		uint32 nFieldNo = m_oSelect.m_pAccess->GetFieldNo((const char*)sName);
		if(nFieldNo == (uint32)(-1))
			oRet += "NULL";
		else
		{
			uint32 nSize = pRecord->GetStringSize(nFieldNo);
			char * pStr = new char[nSize+1];
			pRecord->GetAsString(nFieldNo, pStr);
			if(!pStr[0])
				oRet += "NULL";
			else
			{
				char* pTmp = pStr;
				while(pTmp[0])
				{
					if(pTmp[0] == '\\' || pTmp[0] == ',' || isspace(pTmp[0]))
					{
						char cBuf[6];
						StringPrint(cBuf, "\\x%2x", pTmp[0]);
						oRet += cBuf;
					}
					else
						oRet += pTmp[0];
					++pTmp;
				}
			}
			delete[] pStr;
		}
		++nCount;
	}
	return oRet;
}

void CSqlDataBase::SetResultSize(uint32 nResultSize)
{
	m_nResultSize = nResultSize;
}

uint32 CSqlDataBase::DescTable(char* sTableName)
{
	if(sTableName)
	{
		uint32 i;
		CRdbTableDef* pTableDefine = m_pDb->GetTableDefine((const char*)sTableName);
		if(!pTableDefine)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there is not the table '%s'", sTableName));
			return RDB_TABLE_NOTEXIST;
		}
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("("));
		for(i=0; i<pTableDefine->nFieldCount; )
		{
			CRdbFieldDef& oField = pTableDefine->pFieldDefines[i];
			if(oField.nJob < 0)
			{
				++i;
				continue;
			}
			++i;

			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, (" %s %s%s%s%s", 
				oField.sFieldName, 
				GetTypeName(oField.nType, oField.nLen, oField.nRecSize).GetStr(),
				oField.bNotNull?" not null":"",
				oField.sDefault&&oField.sDefault[0]?(CString(" default='")+oField.sDefault+"'").GetStr():"", 
				(i<pTableDefine->nFieldCount)?",":""));
		}
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, (");"));
		uint32 nIndexCount;
		m_pDb->GetIndexCount((const char*)sTableName, nIndexCount);
		if(nIndexCount)
		{
			char sIndexName[RDB_NAME_MAXLEN+1];
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("the table have %u indexes", nIndexCount));
			for(i=0; i<nIndexCount;)
			{
				m_pDb->GetIndex((const char*)sTableName, i, sIndexName);
				 ++i;
				CRdbIndexDef* pIndexDefine = m_pDb->QueryIndexDefine((const char*)sIndexName);
				FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%sindex %s on(%s) by %s%s%s",
					(pIndexDefine->nQualifier==RDB_UNIQUE_INDEX)?"unique ":"", 
					sIndexName,
					pIndexDefine->pFieldList, 
					GetArithmeticName(pIndexDefine->nArithmetic, pIndexDefine->nHashRate).GetStr(),
					pIndexDefine->sPrimaryIndex?((CString(" foreign(")+pIndexDefine->sPrimaryIndex+")").GetStr()):"",
					(i<nIndexCount)?",":""));
				m_pDb->ReleaseIndexDefine(sIndexName);
			}
		}
		m_pDb->ReleaseTableDefine(sTableName);
	}
	else
	{
		CString oTableList;
		char sName[RDB_NAME_MAXLEN+1];
		m_pDb->QueryTableList(oTableList);
		const char* s = oTableList.GetStr();
		uint32 nCount = 0;
		while(s)
		{
			char* pShift = CString::CharOfString((const char*)s, ',');
			if(pShift)
			{
				uint32 nLen = pShift - s;
				CBinary::MemoryCopy(sName, s, nLen);
				sName[nLen] = 0;
			}
			else
				CString::StringCopy(sName, s);
			if(!sName[0])
				break;
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s", sName));
			s = pShift;
			if(s) ++s;
			++nCount;
		}
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("total %u tables", nCount));
	}
	return RDB_SUCCESS;
}

CString CSqlDataBase::GetTypeName(uint32 nType, uint32 nLen, uint32 nRecSize)
{
	CString oRet;
	char sLen[64];
	switch(nType)
	{
	case RDB_INT8_FIELD:
		oRet += "int8";
		break;
	case RDB_INT16_FIELD:
		oRet += "int16";
		break;
	case RDB_INT32_FIELD:
		oRet += "int32";
		break;
	case RDB_INT64_FIELD:
		oRet += "int64";
		break;
	case RDB_UINT8_FIELD:
		oRet += "uint8";
		break;
	case RDB_UINT16_FIELD:
		oRet += "uint16";
		break;
	case RDB_UINT32_FIELD:
		oRet += "uint32";
		break;
	case RDB_UINT64_FIELD:
		oRet += "uint64";
		break;
	case RDB_FLOAT_FIELD:
		oRet += "float";
		break;
	case RDB_DOUBLE_FIELD:
		oRet += "double";
		break;
	case RDB_CHAR_FIELD:
		oRet += "string";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case RDB_LCHAR_FIELD:
		oRet += "cstring";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case RDB_VARCHAR_FIELD:
		oRet += "vstring";
		StringPrint(sLen, " size=%u recsize=%u", nLen, nRecSize);
		oRet += sLen;
		break;
	case RDB_VARLCHAR_FIELD:
		oRet += "vcstring";
		StringPrint(sLen, " size=%u recsize=%u", nLen, nRecSize);
		oRet += sLen;
		break;
	case RDB_RAW_FIELD:
		oRet += "raw";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case RDB_VARRAW_FIELD:
		oRet += "vraw";
		StringPrint(sLen, " size=%u recsize=%u", nLen, nRecSize);
		oRet += sLen;
		break;
	}
	return oRet;
}

CString CSqlDataBase::GetArithmeticName(uint32 nArithmetic, uint32 nHashRate)
{
	CString oRet;
	char sLen[64];
	switch(nArithmetic)
	{
	case RDB_RBTREE_INDEX:
		oRet += "rbtree";
		break;
	case RDB_NTREE_INDEX:
		oRet += "ntree";
		break;
	case RDB_HASH_INDEX:
		oRet += "hash";
		StringPrint(sLen, "(%u)", nHashRate);
		oRet += sLen;
		break;
	}
	return oRet;
}

//-----------------------------------------------------------------
// CSqlReadWriteObject
//-----------------------------------------------------------------
static const char* GetFileLine(void* pTerminal)
{
	CFileTerminal* pFileTerm = (CFileTerminal*)pTerminal;
	CFileFormatter(&pFileTerm->fp).Scan("r", &pFileTerm->oLine);
	return pFileTerm->oLine.GetStr();
}

static void GetFileBufferSize(void*, uint32 * pReadSize, uint32 * pWriteSize)
{
	pReadSize[0] = 1024;
	pWriteSize[0] = 1024;
}

CSqlReadWriteObject::CSqlReadWriteObject(CSqlTerminal* pTerminal, CSqlEnv* pEnv)
{
	m_pTerminal = pTerminal;
	pTerminal->BufferSize(pTerminal->pTerminal, &m_nReadBufSize, &m_nWriteBufSize);
	m_sReadBuf = 0;
	m_nReadSize = 0;
	m_pTerminalList = NULL;
	m_pEnv = pEnv;
}

CSqlReadWriteObject::~CSqlReadWriteObject()
{
}

bool CSqlReadWriteObject::PushFile(char* sFileName)
{
	CFileTerminal* pFileTerminal = new CFileTerminal;
	if(pFileTerminal->fp.Open(sFileName, "r"))
	{
		delete pFileTerminal;
        FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("open script file '%s' failure", sFileName));
		return false;
	}
	CSqlTerminal* pSqlTerminal = new CSqlTerminal;
	pSqlTerminal->pTerminal = pFileTerminal;
	pSqlTerminal->GetLine = GetFileLine;
	pSqlTerminal->BufferSize = GetFileBufferSize;
	CFileTerminalList* pList = new CFileTerminalList;
	pList->pTerminal = pSqlTerminal;
	pList->pNext = m_pTerminalList;
	m_pTerminalList = pList;
	return true;
}

int CSqlReadWriteObject::read(char* s, uint32 nSize)
{
	CSqlTerminal* pTerminal;
	while(!m_nReadSize)
	{
		if(m_pTerminalList)
			pTerminal = m_pTerminalList->pTerminal;
		else
			pTerminal = m_pTerminal;
		m_sReadBuf = (char*)pTerminal->GetLine(pTerminal->pTerminal);
		m_nReadSize = CString::StringLength(m_sReadBuf);
		if(!m_nReadSize)
		{
			if(!m_pTerminalList)
				break;
			CFileTerminal* pFileTerm = (CFileTerminal*)pTerminal->pTerminal;
			delete pFileTerm;
			delete pTerminal;
			CFileTerminalList* pList = m_pTerminalList->pNext;
			delete m_pTerminalList;
			m_pTerminalList = pList;
		}
	}
	uint32 nFactSize = m_nReadSize;
	if(nFactSize > nSize)
		nFactSize = nSize;
	if(nFactSize)
	{
		CBinary::MemoryCopy(s, m_sReadBuf, nFactSize);
		m_nReadSize -= nFactSize;
		m_sReadBuf += nFactSize;
	}
	return nFactSize;
}

int CSqlReadWriteObject::write(const char* s, uint32 nSize)
{
	m_oText.Insert((uint32)(-1), s);
	return nSize;
}

void CSqlReadWriteObject::Flush(bool bCarriage)
{
	uint32 nSize = m_oText.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		m_pTerminal->PutLine(m_pTerminal->pTerminal, m_oText[i].GetStr());
	m_oText.Clear();
	if(bCarriage)
		m_pTerminal->PutLine(m_pTerminal->pTerminal, "");
}

int CSqlReadWriteObject::readbufsize()
{
	return m_nReadBufSize;
}

int CSqlReadWriteObject::writebufsize()
{
	return m_nWriteBufSize;
}

//-----------------------------------------------------------------
// CSqlEnv
//-----------------------------------------------------------------
void SqlLog(void* pSqlEnv, uint32 nLogLevel, const char* sFormat, ...)
{
	char sBuffer[2048];
	va_list pArgList;
	va_start(pArgList, sFormat);
	StringPrintExV(sBuffer, 2047, sFormat, pArgList);
	va_end(pArgList);

	CSqlEnv* pEnv = (CSqlEnv*)pSqlEnv;
	CSqlStream& oStream = pEnv->GetStream();
	oStream.GetStream()->write(sBuffer, CString::StringLength(sBuffer)+1);
	oStream.GetStream()->flush();

	va_start(pArgList, sFormat);
	FocpLogVx("SQL", nLogLevel, sFormat, pArgList);
	va_end(pArgList);
}

static CDefaultReadWrite<char> g_oDefWrObj;

CSqlEnv::CSqlEnv(CSqlTerminal* pTerminal, char* sDataBakPath, char* sDataBakPath2):
yyFlexLexer(NULL, NULL),
m_oDataBase(this),
m_oOutStream(g_oDefWrObj, ios_base::in | ios_base::out),
m_oTerminal(*pTerminal),
m_oRwObj(&m_oTerminal, this),
m_oStream(m_oRwObj, ios_base::in | ios_base::out),
m_sDataBakPath(sDataBakPath),
m_sDataBakPath2(sDataBakPath2)
{
	switch_streams(m_oStream.GetStream(), m_oOutStream.GetStream());
}

CSqlEnv::~CSqlEnv()
{
}

void CSqlEnv::Backup()
{
	CRdbSystem::GetInstance()->Backup(m_sDataBakPath, m_sDataBakPath2);
}

CSqlDataBase* CSqlEnv::GetDataBase()
{
	if(!m_oDataBase.m_pDb)
		return NULL;
	return &m_oDataBase;
}

CSqlStream& CSqlEnv::GetStream()
{
	return m_oStream;
}

uint32 CSqlEnv::SelectDataBase(const CString& oName)
{
	CRdb *pDb = CRdbSystem::GetInstance()->QueryDataBase(oName.GetStr());
	if(!pDb)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, 
			("CSqlEnv::SelectDataBase(%s) failure", oName.GetStr()));
		return RDB_DB_NOTEXIST;
	}
	m_oDataBase.Bind(pDb);
	FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, 
		("CSqlEnv::SelectDataBase(%s) success", oName.GetStr()));
	return RDB_SUCCESS;
}

uint32 CSqlEnv::CreateDataBase(const CString& oName)
{
	uint32 nRet = CRdbSystem::GetInstance()->CreateDataBase(oName.GetStr());
	if(nRet)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, 
			("CSqlEnv::CreateDataBase(%s) failure", oName.GetStr()));
	}
	else
	{
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, 
			("CSqlEnv::CreateDataBase(%s) success", oName.GetStr()));
	}
	return nRet;
}

uint32 CSqlEnv::RemoveDataBase(const CString& oName)
{
	CRdbSystem* pRdbSys = CRdbSystem::GetInstance();
	CRdb *pDb = pRdbSys->QueryDataBase(oName.GetStr());
	if(!pDb)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, 
			("CSqlEnv::RemoveDataBase(%s) failure: RDB_DB_NOTEXIST", oName.GetStr()));
		return RDB_DB_NOTEXIST;
	}
	if(pDb == m_oDataBase.m_pDb)
	{
		pDb->Release();
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, 
			("CSqlEnv::RemoveDataBase(%s) failure: RDB_DB_BUSY", oName.GetStr()));
		return RDB_DB_BUSY;
	}
	pDb->Release();
	uint32 nRet = pRdbSys->DestroyDataBase(oName.GetStr());
	if(nRet)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, 
			("CSqlEnv::RemoveDataBase(%s) failure: RDB_DB_NOTEXIST", oName.GetStr()));
		return RDB_DB_NOTEXIST;
	}
	FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, 
		("CSqlEnv::RemoveDataBase(%s) success", oName.GetStr()));
	return nRet;
}

uint32 CSqlEnv::DescAllDataBase()
{
	char sName[RDB_NAME_MAXLEN+1];
	CString oList;
	CRdbSystem::GetInstance()->QueryDataBaseList(oList);
	const char* s = oList.GetStr();
	uint32 nCount = 0;
	while(s)
	{
		char* pShift = CString::CharOfString(s, ',');
		if(pShift)
		{
			uint32 nLen = pShift - s;
			CBinary::MemoryCopy(sName, s, nLen);
			sName[nLen] = 0;
		}
		else
			CString::StringCopy(sName, s);
		if(!sName[0])
			break;
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s", sName));
		s = pShift;
		if(s) ++s;
		++nCount;
	}
	FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("total %u databases", nCount));
	return 0;
}

void CSqlEnv::Help()
{
	const char* sBuf[] = 
	{
		"QUIT",
		"HELP",
		"BATCH 'filename' ; ",
		"BACKUP ;",
		"USE dbname ; ",
		"SELECT * | fieldname {, fieldname} FROM tablename [WHERE cond]; ",
		"INSERT INTO tablename [( fieldname {, fieldname} )] ",
			"\tVALUES ( value {, value} ) ; ",
		"DELETE FROM tablename [WHERE cond] ; ",
		"UPDATE tablename SET fieldname=value {, fieldname=value}",
			"\t[WHERE cond]; ",
		"TRUNCATE TABLE tablename ; ",
		"CREATE [UNIQUE] [FOREIGN ( primaryidxname ) ] INDEX idxname",
			"\tON tablename ( fieldname {, fieldname} ) ",
			"\t[BY (RBTREE|NTREE|HASH [( hashrate )] )] ; ",
		"DROP INDEX idxname ; ",
#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
		"CREATE [TEMPORARY] TABLE tablename ( field_define, {field_define} ) ; ",
#else
		"CREATE TABLE tablename ( field_define, {field_define} ) ; ",
#endif
		"MODIFY TABLE tablename ( field_define, {field_define} ) ; ",
		"DROP TABLE tablename ; ",
		"CREATE DATABASE dbname ; ",
		"DROP DATABASE dbname ; ",
		"DESC ((TABLE tablename|*) | (DATABASE *)) ; ",
#if defined(RDB_SUPPORT_MEMORY_DB)
		"CREATE TEMPORARY DEVICE devicesize ; ",
#endif
		"CREATE DEVICE 'devicefile' [devicesize] ; ",
		"",
		"-----------------------------------------------",
		"",
		"cond := fieldname (< | <= | = | >= | >) value",
			"\t cond AND cond",
		"field_define := fieldname type [size=number] [recsize=number] [NOT NULL] [default=defalutvalue]",
		"type := int8 int16 int32 int64 uint8 uint16 uint32 uint64 float double",
		"		 string cstring vstring vcstring raw vraw"
	};
	uint32 nCount = sizeof(sBuf)/sizeof(char*);
	for(uint32 i=0; i<nCount; ++i)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s", sBuf[i]));
	}
}

void CSqlEnv::Flush(bool bCarriage)
{
	m_oRwObj.Flush(bCarriage);
}

void CSqlEnv::ExecuteFile(char* sScriptFile)
{
	if(!m_oRwObj.PushFile(sScriptFile))
		Flush();
}

void CSqlEnv::Interpret()
{
	int yyparse(void* lexer);
	yyparse(this);
}

FOCP_END();
