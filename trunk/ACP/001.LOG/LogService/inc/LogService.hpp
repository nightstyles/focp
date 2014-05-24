
#include "LogMonitor.hpp"
#include "LogForward.hpp"
#include "LogArchiver.hpp"

#ifndef _AFW_LOG_SERVICE_HPP_
#define _AFW_LOG_SERVICE_HPP_

FOCP_BEGIN();

class CLogService: public CService
{
public:
	CLogService();
	virtual ~CLogService();
	virtual const char* GetServiceName();

protected:
	virtual bool OnInitialize();
	virtual bool OnStart();
	virtual void OnStop();
	virtual void OnCleanup();
};

FOCP_END();

#endif
