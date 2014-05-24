
#include "MdbApi.hpp"

#ifndef _MDB_HPP_
#define _MDB_HPP_

FOCP_BEGIN();

class CMdbApplication: public CServiceApplication
{
public:
	CMdbApplication();
	virtual ~CMdbApplication();

protected:
	virtual bool StartServiceBefore();//启动服务前
	virtual bool StartServiceAfter();//启动服务后
	virtual void StopServiceBefore();//停止服务前
	virtual void StopServiceAfter();//停止服务后
};

FOCP_END();

#endif
