
#include "RuleSystem.hpp"

FOCP_BEGIN();

// ---------------------------------------------------
// CRule
// ---------------------------------------------------
CRule::CRule()
{
}

CRule::CRule(const CRule& oRule)
{
	SetFile(oRule);
}

CRule::~CRule()
{
}

CRule* CRule::Clone() const
{
	return new CRule(*this);
}

uint32 CRule::Match(CRuleStack &oStack, bool bMust) const
{
	return 0;
}

bool CRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	return true;
}

bool CRule::Same(const CRule *pRule)
{
	return true;
}

bool CRule::Merge(const CRule *pRule)
{
	return Same(pRule);
}

CRule* CRule::Optimize()
{
	return this;
}

void CRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "success\n";
}

uint32 CRule::GetRuleType() const
{
	return ARF_RULE_ATOM;
}

uint32 CRule::Call(CRuleStack &oStack, bool bMust) const
{
	uint32 nRet = Match(oStack, bMust);
	if(ARF_RULE_ATOM==GetRuleType())
	{
		if(nRet > 1)
			nRet = 1;
		if(nRet == 1 && bMust)
			nRet = 2;
	}
	return nRet;
}

COrRule CRule::operator|(const CRule& oRule)
{
	if(GetRuleType() == ARF_RULE_OR)
	{
		COrRule oRet(*(const COrRule*)this);
		if(oRule.GetRuleType() == ARF_RULE_OR)
		{
			const COrRule &oSrc = (const COrRule&)oRule;
			uint32 nSize = oSrc.GetSize();
			for(uint32 i=0; i<nSize; ++i)
			{
				const CRule* pRule = oSrc.GetRule(i);
				oRet.InsertRule(*pRule);
			}
		}
		else
			oRet.InsertRule(oRule);
		return oRet;
	}
	COrRule oRet;
	oRet.InsertRule(*this);
	if(oRule.GetRuleType() == ARF_RULE_OR)
	{
		const COrRule &oSrc = (const COrRule&)oRule;
		uint32 nSize = oSrc.GetSize();
		for(uint32 i=0; i<nSize; ++i)
		{
			const CRule* pRule = oSrc.GetRule(i);
			oRet.InsertRule(*pRule);
		}
		return oRet;
	}
	else
		oRet.InsertRule(oRule);
	return oRet;
}

CSequenceRule CRule::operator>>(const CRule& oRule)
{
	if(GetRuleType() == ARF_RULE_SEQUENCE)
	{
		CSequenceRule oRet(*(const COrRule*)this);
		if(oRule.GetRuleType() == ARF_RULE_SEQUENCE)
		{
			const CSequenceRule &oSrc = (const CSequenceRule&)oRule;
			uint32 nSize = oSrc.GetSize();
			for(uint32 i=0; i<nSize; ++i)
			{
				const CRule* pRule = oSrc.GetRule(i);
				oRet.InsertRule(*pRule);
			}
		}
		else
			oRet.InsertRule(oRule);
		return oRet;
	}
	CSequenceRule oRet;
	oRet.InsertRule(*this);
	if(oRule.GetRuleType() == ARF_RULE_SEQUENCE)
	{
		const CSequenceRule &oSrc = (const CSequenceRule&)oRule;
		uint32 nSize = oSrc.GetSize();
		for(uint32 i=0; i<nSize; ++i)
		{
			const CRule* pRule = oSrc.GetRule(i);
			oRet.InsertRule(*pRule);
		}
		return oRet;
	}
	else
		oRet.InsertRule(oRule);
	return oRet;
}

CLoopRule CRule::operator*()
{
	return CLoopRule(*this, true);
}

CLoopRule CRule::operator-()
{
	return CLoopRule(*this, false);
}

// ---------------------------------------------------
// CRuleProc
// ---------------------------------------------------
CRuleProc::CRuleProc(CRuleSystem *pRuleSystem, const char* sName, CRule *pRule, bool bMain, ...):
	m_pRuleSystem(pRuleSystem), m_oName(sName), m_bMain(bMain), m_oParameters(NULL, NULL), m_oVariables(NULL, NULL)
{
	CVaList args;

	m_pProcBody = pRule;
	m_fFunc = NULL;

	VaStart(args, bMain);
	if(!bMain)
		InitializeParameters(args);
	if(m_pProcBody)
		InitializeVariables(args);
	VaEnd(args);

	m_pRuleSystem->RegRule(this);
}

CRuleProc::~CRuleProc()
{
	if(m_pProcBody)
	{
		delete m_pProcBody;
		m_pProcBody = NULL;
	}
}

void CRuleProc::InitializeParametersAndVariables(CVaList& args)
{
	if(!m_bMain)
		InitializeParameters(args);
	if(m_pProcBody)
		InitializeVariables(args);
}

CRuleStruct* CRuleProc::GetParameters()
{
	return &m_oParameters;
}

CRuleStruct* CRuleProc::GetVariables()
{
	return &m_oVariables;
}

void CRuleProc::SetRule(CRule* pRule)
{
	if(m_pProcBody)
		delete m_pProcBody;
	m_pProcBody = pRule;
}

void CRuleProc::InitializeParameters(CVaList& args)
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
			FocpAbort(("CRuleProc::InitializeParameters(), undefined type '%s'", s));
		s = CString::SkipSpace(s);
		b = (s[0] == '&');
		if(b)
			s++;
		CString::GetCIdentifier(oIdentifier, s);
		s = oIdentifier.GetStr();
		if(m_oParameters.FindField(s))
			FocpAbort(("CRuleProc::InitializeParameters(), redefine parameter '%s'", s));
		m_oParameters.AddField(new CRuleParameter(pType, s, b));
	}
	m_oParameters.FinishDefine();
}

void CRuleProc::InitializeVariables(CVaList& args)
{
	const char *s;
	CRuleType* pType;
	while((s = VaArg<const char*>(args)))
	{
		CString oIdentifier;
		CString::GetCIdentifier(oIdentifier, s);
		pType = m_pRuleSystem->GetType(oIdentifier.GetStr());
		if(!pType)
			FocpAbort(("CRuleProc::InitializeParameters(), undefined type '%s'", s));
		CString::GetCIdentifier(oIdentifier, s);
		s = oIdentifier.GetStr();
		if(m_oVariables.FindField(s))
			FocpAbort(("CRuleProc::InitializeParameters(), redefine parameter '%s'", s));
		m_oVariables.AddField(new CRuleVariable(pType, s));
	}
	m_oVariables.FinishDefine();
}

const char* CRuleProc::GetName() const
{
	return m_oName.GetStr();
}

bool CRuleProc::IsMain() const
{
	return m_bMain;
}

void CRuleProc::Optimize()
{
	if(m_pProcBody)
	{
		CRule* pRule = m_pProcBody->Optimize();
		if(pRule != m_pProcBody)
		{
			delete m_pProcBody;
			m_pProcBody = pRule;
		}
	}
}

bool CRuleProc::Check(CRuleChecker* pChecker)
{
	bool bRet = true;
	if(m_bMain && m_oParameters.GetFieldCount())
	{
		pChecker->OnError(*this, "the main rule procedure '%s' can't include any parameter", m_oName.GetStr());
		bRet = false;
	}
	if(m_pProcBody)
	{
		if(!m_pProcBody->Check(*m_pRuleSystem, pChecker, 0))
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

void CRuleProc::Before(CRuleModule* pModule)const
{
}

uint32 CRuleProc::After(CRuleModule* pModule)const
{
	return 0;
}

void CRuleProc::OnSuccess(CRuleModule* pModule)const
{
}

void CRuleProc::OnFailure(CRuleModule* pModule)const
{
}

uint32 CRuleProc::Call(CRuleStack &oStack, bool bMust) const
{
	if(!IsMain())
		return DoCall(oStack, bMust);
	CRuleModule* pModule = oStack.pModule;
	Before(pModule);
	pModule->Push();
	if(pModule->IsBroken())
	{
		pModule->Pop(0);
		return 0;
	}
	uint32 nRet = DoCall(oStack, bMust);
	if(nRet == 0)
		nRet = After(pModule);
	switch(nRet)
	{
	case 0:
		OnSuccess(pModule);
		break;
	case 2:
		OnFailure(pModule);
		break;
	}
	pModule->Pop(nRet);
	return nRet;
}

uint32 CRuleProc::DoCall(CRuleStack &oStack, bool bMust) const
{
	if(m_pProcBody)
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

		oStack.pProc = (CRuleProc*)this;
		oStack.pFunc = NULL;
		oStack.pVarv = pVarv;

		uint32 nRet = m_pProcBody->Call(oStack, bMust);

		oStack.pVarv = pOldVarv;
		oStack.pProc = pProc;
		oStack.pFunc = pFunc;

		if(pVarv)
		{
			m_oVariables.ClearObject(pVarv);
			delete[] (char*)pVarv;
		}

		return nRet;
	}

	m_fFunc(oStack);
	return 0;
}

void CRuleProc::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bMain)
		oDump += "Main ";
	oDump += m_oName;
	if(m_oParameters.GetFieldCount())
		m_oParameters.Dump(oDump);
	bool bHaveEqual = false;
	if(m_oVariables.GetFieldCount())
	{
		oDump += " = ";
		m_oVariables.Dump(oDump);
		bHaveEqual = true;
	}
	if(m_pProcBody)
	{
		if(!bHaveEqual)
			oDump += " = \n";
		else
			oDump += "\n";
		m_pProcBody->Dump(oDump, nLevel);
	}
	if(m_fFunc)
		oDump += " .\n";
}

// ---------------------------------------------------
// CCallRule
// ---------------------------------------------------
CCallRule::CCallRule(const char* sName, ...)
{
	if(!sName || !sName[0])
		FocpAbort(("CRuleCallExpress::CRuleCallExpress(), invalid rule/func name"));
	m_oName = sName;
	CRuleExpress* exp;
	CVaList args;
	VaStart(args, sName);
	while((exp=VaArg<CRuleExpress*>(args)))
		m_oArgv.Insert((uint32)(-1), exp);
	VaEnd(args);
	m_pProc = NULL;
}

CCallRule::CCallRule(const CCallRule& oRule):
	CRule(oRule), m_oName(oRule.m_oName)
{
	m_pProc = NULL;
	uint32 i, nSize = oRule.m_oArgv.GetSize();
	for(i=0; i<nSize; ++i)
		m_oArgv.Insert((uint32)(-1), oRule.m_oArgv[i]->Clone());
}

CCallRule::~CCallRule()
{
}

void CCallRule::AddExpress(CRuleExpress* pExp)
{
	m_oArgv.Insert((uint32)(-1), pExp);
}

uint32 CCallRule::GetRuleType() const
{
	return ARF_RULE_CALL;
}

CRule* CCallRule::Clone() const
{
	return new CCallRule(*this);
}

static const char* g_sDebugRule = GetEnvVar("DebugRuleProc");
uint32 CCallRule::Match(CRuleStack &oStack, bool bMust) const
{
	void * pArgv = NULL;
	if(g_sDebugRule && !CString::StringCompare(m_pProc->GetName(), g_sDebugRule, false))
		Print("Debug CCallRule::Match(%s)\n", m_pProc->GetName());
	CRuleStruct* pParaInfo = m_pProc->GetParameters();
	uint32 nArgvSize = pParaInfo->GetSize();
	if(nArgvSize > sizeof(uint32))
	{
		pArgv = new char[nArgvSize];
		pParaInfo->InitObject(pArgv);
		CRuleCallExpress::BuildArgv(&oStack, pArgv, pParaInfo, m_oArgv);
	}
	void* pOldArgv = oStack.pArgv;
	oStack.pArgv = pArgv;
	uint32 nArgOff = oStack.nArgOff;
	oStack.nArgOff = 0;
	if(pArgv)
	{
		CRuleParameter* pField = (CRuleParameter*)pParaInfo->GetField((uint32)0);
		oStack.nArgOff = pField->GetOffset();
	}
	uint32 nRet = m_pProc->Call(oStack, bMust);
	oStack.pArgv = pOldArgv;
	if(nArgvSize)
	{
		pParaInfo->ClearObject(pArgv);
		delete[] (char*)pArgv;
	}
	oStack.nArgOff = nArgOff;
	return nRet;
}

bool CCallRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	m_pProc = oSystem.GetRule(m_oName.GetStr());
	if(m_pProc == NULL)
	{
		pChecker->OnError(*this, "the rule '%s' is undefined", m_oName.GetStr());
		return false;
	}
	CRuleStruct* pStruct = m_pProc->GetParameters();
	uint32 i, nArgc = pStruct->GetFieldCount();
	if(nArgc != m_oArgv.GetSize())
	{
		pChecker->OnError(*this, "the rule '%s' argc(%u) isn't the fact value(%u)", m_oName.GetStr(), nArgc, m_oArgv.GetSize());
		return false;
	}
	for(i=0; i<nArgc; ++i)
	{
		CRuleExpress *pExp = (CRuleExpress*)m_oArgv[i];
		if(!pExp->Check(oSystem, pChecker))
			return false;
		CRuleParameter* pVar = (CRuleParameter*)pStruct->GetField(i);
		if(pVar->IsOut())
		{
			if(!pExp->IsVariable())
			{
				pChecker->OnError(*this, "the rule '%s' arg(%u) can't be constant data", m_oName.GetStr(), i);
				return false;
			}
			if(pExp->GetExpressType(&oSystem) != pVar->GetType())
			{
				bool bRet = false;
				uint32 nType1 = pVar->GetType()->TypeCode();
				uint32 nType2 = pExp->GetExpressType(&oSystem)->TypeCode();
				switch(nType1)
				{
				case ARF_CHAR:
					if(nType2 == ARF_INT8 || nType2 == ARF_UINT8)
						bRet = true;
					break;
				case ARF_INT8:
					if(nType2 == ARF_CHAR || nType2 == ARF_UINT8)
						bRet = true;
					break;
				case ARF_UINT8:
					if(nType2 == ARF_CHAR || nType2 == ARF_INT8)
						bRet = true;
					break;
				case ARF_INT16:
					if(nType2 == ARF_UINT16)
						bRet = true;
					break;
				case ARF_UINT16:
					if(nType2 == ARF_INT16)
						bRet = true;
					break;
				case ARF_INT32:
					if(nType2 == ARF_UINT32)
						bRet = true;
					break;
				case ARF_UINT32:
					if(nType2 == ARF_INT32)
						bRet = true;
					break;
				case ARF_INT64:
					if(nType2 == ARF_UINT64)
						bRet = true;
					break;
				case ARF_UINT64:
					if(nType2 == ARF_INT64)
						bRet = true;
					break;
				}
				if(bRet == false)
				{
					pChecker->OnError(*this, "the rule '%s' arg(%u) type isn't consistent", m_oName.GetStr(), i);
					return false;
				}
			}
		}
		else
		{
			if(pExp->GetExpressType(&oSystem) != pVar->GetType())
			{
				bool bRet = false;
				uint32 nType1 = pVar->GetType()->TypeCode();
				uint32 nType2 = pExp->GetExpressType(&oSystem)->TypeCode();
				switch(nType1)
				{
				case ARF_BOOL:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_INT64:
					case ARF_UINT64:
					case ARF_FLOAT:
					case ARF_DOUBLE:
						bRet = true;
						break;
					}
					break;
				case ARF_CHAR:
					switch(nType2)
					{
					case ARF_INT8:
					case ARF_UINT8:
						bRet = true;
						break;
					}
					break;
				case ARF_INT8:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_UINT8:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT8:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
						bRet = true;
						break;
					}
					break;
				case ARF_INT16:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_UINT16:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT16:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
						bRet = true;
						break;
					}
					break;
				case ARF_INT32:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_UINT32:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT32:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
						bRet = true;
						break;
					}
					break;
				case ARF_INT64:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_UINT64:
						bRet = true;
						break;
					}
					break;
				case ARF_UINT64:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_INT64:
						bRet = true;
						break;
					}
					break;
				case ARF_FLOAT:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
						bRet = true;
						break;
					}
					break;
				case ARF_DOUBLE:
					switch(nType2)
					{
					case ARF_CHAR:
					case ARF_INT8:
					case ARF_UINT8:
					case ARF_INT16:
					case ARF_UINT16:
					case ARF_INT32:
					case ARF_UINT32:
					case ARF_FLOAT:
						bRet = true;
						break;
					}
					break;
				}
				if(bRet == false)
				{
					pChecker->OnError(*this, "the rule '%s' arg(%u) type isn't consistent", m_oName.GetStr(), i);
					return false;
				}
			}
		}
	}
	return true;
}

bool CCallRule::Same(const CRule *pRule)
{
	const CCallRule* pCallRule = dynamic_cast<const CCallRule*>(pRule);
	if(!pCallRule)
		return false;
	if(m_pProc != pCallRule->m_pProc)
		return false;
	uint32 nSize = m_oArgv.GetSize();
	if(nSize != pCallRule->m_oArgv.GetSize())
		return false;
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!m_oArgv[i]->Same(pCallRule->m_oArgv[i]))
			return false;
	}
	return true;
}

void CCallRule::Dump(CString & oDump, uint32 nLevel)
{
	oDump += m_oName;
	oDump += "<";
	uint32 nSize = m_oArgv.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		if(i)
			oDump += ",";
		CRuleExpress* pExp = (CRuleExpress*)m_oArgv[i];
		pExp->Dump(oDump);
	}
	oDump += ">\n";
}

// ---------------------------------------------------
// CRuleModule
// ---------------------------------------------------
CRuleModule::CRuleModule()
{
	m_pData = NULL;
}

CRuleModule::~CRuleModule()
{
}

void CRuleModule::InitData(CRuleSystem* pRuleSystem)
{
	CRuleStruct* pStruct = pRuleSystem->GetVariables();
	uint32 nSize = pStruct->GetSize();
	if(nSize)
	{
		m_pData = new char[nSize];
		pStruct->InitObject(m_pData);
	}
}

void CRuleModule::ClearData(CRuleSystem* pRuleSystem)
{
	if(m_pData)
	{
		CRuleStruct* pStruct = pRuleSystem->GetVariables();
		pStruct->ClearObject(m_pData);
		delete[] m_pData;
		m_pData = NULL;
	}
}

void CRuleModule::Push()
{
}

bool CRuleModule::Pop(uint32 nRet, bool bCheck)
{
	return true;
}

bool CRuleModule::IsBroken()
{
	return true;
}

void CRuleModule::OnParseError()
{
}

void CRuleModule::OnUnknown()
{
}

// ---------------------------------------------------
// CRuleSystem
// ---------------------------------------------------
CRuleSystem::CRuleSystem():m_oVariables(NULL, NULL)
{
	m_pCheckProc = NULL;
	m_pCheckFunc = NULL;
}

CRuleSystem::~CRuleSystem()
{
	Reset();
}

void CRuleSystem::Reset()
{
	m_oSystemRules.Clear();
	m_oSystemFunctions.Clear();
	m_oSystemTypes.Clear();
}

CRuleStruct* CRuleSystem::GetVariables()
{
	return &m_oVariables;
}

void CRuleSystem::RegRule(CRuleProc* pProc)
{
	if(GetRule(pProc->GetName()))
		FocpAbort(("CRuleSystem::RegRule('%s') failure", pProc->GetName()));
	m_oSystemRules.Insert((uint32)(-1), pProc);
}

CRuleProc* CRuleSystem::GetRule(const char* sName) const
{
	uint32 nCount = m_oSystemRules.GetSize();
	for(uint32 i=0; i<nCount; ++i)
	{
		const CRuleProc* pProc = m_oSystemRules[i];
		if(!CString::StringCompare(sName, pProc->GetName(), false))
			return (CRuleProc*)pProc;
	}
	return NULL;
}

void CRuleSystem::RegType(CRuleType* pType)
{
	if(GetType(pType->GetName()))
		FocpAbort(("CRuleSystem::RegType('%s') failure", pType->GetName()));
	m_oSystemTypes.Insert((uint32)(-1), pType);
}

CRuleType* CRuleSystem::GetType(const char* sName) const
{
	uint32 nCount = m_oSystemTypes.GetSize();
	for(uint32 i=0; i<nCount; ++i)
	{
		const CRuleType* pType = m_oSystemTypes[i];
		if(!CString::StringCompare(sName, pType->GetName(), false))
			return (CRuleType*)pType;
	}
	return NULL;
}

void CRuleSystem::RegFunc(CRuleFunc* pFunc)
{
	if(GetFunc(pFunc->GetName()))
		FocpAbort(("CRuleSystem::RegFunc('%s') failure", pFunc->GetName()));
	m_oSystemFunctions.Insert((uint32)(-1), pFunc);
}

CRuleFunc* CRuleSystem::GetFunc(const char* sName) const
{
	uint32 nCount = m_oSystemFunctions.GetSize();
	for(uint32 i=0; i<nCount; ++i)
	{
		const CRuleFunc* pFunc = m_oSystemFunctions[i];
		if(!CString::StringCompare(sName, pFunc->GetName(), false))
			return (CRuleFunc*)pFunc;
	}
	return NULL;
}

FRuleHost CRuleSystem::GetHost(const char* sName) const
{
	return NULL;
}

bool CRuleSystem::Check(CRuleChecker* pChecker)
{
	bool bRet = true;
	uint32 i, nCount;

	nCount = m_oSystemTypes.GetSize();
	for(i=0; i<nCount; ++i)
	{
		CRuleType* pType = m_oSystemTypes[i];
		if(!pType->Check(pChecker))
			bRet = false;
	}

	m_oVariables.FinishDefine();
	if(!m_oVariables.Check(pChecker))
		bRet = false;

	nCount = m_oSystemFunctions.GetSize();
	for(i=0; i<nCount; ++i)
	{
		m_pCheckFunc = m_oSystemFunctions[i];
		if(!m_pCheckFunc->Check(pChecker))
			bRet = false;
	}

	m_pCheckFunc = NULL;

	nCount = m_oSystemRules.GetSize();
	for(i=0; i<nCount; ++i)
	{
		m_pCheckProc = m_oSystemRules[i];
		if(!m_pCheckProc->Check(pChecker))
			bRet = false;
	}

	/*
		if(bRet)
		{
			nCount = m_oSystemRules.GetSize();
			for(i=0; i<nCount; ++i)
				m_oSystemRules[i]->Optimize();
			for(i=0; i<nCount; ++i)
			{
				if(!m_oSystemRules[i]->Check(pChecker))
					bRet = false;
			}
		}
	*/

	return bRet;
}

void CRuleSystem::Parse(bool bLoop, CRuleModule &oModule) const
{
	uint32 i, nRet = 0;
	uint32 nCount = m_oSystemRules.GetSize();
loop:
	for(i=0; i<nCount; ++i)
	{
		const CRuleProc* pMain = m_oSystemRules[i];
		if(pMain->IsMain())
		{
			CRuleStack oStack = {(CRuleSystem*)this, &oModule, NULL, NULL, oModule.m_pData, NULL, NULL, 0};
			nRet = pMain->Call(oStack, false);
			if(nRet != 1)
				break;
		}
	}
	switch(nRet)
	{
	case 1:
		oModule.OnUnknown();
		break;
	case 0:
		if(bLoop && !oModule.IsBroken())
			goto loop;
		break;
	}
}

const char* CRuleSystem::GetRuleSection()
{
	return "RULES:\n";
}

void CRuleSystem::Dump(CString & oDump)
{
	uint32 i, nCount;

	nCount = m_oSystemTypes.GetSize();
	if(nCount)
	{
		oDump += "TYPES:\n";
		for(i=0; i<nCount; ++i)
		{
			CRuleType* pType = m_oSystemTypes[i];
			pType->Dump(oDump, 1);
		}
	}

	if(m_oVariables.GetFieldCount())
	{
		oDump += "VARIABLES:\n";
		m_oVariables.Dump(oDump, (uint32)0);
	}

	nCount = m_oSystemRules.GetSize();
	if(nCount)
	{
		oDump += GetRuleSection();
		for(i=0; i<nCount; ++i)
		{
			CRuleProc* pProc = m_oSystemRules[i];
			pProc->Dump(oDump, 1);
		}
	}
}

CRuleStruct* CRuleSystem::GetCheckVariables()
{
	CRuleStruct* pVariables;
	if(m_pCheckProc)
		pVariables = m_pCheckProc->GetVariables();
	else
		pVariables = m_pCheckFunc->GetVariables();
	return pVariables;
}

CRuleStruct* CRuleSystem::GetCheckParameters()
{
	CRuleStruct* pVariables;
	if(m_pCheckProc)
		pVariables = m_pCheckProc->GetParameters();
	else
		pVariables = m_pCheckFunc->GetParameters();
	return pVariables;
}

// ---------------------------------------------------
// CSequenceRule
// ---------------------------------------------------
CSequenceRule::CSequenceRule(uint32 nType)
{
	if(nType > 2)
		nType = 0;
	m_nType = nType;
}

CSequenceRule::CSequenceRule(const CSequenceRule& oRule):
	CRule(oRule)
{
	m_nType = oRule.m_nType;
	uint32 nSize = oRule.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		InsertRule(*(((CSequenceRule&)oRule).GetRule(i)));
}

CSequenceRule::~CSequenceRule()
{
}

CRule* CSequenceRule::Clone() const
{
	return new CSequenceRule(*this);
}

uint32 CSequenceRule::Match(CRuleStack &oStack, bool bMust) const
{
	bool bNeedPush;
	uint32 i, nRet, nSize, nStart;

	nSize=GetSize();

	switch(m_nType)
	{
	case 0:
		nStart = 0;
		bNeedPush = true;
		for(i=0; i<nSize; ++i)
		{
			CRule* pRule = GetRule(i);
			if(bNeedPush)
				oStack.pModule->Push();
			if(i > nStart)
				nRet = pRule->Call(oStack, true);
			else
				nRet = pRule->Call(oStack, bMust);
			if(nRet)
			{
				if(bNeedPush)
					oStack.pModule->Pop(nRet);
				break;
			}
			if(bNeedPush)
			{
				bool bChange = oStack.pModule->Pop(0, true);
				if(bChange)
					bNeedPush = false;
				else
					++nStart;
			}
		}
		break;
	case 1:
		oStack.pModule->Push();
		for(i=0; i<nSize; ++i)
		{
			CRule* pRule = GetRule(i);
			nRet = pRule->Call(oStack, bMust);
			if(nRet)
				break;
		}
		oStack.pModule->Pop(1);
		break;
	case 2:
		for(i=0; i<nSize; ++i)
		{
			CRule* pRule = GetRule(i);
			nRet = pRule->Call(oStack, bMust);
			if(nRet)
				break;
		}
		break;
	}
	return nRet;
}

bool CSequenceRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pRule = GetRule(i);
		if(!pRule->Check(oSystem, pChecker, nLoopLevel))
			return false;
	}
	return true;
}

bool CSequenceRule::Same(const CRule *pRule)
{
	const CSequenceRule* pSequenceRule = dynamic_cast<const CSequenceRule*>(pRule);
	if(!pSequenceRule)
		return false;
	if(m_nType != pSequenceRule->m_nType)
		return false;
	uint32 nSize = GetSize();
	if(nSize != pSequenceRule->GetSize())
		return false;
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pDstRule = GetRule(i);
		const CRule* pSrcRule = pSequenceRule->GetRule(i);
		if(!pDstRule->Same(pSrcRule))
			return false;
	}
	return true;
}

bool CSequenceRule::Merge(const CRule *pRule)
{
	const CSequenceRule* pSequenceRule = dynamic_cast<const CSequenceRule*>(pRule);
	if(!pSequenceRule)
		return false;
	uint32 nSize = GetSize();
	if(nSize != pSequenceRule->GetSize())
		return false;
	CSequenceRule oRet(*this);
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pDstRule = oRet.GetRule(i);
		const CRule* pSrcRule = pSequenceRule->GetRule(i);
		if(!pDstRule->Merge(pSrcRule))
			return false;
	}
	m_oRuleTable.Swap(oRet.m_oRuleTable);
	return true;
}

CRule* CSequenceRule::Optimize()
{
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pSrcRule = GetRule(i);
		CRule* pDstRule = pSrcRule->Optimize();
		if(pDstRule != pSrcRule)
			m_oRuleTable[i] = pDstRule;//自动删除了pSrcRule
	}
	if(nSize == 1)
		return GetRule(0)->Clone();
	if(nSize == 0)
		return new CRule();
	return this;
}

void CSequenceRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	switch(m_nType)
	{
	case 0:
		oDump += "(\n";
		break;
	case 1:
		oDump += "[?\n";
		break;
	case 2:
		oDump += "[\n";
		break;
	}
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pRule = GetRule(i);
		pRule->Dump(oDump, nLevel+1);
	}
	CRuleType::DumpLevel(oDump, nLevel);
	switch(m_nType)
	{
	case 0:
		oDump += ")\n";
		break;
	case 1:
		oDump += "?]\n";
		break;
	case 2:
		oDump += "]\n";
		break;
	}
}

void CSequenceRule::InsertRule(const CRule& oRule, bool bAppend)
{
	if(bAppend)
		m_oRuleTable.Insert((uint32)(-1), oRule.Clone());
	else
		m_oRuleTable.Insert(0, oRule.Clone());
}

uint32 CSequenceRule::GetSize() const
{
	return m_oRuleTable.GetSize();
}

CRule* CSequenceRule::GetRule(uint32 nIdx) const
{
	if(nIdx < m_oRuleTable.GetSize())
	{
		const CAutoPointer< CRule >& oRule = m_oRuleTable[nIdx];
		return (CRule*)(const CRule*)oRule;
	}
	return NULL;
}

void CSequenceRule::Remove(uint32 nIdx)
{
	m_oRuleTable.Remove(nIdx);
}

uint32 CSequenceRule::GetRuleType() const
{
	return ARF_RULE_SEQUENCE;
}

// ---------------------------------------------------
// COrRule
// ---------------------------------------------------
COrRule::COrRule()
{
}

COrRule::COrRule(const COrRule& oRule):
	CSequenceRule(oRule)
{
}

COrRule::~COrRule()
{
}

CRule* COrRule::Clone() const
{
	return new COrRule(*this);
}

uint32 COrRule::Match(CRuleStack &oStack, bool bMust) const
{
	uint32 nRet = 0;
	uint32 i,nSize = GetSize();
	for(i=0; i<nSize; ++i)
	{
		CRule* pRule = GetRule(i);
		oStack.pModule->Push();
		nRet = pRule->Call(oStack, false);
		oStack.pModule->Pop(nRet);
		if(nRet != 1)
			break;
	}
	if(nRet == 1)
	{
		if(bMust)
		{
			nRet = 2;
			oStack.pModule->OnParseError();
		}
	}
	return nRet;
}

bool COrRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pRule = GetRule(i);
		if(!pRule->Check(oSystem, pChecker, nLoopLevel))
			return false;
	}
	return true;
}

bool COrRule::Same(const CRule *pRule)
{
	if(!dynamic_cast<const COrRule*>(pRule))
		return false;
	return CSequenceRule::Same(pRule);
}

bool COrRule::Merge(const CRule *pRule)
{
	const COrRule* pOrRule = dynamic_cast<const COrRule*>(pRule);
	if(!pOrRule)
		return false;
	uint32 nSize = GetSize();
	if(nSize != pOrRule->GetSize())
		return false;
	COrRule oRet(*this);
	for(uint32 j, i=0; i<nSize-1; ++i)
	{
		CRule* pSrcRule = oRet.GetRule(i);
		for(j=0; j<nSize; ++j)
		{
			CRule* pDstRule = pOrRule->GetRule(j);
			if(pSrcRule->Merge(pDstRule))
				break;
		}
		if(j >= nSize)
			return false;
	}
	m_oRuleTable.Swap(oRet.m_oRuleTable);
	return true;
}

CRule* COrRule::Optimize()
{
	CRule* pRet = CSequenceRule::Optimize();
	if(pRet != this)
		return pRet;
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize-1; ++i)
	{
		CRule* pSrcRule = GetRule(i);
		for(uint32 j=i+1; j<nSize; ++j)
		{
			CRule* pDstRule = GetRule(j);
			if(pSrcRule->Merge(pDstRule))
			{
				m_oRuleTable.Remove(j);
				--nSize;
				--j;
			}
		}
	}
	if(nSize == 1)
		return GetRule(0)->Clone();
	if(nSize == 0)
		return new CRule();
	return this;
}

void COrRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "(?\n";
	uint32 nSize = GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		CRule* pRule = GetRule(i);
		pRule->Dump(oDump, nLevel+1);
	}
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "?)\n";
}

uint32 COrRule::GetRuleType() const
{
	return ARF_RULE_OR;
}

// ---------------------------------------------------
// CLoopRule
// ---------------------------------------------------
CLoopRule::CLoopRule(const CRule& oRule, bool bAnyTimes)
{
	m_bAnyTimes = bAnyTimes;
	if(oRule.GetRuleType() == ARF_RULE_LOOP)
	{
		CLoopRule* pRule = (CLoopRule*)&oRule;
		m_pRule = pRule->m_pRule->Clone();
	}
	else
		m_pRule = oRule.Clone();
}

CLoopRule::CLoopRule(const CLoopRule& oRule):
	CRule(oRule)
{
	m_bAnyTimes = oRule.m_bAnyTimes;
	m_pRule = oRule.m_pRule->Clone();
}

CLoopRule::~CLoopRule()
{
	delete m_pRule;
}

CRule* CLoopRule::Clone() const
{
	return new CLoopRule(*this);
}

uint32 CLoopRule::Match(CRuleStack &oStack, bool bMust) const
{
	uint32 nRet;
	do
	{
		oStack.pModule->Push();
		nRet = m_pRule->Call(oStack, false);
		oStack.pModule->Pop(nRet);
		if(nRet)
			break;
	}
	while(m_bAnyTimes);
	if(nRet != 2)
		nRet = 0;
	return nRet;
}

bool CLoopRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	return m_pRule->Check(oSystem, pChecker, nLoopLevel+1);
}

bool CLoopRule::Same(const CRule *pRule)
{
	const CLoopRule* pForRule = dynamic_cast<const CLoopRule*>(pRule);
	if(!pForRule)
		return false;
	if(m_bAnyTimes != pForRule->m_bAnyTimes)
		return false;
	return m_pRule->Same(pForRule->m_pRule);
}

bool CLoopRule::Merge(const CRule *pRule)
{
	const CLoopRule* pForRule = dynamic_cast<const CLoopRule*>(pRule);
	if(!pForRule)
		return false;
	if(!m_pRule->Merge(pForRule->m_pRule))
		return false;
	if(pForRule->m_bAnyTimes)
		m_bAnyTimes = pForRule->m_bAnyTimes;
	return true;
}

CRule* CLoopRule::Optimize()
{
	CRule* pSubRule = m_pRule->Optimize();
	if(pSubRule != m_pRule)
	{
		delete m_pRule;
		m_pRule = pSubRule;
	}
	if(m_pRule->GetRuleType() == ARF_RULE_BREAK)
		return new CRule();
	return this;
}

void CLoopRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bAnyTimes)
	{
		oDump += "{\n";
		m_pRule->Dump(oDump, nLevel+1);
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "}\n";
	}
	else
	{
		oDump += "{?\n";
		m_pRule->Dump(oDump, nLevel+1);
		CRuleType::DumpLevel(oDump, nLevel);
		oDump += "?}\n";
	}
}

uint32 CLoopRule::GetRuleType() const
{
	return ARF_RULE_LOOP;
}

// ---------------------------------------------------
// CBreakRule
// ---------------------------------------------------
CBreakRule::CBreakRule()
{
}

CBreakRule::CBreakRule(const CBreakRule& oRule):
	CRule(oRule)
{
}

CBreakRule::~CBreakRule()
{
}

uint32 CBreakRule::GetRuleType() const
{
	return ARF_RULE_BREAK;
}

CRule* CBreakRule::Clone() const
{
	return new CBreakRule(*this);
}

uint32 CBreakRule::Match(CRuleStack &oStack, bool bMust) const
{
	return 3;//跳出循环
}

bool CBreakRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	if(nLoopLevel == 0)
	{
		pChecker->OnError(*this, "the break rule should be used in the for rule");
		return false;
	}
	return true;
}

bool CBreakRule::Same(const CRule *pRule)
{
	return (dynamic_cast<const CBreakRule*>(pRule) != NULL);
}

void CBreakRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "break\n";
}

// ---------------------------------------------------
// CIfRule
// ---------------------------------------------------
CIfRule::CIfRule(CRuleExpress* pExp)
{
	m_pExp = pExp;
}

CIfRule::CIfRule(const CIfRule& oRule):
	CRule(oRule)
{
	m_pExp = oRule.m_pExp->Clone();
}

CIfRule::~CIfRule()
{
	delete m_pExp;
}

CRule* CIfRule::Clone() const
{
	return new CIfRule(*this);
}

uint32 CIfRule::Match(CRuleStack &oStack, bool bMust) const
{
	bool bRet;
	void* pCond = m_pExp->GetExpressValue(&oStack);
	CRuleType* pCondType = m_pExp->GetExpressType(oStack.pSystem);
	uint32 nCondType = pCondType->TypeCode();
	switch(nCondType)
	{
	case ARF_STRUCT:
		bRet = (*(void**)pCond)?true:false;
		break;
	case ARF_BOOL:
		bRet = (*(bool*)pCond)?true:false;
		break;
	case ARF_CHAR:
		bRet = (*(char*)pCond)?true:false;
		break;
	case ARF_INT8:
		bRet = (*(int8*)pCond)?true:false;
		break;
	case ARF_UINT8:
		bRet = (*(uint8*)pCond)?true:false;
		break;
	case ARF_INT16:
		bRet = (*(int16*)pCond)?true:false;
		break;
	case ARF_UINT16:
		bRet = (*(uint16*)pCond)?true:false;
		break;
	case ARF_INT32:
		bRet = (*(int32*)pCond)?true:false;
		break;
	case ARF_UINT32:
		bRet = (*(uint32*)pCond)?true:false;
		break;
	case ARF_INT64:
		bRet = (*(int64*)pCond)?true:false;
		break;
	case ARF_UINT64:
		bRet = (*(uint64*)pCond)?true:false;
		break;
	case ARF_FLOAT:
		bRet = (*(float*)pCond)?true:false;
		break;
	case ARF_DOUBLE:
		bRet = (*(double*)pCond)?true:false;
		break;
	}
	if(bRet)
		return 0;
	if(bMust)
		return 2;
	return 1;
}

bool CIfRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	CRuleStruct* pVariables;
	pVariables = oSystem.GetCheckVariables();
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
	return bRet;
}

bool CIfRule::Same(const CRule *pRule)
{
	const CIfRule* pRet = dynamic_cast<const CIfRule*>(pRule);
	if(pRet == NULL)
		return false;
	if(!m_pExp->Same(*pRet->m_pExp))
		return false;
	return true;
}

void CIfRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	oDump += "if<";
	m_pExp->Dump(oDump);
	oDump += ">\n";
}

// ---------------------------------------------------
// CSemanticRule
// ---------------------------------------------------
CSemanticRule::CSemanticRule(CRuleSentence* pSentence)
{
	m_pSentence = pSentence;
}

CSemanticRule::~CSemanticRule()
{
	delete m_pSentence;
}

CSemanticRule::CSemanticRule(const CSemanticRule& oRule):
	CRule(oRule)
{
	m_pSentence = oRule.m_pSentence->Clone();
}

CRule* CSemanticRule::Clone() const
{
	return new CSemanticRule(*this);
}

uint32 CSemanticRule::Match(CRuleStack &oStack, bool bMust) const
{
	m_pSentence->Execute(&oStack);
	return 0;
}

bool CSemanticRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	bool bRet = true;
	if(!m_pSentence->Check(oSystem, pChecker, 0, 0))
		bRet = false;
	else if(m_pSentence->GetSentenceType() != ARF_SENTENCE_COMPLEX)
	{
		pChecker->OnError(*m_pSentence, "the semantic rule must be the complex sentence");
		bRet = false;
	}
	return bRet;
}

bool CSemanticRule::Same(const CRule *pRule)
{
	const CSemanticRule* pRet = dynamic_cast<const CSemanticRule*>(pRule);
	if(pRet == NULL)
		return false;
	return m_pSentence->Same(pRet->m_pSentence);
}

void CSemanticRule::Dump(CString & oDump, uint32 nLevel)
{
	m_pSentence->Dump(oDump, nLevel);
}

FOCP_END();
