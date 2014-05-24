
#include "AcmToken.hpp"

#include <math.h>

FOCP_BEGIN();

FOCP_PRIVATE_BEGIN();

struct CAcmTokenBsmMsgBody
{
	uint32 nTokenId;
	uint32 nWeight;
};

struct CAcmTokenTimerMsg
{
	uint32 nDomain;
	uint32 nTokenId;
};

enum
{
	ACM_BSM_P_MSG,
	ACM_BSM_N_MSG,
	ACM_BSM_NS_MSG,
	ACM_BSM_T_MSG,

	ACM_BSM_A_STATE,
	ACM_BSM_C_STATE,
	ACM_BSM_P_STATE,
	ACM_BSM_E_STATE,
};

FOCP_PRIVATE_END();

CAcmToken::CAcmToken(uint32 nDomain, uint32 nToken, uint8 nWeight)
{
	m_nDomain = nDomain;
	m_nToken = nToken;
	m_nWeight = nWeight;
}

CAcmToken::~CAcmToken()
{
}

bool CAcmToken::GetOwner(uint32 &nOwner)
{
	return CAcmTokenModule::GetInstance()->QueryToken(m_nDomain, m_nToken, nOwner);
}

uint32 CAcmToken::GetDomain() const
{
	return m_nDomain;
}

uint32 CAcmToken::GetToken() const
{
	return m_nToken;
}

void CAcmToken::OnRoleChange(bool bTakeUp)
{
}

void CAcmToken::OnOwnerChange(uint32 nOldOwner, uint32 nNewOwner)
{
}

CAcmMutex::CAcmMutex(uint32 nDomain, uint32 nToken, bool bThread):
	CAcmToken(nDomain, nToken, 64), m_oLock(bThread), m_oEvent(false, bThread)
{
	m_bOwned = false;
	m_nCounter = 0;
}

CAcmMutex::~CAcmMutex()
{
}

bool CAcmMutex::Enter(uint32 nTimeOut)
{
	CAcmTokenModule* pModule = CAcmTokenModule::GetInstance();
	if(pModule == NULL)
		return false;
	bool bNeedDelete = false;
	m_oLock.Enter();
	m_oMutex.Enter();
	if(m_bOwned)
		++m_nCounter;
	else
	{
		m_oEvent.Reset();
		if(pModule->AddToken(this))
		{
			m_oMutex.Leave();
			m_oEvent.Wait(nTimeOut);
			m_oMutex.Enter();
			if(m_bOwned)
				m_nCounter = 1;
			else
				bNeedDelete = true;
		}
	}
	bool bRet = m_bOwned;
	m_oMutex.Leave();
	if(!bRet)
		m_oLock.Leave();
	if(bNeedDelete)
		pModule->DelToken(this);
	return bRet;
}

void CAcmMutex::Leave()
{
	CAcmTokenModule* pModule = CAcmTokenModule::GetInstance();
	if(pModule == NULL)
		return;
	bool bNeedDelete = false;
	m_oLock.Enter();
	m_oMutex.Enter();
	if(m_nCounter)
	{
		--m_nCounter;
		m_oLock.Leave();
		if(!m_nCounter)
		{
			m_bOwned = false;
			bNeedDelete = true;
		}
	}
	m_oMutex.Leave();
	m_oLock.Leave();
	if(bNeedDelete)
		pModule->DelToken(this);
}

void CAcmMutex::OnRoleChange(bool bTakeUp)
{
	m_oMutex.Enter();
	if(bTakeUp || m_bOwned)
	{
		m_bOwned = bTakeUp;
		m_oEvent.Set();
	}
	m_oMutex.Leave();
}

CAcmTokenWalker::CAcmTokenWalker()
{
}

CAcmTokenWalker::~CAcmTokenWalker()
{
}

void CAcmTokenWalker::OnWalk(uint32 nToken, uint32 nDomain, uint32 nOwner)
{
}

static CMutex g_oMutex;
static CAcmTokenModule* g_pTokenModule = NULL;

CAcmTokenModule* CAcmTokenModule::GetInstance(bool bQuery)
{
	if(!g_pTokenModule && !bQuery)
	{
		g_oMutex.Enter();
		if(!g_pTokenModule)
			g_pTokenModule = new CAcmTokenModule;
		g_oMutex.Leave();
	}
	return g_pTokenModule;
}

bool CAcmTokenModule::QueryToken(uint32 nDomain, uint32 nToken, uint32 &nOwner)
{
	m_oMutex.Enter();
	CToken* pToken = GetToken(nDomain, nToken);
	if(pToken)
		nOwner = pToken->nOwner;
	m_oMutex.Leave();
	return pToken!=NULL;
}

CAcmTokenModule::CToken* CAcmTokenModule::GetToken(uint32 nDomain, uint32 nToken)
{
	CToken* pToken = NULL;
	CRbTreeNode* pIt = m_oTokens.Find(nDomain);
	if(pIt != m_oTokens.End())
	{
		CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
		pIt = oTokenTable.Find(nToken);
		if(pIt != oTokenTable.End())
		{
			CToken &oToken = oTokenTable.GetItem(pIt);
			pToken = &oToken;
		}
	}
	return pToken;
}

bool CAcmTokenModule::ProcessBsmA(CAcmTokenModule::CToken* pToken, CAcmTokenMsg* pMsg)
{
	uint32 nNewOwner;
	bool bRet = false;
	uint32 nMsgType = GetMsgType(pToken, pMsg);
	switch(nMsgType)
	{
	case ACM_BSM_P_MSG:
		nNewOwner = pMsg->oMsg.oBsm.nSender;
		if(nNewOwner != pToken->nOwner)
			pToken->pToken->OnOwnerChange(pToken->nOwner, nNewOwner);
		if(pToken->nOwner == pToken->nNode)
			pToken->pToken->OnRoleChange(false);
		pToken->nStatus = ACM_BSM_C_STATE;
		pToken->nOwner = pMsg->oMsg.oBsm.nSender;
		pToken->nOwnerWeight = nNewOwner;
		pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 2);
		break;

	case ACM_BSM_T_MSG:
		if(pToken->nTimerId == pMsg->oMsg.oBst.nTimerId)
		{
			bRet = true;
			pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 1);
		}
		break;

	case ACM_BSM_N_MSG:
		bRet = true;
		break;
	}
	return bRet;
}

bool CAcmTokenModule::ProcessBsmC(CAcmTokenModule::CToken* pToken, CAcmTokenMsg* pMsg)
{
	uint32 nNewOwner;
	uint32 nMsgType = GetMsgType(pToken, pMsg);
	switch(nMsgType)
	{
	case ACM_BSM_P_MSG:
		nNewOwner = pMsg->oMsg.oBsm.nSender;
		if(pToken->nOwner != nNewOwner)
			pToken->pToken->OnOwnerChange(pToken->nOwner, nNewOwner);
		pToken->nOwner = nNewOwner;
		pToken->nOwnerWeight = pMsg->oMsg.oBsm.nWeight;
		pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 2);
		break;

	case ACM_BSM_T_MSG:
		if(pToken->nTimerId == pMsg->oMsg.oBst.nTimerId)
		{
			pToken->nStatus = ACM_BSM_P_STATE;
			pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 4);
		}
		break;

	case ACM_BSM_NS_MSG:
		pToken->nStatus = ACM_BSM_P_STATE;
		pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 5);
		break;
	}
	return false;
}

bool CAcmTokenModule::ProcessBsmP(CAcmTokenModule::CToken* pToken, CAcmTokenMsg* pMsg)
{
	uint32 nNewOwner;
	bool bRet = false;
	uint32 nMsgType = GetMsgType(pToken, pMsg);
	switch(nMsgType)
	{
	case ACM_BSM_P_MSG:
		nNewOwner = pMsg->oMsg.oBsm.nSender;
		if(pToken->nOwner != nNewOwner)
			pToken->pToken->OnOwnerChange(pToken->nOwner, nNewOwner);
		pToken->nStatus = ACM_BSM_C_STATE;
		pToken->nOwner = nNewOwner;
		pToken->nOwnerWeight = pMsg->oMsg.oBsm.nWeight;
		pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 2);
		break;

	case ACM_BSM_T_MSG:
		if(pToken->nTimerId == pMsg->oMsg.oBst.nTimerId)
		{
			bRet = true;
			pToken->nStatus = ACM_BSM_E_STATE;
			pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 3);
		}
		break;
	}
	return bRet;
}

bool CAcmTokenModule::ProcessBsmE(CAcmTokenModule::CToken* pToken, CAcmTokenMsg* pMsg)
{
	uint32 nNewOwner;
	bool bRet = false;
	uint32 nMsgType = GetMsgType(pToken, pMsg);
	switch(nMsgType)
	{
	case ACM_BSM_P_MSG:
		nNewOwner = pMsg->oMsg.oBsm.nSender;
		if(pToken->nOwner != nNewOwner)
			pToken->pToken->OnOwnerChange(pToken->nOwner, nNewOwner);
		pToken->nStatus = ACM_BSM_C_STATE;
		pToken->nOwner = nNewOwner;
		pToken->nOwnerWeight = pMsg->oMsg.oBsm.nWeight;
		pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 2);
		break;

	case ACM_BSM_T_MSG:
		if(pToken->nTimerId == pMsg->oMsg.oBst.nTimerId)
		{
			bRet = true;
			if(pToken->nOwner != pToken->nNode)
			{
				pToken->pToken->OnOwnerChange(pToken->nOwner, pToken->nNode);
				pToken->pToken->OnRoleChange(true);
			}
			pToken->nStatus = ACM_BSM_A_STATE;
			pToken->nTimerId = SetTimer(pMsg->nToken, pToken, 1);
			pToken->nOwner = pToken->nNode;
			pToken->nOwnerWeight = pToken->nWeight;
		}
		break;

	case ACM_BSM_N_MSG:
		bRet = true;
		break;
	}
	return bRet;
}

uint32 CAcmTokenModule::GetMsgType(CAcmTokenModule::CToken* pToken, CAcmTokenMsg* pMsg)
{
	uint32 nMsgType;
	uint64 nSenderWeight, nOwnerWeight, nMyWeight;

	if(pMsg->oMsg.nCmd == 1)
		nMsgType = ACM_BSM_T_MSG;
	else switch(pToken->nStatus)
		{
		case ACM_BSM_P_STATE:
			nSenderWeight = ComputeWeight(pMsg->oMsg.oBsm.nSender, pMsg->oMsg.oBsm.nWeight);
			nMyWeight = ComputeWeight(pToken->nNode, pToken->nWeight);
			if(nSenderWeight > nMyWeight)
				nMsgType = ACM_BSM_P_MSG;
			else
				nMsgType = ACM_BSM_N_MSG;
			break;

		case ACM_BSM_C_STATE:
			nSenderWeight = ComputeWeight(pMsg->oMsg.oBsm.nSender, pMsg->oMsg.oBsm.nWeight);
			nOwnerWeight = ComputeWeight(pToken->nOwner, pToken->nOwnerWeight);
			if(nSenderWeight >= nOwnerWeight)
				nMsgType = ACM_BSM_P_MSG;
			else
			{
				nMyWeight = ComputeWeight(pToken->nNode, pToken->nWeight);
				if(nSenderWeight >= nMyWeight)
					nMsgType = ACM_BSM_P_MSG;
				else if(pToken->nOwner == pMsg->oMsg.oBsm.nSender)
					nMsgType = ACM_BSM_NS_MSG;
				else
					nMsgType = ACM_BSM_N_MSG;
			}
			break;

		case ACM_BSM_A_STATE:
		case ACM_BSM_E_STATE:
			nSenderWeight = ComputeWeight(pMsg->oMsg.oBsm.nSender, pMsg->oMsg.oBsm.nWeight);
			nOwnerWeight = ComputeWeight(pToken->nOwner, pToken->nOwnerWeight);
			if(nSenderWeight >= nOwnerWeight)
				nMsgType = ACM_BSM_P_MSG;
			else
				nMsgType = ACM_BSM_N_MSG;
			break;
		}

	return nMsgType;
}

uint64 CAcmTokenModule::ComputeWeight(uint32 nNode, uint32 nWeight)
{
	return (nNode == 0xFFFFFFFF)?(uint64)0:((((uint64)nWeight)<<32)|nNode);
}

void CAcmTokenModule::SendBsm(CAcmUdp* pUdp, uint32 nToken, uint32 nWeight)
{
	char pBuf[64];
	CUdpMsg* pMsg = (CUdpMsg*)pBuf;
	pMsg->nSize = 20;
	pMsg->oHead.nModule = 1; //TokenModule
	pMsg->oHead.nCmd = 0; //TokenBootStrap
	CAcmTokenBsmMsgBody* pBody = (CAcmTokenBsmMsgBody*)pMsg->sBody;
	pBody->nTokenId = CBinary::U32Code(nToken);
	pBody->nWeight = CBinary::U32Code(nWeight);
	pUdp->Send(*pMsg);
}

static void AcmTokenTimerFunc(uint8* pMsg, uint32 nMsgLen, uint32 nTimerId)
{
	CAcmTokenTimerMsg* pMsg2 = (CAcmTokenTimerMsg*)pMsg;
	CAcmTokenModule::GetInstance()->ProcessTimer(pMsg2->nDomain, pMsg2->nTokenId, nTimerId);
}

uint32 CAcmTokenModule::SetTimer(uint32 nToken, CToken* pToken, uint32 nTimerIdx)
{
	CAcmTokenTimerMsg oMsg = {pToken->pUdp->GetDomain(), nToken};
	uint32 nTimeOut;
	if(nTimerIdx == 5)
		nTimeOut = m_nTimeOut[2] + pToken->nTimeOut;
	else if(nTimerIdx == 4)
		nTimeOut = pToken->nTimeOut;
	else
		nTimeOut = m_nTimeOut[nTimerIdx];
	return CTimer::GetInstance()->SetTimer(nTimeOut, AcmTokenTimerFunc, (uint8*)&oMsg, sizeof(oMsg));
}

void CAcmTokenModule::ProcessBsm(CAcmTokenMsg& oTokenMsg)
{
	CAcmUdp* pUdp;
	uint32 nToken = -1, nWeight;
	m_oMutex.Enter();
	if(!m_bStarted)
	{
		m_oMutex.Leave();
		return;
	}
	CToken* pToken = GetToken(oTokenMsg.nDomain, oTokenMsg.nToken);
	if(pToken)
	{
		bool bSendBsm;
		if(oTokenMsg.oMsg.nCmd == 0 && pToken->pUdp != oTokenMsg.pUdp)
		{
			m_oMutex.Leave();
			return;
		}
		switch(pToken->nStatus)
		{
		case ACM_BSM_A_STATE:
			bSendBsm = ProcessBsmA(pToken, &oTokenMsg);
			break;
		case ACM_BSM_C_STATE:
			bSendBsm = ProcessBsmC(pToken, &oTokenMsg);
			break;
		case ACM_BSM_P_STATE:
			bSendBsm = ProcessBsmP(pToken, &oTokenMsg);
			break;
		case ACM_BSM_E_STATE:
			bSendBsm = ProcessBsmE(pToken, &oTokenMsg);
			break;
		}
		if(bSendBsm)
		{
			nToken = oTokenMsg.nToken;
			nWeight = pToken->nWeight;
			pUdp = oTokenMsg.pUdp;
		}
	}
	m_oMutex.Leave();
	if(nToken != (uint32)-1)
		SendBsm(pUdp, nToken, nWeight);
}

void CAcmTokenModule::ProcessTimer(uint32 nDomain, uint32 nTokenId, uint32 nTimerId)
{
	CAcmTokenMsg oTokenMsg;
	if(!m_bStarted)
		return;
	oTokenMsg.nDomain = nDomain;
	oTokenMsg.nToken = nTokenId;
	oTokenMsg.pUdp = CAcmUdp::QueryUdp(nDomain);
	oTokenMsg.oMsg.nCmd = 1;
	oTokenMsg.oMsg.oBst.nTimerId = nTimerId;
	ProcessBsm(oTokenMsg);
}

void CAcmTokenModule::ProcessAcmModuleMsg(CAcmUdp* pUdp,CUdpMsg* &pMsg)//for CAcmUdpModule
{
	if(pMsg->oHead.nCmd == 0)//TokenBootStrap
	{
		if(!m_bStarted)
			return;
		CAcmTokenBsmMsgBody* pBody = (CAcmTokenBsmMsgBody*)pMsg->sBody;
		CAcmTokenMsg oTokenMsg;
		oTokenMsg.nToken = CBinary::U32Code(pBody->nTokenId);
		oTokenMsg.nDomain = pUdp->GetDomain();
		oTokenMsg.pUdp = pUdp;
		oTokenMsg.oMsg.nCmd = 0;
		oTokenMsg.oMsg.oBsm.nSender = pMsg->oHead.nNode;
		oTokenMsg.oMsg.oBsm.nWeight = CBinary::U32Code(pBody->nWeight);
		ProcessBsm(oTokenMsg);
	}
}

bool CAcmTokenModule::AddToken(CAcmToken* pToken)
{
	CAcmUdp* pUdp = CAcmUdp::QueryUdp(pToken->m_nDomain);
	if(pUdp == NULL)
		return false;
	bool bNeedRegister = false;
	CRbMap<uint32/*nTokenId*/, CToken>* pTokenTable;
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oTokens.Find(pToken->m_nDomain);
	if(pIt != m_oTokens.End())
	{
		CRbMap<uint32/*nTokenId*/, CToken> &oTokenTable = m_oTokens.GetItem(pIt);
		pIt = oTokenTable.Find(pToken->m_nToken);
		if(pIt != oTokenTable.End())
		{
			m_oMutex.Leave();
			return false;
		}
		pTokenTable = &oTokenTable;
	}
	else
	{
		CRbMap<uint32/*nTokenId*/, CToken> &oTokenTable = m_oTokens[pToken->m_nDomain];
		pTokenTable = &oTokenTable;
		bNeedRegister = true;
	}
	uint32 nNode = pUdp->GetNode();
	CRbMap<uint32/*nTokenId*/, CToken> &oTokenTable = *pTokenTable;
	CToken &oToken = oTokenTable[pToken->m_nToken];
	oToken.nStatus = ACM_BSM_P_STATE;
	oToken.nWeight = pToken->m_nWeight;
	oToken.nOwner =(uint32)-1;
	oToken.nOwnerWeight =0;
	oToken.nTimerId = -1;
	oToken.pUdp = pUdp;
	oToken.nNode = nNode;
	oToken.pToken = pToken;
	UpdateTokenTimeOut(&oToken, m_nTimeOut[0]);
	if(bNeedRegister)
		pUdp->RegisterModule(ACM_TOKEN_MODULE, this);
	if(m_bStarted)
		StartToken(pToken->m_nToken, &oToken);
	m_oMutex.Leave();
	return true;
}

void CAcmTokenModule::DelToken(CAcmToken* pToken)
{
	bool bChange = false;
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oTokens.Find(pToken->m_nDomain);
	if(pIt != m_oTokens.End())
	{
		CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
		pIt = oTokenTable.Find(pToken->m_nToken);
		if(pIt != oTokenTable.End())
		{
			CToken* pTokenX = &oTokenTable.GetItem(pIt);
			if(pTokenX->pToken == pToken)
			{
				bChange = true;
				CAcmUdp* pUdp = pTokenX->pUdp;
				oTokenTable.Remove(pIt);
				if(!oTokenTable.GetSize())
				{
					m_oTokens.Remove(pToken->m_nDomain);
					pUdp->DeRegisterModule(ACM_TOKEN_MODULE);
				}
			}
		}
	}
	m_oMutex.Leave();
	if(bChange)
		pToken->OnRoleChange(false);
}

void CAcmTokenModule::Walk(CAcmTokenWalker& oWalker)
{
	CRbTreeNode* pEnd = m_oTokens.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oTokens.First();
	for(; pIt!=pEnd; pIt=m_oTokens.GetNext(pIt))
	{
		uint32 nDomain = m_oTokens.GetKey(pIt);
		CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
		CRbTreeNode* pIt2 = oTokenTable.First();
		CRbTreeNode* pEnd2 = oTokenTable.End();
		for(; pIt2!=pEnd2; pIt2=oTokenTable.GetNext(pIt2))
		{
			uint32 nToken = oTokenTable.GetKey(pIt2);
			CToken &oToken = oTokenTable.GetItem(pIt2);
			oWalker.OnWalk(nDomain, nToken, oToken.nOwner);
		}
	}
	m_oMutex.Leave();
}

void CAcmTokenModule::UpdateTimeOut(uint32 nT0)
{
	if(nT0 == 0)
		nT0 = 1000;
	m_oMutex.Enter();
	if(nT0 != m_nTimeOut[0])
	{
		m_nTimeOut[0] = nT0;
		m_nTimeOut[1] = nT0 + (nT0>>1);
		m_nTimeOut[2] = nT0 + (m_nTimeOut[1]<<1);
		m_nTimeOut[3] = nT0 + (nT0>>1);
		UpdateTokenTimeOut(NULL, nT0);
	}
	m_oMutex.Leave();
}

void CAcmTokenModule::UpdateTokenTimeOut(CToken* pToken, uint32 nT0)
{
	if(pToken == NULL)
	{
		CRbTreeNode* pEnd = m_oTokens.End();
		m_oMutex.Enter();
		CRbTreeNode* pIt = m_oTokens.First();
		for(; pIt!=pEnd; pIt=m_oTokens.GetNext(pIt))
		{
			CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
			CRbTreeNode* pIt2 = oTokenTable.First();
			CRbTreeNode* pEnd2 = oTokenTable.End();
			for(; pIt2!=pEnd2; pIt2=oTokenTable.GetNext(pIt2))
			{
				CToken &oToken = oTokenTable.GetItem(pIt2);
				UpdateTokenTimeOut(&oToken, nT0);
			}
		}
		m_oMutex.Leave();
	}
	else
	{
		CIpAddr oAddr;
		pToken->pUdp->GetInterfaceAddr(oAddr);
		double nAddrDeplay = ((uint8*)&oAddr.nAddr)[3]/128.0*nT0/10;
		double nWeightDeplay = pow(255 - pToken->nWeight, 0.5)*nT0/10;
		pToken->nTimeOut = (uint32)(nWeightDeplay + nAddrDeplay + nT0);
	}
}

void CAcmTokenModule::StartToken(uint32 nToken, CToken* pToken)
{
	pToken->nTimerId = SetTimer(nToken, pToken, 4);
	SendBsm(pToken->pUdp, nToken, pToken->nWeight);
}

void CAcmTokenModule::Start()
{
	m_bStarted = true;
	CRbTreeNode* pEnd = m_oTokens.End();
	m_oMutex.Enter();
	CRbTreeNode* pIt = m_oTokens.First();
	for(; pIt!=pEnd; pIt=m_oTokens.GetNext(pIt))
	{
		CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
		CRbTreeNode* pIt2 = oTokenTable.First();
		CRbTreeNode* pEnd2 = oTokenTable.End();
		for(; pIt2!=pEnd2; pIt2=oTokenTable.GetNext(pIt2))
		{
			uint32 nToken = oTokenTable.GetKey(pIt2);
			CToken &oToken = oTokenTable.GetItem(pIt2);
			StartToken(nToken, &oToken);
		}
	}
	m_oMutex.Leave();
}

void CAcmTokenModule::Stop()
{
	if(m_bStarted)
	{
		CRbTreeNode* pEnd = m_oTokens.End();
		m_oMutex.Enter();
		if(!m_bStarted)
		{
			m_oMutex.Leave();
			return;
		}
		m_bStarted = false;
		CRbTreeNode* pIt = m_oTokens.First();
		for(; pIt!=pEnd; pIt=m_oTokens.GetNext(pIt))
		{
			CRbMap<uint32/*nTokenId*/, CToken>& oTokenTable = m_oTokens.GetItem(pIt);
			CRbTreeNode* pIt2 = oTokenTable.First();
			CRbTreeNode* pEnd2 = oTokenTable.End();
			for(; pIt2!=pEnd2; pIt2=oTokenTable.GetNext(pIt2))
			{
				CToken &oToken = oTokenTable.GetItem(pIt2);
				if(oToken.nOwner == oToken.nNode)
				{
					oToken.pToken->OnOwnerChange(oToken.nOwner, (uint32)(-1));
					oToken.pToken->OnRoleChange(false);
					oToken.nOwner = -1;
				}
			}
		}
		m_oMutex.Leave();
	}
}

CAcmTokenModule::CAcmTokenModule()
{
	m_nTimeOut[0] = 0;
	UpdateTimeOut(1000);
	m_bStarted = false;
}

CAcmTokenModule::~CAcmTokenModule()
{
	if(m_oTokens.GetSize())
		FocpAbort(("CAcmTokenModule::~CAcmTokenModule(), m_oTokens isn't empty"));
	g_oMutex.Enter();
	g_pTokenModule = NULL;
	g_oMutex.Leave();
}

FOCP_END();
