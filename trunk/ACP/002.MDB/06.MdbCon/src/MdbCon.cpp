
#include "MdbCon.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();

//-----------------------------------------------------
// CRemoteAccess
//-----------------------------------------------------
CRemoteAccess::CRemoteAccess(CMdb* pDb, CMdbTableDef* pTabDef):
	CMdbTableAccess(pDb, pTabDef)
{
}

CRemoteAccess::~CRemoteAccess()
{
}

uint32 CRemoteAccess::Insert(uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Insert(sLocalDbName, sRemoteDbName, this);
}

uint32 CRemoteAccess::Update(uint32* pModifiedCount, uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Update(sLocalDbName, sRemoteDbName, pModifiedCount, this);
}

uint32 CRemoteAccess::Delete(uint32* pDeletedCount, uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Delete(sLocalDbName, sRemoteDbName, pDeletedCount, this);
}

uint32 CRemoteAccess::Truncate(uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Truncate(sLocalDbName, sRemoteDbName, this);
}

uint32 CRemoteAccess::Query(uint32 nPageSize, uint32 nSkipCount, uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Query(sLocalDbName, sRemoteDbName, this, nPageSize, nSkipCount);
}

uint32 CRemoteAccess::Count(uint32 &nCount, uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Query(sLocalDbName, sRemoteDbName, this, nCount);
}

uint32 CRemoteAccess::Exist(uint32& bExist, uint32)
{
	CRemoteMdb* pDb = (CRemoteMdb*)m_pDb;
	const char* sLocalDbName = pDb->GetDbName();
	const char* sRemoteDbName = pDb->m_oRemoteDbName.GetStr();
	return pDb->m_pConnector->Exist(sLocalDbName, sRemoteDbName, this, bExist);
}

//-----------------------------------------------------
// CRemoteMdb
//-----------------------------------------------------
CRemoteMdb::CRemoteMdb(const char* sLocalDbName, const char* sRemoteDbName)
	:CMdb(sLocalDbName), m_oDbDef(GetDbName()), m_oRemoteDbName(sRemoteDbName)
{
	m_pConnector = NULL;
}

CRemoteMdb::~CRemoteMdb()
{
}

CMdbAccess* CRemoteMdb::CreateAccess(const char* sTableName)
{
	CMdbTableDef* pTableDef = m_oDbDef.GetTableDefine(sTableName);
	if(pTableDef == NULL)
		return NULL;

	return new CRemoteAccess(this, pTableDef);
}

CMdbTableDef* CRemoteMdb::GetTableDefine(const char* sTableName)
{
	return m_oDbDef.GetTableDefine(sTableName);
}

const char* CRemoteMdb::GetTableList()
{
	return m_oDbDef.GetTableList();
}

CMdbIndexDef* CRemoteMdb::GetIndexDefine(const char* sIndexName)
{
	return m_oDbDef.GetIndexDefine(sIndexName);
}

const char* CRemoteMdb::GetIndexList()
{
	return m_oDbDef.GetIndexList();
}

uint32 CRemoteMdb::GetIndexCount(const char* sTableName, uint32 & nCount)
{
	nCount = 0;
	CMdbTableDef* pTabDef = m_oDbDef.GetTableDefine(sTableName);
	if(pTabDef == NULL)
		return MDB_TABLE_NOTEXIST;
	nCount = pTabDef->pExtendAttr->nIndexCount;
	return MDB_SUCCESS;
}

uint32 CRemoteMdb::GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName)
{
	CMdbTableDef* pTabDef = m_oDbDef.GetTableDefine(sTableName);
	if(pTabDef == NULL)
		return MDB_TABLE_NOTEXIST;
	if(nIdx >= pTabDef->pExtendAttr->nIndexCount)
		return MDB_INDEX_NOTEXIST;
	CString::StringCopy(pIndexName, pTabDef->pExtendAttr->pIndexDefineSet[nIdx]->sIndexName);
	return MDB_SUCCESS;
}

//-----------------------------------------------------
// CMdbConnector
//-----------------------------------------------------
CMdbConnector::CMdbConnector():	m_oTcp(this)
{
	m_pLoginContext = NULL;
}

CMdbConnector::~CMdbConnector()
{
	Cleanup();
}

bool CMdbConnector::GetAccessInfo(char* sDbList, CString& oAccessInfo)
{
	char* pShift = (char*)CString::CharOfString(sDbList, ')');
	if(pShift)
		pShift[0] = 0;
	oAccessInfo = sDbList;
	if(pShift)
		pShift[0] = ')';
	return !oAccessInfo.Empty();
}

bool CMdbConnector::InitializeDbList(char* sDbList, bool bForce)
{
	if(!sDbList || !sDbList[0])
		return true;
	m_bForce = bForce;
	CString oDbList(sDbList);
	sDbList = (char*)oDbList.GetStr();
	if(!bForce && sDbList[0] == '(')
		return GetAccessInfo(sDbList+1, m_oSysdbAccess);
	char* pShift, *sLdbName=sDbList, *sRdbName, *sAccess;
	while(sLdbName)
	{
		pShift = (char*)CString::CharOfString(sLdbName, ',');
		if(pShift)
			pShift[0] = 0;
		sRdbName = (char*)CString::CharOfString(sLdbName, ':');
		if(sRdbName)
		{
			sRdbName[0] = 0;
			++sRdbName;
		}
		else
			sRdbName = sLdbName;
		CString oAccess;
		if(!bForce)
		{
			sAccess = (char*)CString::CharOfString(sRdbName, '(');
			if(sAccess)
			{
				sAccess[0] = 0;
				GetAccessInfo(sAccess+1, oAccess);
			}
		}
		if(!CString(sRdbName).IsIdentifierOfC())
		{
			FocpLog(FOCP_LOG_WARNING, ("CMdbConnector::Initialize(%s) include invalid rdb name", sDbList));
			return false;
		}
		if(!CString(sLdbName).IsIdentifierOfC())
		{
			FocpLog(FOCP_LOG_WARNING, ("CMdbConnector::Initialize(%s) include invalid ldb name", sDbList));
			return false;
		}
		if(!m_pLoginContext->oRdbList.Empty())
		{
			m_pLoginContext->oRdbList += ",";
			m_pLoginContext->oLdbList += ",";
			if(!bForce)
				m_pLoginContext->oAccessList += ",";
		}
		m_pLoginContext->oRdbList += sRdbName;
		m_pLoginContext->oLdbList += sLdbName;
		if(!bForce)
			m_pLoginContext->oAccessList += oAccess.GetStr();
		sLdbName = pShift;
		if(sLdbName)
			sLdbName++;
	}
	return true;
}

bool CMdbConnector::Initialize(const char* sDbList, const char* sServerAddr, uint16 nServerPort)
{
	m_oTcp.Initialize(sServerAddr, nServerPort);
	m_nMagic = m_oTcp.GetMagic();
	m_sServerAddr = m_oTcp.GetServerAddr();
	m_nServerPort = nServerPort;
	m_pLoginContext = new CLoginContext;
	return InitializeDbList((char*)sDbList, false);
}

void CMdbConnector::Cleanup()
{
	m_oTcp.Cleanup();
	if(m_pLoginContext)
	{
		delete m_pLoginContext;
		m_pLoginContext = NULL;
	}
}

bool CMdbConnector::Start()
{
	return m_oTcp.Start();
}

void CMdbConnector::Stop(bool bBlock)
{
	m_oTcp.Stop(bBlock);
}

uint32 CMdbConnector::Insert(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_INSERT_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oInsertAttr.Write(oStream);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Insert(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Update(const char* sLocalDbName, const char* sRemoteDbName, uint32* pModifiedCount, CRemoteAccess* pAccess)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(pModifiedCount)
		pModifiedCount[0] = 0;
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_UPDATE_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oIdxAttr.Write(oStream);
	if(bRet)
		bRet = pAccess->m_oSetAttr.Write(oStream);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		if(pModifiedCount)
			pModifiedCount[0] = pAccess->m_nCount;
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Delete(const char* sLocalDbName, const char* sRemoteDbName, uint32* pDeletedCount, CRemoteAccess* pAccess)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(pDeletedCount)
		pDeletedCount[0] = 0;
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_DELETE_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oIdxAttr.Write(oStream);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Update(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		if(pDeletedCount)
			pDeletedCount[0] = pAccess->m_nCount;
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Truncate(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_TRUNCATE_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Truncate(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Delete(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Query(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32 nPageSize, uint32 nSkipCount)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_QUERY_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oIdxAttr.Write(oStream);
	if(bRet)
		bRet = pAccess->m_oFilter.Write(oStream);
	if(bRet)
		bRet = oStream.Write(nPageSize);
	if(bRet)
		bRet = oStream.Write(nSkipCount);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Query(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Query(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32 &nCount)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	nCount = 0;
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_QUERYCOUNT_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oIdxAttr.Write(oStream);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		else
			nCount = pAccess->m_nCount;
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Count(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

uint32 CMdbConnector::Exist(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32& bExist)
{
	char sMsg[4096];
	CSyncCallee oCallee(pAccess);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	const char* sTableName = pAccess->GetTableName();
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	bExist = 0;
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_SESSION_BUSY", sLocalDbName, sTableName));
		return MDB_SESSION_BUSY;
	}
	bool bRet = true;
	if(bRet)
		bRet = oStream.Write((uint32)0);
	if(bRet)
		bRet = oStream.Write(m_nMagic);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_SERVER_MODULE);
	if(bRet)
		bRet = oStream.Write((uint32)MDB_QUERYEXIT_REQ);
	if(bRet)
		bRet = oStream.Write(nSessionId);
	if(bRet)
		bRet = WriteString(oStream, sRemoteDbName);
	if(bRet)
		bRet = WriteString(oStream, sTableName);
	if(bRet)
		bRet = pAccess->m_oIdxAttr.Write(oStream);
	if(!bRet)
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_PACKAGE_ERROR", sLocalDbName, sTableName));
		return MDB_PACKAGE_ERROR;
	}
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_SEND_ERROR", sLocalDbName, sTableName));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 0xFFFFFFFF))
	{
	case FOCP_SESSION_NORMAL:
		if(pAccess->m_nStatus)
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): %s", sLocalDbName, sTableName, GetMdbError(pAccess->m_nStatus)));
		else
			bExist = pAccess->m_nCount;
		return pAccess->m_nStatus;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_TIMEOUT_ERROR", sLocalDbName, sTableName));
		return MDB_TIMEOUT_ERROR;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_SESSION_BREAK", sLocalDbName, sTableName));
		return MDB_SESSION_BREAK;
	}
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::Exist(%s.%s): MDB_INVALID_SESSION", sLocalDbName, sTableName));
	return MDB_INVALID_SESSION;
}

static void GetRandomChar(char &c, char d)
{
	uint32 x;
loop:
	x = (uint32)Random();
	x %= 26;
	c = x + 'A';
	if(c == d)
		goto loop;
}

static void GetRandomString(CString &oStr, uint32 nLen, const char* sNot)
{
	char c;
	for(uint32 i=0; i<nLen; ++i)
	{
		GetRandomChar(c, *sNot);
		oStr += c;
		if(*sNot)
			++sNot;
	}
}

static void CreateAccessInfo(CString& oAccessInfo, bool bForce)
{
	GetRandomString(oAccessInfo, 10, "");
	if(bForce)
		oAccessInfo += " 1";
	else
		oAccessInfo += " 0";
	uint32 i, nLen = oAccessInfo.GetSize();
	char* sInfo = (char*)oAccessInfo.GetStr();
	for(i=0; i<nLen; ++i)
		sInfo[i] = (char)(((uint8)sInfo[i]) ^ ((uint8)'J'));//Jacky's First Char
	sInfo = Base64Encode(sInfo, nLen, &nLen);
	oAccessInfo = sInfo;
	CMalloc::Free(sInfo);
}

uint32 CMdbConnector::GetDbList()
{
	if(m_pLoginContext->oRdbList.Empty())
	{
		if(m_oSysdbAccess.Empty())
			CreateAccessInfo(m_oSysdbAccess, false);
		const char* pDbList = CMdb::GetDbList();
		if(pDbList && pDbList[0])
		{
			FocpLog(FOCP_LOG_ERROR, ("Pure remote database cann't define the local database"));
			return 1;
		}
		char sMsg[128];
		CString oDbList;
		CSyncCallee oCallee(&oDbList);
		CMemoryStream oStream(sMsg, sizeof(sMsg));
		uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
		if(!nSessionId)
		{
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): MDB_SESSION_BUSY", m_sServerAddr, m_nServerPort));
			return 1;
		}
		oStream.Write((uint32)0);
		oStream.Write(m_nMagic);
		oStream.Write((uint32)MDB_SERVER_MODULE);
		oStream.Write((uint32)MDB_GETDBLIST_REQ);
		oStream.Write(nSessionId);
		WriteString(oStream, m_oSysdbAccess.GetStr());
		nSize = oStream.GetPosition();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		if(!m_oTcp.Send(sMsg, nSize))
		{
			m_oTcp.ReleaseCall(&oCallee);
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): MDB_SEND_ERROR", m_sServerAddr, m_nServerPort));
			return MDB_SEND_ERROR;
		}
		switch(m_oTcp.WaitCall(nSessionId, 2000))
		{
		case FOCP_SESSION_NORMAL:
			if(m_pLoginContext->nStatus)
			{
				FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): %s", m_sServerAddr, m_nServerPort, GetMdbError(m_pLoginContext->nStatus)));
				return 1;
			}
			break;
		case FOCP_SESSION_TIMEOUT:
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): MDB_TIMEOUT_ERROR", m_sServerAddr, m_nServerPort));
			return 1;
		case FOCP_SESSION_BREAK:
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): MDB_SESSION_BREAK", m_sServerAddr, m_nServerPort));
			return MDB_SESSION_BREAK;
		default:
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16): MDB_INVALID_SESSION", m_sServerAddr, m_nServerPort));
			return 1;
		}
		m_oSysdbAccess = "";
		if(!InitializeDbList((char*)oDbList.GetStr(), true))
		{
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetDbList(%s:%u16) failure", m_sServerAddr, m_nServerPort));
			return 1;
		}
	}
	char*pShift, *sRdbName = (char*)m_pLoginContext->oRdbList.GetStr(), *pShift2=NULL, *pShift3=NULL;
	char* sLdbName = (char*)m_pLoginContext->oLdbList.GetStr();
	char* sAccess = (char*)m_pLoginContext->oAccessList.GetStr();
	if(m_bForce)
		sAccess = NULL;
	while(sRdbName)
	{
		pShift = (char*)CString::CharOfString(sRdbName, ',');
		if(pShift)
			pShift[0] = 0;
		pShift2 = (char*)CString::CharOfString(sLdbName, ',');
		if(pShift2)
			pShift2[0] = 0;
		if(!m_bForce)
		{
			pShift3 = (char*)CString::CharOfString(sAccess, ',');
			if(pShift3)
				pShift3[0] = 0;
		}
		CRemoteMdb* pDb = new CRemoteMdb(sLdbName, sRdbName);
		pDb->m_pConnector = this;
		if(GetTableList(pDb, sAccess))
			return 1;
		if(GetIndexList(pDb))
			return 1;
		sRdbName = pShift;
		if(sRdbName)
		{
			sRdbName[0] = ',';
			++sRdbName;
		}
		sLdbName = pShift2;
		if(sLdbName)
		{
			sLdbName[0] = ',';
			++sLdbName;
		}
		if(!m_bForce)
		{
			sAccess = pShift3;
			if(sAccess)
			{
				sAccess[0] = ',';
				++sAccess;
			}
		}
	}
	CMdb* pDb = CMdb::GetMdb(MDB_SYSDB_NAME);
	if(!pDb)
	{
		CRemoteMdb* pRdb = new CRemoteMdb(MDB_SYSDB_NAME, MDB_SYSDB_NAME);
		pRdb->m_pConnector = this;
		if(GetTableDefine(pRdb, "MdbSecurity"))
		{
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.MdbSecurity-%s:%u16) failure", MDB_SYSDB_NAME, m_sServerAddr, m_nServerPort));
			return 1;
		}
		//没有获取索引定义
	}
	return 0;
}

uint32 CMdbConnector::GetTableList(CRemoteMdb* pDb, const char* sAccessInfo)
{
	char sMsg[128];
	CString oTableList, oAccessInfo;
	const char* sDbName = pDb->m_oRemoteDbName.GetStr();
	CSyncCallee oCallee(&oTableList);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): MDB_SESSION_BUSY", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	oStream.Write((uint32)0);
	oStream.Write(m_nMagic);
	oStream.Write((uint32)MDB_SERVER_MODULE);
	oStream.Write((uint32)MDB_GETTABLELIST_REQ);
	oStream.Write(nSessionId);
	WriteString(oStream, sDbName);
	if(!sAccessInfo)
	{//强制访问
		CreateAccessInfo(oAccessInfo, true);
		sAccessInfo = oAccessInfo.GetStr();
	}
	else if(!sAccessInfo[0])
	{//直接访问
		CreateAccessInfo(oAccessInfo, false);
		sAccessInfo = oAccessInfo.GetStr();
	}
	WriteString(oStream, sAccessInfo);
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): MDB_SEND_ERROR", sDbName, m_sServerAddr, m_nServerPort));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 2000))
	{
	case FOCP_SESSION_NORMAL:
		break;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): MDB_TIMEOUT_ERROR", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): MDB_SESSION_BREAK", sDbName, m_sServerAddr, m_nServerPort));
		return MDB_SESSION_BREAK;
	default:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): MDB_INVALID_SESSION", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	char*pShift, *sTableName = (char*)oTableList.GetStr();
	while(sTableName)
	{
		pShift = (char*)CString::CharOfString(sTableName, ',');
		if(pShift)
			pShift[0] = 0;
		if(GetTableDefine(pDb, sTableName))
		{
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableList(%s-%s:%u16): GetTableDefine(%s) failure", sDbName, m_sServerAddr, m_nServerPort, sTableName));
			return 1;
		}
		sTableName = pShift;
		if(sTableName)
		{
			sTableName[0] = ',';
			++sTableName;
		}
	}
	return 0;
}

uint32 CMdbConnector::GetIndexList(CRemoteMdb* pDb)
{
	char sMsg[128];
	CString oIndexList;
	const char* sDbName = pDb->m_oRemoteDbName.GetStr();
	CSyncCallee oCallee(&oIndexList);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexList(%s-%s:%u16): MDB_SESSION_BUSY", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	oStream.Write((uint32)0);
	oStream.Write(m_nMagic);
	oStream.Write((uint32)MDB_SERVER_MODULE);
	oStream.Write((uint32)MDB_GETINDEXLIST_REQ);
	oStream.Write(nSessionId);
	WriteString(oStream, sDbName);
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexList(%s-%s:%u16): MDB_SEND_ERROR", sDbName, m_sServerAddr, m_nServerPort));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 2000))
	{
	case FOCP_SESSION_NORMAL:
		break;
	case FOCP_SESSION_TIMEOUT:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexList(%s-%s:%u16): MDB_TIMEOUT_ERROR", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	case FOCP_SESSION_BREAK:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexList(%s-%s:%u16): MDB_SESSION_BREAK", sDbName, m_sServerAddr, m_nServerPort));
		return MDB_SESSION_BREAK;
	default:
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexList(%s-%s:%u16): MDB_INVALID_SESSION", sDbName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	char*pShift, *sIndexName = (char*)oIndexList.GetStr();
	while(sIndexName)
	{
		pShift = (char*)CString::CharOfString(sIndexName, ',');
		if(pShift)
			pShift[0] = 0;
		if(GetIndexDefine(pDb, sIndexName))
			return 1;
		sIndexName = pShift;
		if(sIndexName)
		{
			sIndexName[0] = ',';
			++sIndexName;
		}
	}
	return 0;
}

struct CGetTableDefine
{
	CMdbDefine* pDbDef;
	CMdbTableDefine* pTabDef;
};

uint32 CMdbConnector::GetTableDefine(CRemoteMdb* pDb, const char* sTabName)
{
	const char* sDbName = pDb->m_oRemoteDbName.GetStr();
	CMdbTableDefine * pDefine = NULL;
	CMdbDefine& oDbDef = pDb->m_oDbDef;
	uint32 nRet = oDbDef.CreateTable(sTabName, pDefine);
	if(nRet)
		return 1;
	char sMsg[256];
	CGetTableDefine oContext = {&oDbDef, pDefine};
	CSyncCallee oCallee(&oContext);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		pDefine->~CMdbTableDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): MDB_SESSION_BUSY", sDbName, sTabName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	oStream.Write((uint32)0);
	oStream.Write(m_nMagic);
	oStream.Write((uint32)MDB_SERVER_MODULE);
	oStream.Write((uint32)MDB_GETTABLEDEF_REQ);
	oStream.Write(nSessionId);
	WriteString(oStream, sDbName);
	WriteString(oStream, sTabName);
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		pDefine->~CMdbTableDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): MDB_SEND_ERROR", sDbName, sTabName, m_sServerAddr, m_nServerPort));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 2000))
	{
	case FOCP_SESSION_NORMAL:
		if(m_pLoginContext->nStatus)
		{
			pDefine->~CMdbTableDefine();
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): %s", sDbName, sTabName, m_sServerAddr, m_nServerPort, GetMdbError(m_pLoginContext->nStatus)));
			return 1;
		}
		return 0;
	case FOCP_SESSION_TIMEOUT:
		pDefine->~CMdbTableDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): MDB_TIMEOUT_ERROR", sDbName, sTabName, m_sServerAddr, m_nServerPort));
		return 1;
	case FOCP_SESSION_BREAK:
		pDefine->~CMdbTableDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): MDB_SESSION_BREAK", sDbName, sTabName, m_sServerAddr, m_nServerPort));
		return MDB_SESSION_BREAK;
	}
	pDefine->~CMdbTableDefine();
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetTableDefine(%s.%s-%s:%u16): MDB_INVALID_SESSION", sDbName, sTabName, m_sServerAddr, m_nServerPort));
	return 1;
}

struct CGetIndexDefine
{
	CMdbDefine* pDbDef;
	CMdbIndexDefine* pIdxDef;
};

uint32 CMdbConnector::GetIndexDefine(CRemoteMdb* pDb, const char* sIdxName)
{
	const char* sDbName = pDb->m_oRemoteDbName.GetStr();
	CMdbIndexDefine * pDefine = NULL;
	CMdbDefine& oDbDef = pDb->m_oDbDef;
	uint32 nRet = oDbDef.CreateIndex(sIdxName, pDefine);
	if(nRet)
		return 1;
	char sMsg[256];
	CGetIndexDefine oContext = {&oDbDef, pDefine};
	CSyncCallee oCallee(&oContext);
	CMemoryStream oStream(sMsg, sizeof(sMsg));
	uint32 nSize, nSessionId = m_oTcp.ApplyCall(&oCallee);
	if(!nSessionId)
	{
		pDefine->~CMdbIndexDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): MDB_SESSION_BUSY", sDbName, sIdxName, m_sServerAddr, m_nServerPort));
		return 1;
	}
	oStream.Write((uint32)0);
	oStream.Write(m_nMagic);
	oStream.Write((uint32)MDB_SERVER_MODULE);
	oStream.Write((uint32)MDB_GETINDEXDEF_REQ);
	oStream.Write(nSessionId);
	WriteString(oStream, sDbName);
	WriteString(oStream, sIdxName);
	nSize = oStream.GetPosition();
	oStream.SetPosition(0);
	oStream.Write(nSize);
	if(!m_oTcp.Send(sMsg, nSize))
	{
		m_oTcp.ReleaseCall(&oCallee);
		pDefine->~CMdbIndexDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): MDB_SEND_ERROR", sDbName, sIdxName, m_sServerAddr, m_nServerPort));
		return MDB_SEND_ERROR;
	}
	switch(m_oTcp.WaitCall(nSessionId, 2000))
	{
	case FOCP_SESSION_NORMAL:
		if(m_pLoginContext->nStatus)
		{
			pDefine->~CMdbIndexDefine();
			FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): %s", sDbName, sIdxName, m_sServerAddr, m_nServerPort, GetMdbError(m_pLoginContext->nStatus)));
			return 1;
		}
		return 0;
	case FOCP_SESSION_TIMEOUT:
		pDefine->~CMdbIndexDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): MDB_TIMEOUT_ERROR", sDbName, sIdxName, m_sServerAddr, m_nServerPort));
		return 1;
	case FOCP_SESSION_BREAK:
		pDefine->~CMdbIndexDefine();
		FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): MDB_SESSION_BREAK", sDbName, sIdxName, m_sServerAddr, m_nServerPort));
		return MDB_SESSION_BREAK;
	}
	pDefine->~CMdbIndexDefine();
	FocpLog(FOCP_LOG_WARNING, ("CRemoteMdb::GetIndexDefine(%s.%s-%s:%u16): MDB_INVALID_SESSION", sDbName, sIdxName, m_sServerAddr, m_nServerPort));
	return 1;
}

void CMdbConnector::ProcessMsg(CAcmTcpClient* pClient, CTcpHead& oHead, CMemoryStream &oStream)
{
	if(oHead.nModule != MDB_SERVER_MODULE)
		return;
	switch(oHead.nCmd)
	{
	case MDB_GETDBLIST_RESP:
	case MDB_GETTABLELIST_RESP:
	case MDB_GETINDEXLIST_RESP:
		ReturnNameList(oStream);
		break;
	case MDB_GETTABLEDEF_RESP:
		ReturnTableDefine(oStream);
		break;
	case MDB_GETINDEXDEF_RESP:
		ReturnIndexDefine(oStream);
		break;
	case MDB_INSERT_RESP:
	case MDB_UPDATE_RESP:
	case MDB_DELETE_RESP:
	case MDB_TRUNCATE_RESP:
		ReturnModify(oStream);
		break;
	case MDB_QUERY_RESP:
		ReturnQuery(oStream);
		break;
	case MDB_QUERYCOUNT_RESP:
	case MDB_QUERYEXIT_RESP:
		ReturnQueryCount(oStream);
		break;
	}
}

uint32 CMdbConnector::OnLogin(CAcmTcpClient* pClient, bool bReLogin)
{
	if(!bReLogin)
		return GetDbList();
	return 0;
}

bool CMdbConnector::ReadString(CMemoryStream &oStream, CString *pStr)
{
	uint32 nSize;
	pStr->Clear();
	bool bRet = oStream.Read(nSize);
	if(bRet && nSize)
	{
		CString oTmp('A', nSize);
		bRet = (nSize==oStream.Read((void*)oTmp.GetStr(), nSize));
		if(bRet)
			pStr->Swap(oTmp);
	}
	return bRet;
}

bool CMdbConnector::WriteString(CMemoryStream &oStream, const char* sStr)
{
	uint32 nSize = CString::StringLength(sStr);
	bool bRet = oStream.Write(nSize);
	if(bRet && nSize)
		bRet = (nSize == oStream.Write((void*)sStr, nSize));
	return bRet;
}

void CMdbConnector::ReturnNameList(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(m_pLoginContext->nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			if(!m_pLoginContext->nStatus)
			{
				CString* pDbList = (CString*)pCallee->QueryCallObject();
				if(!ReadString(oStream, pDbList))
					m_pLoginContext->nStatus = MDB_PACKAGE_ERROR;
			}
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

void CMdbConnector::ReturnTableDefine(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(m_pLoginContext->nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			if(!m_pLoginContext->nStatus)
			{
				CGetTableDefine* pContext = (CGetTableDefine*)pCallee->QueryCallObject();
				if(!pContext->pTabDef->Read(pContext->pDbDef, oStream))
					m_pLoginContext->nStatus = MDB_PACKAGE_ERROR;
			}
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

void CMdbConnector::ReturnIndexDefine(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(m_pLoginContext->nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			if(!m_pLoginContext->nStatus)
			{
				CGetIndexDefine* pContext = (CGetIndexDefine*)pCallee->QueryCallObject();
				if(!pContext->pIdxDef->Read(pContext->pDbDef, oStream))
					m_pLoginContext->nStatus = MDB_PACKAGE_ERROR;
			}
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

void CMdbConnector::ReturnModify(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId, nStatus, nCount=0;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(nStatus);
	if(bRet)
	{
		oStream.Read(nCount);
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			CRemoteAccess* pAccess = (CRemoteAccess*)pCallee->QueryCallObject();
			pAccess->m_nStatus = nStatus;
			pAccess->m_nCount = nCount;
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

void CMdbConnector::ReturnQuery(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId, nStatus;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			CRemoteAccess* pAccess = (CRemoteAccess*)pCallee->QueryCallObject();
			pAccess->m_nStatus = nStatus;
			if(!nStatus)
			{
				if(!pAccess->m_oGetAttr.Read(oStream))
				{
					pAccess->m_oGetAttr.Clear();
					pAccess->m_nStatus = MDB_PACKAGE_ERROR;
				}
			}
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

void CMdbConnector::ReturnQueryCount(CMemoryStream &oStream)
{
	bool bRet = true;
	uint32 nSessionId, nStatus;
	if(bRet)
		bRet = oStream.Read(nSessionId);
	if(bRet)
		bRet = oStream.Read(nStatus);
	if(bRet)
	{
		CSyncCallee* pCallee = m_oTcp.QueryCall(nSessionId);
		if(pCallee)
		{
			CRemoteAccess* pAccess = (CRemoteAccess*)pCallee->QueryCallObject();
			pAccess->m_nStatus = nStatus;
			if(!nStatus)
			{
				if(!oStream.Read(pAccess->m_nCount))
					pAccess->m_nStatus = MDB_PACKAGE_ERROR;
			}
			pCallee->Answer();
			m_oTcp.ReleaseCall(pCallee);
		}
	}
}

FOCP_END();
