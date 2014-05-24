
#include "MdbLsn.hpp"
#include "MdbAccess.hpp"
#include "MdbDef.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();

CMdbSvrModule::CMdbSvrModule()
{
	CAcmTcpServer* pTcp = CAcmTcpServer::GetInstance();
	pTcp->RegisterModule(MDB_SERVER_MODULE, this);
}

CMdbSvrModule::~CMdbSvrModule()
{
	CAcmTcpServer* pTcp = CAcmTcpServer::GetInstance();
	pTcp->DeRegisterModule(MDB_SERVER_MODULE);
}

void CMdbSvrModule::ProcessAcmModuleMsg(CAcmTcpLink* pLink, uint32 nCmd, CMemoryStream& oStream)
{
	uint32 nStatus = pLink->GetStatus();
	if(nStatus == 2)
	{
		switch(nCmd)
		{
		default:
			FocpLog(FOCP_LOG_WARNING, ("Invalid Msg(op=%u, status=%u)", nCmd, nStatus));
			pLink->Close();
			break;
		case MDB_GETDBLIST_REQ:
			ProcessGetDbList(pLink, oStream);
			break;
		case MDB_GETTABLELIST_REQ:
			ProcessGetTableList(pLink, oStream);
			break;
		case MDB_GETINDEXLIST_REQ:
			ProcessGetIndexList(pLink, oStream);
			break;
		case MDB_GETTABLEDEF_REQ:
			ProcessGetTableDef(pLink, oStream);
			break;
		case MDB_GETINDEXDEF_REQ:
			ProcessGetIndexDef(pLink, oStream);
			break;
		}
	}
	else if(nStatus == 3)
	{
		switch(nCmd)
		{
		default:
			FocpLog(FOCP_LOG_WARNING, ("Invalid Msg(op=%u, status=%u)", nCmd, nStatus));
			pLink->Close();
			break;
		case MDB_INSERT_REQ:
			ProcessInsert(pLink, oStream);
			break;
		case MDB_UPDATE_REQ:
			ProcessUpdate(pLink, oStream);
			break;
		case MDB_DELETE_REQ:
			ProcessDelete(pLink, oStream);
			break;
		case MDB_TRUNCATE_REQ:
			ProcessTruncate(pLink, oStream);
			break;
		case MDB_QUERY_REQ:
			ProcessQuery(pLink, oStream);
			break;
		case MDB_QUERYCOUNT_REQ:
			ProcessQueryCount(pLink, oStream);
			break;
		case MDB_QUERYEXIST_REQ:
			ProcessQueryExist(pLink, oStream);
			break;
		}
	}
}

bool CMdbSvrModule::ReadString(CMemoryStream &oStream, CString *pStr)
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

void CMdbSvrModule::WriteString(CMemoryStream &oStream, const char* sStr)
{
	uint32 nSize = CString::StringLength(sStr);
	oStream.Write(nSize);
	if(nSize)
		oStream.Write((void*)sStr, nSize);
}

static bool GetAccess(const char* sAccessInfo, uint32 &nAccessMode, uint64 &nHostId, CString &oUser, CString &oPasswd)
{
	uint32 nLen = CString::StringLength(sAccessInfo);
	char* sInfo = Base64Decode(sAccessInfo, nLen, &nLen);
	if(!sInfo)
		return false;
	//????, MODE, HOSTID, USER, PASSWD
	for(uint32 i=0; i<nLen; ++i)
		sInfo[i] = (char)(((uint8)sInfo[i]) ^ ((uint8)'J'));//Jacky's First Char
	CFormatString oInfo(sInfo, nLen);
	CMalloc::Free(sInfo);

	int32 nRet = oInfo.Scan("%*s%u32%i64%s%s", &nAccessMode, &nHostId, &oUser, &oPasswd);
	if(nRet < 1)
		return false;
	if(nAccessMode==2 && nRet < 4)
		return false;

	return true;
}

static bool CheckAccess(CString &oDbName, uint32 nAccessMode, uint64 nHostId, CString &oUser, CString &oPasswd)
{
	uint32 nCount;
	if(nAccessMode == 1)
		return true;
	CMdb* pDb = CMdb::GetMdb(MDB_SYSDB_NAME);
	if(!pDb)
		return true;
	CMdbAccess* pAccess = pDb->QueryAccess("AccessControl");
	if(!pAccess)
		return true;
	CMdbPara* pPara = pAccess->GetQueryPara()->AddParaSet()->AddPara();
	pPara->SetString(1, oDbName.GetStr());
	if(!nAccessMode)
	{
		nCount = 0;
		pAccess->Count(nCount);
		pAccess->Release();
		return (nCount == 0);
	}
	pPara->SetUInt64(0, nHostId);
	pPara->SetString(2, oUser.GetStr());
	pPara->SetString(3, oPasswd.GetStr());
	pAccess->Query(1, 0);
	nCount = pAccess->GetResultSet()->GetResultCount();
	pAccess->Release();
	return ( nCount!=0 );
}

void CMdbSvrModule::ProcessGetDbList(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId;
	if(oStream.Read(nSessionId))
	{
		uint64 nHostId;
		const char* sDbList;
		uint32 nAccessMode, nStatus=0, nSize;
		CString oAccessInfo, oUser, oPasswd, oDbName(MDB_SYSDB_NAME);
		nSize = oStream.GetPosition();
		if(!ReadString(oStream, &oAccessInfo))
			nStatus = MDB_PACKAGE_ERROR;
		else if(!GetAccess(oAccessInfo.GetStr(), nAccessMode, nHostId, oUser, oPasswd))
			nStatus = MDB_PACKAGE_ERROR;
		else if(!CheckAccess(oDbName, nAccessMode, nHostId, oUser, oPasswd))
			nStatus = MDB_FORBIT_ACCESS;
		else sDbList = CMdb::GetDbList();
		oStream.SetPosition(nSize);
		oStream.Write(nStatus);//nStatus
		if(!nStatus)
			WriteString(oStream, sDbList);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_GETDBLIST_RESP);
		pLink->Send(oStream);
		if(nStatus)
			pLink->Close();
	}
}

void CMdbSvrModule::ProcessGetTableList(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		CString oDbName;
		const char* sTableList = NULL;
		uint32 nStatus = 0;
		if(!ReadString(oStream, &oDbName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				uint64 nHostId;
				uint32 nAccessMode;
				CString oAccessInfo, oUser, oPasswd;
				if(!ReadString(oStream, &oAccessInfo))
					nStatus = MDB_PACKAGE_ERROR;
				else if(!GetAccess(oAccessInfo.GetStr(), nAccessMode, nHostId, oUser, oPasswd))
					nStatus = MDB_PACKAGE_ERROR;
				else if(!CheckAccess(oDbName, nAccessMode, nHostId, oUser, oPasswd))
					nStatus = MDB_FORBIT_ACCESS;
				else
					sTableList = pDb->GetTableList();
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_GETTABLELIST_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(!nStatus)
			WriteString(oStream, sTableList);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
		if(nStatus)
			pLink->Close();
	}
}

void CMdbSvrModule::ProcessGetIndexList(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		CString oDbName;
		const char* sIndexList = NULL;
		uint32 nStatus = 0;
		if(!ReadString(oStream, &oDbName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
				sIndexList = pDb->GetIndexList();
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_GETINDEXLIST_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(!nStatus)
			WriteString(oStream, sIndexList);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessGetTableDef(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0;
		CString oDbName, oTabName;
		CMdbTableDef* pDef = NULL;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				pDef = pDb->GetTableDefine(oTabName.GetStr());
				if(pDef == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_GETTABLEDEF_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(!nStatus)
		{
			CMdbTableDefine* pDefine = (CMdbTableDefine*)pDef;
			pDefine->Write(oStream);
		}
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessGetIndexDef(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0;
		CString oDbName, oIdxName;
		CMdbIndexDef* pDef = NULL;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oIdxName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				pDef = pDb->GetIndexDefine(oIdxName.GetStr());
				if(pDef == NULL)
					nStatus = MDB_INDEX_NOTEXIST;
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_GETINDEXDEF_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(!nStatus)
		{
			CMdbIndexDefine* pDefine = (CMdbIndexDefine*)pDef;
			pDefine->Write(oStream);
		}
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessInsert(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0, nSize;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else if(!oDbName.Compare("sysdb", false))
				nStatus = MDB_FORBIT_ACCESS;
			else
			{
				CMdbTableAccess* pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					CMdbSqlPara* pPara = (CMdbSqlPara*)pAccess->GetInsertPara();
					if(!pPara->Read(oStream))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Insert();
					pAccess->Release();
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_INSERT_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessUpdate(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0, nCount=0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else if(!oDbName.Compare("sysdb", false))
				nStatus = MDB_FORBIT_ACCESS;
			else
			{
				CMdbTableAccess* pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
					CMdbSqlPara* pPara = (CMdbSqlPara*)pAccess->GetUpdatePara();
					if(!pCond->Read(oStream) || !pPara->Read(oStream))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Update(&nCount);
					pAccess->Release();
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_UPDATE_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		oStream.Write(nCount);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessDelete(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0, nCount=0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else if(!oDbName.Compare("sysdb", false))
				nStatus = MDB_FORBIT_ACCESS;
			else
			{
				CMdbTableAccess* pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
					if(!pCond->Read(oStream))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Delete(&nCount);
					pAccess->Release();
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_DELETE_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		oStream.Write(nCount);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessTruncate(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nStatus = 0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else if(!oDbName.Compare("sysdb", false))
				nStatus = MDB_FORBIT_ACCESS;
			else
			{
				CMdbTableAccess* pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
					nStatus = pAccess->Truncate();
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_TRUNCATE_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessQuery(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		CMdbTableAccess* pAccess = NULL;
		uint32 nStatus = 0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					uint32 nPageSize, nSkipCount;
					CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
					CMdbSqlFilter* pFilter = (CMdbSqlFilter*)pAccess->GetResultFilter();
					if(!pCond->Read(oStream) || !pFilter->Read(oStream) || !oStream.Read(nPageSize) || !oStream.Read(nSkipCount))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Query(nPageSize, nSkipCount);
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_QUERY_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(pAccess)
		{
			if(!nStatus)
				pAccess->m_oGetAttr.Write(oStream, pAccess->m_oFilter);
			pAccess->Release();
		}
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessQueryCount(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 nCount;
		CMdbTableAccess* pAccess = NULL;
		uint32 nStatus = 0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
					if(!pCond->Read(oStream))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Count(nCount);
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_QUERYCOUNT_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(pAccess)
		{
			if(!nStatus)
				oStream.Write(nCount);
			pAccess->Release();
		}
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

void CMdbSvrModule::ProcessQueryExist(CAcmTcpLink* pLink, CMemoryStream &oStream)
{
	uint32 nSessionId, nSize;
	if(oStream.Read(nSessionId))
	{
		uint32 bExist = 0;
		CMdbTableAccess* pAccess = NULL;
		uint32 nStatus = 0;
		CString oDbName, oTabName;
		if(!ReadString(oStream, &oDbName) || !ReadString(oStream, &oTabName))
			nStatus = MDB_PACKAGE_ERROR;
		else
		{
			CMdb* pDb = CMdb::GetMdb(oDbName.GetStr());
			if(pDb == NULL)
				nStatus = MDB_DB_NOTEXIST;
			else
			{
				pAccess = (CMdbTableAccess*)pDb->QueryAccess(oTabName.GetStr());
				if(pAccess == NULL)
					nStatus = MDB_TABLE_NOTEXIST;
				else
				{
					CMdbSqlParaSet* pCond = (CMdbSqlParaSet*)pAccess->GetQueryPara();
					if(!pCond->Read(oStream))
						nStatus = MDB_PACKAGE_ERROR;
					else
						nStatus = pAccess->Exist(bExist);
				}
			}
		}
		oStream.SetPosition(12);
		oStream.Write((uint32)MDB_QUERYEXIST_RESP);
		oStream.SetPosition(20);
		oStream.Write(nStatus);
		if(pAccess)
		{
			if(!nStatus)
				oStream.Write(bExist);
			pAccess->Release();
		}
		nSize = oStream.GetPosition();
		oStream.Truncate();
		oStream.SetPosition(0);
		oStream.Write(nSize);
		pLink->Send(oStream);
	}
}

FOCP_END();
