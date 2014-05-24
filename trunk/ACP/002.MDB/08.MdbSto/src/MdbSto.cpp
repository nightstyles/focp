
#include "MdbSto.hpp"

FOCP_C_BEGIN();

MDBSTO_API bool InitializeMdbStorageAttr(const char* sMdbName, const char* sMdbTabName,
		const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere,
		const char* sStorageIdx, const char* sFieldList)
{
	return FOCP_NAME::CMdbStorager::GetInstance()->InitializeStorageAttr(sMdbName, sMdbTabName,
			sDbTabName, sLoadWhere, sStorageWhere, sCacheWhere, sStorageIdx, sFieldList);
}

MDBSTO_API bool InitializeMdbDataSource(const char* sMdbName, FOCP_NAME::uint32 nDomain, bool bSupportStorage,
										const char* sDSN, const char* sUser, const char* sPasswd,
										const char* sEventDb, const char* sEventTable)
{
	return FOCP_NAME::CMdbStorager::GetInstance()->InitializeDataSource(sMdbName, nDomain, bSupportStorage,
			sDSN, sUser, sPasswd, sEventDb, sEventTable);
}

MDBSTO_API void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FOCP_NAME::FStorageTrigger StorageTrigger)
{
	FOCP_NAME::CMdbStorager::GetInstance()->SetStorageTrigger(sMdbName, sMdbTabName, StorageTrigger);
}

MDBSTO_API void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FOCP_NAME::FStorageTrigger CacheTrigger)
{
	FOCP_NAME::CMdbStorager::GetInstance()->SetCacheTrigger(sMdbName, sMdbTabName, CacheTrigger);
}

MDBSTO_API void CleanupMdbStorage()
{
	FOCP_NAME::CMdbStorager::GetInstance()->Cleanup();
}

MDBSTO_API bool StartMdbStorage()
{
	return FOCP_NAME::CMdbStorager::GetInstance()->Start();
}

MDBSTO_API void StopMdbStorage(bool bBlock)
{
	FOCP_NAME::CMdbStorager::GetInstance()->Stop(bBlock);
}

FOCP_C_END();

FOCP_BEGIN();

//////////////////////////////////////////////////////////////////
// COdbcConnect
//////////////////////////////////////////////////////////////////
COdbcConnect::COdbcConnect(void* pOdbcEnv)
{
	m_pEnv = pOdbcEnv;
	m_pDbc = NULL;
	m_bOpened = false;
	m_bConnected = false;
	m_bBreak = true;
}

COdbcConnect::~COdbcConnect()
{
	Cleanup();
}

void COdbcConnect::OdbcError(void* hStmt, bool bWarning)
{
	int32 rec;
	SQLCHAR     szError[501];
	SQLCHAR     szSqlState[10];
	SQLINTEGER  nNativeError;
	SQLSMALLINT nErrorMsg;

	CFormatString oLog;

	if(hStmt)
	{
		rec = 0;
		while ( SQLGetDiagRec( SQL_HANDLE_STMT, hStmt, ++rec, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			if(rec == 1)
				oLog.Print("SQLERR:");
			oLog.Print(" [%s]%s", szSqlState, szError);
		}
	}
	if(m_pDbc)
	{
		rec = 0;
		while ( SQLGetDiagRec( SQL_HANDLE_DBC, m_pDbc, ++rec, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			if(rec == 1)
				oLog.Print("DBCERR:");
			oLog.Print(" [%s]%s", szSqlState, szError);
		}

		ulong nCode;
		SQLRETURN nRet = SQLGetConnectAttr(m_pDbc, SQL_ATTR_CONNECTION_DEAD, (SQLPOINTER)&nCode, SQL_IS_UINTEGER, NULL);
		if( ((nRet != SQL_SUCCESS) && (nRet != SQL_SUCCESS_WITH_INFO)) || (nCode==SQL_CD_TRUE))
			m_bBreak = true;
	}

	if(!oLog.Empty())
		FocpLog(bWarning?FOCP_LOG_WARNING:FOCP_LOG_ERROR,("%s", oLog.GetStr()));
}

bool COdbcConnect::Initialize(const char* pszDSN, const char* pszUName, const char* pszUPasswd)
{
	bool bWarning;
	SQLRETURN nRet;

	if(!m_pEnv)
		return false;

	if(!m_bOpened)
	{
		nRet = SQLAllocHandle(SQL_HANDLE_DBC, m_pEnv, &m_pDbc);
		if(nRet != SQL_SUCCESS)
		{
			bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
			OdbcError(NULL, bWarning);
			if(!bWarning)
				return false;
		}
		m_oDsn = pszDSN;
		m_oUser = pszUName;
		m_oPasswd = pszUPasswd;
		m_bOpened = true;
	}
	return true;
}

bool COdbcConnect::Connect()
{
	bool bWarning;
	if(!m_bOpened)
		return false;

	if(m_bConnected)
	{
		if(m_bBreak)
			Disconnect();
		else
			return true;
	}

	SQLRETURN nRet = SQLConnect(m_pDbc, (SQLCHAR*)m_oDsn.GetStr(), SQL_NTS,
								(SQLCHAR*)m_oUser.GetStr(), SQL_NTS, (SQLCHAR*)m_oPasswd.GetStr(), SQL_NTS);

	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			SQLDisconnect(m_pDbc);
			return false;
		}
	}
	m_bConnected = true;
	m_bBreak = false;
	return true;
}

void COdbcConnect::Disconnect()
{
	if(m_bConnected)
	{
		SQLDisconnect(m_pDbc);
		m_bConnected = false;
	}
}

void COdbcConnect::Cleanup()
{
	Disconnect();
	if(m_bOpened)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, m_pDbc);
		m_bOpened = false;
	}
}

bool COdbcConnect::IsBroken()
{
	if(!m_bOpened || !m_bConnected || m_bBreak)
		return true;
	return false;
}

int16 COdbcConnect::GetValueType(CMdbFieldDef* pFieldDef, long& nBufSize)
{
	int16 nRet = 0;
	switch(pFieldDef->nType)
	{
	case MDB_INT8_FIELD:
	case MDB_INT16_FIELD:
	case MDB_INT32_FIELD:
		nRet = SQL_C_SLONG;
		nBufSize = sizeof(long);
		break;
	case MDB_INT64_FIELD:
		nRet = SQL_C_SBIGINT;
		nBufSize = sizeof(int64);
		break;
	case MDB_UINT8_FIELD:
	case MDB_UINT16_FIELD:
	case MDB_UINT32_FIELD:
		nRet = SQL_C_ULONG;
		nBufSize = sizeof(ulong);
		break;
	case MDB_UINT64_FIELD:
		nRet = SQL_C_UBIGINT;
		nBufSize = sizeof(uint64);
		break;
	case MDB_FLOAT_FIELD:
		nRet = SQL_C_FLOAT;
		nBufSize = sizeof(float);
		break;
	case  MDB_DOUBLE_FIELD:
		nRet = SQL_C_DOUBLE;
		nBufSize = sizeof(double);
		break;
	case MDB_DATE_FIELD:
		nRet = SQL_C_TYPE_DATE;
		nBufSize = sizeof(SQL_DATE_STRUCT);
		break;
	case MDB_TIME_FIELD:
		nRet = SQL_C_TYPE_TIME;
		nBufSize = sizeof(SQL_TIME_STRUCT);
		break;
	case MDB_DATETIME_FIELD:
		nRet = SQL_C_TYPE_TIMESTAMP;
		nBufSize = sizeof(SQL_TIMESTAMP_STRUCT);
		break;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
		nRet = SQL_C_CHAR;
		nBufSize = pFieldDef->nLen;
		break;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		nRet = SQL_C_CHAR;
		nBufSize = pFieldDef->nLen + 1;
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		nRet = SQL_C_BINARY;
		nBufSize = pFieldDef->nLen;
		break;
	}
	return nRet;
}

bool COdbcConnect::InitializeTable(const char* sDbName, CMdbTableDef* pTabDef)
{
	SQLCHAR sFieldName[MDB_NAME_MAXLEN+1];
	SQLSMALLINT nDataType, nDecimal;
	SQLINTEGER nColumnSize;
//	SQLINTEGER cbDataType, cbDecimal, cbColumnSize, cbColumnName;
	SQLLEN cbDataType, cbDecimal, cbColumnSize, cbColumnName;

	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	uint32 i, nFieldCount = pTabDef->nFieldCount;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];

	if(!m_bConnected)
	{
		FocpLog(FOCP_LOG_ERROR, ("Map the mdb table field(%s.%s) to %s.%s, not connected",
								 sDbName, pTabDef->sTableName, m_oDsn.GetStr(), pTabStoAttr->oTableName.GetStr()));
		return false;
	}

	bool bWarning;
	SQLHSTMT hSentence;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDbc, &hSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(NULL, bWarning);
		if(!bWarning)
			return false;
	}

	nRet = SQLColumns(hSentence, NULL, 0, NULL, 0, (SQLCHAR*)pTabStoAttr->oTableName.GetStr(), SQL_NTS, NULL, 0);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(hSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
			return false;
		}
	}

	nRet = SQLBindCol(hSentence, 4, SQL_C_CHAR, sFieldName, MDB_NAME_MAXLEN+1, &cbColumnName);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(hSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
			return false;
		}
	}
	nRet = SQLBindCol(hSentence, 5, SQL_C_SSHORT, &nDataType, 0, &cbDataType);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(hSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
			return false;
		}
	}
	nRet = SQLBindCol(hSentence, 7, SQL_C_SLONG, &nColumnSize, 0, &cbColumnSize);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(hSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
			return false;
		}
	}
	nRet = SQLBindCol(hSentence, 9, SQL_C_SSHORT, &nDecimal, 0, &cbDecimal);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(hSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
			return false;
		}
	}

	while(true)
	{
		nColumnSize = 0;
		nDecimal = 0;
		nRet = SQLFetch(hSentence);
		if(nRet != SQL_SUCCESS)
		{
			bWarning = (nRet==SQL_SUCCESS_WITH_INFO || nRet==SQL_NO_DATA);
			if(nRet != SQL_NO_DATA)
				OdbcError(hSentence, bWarning);
			if(!bWarning)
			{
				if(IsBroken())
				{
					SQLFreeHandle(SQL_HANDLE_STMT, hSentence);
					return false;
				}
				break;
			}
			if(nRet == SQL_NO_DATA)
				break;
		}
		for(i=0; i<nFieldCount; ++i)
		{
			CMdbFieldDef& oFieldDef = pTabDef->pFieldDefines[i];
			CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
			if(pFieldAttr && !CString::StringCompare(pFieldAttr->oFieldName.GetStr(), (const char*)sFieldName, false))
			{
				pFieldAttr->nValueType = GetValueType(&oFieldDef, pFieldAttr->nBufSize);
				pFieldAttr->nParaType = nDataType;
				pFieldAttr->nColumnSize = nColumnSize;
				pFieldAttr->nDecimalDigits = nDecimal;
				break;
			}
		}
	}
	SQLFreeHandle(SQL_HANDLE_STMT, hSentence);

	bool bMapError = false;
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbFieldDef& oFieldDef = pTabDef->pFieldDefines[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr && !pFieldAttr->nBufSize)
		{
			FocpLog(FOCP_LOG_ERROR, ("Map the mdb table field(%s.%s.%s) to %s.%s.%s failure",
									 sDbName, pTabDef->sTableName, oFieldDef.sFieldName, m_oDsn.GetStr(), pTabStoAttr->oTableName.GetStr(),
									 pFieldAttr->oFieldName.GetStr()));
			bMapError = true;
		}
	}

	if(bMapError)
		return false;

	COdbcInsert oInsert(this, pTabDef);
	if(!oInsert.PrepareSql() || !oInsert.BindPara(NULL))
		return false;

	return true;
}

bool COdbcConnect::BindSetField(void* pSentence, CMdbStorageFieldAttr* pFieldAttr, uint32 nCol, CMdbField* pField)
{
	SQLRETURN nRet;
	if(pField)
	{
		if(pField->IsNull())
		{
			pFieldAttr->nLen = SQL_NULL_DATA;
			nRet = SQLBindParameter(pSentence, nCol, SQL_PARAM_INPUT,
									pFieldAttr->nValueType, pFieldAttr->nParaType,
									pFieldAttr->nColumnSize, pFieldAttr->nDecimalDigits,
									NULL, 0, &pFieldAttr->nLen);
		}
		else
		{
			uint32 nSize;
			void* pPtr = pField->GetData(nSize);
			pFieldAttr->nLen = nSize;
			uint32 nType = pField->GetType();
			switch(nType)
			{
			case MDB_INT8_FIELD:
			case MDB_INT16_FIELD:
			case MDB_INT32_FIELD:
			case MDB_INT64_FIELD:
			case MDB_UINT8_FIELD:
			case MDB_UINT16_FIELD:
			case MDB_UINT32_FIELD:
			case MDB_UINT64_FIELD:
			case MDB_FLOAT_FIELD:
			case MDB_DOUBLE_FIELD:
				pFieldAttr->nLen = 0;
				nSize = 0;
				break;
			case MDB_DATE_FIELD:
			{
				uint16 Y,M,D;
				pFieldAttr->nLen = 0;
				nSize = 0;
				CDate oDate(*(int32*)pPtr);
				oDate.GetDate(Y,M,D);
				pFieldAttr->oDate.year = Y;
				pFieldAttr->oDate.month = M;
				pFieldAttr->oDate.day = D;
				pPtr = &pFieldAttr->oDate;
			}
			break;
			case MDB_TIME_FIELD:
			{
				uint16 h,m,s,ms;
				pFieldAttr->nLen = 0;
				nSize = 0;
				CTime oTime(*(int32*)pPtr);
				oTime.GetTime(h,m,s,ms);
				pFieldAttr->oTime.hour = h;
				pFieldAttr->oTime.minute = m;
				pFieldAttr->oTime.second = s;
				pPtr = &pFieldAttr->oTime;
			}
			break;
			case MDB_DATETIME_FIELD:
			{
				uint16 Y,M,D,h,m,s,ms;
				pFieldAttr->nLen = 0;//sizeof(SQL_TIMESTAMP_STRUCT)
				nSize = 0;//sizeof(SQL_TIMESTAMP_STRUCT)
				CDateTime oDateTime(*(double*)pPtr);
				oDateTime.GetDateTime(Y,M,D,h,m,s,ms);
				pFieldAttr->oDateTime.year = Y;
				pFieldAttr->oDateTime.month = M;
				pFieldAttr->oDateTime.day = D;
				pFieldAttr->oDateTime.hour = h;
				pFieldAttr->oDateTime.minute = m;
				pFieldAttr->oDateTime.second = s;
				pFieldAttr->oDateTime.fraction = ms;
				pPtr = &pFieldAttr->oDateTime;
			}
			break;
			case MDB_CHAR_FIELD:
			case MDB_LCHAR_FIELD:
				pFieldAttr->nLen = nSize;
				nSize = pFieldAttr->nBufSize;
				if(pFieldAttr->nLen < (SQLINTEGER)nSize)
					pFieldAttr->nLen = SQL_NTS;
				break;
			case MDB_RAW_FIELD:
				pFieldAttr->nLen = nSize;
				nSize = pFieldAttr->nBufSize;
				break;
			case MDB_VARCHAR_FIELD:
			case MDB_VARLCHAR_FIELD:
				pFieldAttr->nLen = SQL_NTS;
				break;
			case MDB_VARRAW_FIELD:
				break;
			}
			nRet = SQLBindParameter(pSentence, nCol, SQL_PARAM_INPUT,
									pFieldAttr->nValueType, pFieldAttr->nParaType,
									pFieldAttr->nColumnSize, pFieldAttr->nDecimalDigits,
									pPtr, (SQLINTEGER)nSize, &pFieldAttr->nLen);
		}
	}
	else
	{
		SQLINTEGER nSize = pFieldAttr->nBufSize;
		pFieldAttr->nLen = nSize;
		uint8* pPtr = new uint8[nSize];
		CBinary::MemorySet(pPtr, '1', nSize);
		nRet = SQLBindParameter(pSentence, nCol, SQL_PARAM_INPUT,
								pFieldAttr->nValueType, pFieldAttr->nParaType,
								pFieldAttr->nColumnSize, pFieldAttr->nDecimalDigits,
								pPtr, (SQLINTEGER)nSize, &pFieldAttr->nLen);
		delete[] pPtr;
	}
	if(nRet != SQL_SUCCESS)
	{
		bool bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(pSentence, bWarning);
		if(!bWarning)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////
// COdbcTruncate
//////////////////////////////////////////////////////////////////
COdbcTruncate::COdbcTruncate(COdbcConnect* pDb, CMdbTableDef* pTabDef)
{
	m_pDb = pDb;
	m_pTabDef = pTabDef;
	m_pSentence = NULL;
}

COdbcTruncate::~COdbcTruncate()
{
	if(m_pSentence)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
		m_pSentence = NULL;
	}
}

bool COdbcTruncate::PrepareSql()
{
	CString oSentence;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	if(pTabStoAttr->oLoadWhere.Empty())
	{
		oSentence = "TRUNCATE TABLE ";
		oSentence += pTabStoAttr->oTableName.GetStr();
	}
	else
	{
		oSentence = "DELETE FROM ";
		oSentence += pTabStoAttr->oTableName.GetStr();
		oSentence += " WHERE ";
		oSentence += pTabStoAttr->oLoadWhere;
	}
	bool bWarning;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDb->m_pDbc, &m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			m_pSentence = NULL;
			return false;
		}
	}
	nRet = SQLPrepare(m_pSentence, (SQLCHAR*)oSentence.GetStr(), SQL_NTS);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
			m_pSentence = NULL;
			return false;
		}
	}
	return true;
}

void COdbcTruncate::Exec()
{
	if(PrepareSql())
	{
		SQLRETURN nRet = SQLExecute(m_pSentence);
		if(nRet != SQL_SUCCESS)
			m_pDb->OdbcError(m_pSentence, (nRet==SQL_SUCCESS_WITH_INFO));
	}
}

//////////////////////////////////////////////////////////////////
// COdbcInsert
//////////////////////////////////////////////////////////////////
COdbcInsert::COdbcInsert(COdbcConnect* pDb, CMdbTableDef* pTabDef)
{
	m_pDb = pDb;
	m_pTabDef = pTabDef;
	m_pSentence = NULL;
}

COdbcInsert::~COdbcInsert()
{
	if(m_pSentence)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
		m_pSentence = NULL;
	}
}

bool COdbcInsert::PrepareSql()
{
	CString oSentence;

	uint32 nCol = 0;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	oSentence = "INSERT INTO ";
	oSentence += pTabStoAttr->oTableName.GetStr();
	oSentence += "(";
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr)
		{
			if(nCol)
				oSentence += ",";
			oSentence += pFieldAttr->oFieldName.GetStr();
			++nCol;
		}
	}
	oSentence += ") VALUES(";
	for(i=0; i<nCol; ++i)
	{
		if(i)
			oSentence += ",";
		oSentence += "?";
	}
	oSentence += ")";

	bool bWarning;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDb->m_pDbc, &m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			m_pSentence = NULL;
			return false;
		}
	}
	nRet = SQLPrepare(m_pSentence, (SQLCHAR*)oSentence.GetStr(), SQL_NTS);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
			m_pSentence = NULL;
			return false;
		}
	}
	return true;
}

bool COdbcInsert::BindPara(CMdbRecord* pRecord)
{
	uint32 nCol = 1;//ODBC
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbFieldDef &oFieldDef = m_pTabDef->pFieldDefines[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr)
		{
			CMdbField *pField = NULL;
			if(pRecord)
				pField = pRecord->GetField(i);
			if(!m_pDb->BindSetField(m_pSentence, pFieldAttr, nCol, pField))
				return false;
			++nCol;
		}
	}
	return true;
}

void COdbcInsert::Exec(CMdbRecord* pRecord)
{
	if(PrepareSql() && BindPara(pRecord))
	{
		SQLRETURN nRet = SQLExecute(m_pSentence);
		if(nRet != SQL_SUCCESS)
			m_pDb->OdbcError(m_pSentence, (nRet==SQL_SUCCESS_WITH_INFO));
	}
}

//////////////////////////////////////////////////////////////////
// COdbcDelete
//////////////////////////////////////////////////////////////////
COdbcDelete::COdbcDelete(COdbcConnect* pDb, CMdbTableDef* pTabDef)
{
	m_pDb = pDb;
	m_pTabDef = pTabDef;
	m_pSentence = NULL;
}

COdbcDelete::~COdbcDelete()
{
	if(m_pSentence)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
		m_pSentence = NULL;
	}
}

bool COdbcDelete::PrepareSql(CMdbRecord* pRecord)
{
	//DELETE FROM tablename WHERE f=? {and f=?}
	CString oSentence;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	CMdbIndexAttr* pIdxAttr = pTabStoAttr->pStorageIdx->pExtendAttr;
	uint32 i, nFieldCount = pIdxAttr->nFieldCount;
	oSentence = "DELETE FROM ";
	oSentence += pTabStoAttr->oTableName.GetStr();
	oSentence += " WHERE ";
	if(!pTabStoAttr->oLoadWhere.Empty())
		oSentence += "(";
	for(i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxAttr->pFields[i];
		CMdbFieldDef &oFieldDef = m_pTabDef->pFieldDefines[nFieldNo];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(i)
			oSentence += " AND ";
		if(pRecord->IsNull(nFieldNo))
		{
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += " IS NULL";
		}
		else if(oFieldDef.nType == MDB_LCHAR_FIELD || oFieldDef.nType == MDB_VARLCHAR_FIELD)
		{
			oSentence += "LOWER(";
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += ")=LOWER(?)";
		}
		else
		{
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += "=?";
		}
	}
	if(!pTabStoAttr->oLoadWhere.Empty())
	{
		oSentence += ") AND (";
		oSentence += pTabStoAttr->oLoadWhere;
		oSentence += ")";
	}

	bool bWarning;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDb->m_pDbc, &m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			m_pSentence = NULL;
			return false;
		}
	}
	nRet = SQLPrepare(m_pSentence, (SQLCHAR*)oSentence.GetStr(), SQL_NTS);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
			m_pSentence = NULL;
			return false;
		}
	}
	return true;
}

bool COdbcDelete::BindPara(CMdbRecord* pRecord)
{
	uint32 nCol = 1;//ODBC
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	CMdbIndexAttr* pIdxAttr = pTabStoAttr->pStorageIdx->pExtendAttr;
	uint32 i, nFieldCount = pIdxAttr->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxAttr->pFields[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[nFieldNo].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		CMdbField *pField = pRecord->GetField(nFieldNo);
		if(!pField->IsNull())
		{
			if(!m_pDb->BindSetField(m_pSentence, pFieldAttr, nCol, pField))
				return false;
			++nCol;
		}
	}
	return true;
}

void COdbcDelete::Exec(CMdbRecord* pRecord)
{
	if(PrepareSql(pRecord) && BindPara(pRecord))
	{
		SQLRETURN nRet = SQLExecute(m_pSentence);
		if(nRet != SQL_SUCCESS)
			m_pDb->OdbcError(m_pSentence, (nRet==SQL_SUCCESS_WITH_INFO));
	}
}

//////////////////////////////////////////////////////////////////
// COdbcUpdate
//////////////////////////////////////////////////////////////////
COdbcUpdate::COdbcUpdate(COdbcConnect* pDb, CMdbTableDef* pTabDef)
{
	m_pDb = pDb;
	m_pTabDef = pTabDef;
	m_pSentence = NULL;
}

COdbcUpdate::~COdbcUpdate()
{
	if(m_pSentence)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
		m_pSentence = NULL;
	}
}

bool COdbcUpdate::PrepareSql(CMdbRecord* pRecord)
{
	//UPDATE tablename SET f=v {, f=v} WHERE c=? {and c=?}
	uint32 nCol = 0;
	CString oSentence;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	oSentence = "UPDATE ";
	oSentence += pTabStoAttr->oTableName.GetStr();
	oSentence += " SET ";
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr && !pFieldAttr->bIdxField)
		{
			if(nCol)
				oSentence += ",";
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += "=?";
			++nCol;
		}
	}
	oSentence += " WHERE ";
	if(!pTabStoAttr->oLoadWhere.Empty())
		oSentence += "(";
	CMdbIndexAttr* pIdxAttr = pTabStoAttr->pStorageIdx->pExtendAttr;
	nFieldCount = pIdxAttr->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxAttr->pFields[i];
		CMdbFieldDef &oFieldDef = m_pTabDef->pFieldDefines[nFieldNo];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(i)
			oSentence += " AND ";
		if(pRecord->IsNull(nFieldNo))
		{
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += " IS NULL";
		}
		else if(oFieldDef.nType == MDB_LCHAR_FIELD || oFieldDef.nType == MDB_VARLCHAR_FIELD)
		{
			oSentence += "LOWER(";
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += ")=LOWER(?)";
		}
		else
		{
			oSentence += pFieldAttr->oFieldName.GetStr();
			oSentence += "=?";
		}
	}
	if(!pTabStoAttr->oLoadWhere.Empty())
	{
		oSentence += ") AND (";
		oSentence += pTabStoAttr->oLoadWhere;
		oSentence += ")";
	}

	bool bWarning;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDb->m_pDbc, &m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			m_pSentence = NULL;
			return false;
		}
	}
	nRet = SQLPrepare(m_pSentence, (SQLCHAR*)oSentence.GetStr(), SQL_NTS);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
			m_pSentence = NULL;
			return false;
		}
	}
	return true;
}

bool COdbcUpdate::BindPara(CMdbRecord* pRecord)
{
	uint32 nCol = 1;//ODBC
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbFieldDef &oFieldDef = m_pTabDef->pFieldDefines[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr && !pFieldAttr->bIdxField)
		{
			CMdbField *pField = pRecord->GetField(i);
			if(!m_pDb->BindSetField(m_pSentence, pFieldAttr, nCol, pField))
				return false;
			++nCol;
		}
	}
	CMdbIndexAttr* pIdxAttr = pTabStoAttr->pStorageIdx->pExtendAttr;
	nFieldCount = pIdxAttr->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pIdxAttr->pFields[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[nFieldNo].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		CMdbField *pField = pRecord->GetField(nFieldNo);
		if(!pField->IsNull())
		{
			if(!m_pDb->BindSetField(m_pSentence, pFieldAttr, nCol, pField))
				return false;
			++nCol;
		}
	}
	return true;
}

void COdbcUpdate::Exec(CMdbRecord* pRecord)
{
	if(PrepareSql(pRecord) && BindPara(pRecord))
	{
		SQLRETURN nRet = SQLExecute(m_pSentence);
		if(nRet != SQL_SUCCESS)
			m_pDb->OdbcError(m_pSentence, (nRet==SQL_SUCCESS_WITH_INFO));
	}
}

//////////////////////////////////////////////////////////////////
// COdbcSelect
//////////////////////////////////////////////////////////////////
COdbcSelect::COdbcSelect(COdbcConnect* pDb, CMdbTableDef* pTabDef, CMdb* pMdb, uint32 &nCount)
	:m_nCount(nCount)
{
	m_pDb = pDb;
	m_pTabDef = pTabDef;
	m_pSentence = NULL;
	m_bError = false;
	m_pFields = NULL;
	m_pOffset = NULL;
	m_pBufSize = NULL;
	m_nRecordSize = 0;
	m_pBuf = NULL;
	m_pAccess = (CMdbTableAccess*)pMdb->QueryAccess(m_pTabDef->sTableName);
	m_sDbName = pMdb->GetDbName();
}

COdbcSelect::~COdbcSelect()
{
	if(m_pFields)
	{
		delete[] m_pFields;
		m_pFields = NULL;
	}
	if(m_pOffset)
	{
		delete[] m_pOffset;
		m_pOffset = NULL;
	}
	if(m_pBufSize)
	{
		delete[] m_pBufSize;
		m_pBufSize = NULL;
	}
	if(m_pBuf)
	{
		delete[] m_pBuf;
		m_pBuf = NULL;
	}
	if(m_pAccess)
	{
		m_pAccess->Release();
		m_pAccess = NULL;
	}
	if(m_pSentence)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
		m_pSentence = NULL;
	}
}

bool COdbcSelect::HaveError()
{
	return m_bError;
}

bool COdbcSelect::PrepareSql()
{
	//SELECT f{,f} FROM tablename [WHERE cond]
	uint32 nCol = 0;
	CString oSentence;
	CMdbTableAttr* pTabAttr = m_pTabDef->pExtendAttr;
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	oSentence = "SELECT ";
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr)
		{
			if(nCol)
				oSentence += ",";
			oSentence += pFieldAttr->oFieldName.GetStr();
			++nCol;
		}
	}
	oSentence += " FROM ";
	oSentence += pTabStoAttr->oTableName.GetStr();
	if(!pTabStoAttr->oLoadWhere.Empty())
	{
		oSentence += " WHERE ";
		oSentence += pTabStoAttr->oLoadWhere.GetStr();
	}
	m_pFields = new uint32[nCol];
	m_pOffset = new uint32[nCol];
	m_pBufSize = new uint32[nCol];
	m_pBuf = NULL;
	m_nRecordSize = 0;
	m_nFieldCount = nCol;
	nCol = 0;
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)m_pTabDef->pFieldDefines[i].pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr)
		{
			m_pFields[nCol] = i;
			m_pOffset[nCol] = ComputeBuffer(m_pBufSize[nCol], i);
			m_nRecordSize = m_pOffset[nCol] + m_pBufSize[nCol];
			++nCol;
		}
	}
	uint32 nMod = m_nRecordSize % sizeof(double);
	if(nMod)//确保每一行都能对齐各种类型的数据
		m_nRecordSize += sizeof(double) - nMod;
	m_pBuf = new uint8[MDB_FASTFETCH_SIZE*m_nRecordSize];

	bool bWarning;
	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_STMT, m_pDb->m_pDbc, &m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(NULL, bWarning);
		if(!bWarning)
		{
			m_pSentence = NULL;
			return false;
		}
	}
	nRet = SQLPrepare(m_pSentence, (SQLCHAR*)oSentence.GetStr(), SQL_NTS);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, m_pSentence);
			m_pSentence = NULL;
			return false;
		}
	}
	return true;
}

#define FOCP_MAX(a,b) (((a)>(b))?(a):(b))
uint32 COdbcSelect::GetItemSize(uint32 nFieldNo, uint32 &nAlignSize)
{
	uint32 nItemSize;
	CMdbFieldDef& oFieldDef = m_pTabDef->pFieldDefines[nFieldNo];
	uint32 nType = oFieldDef.nType;
	switch(nType)
	{
	case MDB_INT8_FIELD:
	case MDB_UINT8_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint8));
		break;
	case MDB_INT16_FIELD:
	case MDB_UINT16_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint16));
		break;
	case MDB_INT32_FIELD:
	case MDB_UINT32_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint32));
		break;
	case MDB_INT64_FIELD:
	case MDB_UINT64_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint64));
		break;
	case MDB_FLOAT_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(float));
		break;
	case MDB_DOUBLE_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(double));
		break;
	case MDB_DATE_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(SQLUSMALLINT));
		break;
	case MDB_TIME_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(SQLUSMALLINT));
		break;
	case MDB_DATETIME_FIELD:
		nItemSize = sizeof(XItem);
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(SQLUINTEGER));
		break;
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
		nItemSize = sizeof(SQLINTEGER)+oFieldDef.nLen + 1;
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint8));
		break;
	case MDB_RAW_FIELD:
	case MDB_VARRAW_FIELD:
		nItemSize = sizeof(SQLINTEGER)+oFieldDef.nLen;
		nAlignSize = FOCP_MAX(sizeof(SQLINTEGER), sizeof(uint8));
		break;
	}
	return nItemSize;
}

uint32 COdbcSelect::ComputeBuffer(uint32& nBufSize, uint32 nFieldNo)
{
	uint32 nAlignSize, nOffset, nMod;
	nOffset = m_nRecordSize;
	nBufSize = GetItemSize(nFieldNo, nAlignSize);
	nMod = nOffset % nAlignSize;
	if(nMod)
		nOffset += nAlignSize - nMod;
	return nOffset;
}

bool COdbcSelect::BindPara()
{
	uint32 nCol = 1;//ODBC
	uint32 i, nFieldCount = m_pTabDef->nFieldCount;
	for(i=0; i<nFieldCount; ++i)
	{
		CMdbFieldDef &oFieldDef = m_pTabDef->pFieldDefines[i];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pFieldAttr)
		{
			if(!BindCol(pFieldAttr, nCol))
				return false;
			++nCol;
		}
	}
	return true;
}

bool COdbcSelect::BindCol(CMdbStorageFieldAttr* pFieldAttr, uint32 nBindCol)
{
	XItem* pItem = (XItem*)(m_pBuf + m_pOffset[nBindCol - 1]);
	pItem->nLen = m_pBufSize[nBindCol - 1];
	SQLRETURN nRet = SQLBindCol(m_pSentence, nBindCol, pFieldAttr->nValueType, &pItem->oValue, pItem->nLen, &pItem->nLen);
	if(nRet != SQL_SUCCESS)
	{
		bool bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
			return false;
	}
	return true;
}

void COdbcSelect::Exec()
{
	SQLRETURN nRet;
	SQLUINTEGER nCount;
	if(!PrepareSql())
	{
		m_bError = true;
		return;
	}
	bool bWarning;
	nRet = SQLSetStmtAttr(m_pSentence, SQL_ATTR_ROW_ARRAY_SIZE, (SQLPOINTER)MDB_FASTFETCH_SIZE, SQL_IS_INTEGER);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			m_bError = true;
			return;
		}
	}
	nRet = SQLSetStmtAttr(m_pSentence, SQL_ATTR_ROW_BIND_TYPE, (SQLPOINTER)m_nRecordSize, (SQLINTEGER)SQL_IS_INTEGER);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			m_bError = true;
			return;
		}
	}
	nRet = SQLSetStmtAttr(m_pSentence, SQL_ATTR_ROWS_FETCHED_PTR, (SQLPOINTER)&nCount, SQL_IS_INTEGER);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			m_bError = true;
			return;
		}
	}
	SQLUSMALLINT* pStatus = new SQLUSMALLINT[MDB_FASTFETCH_SIZE];
	nRet = SQLSetStmtAttr(m_pSentence, SQL_ATTR_ROW_STATUS_PTR, (SQLPOINTER)pStatus, SQL_IS_INTEGER);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			m_bError = true;
			delete[] pStatus;
			return;
		}
	}
	nRet = SQLExecute(m_pSentence);
	if(nRet != SQL_SUCCESS)
	{
		bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		m_pDb->OdbcError(m_pSentence, bWarning);
		if(!bWarning)
		{
			m_bError = true;
			delete[] pStatus;
			return;
		}
	}
	if(!BindPara())
	{
		m_bError = true;
		delete[] pStatus;
		return;
	}
	while(true)
	{
		//nRet = SQLExtendedFetch(m_pSentence, SQL_FETCH_NEXT, 0, &nCount, pStatus);
		nRet = SQLFetch(m_pSentence);
		if(nRet != SQL_SUCCESS)
		{
			bWarning = (nRet==SQL_SUCCESS_WITH_INFO || nRet==SQL_NO_DATA);
			if(nRet != SQL_NO_DATA)
				m_pDb->OdbcError(m_pSentence, bWarning);
			if(!bWarning)
			{
				m_bError = true;
				delete[] pStatus;
				return;
			}
			if(nRet == SQL_NO_DATA)
				break;
		}
		uint8* pShift = m_pBuf;
		for(SQLUINTEGER k=0; k<nCount; ++k, pShift+=m_nRecordSize)
		{
			if(pStatus[k] == SQL_ROW_DELETED || pStatus[k] == SQL_ROW_ERROR)
				continue;
			CMdbPara* pPara = m_pAccess->GetInsertPara();
			for(uint32 i=0; i<m_nFieldCount; ++i)
			{
				int32 nDate, nTime;
				double nDateTime;
				XItem* pItem = (XItem*)(pShift + m_pOffset[i]);
				if(pItem->nLen == SQL_NULL_DATA)
					continue;
				CMdbValue & oVal = pItem->oValue;
				uint32 nCol = m_pFields[i];
				uint32 nType = pPara->GetType(nCol);
				switch(nType)
				{
				case MDB_INT8_FIELD:
					pPara->SetInt8(nCol, oVal.i8);
					break;
				case MDB_INT16_FIELD:
					pPara->SetInt16(nCol, oVal.i16);
					break;
				case MDB_INT32_FIELD:
					pPara->SetInt32(nCol, oVal.i32);
					break;
				case MDB_INT64_FIELD:
					pPara->SetInt64(nCol, oVal.i64);
					break;
				case MDB_UINT8_FIELD:
					pPara->SetUInt8(nCol, oVal.u8);
					break;
				case MDB_UINT16_FIELD:
					pPara->SetUInt16(nCol, oVal.u16);
					break;
				case MDB_UINT32_FIELD:
					pPara->SetUInt32(nCol, oVal.u32);
					break;
				case MDB_UINT64_FIELD:
					pPara->SetUInt64(nCol, oVal.u64);
					break;
				case MDB_FLOAT_FIELD:
					pPara->SetFloat(nCol, oVal.f32);
					break;
				case MDB_DOUBLE_FIELD:
					pPara->SetDouble(nCol, oVal.f64);
					break;
				case MDB_DATE_FIELD:
					if(EncodeDate(nDate, oVal.oDate.year, oVal.oDate.month, oVal.oDate.day))
						pPara->SetDate(nCol, nDate);
					break;
				case MDB_TIME_FIELD:
					if(EncodeTime(nTime, oVal.oTime.hour, oVal.oTime.minute, oVal.oTime.second, 0))
						pPara->SetDate(nCol, nTime);
					break;
				case MDB_DATETIME_FIELD:
					if(EncodeDate(nDate, oVal.oDateTime.year, oVal.oDateTime.month, oVal.oDateTime.day) &&
							EncodeTime(nTime, oVal.oDateTime.hour, oVal.oDateTime.minute, oVal.oDateTime.second, (uint16)oVal.oDateTime.fraction))
					{
						EncodeDateTime(nDateTime, nDate, nTime);
						pPara->SetDateTime(nCol, nDateTime);
					}
					break;
				case MDB_CHAR_FIELD:
				case MDB_LCHAR_FIELD:
				case MDB_VARCHAR_FIELD:
				case MDB_VARLCHAR_FIELD:
					oVal.s[m_pBufSize[i]-1] = 0;//确保数据以0结尾
					pPara->SetString(nCol, (const char*)oVal.s);
					break;
				case MDB_RAW_FIELD:
				case MDB_VARRAW_FIELD:
					pPara->SetRaw(nCol, oVal.s, pItem->nLen);
					break;
				}
			}
			m_pAccess->Insert(MDB_LOAD_CALLER);
			m_nCount++;
			if(!(m_nCount%5000))
				FocpInfo(("CMdbStorager::Load(%s): %u", m_sDbName, m_nCount));
		}
		if(nCount < MDB_FASTFETCH_SIZE)
			break;
	}
	delete[] pStatus;
}

//////////////////////////////////////////////////////////////////
// CMdbOdbc
//////////////////////////////////////////////////////////////////
CMdbOdbc::CMdbOdbc(void* pOdbcEnv):m_oLink(pOdbcEnv), m_oWorkerThread(this)
{
	m_pMdb = NULL;
	m_pEventDb = NULL;
	m_pEventAccess = NULL;
	m_bWork = false;
	m_oWorkEvent.Reset();
	m_bSupportStorage = false;
}

CMdbOdbc::~CMdbOdbc()
{
	Cleanup();
}

COdbcConnect* CMdbOdbc::GetLink()
{
	return &m_oLink;
}

bool CMdbOdbc::Initialize(const char* sDSN, const char* sUser, const char* sPasswd, bool bSupportStorage,
						  const char* sMdbName, const char* sEventDb, const char* sEventTable)
{
	uint32 i;
	CString oDefaultEventTable;

	m_bSupportStorage = bSupportStorage;

	if(!sEventDb || !sEventDb[0])
		sEventDb = MDB_SYSDB_NAME;
	if(!sEventTable || !sEventTable[0])
	{
		oDefaultEventTable = sMdbName;
		oDefaultEventTable += "StoreEvent";
		sEventTable = oDefaultEventTable.GetStr();
	}

	m_pMdb = CMdb::GetMdb(sMdbName);
	if(!m_pMdb)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open database '%s' failure", sMdbName));
		return false;
	}

	m_pEventDb = CMdb::GetMdb(sEventDb);
	if(!m_pEventDb)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open database '%s' failure", sEventDb));
		return false;
	}

	m_pEventAccess = m_pEventDb->QueryAccess(sEventTable);
	if(!m_pEventAccess)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open table '%s.%s' failure", sEventDb, sEventTable));
		return false;
	}

	m_nFields[0] = m_pEventAccess->GetFieldNo("TableName");
	m_nFields[1] = m_pEventAccess->GetFieldNo("PrimaryKey");
	m_nFields[2] = m_pEventAccess->GetFieldNo("Action");
	m_nFields[3] = m_pEventAccess->GetFieldNo("EventTime");
	if(m_nFields[0] == (uint32)(-1) || m_nFields[1] == (uint32)(-1) ||
			m_nFields[2] == (uint32)(-1) || m_nFields[3] == (uint32)(-1))
	{
		FocpLog(FOCP_LOG_ERROR, ("The table '%s.%s' structure is invalid for CMdbOdbc", sEventDb, m_pEventAccess));
		return false;
	}
	CMdbTableDef* pTabDef = m_pEventAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbIndexDef* pIdxDef = NULL;
	for(i=0; i<pTabAttr->nIndexCount; ++i)
	{
		pIdxDef = pTabAttr->pIndexDefineSet[i];
		if(pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
		{
			CMdbIndexAttr* pIdxAttr = pIdxDef->pExtendAttr;
			if(pIdxAttr->nFieldCount == 2 && pIdxAttr->pFields[0] == m_nFields[0] && pIdxAttr->pFields[1] == m_nFields[1])
				break;
		}
	}
	if(i >= pTabAttr->nIndexCount)
	{
		FocpLog(FOCP_LOG_ERROR, ("The table '%s.%s' index structure is invalid for CMdbOdbc", sEventDb, m_pEventAccess));
		return false;
	}

	if(!m_oLink.Initialize(sDSN, sUser, sPasswd))
		return false;

	if(!m_oLink.Connect())
		return false;

	CString oTableList(m_pMdb->GetTableList());
	char* sTableName = (char*)oTableList.GetStr();
	while(sTableName)
	{
		char* pShift = (char*)CString::CharOfString(sTableName, ',');
		if(pShift)
			pShift[0] = 0;
		pTabDef = m_pMdb->GetTableDefine(sTableName);
		pTabAttr = pTabDef->pExtendAttr;
		CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pTabStoAttr && !m_oLink.InitializeTable(sMdbName, pTabDef))
			return false;
		sTableName = pShift;
		if(sTableName)
		{
			sTableName[0] = ',';
			++sTableName;
		}
	}

	m_oLink.Disconnect();
	return true;
}

void CMdbOdbc::Cleanup()
{
	m_oLink.Cleanup();
	if(m_pEventAccess)
	{
		m_pEventAccess->Release();
		m_pEventAccess = NULL;
	}
	m_pMdb = NULL;
	m_pEventDb = NULL;
}

bool CMdbOdbc::Load()
{
	uint32 nCount = 0;
	if(!m_oLink.Connect())
	{
		FocpInfo(("CMdbStorager::Load(%s): %u", m_pMdb->GetDbName(), nCount));
		return false;
	}
	CString oTableList(m_pMdb->GetTableList());
	char *pShift, *sTableName = (char*)oTableList.GetStr();
	while(sTableName)
	{
		pShift = (char*)CString::CharOfString(sTableName, ',');
		if(pShift)
			pShift[0] = 0;
		CMdbTableDef* pTabDef = m_pMdb->GetTableDefine(sTableName);
		CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
		CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pTabStoAttr)
		{
			COdbcSelect oSelect(&m_oLink, pTabDef, m_pMdb, nCount);
			oSelect.Exec();
			if(oSelect.HaveError())
			{
				FocpInfo(("CMdbStorager::Load(%s): %u", m_pMdb->GetDbName(), nCount));
				return false;
			}
		}
		sTableName = pShift;
		if(sTableName)
		{
			sTableName[0] = ',';
			++sTableName;
		}
	}
	m_oLink.Disconnect();
	FocpInfo(("CMdbStorager::Load(%s): %u", m_pMdb->GetDbName(), nCount));
	return true;
}

void CMdbOdbc::Start()
{
	m_bWork = false;
	m_oWorkEvent.Reset();
	if(m_bSupportStorage)
		m_oWorkerThread.Start();
}

void CMdbOdbc::Stop(bool bBlock)
{
	if(!m_bSupportStorage)
		return;
	if(m_bWork)
	{
		uint32 nCount;
		CMdbAccess* pAccess = GetEventAccess();
		pAccess->GetQueryPara();
		while(true)
		{
			pAccess->Count(nCount);
			if(nCount == 0)
				break;
			FocpInfo(("CMdbOdbc::Stop(%s) wait the data's storage task, there are %u events", m_pMdb->GetDbName(), nCount));
			CCooperator::Sleep(1000);
		}
		pAccess->Release();
	}
	m_oWorkerThread.Stop(false);
	m_oWorkEvent.Set();
	if(bBlock)
		m_oWorkerThread.Stop(true);
}

void CMdbOdbc::StartStorage()
{
	if(m_bSupportStorage)
	{
		FocpInfo(("Start storage process for the database '%s'.", m_pMdb->GetDbName()));
		m_bWork = true;
		m_oWorkEvent.Set();
	}
}

void CMdbOdbc::StopStorage()
{
	if(m_bSupportStorage)
	{
		FocpInfo(("End storage process for the database '%s'.", m_pMdb->GetDbName()));
		m_bWork = false;
		m_oWorkEvent.Reset();
	}
}

CMdbAccess* CMdbOdbc::GetEventAccess()
{
	return m_pEventDb->QueryAccess(m_pEventAccess->GetTableName());
}

void CMdbOdbc::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	m_oWorkEvent.Wait(1000);

	if(!bRunning || !m_bWork || !m_oLink.Connect())
		return;
	uint32 nOp;
	CString oTableName, oPrimaryKey;
	while(bRunning && m_bWork)
	{
		if(!GetEvent(oTableName, oPrimaryKey, nOp))
		{
			CCooperator::Sleep(300);
			continue;
		}
		ProcEvent(oTableName, oPrimaryKey, nOp);
		if(m_oLink.IsBroken())
			break;
		DelEvent(oTableName, oPrimaryKey);
	}
	m_oLink.Disconnect();
}

bool CMdbOdbc::GetEvent(CString& oTableName, CString& oPrimaryKey, uint32 &nOp)
{
	uint32 nLen;
	m_pEventAccess->GetQueryPara();//不设条件，仅做清空处理
	if(m_pEventAccess->Query(1, 0))//读取头端一条记录
		return false;
	CMdbResult* pResult = m_pEventAccess->GetResultSet()->GetResult(0);
	if(!pResult)
		return false;
	const char* sTableName = pResult->GetString(m_nFields[0], &nLen);
	CString x(sTableName, nLen);
	oTableName.Swap(x);
	const char* sPrimaryKey = pResult->GetString(m_nFields[1], &nLen);
	if(!sPrimaryKey)
		sPrimaryKey = "";
	CString y(sPrimaryKey, nLen);
	oPrimaryKey.Swap(y);
	nOp = pResult->GetUInt32(m_nFields[2]);
	return true;
}

void CMdbOdbc::DelEvent(const CString& oTableName, const CString& oPrimaryKey)
{
	CMdbPara* pPara = m_pEventAccess->GetQueryPara()->AddParaSet()->AddPara();
	pPara->SetString(m_nFields[0], oTableName.GetStr());
	pPara->SetString(m_nFields[1], oPrimaryKey.GetStr());
	m_pEventAccess->Delete();
}

void CMdbOdbc::ProcEvent(const CString& oTableName, const CString& oPrimaryKey, uint32 nOp)
{
	CMdbSqlParaSet* pCond;
	const char* sTableName = oTableName.GetStr();
	const char* sPrimaryKey = oPrimaryKey.GetStr();
	CMdbAccess* pAccess = m_pMdb->QueryAccess(sTableName);
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	switch(nOp)
	{
	case MDB_EVENT_TRUNCATE:
		COdbcTruncate(&m_oLink, pTabDef).Exec();
		break;
	case MDB_EVENT_DELETE:
		pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
		if(pCond->SetFromWhere(sPrimaryKey))
		{
			bool bParaSet;
			pCond = (CMdbSqlParaSet*)pCond->GetPara(0, bParaSet);
			CMdbSqlPara* pPara = (CMdbSqlPara*)pCond->GetPara(0, bParaSet);
			COdbcDelete(&m_oLink, pTabDef).Exec(&pPara->GetRecord());
		}
		break;
	case MDB_EVENT_INSERT:
	case MDB_EVENT_UPDATE:
		pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
		if(pCond->SetFromWhere(sPrimaryKey) && !pAccess->Query(1, 0))
		{
			CMdbRecord* pRecord = (CMdbRecord*)pAccess->GetResultSet()->GetResult(0);
			if(pRecord)
			{
				if(nOp == MDB_EVENT_INSERT)
					COdbcInsert(&m_oLink, pTabDef).Exec(pRecord);
				else
					COdbcUpdate(&m_oLink, pTabDef).Exec(pRecord);
				bool bNotCache = false;
				CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabDef->pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
				if(pTabStoAttr->CacheTrigger)
				{
					if(!pTabStoAttr->CacheTrigger(pAccess, pRecord))
						bNotCache = true;
				}
				else
				{
					CMdbSqlParaSet* pCache = (CMdbSqlParaSet*)pTabStoAttr->pCacheWhere;
					if(pCache && !pCache->IsCoincident(*pRecord))
						bNotCache = true;
				}
				if(bNotCache)
				{
					uint32 nDeletedCount = 0;
					CMdbPara* pPara = (CMdbSqlPara*)pAccess->GetQueryPara()->AddParaSet()->AddPara();
					CMdbIndexDef* pStorageIdx = pTabStoAttr->pStorageIdx;
					((CMdbSqlPara*)pPara)->SetFrom(pStorageIdx, pRecord);
					pAccess->Delete(&nDeletedCount, MDB_STO_CALLER);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// CMdbStorageToken
//////////////////////////////////////////////////////////////////
CMdbStorageToken::CMdbStorageToken(uint32 nDomain):CAcmToken(nDomain, ACM_MDB_STORAGE_TOKEN)
{
}

CMdbStorageToken::~CMdbStorageToken()
{
}

void CMdbStorageToken::OnRoleChange(bool bTakeUp)
{
	CMdbStorager* pStorager = CMdbStorager::GetInstance();
	if(bTakeUp)
		pStorager->StartStorage(m_nDomain);
	else
		pStorager->StopStorage(m_nDomain);
}

//////////////////////////////////////////////////////////////////
// CMdbStorager
//////////////////////////////////////////////////////////////////
static void OdbcError(void* pEnv, bool bWarning)
{
	int32 rec;
	SQLCHAR     szError[501];
	SQLCHAR     szSqlState[10];
	SQLINTEGER  nNativeError;
	SQLSMALLINT nErrorMsg;

	CFormatString oLog;

	if ( pEnv )
	{
		rec = 0;
		while ( SQLGetDiagRec( SQL_HANDLE_ENV, pEnv, ++rec, szSqlState, &nNativeError, szError, 500, &nErrorMsg ) == SQL_SUCCESS )
		{
			if(rec == 1)
				oLog.Print("ENVERR:");
			oLog.Print(" [%s]%s", szSqlState, szError);
		}
	}
	if(!oLog.Empty())
		FocpLog(bWarning?FOCP_LOG_WARNING:FOCP_LOG_ERROR,("%s", oLog.GetStr()));
}

CMdbStorager::CMdbStorager():CMdbExtPlugIn(MDB_EXTEND_PLUGIN_STORAGER)
{
	m_pOdbcEnv = NULL;

	SQLRETURN nRet = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_pOdbcEnv);
	if(nRet != SQL_SUCCESS && nRet != SQL_SUCCESS_WITH_INFO)
		return;

	nRet = SQLSetEnvAttr(m_pOdbcEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER);
	if(nRet != SQL_SUCCESS)
	{
		bool bWarning = (nRet==SQL_SUCCESS_WITH_INFO);
		OdbcError(m_pOdbcEnv, bWarning);
		if(!bWarning)
		{
			SQLFreeHandle(SQL_HANDLE_ENV, m_pOdbcEnv);
			m_pOdbcEnv = NULL;
			return;
		}
	}
}

CMdbStorager::~CMdbStorager()
{
	Cleanup();
}

CMdbStorager* CMdbStorager::GetInstance()
{
	return CSingleInstance<CMdbStorager>::GetInstance();
}

bool CMdbStorager::InitializeStorageAttr(const char* sMdbName, const char* sMdbTabName,
		const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere, 
		const char* sStorageIdx, const char* sFieldList)
{
	uint32 i;
	CMdb* pDb = CMdb::GetMdb(sMdbName);
	if(!pDb)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open database '%s' failure", sMdbName));
		return false;
	}
	CMdbLocalInterface* pItf = pDb->GetLocalInterface();
	if(!pItf)
	{
		FocpLog(FOCP_LOG_ERROR, ("Database '%s' not support the local interface", sMdbName));
		return false;
	}
	CMdbTableDef* pTabDef = pDb->GetTableDefine(sMdbTabName);
	if(!pTabDef)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open table '%s.%s' failure", sMdbName, sMdbTabName));
		return false;
	}
	if(!sDbTabName || !sDbTabName[0])
		sDbTabName = sMdbTabName;
	if(!sStorageIdx || !sStorageIdx[0])
	{
		FocpLog(FOCP_LOG_ERROR, ("Open table '%s.%s' mapping failure", sMdbName, sMdbTabName));
		return false;
	}
	CMdbIndexDef* pStorageIdx = NULL;
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	for(i=0; i<pTabAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIdxDef = pTabAttr->pIndexDefineSet[i];
		if(pIdxDef->nQualifier & MDB_UNIQUE_INDEX)
		{
			if(!CString::StringCompare(pIdxDef->sIndexName, sStorageIdx, false))
			{
				pStorageIdx = pIdxDef;
				break;
			}
		}
	}
	if(!pStorageIdx)
	{
		FocpError(("There aren't unique range index '%s' on %s.%s", sStorageIdx, sMdbName, sMdbTabName));
		return false;
	}
	CMdbSqlParaSet * pStorageWhere = NULL;
	if(sStorageWhere && sStorageWhere[0])
	{
		pStorageWhere = new CMdbSqlParaSet(pTabDef);
		if(!pStorageWhere->SetFromWhere(sStorageWhere))
		{
			delete pStorageWhere;
			FocpLog(FOCP_LOG_ERROR, ("Build table '%s.%s' StorageWhere failure", sMdbName, sMdbTabName));
			return false;
		}
		if(pStorageWhere->IsEmpty())
		{
			delete pStorageWhere;
			pStorageWhere = NULL;
		}
	}
	CMdbSqlParaSet * pCacheWhere = NULL;
	if(sCacheWhere && sCacheWhere[0])
	{
		pCacheWhere = new CMdbSqlParaSet(pTabDef);
		if(!pCacheWhere->SetFromWhere(sCacheWhere))
		{
			delete pCacheWhere;
			FocpLog(FOCP_LOG_ERROR, ("Build table '%s.%s' CacheWhere failure", sMdbName, sMdbTabName));
			return false;
		}
		if(pCacheWhere->IsEmpty())
		{
			delete pCacheWhere;
			pCacheWhere = NULL;
		}
	}
	CMdbStorageTableAttr* pTabStorAttr = new CMdbStorageTableAttr;
	pTabStorAttr->oTableName = sDbTabName;
	if(sLoadWhere && sLoadWhere[0])
		pTabStorAttr->oLoadWhere = sLoadWhere;
	pTabStorAttr->pStorageWhere = pStorageWhere;
	pTabStorAttr->pCacheWhere = pCacheWhere;
	pTabStorAttr->pStorageIdx = pStorageIdx;
	pTabStorAttr->StorageTrigger = NULL;
	pTabStorAttr->CacheTrigger = NULL;

	pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER] = pTabStorAttr;

	if(!sFieldList || !sFieldList[0])
	{
		for(i=0; i<pTabDef->nFieldCount; ++i)
		{
			CMdbFieldDef& oFieldDef = pTabDef->pFieldDefines[i];
			CMdbStorageFieldAttr* pFieldAttr = new CMdbStorageFieldAttr;
			pFieldAttr->oFieldName = oFieldDef.sFieldName;
			pFieldAttr->nBufSize = 0;
			pFieldAttr->bIdxField = false;
			oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER] = pFieldAttr;
		}
	}
	else
	{
		CString oFieldList(sFieldList);
		char* pShift, *sFieldName=(char*)oFieldList.GetStr(), *sDbFieldName;
		while(sFieldName)
		{
			pShift = (char*)CString::CharOfString(sFieldName, ',');
			if(pShift)
				pShift[0] = 0;
			sDbFieldName = (char*)CString::CharOfString(sFieldName, ':');
			if(sDbFieldName)
			{
				sDbFieldName[0] = 0;
				++sDbFieldName;
			}
			else
				sDbFieldName = sFieldName;
			if(!sDbFieldName[0])
				sDbFieldName = sFieldName;
			for(i=0; i<pTabDef->nFieldCount; ++i)
			{
				CMdbFieldDef& oFieldDef = pTabDef->pFieldDefines[i];
				if(!CString::StringCompare(sFieldName, oFieldDef.sFieldName, false))
				{
					CMdbStorageFieldAttr* pFieldAttr = new CMdbStorageFieldAttr;
					pFieldAttr->oFieldName = oFieldDef.sFieldName;
					pFieldAttr->nBufSize = 0;
					pFieldAttr->bIdxField = false;
					oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER] = pFieldAttr;
				}
			}
			sFieldName = pShift;
			if(sFieldName)
				sFieldName++;
		}
	}
	uint32 nIdxFieldCount = pStorageIdx->pExtendAttr->nFieldCount;
	uint32* pIdxFields = pStorageIdx->pExtendAttr->pFields;
	for(i=0; i<nIdxFieldCount; ++i)
	{
		CMdbFieldDef &oFieldDef = pTabDef->pFieldDefines[pIdxFields[i]];
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)oFieldDef.pExtendAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(!pFieldAttr)
		{
			FocpLog(FOCP_LOG_ERROR, ("'%s.%s.%s' StorageIndex field doesn't support storage", sMdbName, sMdbTabName, oFieldDef.sFieldName));
			return false;
		}
		if(oFieldDef.nType == MDB_RAW_FIELD || oFieldDef.nType == MDB_RAW_FIELD)
		{
			FocpLog(FOCP_LOG_ERROR, ("'%s.%s.%s' StorageIndex field cann't be binary type", sMdbName, sMdbTabName, oFieldDef.sFieldName));
			return false;
		}
		pFieldAttr->bIdxField = true;
	}

	return true;
}

bool CMdbStorager::InitializeDataSource(const char* sMdbName, uint32 nDomain, bool bSupportStorage,
										const char* sDSN, const char* sUser, const char* sPasswd,
										const char* sEventDb, const char* sEventTable)
{
	CMdb* pDb = CMdb::GetMdb(sMdbName);
	if(!pDb)
	{
		FocpLog(FOCP_LOG_ERROR, ("Open database '%s' failure", sMdbName));
		return false;
	}
	CMdbLocalInterface* pItf = pDb->GetLocalInterface();
	if(!pItf)
	{
		FocpLog(FOCP_LOG_ERROR, ("Database '%s' not support the local interface", sMdbName));
		return false;
	}
	CRbTreeNode* pIt = m_oLinks.Find(sMdbName);
	if(pIt != m_oLinks.End())
	{
		FocpLog(FOCP_LOG_ERROR, ("Repeate config the database '%s' datasource", sMdbName));
		return false;
	}
	CMdbOdbc* pOdbc = new CMdbOdbc(m_pOdbcEnv);
	if(!pOdbc->Initialize(sDSN, sUser, sPasswd, bSupportStorage, sMdbName, sEventDb, sEventTable))
	{
		delete pOdbc;
		return false;
	}
	m_oLinks[sMdbName] = pOdbc;
	pIt = m_oDomainMdbTable.Find(nDomain);
	if(pIt == m_oDomainMdbTable.End())
	{
		CMdbDomainItem &oItem = m_oDomainMdbTable[nDomain];
		if(bSupportStorage)
			oItem.pToken = new CMdbStorageToken(nDomain);
		else
			oItem.pToken = NULL;
		oItem.oMdbList.Push(sMdbName);
		oItem.bLoaded = false;
	}
	else
	{
		CMdbDomainItem &oItem = m_oDomainMdbTable.GetItem(pIt);
		if(bSupportStorage && !oItem.pToken)
			oItem.pToken = new CMdbStorageToken(nDomain);
		oItem.oMdbList.Push(sMdbName);
	}
	CString oTableList(pDb->GetTableList());
	char *pShift, *sTableName = (char*)oTableList.GetStr();
	while(sTableName)
	{
		pShift = (char*)CString::CharOfString(sTableName, ',');
		if(pShift)
			pShift[0] = 0;
		CMdbTableDef* pTabDef = pDb->GetTableDefine(sTableName);
		CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
		CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
		if(pTabStoAttr)
		{
			pItf->RegInsertAfterTrigger(sTableName, &CMdbStorager::OnInsertAfter, this);
			pItf->RegDeleteAfterTrigger(sTableName, &CMdbStorager::OnDeleteAfter, this);
			pItf->RegUpdateAfterTrigger(sTableName, &CMdbStorager::OnUpdateAfter, this);
			pItf->RegTruncateAfterTrigger(sTableName, &CMdbStorager::OnTruncateAfter, this);
		}
		sTableName = pShift;
		if(sTableName)
		{
			sTableName[0] = ',';
			++sTableName;
		}
	}

	return true;
}

void CMdbStorager::Cleanup()
{
	CRbTreeNode* pIt = m_oLinks.First();
	CRbTreeNode* pEnd = m_oLinks.End();
	for(; pIt!=pEnd; pIt=m_oLinks.GetNext(pIt))
	{
		CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
		delete pOdbc;
	}
	m_oLinks.Clear();
	pIt = m_oDomainMdbTable.First();
	pEnd = m_oDomainMdbTable.End();
	for(; pIt!=pEnd; pIt=m_oDomainMdbTable.GetNext(pIt))
	{
		CMdbDomainItem &oItem = m_oDomainMdbTable.GetItem(pIt);
		if(oItem.pToken)
			delete oItem.pToken;
	}
	m_oDomainMdbTable.Clear();
	if(m_pOdbcEnv)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_pOdbcEnv);
		m_pOdbcEnv = NULL;
	}
}

bool CMdbStorager::OnFirstNode(uint32 nDomain)
{
	CRbTreeNode* pIt = m_oDomainMdbTable.Find(nDomain);
	if(pIt == m_oDomainMdbTable.End())
		return true;

	CMdbDomainItem& oItem = m_oDomainMdbTable.GetItem(pIt);
	if(oItem.bLoaded)
		FocpAbort(("CMdbStorager::OnFirstNode(%u) has been loaded", nDomain));

	oItem.bLoaded = true;
	void* pIdx = oItem.oMdbList.First();
	for(; pIdx; pIdx=oItem.oMdbList.GetNext(pIdx))
	{
		CString &oDbName = oItem.oMdbList.GetItem(pIdx);
		pIt = m_oLinks.Find(oDbName);
		if(pIt != m_oLinks.End())
		{
			CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
			if(!pOdbc->Load())
				return false;
		}
	}

	return true;
}

void CMdbStorager::OnOtherNode(uint32 nDomain)
{
	CRbTreeNode* pIt = m_oDomainMdbTable.Find(nDomain);
	if(pIt != m_oDomainMdbTable.End())
	{
		CMdbDomainItem& oItem = m_oDomainMdbTable.GetItem(pIt);
		if(oItem.bLoaded)
			FocpAbort(("CMdbStorager::OnOtherNode(%u) has been loaded", nDomain));
		oItem.bLoaded = true;
	}
}

bool CMdbStorager::Start()
{
	CRbTreeNode* pIt = m_oDomainMdbTable.First();
	CRbTreeNode* pEnd = m_oDomainMdbTable.End();
	for(; pIt!=pEnd; pIt=m_oDomainMdbTable.GetNext(pIt))
	{
		CMdbDomainItem &oItem = m_oDomainMdbTable.GetItem(pIt);
		if(!oItem.bLoaded && !OnFirstNode(m_oDomainMdbTable.GetKey(pIt)))
			return false;
	}

	pIt = m_oLinks.First();
	pEnd = m_oLinks.End();
	for(; pIt!=pEnd; pIt=m_oLinks.GetNext(pIt))
	{
		CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
		pOdbc->Start();
	}
	pIt = m_oDomainMdbTable.First();
	pEnd = m_oDomainMdbTable.End();
	CAcmTokenModule* pTokenModule = CAcmTokenModule::GetInstance();

	for(; pIt!=pEnd; pIt=m_oDomainMdbTable.GetNext(pIt))
	{
		uint32 nDomain = m_oDomainMdbTable.GetKey(pIt);
		CMdbDomainItem &oItem = m_oDomainMdbTable.GetItem(pIt);
		if(oItem.pToken && !pTokenModule->AddToken(oItem.pToken))
		{
			FocpLog(FOCP_LOG_ERROR, ("Add storage token(%u) in domain(%u)", ACM_MDB_STORAGE_TOKEN, nDomain));
			return false;
		}
	}

	return true;
}

void CMdbStorager::Stop(bool bBlock)
{
	CRbTreeNode* pIt = m_oDomainMdbTable.First();
	CRbTreeNode* pEnd = m_oDomainMdbTable.End();
	CAcmTokenModule* pTokenModule = CAcmTokenModule::GetInstance();
	for(; pIt!=pEnd; pIt=m_oDomainMdbTable.GetNext(pIt))
	{
		CMdbDomainItem &oItem = m_oDomainMdbTable.GetItem(pIt);
		if(oItem.pToken)
			pTokenModule->DelToken(oItem.pToken);
	}
	pIt = m_oLinks.First();
	pEnd = m_oLinks.End();
	for(; pIt!=pEnd; pIt=m_oLinks.GetNext(pIt))
	{
		CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
		pOdbc->Stop(false);
	}
	pIt = m_oLinks.First();
	for(; pIt!=pEnd; pIt=m_oLinks.GetNext(pIt))
	{
		CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
		pOdbc->Stop(true);
	}
}

void CMdbStorager::OnFree(void* pAttr, uint32 nAttrType/*0=字段属性，1表属性，2索引属性*/)
{
	if(nAttrType == 0)
	{
		CMdbStorageFieldAttr* pFieldAttr = (CMdbStorageFieldAttr*)pAttr;
		delete pFieldAttr;
	}
	else if(nAttrType == 1)
	{
		CMdbStorageTableAttr* pTabAttr = (CMdbStorageTableAttr*)pAttr;
		if(pTabAttr->pStorageWhere)
			delete (CMdbSqlParaSet*)pTabAttr->pStorageWhere;
		if(pTabAttr->pCacheWhere)
			delete (CMdbSqlParaSet*)pTabAttr->pCacheWhere;
		delete pTabAttr;
	}
}

void CMdbStorager::StartStorage(uint32 nDomain)
{
	FocpInfo(("Start storage process for the domain(%u).", nDomain));

	CRbTreeNode* pIt = m_oDomainMdbTable.Find(nDomain);
	if(pIt != m_oDomainMdbTable.End())
	{
		CMdbDomainItem& oItem = m_oDomainMdbTable.GetItem(pIt);
		CSingleList<CString> &oMdbList = oItem.oMdbList;
		void* pIdx = oMdbList.First();
		while(pIdx)
		{
			CString& oName = oMdbList.GetItem(pIdx);
			pIt = m_oLinks.Find(oName);
			if(pIt != m_oLinks.End())
			{
				CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
				pOdbc->StartStorage();
			}
			pIdx = oMdbList.GetNext(pIdx);
		}
	}
}

void CMdbStorager::StopStorage(uint32 nDomain)
{
	CRbTreeNode* pIt = m_oDomainMdbTable.Find(nDomain);
	if(pIt != m_oDomainMdbTable.End())
	{
		CMdbDomainItem& oItem = m_oDomainMdbTable.GetItem(pIt);
		CSingleList<CString> &oMdbList = oItem.oMdbList;
		void* pIdx = oMdbList.First();
		while(pIdx)
		{
			CString& oName = oMdbList.GetItem(pIdx);
			pIt = m_oLinks.Find(oName);
			if(pIt != m_oLinks.End())
			{
				CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
				pOdbc->StopStorage();
			}
			pIdx = oMdbList.GetNext(pIdx);
		}
	}
	FocpInfo(("End storage process for the domain(%u).", nDomain));
}

CMdbAccess* CMdbStorager::GetEventAccess(const char* sDbName)
{
	CRbTreeNode* pIt = m_oLinks.Find(sDbName);
	if(pIt == m_oLinks.End())
		return NULL;
	CMdbOdbc* pOdbc = m_oLinks.GetItem(pIt);
	return pOdbc->GetEventAccess();
}

void CMdbStorager::CreatePrimaryKey(CString &oPrimaryKey, CMdbIndexDef* pStorageIdx, CMdbRecord* pRecord)
{
	CMdbTableDef* pTabDef = pRecord->m_pTabDef;
	uint32 nFieldCount = pStorageIdx->pExtendAttr->nFieldCount;
	for(uint32 i=0; i<nFieldCount; ++i)
	{
		uint32 nFieldNo = pStorageIdx->pExtendAttr->pFields[i];
		CMdbField* pField = pRecord->GetField(nFieldNo);
		CMdbFieldDef &oFieldDef = pTabDef->pFieldDefines[nFieldNo];
		if(i)
			oPrimaryKey += " and ";
		oPrimaryKey += oFieldDef.sFieldName;
		oPrimaryKey += "=";
		uint32 nSize = pField->GetStringSize();
		char* sVal = new char[nSize], *pShift;
		pField->GetAsString(sVal);
		if(!sVal[0])
			oPrimaryKey += "NULL";
		else
		{
			uint32 nType = pField->GetType();
			switch(nType)
			{
			case MDB_INT8_FIELD:
			case MDB_INT16_FIELD:
			case MDB_INT32_FIELD:
			case MDB_INT64_FIELD:
			case MDB_UINT8_FIELD:
			case MDB_UINT16_FIELD:
			case MDB_UINT32_FIELD:
			case MDB_UINT64_FIELD:
			case MDB_FLOAT_FIELD:
			case MDB_DOUBLE_FIELD:
				oPrimaryKey += sVal;
				break;
			case MDB_CHAR_FIELD:
			case MDB_VARCHAR_FIELD:
			case MDB_LCHAR_FIELD:
			case MDB_VARLCHAR_FIELD:
			case MDB_DATE_FIELD:
			case MDB_TIME_FIELD:
			case MDB_DATETIME_FIELD:
				oPrimaryKey += '\'';
				for(pShift = sVal; pShift[0]; ++pShift)
				{
					if(pShift[0] == '\'')
						oPrimaryKey += '\'';
					oPrimaryKey += pShift[0];
				}
				oPrimaryKey += '\'';
				break;
			}
		}
		delete[] sVal;
	}
}

static void InsertEvent(CMdbAccess* pEventAccess, const char* sTableName, const char* sPrimaryKey, uint32 nOp)
{
	//连接号，表名、主键、操作、操作时间。
	CMdbPara* pInsert = pEventAccess->GetInsertPara();
	pInsert->SetString(0, sTableName);
	pInsert->SetString(1, sPrimaryKey);
	pInsert->SetUInt32(2, nOp);
	pInsert->SetUInt32(3, CTimer::GetTime());
	pEventAccess->Insert();
}

void CMdbStorager::OnInsertAfter(CMdbStorageTableAttr* pTabStoAttr, CMdbAccess* pAccess, CMdbRecord* pRecord)
{
	CString oPrimaryKey;
	CreatePrimaryKey(oPrimaryKey, pTabStoAttr->pStorageIdx, pRecord);
	if(oPrimaryKey.GetSize() > MDB_STORAGEKEY_MAXSIZE)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbStorager::OnInsertAfter(%s) Primary key too long", pAccess->GetTableName()));
		return;
	}
	bool bInsert = true;
	uint32 nDeletedCount = 0;
	CMdbAccess* pEventAccess = GetEventAccess(pAccess->GetMdb()->GetDbName());
	CMdbPara* pPara = pEventAccess->GetQueryPara()->AddParaSet()->AddPara();

	//表名、主键、操作、操作时间。
	const char* sTableName = pRecord->m_pTabDef->sTableName;
	const char* sPrimaryKey = oPrimaryKey.GetStr();
	pPara->SetString(0, sTableName);
	pPara->SetString(1, sPrimaryKey);
	if(!pEventAccess->Delete(&nDeletedCount) && nDeletedCount)
		bInsert = false;

	InsertEvent(pEventAccess, sTableName, sPrimaryKey, bInsert?MDB_EVENT_INSERT:MDB_EVENT_UPDATE);
	pEventAccess->Release();
}

void CMdbStorager::SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger)
{
	CMdb* pDb = CMdb::GetMdb(sMdbName);
	if(pDb)
	{
		CMdbAccess* pAccess = pDb->QueryAccess(sMdbTabName);
		if(pAccess)
		{
			CMdbTableDef* pTabDef = pAccess->GetTableDefine();
			CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
			CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
			pTabStoAttr->StorageTrigger = StorageTrigger;
		}
	}
}

void CMdbStorager::SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger)
{
	CMdb* pDb = CMdb::GetMdb(sMdbName);
	if(pDb)
	{
		CMdbAccess* pAccess = pDb->QueryAccess(sMdbTabName);
		if(pAccess)
		{
			CMdbTableDef* pTabDef = pAccess->GetTableDefine();
			CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
			CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
			pTabStoAttr->CacheTrigger = CacheTrigger;
		}
	}
}

void CMdbStorager::OnInsertAfter(CMdbAccess* pAccess, uint32 nCaller)
{
	if(!(nCaller&MDB_APP_CALLER))
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	CMdbRecord* pRecord = (CMdbRecord*)((CMdbTableAccess*)pAccess)->m_oGetAttr.GetResult(0);
	if(pTabStoAttr->StorageTrigger)
	{
		if(!pTabStoAttr->StorageTrigger(pAccess, pRecord))
			return;
	}
	else
	{
		CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabStoAttr->pStorageWhere;
		if(pCond && !pCond->IsCoincident(*pRecord))
			return;
	}
	OnInsertAfter(pTabStoAttr, pAccess, pRecord);
}

void CMdbStorager::OnDeleteAfter(CMdbStorageTableAttr* pTabStoAttr, CMdbAccess* pAccess, CMdbRecord* pRecord)
{
	CString oPrimaryKey;
	CreatePrimaryKey(oPrimaryKey, pTabStoAttr->pStorageIdx, pRecord);
	if(oPrimaryKey.GetSize() > MDB_STORAGEKEY_MAXSIZE)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbStorager::OnDeleteAfter(%s) Primary key too long", pAccess->GetTableName()));
		return;
	}
	//	（a）如果事件表中存在插入操作，那么删除该事件，并忽略当前事件。
	//	（b）否则删除修改操作，并插入删除事件。
	CMdbAccess* pEventAccess = GetEventAccess(pAccess->GetMdb()->GetDbName());
	CMdbPara* pPara = pEventAccess->GetQueryPara()->AddParaSet()->AddPara();

	//连接号，表名、主键、操作、操作时间。
	const char* sTableName = pRecord->m_pTabDef->sTableName;
	const char* sPrimaryKey = oPrimaryKey.GetStr();
	pPara->SetString(0, sTableName);
	pPara->SetString(1, sPrimaryKey);
	if(!pEventAccess->Query(1, 0))
	{
		CMdbResult* pResult = pEventAccess->GetResultSet()->GetResult(0);
		if(pResult)
		{
			uint32 nOp = pResult->GetUInt32(3);
			pEventAccess->Delete();
			if(nOp == MDB_EVENT_INSERT)
			{
				pEventAccess->Release();
				return;
			}
		}
	}
	InsertEvent(pEventAccess, sTableName, sPrimaryKey, MDB_EVENT_DELETE);
	pEventAccess->Release();
}

void CMdbStorager::OnDeleteAfter(CMdbAccess* pAccess, uint32 nCaller)
{
	if(!(nCaller&MDB_APP_CALLER))
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	CMdbRecord* pRecord = (CMdbRecord*)((CMdbTableAccess*)pAccess)->m_oGetAttr.GetResult(0);

	if(pTabStoAttr->StorageTrigger)
	{
		if(!pTabStoAttr->StorageTrigger(pAccess, pRecord))
			return;
	}
	else
	{
		CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabStoAttr->pStorageWhere;
		if(pCond && !pCond->IsCoincident(*pRecord))
			return;
	}
	OnDeleteAfter(pTabStoAttr, pAccess, pRecord);
}

void CMdbStorager::OnUpdateAfter(CMdbAccess* pAccess, uint32 nCaller)
{
	if(!(nCaller&MDB_APP_CALLER))
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	CMdbTableAttr* pTabAttr = pTabDef->pExtendAttr;
	CMdbStorageTableAttr* pTabStoAttr = (CMdbStorageTableAttr*)pTabAttr->pExtAttr[MDB_EXTEND_PLUGIN_STORAGER];
	CMdbRecord* pRecordNew = (CMdbRecord*)((CMdbTableAccess*)pAccess)->m_oGetAttr.GetResult(0);
	CMdbRecord* pRecordOld = (CMdbRecord*)((CMdbTableAccess*)pAccess)->m_oGetAttr.GetResult(1);
	CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pTabStoAttr->pStorageWhere;
	if(pTabStoAttr->StorageTrigger || pCond)
	{
		bool bOld = pTabStoAttr->StorageTrigger?pTabStoAttr->StorageTrigger(pAccess, pRecordOld):pCond->IsCoincident(*pRecordOld);
		bool bNew = pTabStoAttr->StorageTrigger?pTabStoAttr->StorageTrigger(pAccess, pRecordNew):pCond->IsCoincident(*pRecordNew);
		if(bOld)
		{
			if(!bNew)
			{//	（a）如果旧记录符合条件，而新记录不符合条件，转删除流程。
				OnDeleteAfter(pTabStoAttr, pAccess, pRecordOld);
				return;
			}
		}
		else if(bNew)
		{//	（b）如果旧记录不符合条件，而新记录符合条件，转插入流程。
			OnInsertAfter(pTabStoAttr, pAccess, pRecordNew);
			return;
		}
		else return;
	}
	if(((CMdbTableAccess*)pAccess)->m_oSetAttr.IncludeIndexField(pTabStoAttr->pStorageIdx))
	{//	（c）如果修改了主键，将产生先删后插两个事件。
		OnDeleteAfter(pTabStoAttr, pAccess, pRecordOld);
		OnInsertAfter(pTabStoAttr, pAccess, pRecordNew);
		return;
	}

	CString oPrimaryKey;
	CreatePrimaryKey(oPrimaryKey, pTabStoAttr->pStorageIdx, pRecordNew);
	if(oPrimaryKey.GetSize() > MDB_STORAGEKEY_MAXSIZE)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbStorager::OnInsertAfter(%s) Primary key too long", pAccess->GetTableName()));
		return;
	}
	uint32 nOp = MDB_EVENT_UPDATE;
	const char* sTableName = pRecordNew->m_pTabDef->sTableName;
	const char* sPrimaryKey = oPrimaryKey.GetStr();
	CMdbAccess* pEventAccess = GetEventAccess(pAccess->GetMdb()->GetDbName());
	pCond = (CMdbSqlParaSet*)pEventAccess->GetQueryPara();
	CMdbPara* pPara = pCond->AddParaSet()->AddPara();
	pPara->SetString(0, sTableName);
	pPara->SetString(1, sPrimaryKey);
	if(!pEventAccess->Query(1, 0))
	{
		CMdbResult* pResult = pEventAccess->GetResultSet()->GetResult(0);
		if(pResult)
		{
		//	（d）如果事件表中存在插入操作，删除该记录，并重新插入（最新时间），事件类型不变。
		//	（e）如果事件表中存在修改操作，删除该记录，并重新插入（最新时间），事件类型不变。
			uint32 nOp2;
			nOp2 = pResult->GetUInt32(2);
			pEventAccess->Delete();
			if(nOp2 == MDB_EVENT_INSERT)
				nOp = MDB_EVENT_INSERT;
		}
	}

	//（f）产生修改事件
	InsertEvent(pEventAccess, sTableName, sPrimaryKey, nOp);
	pEventAccess->Release();
}


void CMdbStorager::OnTruncateAfter(CMdbAccess* pAccess, uint32 nCaller)
{
	//（B）当清除操作产生时，将删除该表之前的所有操作。
	if(!(nCaller&MDB_APP_CALLER))
		return;
	CMdbTableDef* pTabDef = pAccess->GetTableDefine();
	const char* sTableName = pTabDef->sTableName;
	CMdbAccess* pEventAccess = GetEventAccess(pAccess->GetMdb()->GetDbName());
	CMdbPara* pPara = pEventAccess->GetQueryPara()->AddParaSet()->AddPara();
	pPara->SetString(0, sTableName);
	pEventAccess->Delete();

	InsertEvent(pEventAccess, sTableName, "", MDB_EVENT_TRUNCATE);
	pEventAccess->Release();
}

void CMdbStorager::OnInsertAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	((CMdbStorager*)pContext)->OnInsertAfter(pAccess, nCaller);
}

void CMdbStorager::OnDeleteAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	((CMdbStorager*)pContext)->OnDeleteAfter(pAccess, nCaller);
}

void CMdbStorager::OnUpdateAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	((CMdbStorager*)pContext)->OnUpdateAfter(pAccess, nCaller);
}

void CMdbStorager::OnTruncateAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext)
{
	((CMdbStorager*)pContext)->OnTruncateAfter(pAccess, nCaller);
}

FOCP_END();
