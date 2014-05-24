
#include "LogListenor.hpp"

FOCP_BEGIN();
CLogProcessor::CLogProcessor()
{
	m_pPrve = m_pNext = NULL;
	m_pListenor = NULL;
}

CLogProcessor::~CLogProcessor()
{
	DeRegister();
}

void CLogProcessor::Register()
{
	if(!m_pListenor)
	{
		m_pListenor = CLogListenor::GetInstance();
		m_pListenor->Register(this);
	}
}

void CLogProcessor::DeRegister()
{
	if(m_pListenor)
	{
		m_pListenor->DeRegister(this);
		m_pListenor = NULL;
		m_pPrve = m_pNext = NULL;
	}
}

bool CLogProcessor::Registered()
{
	return m_pListenor!=NULL;
}

CLogListenor::CLogListenor():
	m_oList(FocpFieldOffset(CLogProcessor,m_pPrve),FocpFieldOffset(CLogProcessor,m_pNext)),
	m_oRecvThread(this), m_oProcThread(this)
{
	m_oEvent.Reset();
}

CLogListenor::~CLogListenor()
{
}

CLogListenor* CLogListenor::GetInstance()
{
	return CSingleInstance<CLogListenor>::GetInstance();
}

void CLogListenor::ProcessLog(CLogMsg& oLog)
{
	m_oProcMutex.Enter();
	for(CLogProcessor* pIt = m_oList.First(); pIt; pIt=m_oList.GetNext(pIt))
	{
		if(pIt->Check(oLog))
			pIt->Process(oLog);
	}
	m_oProcMutex.Leave();
}

void CLogListenor::Register(CLogProcessor* pProcessor)
{
	m_oProcMutex.Enter();
	m_oList.Push(pProcessor);
	m_oProcMutex.Leave();
}

void CLogListenor::DeRegister(CLogProcessor* pProcessor)
{
	m_oProcMutex.Enter();
	m_oList.Remove(pProcessor);
	m_oProcMutex.Leave();
}

void CLogListenor::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
	if(pCooperator == &m_oRecvThread)
	{
		CIpAddr oIpAddr;
		char sLog[FOCP_LOG_MAXMSG];
		int32 nLen = m_oFile.ReadFrom(sLog, FOCP_LOG_MAXMSG, oIpAddr);
		if(nLen > 0)
		{
			CString oVal;
			char* pShift = sLog;
			CLogMsg* pLog = new CLogMsg;
			pLog->oHost = pShift;
			pShift += pLog->oHost.GetSize() + 1;
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			oVal = pShift;
			pShift += oVal.GetSize() + 1;
			pLog->nDate = CDateTime(oVal).GetValue();
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			oVal = pShift;
			pShift += oVal.GetSize() + 1;
			pLog->nLevel = CString::Atoi(oVal.GetStr());
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			pLog->oAppName = pShift;
			pShift += pLog->oAppName.GetSize() + 1;
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			oVal = pShift;
			pShift += oVal.GetSize() + 1;
			pLog->nDMN = CString::Atoi(oVal.GetStr());
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			oVal = pShift;
			pShift += oVal.GetSize() + 1;
			pLog->nAIN = CString::Atoi(oVal.GetStr());
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			pLog->oModuleName = pShift;
			pShift += pLog->oModuleName.GetSize() + 1;
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			pLog->oFuncName = pShift;
			pShift += pLog->oFuncName.GetSize() + 1;
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			pLog->oFile = pShift;
			pShift += pLog->oFile.GetSize() + 1;
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			oVal = pShift;
			pShift += oVal.GetSize() + 1;
			pLog->nLine = CString::Atoi(oVal.GetStr());
			if(pShift - sLog >= FOCP_LOG_MAXMSG)
				return;
			pLog->oInfo = pShift;
			pShift += pLog->oInfo.GetSize() + 1;
			if(pShift - sLog > FOCP_LOG_MAXMSG)
				return;
			m_oQueueMutex.Enter();
			m_oLogQueue.Push(pLog);
			if(m_oLogQueue.GetSize() == 1)
				m_oEvent.Set();
			m_oQueueMutex.Leave();
		}
	}
	else if(pCooperator == &m_oProcThread)
	{
		CLogMsg* pLog = NULL;
		m_oEvent.Wait(1000);
		m_oQueueMutex.Enter();
		if(m_oLogQueue.Pop(pLog))
		{
			if(m_oLogQueue.GetSize() == 0)
				m_oEvent.Reset();
		}
		m_oQueueMutex.Leave();
		if(pLog)
		{
			ProcessLog(*pLog);
			delete pLog;
		}
	}
}

bool CLogListenor::Initialize()
{
	CFileName oFileName;

	//FocpLogServerPort
	uint16 nPort = 2269;
	const char* s = GetEnvVar("FocpLogServerPort");
	if(s)
	{
		uint32 nVal = CString::Atoi(s);
		if(nVal && nVal <= 65535)
			nPort = (uint16)nVal;
	}

	oFileName.oProtocol = "udp";
	CIpAddr oBindAddr = {0, nPort};//INADDR_ANY
	CFile::GetIpFileName(oBindAddr, oFileName.oBindName);
	if(m_oFile.Open(oFileName, "rw"))
		return false;
	return true;
}

void CLogListenor::Cleanup()
{
	m_oFile.Redirect(NULL);
}

void CLogListenor::Start()
{
	m_oRecvThread.Start();
	m_oProcThread.Start();
}

void CLogListenor::Stop(bool bBlock)
{
	m_oFile.Close();
	m_oRecvThread.Stop(false);
	m_oProcThread.Stop(false);
	if(bBlock)
	{
		m_oRecvThread.Stop(true);
		m_oProcThread.Stop(true);
	}
}

FOCP_END();
