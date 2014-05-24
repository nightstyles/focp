
#include "TextTable.hpp"
#include "Application.hpp"

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

CFieldValue::CFieldValue()
{
	sVal = NULL;
	nSize = 0;
}

CFieldValue::~CFieldValue()
{
	if(sVal)
	{
		CMalloc::Free(sVal);
		sVal = NULL;
		nSize = 0;
	}
}

CFieldValue::CFieldValue(const CFieldValue& oVal)
{
	sVal = oVal.sVal;
	nSize = oVal.nSize;
	CFieldValue & oField = (CFieldValue&)oVal;
	oField.sVal = NULL;
	oField.nSize = 0;
}

CFieldValue& CFieldValue::operator=(const CFieldValue& oVal)
{
	if(this != &oVal)
	{
		sVal = oVal.sVal;
		nSize = oVal.nSize;
		CFieldValue & oField = (CFieldValue&)oVal;
		oField.sVal = NULL;
		oField.nSize = 0;
	}
	return *this;
}

void CFieldValue::Parse(const CString &oValue)
{
	uint8 nCh;
	CBinary oBinary;
	uint32 nCapacity;
	const uint8* sStr = (const uint8*)oValue.GetStr();

	if(sVal)
		CMalloc::Free(sVal);
	if(CString::StringCompare((const char*)sStr, "NULL", false))
	{
		while(*sStr)
		{
			switch(*sStr)
			{
			default:
				oBinary.Write(oBinary.GetSize(), sStr, 1);
				break;
			case '\\':
				++sStr;
				switch(*sStr)
				{
				default:
					nCh = *sStr;
					break;
				case '\0':
					continue;
				case 'w':
					nCh = ' ';
					break;
				case 'r':
					nCh = '\r';
					break;
				case 'n':
					nCh = '\n';
					break;
				case 'f':
					nCh = '\f';
					break;
				case 'v':
					nCh = '\v';
					break;
				case 't':
					nCh = '\t';
					break;
				case 'a':
					nCh = '\a';
					break;
				case 'b':
					nCh = '\b';
					break;
				case 'x':
				case 'X':
					++sStr;
					ReadHex(sStr, nCh);
					break;
				}
				oBinary.Write(oBinary.GetSize(), &nCh, 1);
			}
			++sStr;
		}
		if(oBinary.GetSize())
		{
			nCh = 0;
			oBinary.Write(oBinary.GetSize(), &nCh, 1);
		}
	}
	sVal = (char*)oBinary.Detach(nSize, nCapacity);
}

void CFieldValue::EnCode(CString &oValue)
{
	oValue.Clear();
	uint32 nFactSize = nSize - 1;
	if(sVal == NULL)
		oValue = "NULL";
	else for(uint32 i=0; i<nFactSize; ++i)
		{
			switch(*sVal)
			{
			case ' ':
				oValue += "\\w";
				break;
			case '\r':
				oValue += "\\r";
				break;
			case '\n':
				oValue += "\\n";
				break;
			case '\f':
				oValue += "\\f";
				break;
			case '\v':
				oValue += "\\v";
				break;
			case '\t':
				oValue += "\\t";
				break;
			case '\a':
				oValue += "\\a";
				break;
			case '\b':
				oValue += "\\b";
				break;
			default:
				if(*sVal < '\0' || CString::IsPrint(*sVal))
					oValue += *sVal;
				else
				{
					char s[]="\\x00";
					ToHex(s+2, (uint8)(*sVal));
					oValue += s;
				}
				break;
			}
		}
}

void CFieldValue::ToHex(char s[3], uint8 nCh)
{
	uint8 c = (nCh>>4);
	if(c < 10)
		s[0] = c + '0';
	else
		s[0] = c - 10 + 'A';
	c = (nCh & 0x0F);
	if(c < 10)
		s[1] = c + '0';
	else
		s[1] = c - 10 + 'A';
}

void CFieldValue::ReadHex(const uint8* &sStr, uint8 &nCh)
{
	uint8 c = 0;

	if(sStr[0] >= '0' && sStr[0] <= '9')
		c = sStr[0] - '0';
	else if(sStr[0] >= 'A' && sStr[0] <= 'F')
		c = sStr[0] - 'A' + 10;
	else if(sStr[0] >= 'a' && sStr[0] <= 'f')
		c = sStr[0] - 'a' + 10;
	else
	{
		--sStr;
		return;
	}
	nCh = c;
	++sStr;
	if(sStr[0] >= '0' && sStr[0] <= '9')
		c = sStr[0] - '0';
	else if(sStr[0] >= 'A' && sStr[0] <= 'F')
		c = sStr[0] - 'A' + 10;
	else if(sStr[0] >= 'a' && sStr[0] <= 'f')
		c = sStr[0] - 'a' + 10;
	else
	{
		--sStr;
		return;
	}
	nCh <<= 4;
	nCh |= c;
}

void CFieldValue::Set(const char * pVal, uint32 nLength)
{
	CBinary oBinary;
	uint32 nCapacity;
	if(pVal)
	{
		if(nLength == 0)
			nLength = CString::StringLength(pVal);
		if(nLength)
		{
			uint8 ch = 0;
			oBinary.Write(0, (const uint8*)pVal, nLength);
			oBinary.Write(nLength, &ch, 1);
		}
	}
	if(sVal)
		CMalloc::Free(sVal);
	sVal = (char*)oBinary.Detach(nSize, nCapacity);
}

bool CFieldValue::Equal(const CFieldValue &oSrc, bool bCaseSensitive)
{
	if(nSize != oSrc.nSize)
		return false;
	if(sVal == oSrc.sVal)
		return true;
	return (CBinary::MemoryCompare(sVal, oSrc.sVal, nSize, bCaseSensitive)==0);
}

FOCP_DETAIL_END();

CTextTable::CTextTable()
{
	m_bDirty = false;
}

CTextTable::~CTextTable()
{
	Clear();
}

void CTextTable::Clear()
{
	Flush();
	m_oColInfo.Clear();
	m_oTable.Clear();
	m_sFileName[0] = '\0';
	m_bDirty = false;
}

bool CTextTable::LoadTable(const char * sFile)
{
	CFormatFile oFile;
	CString::StringCopy(m_sFileName, sFile);
	if(oFile.Open(CString("disk://").Append(m_sFileName).GetStr(), "r"))
	{
		FocpLog(FOCP_LOG_ERROR, ("Open %s failure", m_sFileName));
		return false;
	}

	CString oLine;
	bool bReadColNameOk = false;
	const char* pShift;

	CVector< CString > oColTable;

	while(oFile.Scan("%r", &oLine)==1)
	{
		pShift = CString::SkipSpace(oLine.GetStr());
		if(pShift[0]== '\0' || pShift[0] == '#')
			continue;//丢弃注释行或空行

		ReadColTable(pShift, oColTable);

		if(bReadColNameOk == false)
		{
			if(!CheckColInfo(oColTable))
			{
				FocpLog(FOCP_LOG_ERROR, ("Check %s column information failure", sFile));
				return false;
			}
			CopyColInfo(oColTable);
			bReadColNameOk = true;
		}
		else
		{
			uint32 nRowNum = m_oTable.GetSize();
			if(oColTable.GetSize() < m_oColInfo.GetSize())
			{
				FocpLog(FOCP_LOG_ERROR, ("(%s:%u) the column amount is less than the field amount", sFile, nRowNum+1));
				return false;
			}
			m_oTable.Insert(nRowNum);
			CVector<FOCP_DETAIL_NAME::CFieldValue> &oRecord = m_oTable[nRowNum];
			oRecord.SetSize(m_oColInfo.GetSize());
			ReadRecord(oRecord, oColTable);
		}
	}
	if(bReadColNameOk == false)
	{
		FocpLog(FOCP_LOG_ERROR, ("Check %s column information failure", sFile));
		return false;
	}
	return true;
}

uint32 CTextTable::GetRowCount()
{
	return m_oTable.GetSize();
}

uint32 CTextTable::GetColCount()
{
	return m_oColInfo.GetSize();
}

const char* CTextTable::GetCell(uint32 nRow, uint32 nCol, uint32 &nLength)
{
	if(nRow >= m_oTable.GetSize() || nCol >= m_oColInfo.GetSize())
		return NULL;
	FOCP_DETAIL_NAME::CFieldValue &oField = m_oTable[nRow][nCol];
	nLength = oField.nSize;
	return oField.sVal;
}

const char* CTextTable::GetColName(uint32 nCol)
{
	if(nCol >= m_oColInfo.GetSize())
		return NULL;
	return m_oColInfo[nCol].GetStr();
}

uint32 CTextTable::GetCol(const char* sColName)
{
	if(!sColName)
		return 0xFFFFFFFF;
	uint32 nSize = m_oColInfo.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!m_oColInfo[i].Compare(sColName, false))
			return i;
	}
	return 0xFFFFFFFF;
}

void CTextTable::ReadColTable(const char* pShift, CVector< CString > &oColTable)
{
	CString oCol;
	oColTable.Clear();
	while(ReadCol(pShift, oCol))
		oColTable.Insert(oColTable.GetSize(), oCol);
}

bool CTextTable::ReadCol(const char* &pShift, CString &oCol)
{
	char* pToken = CString::TokenOfString(pShift);
	if(pToken == NULL)
		return false;
	oCol.Assign(pToken, pShift - pToken);
	return true;
}

bool CTextTable::CheckColInfo(CVector< CString > &oColTable)
{
	uint32 nSize = oColTable.GetSize();
	if(nSize == 0)
		return false;
	for(uint32 i=0; i<nSize; ++i)
	{
		if(!oColTable[i].IsIdentifierOfC())
			return false;
	}
	return true;
}

void CTextTable::CopyColInfo(CVector< CString > &oColTable)
{
	uint32 nSize = oColTable.GetSize();
	m_oColInfo.SetSize(nSize);
	for(uint32 i=0; i<nSize; ++i)
		m_oColInfo[i].Swap(oColTable[i]);
}

void CTextTable::ReadRecord(CVector<FOCP_DETAIL_NAME::CFieldValue>& oRecord, const CVector< CString > &oColTable)
{
	uint32 nSize = oRecord.GetSize();
	for(uint32 i=0; i<nSize; ++i)
		oRecord[i].Parse(oColTable[i]);
}

void CTextTable::Truncate(bool bFlush)
{
	if(m_oTable.GetSize())
	{
		m_oTable.Clear();
		m_bDirty = true;
		if(bFlush)
			Flush();
	}
}

void CTextTable::Flush()
{
	if(m_bDirty == false)
		return;
	CFormatFile oFile;
	if(oFile.Open(CString("disk://").Append(m_sFileName).GetStr(), "wde"))
	{
		FocpLog(FOCP_LOG_ERROR, ("Open(%s) failure", m_sFileName));
		return;
	}

	m_bDirty = false;
	uint32 i, nColNum = m_oColInfo.GetSize();
	for(i=0; i<nColNum; ++i)
	{
		CString &oColName = m_oColInfo[i];
		if(i)
			oFile.Print("\t");
		oFile.Print("%s", oColName.GetStr());
	}
	oFile.Print("\n");
	uint32 j, nRecNum = m_oTable.GetSize();
	for(j=0; j<nRecNum; ++j)
	{
		CVector<FOCP_DETAIL_NAME::CFieldValue> &oRecord = m_oTable[j];
		for(i=0; i<nColNum; ++i)
		{
			FOCP_DETAIL_NAME::CFieldValue &oField = oRecord[i];
			if(i)
				oFile.Print("\t");
			CString oColValue;
			oField.EnCode(oColValue);
			oFile.Print("%s", oColValue.GetStr());
		}
		oFile.Print("\n");
	}
}

CTextAccess::CTextAccess(CTextTable* pTable)
{
	m_pTable = pTable;
	m_nQueryRowNo = 0xFFFFFFFF;
}

CTextAccess::~CTextAccess()
{
}

CTextTable& CTextAccess::GetTable()
{
	return *m_pTable;
}

void CTextAccess::OpenIdxVal()
{
	m_oCondTable.Clear();
	m_nQueryRowNo = 0xFFFFFFFF;
}

void CTextAccess::OpenColVal()
{
	m_oSetTable.Clear();
}

void CTextAccess::OpenAdditiveIdxVal()
{
	m_oAdditiveCond.Clear();
}

bool CTextAccess::SetIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive, uint32 nLength)
{
	uint32 nCol = m_pTable->GetCol(sIdxName);
	if(nCol == 0xFFFFFFFF)
		return false;
	CCondition& oCondition = m_oCondTable[nCol];
	oCondition.oValue.Set(sIdxVal, nLength);
	oCondition.bSensitive = bSensitive;
	return true;
}

bool CTextAccess::SetColVal(const char * sColName, const char * sColVal, uint32 nLength)
{
	uint32 nCol = m_pTable->GetCol(sColName);
	if(nCol == 0xFFFFFFFF)
		return false;
	m_oSetTable[nCol].Set(sColVal, nLength);
	return true;
}

bool CTextAccess::SetAdditiveIdxVal(const char * sIdxName, const char * sIdxVal, bool bSensitive, uint32 nLength)
{
	uint32 nCol = m_pTable->GetCol(sIdxName);
	if(nCol == 0xFFFFFFFF)
		return false;
	CCondition& oCondition = m_oAdditiveCond[nCol];
	oCondition.oValue.Set(sIdxVal, nLength);
	oCondition.bSensitive = bSensitive;
	return true;
}

bool CTextAccess::Query()//根据SetIdxVal
{
	while(true)
	{
		if(m_nQueryRowNo >= m_pTable->m_oTable.GetSize())
			m_nQueryRowNo = 0;
		else
			++m_nQueryRowNo;

		if(m_nQueryRowNo >= m_pTable->m_oTable.GetSize())
			return false;

		if(CompareRecord(m_nQueryRowNo))
			break;
	}
	return true;
}

const char* CTextAccess::GetVal(const char * sColName, uint32 &nLength)//根据Query或QueryNext,返回NULL表示空值
{
	return m_pTable->GetCell(m_nQueryRowNo, m_pTable->GetCol(sColName), nLength);
}

void CTextAccess::Insert(bool bFlush)//根据SetColVal
{
	uint32 nColNum = m_pTable->m_oColInfo.GetSize();
	uint32 nRecNum = m_pTable->m_oTable.GetSize();
	m_pTable->m_oTable.Insert(nRecNum);
	CVector<FOCP_DETAIL_NAME::CFieldValue> &oRecord = m_pTable->m_oTable[nRecNum];
	oRecord.SetSize(nColNum);
	CRbTreeNode* pEnd = m_oSetTable.End();
	CRbTreeNode* pFirst = m_oSetTable.First();
	for(; pFirst!=pEnd; pFirst=m_oSetTable.GetNext(pFirst))
	{
		FOCP_DETAIL_NAME::CFieldValue& oField = m_oSetTable.GetItem(pFirst);
		uint32 nCol = m_oSetTable.GetKey(pFirst);
		oRecord[nCol].Set(oField.sVal, oField.nSize);
	}
	pEnd = m_oAdditiveCond.End();
	pFirst = m_oAdditiveCond.First();
	for(; pFirst!=pEnd; pFirst=m_oAdditiveCond.GetNext(pFirst))
	{
		FOCP_DETAIL_NAME::CFieldValue& oField = m_oAdditiveCond.GetItem(pFirst).oValue;
		uint32 nCol = m_oAdditiveCond.GetKey(pFirst);
		oRecord[nCol].Set(oField.sVal, oField.nSize);
	}
	m_pTable->m_bDirty = true;
	if(bFlush)
		m_pTable->Flush();
}

void CTextAccess::Update(bool bFlush)//根据SetIdxVal和SetColVal
{
	if(!m_oSetTable.GetSize())
		return;
	m_nQueryRowNo = 0xFFFFFFFF;
	while(Query())
	{
		CVector<FOCP_DETAIL_NAME::CFieldValue>& oRecord = m_pTable->m_oTable[m_nQueryRowNo];
		CRbTreeNode* pEnd = m_oSetTable.End();
		CRbTreeNode* pFirst = m_oSetTable.First();
		for(; pFirst!=pEnd; pFirst=m_oSetTable.GetNext(pFirst))
		{
			FOCP_DETAIL_NAME::CFieldValue& oField = m_oSetTable.GetItem(pFirst);
			uint32 nCol = m_oSetTable.GetKey(pFirst);
			oRecord[nCol].Set(oField.sVal, oField.nSize);
		}
		m_pTable->m_bDirty = true;
	}
	if(bFlush)
		m_pTable->Flush();
}

void CTextAccess::Delete(bool bFlush)//根据SetIdxVal
{
	m_nQueryRowNo = 0xFFFFFFFF;
	while(Query())
	{
		m_pTable->m_oTable.Remove(m_nQueryRowNo);
		--m_nQueryRowNo;
		m_pTable->m_bDirty = true;
	}
	if(bFlush)
		m_pTable->Flush();
}

bool CTextAccess::CompareRecord(uint32 nRowNo)
{
	CVector<FOCP_DETAIL_NAME::CFieldValue>& oRecord = m_pTable->m_oTable[nRowNo];
	CRbTreeNode* pEnd = m_oCondTable.End();
	CRbTreeNode* pFirst = m_oCondTable.First();
	for(; pFirst!=pEnd; pFirst=m_oCondTable.GetNext(pFirst))
	{
		CCondition& oCondition = m_oCondTable.GetItem(pFirst);
		uint32 nCol = m_oCondTable.GetKey(pFirst);
		bool bEqual = oCondition.oValue.Equal(oRecord[nCol], oCondition.bSensitive);
		if(!bEqual)
			return false;
	}
	pEnd = m_oAdditiveCond.End();
	pFirst = m_oAdditiveCond.First();
	for(; pFirst!=pEnd; pFirst=m_oAdditiveCond.GetNext(pFirst))
	{
		CCondition& oCondition = m_oAdditiveCond.GetItem(pFirst);
		uint32 nCol = m_oAdditiveCond.GetKey(pFirst);
		bool bEqual = oCondition.oValue.Equal(oRecord[nCol], oCondition.bSensitive);
		if(!bEqual)
			return false;
	}

	return true;
}

CInitConfigSystem::CTableItem::CTableItem()
{
	pTable = NULL;
}

CInitConfigSystem::CTableItem::CTableItem(const CInitConfigSystem::CTableItem& oSrc)
{
	oTableName = oSrc.oTableName;
	nDeploy = oSrc.nDeploy;
	nSubsystemType = oSrc.nSubsystemType;
	nNetworkType = oSrc.nNetworkType;
	pTable = oSrc.pTable;
}

CInitConfigSystem::CTableItem& CInitConfigSystem::CTableItem::operator=(const CInitConfigSystem::CTableItem& oSrc)
{
	if(this != &oSrc)
	{
		oTableName = oSrc.oTableName;
		nDeploy = oSrc.nDeploy;
		nNetworkType = oSrc.nNetworkType;
		nSubsystemType = oSrc.nSubsystemType;
		pTable = oSrc.pTable;
	}
	return *this;
}

CInitConfigSystem::CTableItem::~CTableItem()
{
	if(pTable)
		delete pTable;
}

CInitConfigSystem::CInitConfigSystem()
{
}

CInitConfigSystem::~CInitConfigSystem()
{
}

CInitConfigSystem* CInitConfigSystem::GetInstance()
{
	return CSingleInstance<CInitConfigSystem>::GetInstance();
}

bool CInitConfigSystem::Initialize(const char* sNmsCfgHome, const char* sAppCfgHome, const char* sAppName)
{
	char sTmpPath[FOCP_MAX_PATH];
	CPathDetailInfo oDetailInfo;
	CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();

	if(sNmsCfgHome)
	{
		CString::StringCopy(m_sNmsAppCfgHome, sNmsCfgHome);
		CString::StringCatenate(m_sNmsAppCfgHome, "/");
		CString::StringCatenate(m_sNmsAppCfgHome, sAppName);

		CString::StringCopy(m_sNmsBaseCfgHome, sNmsCfgHome);
	}
	else
	{
		CString::StringCopy(m_sNmsAppCfgHome, sAppCfgHome);
		CString::StringCatenate(m_sNmsAppCfgHome, "/../../nms/cfg");
		CString::StringCatenate(m_sNmsAppCfgHome, sAppName);

		CString::StringCopy(m_sNmsBaseCfgHome, sAppCfgHome);
		CString::StringCatenate(m_sNmsBaseCfgHome, "/../../nms/cfg");
	}

	if(!pFileSystem->GetFullPath(m_sNmsAppCfgHome, sTmpPath, &oDetailInfo))
		return false;
	if(oDetailInfo.bExist && oDetailInfo.sFilePart)
		return false;
	CString::StringCopy(m_sNmsAppCfgHome, sTmpPath);

	if(!pFileSystem->GetFullPath(m_sNmsBaseCfgHome, sTmpPath, &oDetailInfo))
		return false;
	if(oDetailInfo.bExist && oDetailInfo.sFilePart)
		return false;
	CString::StringCopy(m_sNmsBaseCfgHome, sTmpPath);

	CString::StringCopy(m_sAppCfgHome, sAppCfgHome);
	if(!pFileSystem->GetFullPath(m_sAppCfgHome, sTmpPath, &oDetailInfo))
		return false;
	if(oDetailInfo.bExist && oDetailInfo.sFilePart)
		return false;
	CString::StringCopy(m_sAppCfgHome, sTmpPath);

	return true;
}

static bool CheckCode(uint32 nCode, uint32 nBits)
{
	if(!nBits)
	{
		if(nCode)
			return false;
		return true;
	}
	if(nCode > ((uint32)(-1) >> (32 - nBits)))
		return false;
	return true;
}

bool CInitConfigSystem::Load()
{
	uint32 nDMN, nCNN, nAIN=0;
	char sTmpPath[FOCP_MAX_PATH];

	CString::StringCopy(sTmpPath, m_sNmsBaseCfgHome);
	CString::StringCatenate(sTmpPath, "/");
	CString::StringCatenate(sTmpPath, "SubSystem.dat");
	if(!m_oSubsystemConfig.LoadTable(sTmpPath))
		return false;

	//检查系统子系统编号
	const char* sSysSsn[] = {"DMN", "CNN", "ATN", "AIN"};
	for(uint32 i=0; i<4; ++i)
	{
		if(!QuerySubsystem(m_oSystemSsn[i], i+1))
		{
			FocpLog(FOCP_LOG_ERROR, ("QuerySubsystem(%s) failure", sSysSsn[i]));
			return false;
		}
	}
	if(!m_oSystemSsn[1].nSubsystemBits)
	{
		FocpLog(FOCP_LOG_ERROR, ("CNN is 0 width"));
		return false;
	}
	if(m_oSystemSsn[0].nSubsystemBits + m_oSystemSsn[1].nSubsystemBits > 32)
	{
		FocpLog(FOCP_LOG_ERROR, ("DMN is conflict with CNN"));
		return false;
	}
	if(m_oSystemSsn[0].nSubsystemOffset <= m_oSystemSsn[1].nSubsystemOffset)
	{
		if(m_oSystemSsn[0].nSubsystemOffset + m_oSystemSsn[0].nSubsystemBits > m_oSystemSsn[1].nSubsystemOffset)
		{
			FocpLog(FOCP_LOG_ERROR, ("DMN is conflict with CNN"));
			return false;
		}
	}
	else
	{
		FocpLog(FOCP_LOG_ERROR, ("DMN is conflict with CNN"));
		return false;
	}
	if(!m_oSystemSsn[2].nSubsystemBits)
	{
		FocpLog(FOCP_LOG_ERROR, ("ATN is 0 width"));
		return false;
	}
	if(m_oSystemSsn[0].nSubsystemBits + m_oSystemSsn[2].nSubsystemBits + m_oSystemSsn[3].nSubsystemBits> 32)
	{
		FocpLog(FOCP_LOG_ERROR, ("DMN, ATN and AIN are conflict"));
		return false;
	}
	if(m_oSystemSsn[0].nSubsystemOffset <= m_oSystemSsn[2].nSubsystemOffset)
	{
		if(m_oSystemSsn[0].nSubsystemOffset + m_oSystemSsn[0].nSubsystemBits > m_oSystemSsn[2].nSubsystemOffset)
		{
			FocpLog(FOCP_LOG_ERROR, ("DMN is conflict with ATN"));
			return false;
		}
	}
	else
	{
		FocpLog(FOCP_LOG_ERROR, ("DMN is conflict with ATN"));
		return false;
	}
	if(m_oSystemSsn[2].nSubsystemOffset <= m_oSystemSsn[3].nSubsystemOffset)
	{
		if(m_oSystemSsn[2].nSubsystemOffset + m_oSystemSsn[2].nSubsystemBits > m_oSystemSsn[3].nSubsystemOffset)
		{
			FocpLog(FOCP_LOG_ERROR, ("ATN is conflict with AIN"));
			return false;
		}
	}
	else
	{
		FocpLog(FOCP_LOG_ERROR, ("ATN is conflict with AIN"));
		return false;
	}
	CServiceManager* pServiceManager = CServiceManager::GetInstance();
	uint32 nATN = pServiceManager->GetATN();
	if(nATN && !CheckCode(nATN, m_oSystemSsn[2].nSubsystemBits))
	{
		FocpLog(FOCP_LOG_ERROR, ("Invalid ATN config"));
		return false;
	}

	//获取网络部署情况
	uint32 nNetworkNo = 0;
	CString oHostIpList;
	if(!CFile::GetHostIpList(oHostIpList))
		return false;
	const char* sHostIpList = oHostIpList.GetStr();
	CString::StringCopy(sTmpPath, m_sNmsBaseCfgHome);
	CString::StringCatenate(sTmpPath, "/");
	CString::StringCatenate(sTmpPath, "NetDeploy.dat");
	CTextTable m_oNetworkConfig;
	if(!m_oNetworkConfig.LoadTable(sTmpPath))
		return false;
	CTextAccess m_oNetworkConfigAccess(&m_oNetworkConfig);
	m_oNetworkConfigAccess.OpenIdxVal();
	while(true)
	{
		uint32 nLen;
		if(!m_oNetworkConfigAccess.Query())
		{
			FocpLog(FOCP_LOG_ERROR, ("Query NetDeploy failure"));
			return false;
		}
		const char* sVal = m_oNetworkConfigAccess.GetVal("PhysicalIp", nLen);
		if(!sVal || sVal[nLen-1])
		{
			FocpLog(FOCP_LOG_ERROR, ("NetDeploy.PhysicalIp is invalid"));
			return false;
		}
		char* sIp = CString::StringOfString(sHostIpList, sVal);
		if(sIp == NULL || ';' != *(sIp-1) || sIp[nLen-1]!= ';')
			continue;
		sVal = m_oNetworkConfigAccess.GetVal("NetworkNo", nLen);
		if(!sVal || sVal[nLen-1])
		{
			FocpLog(FOCP_LOG_ERROR, ("NetDeploy.NetworkNo is invalid"));
			return false;
		}
		nNetworkNo = CString::Atoi(sVal);
		break;
	}
	nDMN = GetSubsystemNo(m_oSystemSsn[0], nNetworkNo);
	nCNN = GetSubsystemNo(m_oSystemSsn[1], nNetworkNo);

	//获取应用部署情况
	CString::StringCopy(sTmpPath, m_sNmsBaseCfgHome);
	CString::StringCatenate(sTmpPath, "/");
	CString::StringCatenate(sTmpPath, "AppDeploy.dat");
	CTextTable m_oAppDeploy;
	if(!m_oAppDeploy.LoadTable(sTmpPath))
		return false;
	CTextAccess m_oAppDeployAccess(&m_oAppDeploy);
	m_oAppDeployAccess.OpenIdxVal();
	CFormatString oIdx;
	oIdx.Print("%u", nNetworkNo);
	if(!m_oAppDeployAccess.SetIdxVal("NetworkNo", oIdx.GetStr()))
	{
		FocpLog(FOCP_LOG_ERROR, ("AppDeploy.NetworkNo isn't existed"));
		return false;
	}
	oIdx.Clear();
	oIdx.Print("%u", nATN);
	if(!m_oAppDeployAccess.SetIdxVal("AppTypeNo", oIdx.GetStr()))
	{
		FocpLog(FOCP_LOG_ERROR, ("AppDeploy.AppTypeNo isn't existed"));
		return false;
	}
	const char* s = GetEnvVar("FocpAppInst");
	if(s)
	{
		nAIN = CString::Atoi(s);
		if(nAIN && !CheckCode(nAIN, m_oSystemSsn[3].nSubsystemBits))
		{
			FocpLog(FOCP_LOG_ERROR, ("Invalid FocpAppInst config"));
			return false;
		}
		oIdx.Clear();
		oIdx.Print("%u", nAIN);
		if(!m_oAppDeployAccess.SetIdxVal("AppInstNo", oIdx.GetStr()))
		{
			FocpLog(FOCP_LOG_ERROR, ("AppDeploy.AppInstNo isn't existed"));
			return false;
		}
	}
	if(!m_oAppDeployAccess.Query())
	{
		FocpLog(FOCP_LOG_ERROR, ("Query AppDeploy failure"));
		return false;
	}
	uint32 nLen;
	const char* sVal = m_oAppDeployAccess.GetVal("AppInstNo", nLen);
	if(!sVal || sVal[nLen-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("Invalid AppDeploy.AppInstNo"));
		return false;
	}
	nAIN = CString::Atoi(sVal);
	if(!CheckCode(nAIN, m_oSystemSsn[3].nSubsystemBits))
	{
		FocpLog(FOCP_LOG_ERROR, ("Invalid AIN config"));
		return false;
	}
	pServiceManager->SetInstance(nDMN,nCNN, nAIN);
	SetLogInstance(nDMN,nAIN);
	m_nNetworkNo1 = nNetworkNo;
	m_nNetworkNo2 = (nDMN << (32-m_oSystemSsn[0].nSubsystemOffset-m_oSystemSsn[0].nSubsystemBits)) |
					(nATN << (32-m_oSystemSsn[2].nSubsystemOffset-m_oSystemSsn[2].nSubsystemBits)) |
					(nAIN << (32-m_oSystemSsn[3].nSubsystemOffset-m_oSystemSsn[3].nSubsystemBits));

	CString::StringCopy(sTmpPath, m_sAppCfgHome);
	CString::StringCatenate(sTmpPath, "/");
	CString::StringCatenate(sTmpPath, "ConfigDeploy.dat");
	if(!m_oConfigDeploy.LoadTable(sTmpPath))
		return false;

	//加载所有表
	CTableItem oItem;
	CTextAccess oConfigDeployAccess(&m_oConfigDeploy);
	while(WalkDeployTable(oConfigDeployAccess, oItem))
	{
		if(!LoadTable(oItem))
			return false;
		m_oTextTables[oItem.oTableName] = oItem;
		oItem.pTable = NULL;
	}
	return true;
}

void CInitConfigSystem::UnLoad()
{
	CTableItem oItem;
	CTextAccess oConfigDeployAccess(&m_oConfigDeploy);
	while(WalkDeployTable(oConfigDeployAccess, oItem))
	{
		if(oItem.nDeploy)
			m_oTextTables.Remove(oItem.oTableName);
	}
}

bool CInitConfigSystem::OpenConfig(CTextAccess& oAccess, const char* sTableName, bool bOptional)
{
	CRbTreeNode* pNode = m_oTextTables.Find(sTableName);
	if(pNode == m_oTextTables.End())
	{
		if(bOptional == false)
			FocpLog(FOCP_LOG_ERROR, ("Config table '%s' isn't existed", sTableName));
		return false;
	}
	CSubsystemItem oSubsysItem;
	CTableItem &oTableItem = m_oTextTables.GetItem(pNode);
	oAccess.m_pTable = oTableItem.pTable;
	oAccess.m_oAdditiveCond.Clear();
	if(oTableItem.nDeploy && oTableItem.nSubsystemType)
	{
		if(!QuerySubsystem(oSubsysItem, oTableItem.nSubsystemType))
		{
			FocpLog(FOCP_LOG_ERROR, ("QuerySubsystem(%s:%u) failure", sTableName, oTableItem.nSubsystemType));
			return false;
		}
		CFormatString oTmp;
		oTmp.Print("%u", oTableItem.nSubsystemType);
		oAccess.SetAdditiveIdxVal("SubsystemType", oTmp.GetStr());
		oTmp.Clear();
		oTmp.Print("%u", GetSubsystemNo(oSubsysItem, oSubsysItem.nNetworkType?m_nNetworkNo2:m_nNetworkNo1));
		oAccess.SetAdditiveIdxVal("SubsystemNo", oTmp.GetStr());
	}
	return true;
}

bool CInitConfigSystem::LoadTable(CTableItem &oItem)
{
	CFormatString oTableName;
	switch(oItem.nDeploy)
	{
	case 0:
		oTableName.Print("%s/%s.dat", m_sAppCfgHome, oItem.oTableName.GetStr());
		break;
	case 1:
		oTableName.Print("%s/%s.dat", m_sNmsAppCfgHome, oItem.oTableName.GetStr());
		break;
	case 2:
		oTableName.Print("%s/%s.dat", m_sNmsBaseCfgHome, oItem.oTableName.GetStr());
		break;
	}
	CTextTable* pTable = new CTextTable;
	if(!pTable->LoadTable(oTableName.GetStr()))
	{
		delete pTable;
		return false;
	}
	oItem.pTable = pTable;
	return true;
}

bool CInitConfigSystem::WalkDeployTable(CTextAccess& oAccess, CTableItem &oItem)
{
	uint32 nLength;
	const char* sVal;
	bool bRet = oAccess.Query();
	if(bRet == false)
		return false;

	sVal = oAccess.GetVal("TableName", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("Field 'TableName' is valid"));
		return false;
	}
	oItem.oTableName = sVal;

	sVal = oAccess.GetVal("Deploy", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("Field 'Deploy' is valid"));
		return false;
	}
	oItem.nDeploy = CString::Atoi(sVal);

	sVal = oAccess.GetVal("SubsystemType", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("Field 'SubsystemType' is valid"));
		return false;
	}
	oItem.nSubsystemType = CString::Atoi(sVal);

	return true;
}

bool CInitConfigSystem::QuerySubsystem(CSubsystemItem &oItem, uint32 nSubsystemType)
{
	CFormatString oIdx;
	uint32 nLength;
	const char* sVal;

	oIdx.Print("%u", nSubsystemType);

	CTextAccess oAccess(&m_oSubsystemConfig);
	oAccess.OpenIdxVal();
	if(!oAccess.SetIdxVal("SubsystemType", oIdx.GetStr()))
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.SubsystemType isn't existed"));
		return false;
	}

	if(!oAccess.Query())
		return false;

	sVal = oAccess.GetVal("SubsystemBits", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.SubsystemBits is invalid"));
		return false;
	}
	oItem.nSubsystemBits = CString::Atoi(sVal);

	sVal = oAccess.GetVal("SubsystemOffset", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.SubsystemOffset is invalid"));
		return false;
	}
	oItem.nSubsystemOffset = CString::Atoi(sVal);
	if(oItem.nSubsystemOffset >= 32)
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.SubsystemOffset is invalid"));
		return false;
	}
	if(oItem.nSubsystemBits > 32 || oItem.nSubsystemBits > 32 - oItem.nSubsystemOffset)
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.SubsystemBits is invalid"));
		return false;
	}

	sVal = oAccess.GetVal("NetworkType", nLength);
	if(!sVal || sVal[nLength-1])
	{
		FocpLog(FOCP_LOG_ERROR, ("SubsystemConfig.NetworkType is invalid"));
		return false;
	}
	oItem.nNetworkType = CString::Atoi(sVal);
	if(oItem.nNetworkType)
		oItem.nNetworkType = 1;

	sVal = oAccess.GetVal("SubsystemDesc", nLength);
	if(sVal && !sVal[nLength-1])
		oItem.oSubsystemDesc = sVal;

	return true;
}

uint32 CInitConfigSystem::GetSubsystemNo(CSubsystemItem &oItem, uint32 nNetworkNo)
{
	uint32 nSubsystemNo = nNetworkNo;
	if(oItem.nSubsystemOffset)
	{
		nSubsystemNo <<= oItem.nSubsystemOffset;
		nSubsystemNo >>= oItem.nSubsystemOffset;
	}
	uint32 nRightBits = 32 - (oItem.nSubsystemOffset + oItem.nSubsystemBits);
	if(nRightBits)
		nSubsystemNo >>= nRightBits;
	return nSubsystemNo;
}

FOCP_END();
