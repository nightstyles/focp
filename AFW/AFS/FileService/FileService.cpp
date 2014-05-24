
#include "AFC.hpp"

FOCP_BEGIN();

class CFileService: public CService
{
private:
	CThreadPool m_oCooperatorPool;
	CIocpServer* m_pIocpServer;

public:
	CFileService()
	{
		m_pIocpServer = NULL;
	}

	virtual ~CFileService()
	{
	}

	virtual const char* GetServiceName()
	{
		return "FileService";
	}

protected:
	virtual bool OnInitialize()
	{
		uint32 nLength;
		const char* sVal;
		CTextAccess oAccess;
		uint32 nWorkerCount=5, nMaxLink=100;
		m_pIocpServer = CIocpServer::GetInstance();
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
		if(pConfigSystem->OpenConfig(oAccess, "FileService", true))
		{
			oAccess.OpenIdxVal();
			if(oAccess.Query())
			{
				sVal = oAccess.GetVal("WorkerCount", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FileService.WorkerCount' is invalid"));
					return false;
				}
				nWorkerCount = CString::Atoi(sVal);
				if(nWorkerCount == 0)
					nWorkerCount = 5;
				sVal = oAccess.GetVal("MaxLink", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FileService.MaxLink' is invalid"));
					return false;
				}
				nMaxLink = CString::Atoi(sVal);
				if(nMaxLink >= 65536)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FileService.MaxLink(%s)' is invalid", sVal));
					return false;
				}
			}
		}
		if(!m_pIocpServer->Initialize(nMaxLink))
			return false;
		m_oCooperatorPool.Initialzie(nWorkerCount, m_pIocpServer);
		return true;
	}

	virtual bool OnStart()
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		m_pIocpServer->Start();
#endif
		m_oCooperatorPool.Start();
		return true;
	}

	virtual void OnStop()
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		m_pIocpServer->Stop(false);
#endif
		m_oCooperatorPool.Stop();
#if defined(WINDOWS) || defined(CYGWIN_NT)
		m_pIocpServer->Stop(true);
#endif
	}

	virtual void OnCleanup()
	{
		m_oCooperatorPool.Cleanup();
		if(m_pIocpServer)
			m_pIocpServer->Cleanup();
	}
};

static CFileService g_oFileService;

FOCP_END();
