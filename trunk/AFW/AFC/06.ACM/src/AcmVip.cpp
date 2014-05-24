
#include "AcmVip.hpp"

FOCP_BEGIN();

static uint32 GetVipToken(const char* sVip)
{
	bool bMap;
	uint32 nAddr;
	CIpAddress oAddr;
	if(!oAddr.SetIp(sVip, false, false, false))
		return 0;
	if(!oAddr.GetIp(nAddr, bMap))
		return 0;
	nAddr = CBinary::U32Code(nAddr);
	return ((nAddr&0xFFFF) << 16) | ACM_VIP_TOKEN;
}

CAcmVipToken::CAcmVipToken(uint32 nDomain, const char* sVip, const char* sMask, const char* sIfName, uint8 nWeight):
	CAcmToken(nDomain, GetVipToken(sVip), nWeight),
	m_nVip(sVip), m_nMask(sMask), m_nIfName(sIfName)
{
}

CAcmVipToken::~CAcmVipToken()
{
}

const char* CAcmVipToken::GetVip()const
{
	return m_nVip.GetStr();
}

void CAcmVipToken::OnRoleChange(bool bTakeUp)
{
	if(bTakeUp)
	{
		if(!AddIpv4Addr(m_nVip.GetStr(), m_nMask.GetStr(), m_nIfName.GetStr()))
			FocpError(("AddIpv4Addr(%s/%s on %s) failure",m_nVip.GetStr(), m_nMask.GetStr(), m_nIfName.GetStr()));
		else
		{
			FocpInfo(("AddIpv4Addr(%s/%s on %s) success",m_nVip.GetStr(), m_nMask.GetStr(), m_nIfName.GetStr()));
			CAcmVipModule::GetInstance()->OnEvent(this, bTakeUp);
		}
	}
	else
	{
		if(!DelIpv4Addr(m_nVip.GetStr()))
			FocpError(("DelIpv4Addr(%s/%s on %s) failure",m_nVip.GetStr(), m_nMask.GetStr(), m_nIfName.GetStr()));
		else
		{
			FocpInfo(("DelIpv4Addr(%s/%s on %s) success",m_nVip.GetStr(), m_nMask.GetStr(), m_nIfName.GetStr()));
			CAcmVipModule::GetInstance()->OnEvent(this, bTakeUp);
		}
	}
}

CAcmVipEvent::CAcmVipEvent()
{
}

CAcmVipEvent::~CAcmVipEvent()
{
}

void CAcmVipEvent::OnEvent(CAcmVipToken* pToken, bool bTakeUp)
{
}

CAcmVipModule::CAcmVipModule()
{
	m_pTokenModule = NULL;
	m_bStarted = false;
}

CAcmVipModule::~CAcmVipModule()
{
}

CAcmVipModule* CAcmVipModule::GetInstance()
{
	return 	CSingleInstance<CAcmVipModule>::GetInstance();
}

bool CAcmVipModule::AddToken(uint32 nDomain, const char* sVip, const char* sMask, const char* sIfName, uint32 nDefaultNode)
{
	CLock oLock(m_oMutex);
	if(m_pTokenModule == NULL)
	{
		m_pTokenModule = CAcmTokenModule::GetInstance();
		if(m_pTokenModule == NULL)
			return false;
	}
	CAcmUdp* pUdp = CAcmUdp::QueryUdp(nDomain);
	if(pUdp == NULL)
		return false;
	uint32 nNode = pUdp->GetNode();
	uint32 nDiff;
	if(nNode > nDefaultNode)
		nDiff = nNode - nDefaultNode;
	else
		nDiff = nDefaultNode - nNode;
	nDiff %= 128;
	CRbTreeNode* pIt = m_oVipTokens.Find(sVip);
	if(pIt != m_oVipTokens.End())
		return false;
	CAcmVipToken* pToken = new CAcmVipToken(nDomain, sVip, sMask, sIfName, 192-nDiff);
	if(m_bStarted && !m_pTokenModule->AddToken(pToken))
		return false;
	m_oVipTokens[sVip] = pToken;
	return true;
}

void CAcmVipModule::RegisterEvent(CAcmVipEvent* pEvent)
{
	CLock oLock(m_oMutex);
	if(pEvent)
		m_oEvents.Push(pEvent);
}

void CAcmVipModule::DeRegisterEvent(CAcmVipEvent* pEvent)
{
	CLock oLock(m_oMutex);
	void* pIt = m_oEvents.First(), *pPrev=NULL;
	while(pIt)
	{
		if(pEvent == m_oEvents.GetItem(pIt))
		{
			m_oEvents.RemoveNext(pPrev);
			break;
		}
		pIt = m_oEvents.GetNext(pIt);
	}
}

CAcmVipToken* CAcmVipModule::QueryVip(const char* sVip)
{
	CLock oLock(m_oMutex);
	CRbTreeNode* pIt = m_oVipTokens.Find(sVip);
	if(pIt == m_oVipTokens.End())
		return NULL;
	return (CAcmVipToken*)m_oVipTokens.GetItem(pIt);
}

void CAcmVipModule::OnEvent(CAcmVipToken* pToken, bool bTakeUp)
{
	CLock oLock(m_oMutex);
	void* pIt = m_oEvents.First();
	while(pIt)
	{
		m_oEvents.GetItem(pIt)->OnEvent(pToken, bTakeUp);
		pIt = m_oEvents.GetNext(pIt);
	}
}

bool CAcmVipModule::Start()
{
	CLock oLock(m_oMutex);
	if(m_pTokenModule == NULL)
		return false;
	if(!m_bStarted)
	{
		CRbTreeNode* pIt = m_oVipTokens.First();
		CRbTreeNode* pEnd = m_oVipTokens.End();
		for(; pIt!=pEnd; pIt=m_oVipTokens.GetNext(pIt))
		{
			CAcmVipToken* pToken = (CAcmVipToken*)m_oVipTokens.GetItem(pIt);
			if(!m_pTokenModule->AddToken(pToken))
			{
				FocpError(("AddVip(%s) failure", pToken->GetVip()));
				return false;
			}
		}
		m_bStarted = true;
	}
	return true;
}

void CAcmVipModule::Stop()
{
	if(m_bStarted)
	{
		CRbTreeNode* pIt = m_oVipTokens.First();
		CRbTreeNode* pEnd = m_oVipTokens.End();
		for(; pIt!=pEnd; pIt=m_oVipTokens.GetNext(pIt))
		{
			CAcmVipToken* pToken = (CAcmVipToken*)m_oVipTokens.GetItem(pIt);
			m_pTokenModule->DelToken(pToken);
		}
		m_bStarted = false;
	}
}

FOCP_END();
