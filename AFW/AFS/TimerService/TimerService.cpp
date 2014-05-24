
#include "AFC.hpp"

FOCP_BEGIN();

class CTimerService: public CService
{
private:
	CCooperator* m_pCooperator;
	CTimer* m_pTimer;

public:
	CTimerService()
	{
		m_pCooperator = NULL;
		m_pTimer = NULL;
	}

	virtual ~CTimerService()
	{
		if(m_pCooperator)
		{
			delete m_pCooperator;
			m_pCooperator = NULL;
		}
	}

	virtual const char* GetServiceName()
	{
		return "TimerService";
	}

protected:
	virtual bool OnInitialize()
	{
		uint32 nLength;
		const char* sVal;
		CTextAccess oAccess;
		uint32 nTimerCapacity=1024;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
		m_pTimer = CTimer::GetInstance();
		m_pCooperator = new CCooperator(m_pTimer);
		if(pConfigSystem->OpenConfig(oAccess, "TimerService", true))
		{
			oAccess.OpenIdxVal();
			if(oAccess.Query())
			{
				sVal = oAccess.GetVal("TimerCapacity", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'TimerService.TimerCapacity' is invalid"));
					return false;
				}
				nTimerCapacity = CString::Atoi(sVal);
				if(nTimerCapacity == 0)
					nTimerCapacity = 1024;
			}
		}

		return m_pTimer->Initialize(nTimerCapacity);
	}

	virtual bool OnStart()
	{
		if(m_pTimer == NULL)
			return true;
		m_pCooperator->Start();
		return true;
	}

	virtual void OnStop()
	{
		if(m_pCooperator)
			m_pCooperator->Stop();
	}

	virtual void OnCleanup()
	{
		if(m_pTimer)
			m_pTimer->Cleanup();
	}
};

static CTimerService g_oTimeService;

FOCP_END();
