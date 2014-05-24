
#include "LogChooser.hpp"

FOCP_BEGIN();

CLogChoice::CLogChoice(uint32 nType)
{
	m_nType = nType;
	m_bNot = false;
}

CLogChoice::~CLogChoice()
{
}

void CLogChoice::SetNot(bool bNot)
{
	m_bNot = bNot;
}

void CLogChoice::GetNot(bool &bNot)
{
	bNot = m_bNot;
}

CLogIntChoice::CLogIntChoice(uint32 nType)
	:CLogChoice(nType)
{
	m_nValue = 15;//默认全选
}

CLogIntChoice::~CLogIntChoice()
{
}

void CLogIntChoice::SetValue(uint32 nValue)
{
	m_nValue = nValue;
}

void CLogIntChoice::GetValue(uint32 &nValue)
{
	nValue = m_nValue;
}

bool CLogIntChoice::Check(CLogMsg& oLog)
{
	bool bRet = false;
	switch(m_nType)
	{
	case 7:
		//1-syslog, 2-errlog, 4-warnlog, 8-dbglog.
		if(oLog.nLevel == FOCP_LOG_SYSLOG)
			bRet = (m_nValue & 1)?true:false;
		else if(oLog.nLevel == FOCP_LOG_ERROR)
			bRet = (m_nValue & 2)?true:false;
		else if(oLog.nLevel == FOCP_LOG_WARNING)
			bRet = (m_nValue & 4)?true:false;
		else if(oLog.nLevel == FOCP_LOG_DEBUG)
			bRet = (m_nValue & 8)?true:false;
		break;
	case 8:
		bRet = (m_nValue == oLog.nDMN);
		break;
	case 9:
		bRet = (m_nValue == oLog.nAIN);
		break;
	case 10:
		bRet = (m_nValue == oLog.nLine);
		break;
	}
	if(m_bNot)
		bRet = !bRet;
	return bRet;
}

void CLogIntChoice::GetChooser(CString& oChooser)
{
	CFormatString oVal;
	oVal.Print("%u", m_nValue);
	switch(m_nType)
	{
	case 7:
		oChooser += "Level";
		break;
	case 8:
		oChooser += "DMN";
		break;
	case 9:
		oChooser += "AIN";
		break;
	case 10:
		oChooser += "Line";
		break;
	}
	if(m_bNot)
		oChooser += "!";
	oChooser += "=";
	oChooser += oVal;
}

CLogTimeChoice::CLogTimeChoice(uint32 nType)
	:CLogChoice(nType)
{
	m_bIncBeg = true;
	m_bIncEnd = true;
	m_nBegin = 0;
	m_nEnd = 0;
}

CLogTimeChoice::~CLogTimeChoice()
{
}

void CLogTimeChoice::SetBegin(double nBegin, bool bInc)
{
	m_nBegin = nBegin;
	m_bIncBeg = bInc;
	if(m_nBegin > m_nEnd)
	{
		m_nEnd = m_nBegin;
		m_bIncEnd = bInc;
	}
}

void CLogTimeChoice::GetBegin(double &nBegin, bool &bInc)
{
	nBegin = m_nBegin;
	bInc = m_bIncBeg;
}

void CLogTimeChoice::SetEnd(double nEnd, bool bInc)
{
	m_nEnd = nEnd;
	m_bIncEnd = bInc;
	if(m_nBegin > m_nEnd)
	{
		m_nBegin = nEnd;
		m_bIncBeg = bInc;
	}
}

void CLogTimeChoice::GetEnd(double &nEnd, bool &bInc)
{
	nEnd = m_nEnd;
	bInc = m_bIncEnd;
}

bool CLogTimeChoice::Check(CLogMsg& oLog)
{
	bool bCond1 = m_bIncBeg?(oLog.nDate>=m_nBegin):(oLog.nDate>m_nBegin);
	bool bCond2 = m_bIncEnd?(oLog.nDate<=m_nEnd):(oLog.nDate<m_nEnd);
	bool bRet = bCond1 && bCond2;
	if(m_bNot)
		bRet = !bRet;
	return bRet;
}

void CLogTimeChoice::GetChooser(CString& oChooser)
{
	CString oBegDate, oEndDate;
	CDateTime(m_nBegin).GetString(oBegDate);
	CDateTime(m_nEnd).GetString(oEndDate);
	CFormatString oFmt;
	oFmt.Print("Date%s%s'%s','%s'%s", m_bNot?"!=":"=", m_bIncBeg?"[":"(", oBegDate.GetStr(), oEndDate.GetStr(), m_bIncEnd?"]":")");
	oChooser += oFmt;
}

CLogTextChoice::CLogTextChoice(uint32 nType)
	:CLogChoice(nType)
{
	m_nValue = 15;//默认全选
}

CLogTextChoice::~CLogTextChoice()
{
}

void CLogTextChoice::SetValue(const CString &nValue)
{
	m_nValue = nValue;
}

void CLogTextChoice::GetValue(CString &nValue)
{
	nValue = m_nValue;
}

bool CLogTextChoice::Check(CLogMsg& oLog)
{
	bool bRet = false;
	switch(m_nType)
	{
	case 0:
		bRet = !m_nValue.Compare(oLog.oHost);
		break;
	case 2:
		bRet = !m_nValue.Compare(oLog.oAppName, false);
		break;
	case 3:
		bRet = !m_nValue.Compare(oLog.oModuleName, false);
		break;
	case 4:
		bRet = !m_nValue.Compare(oLog.oFuncName, false);
		break;
	case 5:
		bRet = !m_nValue.Compare(oLog.oFile, false);
		break;
	case 6:
		bRet = (NULL != CString::StringOfString(oLog.oInfo.GetStr(), m_nValue.GetStr(), false));
		break;
	}
	if(m_bNot)
		bRet = !bRet;
	return bRet;
}

void CLogTextChoice::GetChooser(CString& oChooser)
{
	CString oName;
	switch(m_nType)
	{
	case 0:
		oName = "Host";
		break;
	case 2:
		oName = "App";
		break;
	case 3:
		oName = "Module";
		break;
	case 4:
		oName = "Func";
		break;
	case 5:
		oName = "File";
		break;
	case 6:
		oName = "Key";
		break;
	}
	oChooser += oName;
	if(m_bNot)
		oChooser += "!='";
	else
		oChooser += "='";
	GetCaption(oChooser);
	oChooser += "'";
}

void CLogTextChoice::GetCaption(CString &oVal)
{
	const char* s = m_nValue.GetStr();
	while(s[0])
	{
		if(s[0] == '\'')
			oVal += s[0];
		oVal += s[0];
		++s;
	}
}

CLogChoiceUnit::CLogChoiceUnit()
{
	for(uint32 i=0; i<11; ++i)
		m_pChoiceUnit[i] = NULL;
}

CLogChoiceUnit::~CLogChoiceUnit()
{
	for(uint32 i=0; i<11; ++i)
	{
		if(m_pChoiceUnit[i])
		{
			delete m_pChoiceUnit[i];
			m_pChoiceUnit[i] = NULL;
		}
	}
}

CLogChoice* CLogChoiceUnit::GetChoice(uint32 nIdx, bool bCreate)
{
	if(nIdx >= 11)
		return NULL;
	CLogChoice* &pChoice = m_pChoiceUnit[nIdx];
	if(!pChoice && bCreate)
	{
		if(nIdx == 1)
			pChoice = new CLogTimeChoice(nIdx);
		else if(nIdx >= 7)
			pChoice = new CLogIntChoice(nIdx);
		else
			pChoice = new CLogTextChoice(nIdx);
	}
	return pChoice;
}

bool CLogChoiceUnit::Check(CLogMsg& oLog)
{
	bool bRet = true;
	for(uint32 i=0; i<11; ++i)
	{
		if(m_pChoiceUnit[i] && !m_pChoiceUnit[i]->Check(oLog))
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}

void CLogChoiceUnit::GetChooser(CString& oChooser)
{
	bool bHave = false;
	for(uint32 i=0; i<11; ++i)
	{
		if(m_pChoiceUnit[i])
		{
			if(bHave)
				oChooser += " and ";
			m_pChoiceUnit[i]->GetChooser(oChooser);
		}
	}
}

bool CLogChoiceUnit::Empty()
{
	bool bHave = false;
	for(uint32 i=0; i<11; ++i)
	{
		if(m_pChoiceUnit[i])
		{
			bHave = true;
			break;
		}
	}
	return !bHave;
}

CLogChoiceGroup::CLogChoiceGroup()
{
}

CLogChoiceGroup::~CLogChoiceGroup()
{
	CLogChoiceUnit* pUnit;
	while(m_oUnits.Pop(pUnit))
		delete pUnit;
}

bool CLogChoiceGroup::Check(CLogMsg& oLog)
{
	bool bRet = true;
	for(void* pIt=m_oUnits.First(); pIt; pIt = m_oUnits.GetNext(pIt))
	{
		CLogChoiceUnit* pUnit = m_oUnits.GetItem(pIt);
		if(!pUnit->Check(oLog))
		{
			bRet = false;
			break;
		}
	}
	return bRet;
}

CLogChoice* CLogChoiceGroup::NewChoice(uint32 nType)
{
	void* pIt;
	CLogChoice* pChoice = NULL;
	for(pIt = m_oUnits.First(); pIt; pIt=m_oUnits.GetNext(pIt))
	{
		CLogChoiceUnit* pUnit = m_oUnits.GetItem(pIt);
		pChoice = pUnit->GetChoice(nType);
		if(!pChoice)
		{
			pChoice = pUnit->GetChoice(nType, true);
			break;
		}
	}
	if(!pChoice)
	{
		CLogChoiceUnit* pUnit = new CLogChoiceUnit;
		pChoice = pUnit->GetChoice(nType, true);
		m_oUnits.Push(pUnit);
	}
	return pChoice;
}

void CLogChoiceGroup::GetChooser(CString& oChooser)
{
	bool bHave = false;
	for(void* pIt=m_oUnits.First(); pIt; pIt = m_oUnits.GetNext(pIt))
	{
		if(bHave)
			oChooser += " and ";
		CLogChoiceUnit* pUnit = m_oUnits.GetItem(pIt);
		pUnit->GetChooser(oChooser);
		bHave = true;
	}
}

bool CLogChoiceGroup::Empty()
{
	bool bHave = false;
	void *pPrev = NULL, *pIt=m_oUnits.First();
	while(pIt)
	{
		CLogChoiceUnit* pUnit = m_oUnits.GetItem(pIt);
		if(pUnit->Empty())
		{
			void* pNext = m_oUnits.GetNext(pIt);
			m_oUnits.RemoveNext(pPrev);
			pIt = pNext;
			delete pUnit;
		}
		else
		{
			bHave = true;
			pPrev = pIt;
			pIt = m_oUnits.GetNext(pIt);
		}
	}
	return !bHave;
}

CLogChooser::CLogChooser()
{
}

CLogChooser::~CLogChooser()
{
	Clear();
}

bool CLogChooser::Check(CLogMsg& oLog)
{
	bool bRet = true;
	m_oMutex.Enter();
	if(m_oGroups.GetSize())
	{
		bRet = false;
		for(void* pIt=m_oGroups.First(); pIt; pIt = m_oGroups.GetNext(pIt))
		{
			CLogChoiceGroup* pGroup = m_oGroups.GetItem(pIt);
			if(pGroup->Check(oLog))
			{
				bRet = true;
				break;
			}
		}
	}
	m_oMutex.Leave();
	return bRet;
}

void CLogChooser::GetChooser(CString& oChooser)
{
	oChooser = "";
	bool bHave = false;
	m_oMutex.Enter();
	for(void* pIt=m_oGroups.First(); pIt; pIt = m_oGroups.GetNext(pIt))
	{
		if(bHave)
			oChooser += " or ";
		CLogChoiceGroup* pGroup = m_oGroups.GetItem(pIt);
		pGroup->GetChooser(oChooser);
		bHave = true;
	}
	m_oMutex.Leave();
}

void CLogChooser::RemoveEmptyGroup()
{
	void *pPrev = NULL, *pIt=m_oGroups.First();
	while(pIt)
	{
		CLogChoiceGroup* pGroup = m_oGroups.GetItem(pIt);
		if(pGroup->Empty())
		{
			void* pNext = m_oGroups.GetNext(pIt);
			m_oGroups.RemoveNext(pPrev);
			pIt = pNext;
			delete pGroup;
		}
		else
		{
			pPrev = pIt;
			pIt = m_oGroups.GetNext(pIt);
		}
	}
}

void CLogChooser::PopChooser()
{
	m_oMutex.Enter();
	CLogChoiceGroup* pGroup;
	if(m_oGroups.Pop(pGroup))
		delete pGroup;
	m_oMutex.Leave();
}

void CLogChooser::Clear()
{
	m_oMutex.Enter();
	CLogChoiceGroup* pGroup;
	while(m_oGroups.Pop(pGroup))
		delete pGroup;
	m_oMutex.Leave();
}

//(1)支持and与or以构建复杂的条件
//(2)支持=与!=操作
//(3)值用单引号扩起（两个连续单引号表示一个单引号值）
//(4)日期的值格式为:[a,b];[a,b);(a,b];(a,b);
//(5)Host,Date,App,Module,Func,File,Key,Level,DMN,AIN,Line
bool CLogChooser::AddChooser(const char* sCond)
{
	bool bFirst = true;
	const char* sWhere = sCond;
	m_oMutex.Leave();
	AddGroup();
	if(sCond)while(sCond[0])
		{
			bool bNot, bIncBeg, bIncEnd;
			CString oName, oVal1, oVal2;
			if(!sCond[0])
				break;
			if(GetIdentifier(sCond, oName))
			{
				RemoveEmptyGroup();
				m_oMutex.Leave();
				FocpError(("CLogChooser::AddChooser(%u:%s): get compare item failure", sCond-sWhere, sWhere));
				return false;
			}
			if(!bFirst)
			{
				if(!oName.Compare("and", false))
					continue;
				if(!oName.Compare("or", false))
				{
					AddGroup();
					continue;
				}
			}
			bFirst = false;
			uint32 nRet, nType = GetItemType(oName);
			if(nType >= 11)
			{
				RemoveEmptyGroup();
				m_oMutex.Leave();
				FocpError(("CLogChooser::AddChooser(%u:%s): the item '%s' is invalid", sCond-sWhere, sWhere, oName.GetStr()));
				return false;
			}
			if(GetOperator(sCond, bNot))
			{
				RemoveEmptyGroup();
				m_oMutex.Leave();
				FocpError(("CLogChooser::AddChooser(%u:%s): get oprand failure", sCond-sWhere, sWhere));
				return false;
			}
			if(nType == 1)
				nRet = GetDateTime(sCond, oVal1, oVal2, bIncBeg, bIncEnd);
			else if(nType >= 7)
				nRet = GetInt(sCond, oVal1);
			else
				nRet = GetString(sCond, oVal1);
			if(nRet)
			{
				RemoveEmptyGroup();
				m_oMutex.Leave();
				FocpError(("CLogChooser::AddChooser(%u:%s): get value failure", sCond-sWhere, sWhere));
				return false;
			}
			AddCond(nType, bNot, oVal1, oVal2, bIncBeg, bIncEnd);
		}
	RemoveEmptyGroup();
	m_oMutex.Leave();
	return true;
}

uint32 CLogChooser::GetItemType(const CString &oName)
{
	if(!oName.Compare("Host", false))
		return 0;
	if(!oName.Compare("Date", false))
		return 1;
	if(!oName.Compare("App", false))
		return 2;
	if(!oName.Compare("Module", false))
		return 3;
	if(!oName.Compare("Func", false))
		return 4;
	if(!oName.Compare("File", false))
		return 5;
	if(!oName.Compare("Key", false))
		return 6;
	if(!oName.Compare("Level", false))
		return 7;
	if(!oName.Compare("DMN", false))
		return 8;
	if(!oName.Compare("AIN", false))
		return 9;
	if(!oName.Compare("Line", false))
		return 10;
	return 11;
}

void CLogChooser::AddGroup()
{
	m_oGroups.Push(new CLogChoiceGroup);
}

void CLogChooser::AddCond(uint32 nType, bool bNot, const CString& oVal1, const CString &oVal2, bool bIncBeg, bool bIncEnd)
{
	CLogChoiceGroup* pGroup = m_oGroups.GetItem(m_oGroups.Last());
	CLogChoice* pChoice = pGroup->NewChoice(nType);
	pChoice->SetNot(bNot);
	if(nType == 1)
	{
		CDateTime nBegin(oVal1), nEnd(oVal2);
		CLogTimeChoice* pTimeChoice = (CLogTimeChoice*)pChoice;
		pTimeChoice->SetBegin(nBegin.GetValue(), bIncBeg);
		pTimeChoice->SetEnd(nEnd.GetValue(), bIncEnd);
	}
	else if(nType >= 7)
	{
		uint32 nValue = CString::Atoi(oVal1.GetStr());
		CLogIntChoice* pIntChoice = (CLogIntChoice*)pChoice;
		pIntChoice->SetValue(nValue);
	}
	else
	{
		CLogTextChoice* pTextChoice = (CLogTextChoice*)pChoice;
		pTextChoice->SetValue(oVal1);
	}
}

uint32 CLogChooser::GetDateTime(const char* &pStr, CString& oVal1, CString &oVal2, bool &bIncBeg, bool &bIncEnd)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] =='[')
	{
		bIncBeg = true;
		++pStr;
	}
	else if(pStr[0] =='(')
	{
		bIncBeg = false;
		++pStr;
	}
	else
		return 1;
	if(GetString(pStr,oVal1))
		return 1;
	SkipWhiteSpace(pStr);
	if(pStr[0] ==',')
	{
		bIncBeg = true;
		++pStr;
	}
	else
		return 1;
	if(GetString(pStr,oVal2))
		return 1;
	SkipWhiteSpace(pStr);
	if(pStr[0] ==']')
	{
		bIncBeg = true;
		++pStr;
	}
	else if(pStr[0] ==')')
	{
		bIncBeg = false;
		++pStr;
	}
	else
		return 1;
	return 0;
}

uint32 CLogChooser::GetInt(const char* &pStr, CString &oValue)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] =='+' || pStr[0] == '-')
	{
		++pStr;
		oValue = pStr[0];
	}
	while(pStr[0] >= '0' && pStr[0] <= '9')
	{
		oValue += pStr[0];
		++pStr;
	}
	if(oValue.GetSize())
		return 0;
	return 1;
}

uint32 CLogChooser::GetString(const char* &pStr, CString &oValue)
{
	SkipWhiteSpace(pStr);
	if(pStr[0] =='\'')
	{
		++pStr;
		while(pStr[0])
		{
			if(pStr[0] == '\'' && pStr[1] == '\'')
			{
				pStr += 2;
				oValue += '\'';
			}
			else if(pStr[0] == '\'' || pStr[0] == '\n' || pStr[0] == '\r')
				break;
			oValue += pStr[0];
			++pStr;
		}
		if(pStr[0] == '\'')
		{
			++pStr;
			return 0;
		}
	}
	return 1;
}

uint32 CLogChooser::GetOperator(const char* &pStr, bool &bNot)
{
	SkipWhiteSpace(pStr);
	bNot = false;
	if(pStr[0] == '!')
	{
		bNot = true;
		++pStr;
	}
	if(pStr[0] == '=')
	{
		++pStr;
		return 0;
	}
	return 1;
}

uint32 CLogChooser::GetIdentifier(const char* &pStr, CString &oIdentifier)
{
	SkipWhiteSpace(pStr);
	if( (pStr[0] >= 'a' && pStr[0] <= 'z') || (pStr[0] >= 'A' && pStr[0] <= 'Z') || pStr[0] == '_')
	{
		oIdentifier += pStr[0];
		++pStr;
		while( (pStr[0] >= 'a' && pStr[0] <= 'z') ||
				(pStr[0] >= 'A' && pStr[0] <= 'Z') ||
				pStr[0] == '_' ||
				(pStr[0] >= '0' && pStr[0] <= '9') )
		{
			oIdentifier += pStr[0];
			++pStr;
		}
		return 0;
	}
	return 1;
}

void CLogChooser::SkipWhiteSpace(const char* &pStr)
{
	while(pStr[0] == ' ' || pStr[0] == '\t')
		++pStr;
}

uint32 CLogChooser::GetFilter(const char* &sFilter)
{
	//Host,Date,App,Module,File,Func
	uint32 nFilter = 0;
	if(sFilter)
	{
		SkipWhiteSpace(sFilter);
		if(sFilter[0] == '*')
		{
			nFilter = 63;
			SkipWhiteSpace(sFilter);
			if(sFilter[0])
				return 0;
		}
		else while(sFilter[0])
		{
			CString oName;
			GetIdentifier(sFilter, oName);
			if(!oName.Compare("Host", false))
				nFilter |= FOCP_LOG_HOST;
			else if(!oName.Compare("Date", false))
				nFilter |= FOCP_LOG_DATE;
			else if(!oName.Compare("App", false))
				nFilter |= FOCP_LOG_APPN;
			else if(!oName.Compare("Module", false))
				nFilter |= FOCP_LOG_MODU;
			else if(!oName.Compare("File", false))
				nFilter |= FOCP_LOG_SRCF;
			else if(!oName.Compare("Func", false))
				nFilter |= FOCP_LOG_FUNC;
			else
				return 0;
			SkipWhiteSpace(sFilter);
			if(sFilter[0] == ',')
				++sFilter;
			else if(sFilter[0])
				return 0;
		}
	}
	return nFilter;
}

FOCP_END();
