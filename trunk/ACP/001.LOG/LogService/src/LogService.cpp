
#include "LogService.hpp"

FOCP_BEGIN();

CLogService::CLogService()
{
}

CLogService::~CLogService()
{
}

const char* CLogService::GetServiceName()
{
	return "LogService";
}

bool CLogService::OnInitialize()
{
	uint32 nLength;
	const char* sVal;
	CTextAccess oAccess;
	CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();

	CLogMonitor::RegisterCmd();
	CLogForwarder::RegisterCmd();
	CLogArchiver::RegisterCmd();

	CFormatString oScript;

	CLogListenor* pListenor = CLogListenor::GetInstance();
	if(!pListenor->Initialize())
		return false;

	if(pConfigSystem->OpenConfig(oAccess, "LogForward", true))
	{
		oAccess.OpenIdxVal();
		while(oAccess.Query())
		{
			CIpAddr oIpAddr;
			oIpAddr.nPort = 0;
			CString oServerAddr;
			sVal = oAccess.GetVal("ServerAddr", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogForward.ServerAddr' is invalid"));
				return false;
			}
			oIpAddr.nAddr = CFile::GetIpAddr(sVal);
			if(oIpAddr.nAddr==0 || oIpAddr.nAddr==(uint32)(-1))
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogForward.ServerAddr' is invalid"));
				return false;
			}
			CFile::GetIpFileName(oIpAddr, oServerAddr);
			sVal = oAccess.GetVal("ServerPort", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogForward.ServerPort' is invalid"));
				return false;
			}
			uint32 nPort = 0;
			if(sVal)
				nPort = CString::Atoi(sVal);
			oScript.Print("/Log/Forward/Add %s %u\n", oServerAddr.GetStr(), nPort);
			sVal = oAccess.GetVal("ChoiceRule", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogForward.ChoiceRule' is invalid"));
				return false;
			}
			if(sVal)
				oScript.Print("/Log/Forward/Push %s %s\n", oServerAddr.GetStr(), sVal);
		}
	}

	if(pConfigSystem->OpenConfig(oAccess, "LogArchive", true))
	{
		oAccess.OpenIdxVal();
		while(oAccess.Query())
		{
			//LogName, LogDir, MaxLogNo, LogSize, NeedArch, ArchDir, MaxArchNo, ChoiceRule, Filter
			CString oLogName, oLogDir, oArchDir, oChiceRule, oFilter;
			uint32 nMaxLogNo=FOCP_LOG_FILENO, nLogSize=FOCP_LOG_MAXFILE, nNeedArch=0, nMaxArchNo=FOCP_LOG_FILENO;
			sVal = oAccess.GetVal("LogName", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.LogName' is invalid"));
				return false;
			}
			oLogName=sVal;
			if(!oLogName.IsIdentifierOfC())
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.LogName' is invalid"));
				return false;
			}
			sVal = oAccess.GetVal("LogDir", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.LogDir' is invalid"));
				return false;
			}
			oLogDir = sVal;
			sVal = oAccess.GetVal("MaxLogNo", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.MaxLogNo' is invalid"));
				return false;
			}
			if(sVal)
				nMaxLogNo = CString::Atoi(sVal);
			sVal = oAccess.GetVal("LogSize", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.LogSize' is invalid"));
				return false;
			}
			if(sVal)
				nLogSize = CString::Atoi(sVal);
			sVal = oAccess.GetVal("NeedArch", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.NeedArch' is invalid"));
				return false;
			}
			if(sVal)
				nNeedArch = CString::Atoi(sVal);
			sVal = oAccess.GetVal("ArchDir", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.ArchDir' is invalid"));
				return false;
			}
			oArchDir = sVal;
			sVal = oAccess.GetVal("MaxArchNo", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.MaxArchNo' is invalid"));
				return false;
			}
			if(sVal)
				nMaxArchNo = CString::Atoi(sVal);
			sVal = oAccess.GetVal("ChoiceRule", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.ChoiceRule' is invalid"));
				return false;
			}
			oChiceRule = sVal;
			sVal = oAccess.GetVal("Filter", nLength);
			if(sVal && sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'LogArchive.Filter' is invalid"));
				return false;
			}
			oFilter = sVal;
			oScript.Print("/Log/Archive/Add LogName='%s'", oLogName.GetStr());
			if(!oLogDir.Empty())
				oScript.Print(",LogDir='%s'", oLogDir.GetStr());
			if(nMaxLogNo != FOCP_LOG_FILENO)
				oScript.Print(",MaxLogNo=%u", nMaxLogNo);
			if(nLogSize != FOCP_LOG_MAXFILE)
				oScript.Print(",LogSize=%u", nLogSize);
			if(nNeedArch)
			{
				oScript.Print(",NeedArch=1");
				if(!oArchDir.Empty())
					oScript.Print(",ArchDir='%s'", oArchDir.GetStr());
				if(nMaxArchNo != FOCP_LOG_FILENO)
					oScript.Print(",MaxArchNo=%u", nMaxArchNo);
			}
			oScript.Print("\n");
			if(!oChiceRule.Empty())
				oScript.Print("/Log/Archive/Push %s %s\n", oLogName.GetStr(), oChiceRule.GetStr());
			if(!oFilter.Empty())
				oScript.Print("/Log/Archive/Select %s %s\n", oLogName.GetStr(), oFilter.GetStr());
		}
	}
	if(!oScript.Empty())
	{
		CCmdSession oCmd;
		oCmd.ProcessText(oScript.GetStr());
	}
	return true;
}

bool CLogService::OnStart()
{
	CForwarderTable::GetInstance()->StartAll();
	CArchiverTable::GetInstance()->StartAll();
	CLogListenor::GetInstance()->Start();
	return true;
}

void CLogService::OnStop()
{
	CLogListenor::GetInstance()->Stop();
}

void CLogService::OnCleanup()
{
	CForwarderTable::GetInstance()->Clear();
	CArchiverTable::GetInstance()->Clear();
	CLogListenor::GetInstance()->Cleanup();
}

static CLogService g_oLogService;

FOCP_END();
