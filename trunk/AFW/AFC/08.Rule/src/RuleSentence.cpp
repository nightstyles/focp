
#include "RuleSystem.hpp"

FOCP_BEGIN();

// ---------------------------------------------------
// CRuleSentence
// ---------------------------------------------------
CRuleSentence::CRuleSentence()
{
}

CRuleSentence::~CRuleSentence()
{
}

uint32 CRuleSentence::GetSentenceType() const
{
	return ARF_SENTENCE_INVALID;
}

CRuleSentence* CRuleSentence::Clone() const
{
	CRuleSentence* pRet = new CRuleSentence;
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleSentence::Execute(CRuleStack* pStack) const//0=normal;1=break;2=continue;3=return;
{
	return 0;
}

bool CRuleSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	return true;
}

bool CRuleSentence::Same(const CRuleSentence *pSentence) const
{
	return true;
}

void CRuleSentence::Dump(CString & oDump, uint32 nLevel) const
{
}

uint32 CRuleSentence::GetDepend() const
{
	return 0;
}

// ---------------------------------------------------
// CRuleReturnSentence
// ---------------------------------------------------
CRuleReturnSentence::CRuleReturnSentence()
{
}

CRuleReturnSentence::~CRuleReturnSentence()
{
}

uint32 CRuleReturnSentence::GetSentenceType() const
{
	return ARF_SENTENCE_RETURN;
}

CRuleSentence* CRuleReturnSentence::Clone() const
{
	CRuleSentence* pRet = new CRuleReturnSentence;
	pRet->SetFile(*this);
	return pRet;
}

//0=normal;1=break;2=continue;3=return;
uint32 CRuleReturnSentence::Execute(CRuleStack* pStack) const
{
	return 3;
}

bool CRuleReturnSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleReturnSentence* pRet = dynamic_cast<const CRuleReturnSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	return true;
}

void CRuleReturnSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CFormatString oFmt;
	oFmt.Print("return;\n");
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += oFmt;
}

// ---------------------------------------------------
// CRuleExpSentence
// ---------------------------------------------------
CRuleExpSentence::CRuleExpSentence(const CRuleExpress &oExp)
{
	m_pExp = oExp.Clone();
}

CRuleExpSentence::~CRuleExpSentence()
{
	delete m_pExp;
}

uint32 CRuleExpSentence::GetSentenceType() const
{
	return ARF_SENTENCE_EXPRESS;
}

CRuleSentence* CRuleExpSentence::Clone() const
{
	CRuleSentence* pRet = new CRuleExpSentence(*m_pExp);
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleExpSentence::Execute(CRuleStack* pStack) const
{
	m_pExp->GetExpressValue(pStack);
	return 0;
}

bool CRuleExpSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = m_pExp->Check(oSystem, pChecker);
	return bRet;
}

bool CRuleExpSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleExpSentence* pRet = dynamic_cast<const CRuleExpSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	return m_pExp->Same(*pRet->m_pExp);
}

void CRuleExpSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	m_pExp->Dump(oDump);
	oDump += ";\n";
}

// ---------------------------------------------------
// CRuleCondSentence
// ---------------------------------------------------
CRuleCondSentence::CRuleCondSentence(const CRuleExpress &oCond, CRuleSentence* pIfSentence, CRuleSentence* pElseSentence)
{
	m_pExp = oCond.Clone();
	m_pIfSentence = pIfSentence;
	m_pElseSentence = pElseSentence;
}

CRuleCondSentence::~CRuleCondSentence()
{
	delete m_pExp;
	if(m_pIfSentence)
	{
		delete m_pIfSentence;
		m_pIfSentence = NULL;
	}
	if(m_pElseSentence)
	{
		delete m_pElseSentence;
		m_pElseSentence = NULL;
	}
}

uint32 CRuleCondSentence::GetSentenceType() const
{
	return ARF_SENTENCE_COND;
}

CRuleSentence* CRuleCondSentence::Clone() const
{
	CRuleSentence* pIfSentence = NULL;
	CRuleSentence* pElseSentence = NULL;
	if(m_pIfSentence)
		pIfSentence = m_pIfSentence->Clone();
	if(m_pElseSentence)
		pElseSentence = m_pElseSentence->Clone();
	CRuleSentence* pRet = new CRuleCondSentence(*m_pExp, pIfSentence, pElseSentence);
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleCondSentence::Execute(CRuleStack* pStack) const
{
	void* pCond = m_pExp->GetExpressValue(pStack);
	CRuleType* pCondType = m_pExp->GetExpressType(pStack->pSystem);
	uint32 nCondType = pCondType->TypeCode();
	bool bRet = CRuleExpress::GetCond(nCondType, pCond);
	uint32 nRet = 0;
	if(bRet)
	{
		if(m_pIfSentence)
			nRet = m_pIfSentence->Execute(pStack);
	}
	else
	{
		if(m_pElseSentence)
			nRet = m_pElseSentence->Execute(pStack);
	}
	return nRet;
}

bool CRuleCondSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = m_pExp->Check(oSystem, pChecker);
	if(bRet)
	{
		uint32 nType = m_pExp->GetExpressType(&oSystem)->TypeCode();
		if(nType > ARF_DOUBLE && nType != ARF_STRUCT)
		{
			pChecker->OnError(*m_pExp, "invalid cond express");
			bRet = false;
		}
	}
	if(m_pIfSentence && !m_pIfSentence->Check(oSystem, pChecker, nLoopLevel, nSwitchLevel))
		bRet = false;
	if(m_pElseSentence && !m_pElseSentence->Check(oSystem, pChecker, nLoopLevel, nSwitchLevel))
		bRet = false;
	return bRet;
}

bool CRuleCondSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleCondSentence* pRet = dynamic_cast<const CRuleCondSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	if(!m_pExp->Same(*pRet->m_pExp))
		return false;
	if(m_pIfSentence && !m_pIfSentence->Same(pRet->m_pIfSentence))
		return false;
	if(m_pElseSentence && !m_pElseSentence->Same(pRet->m_pIfSentence))
		return false;
	return true;
}

void CRuleCondSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "if(";
	m_pExp->Dump(oDump);
	oDump += ")\n";
	if(m_pIfSentence)
	{
		if(m_pIfSentence->GetSentenceType() == ARF_SENTENCE_COMPLEX)
			m_pIfSentence->Dump(oDump, nLevel);
		else
			m_pIfSentence->Dump(oDump, nLevel+1);
	}
	else
	{
		CRuleType::DumpLevel(oDump, nLevel+1);
		oDump += ";\n";//¿ÕÓï¾ä
	}
	if(m_pElseSentence)
	{
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "else\n";
		if(m_pElseSentence->GetSentenceType() == ARF_SENTENCE_COMPLEX)
			m_pElseSentence->Dump(oDump, nLevel);
		else
			m_pElseSentence->Dump(oDump, nLevel+1);
	}
}

// ---------------------------------------------------
// CRuleComplexSentence
// ---------------------------------------------------
CRuleComplexSentence::CRuleComplexSentence(bool bTop)
{
	m_bTop = bTop;
}

CRuleComplexSentence::~CRuleComplexSentence()
{
}

uint32 CRuleComplexSentence::GetSentenceType() const
{
	return ARF_SENTENCE_COMPLEX;
}

CRuleSentence* CRuleComplexSentence::Clone() const
{
	CRuleComplexSentence* pRet = new CRuleComplexSentence;
	pRet->m_bTop = m_bTop;
	uint32 nSize  = m_oSentences.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		pRet->AddSentence(m_oSentences[i]->Clone());
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleComplexSentence::Execute(CRuleStack* pStack) const//0=normal;1=break;2=continue;3=return;
{
	uint32 nRet = 0;
	uint32 nSize  = m_oSentences.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		nRet = m_oSentences[i]->Execute(pStack);
		if(nRet)
			break;
	}
	return nRet;
}

bool CRuleComplexSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = true;
	uint32 nSize  = m_oSentences.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!m_oSentences[i]->Check(oSystem, pChecker, nLoopLevel, nSwitchLevel))
			bRet = false;
	}
	return bRet;
}

bool CRuleComplexSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleComplexSentence* pRet = dynamic_cast<const CRuleComplexSentence*>(pSentence);
	if(!pRet)
		return false;
	if(m_bTop != pRet->m_bTop)
		return false;
	uint32 nSize  = m_oSentences.GetSize();
	if(nSize != pRet->m_oSentences.GetSize())
		return false;
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!m_oSentences[i]->Same((const CRuleSentence*)pRet->m_oSentences[i]))
			return false;
	}
	return true;
}

void CRuleComplexSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bTop)
		oDump += "(.\n";
	else
		oDump += "{\n";
	DumpFunc(oDump, nLevel);
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bTop)
		oDump += ".)\n";
	else
		oDump += "}\n";
}

void CRuleComplexSentence::DumpFunc(CString & oDump, uint32 nLevel) const
{
	uint32 nSize  = m_oSentences.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		m_oSentences[i]->Dump(oDump, nLevel+1);
}

CRuleComplexSentence& CRuleComplexSentence::AddSentence(CRuleSentence *pSentence)
{
	m_oSentences.Insert((uint32)(-1), pSentence);
	return *this;
}

// ---------------------------------------------------
// CRuleWhileSentence
// ---------------------------------------------------
CRuleWhileSentence::CRuleWhileSentence(const CRuleExpress &oCond, CRuleSentence* pSentence, bool bDoWhile)
{
	m_bDoWhile = bDoWhile;
	m_pExp = oCond.Clone();
	m_pDoSentence = pSentence;
}

CRuleWhileSentence::~CRuleWhileSentence()
{
	delete m_pExp;
	if(m_pDoSentence)
	{
		delete m_pDoSentence;
		m_pDoSentence = NULL;
	}
}

uint32 CRuleWhileSentence::GetSentenceType() const
{
	return ARF_SENTENCE_WHILE;
}

CRuleSentence* CRuleWhileSentence::Clone() const
{
	CRuleSentence* pSentence = NULL;
	if(m_pDoSentence)
		pSentence = m_pDoSentence->Clone();
	CRuleSentence* pRet = new CRuleWhileSentence(*m_pExp, pSentence, m_bDoWhile);
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleWhileSentence::Execute(CRuleStack* pStack) const
{
	bool bRet;
	uint32 nRet = 0;
	CRuleType* pCondType = m_pExp->GetExpressType(pStack->pSystem);
	uint32 nCondType = pCondType->TypeCode();
	if(m_bDoWhile)
	{
		while(true)
		{
			if(m_pDoSentence)
			{
				nRet = m_pDoSentence->Execute(pStack);
				if(nRet == 1)
				{
					nRet = 0;
					break;
				}
				else if(nRet == 2)
					nRet = 0;
				else if(nRet == 3)
					break;
			}
			void* pCond = m_pExp->GetExpressValue(pStack);
			bRet = CRuleExpress::GetCond(nCondType, pCond);
			if(!bRet)
				break;
		}
	}
	else
	{
		while(true)
		{
			void* pCond = m_pExp->GetExpressValue(pStack);
			bRet = CRuleExpress::GetCond(nCondType, pCond);
			if(!bRet)
				break;
			if(m_pDoSentence)
			{
				nRet = m_pDoSentence->Execute(pStack);
				if(nRet == 1)
				{
					nRet = 0;
					break;
				}
				else if(nRet == 2)
					nRet = 0;
				else if(nRet == 3)
					break;
			}
		}
	}
	return nRet;
}

bool CRuleWhileSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = true;
	nLoopLevel++;
	if(m_bDoWhile)
	{
		if(!m_pDoSentence || m_pDoSentence->GetSentenceType() != ARF_SENTENCE_COMPLEX)
		{
			pChecker->OnError(*this, "invalid do sentence");
			bRet = false;
		}
		else if(!m_pDoSentence->Check(oSystem, pChecker, nLoopLevel, nSwitchLevel))
			bRet = false;
		if(!m_pExp->Check(oSystem, pChecker))
			bRet = false;
		else
		{
			uint32 nType = m_pExp->GetExpressType(&oSystem)->TypeCode();
			if(nType > ARF_DOUBLE && nType != ARF_STRUCT)
			{
				pChecker->OnError(*m_pExp, "invalid cond express");
				bRet = false;
			}
		}
	}
	else
	{
		if(!m_pExp->Check(oSystem, pChecker))
			bRet = false;
		else
		{
			uint32 nType = m_pExp->GetExpressType(&oSystem)->TypeCode();
			if(nType > ARF_DOUBLE && nType != ARF_STRUCT)
			{
				pChecker->OnError(*m_pExp, "invalid cond express");
				bRet = false;
			}
		}
		if(m_pDoSentence && !m_pDoSentence->Check(oSystem, pChecker, nLoopLevel, nSwitchLevel))
			bRet = false;
	}
	nLoopLevel--;
	return bRet;
}

bool CRuleWhileSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleWhileSentence* pRet = dynamic_cast<const CRuleWhileSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	if(!m_pExp->Same(*pRet->m_pExp))
		return false;
	if(m_pDoSentence && !m_pDoSentence->Same(pRet->m_pDoSentence))
		return false;
	return true;
}

void CRuleWhileSentence::Dump(CString & oDump, uint32 nLevel) const
{
	if(m_bDoWhile)
	{
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "do\n";
		m_pDoSentence->Dump(oDump, nLevel);
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "while(";
		m_pExp->Dump(oDump);
		oDump += ");\n";
	}
	else
	{
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "while(";
		m_pExp->Dump(oDump);
		oDump += ")\n";
		if(m_pDoSentence)
		{
			if(m_pDoSentence->GetSentenceType() != ARF_SENTENCE_COMPLEX)
				m_pDoSentence->Dump(oDump, nLevel+1);
			else
				m_pDoSentence->Dump(oDump, nLevel);
		}
		else
		{
			CRuleType::DumpLevel(oDump, nLevel+1);
			oDump += ";\n";
		}
	}
}

// ---------------------------------------------------
// CRuleForSentence
// ---------------------------------------------------
CRuleForSentence::CRuleForSentence(CRuleExpress* pInitExp, CRuleExpress* pCondExp, CRuleExpress* pLoopExp, CRuleSentence* pSentence)
{
	m_pInitExp = pInitExp;
	m_pCondExp = pCondExp;
	m_pLoopExp = pLoopExp;
	m_pDoSentence = pSentence;
}

CRuleForSentence::~CRuleForSentence()
{
	if(m_pInitExp)
	{
		delete m_pInitExp;
		m_pInitExp = NULL;
	}
	if(m_pCondExp)
	{
		delete m_pCondExp;
		m_pCondExp = NULL;
	}
	if(m_pLoopExp)
	{
		delete m_pLoopExp;
		m_pLoopExp = NULL;
	}
	if(m_pDoSentence)
	{
		delete m_pDoSentence;
		m_pDoSentence = NULL;
	}
}

uint32 CRuleForSentence::GetSentenceType() const
{
	return ARF_SENTENCE_FOR;
}

CRuleSentence* CRuleForSentence::Clone() const
{
	CRuleExpress* pInitExp = NULL, * pCondExp = NULL, * pLoopExp = NULL;
	CRuleSentence* pSentence = NULL;
	if(m_pInitExp)
		pInitExp = m_pInitExp->Clone();
	if(m_pCondExp)
		pCondExp = m_pCondExp->Clone();
	if(m_pLoopExp)
		pLoopExp = m_pLoopExp->Clone();
	if(m_pDoSentence)
		pSentence = m_pDoSentence->Clone();
	CRuleSentence* pRet = new CRuleForSentence(pInitExp, pCondExp, pLoopExp, pSentence);
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleForSentence::Execute(CRuleStack* pStack) const
{
	uint32 nRet = 0;
	uint32 nCondType;
	CRuleType* pCondType;
	if(m_pInitExp)
		m_pInitExp->GetExpressValue(pStack);
	if(m_pCondExp)
	{
		pCondType = m_pCondExp->GetExpressType(pStack->pSystem);
		nCondType = pCondType->TypeCode();
	}
	while(true)
	{
		if(m_pCondExp)
		{
			void* pCond = m_pCondExp->GetExpressValue(pStack);
			bool bRet = CRuleExpress::GetCond(nCondType, pCond);
			if(!bRet)
				break;
		}
		if(m_pDoSentence)
		{
			nRet = m_pDoSentence->Execute(pStack);
			if(nRet == 1)
			{
				nRet = 0;
				break;
			}
			else if(nRet == 2)
				nRet = 0;
			else if(nRet == 3)
				break;
		}
		if(m_pLoopExp)
			m_pLoopExp->GetExpressValue(pStack);
	}
	return nRet;
}

bool CRuleForSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = true;
	if(m_pInitExp)
	{
		if(!m_pInitExp->Check(oSystem, pChecker))
			bRet = false;
	}
	if(m_pCondExp)
	{
		if(!m_pCondExp->Check(oSystem, pChecker))
			bRet = false;
		else
		{
			uint32 nType = m_pCondExp->GetExpressType(&oSystem)->TypeCode();
			if(nType > ARF_DOUBLE && nType != ARF_STRUCT)
			{
				pChecker->OnError(*m_pCondExp, "invalid cond express");
				bRet = false;
			}
		}
	}
	if(m_pLoopExp && !m_pLoopExp->Check(oSystem, pChecker))
		bRet = false;
	if(m_pDoSentence && !m_pDoSentence->Check(oSystem, pChecker, nLoopLevel+1, nSwitchLevel))
		bRet = false;
	return bRet;
}

bool CRuleForSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleForSentence* pRet = dynamic_cast<const CRuleForSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	if( (m_pInitExp && !pRet->m_pInitExp) || (!m_pInitExp && pRet->m_pInitExp) )
		return false;
	if(m_pInitExp && !m_pInitExp->Same(*pRet->m_pInitExp))
		return false;
	if( (m_pCondExp && !pRet->m_pCondExp) || (!m_pCondExp && pRet->m_pCondExp) )
		return false;
	if(m_pCondExp && !m_pCondExp->Same(*pRet->m_pCondExp))
		return false;
	if( (m_pDoSentence && !pRet->m_pDoSentence) || (!m_pDoSentence && pRet->m_pDoSentence) )
		return false;
	if(m_pDoSentence && !m_pDoSentence->Same(pRet->m_pDoSentence))
		return false;
	if( (m_pLoopExp && !pRet->m_pLoopExp) || (!m_pLoopExp && pRet->m_pLoopExp) )
		return false;
	if(m_pLoopExp && !m_pLoopExp->Same(*pRet->m_pLoopExp))
		return false;
	return true;
}

void CRuleForSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "for(";
	if(m_pInitExp)
		m_pInitExp->Dump(oDump);
	oDump += ";";
	if(m_pCondExp)
		m_pCondExp->Dump(oDump);
	oDump += ";";
	if(m_pLoopExp)
		m_pLoopExp->Dump(oDump);
	oDump += ")";
	if(m_pDoSentence)
	{
		if(m_pDoSentence->GetSentenceType() != ARF_SENTENCE_COMPLEX)
			m_pDoSentence->Dump(oDump, nLevel+1);
		else
			m_pDoSentence->Dump(oDump, nLevel);
	}
	else
	{
		CRuleType::DumpLevel(oDump, nLevel+1);
		oDump += ";\n";
	}
}

// ---------------------------------------------------
// CRuleSwitchSentence
// ---------------------------------------------------
CRuleSwitchSentence::CRuleSwitchSentence(const CRuleExpress &oSwitchExp):
	CRuleComplexSentence(false)
{
	m_pSwitchExp = oSwitchExp.Clone();
	m_nDefaultLabel = -1;
}

CRuleSwitchSentence::~CRuleSwitchSentence()
{
	delete m_pSwitchExp;
}

uint32 CRuleSwitchSentence::GetSentenceType() const
{
	return ARF_SENTENCE_SWITCH;
}

CRuleSentence* CRuleSwitchSentence::Clone() const
{
	CRuleSwitchSentence* pRet = new CRuleSwitchSentence(*m_pSwitchExp);
	pRet->m_nDefaultLabel = m_nDefaultLabel;
	pRet->m_oCaseLabel = m_oCaseLabel;
	uint32 nSize  = m_oSentences.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		pRet->AddSentence(m_oSentences[i]->Clone());
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleSwitchSentence::Execute(CRuleStack* pStack) const
{
	CRuleType* pSwitchType = m_pSwitchExp->GetExpressType(pStack->pSystem);
	void* pSwitchValue = m_pSwitchExp->GetExpressValue(pStack);
	uint32 nSwitchType = pSwitchType->TypeCode();
	uint64 nSwitch = 0;
	switch(nSwitchType)
	{
	case ARF_CHAR:
		nSwitch = *(char*)pSwitchValue;
		break;
	case ARF_INT8:
		nSwitch = *(int8*)pSwitchValue;
		break;
	case ARF_UINT8:
		nSwitch = *(uint8*)pSwitchValue;
		break;
	case ARF_INT16:
		nSwitch = *(int16*)pSwitchValue;
		break;
	case ARF_UINT16:
		nSwitch = *(uint16*)pSwitchValue;
		break;
	case ARF_INT32:
		nSwitch = *(int32*)pSwitchValue;
		break;
	case ARF_UINT32:
		nSwitch = *(uint32*)pSwitchValue;
		break;
	case ARF_INT64:
		nSwitch = *(int64*)pSwitchValue;
		break;
	case ARF_UINT64:
		nSwitch = *(uint64*)pSwitchValue;
		break;
	}
	uint32 nCase = m_nDefaultLabel;
	CRbTreeNode* pIt = m_oCaseLabel.Find(nSwitch);
	if(pIt != m_oCaseLabel.End())
		nCase = m_oCaseLabel.GetItem(pIt);
	uint32 nRet = 0;
	uint32 nSize = m_oSentences.GetSize();
	for(; nCase<nSize; ++nCase)
	{
		nRet = m_oSentences[nCase]->Execute(pStack);
		if(nRet)
			break;
	}
	if(nRet == 1)
		nRet = 0;
	return nRet;
}

bool CRuleSwitchSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	bool bRet = true;
	if(!m_pSwitchExp->Check(oSystem, pChecker))
		bRet = false;
	else
	{
		uint32 nSwitchType = m_pSwitchExp->GetExpressType(&oSystem)->TypeCode();
		if(nSwitchType < ARF_CHAR || nSwitchType > ARF_UINT64)
		{
			bRet = false;
			pChecker->OnError(*this, "the switch express isn't integer type data");
		}
	}
	if(!CRuleComplexSentence::Check(oSystem, pChecker, nLoopLevel, nSwitchLevel+1))
		bRet = false;
	return bRet;
}

bool CRuleSwitchSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleSwitchSentence* pRet = dynamic_cast<const CRuleSwitchSentence*>(pSentence);
	if(pRet == NULL)
		return false;
	if(!m_pSwitchExp->Same(*pRet->m_pSwitchExp))
		return false;
	if(pRet->m_nDefaultLabel != m_nDefaultLabel)
		return false;
	uint32 nSize = m_oCaseLabel.GetSize();
	if(nSize != pRet->m_oCaseLabel.GetSize())
		return false;
	CRbTreeNode* pIt = m_oCaseLabel.First();
	CRbTreeNode* pEnd = m_oCaseLabel.End();
	CRbTreeNode* pEnd2 = pRet->m_oCaseLabel.End();
	for(; pIt!=pEnd; pIt=m_oCaseLabel.GetNext(pIt))
	{
		uint64 nKey = m_oCaseLabel.GetKey(pIt);
		uint32 nVal = m_oCaseLabel.GetItem(pIt);
		CRbTreeNode* pIt2 = pRet->m_oCaseLabel.Find(nKey);
		if(pIt2 == pEnd2)
			return false;
		if(nVal != pRet->m_oCaseLabel.GetItem(pIt2))
			return false;
	}
	return CRuleComplexSentence::Same(pSentence);
}

void CRuleSwitchSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "switch(";
	m_pSwitchExp->Dump(oDump);
	oDump += ")\n";
	CRuleComplexSentence::Dump(oDump, nLevel);
}

bool CRuleSwitchSentence::SetCaseLable(uint64 nCase)
{
	if(m_oCaseLabel.Find(nCase) == m_oCaseLabel.End())
	{
		m_oCaseLabel[nCase] = m_oSentences.GetSize();
		return true;
	}
	return false;
}

bool CRuleSwitchSentence::SetDefaultLable()
{
	if(m_nDefaultLabel != (uint32)(-1))
	{
		m_nDefaultLabel = m_oSentences.GetSize();
		return true;
	}
	return false;
}

// ---------------------------------------------------
// CRuleBreakSentence
// ---------------------------------------------------
CRuleBreakSentence::CRuleBreakSentence()
{
}

CRuleBreakSentence::~CRuleBreakSentence()
{
}

uint32 CRuleBreakSentence::GetSentenceType() const
{
	return ARF_SENTENCE_BREAK;
}

CRuleSentence* CRuleBreakSentence::Clone() const
{
	CRuleSentence* pRet = new CRuleBreakSentence;
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleBreakSentence::Execute(CRuleStack* pStack) const
{
	return 1;
}

bool CRuleBreakSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	if(nLoopLevel == 0 && nSwitchLevel == 0)
	{
		pChecker->OnError(*this, "the break sentence must be used in the switch, for, while, do-while sentence");
		return false;
	}
	return true;
}

bool CRuleBreakSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleBreakSentence* pRet = dynamic_cast<const CRuleBreakSentence*>(pSentence);
	return pRet != NULL;
}

void CRuleBreakSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "break;\n";
}

// ---------------------------------------------------
// CRuleBreakSentence
// ---------------------------------------------------
CRuleContinueSentence::CRuleContinueSentence()
{
}

CRuleContinueSentence::~CRuleContinueSentence()
{
}

uint32 CRuleContinueSentence::GetSentenceType() const
{
	return ARF_SENTENCE_CONTINUE;
}

CRuleSentence* CRuleContinueSentence::Clone() const
{
	CRuleSentence* pRet = new CRuleContinueSentence;
	pRet->SetFile(*this);
	return pRet;
}

uint32 CRuleContinueSentence::Execute(CRuleStack* pStack) const
{
	return 2;
}

bool CRuleContinueSentence::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel)
{
	if(nLoopLevel == 0)
	{
		pChecker->OnError(*this, "the continue sentence must be used in the for, while and do-while sentence");
		return false;
	}
	return true;
}

bool CRuleContinueSentence::Same(const CRuleSentence *pSentence) const
{
	const CRuleContinueSentence* pRet = dynamic_cast<const CRuleContinueSentence*>(pSentence);
	return pRet != NULL;
}

void CRuleContinueSentence::Dump(CString & oDump, uint32 nLevel) const
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "continue;\n";
}

// ---------------------------------------------------
// CRuleElseSentence
// ---------------------------------------------------
CRuleElseSentence::CRuleElseSentence(CRuleSentence* pSentence)
{
	m_pSentence = pSentence;
}

CRuleElseSentence::~CRuleElseSentence()
{
	if(m_pSentence)
	{
		delete m_pSentence;
		m_pSentence = NULL;
	}
}

CRuleSentence* CRuleElseSentence::Clone() const
{
	CRuleSentence* pSentence = NULL;
	if(m_pSentence)
		pSentence = m_pSentence->Clone();
	pSentence = new CRuleElseSentence(pSentence);
	pSentence->SetFile(*this);
	return pSentence;
}

uint32 CRuleElseSentence::GetDepend() const
{
	return 1;//1=Else,2=case,3=default
}

CRuleSentence* CRuleElseSentence::Detach()
{
	CRuleSentence* pRet = m_pSentence;
	m_pSentence = NULL;
	return pRet;
}

// ---------------------------------------------------
// CRuleDefaultSentence
// ---------------------------------------------------
CRuleDefaultSentence::CRuleDefaultSentence()
{
}

CRuleDefaultSentence::~CRuleDefaultSentence()
{
}

CRuleSentence* CRuleDefaultSentence::Clone() const
{
	CRuleSentence* pSentence = new CRuleDefaultSentence();
	pSentence->SetFile(*this);
	return pSentence;
}

uint32 CRuleDefaultSentence::GetDepend() const
{
	return 3;//1=Else,2=case,3=default
}

// ---------------------------------------------------
// CRuleCaseSentence
// ---------------------------------------------------
CRuleCaseSentence::CRuleCaseSentence(uint64 nVal, bool bError)
{
	m_nVal = nVal;
	m_bError = bError;
}

CRuleCaseSentence::~CRuleCaseSentence()
{
}

CRuleSentence* CRuleCaseSentence::Clone() const
{
	CRuleSentence* pSentence = new CRuleCaseSentence(m_nVal, m_bError);
	pSentence->SetFile(*this);
	return pSentence;
}

uint32 CRuleCaseSentence::GetDepend() const
{
	return 2;//1=Else,2=case,3=default
}

bool CRuleCaseSentence::GetSwitchValue(uint64 &nVal) const
{
	nVal = m_nVal;
	return m_bError;
}

// ---------------------------------------------------
// CRuleFunc
// ---------------------------------------------------
CRuleFunc::CRuleFunc(CRuleSystem *pRuleSystem, const char* sName, const char* sReturnType, ...):
	m_pRuleSystem(pRuleSystem), m_oName(sName), m_oParameters(NULL, NULL), m_oVariables(NULL, NULL)
{
	m_pRetType = m_pRuleSystem->GetType(sReturnType);
	if(!m_pRetType)
		FocpAbort(("CRuleFunc(%s), undefined type '%s'", sName, sReturnType));
	m_pFuncBody = NULL;
	m_fFunc = NULL;

	m_oParameters.AddField(new CRuleParameter(m_pRetType, "result", true));

	CVaList args;
	VaStart(args, sReturnType);
	InitializeParameters(args);
	if(m_pFuncBody)
		InitializeVariables(args);
	VaEnd(args);

	m_pRuleSystem->RegFunc(this);
}

CRuleFunc::CRuleFunc(CRuleSystem *pRuleSystem, const char* sName, CRuleType* pRetType, ...):
	m_pRuleSystem(pRuleSystem), m_oName(sName), m_oParameters(NULL, NULL), m_oVariables(NULL, NULL)
{
	m_pRetType = pRetType;
	if(!m_pRetType)
		FocpAbort(("CRuleFunc(%s), return type is NULL", sName));
	m_pFuncBody = NULL;
	m_fFunc = NULL;

	m_oParameters.AddField(new CRuleParameter(m_pRetType, "result", true));

	CVaList args;
	VaStart(args, pRetType);
	InitializeParameters(args);
	if(m_pFuncBody)
		InitializeVariables(args);
	VaEnd(args);

	m_pRuleSystem->RegFunc(this);
}

CRuleFunc::~CRuleFunc()
{
	if(m_pFuncBody)
	{
		delete m_pFuncBody;
		m_pFuncBody = NULL;
	}
	if(m_pRetType->TypeCode() == ARF_VECTOR)
	{
		delete m_pRetType;
		m_pRetType = NULL;
	}
}

void CRuleFunc::InitializeParametersAndVariables(CVaList& args)
{
	InitializeParameters(args);
	if(m_pFuncBody)
		InitializeVariables(args);
}

CRuleStruct* CRuleFunc::GetParameters()
{
	return &m_oParameters;
}

CRuleStruct* CRuleFunc::GetVariables()
{
	return &m_oVariables;
}

void CRuleFunc::SetSentence(CRuleComplexSentence* pSentence)
{
	if(m_pFuncBody)
		delete m_pFuncBody;
	m_pFuncBody = pSentence;
}

bool CRuleFunc::Implemented()
{
	return m_pFuncBody || m_pFuncBody;
}

const char* CRuleFunc::GetName() const
{
	return m_oName.GetStr();
}

CRuleType* CRuleFunc::GetRetType() const
{
	return (CRuleType*)m_pRetType;
}

bool CRuleFunc::Check(CRuleChecker* pChecker)
{
	bool bRet = true;
	if(m_pFuncBody)
	{
		if(!m_pFuncBody->Check(*m_pRuleSystem, pChecker, 0, 0))
			bRet = false;
	}
	else
	{
		m_fFunc = m_pRuleSystem->GetHost(m_oName.GetStr());
		if(!m_fFunc)
		{
			pChecker->OnError(*this, "the rule function '%s' is undefined", m_oName.GetStr());
			bRet = false;
		}
	}
	m_oParameters.FinishDefine();
	m_oVariables.FinishDefine();
	if(!m_oParameters.Check(pChecker))
		bRet = false;
	if(!m_oVariables.Check(pChecker))
		bRet = false;
	return bRet;
}

void CRuleFunc::Call(CRuleStack &oStack) const
{
	if(m_pFuncBody == NULL)
		m_fFunc(oStack);
	else
	{
		void* pVarv = NULL;
		uint32 nVarSize = m_oVariables.GetSize();
		if(nVarSize)
		{
			pVarv = new char[nVarSize];
			m_oVariables.InitObject(pVarv);
		}
		CRuleProc *pProc = oStack.pProc;
		CRuleFunc* pFunc = oStack.pFunc;
		void* pOldVarv = oStack.pVarv;

		oStack.pProc = NULL;
		oStack.pFunc = (CRuleFunc*)this;
		oStack.pVarv = pVarv;

		m_pFuncBody->Execute(&oStack);

		oStack.pVarv = pOldVarv;
		oStack.pProc = pProc;
		oStack.pFunc = pFunc;

		if(pVarv)
		{
			m_oVariables.ClearObject(pVarv);
			delete[] (char*)pVarv;
		}
	}
}

void CRuleFunc::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += m_pRetType->GetName();
	oDump += " ";
	oDump += m_oName;
	m_oParameters.Dump(oDump, false);
	if(m_pFuncBody == NULL)
		oDump += ";\n";
	else
	{
		oDump += "\n";
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "{\n";
		if(m_oVariables.GetFieldCount())
			m_oVariables.Dump(oDump, nLevel);
		m_pFuncBody->DumpFunc(oDump, nLevel);
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "}\n";
	}
}

void CRuleFunc::InitializeParameters(CVaList& args)
{
	bool b;
	const char *s;
	CRuleType* pType;
	while((s = VaArg<const char*>(args)))
	{
		CString oIdentifier;
		CString::GetCIdentifier(oIdentifier, s);
		pType = m_pRuleSystem->GetType(oIdentifier.GetStr());
		if(!pType)
			FocpAbort(("CRuleFunc::InitializeParameters(), undefined type '%s'", s));
		s = CString::SkipSpace(s);
		b = (s[0] == '&');
		if(b)
			s++;
		CString::GetCIdentifier(oIdentifier, s);
		s = oIdentifier.GetStr();
		if(m_oParameters.FindField(s))
			FocpAbort(("CRuleFunc::InitializeParameters(), redefine parameter '%s'", s));
		m_oParameters.AddField(new CRuleParameter(pType, s, b));
	}
	m_oParameters.FinishDefine();
}

void CRuleFunc::InitializeVariables(CVaList& args)
{
	const char *s;
	CRuleType* pType;
	while((s = VaArg<const char*>(args)))
	{
		CString oIdentifier;
		CString::GetCIdentifier(oIdentifier, s);
		pType = m_pRuleSystem->GetType(oIdentifier.GetStr());
		if(!pType)
			FocpAbort(("CRuleFunc::InitializeParameters(), undefined type '%s'", s));
		CString::GetCIdentifier(oIdentifier, s);
		s = oIdentifier.GetStr();
		if(m_oVariables.FindField(s))
			FocpAbort(("CRuleFunc::InitializeParameters(), redefine parameter '%s'", s));
		m_oVariables.AddField(new CRuleVariable(pType, s));
	}
	m_oVariables.FinishDefine();
}

FOCP_END();
