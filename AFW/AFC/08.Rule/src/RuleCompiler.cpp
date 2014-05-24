
#include "RuleCompiler.hpp"

FOCP_BEGIN();

CSourceCode::CSourceCode(CLexicalModule* pLexModule):
	m_pLexModule(pLexModule),
	m_oCodes(FocpFieldOffset(CCodeLine, pPrev), FocpFieldOffset(CCodeLine, pNext))
{
	m_nPos = 0;
	m_pLine = NULL;
}

CSourceCode::~CSourceCode()
{
	CCodeLine* pHead = m_oCodes.First();
	while(pHead)
	{
		CCodeLine* pNext = pHead->pNext;
		delete[] pHead->sLine;
		delete pHead;
		pHead = pNext;
	}
	m_oCodes.Clear();
}

uint32 CSourceCode::GetPos() const
{
	return m_nPos;
}

void CSourceCode::SetPos(uint32 nPos)
{
	if(m_pLine == NULL)
		return;
	if(m_nPos < nPos)
	{
		CCodeLine* pLine = m_pLine;
		uint32 nRest = nPos - m_nPos;
		while(nRest)
		{
			uint32 nSize = pLine->nSize;
			nSize -= pLine->nCol - 1;
			if(nSize > nRest)
			{
				pLine->nCol += nRest;
				m_nPos += nRest;
				m_pLine = pLine;
				break;
			}
			pLine->nCol += nSize;
			nRest -= nSize;
			m_nPos += nSize;
			m_pLine = pLine;
			pLine = pLine->pNext;
			if(pLine == NULL)
				break;
			m_pLine = pLine;
		}
	}
	else if(m_nPos > nPos)
	{
		CCodeLine* pLine = m_pLine;
		uint32 nRest = m_nPos - nPos;
		while(nRest)
		{
			uint32 nSize = pLine->nCol - 1;
			if(nSize > nRest)
			{
				pLine->nCol -= nRest;
				m_nPos -= nRest;
				m_pLine = pLine;
				break;
			}
			pLine->nCol -= nSize;
			nRest -= nSize;
			m_nPos -= nSize;
			m_pLine = pLine;
			if(nRest)
			{
				pLine = pLine->pPrev;
				if(pLine == NULL)
					break;
				m_pLine = pLine;
			}
		}
	}
}

CCodeLine* CSourceCode::GetLine() const
{
	return m_pLine;
}

void CSourceCode::GetLine(uint32 nPos, CCodeLine &oLine)
{
	if(m_pLine)
	{
		uint32 nOldPos = m_nPos;
		SetPos(nPos);
		oLine.sFileName = m_pLine->sFileName;
		oLine.nLine = m_pLine->nLine;
		oLine.nCol = m_pLine->nCol;
		SetPos(nOldPos);
	}
	else
	{
		oLine.sFileName = "";
		oLine.nLine = 1;
		oLine.nCol = 1;
	}
}

bool CSourceCode::GetChar(char &c, bool bNextPos)
{
	uint32 nLine, nSize;
	const char* sFileName;
	char* sLine;
	CCodeLine* pLine;
	while(true)
	{
		if(m_pLine)
		{
			nSize = m_pLine->nSize;
			if(m_pLine->nCol <= nSize)
			{
				c = m_pLine->sLine[m_pLine->nCol - 1];
				if(c >= 0)
				{
					if(CString::IsControl(c) && c!= '\t' && c!='\n')
					{
						m_pLexModule->WriteError(false, m_pLine->sFileName, m_pLine->nLine, m_pLine->nCol, "meeting the invalid text byte '0x%2X'", c);
						return false;//文件异常结束
					}
				}
				if(c == (char)(-1))//文件正常结束
					return false;
				if(bNextPos)
				{
					++m_nPos;
					++m_pLine->nCol;
					if(m_pLine->nCol == nSize)
					{
						CCodeLine* pNext = m_pLine->pNext;
						if(pNext)
							m_pLine = pNext;
					}
				}
				break;
			}
		}
		sLine = GetCodeLine(sFileName, nLine);
		pLine = new CCodeLine;
		pLine->nSize = CString::StringLength(sLine);
		pLine->sLine = new char[pLine->nSize + 1];
		CString::StringCopy(pLine->sLine, sLine);
		pLine->nLine = nLine;
		pLine->nCol = 1;
		pLine->sFileName = m_pLexModule->GetSymbol(sFileName);
		m_oCodes.Push(pLine);
		m_pLine = pLine;
	}
	return true;
}

CSourceFile::~CSourceFile()
{
	if(m_bOwner && m_pFile)
	{
		delete m_pFile;
		m_pFile = NULL;
	}
	if(m_pFmt)
	{
		delete m_pFmt;
		m_pFmt = NULL;
	}
}

CSourceFile::CSourceFile(CLexicalModule* pLexModule, const char* sFileName):
	CSourceCode(pLexModule)
{
	m_bOwner = true;
	m_pFile = new CFile;
	m_pFmt = NULL;
	m_nLine = 1;
	m_bContinuation = pLexModule->m_pLexicalSystem->m_bContinuation;
	if(m_pFile->Open(sFileName, "r"))
	{
		delete m_pFile;
		m_pFile = NULL;
	}
	else
	{
		m_oFileName = m_pFile->GetFileName().oConnectName;
		m_pFmt = new CFileFormatter(m_pFile);
	}
}

CSourceFile::CSourceFile(CLexicalModule* pLexModule, const char* sCode, const char* sFileName):
	CSourceCode(pLexModule)
{
	m_bOwner = true;
	m_pFile = new CFile;
	m_nLine = 1;
	m_bContinuation = pLexModule->m_pLexicalSystem->m_bContinuation;
	uint32 nSize = CString::StringLength(sCode);
	CFormatString oFileName;
	oFileName.Print("memory://|%p:%d", sCode, nSize);
	m_pFmt = NULL;
	if(m_pFile->Open(oFileName.GetStr(), "r"))
	{
		delete m_pFile;
		m_pFile = NULL;
	}
	else
	{
		m_oFileName = sFileName;
		m_pFmt = new CFileFormatter(m_pFile);
	}
}

CSourceFile::CSourceFile(CLexicalModule* pLexModule, CFile& oFile):
	CSourceCode(pLexModule)
{
	m_bOwner = false;
	m_pFile = &oFile;
	m_nLine = 1;
	m_bContinuation = pLexModule->m_pLexicalSystem->m_bContinuation;
	m_oFileName = m_pFile->GetFileName().oConnectName;
	m_pFmt = new CFileFormatter(m_pFile);
}

char* CSourceFile::GetCodeLine(const char* &sFileName, uint32 &nLine)
{
	nLine = m_nLine;
	if(m_pFmt == NULL)
	{
		sFileName = "";
		nLine = 1;
		if(m_oLine.Empty())
			m_oLine += (char)(-1);
	}
	else
	{
		int32 nCount;
		if(m_bContinuation)
			nCount = m_pFmt->Scan("%rr", &m_oLine);
		else
			nCount = m_pFmt->Scan("%r", &m_oLine);
		if(nCount == 1)
		{
			m_oLine += "\n";
			++m_nLine;
		}
		else
		{
			char s[2] = {(char)(-1), '\0'};
			m_oLine = s;
		}
	}
	sFileName = m_oFileName.GetStr();
	return (char*)m_oLine.GetStr();
}

CToken::CToken(const char* sKind): m_sKind(sKind)
{
	m_sToken = NULL;
	m_pPrev = NULL;
	m_pNext = NULL;
}

CToken::~CToken()
{
}

const char* CToken::GetToken() const
{
	return m_sToken;
}

void CToken::SetFrom(CToken* pFrom)
{
	m_sToken = pFrom->m_sToken;
	SetFile(*pFrom);
}

void CToken::Parse(CLexicalModule& oLexical, const char* sToken, uint32 nLine, uint32 nCol, const char* sFileName)
{
	m_sToken = sToken;
	m_bNewLine = true;
	SetFile(sFileName, nLine, nCol);
}

bool CToken::NewLine() const
{
	return m_bNewLine;
}

void CToken::SetNewLine()
{
	m_bNewLine = true;
}

const char* CToken::GetKind() const
{
	return m_sKind;
}

CToken*& GetRuleArgToken(CRuleArgv& oArgv)
{
	uint32 nSize = sizeof(CToken*);
	ulong nPos = (ulong)oArgv.pArgv;
	uint32 nMod = nPos % nSize;
	if(nMod)
	{
		nPos += nSize - nMod;
		oArgv.pArgv = (void*)nPos;
	}
	CToken* &pRet = *(CToken**)oArgv.pArgv;
	nPos += nSize;
	oArgv.pArgv = (void*)nPos;
	return pRet;
}

CCharRule::CCharRule(uint8 nMin, uint8 nMax, bool bNot)
	:m_bNot(bNot), m_nMin(nMin), m_nMax(nMax)
{
}

CCharRule::CCharRule(const CCharRule& oSrc)
	:CRule(oSrc), m_bNot(oSrc.m_bNot), m_nMin(oSrc.m_nMin), m_nMax(oSrc.m_nMax)
{
}

CCharRule::~CCharRule()
{
}

CRule* CCharRule::Clone() const
{
	return new CCharRule(*this);
}

uint32 CCharRule::Match(CRuleStack &oStack, bool bMust) const
{
	char c;
	bool bMatch;
	CLexicalModule* pLexical = dynamic_cast<CLexicalModule*>(oStack.pModule);
	if(pLexical == NULL)
		FocpAbort(("CCharRule::Match() failure"));
	CCodeLine* pLine = pLexical->m_pSourceCode->GetLine();
	uint32 nLine = pLine->nLine;
	uint32 nCol = pLine->nCol;
	const char* sFileName = pLine->sFileName;
	if(!pLexical->GetChar(c))
	{
		if(bMust)
		{
			if(m_nMin == m_nMax)
				pLexical->WriteError(false, sFileName, nLine, nCol, "Missing char '%c'", (char)m_nMin);
			else
				pLexical->WriteError(false, sFileName, nLine, nCol, "Missing char");
		}
		return 1;
	}
	uint8 x = c;
	if(m_bNot)
		bMatch = (x<m_nMin || x > m_nMax);
	else
		bMatch = (x>=m_nMin && x<=m_nMax);
	if(bMatch)
	{
		pLexical->AppendToken(c);
		return 0;
	}
	if(bMust)
	{
		if(m_nMin == m_nMax)
			pLexical->WriteError(false, sFileName, nLine, nCol, "Expect char '%c'", (char)m_nMin);
		else if(CString::IsPrint(c))
			pLexical->WriteError(false, sFileName, nLine, nCol, "Unknown char '%c'", (char)c);
		else
			pLexical->WriteError(false, sFileName, nLine, nCol, "Unknown char");
	}
	return 1;
}

bool CCharRule::Same(const CRule *pRule)
{
	const CCharRule* pCharRule = dynamic_cast<const CCharRule*>(pRule);
	if(!pCharRule)
		return false;
	if(m_bNot != pCharRule->m_bNot)
		return false;
	if(m_nMin != pCharRule->m_nMin)
		return false;
	if(m_nMax != pCharRule->m_nMax)
		return false;
	return true;
}

CStringRule::CStringRule(const char* sStr, bool bSenstive):
	m_bSenstive(bSenstive), m_oString(sStr)
{
}

CStringRule::CStringRule(const CStringRule& oRule):
	m_bSenstive(oRule.m_bSenstive), m_oString(oRule.m_oString)
{
}

CStringRule::~CStringRule()
{
}

CRule* CStringRule::Clone() const
{
	return new CStringRule(*this);
}

uint32 CStringRule::Match(CRuleStack &oStack, bool bMust) const
{
	char c;
	CLexicalModule* pLexical = dynamic_cast<CLexicalModule*>(oStack.pModule);
	if(pLexical == NULL)
		FocpAbort(("CStringRule::Match() failure"));
	const char* sStr = m_oString.GetStr();
	uint32 i, nLen = m_oString.GetSize();
	for(i=0; i<nLen; ++i)
	{
		CCodeLine *pLine = pLexical->m_pSourceCode->GetLine();
		uint32 nLine = pLine->nLine;
		uint32 nCol = pLine->nCol;
		const char* sFileName = pLine->sFileName;
		if(!pLexical->GetChar(c))
		{
			if(bMust)
			{
				if(CString::IsPrint(sStr[i]))
					pLexical->WriteError(false, sFileName, nLine, nCol, "Meeting the end of file, Missing char '%c'", sStr[i]);
				else
					pLexical->WriteError(false, sFileName, nLine, nCol, "Meeting the end of file, Missing char '%2x8'", sStr[i]);
			}
			return 1;
		}
		char c1, c2;
		if(m_bSenstive)
		{
			c1 = sStr[i];
			c2 = c;
		}
		else
		{
			c1 = CString::ToUpper(sStr[i]);
			c2 = CString::ToUpper(c);
		}
		if(c1 != c2)
		{
			if(bMust)
			{
				if(CString::IsPrint(sStr[i]))
					pLexical->WriteError(false, sFileName, nLine, nCol, "Expect char '%c'", (char)sStr[i]);
				else
					pLexical->WriteError(false, sFileName, nLine, nCol, "Expect char '%2x8'", (char)sStr[i]);
			}
			return 1;
		}
		pLexical->AppendToken(c);
	}
	return 0;
}

bool CStringRule::Same(const CRule *pRule)
{
	const CStringRule* pStringRule = dynamic_cast<const CStringRule*>(pRule);
	if(!pStringRule)
		return false;
	if(m_bSenstive != pStringRule->m_bSenstive)
		return false;
	return (m_oString.Compare(pStringRule->m_oString, m_bSenstive)==0);
}

CLexRule::CLexRule(CLexicalSystem* pSystem, const char* sRegex, const char* sKind, bool bConst, bool bSenstive, bool bSkip):
	CRuleProc(pSystem, sKind, BuildRegexRule(sRegex, bConst, bSenstive), true, NULL, NULL)
{
	m_bConst = bConst;
	m_bSenstive = bSenstive;
	m_bSkip = bSkip;
	m_oRegex = sRegex;
}

CLexRule::~CLexRule()
{
}

void CLexRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bConst)
		oDump += "CONST ";
	if(!m_bSenstive)
		oDump += "INSENSITIVE ";
	oDump += GetName();
	oDump += " = ";
	if(m_bSkip)
		oDump += "SKIP ";
	oDump += CString(m_oRegex).ToCString(false);
	oDump += " .\n";
}

//字符规则
//	普通字符	直接识别为字符
//	/			转义字符，将下一字符作为真实匹配字符。
//	%c1c2% 		字符范围匹配
//	^r			取反识别,r是字符规则
//	@			任意字符均可匹配(0x01~0xFF)
//序列规则
//	*  			匹配任意次数的前一规则,非独立规则
//	+  			至少匹配一次前一规则,非独立规则
//	?	 		最多匹配一次前一规则,非独立规则
//	{n,m}		至少匹配n次,最多匹配m次前一规则,非独立规则
//	{n,}		至少匹配n次前一规则,非独立规则
//	{n} 		匹配n次前一规则,非独立规则
//	[r1r2r3]	选择规则
//	(r1r2r3)	顺序规则
//	#r1r2r3#	乐观规则
//	<r1r2r3>	测试规则
//特殊规则：
//	!			中断规则
CRule* CLexRule::BuildRegexRule(const char* sRegex, bool bConst, bool bSenstive)
{
	if(bConst)
		return new CStringRule(sRegex, bSenstive);
	CSequenceRule* pRule = new CSequenceRule;
	BuildRegexRule(*pRule, sRegex, '\0');
	return pRule;
}

void CLexRule::BuildRegexRule(CSequenceRule& oRule, const char* &sRegex, char cEnd)
{
	while(sRegex[0] && sRegex[0] != cEnd)
	{
		switch(sRegex[0])
		{
		default:
			oRule.InsertRule(CCharRule(sRegex[0], sRegex[0]));
			break;

		case '!':
			oRule.InsertRule(CBreakRule());
			break;

		case '@':
			oRule.InsertRule(CCharRule((uint8)0x01, (uint8)0xFF));
			break;

		case '/':
			if(sRegex[1])
			{
				oRule.InsertRule(CCharRule(sRegex[1], sRegex[1]));
				++sRegex;
			}
			else
				FocpAbort(("CRegexRule::BuildRegexRule() failure"));
			break;

		case '%':
			if(sRegex[1] && sRegex[2] && sRegex[3]=='%')
			{
				oRule.InsertRule(CCharRule(sRegex[1], sRegex[2]));
				sRegex += 3;
			}
			else
				FocpAbort(("CRegexRule::BuildRegexRule() failure"));
			break;

		case '^':
			switch(sRegex[1])
			{
			default:
				oRule.InsertRule(CCharRule(sRegex[1], sRegex[1], true));
				++sRegex;
				break;
			case '/':
				if(sRegex[2])
				{
					oRule.InsertRule(CCharRule(sRegex[2], sRegex[2], true));
					sRegex += 2;
				}
				else
					FocpAbort(("CRegexRule::BuildRegexRule() failure"));
				break;
			case '%':
				if(sRegex[2] && sRegex[3] && sRegex[4]=='%')
				{
					oRule.InsertRule(CCharRule(sRegex[2], sRegex[3], true));
					sRegex += 4;
				}
				else
					FocpAbort(("CRegexRule::BuildRegexRule() failure"));
				break;
			case '\0':
			case '^':
			case '*':
			case '?':
			case '<':
			case '(':
			case '[':
			case '!':
			case '@':
			case '#':
				FocpAbort(("CRegexRule::BuildRegexRule() failure"));
				break;
			}
			break;

		case '*':
		{
			CRule* pLast = GetLastRule(oRule);
			if(pLast == NULL)
				FocpAbort(("CRegexRule::BuildRegexRule() failure"));
			CLoopRule oFor(*pLast, true);
			RemoveLastRule(oRule);
			oRule.InsertRule(oFor);
		}
		break;

		case '?':
		{
			CRule* pLast = GetLastRule(oRule);
			if(pLast == NULL)
				FocpAbort(("CRegexRule::BuildRegexRule() failure"));
			CLoopRule oFor(*pLast, false);
			RemoveLastRule(oRule);
			oRule.InsertRule(oFor);
		}
		break;

		case '[':
		{
			COrRule oOr;
			++sRegex;
			BuildRegexRule(oOr, sRegex, ']');
			oRule.InsertRule(oOr);
		}
		break;
		case '(':
		{
			CSequenceRule oSeq;
			++sRegex;
			BuildRegexRule(oSeq, sRegex, ')');
			oRule.InsertRule(oSeq);
		}
		break;
		case '<':
		{
			CSequenceRule oSeq(1);
			++sRegex;
			BuildRegexRule(oSeq, sRegex, '>');
			oRule.InsertRule(oSeq);
		}
		break;
		case '#':
		{
			CSequenceRule oSeq(2);
			++sRegex;
			BuildRegexRule(oSeq, sRegex, '#');
			oRule.InsertRule(oSeq);
		}
		break;
		}
		++sRegex;
	}
}

CRule* CLexRule::GetLastRule(CSequenceRule& oRule)
{
	uint32 nSize = oRule.GetSize();
	if(nSize)
		return oRule.GetRule(nSize - 1);
	return NULL;
}

void CLexRule::RemoveLastRule(CSequenceRule& oRule)
{
	uint32 nSize = oRule.GetSize();
	if(nSize)
		oRule.Remove(nSize - 1);
}

void CLexRule::Before(CRuleModule* pModule)const
{
	CLexicalModule* pLexModule = (CLexicalModule*)pModule;
	pLexModule->m_oToken.Clear();
	pLexModule->JumpSpace();
	pLexModule->m_sKind = GetName();
}

uint32 CLexRule::After(CRuleModule* pModule)const
{
	CLexicalModule* pLexModule = (CLexicalModule*)pModule;
	return pLexModule->CheckTokenEnd();
}

void CLexRule::OnSuccess(CRuleModule* pModule)const
{
	if(!m_bSkip)
		((CLexicalModule*)pModule)->CreateToken(GetName());
}

void CLexRule::OnFailure(CRuleModule* pModule)const
{
	CLexicalModule* pLexModule = (CLexicalModule*)pModule;
	pLexModule->WriteError(false, pLexModule->m_pPos->sFileName, pLexModule->m_pPos->nLine, pLexModule->m_pPos->nCol, "Expect token '%s'", pLexModule->m_sKind);
	pLexModule->JumpToTokenEnd();
}

CLexicalSystem::CLexicalSystem()
{
	m_bContinuation = false;
}

CLexicalSystem::~CLexicalSystem()
{
}

void CLexicalSystem::SetWhiteSpace(const CString& oWhiteSpace)
{
	m_oWhiteSpace = oWhiteSpace;
}

void CLexicalSystem::SetPuncts(const CString& oPuncts)
{
	m_oPuncts = oPuncts;
}

void CLexicalSystem::SetContinuation(bool bContinuation)
{
	m_bContinuation = bContinuation;
}

const char* CLexicalSystem::GetRuleSection()
{
	return "TOKENS:\n";
}

void CLexicalSystem::Dump(CString & oDump)
{
	if(!m_oWhiteSpace.Empty())
	{
		oDump += "WhiteSpace = ";
		oDump += CString(m_oWhiteSpace).ToCString(false);
		oDump += " .\n\n";
	}
	if(!m_oPuncts.Empty())
	{
		oDump += "WhiteSpace = ";
		oDump += CString(m_oPuncts).ToCString(false);
		oDump += " .\n\n";
	}
	CRuleSystem::Dump(oDump);
}

void CLexicalSystem::Reset()
{
	CRuleSystem::Reset();
	m_bContinuation = false;
	m_oWhiteSpace.Clear();
	m_oPuncts.Clear();
}

CLexicalModule::CLexicalModule(CLexicalSystem* pSystem, CFile &oErrorFile):
	m_oTokenTable(FocpFieldOffset(CToken, m_pPrev), FocpFieldOffset(CToken, m_pNext)), m_pLexicalSystem(pSystem)
{
//	m_pSourceFile = NULL;
	m_pPos = NULL;
	m_pErrorFile = &oErrorFile;
	m_nErrorCount = 0;
	m_nWarningCount = 0;
	m_cChar = '\0';
	m_sLastFileName = NULL;
	m_pTokenIdx = NULL;
}

CLexicalModule::~CLexicalModule()
{
	m_pSourceCode = NULL;
	//清空token表
	ClearTokenTable();
}

void CLexicalModule::ClearTokenTable()
{
	CToken* pToken = m_oTokenTable.First();
	while(pToken)
	{
		CToken* pNext = m_oTokenTable.GetNext(pToken);
		delete pToken;
		pToken = pNext;
	}
	m_oTokenTable.Clear();
	m_pTokenIdx = NULL;
}

void CLexicalModule::Push()
{
	CCodePosition oPos;
	CCodeLine* pLine = m_pSourceCode->GetLine();
	oPos.sFileName = pLine->sFileName;
	oPos.nLine = pLine->nLine;
	oPos.nCol = pLine->nCol;
	oPos.nPos = m_pSourceCode->GetPos();
	oPos.nTokenLen = m_oToken.GetSize();
	m_oPositionStack.Push(oPos, false);
	m_pPos = &m_oPositionStack.GetItem(m_oPositionStack.First());
}

bool CLexicalModule::Pop(uint32 nRet, bool bCheck)
{
	bool bRet = true;
	if(m_pPos)
	{
		switch(nRet)
		{
		case 3:
		case 0:
			if(bCheck)
			{
				if(m_pPos->nPos == m_pSourceCode->GetPos())
					bRet = false;
			}
			break;
		case 1:
			m_pSourceCode->SetPos(m_pPos->nPos);
			m_oToken.SetSize(m_pPos->nTokenLen);
			break;
		case 2:
			break;
		}
		m_oPositionStack.RemoveNext(NULL);
		void* pFirst = m_oPositionStack.First();
		if(pFirst)
			m_pPos = &m_oPositionStack.GetItem(pFirst);
		else
			m_pPos = NULL;
	}
	return bRet;
}

void CLexicalModule::OnParseError()
{
	if(m_pPos)
	{
		char c;
		if(!m_pSourceCode)
			WriteError(false, m_sLastFileName, m_nLastLine, m_nLastCol, "Missing char");
		else if(GetChar(c, false))
		{
			CCodeLine* pLine = m_pSourceCode->GetLine();
			WriteError(false, pLine->sFileName, pLine->nLine, pLine->nCol, "Missing char '%c'", c);
		}
		else
		{
			CCodeLine* pLine = m_pSourceCode->GetLine();
			WriteError(false, pLine->sFileName, pLine->nLine, pLine->nCol, "Missing char");
		}
	}
}

bool CLexicalModule::IsBroken()
{
	char c;
	return !GetChar(c, false);
}

//回车字符作为冗余字符，不会被返回，换行采用统一的'\n'
bool CLexicalModule::GetChar(char &c, bool bNextPos)
{
	if(m_pSourceCode == NULL)
		return false;
	bool bRet = m_pSourceCode->GetChar(c, bNextPos);
	if(bNextPos)
		m_cChar = c;
	return bRet;
}

void CLexicalModule::AppendToken(char c)
{
	m_oToken.Insert((uint32)(-1), c);
}

void CLexicalModule::AppendToken(const char* s)
{
	uint32 nLen = CString::StringLength(s);
	if(nLen)
		m_oToken.Insert((uint32)(-1), s, nLen);
}

void CLexicalModule::WriteErrorV(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, CVaList& pArgList)
{
	CFileFormatter oFormater(m_pErrorFile);
	if(bWarning)
	{
		++m_nWarningCount;
		oFormater.Print("Warning: %s[%u,%u]: ", sFileName, nLine, nCol);
	}
	else
	{
		++m_nErrorCount;
		oFormater.Print("Error: %s[%u,%u]: ", sFileName, nLine, nCol);
	}
	oFormater.PrintV(sFormat, pArgList);
	oFormater.Print("\n");
}

void CLexicalModule::WriteError(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	WriteErrorV(bWarning, sFileName, nLine, nCol, sFormat, pArgList);
	VaEnd(pArgList);
}

void CLexicalModule::OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	WriteErrorV(false, oFileInfo.m_sFileName, oFileInfo.m_nLine, oFileInfo.m_nCol, sFormat, pArgList);
	VaEnd(pArgList);
}

uint32 CLexicalModule::GetErrorCount()
{
	return m_nErrorCount;
}

uint32 CLexicalModule::GetWarningCount()
{
	return m_nWarningCount;
}

bool CLexicalModule::IsSpace(char c)
{
	const char* sSpace = m_pLexicalSystem->m_oWhiteSpace.GetStr();
	if(sSpace[0])
		return CString::CharOfString(sSpace, c)!=NULL;
	else
		return CString::IsSpace(c);
}

bool CLexicalModule::IsPunct(char c)
{
	const char* sPuncts = m_pLexicalSystem->m_oPuncts.GetStr();
	if(sPuncts[0])
		return CString::CharOfString(sPuncts, c)!=NULL;
	else
		return CString::IsPunct(c);
}

bool CLexicalModule::JumpSpace()
{
	char c;
	bool bRet = false;
	while(GetChar(c, false))
	{
		if(IsSpace(c))
		{
			GetChar(c);
			bRet = true;
		}
		else
			break;
	}
	return bRet;
}

void CLexicalModule::JumpToTokenEnd()
{
	char c;
	if(IsSpace(m_cChar) || IsPunct(m_cChar))
		return;
	while(GetChar(c, false))
	{
		if(IsSpace(c) || IsPunct(c))
			break;
		GetChar(c);
	}
}

uint32 CLexicalModule::CheckTokenEnd()
{
	char c;
	CCodeLine *pLine = m_pSourceCode->GetLine();
	if(m_pPos->sFileName != pLine->sFileName)
	{//一个单词不能跨文件
		WriteError(false, m_pPos->sFileName, m_pPos->nLine, m_pPos->nCol, "the token is broken in the end of file");
		return 2;
	}
	if(IsSpace(m_cChar) || IsPunct(m_cChar))
		return 0;
	if(GetChar(c, false))
	{
		if(IsSpace(c) || IsPunct(c))
			return 0;
		return 1;
	}
	return 0;
}

const char* CLexicalModule::GetSymbol(const CString& oSymbol)
{
	CRbTreeNode* pNode;
	pNode = m_oStringTable.Find(oSymbol);
	if(pNode == m_oStringTable.End())
		pNode = m_oStringTable.Insert(oSymbol);
	return m_oStringTable.GetItem(pNode).GetStr();
}

void CLexicalModule::Parse(CSourceCode& oSourceCode)
{
	Reset(oSourceCode);
	m_pLexicalSystem->Parse(true, *this);
}

void CLexicalModule::Reset(CSourceCode& oSourceCode)
{
	char c;
	ClearTokenTable();
	m_oStringTable.Clear();
	m_nErrorCount = 0;
	m_nWarningCount = 0;
	m_nContinuation = 0;
	m_pSourceCode = &oSourceCode;
	m_pSourceCode->GetChar(c, false);
	CCodeLine* pLine = m_pSourceCode->GetLine();
	m_sLastFileName = pLine->sFileName;
	m_nLastLine = pLine->nLine;
	m_nLastCol = pLine->nCol;
}

CToken* CLexicalModule::PullToken(bool bJmpToNextToken)
{
	if(m_pTokenIdx == NULL)//初始
	{
		CToken* pToken = m_oTokenTable.First();
		while(!pToken)
		{//因为可能扫描到的token被丢弃，所以需要循环
			if(IsBroken())
				break;
			m_pLexicalSystem->Parse(false, *this);
			pToken = m_oTokenTable.First();
		}
		if(bJmpToNextToken)
		{
			if(pToken)
				m_pTokenIdx = pToken;
			else
				m_pTokenIdx = (CToken*)(uintptr)(-1);//结束
		}
		return pToken;
	}
	if(m_pTokenIdx == (CToken*)(uintptr)(-1))//结束
		return NULL;
	CToken* pToken = m_oTokenTable.GetNext(m_pTokenIdx);
	while(!pToken)
	{//因为可能扫描到的token被丢弃，所以需要循环
		if(IsBroken())
			break;
		m_pLexicalSystem->Parse(false, *this);
		pToken = m_oTokenTable.GetNext(m_pTokenIdx);
	}
	if(bJmpToNextToken)
	{
		if(pToken)
			m_pTokenIdx = pToken;
		else
			m_pTokenIdx = (CToken*)(uintptr)(-1);//结束
	}
	return pToken;
}

CToken* CLexicalModule::PeekToken()
{
	CToken* pToken = m_oTokenTable.GetNext(m_pTokenIdx);
	if(!pToken)
	{
		if(!IsBroken())
		{
			m_pLexicalSystem->Parse(false, *this);
			pToken = m_oTokenTable.GetNext(m_pTokenIdx);
		}
		if(!pToken)
			return NULL;
	}
	CToken* pNext = m_oTokenTable.GetNext(pToken);
	if(!pNext && !IsBroken())
	{
		m_pLexicalSystem->Parse(false, *this);
		pNext = m_oTokenTable.GetNext(pToken);
	}
	return pNext;
}

CToken* CLexicalModule::GetToken()
{
	if(m_pTokenIdx == (CToken*)(uintptr)(-1))
		return NULL;
	return m_pTokenIdx;
}

void CLexicalModule::SetToken(CToken* pToken)
{
	m_pTokenIdx = pToken;
}

void CLexicalModule::SetToken(CToken* pOldToken, CToken* pNewToken)
{
	m_oTokenTable.Append(pOldToken, pNewToken);
	if(pOldToken)
	{
		m_oTokenTable.Remove(pOldToken);
		delete pOldToken;
	}
	if(pOldToken == m_pTokenIdx)
		m_pTokenIdx = pNewToken;
}

void CLexicalModule::OnUnknown()
{
	char c;
	CCodeLine* pLine = m_pSourceCode->GetLine();
	WriteError(false, pLine->sFileName, pLine->nLine, pLine->nCol, "unknown token");
	if(GetChar(c))
		JumpToTokenEnd();
}

void CLexicalModule::CreateToken(const char* sKind)
{
	CToken* pToken = new CToken(sKind);
	m_oToken.Insert((uint32)(-1), '\0');
	pToken->Parse(*this, GetSymbol(m_oToken.At(0)), m_pPos->nLine, m_pPos->nCol, m_pPos->sFileName);
	m_oTokenTable.Insert(NULL, pToken);
	if(pToken->m_pPrev)
	{
		if(pToken->m_pPrev->m_nLine != pToken->m_nLine || pToken->m_pPrev->m_sFileName != pToken->m_sFileName)
			pToken->SetNewLine();
	}
	else
		pToken->SetNewLine();
}

CTokenRule::CTokenRule(CLexicalSystem* pLexSystem, const char* sKind)
	:m_sToken(pLexSystem->GetRule(sKind)->GetName())
{
	m_pVarExp = NULL;
	m_bHome = false;
	m_bFirst = false;
}

CTokenRule::CTokenRule(const CTokenRule& oRule)
	:CRule(oRule), m_sToken(oRule.m_sToken),
	 m_bHome(oRule.m_bHome),
	 m_bFirst(oRule.m_bFirst)
{
	m_pVarExp = NULL;
	if(oRule.m_pVarExp)
		m_pVarExp = oRule.m_pVarExp->Clone();
}

CTokenRule::~CTokenRule()
{
	if(m_pVarExp)
	{
		delete m_pVarExp;
		m_pVarExp = NULL;
	}
}

CTokenRule& CTokenRule::SetOutput(CRuleExpress* pVarExp)
{
	m_pVarExp = pVarExp;
	return *this;
}

CRuleExpress* CTokenRule::GetOutExp()
{
	return m_pVarExp;
}

CTokenRule& CTokenRule::SetHome()
{
	m_bHome = true;
	return *this;
}

CTokenRule& CTokenRule::SetFirst()
{
	m_bFirst = true;
	return *this;
}

void CTokenRule::Dump(CString & oDump, uint32 nLevel)
{
	CRuleType::DumpLevel(oDump, nLevel);
	if(m_bFirst)
		oDump += "#";
	if(m_bHome)
		oDump += "$";
	oDump += m_sToken;
	if(m_pVarExp)
	{
		oDump += "<";
		m_pVarExp->Dump(oDump);
		oDump += ">";
	}
	oDump += "\n";
}

CRule* CTokenRule::Clone()const
{
	return new CTokenRule(*this);
}

static const char* g_sDebugToken = GetEnvVar("DebugRuleToken");
uint32 CTokenRule::Match(CRuleStack &oStack, bool bMust) const
{
	CSyntaxModule* pSyntax = dynamic_cast<CSyntaxModule*>(oStack.pModule);
	if(pSyntax == NULL)
		FocpAbort(("CTokenRule::Match() failure"));
	CLexicalModule* pLexical = pSyntax->m_pLexical;
	if(pLexical->IsBroken())
	{
		if(bMust)
			pLexical->WriteError(false, pLexical->m_sLastFileName, pLexical->m_nLastLine, pLexical->m_nLastCol, "Missing token '%s'", m_sToken);
		return 1;
	}
	CCodeLine* pLine = pLexical->m_pSourceCode->GetLine();
	uint32 nLine = pLine->nLine;
	uint32 nCol = pLine->nCol;
	const char* sFileName = pLine->sFileName;
	CToken* pToken = pSyntax->PullToken();
	if(pToken == NULL)
	{
		if(bMust)
			pLexical->WriteError(false, sFileName, nLine, nCol, "Missing token '%s'", m_sToken);
		return 1;
	}
	if(m_sToken != pToken->GetKind())
	{
		if(bMust)
			pLexical->WriteError(false, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "Expect token '%s'", m_sToken);
		return 1;
	}
	if(m_bHome && pToken->m_nCol != 1)
	{
		if(bMust)
			pLexical->WriteError(false, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "The token '%s' should be ahead of line", pToken->GetToken());
		return 1;
	}
	if(m_bFirst && !pToken->m_bNewLine)
	{
		if(bMust)
			pLexical->WriteError(false, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "The token '%s' should be the first token of line", pToken->GetToken());
		return 1;
	}
	if(g_sDebugToken && !CString::StringCompare(g_sDebugToken, m_sToken, false))
		Print("Debug CTokenRule::Match(%s)\n", m_sToken);
	if(m_pVarExp)
	{
		CToken** pValue = (CToken**)m_pVarExp->GetExpressValue(&oStack);
		pValue[0] = pToken;
	}
	return 0;
}

bool CTokenRule::Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel)
{
	bool bRet = true;
	if(m_pVarExp)
	{
		bRet = m_pVarExp->Check(oSystem, pChecker);
		uint32 nType = (sizeof(void*) == sizeof(uint32))?ARF_UINT32:ARF_UINT64;
		if(bRet && m_pVarExp->GetExpressType(&oSystem)->TypeCode() != nType)
		{
			pChecker->OnError(*m_pVarExp, "token express must be ulong type");
			bRet = false;
		}
		if(bRet && !m_pVarExp->IsVariable())
		{
			pChecker->OnError(*m_pVarExp, "token express must be writeable");
			bRet = false;
		}
	}
	return bRet;
}

bool CTokenRule::Same(const CRule *pRule)
{
	const CTokenRule* pTokenRule = dynamic_cast<const CTokenRule*>(pRule);
	if(!pTokenRule)
		return false;
	if((m_pVarExp && !pTokenRule->m_pVarExp) || (!m_pVarExp && pTokenRule->m_pVarExp))
		return false;
	if(m_pVarExp && !m_pVarExp->Same(*pTokenRule->m_pVarExp))
		return false;
	if(m_bHome != pTokenRule->m_bHome)
		return false;
	if(m_bFirst != pTokenRule->m_bFirst)
		return false;
	if(m_sToken != pTokenRule->m_sToken)
		return false;
	return true;
}

CSyntaxSystem::CSyntaxSystem()
{
}

CSyntaxSystem::~CSyntaxSystem()
{
}

FRuleHost CSyntaxSystem::GetHost(const char* sName) const
{
	CRbTreeNode* pIt = m_oFunctions.Find(sName);
	if(pIt != m_oFunctions.End())
		return m_oFunctions.GetItem(pIt);
	return NULL;
}

void CSyntaxSystem::RegHost(const char* sName, FRuleHost fFunc)
{
	m_oFunctions[sName] = fFunc;
}

bool CSyntaxSystem::Load(const char* sLibName)
{
	m_oLibName = sLibName;
	bool bRet = m_oLib.Load(sLibName, true);
	if(bRet)
	{
		typedef void (*FInitSyntaxSystem)(CSyntaxSystem* pSyntaxSystem);
		FInitSyntaxSystem InitSyntaxSystem = (FInitSyntaxSystem)m_oLib.FindSymbol("InitSyntaxSystem");
		if(InitSyntaxSystem)
			InitSyntaxSystem(this);
	}
	return bRet;
}

void CSyntaxSystem::Reset()
{
	m_oLibName.Clear();
	m_oLib.UnLoad();
	m_oFunctions.Clear();
}

CSyntaxRule::CSyntaxRule(CSyntaxSystem *pSyntaxSystem, const char* sName, CRule *pRule, bool bMain, ...):
	CRuleProc(pSyntaxSystem, sName, pRule, bMain, NULL, NULL)
{
	CVaList args;
	VaStart(args, bMain);
	InitializeParametersAndVariables(args);
	VaEnd(args);
}

void CSyntaxRule::OnFailure(CRuleModule* pModule)const
{
	CToken* pToken;
	CSyntaxModule* pSyntax = (CSyntaxModule*)pModule;
	pToken = pSyntax->PullToken();
	if(pToken)
		pSyntax->OnError(*pToken, "Invalid token '%s'", pToken->GetToken());
	else
	{
		CRuleFileInfo oFileInfo;
		oFileInfo.SetFile(pSyntax->m_pLexical->m_sLastFileName, pSyntax->m_pLexical->m_nLastLine, pSyntax->m_pLexical->m_nLastCol);
		pSyntax->OnError(oFileInfo, "Syntax incomplete");
	}
}

CSyntaxModule::CSyntaxModule(CLexicalModule &oLexical, CSyntaxSystem* pSyntaxSystem):
	m_pLexical(&oLexical), m_pSyntaxSystem(pSyntaxSystem)
{
}

CSyntaxModule::~CSyntaxModule()
{
}

void CSyntaxModule::Push()
{
	CTokenPos oPos;
	oPos.pIdx = m_pLexical->GetToken();
	m_oTokenStack.Push(oPos, false);
}

bool CSyntaxModule::Pop(uint32 nRet, bool bCheck)
{
	bool bRet = true;
	void* pFirst = m_oTokenStack.First();
	if(pFirst)
	{
		switch(nRet)
		{
		case 3:
		case 0:
			if(bCheck)
			{
				CToken* pPrevIdx = m_oTokenStack.GetItem(pFirst).pIdx;
				CToken* pCurrIdx = m_pLexical->GetToken();
				if(pPrevIdx == pCurrIdx)
					bRet = false;
			}
			break;
		case 1:
			m_pLexical->SetToken(m_oTokenStack.GetItem(pFirst).pIdx);
			break;
		case 2:
			break;
		}
		m_oTokenStack.RemoveNext(NULL);
	}
	return bRet;
}

void CSyntaxModule::OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	m_pLexical->WriteErrorV(false, oFileInfo.m_sFileName, oFileInfo.m_nLine, oFileInfo.m_nCol, sFormat, pArgList);
	VaEnd(pArgList);
}

void CSyntaxModule::OnUnknown()
{
	CToken* pToken = PullToken();
	m_pLexical->WriteError(false, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "Invalid token '%s'", pToken->GetToken());
}

void CSyntaxModule::OnParseError()
{
	void* pFirst = m_oTokenStack.First();
	if(pFirst)
	{
		if(m_pLexical->IsBroken())
		{
			m_pLexical->WriteError(false, m_pLexical->m_sLastFileName,
								   m_pLexical->m_nLastLine, m_pLexical->m_nLastCol, "Missing token");
		}
		else
		{
			CToken* pToken;
			CCodeLine* pLine = m_pLexical->m_pSourceCode->GetLine();
			uint32 nLine = pLine->nLine;
			uint32 nCol = pLine->nCol;
			const char* sFileName = pLine->sFileName;
			if((pToken=PullToken(false)))
				m_pLexical->WriteError(false, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "Unknown token '%s'", pToken->GetToken());
			else
				m_pLexical->WriteError(false, sFileName, nLine, nCol, "Missing token");
		}
	}
}

bool CSyntaxModule::IsBroken()
{
	return (PullToken(false) == NULL);
}

CToken* CSyntaxModule::PullToken(bool bNextToken)
{
	CToken* pToken = m_pLexical->PullToken(bNextToken);
	if(bNextToken)
		m_pToken = pToken;
	return pToken;
}

CToken* CSyntaxModule::GetCurToken()
{
	return m_pToken;
}

CToken* CSyntaxModule::PeekToken()
{
	return m_pLexical->PeekToken();
}

CLexicalModule& CSyntaxModule::GetLexical()
{
	return *m_pLexical;
}

void CSyntaxModule::Reset(CSourceCode& oSourceCode)
{
	m_oTokenStack.Clear();
	m_pLexical->Reset(oSourceCode);
}

void CSyntaxModule::Parse(CSourceCode& oSourceCode, bool bLoop)
{
	Reset(oSourceCode);
	m_pSyntaxSystem->Parse(bLoop, *this);
	if(!bLoop && !IsBroken())
		FocpError(("CSyntaxModule::Parse(false) broken"));
}

CCompileSystem::CCompileSystem()
{
	m_bChecked = false;
}

CCompileSystem::~CCompileSystem()
{
}

bool CCompileSystem::Load(const char* sLibName)
{
	return m_oSyntaxSystem.Load(sLibName);
}

void CCompileSystem::Dump(CString & oDump)
{
	oDump = "";
	m_oLexicalSystem.Dump(oDump);
	m_oSyntaxSystem.Dump(oDump);
}

CLexicalSystem* CCompileSystem::GetLexicalSystem()
{
	return &m_oLexicalSystem;
}

CSyntaxSystem* CCompileSystem::GetSyntaxSystem()
{
	return &m_oSyntaxSystem;
}

bool CCompileSystem::Check(CCompileModule& oModule)
{
	bool bRet = true;
	m_oMutex.Enter();
	if(m_bChecked == false)
	{
		if(!m_oLexicalSystem.Check(oModule.GetLexicalModule()))
			bRet = false;
		if(!m_oSyntaxSystem.Check(oModule.GetSyntaxModule()))
			bRet = false;
		m_bChecked = bRet;
	}
	m_oMutex.Leave();
	return bRet;
}

void CCompileSystem::Reset()
{
	m_oMutex.Enter();
	m_bChecked = false;
	m_oLexicalSystem.Reset();
	m_oSyntaxSystem.Reset();
	m_oMutex.Leave();
}

CCompileModule::CCompileModule(CFile &oErrorFile, CCompileSystem& oCompileSystem):
	m_oLexicalModule(&oCompileSystem.m_oLexicalSystem, oErrorFile),
	m_oSyntaxModule(m_oLexicalModule, &oCompileSystem.m_oSyntaxSystem)
{
}

CCompileModule::~CCompileModule()
{
}

bool CCompileModule::Compile(CFile& oSrcFile)
{
	FocpInfo(("Begin compile '%s'", oSrcFile.GetFileName().oConnectName.GetStr()));
	CSourceFile oSourceCode(&m_oLexicalModule, oSrcFile);
	m_oSyntaxModule.Parse(oSourceCode);
	FocpInfo(("End compile '%s'", oSrcFile.GetFileName().oConnectName.GetStr()));
	return (0 == GetErrorCount());
}

uint32 CCompileModule::GetErrorCount()
{
	return m_oLexicalModule.GetErrorCount();
}

uint32 CCompileModule::GetWarningCount()
{
	return m_oLexicalModule.GetWarningCount();
}

CCompileModule* CCompileModule::GetCompileModule(CRuleModule* pModule)
{
	CLexicalModule* pLexical = dynamic_cast<CLexicalModule*>(pModule);
	if(pLexical)
		return (CCompileModule*)((char*)pLexical-FocpFieldOffset(CCompileModule,m_oLexicalModule));
	CSyntaxModule* pSyntax = dynamic_cast<CSyntaxModule*>(pModule);
	if(pSyntax)
		return (CCompileModule*)((char*)pSyntax-FocpFieldOffset(CCompileModule,m_oSyntaxModule));
	return NULL;
}

CLexicalModule* CCompileModule::GetLexicalModule()
{
	return &m_oLexicalModule;
}

CSyntaxModule* CCompileModule::GetSyntaxModule()
{
	return &m_oSyntaxModule;
}

FOCP_END();
