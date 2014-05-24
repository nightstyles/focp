
#include "MdbApi.hpp"
#include "SqlApi.hpp"

FOCP_BEGIN();

//MDB.dll/so
typedef bool (*FCreateMemoryDataBase)(const char* sDbName);

//MdbCon.dll/so
typedef void* (*FCreateMdbClient)(const char* sDbList, const char* sServerAddr, uint16 nServerPort);
typedef void (*FDestroyMdbClient)(void* pMdbClient);
typedef uint32 (*FStartMdbClient)(void* pMdbClient);
typedef void (*FStopMdbClient)(void* pMdbClient, bool bBlock);

//MdbLsn.dll/so
typedef void* (*FCreateMdbServer)();
typedef void (*FDestroyMdbServer)(void* pMdbServer);

//MdbRep.dll/so
typedef bool (*FMdbTransfer)();
typedef bool (*FConfigMdbDomain)(const char* sDbName, uint32 nDomain);
typedef bool (*FCreateMdbReplicator)(const char* sDbName, const char* sTableName, const char* sTransferIdx, bool bTableReplicative, bool bWithout, const char* sFields, const char* sCond);
typedef bool (*FRegisterMdbTransfer)();
typedef void (*FCleanupMdbReplication)();

//MdbSTO.dll/so
typedef bool (*FInitializeMdbStorageAttr)(const char* sMdbName, const char* sMdbTabName,
		const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere, 
		const char* sStorageIdx, const char* sFieldList);
typedef bool (*FInitializeMdbDataSource)(const char* sMdbName, uint32 nDomain, bool bSupportStorage,
		const char* sDSN, const char* sUser, const char* sPasswd,
		const char* sEventDb, const char* sEventTable);
typedef void (*FCleanupMdbStorage)();
typedef bool (*FStartMdbStorage)();
typedef void (*FStopMdbStorage)(bool bBlock);
typedef void (*FSetStorageTrigger)(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger);
typedef void (*FSetCacheTrigger)(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger);

static bool GetOdbcSourceInfo(const char* sOdbcDsnInfo, CString& oDsn, CString &oUser, CString &oPasswd)
{
	uint32 nLen = CString::StringLength(sOdbcDsnInfo);
	char* sInfo = Base64Decode(sOdbcDsnInfo, nLen, &nLen);
	if(!sInfo)
		return false;
	//HOSTID, DSN, USER, PASSWD
	uint64 nHostId = 0;
	for(uint32 i=0; i<nLen; ++i)
		sInfo[i] = (char)(((uint8)sInfo[i]) ^ ((uint8)'J'));//Jacky's First Char
	CFormatString oInfo(sInfo, nLen);
	CMalloc::Free(sInfo);

	int32 nRet = oInfo.Scan("%i64%s%s%s", &nHostId, &oDsn, &oUser, &oPasswd);
	if(nRet != 4)
		return false;

	if(nHostId != CFile::GetHostId())
		return false;

	return true;
}

class CLocalMdbItf
{
private:
	CDynamicLibrary m_oLib;
	FCreateMemoryDataBase m_fCreateMemoryDataBase;

public:
	CLocalMdbItf()
		:m_oLib("MdbLoc")
	{
		m_fCreateMemoryDataBase = (FCreateMemoryDataBase)m_oLib.FindSymbol("CreateMemoryDataBase");
	}

	~CLocalMdbItf()
	{
	}

	bool Valid()
	{
		if(!m_oLib.Valid())
			return false;
		if(!m_fCreateMemoryDataBase)
			return false;
		return true;
	}

	bool CreateMemoryDataBase(const char* sDbName)
	{
		return m_fCreateMemoryDataBase(sDbName);
	}
};

class CRemoteMdbItf
{
private:
	CDynamicLibrary m_oLib;
	FCreateMdbClient m_fCreateMdbClient;
	FDestroyMdbClient m_fDestroyMdbClient;
	FStartMdbClient m_fStartMdbClient;
	FStopMdbClient m_fStopMdbClient;

public:
	CRemoteMdbItf()
		:m_oLib("MdbCon")
	{
		m_fCreateMdbClient = (FCreateMdbClient)m_oLib.FindSymbol("CreateMdbClient");
		m_fDestroyMdbClient = (FDestroyMdbClient)m_oLib.FindSymbol("DestroyMdbClient");
		m_fStartMdbClient = (FStartMdbClient)m_oLib.FindSymbol("StartMdbClient");
		m_fStopMdbClient = (FStopMdbClient)m_oLib.FindSymbol("StopMdbClient");
	}

	~CRemoteMdbItf()
	{
	}

	bool Valid()
	{
		if(!m_oLib.Valid())
			return false;
		if(!m_fCreateMdbClient || !m_fDestroyMdbClient || !m_fStartMdbClient || !m_fStopMdbClient)
			return false;
		return true;
	}

	void* CreateMdbClient(const char* sDbList, const char* sServerAddr, uint16 nServerPort)
	{
		return m_fCreateMdbClient(sDbList, sServerAddr, nServerPort);
	}

	void DestroyMdbClient(void* pMdbClient)
	{
		m_fDestroyMdbClient(pMdbClient);
	}

	uint32 StartMdbClient(void* pMdbClient)
	{
		return m_fStartMdbClient(pMdbClient);
	}

	void StopMdbClient(void* pMdbClient, bool bBlock)
	{
		m_fStopMdbClient(pMdbClient, bBlock);
	}
};

class CMdbServerItf
{
private:
	CDynamicLibrary m_oLib;
	FCreateMdbServer m_fCreateMdbServer;
	FDestroyMdbServer m_fDestroyMdbServer;

public:
	CMdbServerItf()
		:m_oLib("MdbLsn")
	{
		m_fCreateMdbServer = (FCreateMdbServer)m_oLib.FindSymbol("CreateMdbServer");
		m_fDestroyMdbServer = (FDestroyMdbServer)m_oLib.FindSymbol("DestroyMdbServer");
	}

	~CMdbServerItf()
	{
	}

	bool Valid()
	{
		if(!m_oLib.Valid())
			return false;
		if(!m_fCreateMdbServer || !m_fDestroyMdbServer)
			return false;
		return true;
	}

	void* CreateMdbServer()
	{
		return m_fCreateMdbServer();
	}

	void DestroyMdbServer(void* pMdbServer)
	{
		m_fDestroyMdbServer(pMdbServer);
	}
};

//MdbRep.dll/so
class CMdbRepItf
{
private:
	CDynamicLibrary m_oLib;
	FMdbTransfer m_fMdbTransfer;
	FConfigMdbDomain m_fConfigMdbDomain;
	FCreateMdbReplicator m_fCreateMdbReplicator;
	FRegisterMdbTransfer m_fRegisterMdbTransfer;
	FCleanupMdbReplication m_fCleanupMdbReplication;

public:
	CMdbRepItf()
		:m_oLib("MdbRep")
	{
		m_fMdbTransfer = (FMdbTransfer)m_oLib.FindSymbol("MdbTransfer");
		m_fConfigMdbDomain = (FConfigMdbDomain)m_oLib.FindSymbol("ConfigMdbDomain");
		m_fCreateMdbReplicator = (FCreateMdbReplicator)m_oLib.FindSymbol("CreateMdbReplicator");
		m_fRegisterMdbTransfer = (FRegisterMdbTransfer)m_oLib.FindSymbol("RegisterMdbTransfer");
		m_fCleanupMdbReplication = (FCleanupMdbReplication)m_oLib.FindSymbol("CleanupMdbReplication");
	}

	~CMdbRepItf()
	{
	}

	bool Valid()
	{
		if(!m_oLib.Valid())
			return false;
		if(!m_fMdbTransfer || !m_fConfigMdbDomain || !m_fCreateMdbReplicator || !m_fRegisterMdbTransfer || !m_fCleanupMdbReplication)
			return false;
		return true;
	}

	bool MdbTransfer()
	{
		return m_fMdbTransfer();
	}

	bool ConfigMdbDomain(const char* sDbName, uint32 nDomain)
	{
		return m_fConfigMdbDomain(sDbName, nDomain);
	}

	bool CreateMdbReplicator(const char* sDbName, const char* sTableName, const char* sTransferIdx, bool bTableReplicative,
							 bool bWithout, const char* sFields, const char* sCond)
	{
		return m_fCreateMdbReplicator(sDbName, sTableName, sTransferIdx, bTableReplicative, bWithout, sFields, sCond);
	}

	bool RegisterMdbTransfer()
	{
		return m_fRegisterMdbTransfer();
	}

	void CleanupMdbReplication()
	{
		m_fCleanupMdbReplication();
	}
};

class CMdbStoItf
{
private:
	CDynamicLibrary m_oLib;
	FInitializeMdbStorageAttr m_fInitializeMdbStorageAttr;
	FInitializeMdbDataSource m_fInitializeMdbDataSource;
	FCleanupMdbStorage m_fCleanupMdbStorage;
	FStartMdbStorage m_fStartMdbStorage;
	FStopMdbStorage m_fStopMdbStorage;
	FSetStorageTrigger m_fSetStorageTrigger;
	FSetCacheTrigger m_fSetCacheTrigger;

public:
	CMdbStoItf()
		:m_oLib("MdbSto")
	{
		m_fInitializeMdbStorageAttr = (FInitializeMdbStorageAttr)m_oLib.FindSymbol("InitializeMdbStorageAttr");
		m_fInitializeMdbDataSource = (FInitializeMdbDataSource)m_oLib.FindSymbol("InitializeMdbDataSource");
		m_fCleanupMdbStorage = (FCleanupMdbStorage)m_oLib.FindSymbol("CleanupMdbStorage");
		m_fStartMdbStorage = (FStartMdbStorage)m_oLib.FindSymbol("StartMdbStorage");
		m_fStopMdbStorage = (FStopMdbStorage)m_oLib.FindSymbol("StopMdbStorage");
		m_fSetStorageTrigger = (FSetStorageTrigger)m_oLib.FindSymbol("SetStorageTrigger");
		m_fSetCacheTrigger = (FSetCacheTrigger)m_oLib.FindSymbol("SetCacheTrigger");
	}

	~CMdbStoItf()
	{
	}

	bool Valid()
	{
		if(!m_oLib.Valid())
			return false;
		if(!m_fInitializeMdbStorageAttr || !m_fInitializeMdbDataSource || !m_fCleanupMdbStorage ||
				!m_fStartMdbStorage || !m_fStopMdbStorage)
			return false;
		return true;
	}

	bool InitializeMdbStorageAttr(const char* sMdbName, const char* sMdbTabName,
								  const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere, 
								  const char* sStorageIdx, const char* sFieldList)
	{
		return m_fInitializeMdbStorageAttr(sMdbName, sMdbTabName, sDbTabName, sLoadWhere, sStorageWhere, sCacheWhere, sStorageIdx, sFieldList);
	}

	bool InitializeMdbDataSource(const char* sMdbName, uint32 nDomain, bool bSupportStorage,
								 const char* sDSN, const char* sUser, const char* sPasswd,
								 const char* sEventDb=MDB_SYSDB_NAME, const char* sEventTable=NULL)
	{
		return m_fInitializeMdbDataSource(sMdbName, nDomain, bSupportStorage, sDSN, sUser, sPasswd, sEventDb, sEventTable);
	}

	void CleanupMdbStorage()
	{
		m_fCleanupMdbStorage();
	}

	bool StartMdbStorage()
	{
		return m_fStartMdbStorage();
	}

	void StopMdbStorage(bool bBlock=true)
	{
		m_fStopMdbStorage(bBlock);
	}

	void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger)
	{
		m_fSetStorageTrigger(sMdbName, sMdbTabName, StorageTrigger);
	}

	void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger)
	{
		m_fSetCacheTrigger(sMdbName, sMdbTabName, CacheTrigger);
	}
};

static void GetSqlBufferSize(void*, uint32 * pReadSize, uint32 * pWriteSize)
{
	pReadSize[0] = 128;
	pWriteSize[0] = 128;
}

static void SqlPuts(void* pTerminal, const char* sLine)
{
	CCmdSession* pSession = (CCmdSession*)pTerminal;
	pSession->Print("%s\r\n", sLine);
}

static const char* SqlGets(void* pTerminal)
{
	CCmdSession* pSession = (CCmdSession*)pTerminal;
	return pSession->ReadLine(true, '%');
}

static CString oMdbDsn("FORBID"), oMdbUser, oMdbPasswd;

static void GetMdbSecurityInfo()
{
	CMdb* pDb = CMdb::GetMdb(MDB_SYSDB_NAME);
	if(pDb)
	{
		//static CString oMdbDsn("FALSE"), oMdbUser, oMdbPasswd;
		CMdbAccess* pAccess = pDb->QueryAccess("MdbSecurity");
		if(pAccess)
		{
			CMdbPara* pPara = pAccess->GetQueryPara()->AddParaSet()->AddPara();
			pPara->SetNull(0);//MdbName
			if(!pAccess->Query(1, 0))
			{
				uint32 nLen1, nLen2;
				CMdbResult* pResult = pAccess->GetResultSet()->GetResult(0);
				if(pResult)
				{
					const char* sUser = pResult->GetString(1, &nLen1);//User
					const char* sPassword = pResult->GetString(2, &nLen2);//PassWord
					oMdbDsn = "TRUE";
					oMdbUser = CString(sUser, nLen1);
					oMdbPasswd = CString(sPassword, nLen2);
				}
			}
			else
				oMdbDsn = "FORBID";
			pAccess->Release();
		}
	}
}

static bool AllowSetMdbSecurityInfo()
{
	CMdb* pDb = CMdb::GetMdb(MDB_SYSDB_NAME);
	if(!pDb)
		return false;
	CMdbAccess* pAccess = pDb->QueryAccess("MdbSecurity");
	if(!pAccess)
		return false;
	pAccess->Release();
	return true;
}

static void InitializeMdbSecurityInfo(CString& oUser, CString& oPasswd)
{
	CMdb* pDb = CMdb::GetMdb(MDB_SYSDB_NAME);
	CMdbAccess* pAccess = pDb->QueryAccess("MdbSecurity");
	CMdbPara* pPara = pAccess->GetInsertPara();
	pPara->SetNull(0);//MdbName
	pPara->SetString(1, oUser.GetStr());
	pPara->SetString(2, oPasswd.GetStr());
	pAccess->Insert();
	pAccess->Release();
}

static bool bStarted = false;
static void MdbSqlCmdFunc(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	if(!bStarted)
		goto execute;
	pSession->ForbidSave(true);
loop:
	GetMdbSecurityInfo();
	if(!oMdbDsn.Compare("TRUE"))
	{
		if(!oMdbUser.Empty() && !oMdbPasswd.Empty())
		{
			pSession->Print("Check your MDB system security information for the '%s'.\r\n", CFilePathInfo::GetInstance()->GetName());
			pSession->Print("Please input your name(max 8 character):");
			pSession->SetMaxRead(8);
			const char* s = pSession->ReadLine(false);
			if(oMdbUser.Compare(s))
			{
				pSession->Print("Sorry, your name is invalid!\r\n");
				pSession->SetMaxRead(0);
				pSession->ForbidSave(false);
				return;
			}
			pSession->Print("Please input your password(max 8 character):");
			pSession->SetHidden(true);
			s = pSession->ReadLine(false);
			pSession->SetHidden(false);
			pSession->SetMaxRead(0);
			if(oMdbPasswd.Compare(s))
			{
				pSession->Print("Sorry, your password is invalid!\r\n");
				pSession->ForbidSave(false);
				return;
			}
		}
	}
	else if(!oMdbDsn.Compare("FORBID"))
	{
		if(!AllowSetMdbSecurityInfo())
		{
			pSession->Print("Sorry, this MDB system forbids telnet!\r\n");
			pSession->ForbidSave(false);
			return;
		}
		pSession->Print("Initialize your MDB system security information for '%s'.\r\n", CFilePathInfo::GetInstance()->GetName());
		pSession->Print("Please input your name(max 8 character):");
		pSession->SetMaxRead(8);
		const char* s = pSession->ReadLine(false);
		CString oUser(s);
		oUser.Trim();
		if(oUser.Empty() || oUser.GetSize() > 8)
		{
			pSession->Print("Sorry, your name is invalid!\r\n");
			pSession->SetMaxRead(0);
			pSession->ForbidSave(false);
			return;
		}
		pSession->Print("Please input your password(max 8 character):");
		pSession->SetHidden(true);
		s = pSession->ReadLine(false);
		pSession->SetHidden(false);
		pSession->SetMaxRead(0);
		CString oPasswd(s);
		oPasswd.Trim();
		if(oPasswd.Empty() || oPasswd.GetSize() > 8)
		{
			pSession->Print("Sorry, your password is invalid!\r\n");
			pSession->ForbidSave(false);
			return;
		}
		InitializeMdbSecurityInfo(oUser, oPasswd);
		goto loop;
	}
	pSession->ForbidSave(false);

execute:
	CHistoryCmd *pLocalCmd = new CHistoryCmd, *pOldCmd;
	CSqlTerminal oSqlTerm = {pSession, SqlPuts, SqlGets, GetSqlBufferSize};
	void* pSqlEnv = CreateSqlEnv(&oSqlTerm);
	pSession->Print("Welcome to the mdb sql environment.\r\n"
					"You can use the command 'help;' to get the sql usage information.\r\n\r\n");
	CCmdSession::SetSession(pSession);
	pOldCmd = pSession->GetHistoryCmd();
	pSession->SetHistoryCmd(pLocalCmd);
	ExecSql(pSqlEnv);
	pSession->SetHistoryCmd(pOldCmd);
	delete pLocalCmd;
	pSession->Print("Mdb say goodbye to you.\r\n\r\n");
	DestroySqlEnv(pSqlEnv);
	CCmdSession::SetSession(NULL);
}

class CMdbService: public CService
{
private:
	CLocalMdbItf* m_pMdbItf;
	CRemoteMdbItf* m_pRdbItf;
	CMdbServerItf* m_pSvrItf;
	CMdbRepItf* m_pRepItf;
	CMdbStoItf* m_pStoItf;
	void* m_pMdbServer;
	CRbTree<char*> m_oMdbClients;

public:
	CMdbService()
	{
		m_pMdbItf = NULL;
		m_pRdbItf = NULL;
		m_pSvrItf = NULL;
		m_pRepItf = NULL;
		m_pStoItf = NULL;
		m_pMdbServer = NULL;
	}

	virtual const char* GetServiceName()
	{
		return "MdbService";
	}

	void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger)
	{
		if(m_pStoItf)
			m_pStoItf->SetStorageTrigger(sMdbName, sMdbTabName, StorageTrigger);
	}

	void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger)
	{
		if(m_pStoItf)
			m_pStoItf->SetStorageTrigger(sMdbName, sMdbTabName, CacheTrigger);
	}

protected:
	virtual bool OnInitialize()
	{
		uint32 nLength;
		const char* sVal = NULL;
		CTextAccess oAccess;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();

		//Register telnet command line interface
		CCmdSystem::GetInstance()->RegisterCmd("/Mdb", "Mdb<CR>:\r\n\t sql command line", MdbSqlCmdFunc);

		//Create Local Memory Database
		uint32 nMdbSvr = 0;
		if(pConfigSystem->OpenConfig(oAccess, "MdbService", true))
		{
			oAccess.OpenIdxVal();
			if(oAccess.Query())
			{
				sVal = oAccess.GetVal("MdbSvr", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'MdbService.ServerPort' is invalid"));
					return false;
				}
				nMdbSvr = CString::Atoi(sVal);
				sVal = oAccess.GetVal("MdbList", nLength);
			}
		}

		bool bInitializeMdbDataSource = false;
		bool bInitializeMdbStorageAttr = false;
		bool bCreateMdbReplicator = false;
		bool bConfigMdbDomain = false;

		if(sVal && sVal[0])
		{
			bool bCreated = false;
			char*pShift, *pDbName = (char*)sVal;
			while(pDbName)
			{
				pShift = (char*)CString::CharOfString(pDbName, ',');
				if(pShift)
					pShift[0] = 0;

				if(m_pMdbItf == NULL)
				{
					m_pMdbItf = new CLocalMdbItf;
					if(!m_pMdbItf->Valid())
					{
						if(pShift)
							pShift[0] = ',';
						return false;
					}
				}
				if(!m_pMdbItf->CreateMemoryDataBase(pDbName))
				{
					if(pShift)
						pShift[0] = ',';
					return false;
				}
				bCreated = true;
				CCmdSession oSession;
				char sDbfScript[FOCP_MAX_PATH];
				const char* sHome = CFilePathInfo::GetInstance()->GetHome();
				StringPrint(sDbfScript, "%s/dbf/%s.Create.sql", sHome, pDbName);
				oSession.ProcessScript(sDbfScript);

				pDbName = pShift;
				if(pDbName)
				{
					pDbName[0] = ',';
					++pDbName;
				}
			}

			if(bCreated)//创建了本地数据库
			{
				if(pConfigSystem->OpenConfig(oAccess, "MdbStorage", true))
				{
					oAccess.OpenIdxVal();
					while(oAccess.Query())
					{
						sVal = oAccess.GetVal("MdbName", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.MdbName' is invalid"));
							return false;
						}
						CString oMdbName(sVal);
						sVal = oAccess.GetVal("CacheName", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.CacheName' is invalid"));
							return false;
						}
						CString oMdbTabName(sVal);
						sVal = oAccess.GetVal("TableName", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.TableName' is invalid"));
							return false;
						}
						CString oDbTabName(sVal?sVal:(char*)"");
						sVal = oAccess.GetVal("LoadWhere", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.LoadWhere' is invalid"));
							return false;
						}
						CString oLoadWhere(sVal?sVal:"");
						sVal = oAccess.GetVal("StorageWhere", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.StorageWhere' is invalid"));
							return false;
						}
						CString oStorageWhere(sVal?sVal:(char*)"");
						sVal = oAccess.GetVal("CacheWhere", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.CacheWhere' is invalid"));
							return false;
						}
						CString oCacheWhere(sVal?sVal:(char*)"");
						sVal = oAccess.GetVal("StorageIdx", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.StorageIdx' is invalid"));
							return false;
						}
						CString oStorageIdx(sVal);
						sVal = oAccess.GetVal("FieldList", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.FieldList' is invalid"));
							return false;
						}
						CString oFieldList(sVal?sVal:(char*)"");
						if(m_pStoItf == NULL)
						{
							m_pStoItf = new CMdbStoItf;
							if(!m_pStoItf->Valid())
								return false;
						}
						if(!m_pStoItf->InitializeMdbStorageAttr(oMdbName.GetStr(), oMdbTabName.GetStr(),
																oDbTabName.GetStr(), oLoadWhere.GetStr(), oStorageWhere.GetStr(), oCacheWhere.GetStr(), 
																oStorageIdx.GetStr(), oFieldList.GetStr()))
							return false;
						bInitializeMdbStorageAttr = true;
					}
				}

				if(pConfigSystem->OpenConfig(oAccess, "MdbReplication", true))
				{
					oAccess.OpenIdxVal();
					while(oAccess.Query())
					{
						sVal = oAccess.GetVal("MdbName", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbReplication.MdbName' is invalid"));
							return false;
						}
						CString oDbName(sVal);
						sVal = oAccess.GetVal("TableName", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbReplication.TableName' is invalid"));
							return false;
						}
						CString oTabName(sVal);
						sVal = oAccess.GetVal("ReplicateTable", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbReplication.ReplicateTable' is invalid"));
							return false;
						}
						uint32 bTableReplicative = CString::Atoi(sVal);
						sVal = oAccess.GetVal("WithoutFields", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbReplication.WithoutFields' is invalid"));
							return false;
						}
						uint32 bWithout = CString::Atoi(sVal);
						sVal = oAccess.GetVal("Fields", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.Fields' is invalid"));
							return false;
						}
						CString oFields(sVal?sVal:(char*)"");
						sVal = oAccess.GetVal("Where", nLength);
						if(sVal && sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbStorage.Where' is invalid"));
							return false;
						}
						CString oWhere(sVal?sVal:(char*)"");
						sVal = oAccess.GetVal("TransferIdx", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbReplication.TransferIdx' is invalid"));
							return false;
						}
						if(!m_pRepItf)
						{
							m_pRepItf = new CMdbRepItf;
							if(!m_pRepItf->Valid())
								return false;
						}
						if(!m_pRepItf->CreateMdbReplicator(oDbName.GetStr(), oTabName.GetStr(), sVal, bTableReplicative?true:false, bWithout?true:false, oFields.GetStr(), oWhere.GetStr()))
							return false;
						bCreateMdbReplicator = true;
					}
				}

				if( (bCreateMdbReplicator || bInitializeMdbStorageAttr) && pConfigSystem->OpenConfig(oAccess, "MdbDomain", true))
				{
					oAccess.OpenIdxVal();
					while(oAccess.Query())
					{
						sVal = oAccess.GetVal("MdbDomain", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.MdbDomain' is invalid"));
							return false;
						}
						uint32 nDomain = CString::Atoi(sVal);
						sVal = oAccess.GetVal("MdbName", nLength);
						if(!sVal || sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.MdbName' is invalid"));
							return false;
						}
						if(bCreateMdbReplicator)
						{
							if(!m_pRepItf->ConfigMdbDomain(sVal, nDomain))
								return false;
							bConfigMdbDomain = true;
						}
						if(bInitializeMdbStorageAttr)
						{
							CString oMdbName(sVal);
							sVal = oAccess.GetVal("OdbcSource", nLength);
							if(sVal && sVal[nLength-1])
							{
								FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.OdbcSource' is invalid"));
								return false;
							}
							if(!sVal)
								continue;
							CString oOdbcDsn, oOdbcUser, oOdbcPasswd;
							if(!GetOdbcSourceInfo(sVal, oOdbcDsn, oOdbcUser, oOdbcPasswd))
							{
								FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.OdbcSource' is invalid"));
								return false;
							}
							sVal = oAccess.GetVal("EventDb", nLength);
							if(sVal && sVal[nLength-1])
							{
								FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.EventDb' is invalid"));
								return false;
							}
							CString oEventDb(sVal?sVal:"");
							sVal = oAccess.GetVal("EventTable", nLength);
							if(sVal && sVal[nLength-1])
							{
								FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.EventTable' is invalid"));
								return false;
							}
							CString oEventTable(sVal?sVal:"");
							uint32 nSupportStorage = 0;
							sVal = oAccess.GetVal("SupportStorage", nLength);
							if(sVal && sVal[nLength-1])
							{
								FocpLog(FOCP_LOG_ERROR, ("The config 'MdbDomain.SupportStorage' is invalid"));
								return false;
							}
							if(sVal)
								nSupportStorage = CString::Atoi(sVal);
							if(!m_pStoItf->InitializeMdbDataSource(oMdbName.GetStr(), nDomain, (nSupportStorage!=0),
																   oOdbcDsn.GetStr(), oOdbcUser.GetStr(), oOdbcPasswd.GetStr(),
																   oEventDb.GetStr(), oEventTable.GetStr()))
								return false;
							bInitializeMdbDataSource = true;
						}
					}
					if(bConfigMdbDomain && !m_pRepItf->RegisterMdbTransfer())
						return false;
				}
			}
		}

		//Create Remote Memory Database
		if(pConfigSystem->OpenConfig(oAccess, "RemoteMdb", true))
		{
			CString oServerAddr;
			uint32 nServerPort;
			const char* sDbList;
			oAccess.OpenIdxVal();
			while(oAccess.Query())
			{
				sVal = oAccess.GetVal("ServerPort", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'RemoteMdb.ServerPort' is invalid"));
					return false;
				}
				nServerPort = CString::Atoi(sVal);
				if(!nServerPort || nServerPort > 65535)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'RemoteMdb.ServerPort' is invalid"));
					return false;
				}
				sVal = oAccess.GetVal("ServerAddr", nLength);
				if(!sVal || !sVal[0] || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'RemoteMdb.ServerAddr' is invalid"));
					return false;
				}
				oServerAddr = sVal;
				sVal = oAccess.GetVal("MdbList", nLength);
				if(sVal && sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'RemoteMdb.MdbList' is invalid"));
					return false;
				}
				sDbList = sVal;

				if(m_pRdbItf == NULL)
				{
					m_pRdbItf = new CRemoteMdbItf;
					if(!m_pRdbItf->Valid())
						return false;
				}

				void* pRdbList = m_pRdbItf->CreateMdbClient(sDbList, oServerAddr.GetStr(), (uint16)nServerPort);
				if(pRdbList == NULL)
					return false;
				m_oMdbClients.Insert((char*)pRdbList);
			}
		}

		// Create Mdb Server
		if(nMdbSvr)
		{
			CService* pService = CServiceManager::GetInstance()->QueryService("AcmService");
			if(!pService)
			{
				FocpLog(FOCP_LOG_ERROR, ("MdbService need AcmService for MdbSvrModule, but there isn't it"));
				return false;
			}
			if(!Wait(pService, FOCP_SERVICE_INITIALIZED))
				return false;
			if(!pService->HaveAbility(ACM_TCPSVR_ABILITY))
			{
				FocpLog(FOCP_LOG_ERROR, ("MdbService need AcmService's tcp server ability for MdbSvrModule, but there isn't it"));
				return false;
			}
			m_pSvrItf = new CMdbServerItf;
			if(!m_pSvrItf->Valid())
				return false;
		}

		if(!bConfigMdbDomain && m_pRepItf)
		{
			m_pRepItf->CleanupMdbReplication();
			delete m_pStoItf;
			m_pStoItf = NULL;
		}

		if(!bInitializeMdbDataSource && m_pStoItf)
		{
			m_pStoItf->CleanupMdbStorage();
			delete m_pStoItf;
			m_pStoItf = NULL;
		}

		const char* sCreateCode = GetEnvVar("CreateMdbCode");
		if(sCreateCode && sCreateCode[0] == '1')
			CMdb::CreateCppCode();

		return true;
	}

	virtual bool OnStart()
	{
		if(m_pRepItf)
		{
			CService* pService = CServiceManager::GetInstance()->QueryService("AcmService");
			if(!pService)
			{
				FocpLog(FOCP_LOG_ERROR, ("MdbService need AcmService for MdbSvrModule, but there isn't it"));
				return false;
			}
			if(!Wait(pService, FOCP_SERVICE_STARTED))
				return false;
			if(!m_pRepItf->MdbTransfer())
				return false;
		}
		if(m_pStoItf)
		{
			if(!m_pStoItf->StartMdbStorage())
				return false;
		}
		CRbTreeNode* pIt = m_oMdbClients.First();
		CRbTreeNode* pEnd = m_oMdbClients.End();
		for(; pIt != pEnd; pIt = m_oMdbClients.GetNext(pIt))
		{
			char* pRdb = m_oMdbClients.GetItem(pIt);
			if(!m_pRdbItf->StartMdbClient(pRdb))
				return false;
		}
		if(m_pSvrItf)
		{
			m_pMdbServer = m_pSvrItf->CreateMdbServer();
			if(!m_pMdbServer)
				return false;
		}
		bStarted = true;
		return true;
	}

	virtual void OnStop()
	{
		if(m_pMdbServer)
		{
			m_pSvrItf->DestroyMdbServer(m_pMdbServer);
			m_pMdbServer = NULL;
		}
		if(m_pRepItf)
			m_pRepItf->CleanupMdbReplication();
		if(m_pStoItf)
			m_pStoItf->StopMdbStorage();
		CRbTreeNode* pIt = m_oMdbClients.First();
		CRbTreeNode* pEnd = m_oMdbClients.End();
		for(; pIt != pEnd; pIt = m_oMdbClients.GetNext(pIt))
		{
			char* pRdb = m_oMdbClients.GetItem(pIt);
			m_pRdbItf->StopMdbClient(pRdb, true);
		}
	}

	virtual void OnCleanup()
	{
		CRbTreeNode* pIt = m_oMdbClients.First();
		CRbTreeNode* pEnd = m_oMdbClients.End();
		for(; pIt != pEnd; pIt = m_oMdbClients.GetNext(pIt))
		{
			char* pRdb = m_oMdbClients.GetItem(pIt);
			m_pRdbItf->DestroyMdbClient(pRdb);
		}
		m_oMdbClients.Clear();
		if(m_pStoItf)
			m_pStoItf->CleanupMdbStorage();
		CMdb::RemoveAll();
		if(m_pRepItf)
		{
			delete m_pRepItf;
			m_pRepItf = NULL;
		}
		if(m_pSvrItf)
		{
			delete m_pSvrItf;
			m_pSvrItf = NULL;
		}
		if(m_pStoItf)
		{
			delete m_pStoItf;
			m_pStoItf = NULL;
		}
		if(m_pRdbItf)
		{
			delete m_pRdbItf;
			m_pRdbItf = NULL;
		}
		if(m_pMdbItf)
		{
			delete m_pMdbItf;
			m_pMdbItf = NULL;
		}
	}
};

static CMdbService g_oMdbService;

FOCP_EXPORT void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger)
{
	g_oMdbService.SetStorageTrigger(sMdbName, sMdbTabName, StorageTrigger);
}

FOCP_EXPORT void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger)
{
	g_oMdbService.SetStorageTrigger(sMdbName, sMdbTabName, CacheTrigger);
}

FOCP_END();
