
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
	virtual bool StartServiceBefore();//��������ǰ
	virtual bool StartServiceAfter();//���������
	virtual void StopServiceBefore();//ֹͣ����ǰ
	virtual void StopServiceAfter();//ֹͣ�����
};

FOCP_END();

#endif
