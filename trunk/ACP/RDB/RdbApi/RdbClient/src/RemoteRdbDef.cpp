
#include "RemoteRdbDef.hpp"

FOCP_BEGIN();
//-----------------------------------------------------
// CRemoteRdbNameTable
//-----------------------------------------------------
CRemoteRdbNameTable::CRemoteRdbNameTable():m_oHashTable(256)
{
}

CRemoteRdbNameTable::~CRemoteRdbNameTable()
{
}

uint32 CRemoteRdbNameTable::GetSize()
{
	return m_oHashTable.GetSize();
}

void CRemoteRdbNameTable::Clear()
{
	m_oHashTable.Clear();
}

uint32 CRemoteRdbNameTable::FindName(char* sName)
{
	CHashIterator oIt = m_oHashTable.Find(CString(sName));
	return m_oHashTable.IteratorEqual(oIt, m_oHashTable.End())?((uint32)(-1)):m_oHashTable.GetItem(oIt);
}

bool CRemoteRdbNameTable::InsertName(char* sName, uint32 nIndex)
{
	return m_oHashTable.IteratorEqual(m_oHashTable.Insert(CString(sName), nIndex), m_oHashTable.End())?false:true;
}

void CRemoteRdbNameTable::Erase(char* sName)
{
	m_oHashTable.Remove(CString(sName));
}

bool InvalidRemoteDbName(char* sName)
{
	bool bFirst = true;
	uint32 nLen = 0;
	while(sName[0])
	{
		if(bFirst)
		{
			bFirst = false;
			if(sName[0] != '_' && !CString::IsAlpha(sName[0]))
				return true;
		}
		else
		{
			if(sName[0] != '_' && !CString::IsAlnum(sName[0]))
				return true;
		}
		++sName;
		++nLen;
	}
	if(nLen > RDB_NAME_MAXLEN)
		return false;
	return false;
}

CRemoteFieldDefine::CRemoteFieldDefine(CRemoteTableDefine* pTableDefine)
{
	m_pBaseAttr = NULL;
	m_nOffset = 0;
	m_pTableDefine = pTableDefine;
}

CRemoteFieldDefine::~CRemoteFieldDefine()
{
	m_pBaseAttr = NULL;
	m_nOffset = 0;
	m_pTableDefine = NULL;
}

CRdbFieldDef * CRemoteFieldDefine::GetBaseAttr()
{
	return m_pBaseAttr;
}

uint32 CRemoteFieldDefine::GetFieldOffset()
{
	return m_nOffset;
}

uint32 CRemoteFieldDefine::GetSizeInRecord()
{
	return m_nSize;
}

uint32 CRemoteFieldDefine::GetFieldNo()
{
	return m_nFieldNo;
}

const char* CRemoteFieldDefine::GetLogName()
{
	return m_pTableDefine->GetLogName();
}

CRemoteTableDefine::CRemoteTableDefine(CRemoteDataBaseDefine* pDataBaseDefine)
{
	m_pDataBaseDefine = pDataBaseDefine;
	m_pBaseAttr = NULL;
	m_pFieldDefineSet = NULL;
	m_nRecordSize = 0;
	m_nFieldCount = 0;
	m_nIndexCount = 0;
	m_pIndexDefineSet = NULL;
	m_nVarFieldCount = 0;
	m_pVarFieldList = NULL;
}

FOCP_END();


