
#include "SqlApi.hpp"
#include "RdbApi.hpp"
#include "VmmApi.hpp"

#include <time.h>

FOCP_BEGIN();

static uint32 g_nEachBackupDay = 0;
static uint32 g_nSaveHour[24];
static uint32 g_nSaveMinute[60];
static uint32 nSwapSize = 32;
static uint32 nAutoIncPageSize = 16;
static uint32 nRedoLogFileSize = 50;
static uint32 nRedoLogFileNum = 4;
static char* sParentPath;
static char sControlPath[FOCP_MAX_PATH];
static char sDataPath[FOCP_MAX_PATH], sDataBakPath[FOCP_MAX_PATH], sDataBakPath2[FOCP_MAX_PATH];
static char sRdoPath[FOCP_MAX_PATH];
static char sControlFile[FOCP_MAX_PATH];
static char sShellPath[FOCP_MAX_PATH];

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
	return pSession->ReadLine();
}

static void RdbSqlCmdFunc(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CSqlTerminal oSqlTerm = {pSession, SqlPuts, SqlGets, GetSqlBufferSize};
	void* pSqlEnv = CreateSqlEnv(&oSqlTerm, sDataBakPath, sDataBakPath2);
	pSession->Print("Welcome to the rdb sql environment.\r\n"
					"You can use the command 'help;' to get the sql usage information.\r\n\r\n"); 
	CCmdSession::SetSession(pSession);
	ExecSql(pSqlEnv);
	DestroySqlEnv(pSqlEnv);
	CCmdSession::SetSession(NULL);
}

class CRdbService: public CService, public CCooperateFunction
{
private:
	CCooperator m_oBackupThread;

private:
	bool ConfigIsNormal()
	{
		bool bDirectory;
		char sFileName[FOCP_MAX_PATH];
		StringPrint(sFileName, "%s/vmm.cfg.bak", sControlPath);
		if(CDiskFileSystem::GetInstance()->AccessPath(sFileName, FOCP_FILEACCESS_EXIST, bDirectory) && !bDirectory)
			return true;
		return false;
	}

public:
	CRdbService():m_oBackupThread(this)
	{
		g_nEachBackupDay = 0;
		CBinary::MemorySet(g_nSaveHour, 0, sizeof(g_nSaveHour));
		CBinary::MemorySet(g_nSaveMinute, 0, sizeof(g_nSaveMinute));
	}

	virtual const char* GetServiceName()
	{
		return "RdbService";
	}

protected:
	virtual bool OnInitialize()
	{
		CString::StringCopy(sShellPath, CFilePathInfo::GetInstance()->GetDir());
		CDiskFileSystem::GetInstance()->GetOsPathName(sShellPath);

		sParentPath = (char*)CFilePathInfo::GetInstance()->GetHome();

		// configure file
		StringPrint(sControlPath, "%s/dbf/ctrl", sParentPath);
		StringPrint(sDataPath, "%s/dbf/data", sParentPath);
		StringPrint(sDataBakPath, "%s/bak/data", sParentPath);
		StringPrint(sDataBakPath2, "%s/bak/data2", sParentPath);
		StringPrint(sRdoPath, "%s/bak/rdo", sParentPath);
		StringPrint(sControlFile, "%s/vmm.ctr", sControlPath);

		CTextAccess oAccess;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
		if(pConfigSystem->OpenConfig(oAccess, "RdbService"))
        {
            oAccess.OpenIdxVal();
            if(oAccess.Query())
            {
                uint32 nVal;
                uint32 nLength;
                const char* sVal;
                char * pShift;
                sVal = oAccess.GetVal("SwapSize", nLength);
                if(sVal == NULL)
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SwapSize' is invalid"));
                    return false;
                }
                if(sVal[nLength-1])
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SwapSize' is invalid"));
                    return false;
                }
                nSwapSize = CString::Atoi(sVal);
                if(nSwapSize < 32)
                    nSwapSize = 32;

                sVal = oAccess.GetVal("AutoExtendPage", nLength);
                if(sVal == NULL)
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.AutoExtendPage' is invalid"));
                    return false;
                }
                if(sVal[nLength-1])
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.AutoExtendPage' is invalid"));
                    return false;
                }
                nAutoIncPageSize = CString::Atoi(sVal);
                if(nAutoIncPageSize < 10)
                    nAutoIncPageSize = 10;

                sVal = oAccess.GetVal("RdoFileSize", nLength);
                if(sVal == NULL)
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.RdoFileSize' is invalid"));
                    return false;
                }
                if(sVal[nLength-1])
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.RdoFileSize' is invalid"));
                    return false;
                }
                nRedoLogFileSize = CString::Atoi(sVal);
                if(!nRedoLogFileSize)
                    nRedoLogFileSize = 10485760*2;

                sVal = oAccess.GetVal("RdoFileNum", nLength);
                if(sVal == NULL)
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.RdoFileNum' is invalid"));
                    return false;
                }
                if(sVal[nLength-1])
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.RdoFileNum' is invalid"));
                    return false;
                }
                nRedoLogFileNum = CString::Atoi(sVal);
                if(!nRedoLogFileNum)
                    nRedoLogFileNum = 4;

                sVal = oAccess.GetVal("EachDayBackup", nLength);
                if(sVal == NULL)
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.EachDayBackup' is invalid"));
                    return false;
                }
                if(sVal[nLength-1])
                {
                    FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.EachDayBackup' is invalid"));
                    return false;
                }
                g_nEachBackupDay = CString::Atoi(sVal);
                if(g_nEachBackupDay < 0)
                    g_nEachBackupDay = 0;
                else if(g_nEachBackupDay > 365)
                    g_nEachBackupDay = 360;

                if(g_nEachBackupDay)
                {
                    sVal = oAccess.GetVal("SaveHour", nLength);
                    if(sVal == NULL)
                    {
                        FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SaveHour' is invalid"));
                        return false;
                    }
                    if(sVal[nLength-1])
                    {
                        FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SaveHour' is invalid"));
                        return false;
                    }
                    pShift = (char*)sVal;
                    while(pShift)
                    {
                        if(StringScan(pShift, "%u", &nVal) != 1)
                            break;
                        if(nVal < 24)
                            g_nSaveHour[nVal] = 1;
                        pShift = CString::CharOfString(pShift, ',');
                        if(pShift)
                            pShift++;
                    }

                    sVal = oAccess.GetVal("SaveMinute", nLength);
                    if(sVal == NULL)
                    {
                        FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SaveMinute' is invalid"));
                        return false;
                    }
                    if(sVal[nLength-1])
                    {
                        FocpLog(FOCP_LOG_ERROR, ("The config 'RdbService.SaveMinute' is invalid"));
                        return false;
                    }
                    pShift = (char*)sVal;
                    while(pShift)
                    {
                        if(StringScan(pShift, "%u", &nVal) != 1)
                            break;
                        if(nVal < 60)
                            g_nSaveMinute[nVal] = 1;
                        pShift = CString::CharOfString(pShift, ',');
                        if(pShift)
                            pShift++;
                    }
                }
            }
        }
		char sCommand[FOCP_MAX_PATH];
	#ifdef WINDOWS
		StringPrint(sCommand, "%s/mkdir.exe -p %s", sShellPath, sControlPath);
	#else
		StringPrint(sCommand, "mkdir -p %s", sControlPath);
	#endif
		System(sCommand);
	#ifdef WINDOWS
		StringPrint(sCommand, "%s/mkdir.exe -p %s", sShellPath, sDataPath);
	#else
		StringPrint(sCommand, "mkdir -p %s", sDataPath);
	#endif
		System(sCommand);
	#ifdef WINDOWS
		StringPrint(sCommand, "%s/mkdir.exe -p %s", sShellPath, sRdoPath);
	#else
		StringPrint(sCommand, "mkdir -p %s", sRdoPath);
	#endif
		System(sCommand);
	#ifdef WINDOWS
		StringPrint(sCommand, "%s/mkdir.exe -p %s", sShellPath, sDataBakPath);
	#else
		StringPrint(sCommand, "mkdir -p %s", sDataBakPath);
	#endif
		System(sCommand);

		return true;
	}

	virtual bool OnStart()
	{
		int32 nRet;
		char sCommand[FOCP_MAX_PATH];
		CRdbSystem* pDbSystem = CRdbSystem::GetInstance();

		FocpLog(FOCP_LOG_SYSLOG, ("RDB is starting control system"));
		if(InitializeVmmControler(sControlFile, 2))
			return 0;
		FocpLog(FOCP_LOG_SYSLOG, ("RDB start control system success"));

		uint8 nDirtyStatus = GetVmmControlStatus(VMM_CONTROL_DIRTY);
		uint8 nBackupStatus = GetVmmControlStatus(VMM_CONTROL_BACKUP);
		bool nConfigStatus = ConfigIsNormal();
		const char* sBackupPath = sDataBakPath;
		bool bRedo = false;
		if(!nConfigStatus)
			bRedo = true;
		else if(!nBackupStatus)
		{
			if(nDirtyStatus)
				bRedo = true;
			else
				sBackupPath = NULL;
		}
		if(nBackupStatus)
		{
			bool bDirectory;
			if(!CDiskFileSystem::GetInstance()->AccessPath(sDataBakPath2, FOCP_FILEACCESS_EXIST, bDirectory))
				pDbSystem->Reset(sDataBakPath, sDataBakPath2);
			SetVmmControlStatus(VMM_CONTROL_BACKUP, 0);
		}
		else
		{
		#ifdef WINDOWS
			StringPrint(sCommand, "%s/rm.exe -rf %s", sShellPath, sDataBakPath2);
		#else
			StringPrint(sCommand, "rm -rf %s", sDataBakPath2);
		#endif
			System(sCommand);
		}

		nRet = StartupVmm(sControlPath, sDataPath, sBackupPath, nSwapSize, nAutoIncPageSize);
		if(nRet)
			return false;

		nRet = pDbSystem->Startup(sControlPath, sRdoPath, nRedoLogFileSize, nRedoLogFileNum, bRedo);
		if(nRet)
		{
			CleanupVmm();
			return false;
		}

		if(g_nEachBackupDay)
			m_oBackupThread.Start();

		CCmdSystem::GetInstance()->RegisterCmd("/rdb", "rdb<CR>:\r\n\t sql command line", RdbSqlCmdFunc);

		return true;
	}

	virtual void OnStop()
	{
		if(g_nEachBackupDay)
			m_oBackupThread.Stop();

		CRdbSystem::GetInstance()->Cleanup();

		CleanupVmm();
	}

	virtual void OnCleanup()
	{
	}

	virtual void MainProc(CCooperator* pThread, bool &bRunning)
	{
		bool to_dump = true;
		CRdbSystem* pDbSystem = CRdbSystem::GetInstance();
		while(bRunning)
		{
			time_t Reload_t;
			struct tm *Reload_pTm;

			CCooperator::Sleep(5000);

			time(&Reload_t);
	#ifdef WINDOWS
			Reload_pTm = localtime(&Reload_t);
	#else
			struct tm Reload_p;
			Reload_pTm = localtime_r(&Reload_t, &Reload_p);
	#endif
			if(Reload_pTm->tm_yday % g_nEachBackupDay != g_nEachBackupDay - 1 ||
				!g_nSaveHour[Reload_pTm->tm_hour] ||
				!g_nSaveMinute[Reload_pTm->tm_min])
				to_dump = true;
			else if(to_dump)
			{
				to_dump = false;
				pDbSystem->Backup(sDataBakPath, sDataBakPath2);
			}
		}
	}
};

static CRdbService g_oRdbService;

FOCP_END();
