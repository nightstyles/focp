
#include "AFC.hpp"

FOCP_BEGIN();

class CFiberService: public CService
{
private:
	CService* m_pTimerService;

public:
	CFiberService()
	{
		m_pTimerService = NULL;
	}

	virtual const char* GetServiceName()
	{
		return "FiberService";
	}

protected:
	virtual bool OnInitialize()
	{
		uint32 nLength;
		const char* sVal;
		CTextAccess oAccess;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		uint32 nFiberIdBits=32;
		uint32 nFiberType, nFiberCount;
		int32 nGroupCount=1, nWorkerCount;
		m_pTimerService = CServiceManager::GetInstance()->QueryService("TimerService");
		if(m_pTimerService == NULL)
		{
			FocpLog(FOCP_LOG_ERROR, ("Missing the service 'TimerService'"));
			return false;
		}
		if(pConfigSystem->OpenConfig(oAccess, "FiberService", true))
		{
			oAccess.OpenIdxVal();
			if(oAccess.Query())
			{
				sVal = oAccess.GetVal("FiberIdBits", nLength);
				if(sVal)
				{
					if(sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'FiberService.FiberIdBits' is invalid"));
						return false;
					}
					nFiberIdBits = CString::Atoi(sVal);
					if(nFiberIdBits > 32 || nFiberIdBits == 0)
						nFiberIdBits = 32;
				}
				sVal = oAccess.GetVal("GroupCount", nLength);
				if(sVal)
				{
					if(sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'FiberService.WorkerCount' is invalid"));
						return false;
					}
					nGroupCount = CString::Atoi(sVal);
					if(nGroupCount <= 0)
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'FiberService.WorkerCount' is invalid"));
						return false;
					}
				}
			}
		}
		if(pConfigSystem->OpenConfig(oAccess, "FiberConfig", true))
		{
			CFactory* pFactory = CFiberManager::GetFactory();
			oAccess.OpenIdxVal();
			while(oAccess.Query())
			{
				sVal = oAccess.GetVal("FiberType", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberConfig.FiberType' is invalid"));
					return false;
				}
				nFiberType = CString::Atoi(sVal);
				sVal = oAccess.GetVal("FiberCount", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberConfig.FiberCount of FiberType[%u]' is invalid", nFiberType));
					return false;
				}
				nFiberCount = CString::Atoi(sVal);
				if(!nFiberCount)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberConfig.FiberCount of FiberType[%u]' is invalid", nFiberType));
					return false;
				}
				CAllocatePolicy oPolicy = {nFiberCount,nFiberCount,nFiberCount};
				if(!pFactory->SetAllocatePolicy(nFiberType, oPolicy))
				{
					FocpLog(FOCP_LOG_ERROR, ("SetFiberCapacity(FiberType=%u, FiberCount=%u)' failure", nFiberType, nFiberCount));
					return false;
				}
			}
		}
		pFiberManager->Initialize(nFiberIdBits, nGroupCount);
		if(pConfigSystem->OpenConfig(oAccess, "FiberWorker", true))
		{
			int32 nGroupIdx;
			oAccess.OpenIdxVal();
			while(oAccess.Query())
			{
				sVal = oAccess.GetVal("GroupIdx", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberWorker.GroupIdx' is invalid"));
					return false;
				}
				nGroupIdx = CString::Atoi(sVal);
				if(nGroupIdx < 0 || nGroupIdx >= nGroupCount)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberWorker.GroupIdx' is invalid"));
					return false;
				}
				sVal = oAccess.GetVal("WorkerCount", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberWorker.WorkerCount' is invalid"));
					return false;
				}
				nWorkerCount = CString::Atoi(sVal);
				if(nWorkerCount <= 0)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'FiberWorker.WorkerCount' is invalid"));
					return false;
				}
				pFiberManager->SetWorkerCount(nGroupIdx, nWorkerCount);
			}
		}

		return true;
	}

	virtual bool OnStart()
	{
		CFiberManager* pFiberManager = CFiberManager::GetInstance();
		if(!Wait(m_pTimerService, FOCP_SERVICE_STARTED))
			return false;
		return pFiberManager->Start();
	}

	virtual void OnStop()
	{
		CFiberManager::GetInstance()->Stop();
	}

	virtual void OnCleanup()
	{
		CFiberManager::GetInstance()->Cleanup();
	}
};

static CFiberService g_oFiberService;

FOCP_END();
