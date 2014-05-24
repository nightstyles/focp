
#include "SqlEnv.hpp"
#include "MdbError.hpp"
#include "MdbAccess.hpp"

FOCP_BEGIN();

//-----------------------------------------------------------------
// CSqlField
//-----------------------------------------------------------------
CSqlField::CSqlField():
	type(0), len(0), null(false)
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

void CSqlTable::Bind(const CString& oName)
{
	m_nFldNo = 0;
	m_oName = oName;
	m_nMaxRecordNum = 0xFFFFFFFF;
	m_oFieldTable.Clear();
}

CSqlField* CSqlTable::FindField(const CString& oName)
{
	CRbTreeNode* r = m_oFieldTable.Find(oName);
	if(r == m_oFieldTable.End())
		return NULL;
	return (CSqlField*)&(m_oFieldTable.GetItem(r));
}

CSqlField* CSqlTable::AddField(const CString& oName)
{
	CSqlField a;
	a.name = oName;
	CRbTreeNode* r = m_oFieldTable.Insert(a);
	if(r == m_oFieldTable.End())
	{
		if(m_oName.Empty())
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlTable::AddField(%s) -> Redefine alias type", oName.GetStr()));
		else
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlTable::AddField(%s) -> Redefine field in the table '%s'", oName.GetStr(), m_oName.GetStr()));
		return NULL;
	}
	m_pField = (CSqlField*)&(m_oFieldTable.GetItem(r));
	m_pField->seq = ++m_nFldNo;
	m_pField->name = oName;
	return m_pField;
}

CSqlField* CSqlTable::GetCurrentField()
{
	return m_pField;
}

void CSqlTable::SetMaxRecordNum(uint32 nMaxRecordNum)
{
	m_nMaxRecordNum = nMaxRecordNum;
}

//-----------------------------------------------------------------
// CSqlOperator
//-----------------------------------------------------------------
CSqlOperator::CSqlOperator()
{
	m_pAccess = NULL;
	m_bError = false;
}

CSqlOperator::~CSqlOperator()
{
	UnBind();
}

void CSqlOperator::Bind(const CString &oName, CMdbAccess * pAccess)
{
	UnBind();
	m_oName = oName;
	m_pAccess = pAccess;
	m_bError = false;
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

void CSqlInsert::Bind(const CString &oName, CMdbAccess * pAccess)
{
	CSqlOperator::Bind(oName, pAccess);
	m_pInsertAttr = pAccess->GetInsertPara();
	m_nFldNo = 0;
	m_oFieldTable.Clear();
}

bool CSqlInsert::AddValue(const CString& oVal)
{
	if(m_pAccess == NULL)
		return false;
	uint32 nFldNo = m_nFldNo++;
	if(!m_oFieldTable.GetSize())
	{
		uint32 nFieldCount = m_pAccess->GetFieldCount();
		if(nFldNo < nFieldCount)
			m_pInsertAttr->SetFromString(nFldNo, (char*)oVal.GetStr(), MDB_SQLPARA_OPERATOR_EQUAL);
		else
		{
			m_bError = true;
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
			m_bError = true;
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlInsert::AddValue() there isn't the field no '%u' in the table '%s'", nFldNo, m_oName.GetStr()));
			return false;
		}
		m_pInsertAttr->SetFromString(nFieldNo, oVal.GetStr(), MDB_SQLPARA_OPERATOR_EQUAL);
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

void CSqlWhere::Bind(const CString &oName, CMdbAccess * pAccess)
{
	CSqlOperator::Bind(oName, pAccess);
	m_pWhere = pAccess->GetQueryPara();
}

void CSqlWhere::NewSet()
{
	m_pWhere->AddParaSet();
}

bool CSqlWhere::AddCond(const CString& oName, uint32 nOp, const CString& oVal)
{
	if(m_pAccess == NULL)
		return false;
	uint32 nFieldNo = m_pAccess->GetFieldNo(oName.GetStr());
	if(nFieldNo == (uint32)(-1))
	{
		m_bError = true;
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlWhere::AddCond() there isn't the field '%s' in the table '%s'", oName.GetStr(), m_oName.GetStr()));
		return false;
	}
	bool bSet;
	CMdbParaSet* pParaSet;
	uint32 i, nCount = m_pWhere->GetParaCount();
	if(nCount)
		pParaSet = (CMdbParaSet*)m_pWhere->GetPara(nCount - 1, bSet);
	else
		pParaSet = (CMdbParaSet*)m_pWhere->AddParaSet();
	nCount = pParaSet->GetParaCount();
	for(i=0; i<nCount; ++i)
	{
		CMdbSqlPara* pPara = (CMdbSqlPara*)pParaSet->GetPara(i, bSet);
		if(!pPara->IsSetField(nFieldNo))
		{
			pPara->SetFromString(nFieldNo, oVal.GetStr(), nOp);
			return true;
		}
	}
	CMdbSqlPara* pPara = (CMdbSqlPara*)pParaSet->AddPara();
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

void CSqlUpdate::Bind(const CString &oName, CMdbAccess * pAccess)
{
	CSqlWhere::Bind(oName, pAccess);
	m_pSetAttr = pAccess->GetUpdatePara();
}

bool CSqlUpdate::AddSetPara(const CString& oName, const CString& oVal, uint32 nOp)
{
	if(m_pAccess == NULL)
		return false;
	uint32 nFieldNo = m_pAccess->GetFieldNo(oName.GetStr());
	if(nFieldNo == (uint32)(-1))
	{
		m_bError = true;
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

CMdbResult* CSqlSelect::GetRecord(uint32 nRow)
{
	return m_pRecordSet->GetResult(nRow);
}

//-----------------------------------------------------------------
// CSqlIndex
//-----------------------------------------------------------------
CSqlIndex::CSqlIndex():
	nQualifier(MDB_COMMON_INDEX), nArithmetic(MDB_HASH_INDEX), nHashRate(300)
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
	m_nResultSize = 1024;
	m_nRepeat = 0;
	m_oAliasTable.Bind("");
	m_pCurrentTable = &m_oAliasTable;
}

CSqlDataBase::~CSqlDataBase()
{
}

void CSqlDataBase::Bind(CMdb* pDb)
{
	m_pDb = pDb;
}

void CSqlDataBase::SetRepeat(uint32 nRepeat)
{
	m_nRepeat = nRepeat;
}

uint32 CSqlDataBase::CreateSqlTable(const CString& oName)
{
	m_oTable.Bind(oName);
	m_pCurrentTable = &m_oTable;
	return MDB_SUCCESS;
}

uint32 CSqlDataBase::CreateTable()
{
	m_pCurrentTable = &m_oAliasTable;
	CRbMap<uint32, CSqlField*> OrderFieldTable;
	CRbTree<CString,CSqlField,CSqlGetFieldName,CNameCompare>& oFieldTable = m_oTable.m_oFieldTable;
	CRbTreeNode* e = oFieldTable.End();
	for(CRbTreeNode* i = oFieldTable.First(); i != e; i=oFieldTable.GetNext(i))
	{
		CSqlField& oField = oFieldTable.GetItem(i);
		OrderFieldTable[oField.seq] = &oField;
	}

	CMdbTableDef oTabDef;
	CBinary::MemorySet(&oTabDef, 0, sizeof(oTabDef));
	oTabDef.nMaxRecordNum = m_oTable.m_nMaxRecordNum;
	oTabDef.sTableName = (char*)m_oTable.m_oName.GetStr();
	oTabDef.nFieldCount = OrderFieldTable.GetSize();
	oTabDef.pFieldDefines = new CMdbFieldDef[oTabDef.nFieldCount];

	uint32 j = 0;
	e = OrderFieldTable.End();
	for(CRbTreeNode* k = OrderFieldTable.First();
			k != e;
			k=OrderFieldTable.GetNext(k), ++j)
	{
		CMdbFieldDef& oField = oTabDef.pFieldDefines[j];
		CSqlField* pField = OrderFieldTable.GetItem(k);
		oField.sFieldName = (char*)pField->name.GetStr();
		oField.sDefault = (char*)pField->defval.GetStr();
		oField.nType = pField->type;
		oField.nLen = pField->len;
		oField.bNotNull = pField->null;
		oField.pExtendAttr = NULL;
		if(!oField.sDefault[0])
			oField.sDefault = NULL;
	}

	CMdbLocalInterface* pItf = m_pDb->GetLocalInterface();
	if(pItf == NULL)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateTable(%s.%s) failure: MDB_INVALID_INTERFACE", m_pDb->GetDbName(), oTabDef.sTableName));
		return MDB_INVALID_INTERFACE;
	}
	uint32 nRet = pItf->CreateTable(&oTabDef);
	delete [] oTabDef.pFieldDefines;
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateTable(%s.%s) failure: %s", m_pDb->GetDbName(), oTabDef.sTableName, GetMdbError(nRet)));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::CreateTable(%s.%s) success", m_pDb->GetDbName(), oTabDef.sTableName));
	return nRet;
}

uint32 CSqlDataBase::CreateSqlIndex()
{
	m_oIndex.oIndexFieldTable.Clear();
	return MDB_SUCCESS;
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

	CMdbIndexDef oIdxDef;
	CBinary::MemorySet(&oIdxDef, 0, sizeof(oIdxDef));
	oIdxDef.sIndexName = (char*)m_oIndex.oIndexName.GetStr();
	oIdxDef.sTableName = (char*)m_oIndex.oTableName.GetStr();
	oIdxDef.sPrimaryIndex = (char*)m_oIndex.oPrimaryIndex.GetStr();
	if(!oIdxDef.sPrimaryIndex[0])
		oIdxDef.sPrimaryIndex = NULL;
	oIdxDef.nQualifier = m_oIndex.nQualifier;
	oIdxDef.nArithmetic = m_oIndex.nArithmetic;
	oIdxDef.nHashRate = m_oIndex.nHashRate;
	oIdxDef.pFieldList = (char*)oFieldName.GetStr();

	CMdbLocalInterface* pItf = m_pDb->GetLocalInterface();
	if(pItf == NULL)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateIndex(%s.%s) failure: MDB_INVALID_INTERFACE", m_pDb->GetDbName(), oIdxDef.sIndexName));
		return MDB_INVALID_INTERFACE;
	}
	uint32 nRet = pItf->CreateIndex(&oIdxDef);
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateIndex(%s.%s) failure: %s", m_pDb->GetDbName(), oIdxDef.sIndexName, GetMdbError(nRet)));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::CreateIndex(%s.%s) success", m_pDb->GetDbName(), oIdxDef.sIndexName));
	return nRet;
}

uint32 CSqlDataBase::CreateSqlInsert(const CString& oName)
{
	CMdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlInsert(%s.%s) failure: MDB_TABLE_NOTEXIST", m_pDb->GetDbName(), oName.GetStr()));
		return MDB_TABLE_NOTEXIST;
	}
	m_oInsert.Bind(oName, pAccess);
	return MDB_SUCCESS;
}

uint32 CSqlDataBase::InsertRecord()
{
	if(!m_oInsert.m_pAccess)
		return MDB_SUCCESS;
	if(m_oInsert.m_bError)
		return MDB_SUCCESS;
	uint32 nRet = m_oInsert.m_pAccess->Insert();
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::InsertRecord(%s.%s) failure: %s", m_pDb->GetDbName(), m_oInsert.m_oName.GetStr(), GetMdbError(nRet)));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_DEBUG, ("CSqlDataBase::InsertRecord(%s.%s) success", m_pDb->GetDbName(), m_oInsert.m_oName.GetStr()));
	m_oInsert.UnBind();
	return nRet;
}

uint32 CSqlDataBase::CreateSqlDelete(const CString& oName)
{
	CMdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlDelete(%s.%s) failure: MDB_TABLE_NOTEXIST", m_pDb->GetDbName(), oName.GetStr()));
		return MDB_TABLE_NOTEXIST;
	}
	m_oDelete.Bind(oName, pAccess);
	m_pWhere = &m_oDelete;
	return MDB_SUCCESS;
}

uint32 CSqlDataBase::DeleteRecord()
{
	if(!m_oDelete.m_pAccess)
		return MDB_SUCCESS;
	if(m_oDelete.m_bError)
		return MDB_SUCCESS;
	uint32 nRet = m_oDelete.m_pAccess->Delete();
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::DeleteRecord(%s.%s) failure: %s", m_pDb->GetDbName(), m_oDelete.m_oName.GetStr(), GetMdbError(nRet)));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_DEBUG, ("CSqlDataBase::DeleteRecord(%s.%s) success", m_pDb->GetDbName(), m_oDelete.m_oName.GetStr()));
	m_oDelete.UnBind();
	return nRet;
}

uint32 CSqlDataBase::CreateSqlUpdate(const CString& oName)
{
	CMdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlUpdate(%s.%s) failure: MDB_TABLE_NOTEXIST", m_pDb->GetDbName(), oName.GetStr()));
		return MDB_TABLE_NOTEXIST;
	}
	m_oUpdate.Bind(oName, pAccess);
	m_pWhere = &m_oUpdate;
	return MDB_SUCCESS;
}

uint32 CSqlDataBase::UpdateRecord()
{
	if(!m_oUpdate.m_pAccess)
		return MDB_SUCCESS;
	if(m_oUpdate.m_bError)
		return MDB_SUCCESS;
	uint32 nRet = m_oUpdate.m_pAccess->Update();
	if(nRet)
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::UpdateRecord(%s.%s) failure: %s", m_pDb->GetDbName(), m_oUpdate.m_oName.GetStr(), GetMdbError(nRet)));
	else
		FocpCmdLogEx("SQL", FOCP_LOG_DEBUG, ("CSqlDataBase::UpdateRecord(%s.%s) success", m_pDb->GetDbName(), m_oUpdate.m_oName.GetStr()));
	m_oUpdate.UnBind();
	return nRet;
}

uint32 CSqlDataBase::TruncateTable(const CString& oName)
{
	CMdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::TruncateTable(%s.%s) failure: MDB_TABLE_NOTEXIST", m_pDb->GetDbName(), oName.GetStr()));
		return MDB_TABLE_NOTEXIST;
	}
	pAccess->Truncate();
	pAccess->Release();
	FocpCmdLogEx("SQL", FOCP_LOG_SYSLOG, ("CSqlDataBase::TruncateTable(%s.%s) success", m_pDb->GetDbName(), oName.GetStr()));
	return 0;
}

void CSqlDataBase::PrepareSqlSelect()
{
	m_oSelect.Clear();
}

uint32 CSqlDataBase::CreateSqlSelect(const CString& oName)
{
	CMdbAccess * pAccess = m_pDb->QueryAccess(oName.GetStr());
	if(!pAccess)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::CreateSqlSelect(%s.%s) failure: MDB_TABLE_NOTEXIST", m_pDb->GetDbName(), oName.GetStr()));
		return MDB_TABLE_NOTEXIST;
	}
	m_oSelect.Bind(oName, pAccess);
	m_pWhere = &m_oSelect;
	return MDB_SUCCESS;
}

CString CSqlDataBase::GetFieldList()
{
	CString oList;
	const char* sTableName = m_oSelect.m_oName.GetStr();
	if(!m_oSelect.m_oFieldTable.GetSize())
	{
		CMdbTableDef* pDefine = m_pDb->GetTableDefine(sTableName);
		bool bHave = false;
		for(uint32 i=0; i<pDefine->nFieldCount; ++i)
		{
			CMdbFieldDef& oField = pDefine->pFieldDefines[i];
			if(bHave)
				oList += ", ";
			oList += oField.sFieldName;
			bHave = true;
		}
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

void CSqlDataBase::OrderBy(const CString& oIdxName, bool bAsc)
{
	if(m_oSelect.m_pAccess && !m_oSelect.m_pAccess->SetOrderBy(oIdxName.GetStr(), bAsc))
		FocpCmdLogEx("SQL", FOCP_LOG_WARNING, ("CSqlDataBase::OrderBy(%s.%s.%s) failure", m_pDb->GetDbName(), m_oSelect.m_oName.GetStr(), oIdxName.GetStr()));
}

uint32 CSqlDataBase::QueryTable()
{
	if(!m_oSelect.m_pAccess)
		return 0;

	CMdbFilter* pFilter = m_oSelect.m_pAccess->GetResultFilter();
	uint32 nSize = m_oSelect.m_oFieldTable.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		pFilter->SetField(m_oSelect.m_pAccess->GetFieldNo(m_oSelect.m_oFieldTable[i].GetStr()));

	uint32 nRet;
	if(m_oSelect.m_bError)
		return 0;
	uint32 nTotal = 0;
	CString oList = GetFieldList();
	while(true)
	{
		nRet = m_oSelect.m_pAccess->Query(m_nResultSize, nTotal);
		m_oSelect.m_pRecordSet = m_oSelect.m_pAccess->GetResultSet();
		if(nRet)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("CSqlDataBase::QueryTable(%s.%s) failure: %s", m_pDb->GetDbName(), m_oSelect.m_oName.GetStr(), GetMdbError(nRet)));
			m_pEnv->Flush(false);
			m_oSelect.UnBind();
			return 0;
		}
		else
		{
			if(!nTotal)
			{
				FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s", oList.GetStr()));
				m_pEnv->Flush(false);
			}
			uint32 nCount = m_oSelect.GetResultCount();
			for(uint32 i=0; i<nCount; ++i)
			{
				CMdbResult* pRecord = m_oSelect.GetRecord(i);
				FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s", GetRecordText(pRecord, oList).GetStr()));
				m_pEnv->Flush(false);
			}
			nTotal += nCount;
			if(nCount < m_nResultSize)
				break;
		}
	}
	FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("\r\ntotal %u records in %s.%s", nTotal, m_pDb->GetDbName(), m_oSelect.m_oName.GetStr()));
	m_oSelect.UnBind();
	return 0;
}

CString CSqlDataBase::GetRecordText(CMdbResult* pRecord, CString& oFieldList)
{
	char sName[MDB_NAME_MAXLEN+1];
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
					switch(pTmp[0])
					{
//					case ' ':   oRet += "\\w"; break;
					case '\r':
						oRet += "\\r";
						break;
					case '\n':
						oRet += "\\n";
						break;
					case '\f':
						oRet += "\\f";
						break;
					case '\v':
						oRet += "\\v";
						break;
					case '\t':
						oRet += "\\t";
						break;
					case '\a':
						oRet += "\\a";
						break;
					case '\b':
						oRet += "\\b";
						break;
					case '\\':
						oRet += "\\\\";
						break;
					default:
						if(pTmp[0] == ',' || !isprint(pTmp[0]))
						{
							char cBuf[6];
							StringPrint(cBuf, "\\X%2X", (uint32)(uint8)pTmp[0]);
							oRet += cBuf;
						}
						else
							oRet += pTmp[0];
						break;
					}
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
		CMdbTableDef* pTableDefine = m_pDb->GetTableDefine((const char*)sTableName);
		if(!pTableDefine)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there is not the table '%s.%s'", m_pDb->GetDbName(), sTableName));
			return MDB_TABLE_NOTEXIST;
		}
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("("));
		for(i=0; i<pTableDefine->nFieldCount; )
		{
			CMdbFieldDef& oField = pTableDefine->pFieldDefines[i];
			++i;
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, (" %s %s%s%s%s",
												 oField.sFieldName,
												 GetTypeName(oField.nType, oField.nLen).GetStr(),
												 oField.bNotNull?" not null":"",
												 oField.sDefault&&oField.sDefault[0]?(CString(" default='")+oField.sDefault+"'").GetStr():"",
												 (i<pTableDefine->nFieldCount)?",":""));
		}
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, (");"));
		uint32 nIndexCount = pTableDefine->pExtendAttr->nIndexCount;
		if(nIndexCount)
		{
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("the table have %u indexes", nIndexCount));
			for(i=0; i<nIndexCount;)
			{
				CMdbIndexDef* pIndexDefine = pTableDefine->pExtendAttr->pIndexDefineSet[i++];
				FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%sindex %s on(%s) by %s%s%s",
													 (pIndexDefine->nQualifier==MDB_UNIQUE_INDEX)?"unique ":"",
													 pIndexDefine->sIndexName,
													 pIndexDefine->pFieldList,
													 GetArithmeticName(pIndexDefine->nArithmetic, pIndexDefine->nHashRate).GetStr(),
													 pIndexDefine->sPrimaryIndex?((CString(" foreign(")+pIndexDefine->sPrimaryIndex+")").GetStr()):"",
													 (i<nIndexCount)?",":""));
			}
		}
		if(pTableDefine->nMaxRecordNum != 0xFFFFFFFF)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("the table can contain %u records at most", pTableDefine->nMaxRecordNum));
	}
	else
	{
		char sName[MDB_NAME_MAXLEN+1];
		const char* s = m_pDb->GetTableList();
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
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("%s have %u tables", m_pDb->GetDbName(), nCount));
	}
	return MDB_SUCCESS;
}

CString CSqlDataBase::GetTypeName(uint32 nType, uint32 nLen)
{
	CString oRet;
	char sLen[64];
	switch(nType)
	{
	case MDB_INT8_FIELD:
		oRet += "int8";
		break;
	case MDB_INT16_FIELD:
		oRet += "int16";
		break;
	case MDB_INT32_FIELD:
		oRet += "int32";
		break;
	case MDB_INT64_FIELD:
		oRet += "int64";
		break;
	case MDB_UINT8_FIELD:
		oRet += "uint8";
		break;
	case MDB_UINT16_FIELD:
		oRet += "uint16";
		break;
	case MDB_UINT32_FIELD:
		oRet += "uint32";
		break;
	case MDB_UINT64_FIELD:
		oRet += "uint64";
		break;
	case MDB_FLOAT_FIELD:
		oRet += "float";
		break;
	case MDB_DOUBLE_FIELD:
		oRet += "double";
		break;
	case MDB_DATE_FIELD:
		oRet += "date";
		break;
	case MDB_TIME_FIELD:
		oRet += "time";
		break;
	case MDB_DATETIME_FIELD:
		oRet += "datetime";
		break;
	case MDB_CHAR_FIELD:
		oRet += "string";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case MDB_LCHAR_FIELD:
		oRet += "cstring";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case MDB_VARCHAR_FIELD:
		oRet += "vstring";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case MDB_VARLCHAR_FIELD:
		oRet += "vcstring";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case MDB_RAW_FIELD:
		oRet += "raw";
		StringPrint(sLen, " size=%u", nLen);
		oRet += sLen;
		break;
	case MDB_VARRAW_FIELD:
		oRet += "vraw";
		StringPrint(sLen, " size=%u", nLen);
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
	case MDB_RBTREE_INDEX:
		oRet += "rbtree";
		break;
		/*
			case MDB_NTREE_INDEX:
				oRet += "ntree";
				break;
		*/
	case MDB_HASH_INDEX:
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
	if(m_pTerminal->PutLine)
	{
		uint32 nSize = m_oText.GetSize();
		for(uint32 i=0; i<nSize; ++i)
			m_pTerminal->PutLine(m_pTerminal->pTerminal, m_oText[i].GetStr());
		if(bCarriage)
			m_pTerminal->PutLine(m_pTerminal->pTerminal, "");
	}
	m_oText.Clear();
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
static CDefaultReadWrite<char> g_oDefWrObj;

CSqlEnv::CSqlEnv(CSqlTerminal* pTerminal):
	yyFlexLexer(NULL, NULL),
	m_oDataBase(this),
	m_oOutStream(g_oDefWrObj, ios_base::in | ios_base::out),
	m_oTerminal(*pTerminal),
	m_oRwObj(&m_oTerminal, this),
	m_oStream(m_oRwObj, ios_base::in | ios_base::out)
{
	switch_streams(m_oStream.GetStream(), m_oOutStream.GetStream());
}

CSqlEnv::~CSqlEnv()
{
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
	CMdb* pDb = CMdb::GetMdb(oName.GetStr());
	if(!pDb)
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR,
					 ("CSqlEnv::SelectDataBase(%s) failure", oName.GetStr()));
		return MDB_DB_NOTEXIST;
	}
	if(!pDb->GetLocalInterface())
	{
		if(!oName.Compare("sysdb", false))
		{
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR,
						 ("CSqlEnv::SelectDataBase(%s) failure", oName.GetStr()));
			return MDB_DB_NOTEXIST;
		}
	}
	m_oDataBase.Bind(pDb);
	FocpCmdLogEx("SQL", FOCP_LOG_CLOSE,
				 ("CSqlEnv::SelectDataBase(%s) success", oName.GetStr()));
	return MDB_SUCCESS;
}

uint32 CSqlEnv::DescAllDataBase()
{
	char sName[MDB_NAME_MAXLEN+1];
	const char* s = CMdb::GetDbList();
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
		CMdb* pDb = CMdb::GetMdb(sName);
		if(!pDb->GetLocalInterface() && !CString::StringCompare(sName, "sysdb", false))
		{
			s = pShift;
			if(s) ++s;
			continue;
		}
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
		"QUIT ;",
		"HELP ;",
		"BATCH 'filename' ; ",
		"USE dbname ; ",
		"SELECT * | fieldname {, fieldname} FROM tablename [WHERE cond] [ORDERBY indexname [ASC|DEC] ]; ",
		"INSERT INTO tablename [( fieldname {, fieldname} )] ",
		"\tVALUES ( value {, value} ) ; ",
		"DELETE FROM tablename [WHERE cond] ; ",
		"UPDATE tablename SET fieldname setop value {, fieldname setop value}",
		"\t[WHERE cond]; ",
		"TRUNCATE TABLE tablename ; ",
		"CREATE [UNIQUE] [FOREIGN ( primaryidxname ) ] INDEX idxname",
		"\tON tablename ( fieldname {, fieldname} ) ",
		"\t[BY (RBTREE|HASH [( hashrate )] )] ; ",
		"CREATE TABLE tablename ( field_define, {field_define} ) [CAPACITY=value]; ",
		"DESC ((TABLE tablename|*) | (DATABASE *)) ; ",
		"",
		"-----------------------------------------------",
		"",
		"cond := fieldname ('<' | '<=' | '=' | '>=' | '>') value",
		"\tcond AND|OR cond",
		"field_define := fieldname type [size=number] [NOT NULL] [default=defalutvalue]",
		"type := int8 int16 int32 int64 uint8 uint16 uint32 uint64 float double",
		"		 string cstring vstring vcstring raw vraw date time datetime"
		"setop := '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^='"
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
