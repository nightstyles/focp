
#include "RdbUtility.hpp"

FOCP_BEGIN();

//-----------------------------------------------------
// CRdbNameTable
//-----------------------------------------------------
CRdbNameTable::CRdbNameTable():m_oHashTable(256)
{
}

CRdbNameTable::~CRdbNameTable()
{
}

uint32 CRdbNameTable::GetSize()
{
	return m_oHashTable.GetSize();
}

void CRdbNameTable::Clear()
{
	m_oHashTable.Clear();
}

uint32 CRdbNameTable::FindName(char* sName)
{
	CHashIterator oIt = m_oHashTable.Find(CString(sName));
	return m_oHashTable.IteratorEqual(oIt, m_oHashTable.End())?((uint32)(-1)):m_oHashTable.GetItem(oIt);
}

bool CRdbNameTable::InsertName(char* sName, uint32 nIndex)
{
	return m_oHashTable.IteratorEqual(m_oHashTable.Insert(CString(sName), nIndex), m_oHashTable.End())?false:true;
}

void CRdbNameTable::Erase(char* sName)
{
	m_oHashTable.Remove(CString(sName));
}

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
bool InvalidDbName(char* sName)
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

const char* GetLogName()
{
	return "Rdb";
}

uint32 CorrectFieldAttr(const char* sLogName, CRdbFieldDef* pFieldDefine)
{
	if(InvalidDbName(pFieldDefine->sFieldName))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CorrectFieldAttr(%s): RDB_INVALID_NAME", pFieldDefine->sFieldName));
		return RDB_INVALID_NAME;
	}
	switch(pFieldDefine->nType)
	{
	default:
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CorrectFieldAttr(%s): RDB_INVALID_TYPE", pFieldDefine->sFieldName));
		return RDB_INVALID_TYPE;
	case RDB_INT8_FIELD: case RDB_UINT8_FIELD:
		pFieldDefine->nRecSize = 0;
		pFieldDefine->nLen = 1;
		break;
	case RDB_INT16_FIELD: case RDB_UINT16_FIELD:
		pFieldDefine->nRecSize = 0;
		pFieldDefine->nLen = 2;
		break;
	case RDB_INT32_FIELD: case RDB_UINT32_FIELD: case RDB_FLOAT_FIELD:
		pFieldDefine->nRecSize = 0;
		pFieldDefine->nLen = 4;
		break;
	case RDB_INT64_FIELD: case RDB_UINT64_FIELD: case RDB_DOUBLE_FIELD:
		pFieldDefine->nRecSize = 0;
		pFieldDefine->nLen = 8;
		break;
	case RDB_CHAR_FIELD: case RDB_RAW_FIELD: case RDB_LCHAR_FIELD:
		pFieldDefine->nRecSize = 0;
		if(!pFieldDefine->nLen)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CorrectFieldAttr(%s): RDB_INVALID_SIZE", pFieldDefine->sFieldName));
			return RDB_INVALID_SIZE;
		}
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARRAW_FIELD: case RDB_VARLCHAR_FIELD:
		if(!pFieldDefine->nLen)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CorrectFieldAttr(%s): RDB_INVALID_SIZE", pFieldDefine->sFieldName));
			return RDB_INVALID_SIZE;
		}
		if(pFieldDefine->nRecSize > pFieldDefine->nLen)
			pFieldDefine->nRecSize = pFieldDefine->nLen;
		else if(!pFieldDefine->nRecSize)
			pFieldDefine->nRecSize = pFieldDefine->nLen/2;
		if(pFieldDefine->nRecSize+8 > RDB_RECORD_MAXSIZE)
			pFieldDefine->nRecSize = RDB_RECORD_MAXSIZE - 8;
		break;
	}
	return RDB_SUCCESS;
}

char* CloneString(const char* sLogName, char* sName)
{
	char* sDst = new char[CString::StringLength(sName)+1];
	if(sDst)
		CString::StringCopy(sDst, sName);
	else
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneString(%s): RDB_LACK_MEMORY", sName));
	return sDst;
}

bool CloneFieldDef(const char* sLogName, CRdbFieldDef* &pDstFieldDefines, CRdbFieldDef* pSrcFieldDefines, uint32 nFieldCount)
{
	uint32 i;
	pDstFieldDefines = NULL;
	if(nFieldCount)
	{
		pDstFieldDefines = new CRdbFieldDef[nFieldCount];
		if(!pDstFieldDefines)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneFieldDef(%s): RDB_LACK_MEMORY"));
			return false;
		}
		CBinary::MemorySet(pDstFieldDefines, 0, nFieldCount*sizeof(CRdbFieldDef));
		for(i=0; i<nFieldCount; ++i)
		{
			pDstFieldDefines[i] = pSrcFieldDefines[i];
			pDstFieldDefines[i].nJob = 1;
			pDstFieldDefines[i].sFieldName = CloneString(sLogName, pSrcFieldDefines[i].sFieldName);
			pDstFieldDefines[i].sDefault = NULL;
			char * sDefault = pSrcFieldDefines[i].sDefault;
			if(sDefault && sDefault[0])
				pDstFieldDefines[i].sDefault = CloneString(sLogName, sDefault);
			if(!pDstFieldDefines[i].sFieldName || 
				(!pDstFieldDefines[i].sDefault && sDefault))
			{
				FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneFieldDef(%s): RDB_LACK_MEMORY", pSrcFieldDefines[i].sFieldName));
				if(pDstFieldDefines[i].sFieldName)
					delete[] pDstFieldDefines[i].sFieldName;
				if(pDstFieldDefines[i].sDefault)
					delete[] pDstFieldDefines[i].sDefault;
				for(uint32 j=0; j<i; ++i)
					FreeFieldDefine(pDstFieldDefines[j]);
				delete[] pDstFieldDefines;
				pDstFieldDefines = NULL;
				return false;
			}
		}
		for(i=0; i<nFieldCount; ++i)
		{
			if(CorrectFieldAttr(sLogName, pDstFieldDefines+i))
			{
				FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneFieldDef(%s) Failure", pDstFieldDefines[i].sFieldName));
				for(i=0; i<nFieldCount; ++i)
					FreeFieldDefine(pDstFieldDefines[i]);
				delete[] pDstFieldDefines;
				pDstFieldDefines = NULL;
				return false;
			}
		}
	}
	return true;
}

void FreeFieldDefine(CRdbFieldDef& oFieldDef)
{
	if(oFieldDef.sFieldName)
		delete[] oFieldDef.sFieldName;
	if(oFieldDef.sDefault)
		delete[] oFieldDef.sDefault;
}

bool CloneTableDef(const char* sLogName, CRdbTableDef & oDstTableDef, CRdbTableDef & oSrcTableDef)
{
	oDstTableDef.sTableName = CloneString(sLogName, oSrcTableDef.sTableName);
	if(!oDstTableDef.sTableName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneTableDef(%s): RDB_LACK_MEMORY", oSrcTableDef.sTableName));
		return false;
	}
	if(!CloneFieldDef(sLogName, oDstTableDef.pFieldDefines, oSrcTableDef.pFieldDefines, oSrcTableDef.nFieldCount))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneTableDef(%s): RDB_LACK_MEMORY", oSrcTableDef.sTableName));
		delete[] oDstTableDef.sTableName;
		return false;
	}
	oDstTableDef.nFieldCount = oSrcTableDef.nFieldCount;
	oDstTableDef.nStorage = oSrcTableDef.nStorage;//default cache none.
	oDstTableDef.nMaxJob = 1;
	oDstTableDef.nTableNo = oSrcTableDef.nTableNo;
	oDstTableDef.nStorageAddr = 0;
	return true;
}

void FreeTableDefine(CRdbTableDef& oTableDef)
{
	if(oTableDef.sTableName)
		delete[] oTableDef.sTableName;
	if(oTableDef.pFieldDefines)
	{
		for(uint32 i=0; i<oTableDef.nFieldCount; ++i)
			FreeFieldDefine(oTableDef.pFieldDefines[i]);
		delete[] oTableDef.pFieldDefines;
	}
}

bool CloneIndexDef(const char* sLogName, CRdbIndexDef & oDstIndexDef, CRdbIndexDef & oSrcIndexDef)
{
	oDstIndexDef = oSrcIndexDef;
	oDstIndexDef.sIndexName = CloneString(sLogName, oSrcIndexDef.sIndexName);
	if(!oDstIndexDef.sIndexName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneIndexDef: RDB_LACK_MEMORY"));
		return false;
	}
	oDstIndexDef.sTableName = CloneString(sLogName, oSrcIndexDef.sTableName);
	if(!oDstIndexDef.sTableName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneIndexDef: RDB_LACK_MEMORY"));
		delete[] oDstIndexDef.sIndexName;
		return false;
	}
	if(oSrcIndexDef.sPrimaryIndex)
	{
		oDstIndexDef.sPrimaryIndex = CloneString(sLogName, oSrcIndexDef.sPrimaryIndex);
		if(!oDstIndexDef.sPrimaryIndex)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneIndexDef: RDB_LACK_MEMORY"));
			delete[] oDstIndexDef.sIndexName;
			delete[] oDstIndexDef.sTableName;
			return false;
		}
	}
	if(oSrcIndexDef.pFieldList)
	{
		oDstIndexDef.pFieldList = CloneString(sLogName, oSrcIndexDef.pFieldList);
		if(!oDstIndexDef.pFieldList)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CloneIndexDef: RDB_LACK_MEMORY"));
			delete[] oDstIndexDef.sIndexName;
			delete[] oDstIndexDef.sTableName;
			if(oDstIndexDef.sPrimaryIndex)
				delete[] oDstIndexDef.sPrimaryIndex;
			return false;
		}
	}
	oDstIndexDef.nIndexNo = oSrcIndexDef.nIndexNo;
	oDstIndexDef.nStorageAddr = 0;
	return true;
}

void FreeIndexDefine(CRdbIndexDef& oIndexDef)
{
	delete[] oIndexDef.sIndexName;
	delete[] oIndexDef.sTableName;
	if(oIndexDef.sPrimaryIndex)
		delete[] oIndexDef.sPrimaryIndex;
	if(oIndexDef.pFieldList)
		delete[] oIndexDef.pFieldList;
}

static bool PackFieldDefine(CMemoryStream & oStream, CRdbFieldDef& oDefine)
{
	uint32 nLen = CString::StringLength(oDefine.sFieldName);
	if(!oStream.Write(nLen))
		return false;
	if(!oStream.Write(oDefine.sFieldName, nLen+1))
		return false;
	if(!oDefine.sDefault)
		nLen = 0;
	else
		nLen = CString::StringLength(oDefine.sDefault);
	if(!oStream.Write(nLen))
		return false;
	if(nLen && !oStream.Write(oDefine.sDefault, nLen+1))
		return false;
	if(!oStream.Write(oDefine.nType))
		return false;
	if(!oStream.Write(oDefine.nLen))
		return false;
	if(!oStream.Write(oDefine.bNotNull))
		return false;
	if(!oStream.Write(oDefine.nRecSize))
		return false;
	return true;
}

static bool UnPackFieldDefine(CMemoryStream & oStream, CRdbFieldDef& oDefine)
{
	uint32 nLen;
	if(!oStream.Read(nLen))
		return false;
	oDefine.sFieldName = new char[nLen+1];
	if(!oStream.Read(oDefine.sFieldName, nLen+1))
		return false;
	if(!oStream.Read(nLen))
		return false;
	if(nLen)
	{
		oDefine.sDefault = new char[nLen+1];
		if(!oStream.Read(oDefine.sDefault, nLen+1))
			return false;
	}
	if(!oStream.Read(oDefine.nType))
		return false;
	if(!oStream.Read(oDefine.nLen))
		return false;
	if(!oStream.Read(oDefine.bNotNull))
		return false;
	if(!oStream.Read(oDefine.nRecSize))
		return false;
	return true;
}

bool PackTableDefine(CMemoryStream & oStream, CRdbTableDef& oDefine)
{
	uint32 nLen = CString::StringLength(oDefine.sTableName);
	if(!oStream.Write(nLen))
		return false;
	if(!oStream.Write(oDefine.sTableName, nLen+1))
		return false;
	if(!oStream.Write(oDefine.nFieldCount))
		return false;
	for(uint32 i=0; i<oDefine.nFieldCount; ++i)
	{
		if(!PackFieldDefine(oStream, oDefine.pFieldDefines[i]))
			return false;
	}
	if(!oStream.Write(oDefine.nStorage))
		return false;
	return true;
}

bool UnPackTableDefine(CMemoryStream & oStream, CRdbTableDef& oDefine)
{
	uint32 nLen;
	CBinary::MemorySet(&oDefine, 0, sizeof(oDefine));
	if(!oStream.Read(nLen))
		return false;
	oDefine.sTableName = new char[nLen+1];
	if(!oStream.Read(oDefine.sTableName, nLen+1))
		return false;
	if(!oStream.Read(oDefine.nFieldCount))
		return false;
	oDefine.pFieldDefines = new CRdbFieldDef[oDefine.nFieldCount];
	CBinary::MemorySet(oDefine.pFieldDefines, 0, oDefine.nFieldCount*sizeof(CRdbFieldDef));
	for(uint32 i=0; i<oDefine.nFieldCount; ++i)
	{
		if(!UnPackFieldDefine(oStream, oDefine.pFieldDefines[i]))
			return false;
	}
	if(!oStream.Read(oDefine.nStorage))
		return false;
	return true;
}

bool PackIndexDefine(CMemoryStream & oStream, CRdbIndexDef &oDefine)
{
	uint32 nLen;
	nLen = CString::StringLength(oDefine.sIndexName);
	if(!oStream.Write(nLen))
		return false;
	if(!oStream.Write(oDefine.sIndexName, nLen+1))
		return false;
	nLen = CString::StringLength(oDefine.sTableName);
	if(!oStream.Write(nLen))
		return false;
	if(!oStream.Write(oDefine.sTableName, nLen+1))
		return false;
	nLen = oDefine.sPrimaryIndex?CString::StringLength(oDefine.sPrimaryIndex):0;
	if(!oStream.Write(nLen))
		return false;
	if(nLen && !oStream.Write(oDefine.sPrimaryIndex, nLen+1))
		return false;
	nLen = CString::StringLength(oDefine.pFieldList);
	if(!oStream.Write(nLen))
		return false;
	if(!oStream.Write(oDefine.pFieldList, nLen+1))
		return false;
	if(!oStream.Write(oDefine.nQualifier))
		return false;
	if(!oStream.Write(oDefine.nArithmetic))
		return false;
	if(!oStream.Write(oDefine.nHashRate))
		return false;
	return true;
}

bool UnPackIndexDefine(CMemoryStream & oStream, CRdbIndexDef &oDefine)
{
	uint32 nLen;
	CBinary::MemorySet(&oDefine, 0, sizeof(oDefine));
	if(!oStream.Read(nLen))
		return false;
	oDefine.sIndexName = new char[nLen+1];
	if(!oStream.Read(oDefine.sIndexName, nLen+1))
		return false;
	if(!oStream.Read(nLen))
		return false;
	oDefine.sTableName = new char[nLen+1];
	if(!oStream.Read(oDefine.sTableName, nLen+1))
		return false;
	if(!oStream.Read(nLen))
		return false;
	if(nLen)
	{
		oDefine.sPrimaryIndex = new char[nLen+1];
		if(!oStream.Read(oDefine.sPrimaryIndex, nLen+1))
			return false;
	}
	if(!oStream.Read(nLen))
		return false;
	oDefine.pFieldList = new char[nLen+1];
	if(!oStream.Read(oDefine.pFieldList, nLen+1))
		return false;
	if(!oStream.Read(oDefine.nQualifier))
		return false;
	if(!oStream.Read(oDefine.nArithmetic))
		return false;
	if(!oStream.Read(oDefine.nHashRate))
		return false;
	return true;
}

CBaseTableDefine::CBaseTableDefine()
{
}

CBaseTableDefine::~CBaseTableDefine()
{
}

bool CBaseTableDefine::IsValidField(uint32 nFieldNo)
{
	if(nFieldNo >= m_nFieldCount)
		return false;
	CBaseFieldDefine* pField = m_pFieldDefineSet[nFieldNo];
	if(pField->m_pBaseAttr->nJob < 0)
		return false;
	return true;
}

uint32 CBaseTableDefine::GetFieldFlagCount()
{
	uint32 nSize = RDB_FLG_L(m_nFieldCount-1) + 1;
	if(nSize & 1)nSize += 1;
	return nSize;
}

FOCP_END();
