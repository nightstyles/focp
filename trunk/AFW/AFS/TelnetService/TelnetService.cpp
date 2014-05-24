
#include "AFC.hpp"

FOCP_BEGIN();

class CTelnetService: public CService
{
private:
	CAcmTelnetServer m_oTelnetServer;

public:
	CTelnetService()
	{
	}

	virtual const char* GetServiceName()
	{
		return "TelnetService";
	}

protected:
	virtual bool OnInitialize()
	{
		int32 nPort;
		uint32 nLength;
		const char* sVal;
		CTextAccess oAccess;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
		if(!pConfigSystem->OpenConfig(oAccess, "TelnetService"))
			return false;
		oAccess.OpenIdxVal();
		if(oAccess.Query())
		{
			sVal = oAccess.GetVal("ListenPort", nLength);
			if(sVal == NULL)
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'TelnetService.ListenPort' is invalid"));
				return false;
			}
			if(sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'TelnetService.ListenPort' is invalid"));
				return false;
			}
			nPort = CString::Atoi(sVal);
			if(nPort > 65535 || nPort <= 0)
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'TelnetService.ListenPort' is invalid"));
				return false;
			}
		}
		CService* pFiberService = CServiceManager::GetInstance()->QueryService("FiberService");
		if(pFiberService == NULL)
		{
			FocpLog(FOCP_LOG_ERROR, ("Missing the service 'FiberService'"));
			return false;
		}
		if(!m_oTelnetServer.Initialize((uint16)nPort))
			return false;
		return true;
	}

	virtual bool OnStart()
	{
		m_oTelnetServer.Start();
		return true;
	}

	virtual void OnStop()
	{
		m_oTelnetServer.Stop();
	}

	virtual void OnCleanup()
	{
		m_oTelnetServer.Cleanup();
	}
};

static CTelnetService g_oTelnetService;

FOCP_END();
