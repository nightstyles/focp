
#include "Functor.hpp"

FOCP_BEGIN();

CVoidExpression::CVoidExpression()
{
}

CVoidExpression::~CVoidExpression()
{
}

CVoidExpression* CVoidExpression::Clone() const
{
	return new CVoidExpression;
}

void CVoidExpression::Compute()
{
}

bool CCondExpression::GetCond()
{
	return false;
}

uint32 CIntegerExpression::GetInteger()
{
	return 0;
}

double CFloatExpression::GetFloat()
{
	return 0.0;
}

FUN_API CUseExpression<void> _use(const CVoidExpression& oExp)
{
	return CUseExpression<void>(oExp);
}

CSentence::CSentence()
{
	m_pNext = NULL;
	m_pParent = NULL;
}

CSentence::~CSentence()
{
}

CSentence::CSentence(const CSentence& oSrc)
{
	m_pNext = NULL;
	m_pParent = NULL;
}

CSentence* CSentence::Clone() const
{
	return new CSentence;
}

uint32 CSentence::Call()
{
	return FOCP_SENTENCE_FINISH;
}

uint32 CSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_INVALID_SENTENCE;
}

void CSentence::Check()
{
	if(GetSentenceType() == FOCP_DETAIL_NAME::FOCP_INVALID_SENTENCE)
		FocpThrow(Sentence::Check);
}

CComplexSentence::~CComplexSentence()
{
	while(m_pHead)
	{
		m_pTail = m_pHead->m_pNext;
		delete m_pHead;
		m_pHead = m_pTail;
	}
}

CComplexSentence::CComplexSentence()
{
	m_pHead = NULL;
	m_pTail = NULL;
}

CComplexSentence::CComplexSentence(const CComplexSentence& oSrc)
{
	m_pHead = NULL;
	m_pTail = NULL;
	CSentence* pSentence = oSrc.m_pHead;
	while(pSentence)
	{
		AddSentence(*pSentence);
		pSentence = pSentence->m_pNext;
	}
}

CSentence* CComplexSentence::Clone() const
{
	return new CComplexSentence(*this);
}

uint32 CComplexSentence::Call()
{
	CSentence* pSentence = m_pHead;
	while(pSentence)
	{
		uint32 nRet = pSentence->Call();
		if(nRet)
			return nRet;
		pSentence = pSentence->m_pNext;
	}
	return FOCP_SENTENCE_FINISH;
}

CSentence* CComplexSentence::Begin()
{
	return m_pHead;
}

uint32 CComplexSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_COMPLEX_SENTENCE;
}

CComplexSentence& CComplexSentence::AddSentence(const CSentence& oSentence)
{
	uint32 nSentenceType;
	nSentenceType = oSentence.GetSentenceType();
	if(nSentenceType == FOCP_DETAIL_NAME::FOCP_INVALID_SENTENCE)
		FocpThrow(ComplexSentence::AddSentence);
	if(nSentenceType == FOCP_DETAIL_NAME::FOCP_COMPLEX_SENTENCE)
	{
		CComplexSentence& oComplexSentence = (CComplexSentence&)oSentence;
		CSentence* pSentence = oComplexSentence.m_pHead;
		while(pSentence)
		{
			AddSentence(*pSentence);
			pSentence = pSentence->m_pNext;
		}
	}
	else
	{
		CSentence* pSentence = oSentence.Clone();
		pSentence->m_pParent = this;
		if(m_pTail)
			m_pTail->m_pNext = pSentence;
		else
			m_pHead = pSentence;
		m_pTail = pSentence;
	}
	return *this;
}

void CComplexSentence::Check()
{
	CSentence* pSentence = m_pHead;
	while(pSentence)
	{
		pSentence->Check();
		pSentence = pSentence->m_pNext;
	}
}

CExpressionSentence::CExpressionSentence(const CVoidExpression& oExp)
{
	m_pExp = oExp.Clone();
}

CExpressionSentence::~CExpressionSentence()
{
	delete m_pExp;
}

CSentence* CExpressionSentence::Clone() const
{
	return new CExpressionSentence(*m_pExp);
}

uint32 CExpressionSentence::Call()
{
	m_pExp->Compute();
	return FOCP_SENTENCE_FINISH;
}

uint32 CExpressionSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_EXPRESSION_SENTENCE;
}

CIfSentence::CIfSentence(const CCondExpression& oExp)
	:_then(this),_else(this)
{
	m_pCond = (CCondExpression*)oExp.Clone();
}

CIfSentence::CIfSentence(const CIfSentence& oSrc)
	:_then(this), _else(this)
{
	m_pCond = (CCondExpression*)oSrc.m_pCond->Clone();
	_then->AddSentence(oSrc._then.body());
	_else->AddSentence(oSrc._else.body());
}

CIfSentence::~CIfSentence()
{
	delete m_pCond;
}

CSentence* CIfSentence::Clone() const
{
	return new CSentence(*this);
}

uint32 CIfSentence::Call()
{
	if(m_pCond->GetCond())
		return _then->Call();
	return _else->Call();
}

CIfSentence& CIfSentence::operator[](const CSentence& oSentence)
{
	return _then[oSentence];
}

uint32 CIfSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_IF_SENTENCE;
}

void CIfSentence::Check()
{
	_then->Check();
	_else->Check();
}

FOCP_DETAIL_BEGIN();
CWhileExpression::CWhileExpression(CDoWhileSentence* pSentence)
{
	m_pSentence = pSentence;
	m_pCond = NULL;
}

CWhileExpression::~CWhileExpression()
{
	if(m_pCond)
		delete m_pCond;
}

CDoWhileSentence& CWhileExpression::operator()(const CCondExpression& oExp)
{
	if(m_pCond)
		delete m_pCond;
	m_pCond = (CCondExpression*)oExp.Clone();
	return *m_pSentence;
}

const CCondExpression* CWhileExpression::GetCond() const
{
	return m_pCond;
}

CWhileExpression::operator bool()
{
	if(m_pCond == NULL)
		return true;
	return m_pCond->GetCond();
}

FOCP_DETAIL_END();

CDoWhileSentence::CDoWhileSentence()
	:_body(this), _while(this)
{
}

CDoWhileSentence::CDoWhileSentence(const CDoWhileSentence& oSrc)
	:_body(this), _while(this)
{
	const CCondExpression* pExp = oSrc._while.GetCond();
	_body->AddSentence(oSrc._body.body());
	if(pExp)
		_while(*pExp);
}

CDoWhileSentence::~CDoWhileSentence()
{
}

CSentence* CDoWhileSentence::Clone() const
{
	return new CDoWhileSentence(*this);
}

uint32 CDoWhileSentence::Call()
{
	do
	{
		uint32 nRet = _body->Call();
		if(nRet == FOCP_SENTENCE_RETURN)
			return FOCP_SENTENCE_RETURN;
		if(nRet == FOCP_SENTENCE_BREAK)
			break;
	}while(_while);
	return FOCP_SENTENCE_FINISH;
}

CDoWhileSentence& CDoWhileSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

uint32 CDoWhileSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_DOWHILE_SENTENCE;
}

void CDoWhileSentence::Check()
{
	_body->Check();
}

CWhileSentence::CWhileSentence(const CCondExpression& oExp)
	:_body(this)
{
	m_pCond = (CCondExpression*)oExp.Clone();
}

CWhileSentence::~CWhileSentence()
{
	delete m_pCond;
}

CWhileSentence::CWhileSentence(const CWhileSentence& oSrc)
	:_body(this)
{
	m_pCond = (CCondExpression*)oSrc.m_pCond->Clone();
	_body->AddSentence(oSrc._body.body());
}

CSentence* CWhileSentence::Clone() const
{
	return new CWhileSentence(*this);
}

uint32 CWhileSentence::Call()
{
	while(m_pCond->GetCond())
	{
		uint32 nRet = _body->Call();
		if(nRet == FOCP_SENTENCE_BREAK)
			break;
		if(nRet == FOCP_SENTENCE_RETURN)
			return FOCP_SENTENCE_RETURN;
	}
	return FOCP_SENTENCE_FINISH;
}

CWhileSentence& CWhileSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

uint32 CWhileSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_WHILE_SENTENCE;
}

void CWhileSentence::Check()
{
	_body->Check();
}

FOCP_DETAIL_BEGIN();
CForExpression::~CForExpression()
{
	if(m_pExp)
		delete m_pExp;
}
CForExpression::CForExpression(CForSentence* pSentence)
{
	m_pSentence = pSentence;
	m_pExp = NULL;
}
CForSentence& CForExpression::operator()(const CVoidExpression& oExp)
{
	if(m_pExp)
		delete m_pExp;
	m_pExp = oExp.Clone();
	return *m_pSentence;
}

const CVoidExpression* CForExpression::GetExp() const
{
	return m_pExp;
}
void CForExpression::Compute()
{
	if(m_pExp)
		m_pExp->Compute();
}
FOCP_DETAIL_END();

CForSentence::~CForSentence()
{
	delete m_pCond;
}

CForSentence::CForSentence(const CCondExpression& oCond)
	:_body(this),_init(this),_loop(this)
{
	m_pCond = (CCondExpression*)oCond.Clone();
}

CForSentence::CForSentence(const CForSentence &oSrc)
	:_body(this),_init(this),_loop(this)
{
	const CVoidExpression* pExp;
	m_pCond = (CCondExpression*)oSrc.m_pCond->Clone();
	pExp = oSrc._init.GetExp();
	if(pExp)
		_init(*pExp);
	pExp = oSrc._loop.GetExp();
	if(pExp)
		_loop(*pExp);
}

CSentence* CForSentence::Clone() const
{
	return new CForSentence(*this);
}

uint32 CForSentence::Call()
{
	for(_init.Compute(); m_pCond->GetCond(); _loop.Compute())
	{
		uint32 nRet = _body->Call();
		if(nRet == FOCP_SENTENCE_BREAK)
			break;
		if(nRet == FOCP_SENTENCE_RETURN)
			return FOCP_SENTENCE_RETURN;
	}
	return FOCP_SENTENCE_FINISH;
}

CForSentence& CForSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

uint32 CForSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_FOR_SENTENCE;
}

void CForSentence::Check()
{
	_body->Check();
}

CSwitchSentence::~CSwitchSentence()
{
	delete m_pExp;
}

CSwitchSentence::CSwitchSentence(const CIntegerExpression& oExp)
	:_body(this)
{
	m_pExp = (CIntegerExpression*)oExp.Clone();
	m_pDefault = NULL;
}

CSwitchSentence::CSwitchSentence(const CSwitchSentence& oSrc)
	:_body(this)
{
	m_pExp = (CIntegerExpression*)oSrc.m_pExp->Clone();
	m_pDefault = NULL;
	_body->AddSentence(oSrc._body.body());
}

CSentence* CSwitchSentence::Clone() const
{
	return new CSwitchSentence(*this);
}

uint32 CSwitchSentence::Call()
{
	uint32 nRet, nValue;
	nValue = m_pExp->GetInteger();
	CSentence* pSentence = _body->Begin();
	bool bFound = false;
	while(pSentence)
	{
		if(pSentence->GetSentenceType() == FOCP_DETAIL_NAME::FOCP_CASE_SENTENCE)
		{
			if(((CCaseSentence*)pSentence)->m_nValue == nValue)
			{
				bFound = true;
loop:
				nRet = pSentence->Call();
				if(nRet == FOCP_SENTENCE_BREAK)
					break;
				if(nRet == FOCP_SENTENCE_CONTINUE)
					return FOCP_SENTENCE_CONTINUE;
				if(nRet == FOCP_SENTENCE_RETURN)
					return FOCP_SENTENCE_RETURN;
				pSentence = pSentence->m_pNext;
				if(pSentence)
					goto loop;
				break;
			}
		}
		pSentence = pSentence->m_pNext;
	}
	if(bFound == false && m_pDefault)
	{
		pSentence = m_pDefault;
loop2:
		nRet = pSentence->Call();
		if(nRet == FOCP_SENTENCE_CONTINUE)
			return FOCP_SENTENCE_CONTINUE;
		if(nRet == FOCP_SENTENCE_RETURN)
			return FOCP_SENTENCE_RETURN;
		if(nRet == FOCP_SENTENCE_FINISH)
		{
			pSentence = pSentence->m_pNext;
			if(pSentence)
				goto loop2;
		}
	}
	return FOCP_SENTENCE_FINISH;
}

CSwitchSentence& CSwitchSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

uint32 CSwitchSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_SWITCH_SENTENCE;
}

void CSwitchSentence::Check()
{
	CSentence* pSentence = _body->Begin();
	while(pSentence)
	{
		uint32 nType = pSentence->GetSentenceType();
		if(nType == FOCP_DETAIL_NAME::FOCP_DEFAULT_SENTENCE)
		{
			if(m_pDefault)
				FocpThrow(SwitchSentence::Check);
			m_pDefault = (CDefaultSentence*)pSentence;
		}
		else if(nType != FOCP_DETAIL_NAME::FOCP_CASE_SENTENCE)
			FocpThrow(SwitchSentence::Check);
		pSentence->Check();
		pSentence = pSentence->m_pNext;
	}
}

CDefaultSentence::CDefaultSentence()
	:_body(this)
{
}

CDefaultSentence::CDefaultSentence(const CDefaultSentence& oSrc)
	:_body(this)
{
	_body->AddSentence(oSrc._body.body());
}

CDefaultSentence::~CDefaultSentence()
{
}

CSentence* CDefaultSentence::Clone() const
{
	return new CDefaultSentence(*this);
}

uint32 CDefaultSentence::Call()
{
	return _body->Call();
}

CDefaultSentence& CDefaultSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

uint32 CDefaultSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_DEFAULT_SENTENCE;
}

void CDefaultSentence::Check()
{
	if(m_pParent == NULL)
		FocpThrow(DefaultSentence::Check);
	if(m_pParent->GetSentenceType() != FOCP_DETAIL_NAME::FOCP_SWITCH_SENTENCE)
		FocpThrow(DefaultSentence::Check);
	_body->Check();
}

CCaseSentence::~CCaseSentence()
{
}

CCaseSentence::CCaseSentence(uint32 nValue)
	:_body(this)
{
	m_nValue = nValue;
}

CCaseSentence::CCaseSentence(const CCaseSentence& oSrc)
	:_body(this)
{
	m_nValue = oSrc.m_nValue;
}

CSentence* CCaseSentence::Clone() const
{
	return new CCaseSentence(*this);
}

uint32 CCaseSentence::Call()
{
	return _body->Call();
}

uint32 CCaseSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_CASE_SENTENCE;
}

CCaseSentence& CCaseSentence::operator[](const CSentence& oSentence)
{
	return _body[oSentence];
}

void CCaseSentence::Check()
{
	if(m_pParent == NULL)
		FocpThrow(CaseSentence::Check);
	if(m_pParent->GetSentenceType() != FOCP_DETAIL_NAME::FOCP_SWITCH_SENTENCE)
		FocpThrow(CaseSentence::Check);
	_body->Check();
}

CBreakSentence::~CBreakSentence()
{
}

CBreakSentence::CBreakSentence()
{
}

CBreakSentence::CBreakSentence(const CBreakSentence& oSrc)
{
}

CSentence* CBreakSentence::Clone() const
{
	return new CBreakSentence;
}

uint32 CBreakSentence::Call()
{
	return FOCP_SENTENCE_BREAK;
}

uint32 CBreakSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_BREAK_SENTENCE;
}

void CBreakSentence::Check()
{
	CSentence* pJmpSentence = NULL;
	CSentence* pParent = m_pParent;
	while(pParent)
	{
		uint32 nSentenceType = pParent->GetSentenceType();
		switch(nSentenceType)
		{
		case FOCP_DETAIL_NAME::FOCP_DOWHILE_SENTENCE:
		case FOCP_DETAIL_NAME::FOCP_WHILE_SENTENCE:
		case FOCP_DETAIL_NAME::FOCP_FOR_SENTENCE:
		case FOCP_DETAIL_NAME::FOCP_SWITCH_SENTENCE:
			pJmpSentence = pParent;
			break;
		}
		if(pJmpSentence)
			break;
		pParent = pParent->m_pParent;
	}
	if(pJmpSentence == NULL)
		FocpThrow(BreakSentence::Check);
}

CContinueSentence::~CContinueSentence()
{
}

CContinueSentence::CContinueSentence()
{
}

CContinueSentence::CContinueSentence(const CContinueSentence& oSrc)
{
}

CSentence* CContinueSentence::Clone() const
{
	return new CContinueSentence;
}

uint32 CContinueSentence::Call()
{
	return FOCP_SENTENCE_CONTINUE;
}

uint32 CContinueSentence::GetSentenceType() const
{
	return FOCP_DETAIL_NAME::FOCP_CONTINUE_SENTENCE;
}

void CContinueSentence::Check()
{
	CSentence* pJmpSentence = NULL;
	CSentence* pParent = m_pParent;
	while(pParent)
	{
		uint32 nSentenceType = pParent->GetSentenceType();
		switch(nSentenceType)
		{
		case FOCP_DETAIL_NAME::FOCP_DOWHILE_SENTENCE:
		case FOCP_DETAIL_NAME::FOCP_WHILE_SENTENCE:
		case FOCP_DETAIL_NAME::FOCP_FOR_SENTENCE:
			pJmpSentence = pParent;
			break;
		}
		if(pJmpSentence)
			break;
		pParent = pParent->m_pParent;
	}
	if(pJmpSentence == NULL)
		FocpThrow(ContinueSentence::Check);
}

FUN_API CComplexSentence operator,(CSentence& oLeft, CSentence& oRight)
{
	return CComplexSentence().AddSentence(oLeft).AddSentence(oRight);
}

FUN_API CComplexSentence operator,(CSentence& oLeft, CVoidExpression& oRight)
{
	return CComplexSentence().AddSentence(oLeft).AddSentence(_exp(oRight));
}

FUN_API CComplexSentence& operator,(CComplexSentence& oLeft, CSentence& oRight)
{
	return oLeft.AddSentence(oRight);
}

FUN_API CComplexSentence operator,(CComplexSentence& oLeft, CVoidExpression& oRight)
{
	return oLeft.AddSentence(_exp(oRight));
}

FUN_API CExpressionSentence _exp(CVoidExpression& oExp)
{
	return CExpressionSentence(oExp);
}

FUN_API CIfSentence _if(const CCondExpression& nCond)
{
	return CIfSentence(nCond);
}

FUN_API CDoWhileSentence _do()
{
	return CDoWhileSentence();
}

FUN_API CWhileSentence _while(const CCondExpression& oExp)
{
	return CWhileSentence(oExp);
}

FUN_API CForSentence _for(const CCondExpression &oCond)
{
	return CForSentence(oCond);
}

FUN_API CSwitchSentence _switch(const CIntegerExpression& oExp)
{
	return CSwitchSentence(oExp);
}

FUN_API CDefaultSentence _default()
{
	return CDefaultSentence();
}

FUN_API CCaseSentence _case(uint32 nValue)
{
	return CCaseSentence(nValue);
}

FUN_API CBreakSentence _break()
{
	return CBreakSentence();
}

FUN_API CContinueSentence _continue()
{
	return CContinueSentence();
}

FOCP_END();
