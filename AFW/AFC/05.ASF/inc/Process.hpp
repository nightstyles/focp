
#include "AsfDef.hpp"

#ifndef _Asf_Process_Hpp_
#define _Asf_Process_Hpp_

FOCP_BEGIN();

ASF_API void StartApp(int32 nArgc, char* sArgv[], bool bLoop=false, uint16 nVirtualPort=0);
ASF_API int32 System(const char* sCmd);

class ASF_API CPipeOpen: public CCooperateFunction
{
private:
	struct COutputMsg
	{
		uint16 bError, nSize, nOffset;
		char sBuf[1024];
	};
	CMutex m_oMutex;
	CEvent m_oEvent;
	CLoopList<COutputMsg> m_oOutputMsgList;
	CFile m_oFile;
	CCooperator m_oCooperator;
	bool m_bQuit, m_bThread;

public:
	CPipeOpen(bool bThread=true);
	virtual ~CPipeOpen();

	bool StartApp(int32 nArgc, char* sArgv[], uint16 nVirtualPort);
	//-1: end or break, 0: invalid argument or timeout(1 second);
	int32 ReadOutput(char* pBuf, uint16 nBufSize, bool &bError);
	//-1: end or break; 0: invalid argument
	int32 WriteInput(const char* pBuf);

protected:
	virtual void MainProc(CCooperator* pCooperator, bool &bRunning);
};

FOCP_END();

#endif
