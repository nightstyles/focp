
#include "VmmApi.hpp"

#ifndef _VMM_CONTROL_HPP_
#define _VMM_CONTROL_HPP_

FOCP_BEGIN();

class CVmmControl
{
private:
	CMutex m_oMutex;
	uint32 m_nStatusNum;
	uint8 * m_pStatus;
	CFile m_nControlFile;

public:
	CVmmControl();
	~CVmmControl();

	int32 Startup(const char* sControlFile, uint32 nStatusNum);

	uint8 GetStatus(uint32 nIdx);
	void SetStatus(uint32 nIdx, uint8 nStatus);
};

FOCP_END();

#endif
