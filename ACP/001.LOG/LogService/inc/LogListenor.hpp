
#include "LogChooser.hpp"

#ifndef _AFW_LOG_LISTENOR_HPP_
#define _AFW_LOG_LISTENOR_HPP_

FOCP_BEGIN();

class CLogListenor;

class CLogProcessor: public CLogChooser
{
	friend class CLogListenor;
private:
	CLogProcessor *m_pPrve, *m_pNext;
	CLogListenor* m_pListenor;

public:
	CLogProcessor();
	virtual ~CLogProcessor();

	void Register();
	void DeRegister();

	bool Registered();

	virtual void Process(CLogMsg& oLog) = 0;
};

class CLogListenor: public CCooperateFunction
{
	friend class CLogProcessor;
private:
	CMutex m_oProcMutex;
	CBaseDoubleList<CLogProcessor> m_oList;
	CFile m_oFile;

	CMutex m_oQueueMutex;
	CEvent m_oEvent;
	CSingleList<CLogMsg*> m_oLogQueue;
	CCooperator m_oRecvThread, m_oProcThread;

public:
	CLogListenor();
	virtual ~CLogListenor();

	static CLogListenor* GetInstance();

	bool Initialize();
	void Cleanup();

	void Start();
	void Stop(bool bBlock=true);

protected:
	void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private:
	void Register(CLogProcessor* pProcessor);
	void DeRegister(CLogProcessor* pProcessor);
	void ProcessLog(CLogMsg& oLog);
};

FOCP_END();

#endif
