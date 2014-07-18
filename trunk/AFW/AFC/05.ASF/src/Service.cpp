
#include "Service.hpp"
#include "LongJmpWin32.hpp"
#include "TextTable.hpp"

FOCP_BEGIN();

static CServiceChecker* g_pServiceChecker = NULL;

CServiceChecker::CServiceChecker()
{
	if(g_pServiceChecker)
		FocpAbort(( "redefine CServiceChecker" ));
	g_pServiceChecker = g_pServiceChecker;
}

CServiceChecker::~CServiceChecker()
{
	g_pServiceChecker = NULL;
}

void CServiceChecker::OnSetServiceStateBefore(CService* pService, uint32 nState, bool &bAllowed)
{
}

void CServiceChecker::OnSetServiceStateAfter(CService* pService, uint32 nState)
{
}

CService::CService(bool bFake)
	:CInterface(NULL),m_oWaitList(FocpFieldOffset(CService, m_pWait))
{
	m_bFailed = false;
	m_nState = FOCP_SERVICE_IDLE;
	m_nWaitState = FOCP_SERVICE_IDLE;
	m_pServiceBreak = new CBreak;
	m_pCurBreak = NULL;
	m_pTopBreak = NULL;
	m_pStackTop = NULL;
	m_pStackBottom = NULL;
	m_pStackBackup = NULL;
	m_pNext = NULL;
	m_pWait = NULL;
}

CService::CService()
	:CInterface(&(CServiceManager::GetInstance()->m_oServiceTable)),
	 m_oWaitList(FocpFieldOffset(CService, m_pWait))
{
	m_bFailed = false;
	m_nState = FOCP_SERVICE_IDLE;
	m_nWaitState = FOCP_SERVICE_IDLE;
	m_pServiceBreak = new CBreak;
	m_pCurBreak = NULL;
	m_pTopBreak = NULL;
	m_pStackTop = NULL;
	m_pStackBottom = NULL;
	m_pStackBackup = NULL;
	m_pNext = NULL;
	m_pWait = NULL;
}

CService::~CService()
{
	delete m_pServiceBreak;
}

void * CService::GetFiberTimer()
{
	return NULL;
}

void CService::StateNotice(bool bForce)
{
	CBaseSingleList<CService> oReadyQueue2(FocpFieldOffset(CService, m_pNext));
	CServiceManager* pServiceManager = CServiceManager::GetInstance();
	CService* pService = m_oWaitList.First(), *pPrev = NULL;
	while(pService)
	{
		if(pService->m_nWaitState <= m_nState || bForce)
		{
			oReadyQueue2.Push(pService);
			m_oWaitList.RemoveNext(pPrev);
			if(pPrev)
				pService = m_oWaitList.GetNext(pPrev);
			else
				pService = m_oWaitList.First();
			continue;
		}
		pPrev = pService;
		pService = m_oWaitList.GetNext(pService);
	}
	if(oReadyQueue2.GetSize())
		pServiceManager->m_oReadyQueue.AppendList(oReadyQueue2.First(), oReadyQueue2.Last(), false);
}

bool CService::LoopWaitCheck(CService* pService)
{
	CService* pItem = m_oWaitList.First();
	while(pItem)
	{
		if(pItem == pService)
			return true;
		pItem = m_oWaitList.GetNext(pItem);
	}
	return false;
}

const char* CService::GetServiceName()
{
	return NULL;
}

const char* CService::GetInterfaceName()
{
	return GetServiceName();
}

bool CService::HaveAbility(uint32 nOption)
{
	return true;
}

const char* CService::GetStateName(uint32 nState)
{
	const char* sName;
	switch(nState)
	{
	case FOCP_SERVICE_IDLE:
		sName = "IDLE";
		break;
	case FOCP_SERVICE_INITIALIZING:
		sName = "INITIALIZING";
		break;
	case FOCP_SERVICE_INITIALIZED:
		sName = "INITIALIZED";
		break;
	case FOCP_SERVICE_STARTING:
		sName = "STARTING";
		break;
	case FOCP_SERVICE_STARTED:
		sName = "STARTED";
		break;
	case FOCP_SERVICE_STOPING:
		sName = "STOPING";
		break;
	case FOCP_SERVICE_STOPED:
		sName = "STOPED";
		break;
	case FOCP_SERVICE_CLEANING:
		sName = "CLEANING";
		break;
	case FOCP_SERVICE_CLEANED:
		sName = "CLEANED";
		break;
	default:
		sName = OnGetStateName(nState);
		break;
	}
	return sName;
}

bool CService::Initialize()
{
	FocpLog(FOCP_LOG_SYSLOG, ("Initialize service '%s'", GetServiceName()));
	if(m_bFailed)
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize service '%s' Failure", GetServiceName()));
		return false;
	}
	if(!SetDefaultState(FOCP_SERVICE_INITIALIZING))
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	if(!OnInitialize())
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	if(!SetDefaultState(FOCP_SERVICE_INITIALIZED))
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	return true;
}

bool CService::Start()
{
	FocpLog(FOCP_LOG_SYSLOG, ("Start service '%s'", GetServiceName()));
	if(m_bFailed)
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize service '%s' Failure", GetServiceName()));
		return false;
	}
	if(!SetDefaultState(FOCP_SERVICE_STARTING))
	{
		FocpLog(FOCP_LOG_ERROR, ("Start service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	if(!OnStart())
	{
		FocpLog(FOCP_LOG_ERROR, ("Start service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	if(!SetDefaultState(FOCP_SERVICE_STARTED))
	{
		FocpLog(FOCP_LOG_ERROR, ("Start service '%s' Failure", GetServiceName()));
		m_bFailed = true;
		return false;
	}
	return true;
}

void CService::Stop()
{
	FocpLog(FOCP_LOG_SYSLOG, ("Stop service '%s'", GetServiceName()));
	if(!SetDefaultState(FOCP_SERVICE_STOPING))
	{
		if(m_nState < FOCP_SERVICE_STOPING || m_nState >= FOCP_SERVICE_STOPED)
			return;
	}
	OnStop();
	SetDefaultState(FOCP_SERVICE_STOPED);
}

void CService::Cleanup()
{
	if(!SetDefaultState(FOCP_SERVICE_CLEANING))
		return;
	FocpLog(FOCP_LOG_SYSLOG, ("Cleanup service '%s'", GetServiceName()));
	OnCleanup();
	SetDefaultState(FOCP_SERVICE_CLEANED);
	m_bFailed = false;
}

uint32 CService::GetState()
{
	return m_nState;
}

bool CService::Wait(CService* pService, uint32 nState)
{
	bool bRet = true;
	if(nState <= FOCP_SERVICE_INITIALIZING || nState > FOCP_SERVICE_STOPED)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	if(m_nState >= FOCP_SERVICE_CLEANING)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	if(m_nState < FOCP_SERVICE_STARTING && nState >= FOCP_SERVICE_STARTING)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	if(m_nState >= FOCP_SERVICE_STARTING && nState < FOCP_SERVICE_STARTING)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	if(m_nState < FOCP_SERVICE_STOPING && nState >= FOCP_SERVICE_STOPING)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	if(m_nState >= FOCP_SERVICE_STOPING && nState < FOCP_SERVICE_STOPING)
	{
		FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		return false;
	}
	while(true)
	{
		if(pService->m_bFailed)
		{
			bRet = false;
			FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
			break;
		}
		if(nState == 0)
		{
			if(pService->m_nState == nState)
				break;
			bRet = false;
			FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
			break;
		}
		if(nState <= FOCP_SERVICE_STARTED)
		{
			if(pService->m_nState >= nState)
				break;
		}
		else
		{
			if(pService->m_nState == FOCP_SERVICE_IDLE || pService->m_nState >= nState)
				break;
		}
		if(pService->LoopWaitCheck(this))
		{
			FocpLog(FOCP_LOG_ERROR, ("The service '%s(%s)' is waiting another service '%s(%s)' failure: loop", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
			bRet = false;
			break;
		}
		FocpLog(FOCP_LOG_SYSLOG, ("The service '%s(%s)' is waiting another service '%s(%s)'", GetServiceName(), GetStateName(m_nState), pService->GetServiceName(), pService->GetStateName(nState)));
		m_nWaitState = nState;
		pService->m_oWaitList.Push(this);
		Switch(this);
	}
	return bRet;
}

bool CService::SetDefaultState(uint32 nState)
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();

	switch(nState)
	{
	case FOCP_SERVICE_IDLE:
		return false;
		break;
	case FOCP_SERVICE_INITIALIZING:
		if((m_nState != FOCP_SERVICE_IDLE) && (m_nState != FOCP_SERVICE_CLEANED))
			return false;
		break;
	case FOCP_SERVICE_STARTING:
		if((m_nState != FOCP_SERVICE_INITIALIZED) && (m_nState != FOCP_SERVICE_STOPED))
			return false;
		break;
	case FOCP_SERVICE_STOPING:
		if((m_nState < FOCP_SERVICE_STARTING) || (m_nState >= FOCP_SERVICE_STOPING))
			return false;
		break;
	case FOCP_SERVICE_CLEANING:
		if((m_nState < FOCP_SERVICE_INITIALIZING) ||
				((m_nState > FOCP_SERVICE_INITIALIZED) && (m_nState != FOCP_SERVICE_STOPED)))
			return false;
		break;
	case FOCP_SERVICE_INITIALIZED:
		break;
	case FOCP_SERVICE_STARTED:
		break;
	case FOCP_SERVICE_STOPED:
		break;
	}
	if(g_pServiceChecker && this != (CService*)pServiceManager)
	{
		bool bAllowed = true;
		g_pServiceChecker->OnSetServiceStateBefore(this, nState, bAllowed);
		if(!bAllowed)
			return false;
		g_pServiceChecker->OnSetServiceStateAfter(this, nState);
	}
	m_nState = nState;
	FocpLog(FOCP_LOG_SYSLOG, ("The service '%s' is changed to the state '%s'", GetServiceName(), GetStateName(m_nState)));
	StateNotice();
	return true;
}

bool CService::SetState(uint32 nState)
{
	CServiceManager* pServiceManager = CServiceManager::GetInstance();

	if((nState > FOCP_SERVICE_INITIALIZING) && (nState < FOCP_SERVICE_INITIALIZED))
	{
		if((m_nState <= FOCP_SERVICE_INITIALIZING) || (m_nState >= FOCP_SERVICE_INITIALIZED))
			return false;
	}
	else if((nState > FOCP_SERVICE_STARTING) && (nState < FOCP_SERVICE_STARTED))
	{
		if((m_nState <= FOCP_SERVICE_STARTING) || (m_nState >= FOCP_SERVICE_STARTED))
			return false;
	}
	else if((nState > FOCP_SERVICE_STOPING) && (nState < FOCP_SERVICE_STOPED))
	{
		if((m_nState <= FOCP_SERVICE_STOPING) || (m_nState >= FOCP_SERVICE_STOPED))
			return false;
	}
	/*
		else if((nState > FOCP_SERVICE_CLEANING) && (nState < FOCP_SERVICE_CLEANED))
		{
			if((m_nState <= FOCP_SERVICE_CLEANING) || (m_nState >= FOCP_SERVICE_CLEANED))
				return false;
		}
	*/
	else
		return false;
	if(g_pServiceChecker && this != (CService*)pServiceManager)
	{
		bool bAllowed = true;
		g_pServiceChecker->OnSetServiceStateBefore(this, nState, bAllowed);
		if(!bAllowed)
			return false;
		g_pServiceChecker->OnSetServiceStateAfter(this, nState);
	}
	m_nState = nState;
	FocpLog(FOCP_LOG_SYSLOG, ("The service '%s' is changed to the state '%s'", GetServiceName(), GetStateName(m_nState)));
	StateNotice();
	return true;
}

bool CService::OnInitialize()
{
	return true;
}

bool CService::OnStart()
{
	return true;
}

void CService::OnStop()
{
}

void CService::OnCleanup()
{
}

const char* CService::OnGetStateName(uint32 nState)
{
	return "UNKNOWN";
}

CServiceManager::CServiceManager():
	CService(false),
	m_oServiceTable("Services"),
	m_oReadyQueue(FocpFieldOffset(CService, m_pNext))
{
	GetStackAttr(m_oStackAttr);
	m_nServiceCount = 0;
	m_pTopBreak = new CBreak;
	m_bUnLoad = true;
	m_nAIN = 0;//GetAppInstanceNo();
}

CServiceManager::~CServiceManager()
{
	delete m_pTopBreak;
}

uint32 CServiceManager::GetDMN()
{
	return m_nDMN;
}

uint32 CServiceManager::GetCNN()
{
	return m_nCNN;
}

uint32 CServiceManager::GetATN()
{
	return m_nATN;
}

uint32 CServiceManager::GetAIN()
{
	return m_nAIN;
}

void CServiceManager::SetATN(uint32 nATN)
{
	m_nATN = nATN;
}

void CServiceManager::SetInstance(uint32 nDMN, uint32 nCNN, uint32 nAIN)
{
	m_nDMN = nDMN;
	m_nCNN = nCNN;
	m_nAIN = nAIN;
}

void CServiceManager::DontUnLoad()
{
	m_bUnLoad = false;
}

uint32 CServiceManager::GetServiceCount()
{
	return m_nServiceCount;
}

CService* CServiceManager::GetService(uint32 nServiceIdx)
{
	return (CService*)m_oServiceTable.GetInterface(nServiceIdx);
}

CService* CServiceManager::QueryService(const char* sServiceName)
{
	return (CService*)m_oServiceTable.QueryInterface(sServiceName);
}

CServiceManager* CServiceManager::GetInstance()
{
	return CSingleInstance<CServiceManager>::GetInstance();
}

const char* CServiceManager::GetServiceName()
{
	return "ServiceManager";
}

bool CServiceManager::OnInitialize()
{
	uint32 i;
	char sAppCfgHome[FOCP_MAX_PATH];

	CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();

	CString::StringCopy(sAppCfgHome, CFilePathInfo::GetInstance()->GetHome());
	CString::StringCatenate(sAppCfgHome, "/cfg");

	if(!pConfigSystem->Initialize(GetEnvVar("FocpNmsCfgHome"), sAppCfgHome, CFilePathInfo::GetInstance()->GetName()))
	{
		FocpLog(FOCP_LOG_ERROR, ("Initialize configure system failure"));
		return false;
	}

	if(!pConfigSystem->Load())
	{
		FocpLog(FOCP_LOG_ERROR, ("Load config data failure"));
		return false;
	}

	m_oServiceTable.Load();

	m_nServiceCount = m_oServiceTable.GetSize();
	if(!m_nServiceCount)
		return true;

	m_oMutex.Enter();
	m_bResult = true;
	for(i=0; i<m_nServiceCount; ++i)
		GetService(i)->m_bFailed = false;
	for(i=0; i<m_nServiceCount; ++i)
		m_oReadyQueue.Push(GetService(i));
	Run(this, 0);
	m_oMutex.Leave();

	if(m_bUnLoad)
		pConfigSystem->UnLoad();

	return m_bResult;
}

bool CServiceManager::OnStart()
{
	uint32 i;
	if(m_nServiceCount == 0)
		return true;

	m_oMutex.Enter();
	m_bResult = true;
	for(i=0; i<m_nServiceCount; ++i)
		GetService(i)->m_bFailed = false;
	for(i=0; i<m_nServiceCount; ++i)
		m_oReadyQueue.Push(GetService(i));
	Run(this, 1);
	m_oMutex.Leave();

	return m_bResult;
}

void CServiceManager::OnStop()
{
	uint32 i;
	if(m_nServiceCount == 0)
		return;

	m_oMutex.Enter();
	for(i=0; i<m_nServiceCount; ++i)
		GetService(i)->m_bFailed = false;
	for(i=0; i<m_nServiceCount; ++i)
		m_oReadyQueue.Push(GetService(m_nServiceCount-i-1));
	Run(this, 2);
	m_oMutex.Leave();
}

void CServiceManager::OnCleanup()
{
	m_oMutex.Enter();
	for(uint32 i=0; i<m_nServiceCount; ++i)
		GetService(m_nServiceCount-i-1)->Cleanup();
	m_oServiceTable.UnLoad();
	m_oMutex.Leave();

	if(!m_bUnLoad)
		CInitConfigSystem::GetInstance()->UnLoad();
}

FOCP_END();
