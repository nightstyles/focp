
#include "MdbDef.hpp"
#include "MdbError.hpp"

FOCP_BEGIN();

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
MCI_API bool InvalidDbName(char* sName)
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
	if(nLen > MDB_NAME_MAXLEN)
		return true;
	return false;
}

static char* CloneString(char* sName)
{
	char* sDst = new char[CString::StringLength(sName)+1];
	CString::StringCopy(sDst, sName);
	return sDst;
}

//-----------------------------------------------------
// CMdbFieldDefine
//-----------------------------------------------------
CMdbFieldDefine::CMdbFieldDefine()
{
	sFieldName = NULL;
	sDefault = NULL;
	pExtendAttr = NULL;
}

CMdbFieldDefine::~CMdbFieldDefine()
{
	if(sFieldName)
	{
		delete[] sFieldName;
		sFieldName = NULL;
	}
	if(sDefault)
	{
		delete[] sDefault;
		sDefault = NULL;
	}
	if(pExtendAttr)
	{
		for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
		{
			if(pExtendAttr->pExtAttr[i])
			{
				CMdbExtPlugIn* pPlug = CMdbExtPlugIn::GetPlugIn(i);
				if(pPlug)
					pPlug->OnFree(pExtendAttr->pExtAttr[i], 0);
			}
		}
		delete pExtendAttr;
		pExtendAttr = NULL;
	}
}

void CMdbFieldDefine::Initialize(CMdbFieldDef* pFieldDef)
{
	sFieldName = CloneString(pFieldDef->sFieldName);
	if(pFieldDef->sDefault && pFieldDef->sDefault[0])
		sDefault = CloneString(pFieldDef->sDefault);
	nType = pFieldDef->nType;
	nLen =  pFieldDef->nLen;
	bNotNull =  pFieldDef->bNotNull;
	pExtendAttr = new CMdbFieldAttr;
	pExtendAttr->nOffset = 0;
	pExtendAttr->nSize = 0;
	for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
		pExtendAttr->pExtAttr[i] = NULL;
}

bool CMdbFieldDefine::IsSame(CMdbFieldDef* pFieldDef)
{
	if(CString::StringCompare(sFieldName, pFieldDef->sFieldName, false))
		return false;
	if(nType != pFieldDef->nType)
		return false;
	if(nLen != pFieldDef->nLen)
		return false;
	if(bNotNull != pFieldDef->bNotNull)
		return false;
	return true;
}

uint32 CMdbFieldDefine::Invalid()
{
	if(InvalidDbName(sFieldName))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbFieldDefine::Invalid(%s): MDB_INVALID_NAME", sFieldName));
		return MDB_INVALID_NAME;
	}
	switch(nType)
	{
	default:
		FocpLog(FOCP_LOG_ERROR, ("CMdbFieldDefine::Invalid(%s): MDB_INVALID_TYPE", sFieldName));
		return MDB_INVALID_TYPE;
	case MDB_INT8_FIELD:
	case MDB_UINT8_FIELD:
		nLen = 1;
		break;
	case MDB_INT16_FIELD:
	case MDB_UINT16_FIELD:
		nLen = 2;
		break;
	case MDB_INT32_FIELD:
	case MDB_UINT32_FIELD:
	case MDB_FLOAT_FIELD:
	case MDB_DATE_FIELD:
	case MDB_TIME_FIELD:
		nLen = 4;
		break;
	case MDB_INT64_FIELD:
	case MDB_UINT64_FIELD:
	case MDB_DOUBLE_FIELD:
	case MDB_DATETIME_FIELD:
		nLen = 8;
		break;
	case MDB_CHAR_FIELD:
	case MDB_RAW_FIELD:
	case MDB_LCHAR_FIELD:
	case MDB_VARCHAR_FIELD:
	case MDB_VARRAW_FIELD:
	case MDB_VARLCHAR_FIELD:
		if(!nLen)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbFieldDefine::Invalid(%s): MDB_INVALID_SIZE", sFieldName));
			return MDB_INVALID_SIZE;
		}
		break;
	}
	return MDB_SUCCESS;
}

static bool WriteString(CMemoryStream & oStream, const char* sStr, bool bForce)
{
	uint32 bExist = (sStr!=NULL);
	if(bForce)
	{
		if(!bExist)
			return false;
	}
	else if(!oStream.Write(bExist))
		return false;
	bool bRet = true;
	if(bExist)
	{
		uint32 nSize = CString::StringLength(sStr);
		bRet = oStream.Write(nSize);
		if(bRet && nSize)
			bRet = (nSize == oStream.Write((void*)sStr, nSize));
	}
	return bRet;
}

static bool ReadString(CMemoryStream & oStream, char* &sStr, bool bForce)
{
	uint32 nSize, bExist = 1;
	sStr = NULL;
	if(!bForce)
	{
		if(!oStream.Read(bExist))
			return false;
	}
	bool bRet = true;
	if(bExist)
	{
		bRet = oStream.Read(nSize);
		if(bRet && nSize)
		{
			sStr = new char[nSize+1];
			sStr[nSize] = 0;
			bRet = (nSize == oStream.Read(sStr, nSize));
		}
	}
	return bRet;
}

bool CMdbFieldDefine::Write(CMemoryStream & oStream)
{
	bool bRet = WriteString(oStream, sFieldName, true);
	if(bRet)
		bRet = WriteString(oStream, sDefault, false);
	if(bRet)
		bRet = oStream.Write(nType);
	if(bRet)
		bRet = oStream.Write(nLen);
	if(bRet)
		bRet = oStream.Write(bNotNull);
	return bRet;
}

bool CMdbFieldDefine::Read(CMemoryStream & oStream)
{
	bool bRet = ReadString(oStream, sFieldName, true);
	if(bRet)
		bRet = ReadString(oStream, sDefault, false);
	if(bRet)
		bRet = oStream.Read(nType);
	if(bRet)
		bRet = oStream.Read(nLen);
	if(bRet)
		bRet = oStream.Read(bNotNull);
	if(bRet)
	{
		pExtendAttr = new CMdbFieldAttr;
		pExtendAttr->nOffset = 0;
		pExtendAttr->nSize = 0;
		for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
			pExtendAttr->pExtAttr[i] = NULL;
	}
	return bRet;
}

//-----------------------------------------------------
// CMdbTableDefine
//-----------------------------------------------------
CMdbTableDefine::CMdbTableDefine()
{
	sTableName = NULL;
	pFieldDefines = NULL;
	nFieldCount = 0;
	nMaxRecordNum = 0xFFFFFFFF;
	pExtendAttr = NULL;
}

CMdbTableDefine::~CMdbTableDefine()
{
	if(sTableName)
	{
		delete[] sTableName;
		sTableName = NULL;
	}
	if(pFieldDefines)
	{
		delete[] (CMdbFieldDefine*)pFieldDefines;
		pFieldDefines = NULL;
	}
	nFieldCount = 0;
	if(pExtendAttr)
	{
		if(pExtendAttr->pVarFieldList)
			CMalloc::Free(pExtendAttr->pVarFieldList);
		if(pExtendAttr->pIndexDefineSet)
			CMalloc::Free(pExtendAttr->pIndexDefineSet);
		for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
		{
			if(pExtendAttr->pExtAttr[i])
			{
				CMdbExtPlugIn* pPlug = CMdbExtPlugIn::GetPlugIn(i);
				if(pPlug)
					pPlug->OnFree(pExtendAttr->pExtAttr[i], 1);
			}
		}
		delete pExtendAttr;
		pExtendAttr = NULL;
	}
}

uint32 CMdbTableDefine::GetFieldNo(const char* sFieldName)
{
	for(uint32 i=0; i<nFieldCount; ++i)
	{
		if(!CString::StringCompare(pFieldDefines[i].sFieldName, sFieldName, false))
			return i;
	}
	return (uint32)(-1);
}

uint32 CMdbTableDefine::Initialize(CMdbDefine* pDbDef, CMdbTableDef * pTabDef)
{
	uint32 i;
	const char* sDbName = pDbDef->GetDbName();

	sTableName = CloneString(pTabDef->sTableName);
	pFieldDefines = new CMdbFieldDefine[pTabDef->nFieldCount];
	for(i=0; i<pTabDef->nFieldCount; ++i)
		((CMdbFieldDefine&)pFieldDefines[i]).Initialize(pTabDef->pFieldDefines+i);
	nFieldCount = pTabDef->nFieldCount;
	nMaxRecordNum = pTabDef->nMaxRecordNum;
	pExtendAttr = new CMdbTableAttr;
	pExtendAttr->nRecordSize = 0;
	pExtendAttr->nVarFieldCount = 0;
	pExtendAttr->pVarFieldList = NULL;
	pExtendAttr->nIndexCount = 0;
	pExtendAttr->pIndexDefineSet = NULL;
	pExtendAttr->pPrimeIndex = NULL;
	pExtendAttr->pForeignIndex = NULL;
	pExtendAttr->pInstance = NULL;
	for(i=0; i<MDB_ATTR_EXTSIZE; ++i)
		pExtendAttr->pExtAttr[i] = NULL;

	if(nFieldCount > MDB_MAX_FIELD_NUM)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbTableDefine::Initialize(%s.%s): MDB_FIELD_TOOMANY", sDbName, sTableName));
		return MDB_FIELD_TOOMANY;
	}

	pExtendAttr->nRecordSize = MDB_FLG_L(nFieldCount-1) + 1;
	if(pExtendAttr->nRecordSize&1)
		pExtendAttr->nRecordSize+=1;
	pExtendAttr->nRecordSize <<= 2;

	for(i=0; i<nFieldCount; ++i)
	{
		uint32 nRet = ((CMdbFieldDefine&)pFieldDefines[i]).Invalid();
		if(!nRet)
			nRet = AddFieldDefine(sDbName, pFieldDefines + i, i);
		if(nRet)
			return nRet;
	}

	return MDB_SUCCESS;
}

uint32 CMdbTableDefine::AddIndex(const char* sDbName, CMdbIndexDef* pIndexDefine)
{
	if(pExtendAttr->nIndexCount >= MDB_MAX_INDEX_NUM)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbTableDefine::AddIndex(%s.%s on %s): MDB_INDEX_TOOMANY", sDbName, pIndexDefine->sIndexName, sTableName));
		return MDB_INDEX_TOOMANY;
	}
	for(uint32 i=0; i<pExtendAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIdxDef = pExtendAttr->pIndexDefineSet[i];
		if(pIdxDef->pExtendAttr->nFieldCount == pIndexDefine->pExtendAttr->nFieldCount)
		{
			uint32 j;
			for(j=0; j<pIdxDef->pExtendAttr->nFieldCount; ++j)
			{
				if(pIdxDef->pExtendAttr->pFields[j] != pIndexDefine->pExtendAttr->pFields[j])
					break;
			}
			if(j >= pIdxDef->pExtendAttr->nFieldCount)
			{
				FocpLog(FOCP_LOG_ERROR, ("CMdbTableDefine::AddIndex(%s.%s on %s): MDB_INDEX_EXIST", sDbName, pIndexDefine->sIndexName, sTableName));
				return MDB_INDEX_EXIST;
			}
		}
	}
	pExtendAttr->pIndexDefineSet = (CMdbIndexDef**)CMalloc::Realloc(pExtendAttr->pIndexDefineSet, (pExtendAttr->nIndexCount+1)*sizeof(void*));
	pExtendAttr->pIndexDefineSet[pExtendAttr->nIndexCount] = pIndexDefine;
	++pExtendAttr->nIndexCount;
	return MDB_SUCCESS;
}

bool CMdbTableDefine::IsIndexField(uint32 nFieldNo)
{
	for(uint32 i=0; i<pExtendAttr->nIndexCount; ++i)
	{
		CMdbIndexDef* pIndex = pExtendAttr->pIndexDefineSet[i];
		uint32 nFieldCount = pIndex->pExtendAttr->nFieldCount;
		uint32* pFields = pIndex->pExtendAttr->pFields;
		for(uint32 j=0; j<nFieldCount; ++j)
		{
			if(nFieldNo == pFields[j])
				return true;
		}
	}
	return false;
}

uint32 CMdbTableDefine::GetIndexFieldNo(CMdbIndexDef* pIdxDef, uint32 nFieldNo)
{
	uint32 nFieldCount = pIdxDef->pExtendAttr->nFieldCount;
	uint32* pFields = pIdxDef->pExtendAttr->pFields;
	for(uint32 j=0; j<nFieldCount; ++j)
	{
		if(nFieldNo == pFields[j])
			return j;
	}
	return (uint32)(-1);
}

bool CMdbTableDefine::CheckRepeateField(CMdbFieldDef* pFieldDefine)
{
	uint32 i = nFieldCount - 1;
	while(i!=0xFFFFFFFF)
	{
		CMdbFieldDef* pField2 = pFieldDefines + i;
		if(pFieldDefine == pField2)
			break;
		if(!CString::StringCompare(pField2->sFieldName, pFieldDefine->sFieldName, false))
			return true;
		--i;
	}
	return false;
}

uint32 CMdbTableDefine::AddFieldDefine(const char* sDbName, CMdbFieldDef* pDefine, uint32 nFieldNo)
{
	if(CheckRepeateField(pDefine))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbTableDefine::AddFieldDefine(%s.%s.%s): MDB_REPEAT_FIELD", sDbName, sTableName, pDefine->sFieldName));
		return MDB_REPEAT_FIELD;
	}
	pDefine->pExtendAttr->nOffset = ComputeFieldOffset(pExtendAttr->nRecordSize, pDefine);
	pDefine->pExtendAttr->nSize = ComputeFieldRecSize(pDefine);
	pExtendAttr->nRecordSize = pDefine->pExtendAttr->nOffset + pDefine->pExtendAttr->nSize;
	switch(pDefine->nType)
	{
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
	case MDB_VARRAW_FIELD:
		pExtendAttr->pVarFieldList = (uint32*)CMalloc::Realloc(pExtendAttr->pVarFieldList, (pExtendAttr->nVarFieldCount+1)*sizeof(uint32));
		pExtendAttr->pVarFieldList[pExtendAttr->nVarFieldCount] = nFieldNo;
		++pExtendAttr->nVarFieldCount;
	}
	return MDB_SUCCESS;
}

uint32 CMdbTableDefine::ComputeFieldRecSize(CMdbFieldDef* pFieldDefine)
{
	uint32 nSize = pFieldDefine->nLen;
	switch(pFieldDefine->nType)
	{
	case MDB_RAW_FIELD:
		nSize += sizeof(uint32);
		break;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
	case MDB_VARRAW_FIELD:
		nSize = sizeof(void*);
		break;
	}
	return nSize;
}

uint32 CMdbTableDefine::ComputeFieldOffset(uint32 nOffset, CMdbFieldDef* pFieldDefine)
{
	uint32 nModSize = pFieldDefine->nLen;
	switch(pFieldDefine->nType)
	{
	case MDB_CHAR_FIELD:
	case MDB_LCHAR_FIELD:
		return nOffset;
	case MDB_RAW_FIELD:
		nModSize = sizeof(uint32);
		break;
	case MDB_VARCHAR_FIELD:
	case MDB_VARLCHAR_FIELD:
	case MDB_VARRAW_FIELD:
		nModSize = sizeof(void*);
		break;
	}
	return MakeAlign(nOffset, nModSize);
}

bool CMdbTableDefine::Write(CMemoryStream & oStream)
{
	bool bRet = WriteString(oStream, sTableName, true);
	if(bRet)
		bRet = oStream.Write(nMaxRecordNum);
	if(bRet)
		bRet = oStream.Write(nFieldCount);
	if(bRet && nFieldCount)
	{
		for(uint32 i=0; bRet && i<nFieldCount; ++i)
			bRet = ((CMdbFieldDefine&)pFieldDefines[i]).Write(oStream);
	}

	return bRet;
}

bool CMdbTableDefine::Read(CMdbDefine* pDbDef, CMemoryStream & oStream)
{
	const char* sDbName = pDbDef->GetDbName();
	bool bRet = ReadString(oStream, sTableName, true);
	if(bRet)
		bRet = oStream.Read(nMaxRecordNum);
	if(bRet)
		bRet = oStream.Read(nFieldCount);
	if(bRet && nFieldCount)
	{
		uint32 i;
		pFieldDefines = new CMdbFieldDefine[nFieldCount];
		for(i=0; bRet && i<nFieldCount; ++i)
			bRet = ((CMdbFieldDefine&)pFieldDefines[i]).Read(oStream);
		if(bRet)
		{
			pExtendAttr = new CMdbTableAttr;
			pExtendAttr->nRecordSize = 0;
			pExtendAttr->nVarFieldCount = 0;
			pExtendAttr->pVarFieldList = NULL;
			pExtendAttr->nIndexCount = 0;
			pExtendAttr->pIndexDefineSet = NULL;
			pExtendAttr->pPrimeIndex = NULL;
			pExtendAttr->pForeignIndex = NULL;
			pExtendAttr->pInstance = NULL;
			for(i=0; i<MDB_ATTR_EXTSIZE; ++i)
				pExtendAttr->pExtAttr[i] = NULL;

			pExtendAttr->nRecordSize = MDB_FLG_L(nFieldCount-1) + 1;
			if(pExtendAttr->nRecordSize&1)
				pExtendAttr->nRecordSize+=1;
			pExtendAttr->nRecordSize <<= 2;

			for(i=0; i<nFieldCount; ++i)
			{
				((CMdbFieldDefine&)pFieldDefines[i]).Invalid();
				AddFieldDefine(sDbName, pFieldDefines + i, i);
			}
		}
	}

	return bRet;
}

//-----------------------------------------------------
// CMdbIndexDefine
//-----------------------------------------------------
CMdbIndexDefine::CMdbIndexDefine()
{
	sIndexName = NULL;
	sTableName = NULL;
	sPrimaryIndex = NULL;
	pFieldList = NULL;
	pExtendAttr = NULL;
}

CMdbIndexDefine::~CMdbIndexDefine()
{
	if(pExtendAttr)
	{
		for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
		{
			if(pExtendAttr->pExtAttr[i])
			{
				CMdbExtPlugIn* pPlug = CMdbExtPlugIn::GetPlugIn(i);
				if(pPlug)
					pPlug->OnFree(pExtendAttr->pExtAttr[i], 2);
			}
		}
		if(pExtendAttr->pFields)
			CMalloc::Free(pExtendAttr->pFields);
		if(pExtendAttr->pDetailIndexs)
			CMalloc::Free(pExtendAttr->pDetailIndexs);
		delete pExtendAttr;
		pExtendAttr = NULL;
	}
	if(sIndexName)
	{
		delete[] sIndexName;
		sIndexName = NULL;
	}
	if(sTableName)
	{
		delete[] sTableName;
		sTableName = NULL;
	}
	if(sPrimaryIndex)
	{
		delete[] sPrimaryIndex;
		sPrimaryIndex = NULL;
	}
	if(pFieldList)
	{
		delete[] pFieldList;
		pFieldList = NULL;
	}
}

uint32 CMdbIndexDefine::Initialize(CMdbDefine* pDbDef, CMdbIndexDef* pIndexDef)
{
	const char* sDbName = pDbDef->GetDbName();
	sIndexName = CloneString(pIndexDef->sIndexName);
	sTableName = CloneString(pIndexDef->sTableName);
	if(pIndexDef->sPrimaryIndex)
		sPrimaryIndex = CloneString(pIndexDef->sPrimaryIndex);
	pFieldList = CloneString(pIndexDef->pFieldList);
	nQualifier = pIndexDef->nQualifier?1:0;
	nArithmetic = pIndexDef->nArithmetic;
	nHashRate = pIndexDef->nHashRate;
	switch(nArithmetic)
	{
	case MDB_RBTREE_INDEX:
	case MDB_HASH_INDEX: //case MDB_NTREE_INDEX:
		break;
	default:
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s): MDB_INVALID_INDEX_ARITHMETIC", sDbName, sIndexName));
		return MDB_INVALID_INDEX_ARITHMETIC;
	}
	pExtendAttr = new CMdbIndexAttr;
	pExtendAttr->pPrimaryIndex = NULL;
	pExtendAttr->nDetailIndexCount = 0;
	pExtendAttr->pDetailIndexs = NULL;
	pExtendAttr->nFieldCount = 0;
	pExtendAttr->pFields = NULL;
	pExtendAttr->pInstance = NULL;
	for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
		pExtendAttr->pExtAttr[i] = NULL;

	pExtendAttr->pTableDefine = pDbDef->GetTableDefine(sTableName);
	if(!pExtendAttr->pTableDefine)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_TABLE_NOTEXIST", sDbName, sIndexName, sTableName));
		return MDB_TABLE_NOTEXIST;
	}
	if(sPrimaryIndex)
	{
		pExtendAttr->pPrimaryIndex = pDbDef->GetIndexDefine(sPrimaryIndex);
		if(!pExtendAttr->pPrimaryIndex)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_INDEX_NOTEXIST", sDbName, sIndexName, sPrimaryIndex));
			return MDB_INDEX_NOTEXIST;
		}
		if(pExtendAttr->pPrimaryIndex->nQualifier != MDB_UNIQUE_INDEX)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, sIndexName, sPrimaryIndex));
			return MDB_INVALID_FOREIGN_INDEX;
		}
		if(pExtendAttr->pTableDefine == pExtendAttr->pPrimaryIndex->pExtendAttr->pTableDefine)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, sIndexName, sPrimaryIndex));
			return MDB_INVALID_FOREIGN_INDEX;
		}
		if(!pExtendAttr->pTableDefine->pExtendAttr->pForeignIndex)
			pExtendAttr->pTableDefine->pExtendAttr->pForeignIndex = pExtendAttr->pPrimaryIndex;
		else if(pExtendAttr->pTableDefine->pExtendAttr->pForeignIndex != pExtendAttr->pPrimaryIndex)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, sIndexName, sPrimaryIndex));
			return MDB_INVALID_FOREIGN_INDEX;
		}
		if(!pExtendAttr->pPrimaryIndex->pExtendAttr->pTableDefine->pExtendAttr->pPrimeIndex)
			pExtendAttr->pPrimaryIndex->pExtendAttr->pTableDefine->pExtendAttr->pPrimeIndex = pExtendAttr->pPrimaryIndex;
		else if(pExtendAttr->pPrimaryIndex != pExtendAttr->pPrimaryIndex->pExtendAttr->pTableDefine->pExtendAttr->pPrimeIndex)
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, sIndexName, sPrimaryIndex));
			return MDB_INVALID_FOREIGN_INDEX;
		}
	}
	char*pShift, *pField = pFieldList;
	while(pField)
	{
		pShift = (char*)CString::CharOfString(pField, ',');
		if(pShift)
			pShift[0] = 0;
		uint32 nFieldNo = ((CMdbTableDefine*)pExtendAttr->pTableDefine)->GetFieldNo(pField);
		if(nFieldNo == (uint32)(-1))
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s): MDB_INVALID_INDEXFIELD(%s)", sDbName, sIndexName, pField));
			if(pShift)
				pShift[0] = ',';
			return MDB_INVALID_INDEXFIELD;
		}
		/*
				CMdbFieldDef* pFieldDef = pExtendAttr->pTableDefine->pFieldDefines + nFieldNo;
				if(pFieldDef->nType == MDB_VARCHAR_FIELD ||
					pFieldDef->nType == MDB_VARRAW_FIELD ||
					pFieldDef->nType == MDB_VARLCHAR_FIELD)
				{
					FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s): MDB_INVALID_INDEXFIELD(%s)", sDbName, sIndexName, pField));
					if(pShift)
						pShift[0] = ',';
					return MDB_INVALID_INDEXFIELD;
				}
		*/
		/*
				if(nArithmetic == MDB_NTREE_INDEX)
				{
					if(pFieldDef->nType != MDB_CHAR_FIELD)
					{
						FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s): MDB_INVALID_INDEXFIELD(%s)", sDbName, sIndexName, pField));
						if(pShift)
							pShift[0] = ',';
						return MDB_INVALID_INDEXFIELD;
					}
				}
		*/
		pExtendAttr->pFields = (uint32*)CMalloc::Realloc(pExtendAttr->pFields, (pExtendAttr->nFieldCount+1)<<2);
		pExtendAttr->pFields[pExtendAttr->nFieldCount] = nFieldNo;
		++pExtendAttr->nFieldCount;
		pField = pShift;
		if(pField)
		{
			pField[0] = ',';
			++pField;
		}
		if(pExtendAttr->nFieldCount >= MDB_MAX_INDEX_FIELD_NUM)// || nArithmetic == MDB_NTREE_INDEX)
			break;
	}
	if(pField)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::Initialize(%s.%s): MDB_INDEXFIELD_TOOMANY(%s)", sDbName, sIndexName, pField));
		return MDB_INDEXFIELD_TOOMANY;
	}
	uint32 nRet = ((CMdbTableDefine*)pExtendAttr->pTableDefine)->AddIndex(sDbName, this);
	if(!nRet && pExtendAttr->pPrimaryIndex)
		nRet =	((CMdbIndexDefine*)pExtendAttr->pPrimaryIndex)->AddDetail(sDbName, this);
	return nRet;
}

uint32 CMdbIndexDefine::AddDetail(const char* sDbName, CMdbIndexDefine * pIndexDefine)
{
	uint32 i=0;
	if(pExtendAttr->nFieldCount < pIndexDefine->pExtendAttr->nFieldCount)
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::AddDetail(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, pIndexDefine->sIndexName, sIndexName));
		return MDB_INVALID_FOREIGN_INDEX;
	}
	for(i=0; i<pExtendAttr->nFieldCount; ++i)
	{
		CMdbFieldDef* pPrimFieldDef = pExtendAttr->pTableDefine->pFieldDefines + pExtendAttr->pFields[i];
		CMdbFieldDef* pDetailFieldDef = pIndexDefine->pExtendAttr->pTableDefine->pFieldDefines + pIndexDefine->pExtendAttr->pFields[i];
		if(!((CMdbFieldDefine*)pPrimFieldDef)->IsSame(pDetailFieldDef))
		{
			FocpLog(FOCP_LOG_ERROR, ("CMdbIndexDefine::AddDetail(%s.%s on %s): MDB_INVALID_FOREIGN_INDEX", sDbName, pIndexDefine->sIndexName, sIndexName));
			return MDB_INVALID_FOREIGN_INDEX;
		}
	}

	pExtendAttr->pDetailIndexs = (CMdbIndexDef**)CMalloc::Realloc(pExtendAttr->pDetailIndexs, (pExtendAttr->nDetailIndexCount+1)*sizeof(void*));
	pExtendAttr->pDetailIndexs[pExtendAttr->nDetailIndexCount] = pIndexDefine;
	++pExtendAttr->nDetailIndexCount;

	return MDB_SUCCESS;
}

bool CMdbIndexDefine::Write(CMemoryStream & oStream)
{
	bool bRet = WriteString(oStream, sIndexName, true);
	if(bRet)
		bRet = WriteString(oStream, sTableName, true);
	if(bRet)
		bRet = WriteString(oStream, sPrimaryIndex, false);
	if(bRet)
		bRet = WriteString(oStream, pFieldList, true);
	if(bRet)
		bRet = oStream.Write(nQualifier);
	if(bRet)
		bRet = oStream.Write(nArithmetic);
	if(bRet)
		bRet = oStream.Write(nHashRate);

	return bRet;
}

bool CMdbIndexDefine::Read(CMdbDefine* pDbDef, CMemoryStream & oStream)
{
	const char* sDbName = pDbDef->GetDbName();
	bool bRet = ReadString(oStream, sIndexName, true);
	if(bRet)
		bRet = ReadString(oStream, sTableName, true);
	if(bRet)
		bRet = ReadString(oStream, sPrimaryIndex, false);
	if(bRet)
		bRet = ReadString(oStream, pFieldList, true);
	if(bRet)
		bRet = oStream.Read(nQualifier);
	if(bRet)
		bRet = oStream.Read(nArithmetic);
	if(bRet)
		bRet = oStream.Read(nHashRate);

	if(bRet)
	{
		pExtendAttr = new CMdbIndexAttr;
		pExtendAttr->pPrimaryIndex = NULL;
		pExtendAttr->nDetailIndexCount = 0;
		pExtendAttr->pDetailIndexs = NULL;
		pExtendAttr->nFieldCount = 0;
		pExtendAttr->pFields = NULL;
		pExtendAttr->pInstance = NULL;
		for(uint32 i=0; i<MDB_ATTR_EXTSIZE; ++i)
			pExtendAttr->pExtAttr[i] = NULL;

		pExtendAttr->pTableDefine = pDbDef->GetTableDefine(sTableName);
		if(sPrimaryIndex)
		{
			pExtendAttr->pPrimaryIndex = pDbDef->GetIndexDefine(sPrimaryIndex);
			pExtendAttr->pTableDefine->pExtendAttr->pForeignIndex = pExtendAttr->pPrimaryIndex;
			pExtendAttr->pPrimaryIndex->pExtendAttr->pTableDefine->pExtendAttr->pPrimeIndex = pExtendAttr->pPrimaryIndex;
		}

		char*pShift, *pField = pFieldList;
		{
			pShift = (char*)CString::CharOfString(pField, ',');
			if(pShift)
				pShift[0] = 0;
			uint32 nFieldNo = ((CMdbTableDefine*)pExtendAttr->pTableDefine)->GetFieldNo(pField);
			pExtendAttr->pFields = (uint32*)CMalloc::Realloc(pExtendAttr->pFields, (pExtendAttr->nFieldCount+1)<<2);
			pExtendAttr->pFields[pExtendAttr->nFieldCount] = nFieldNo;
			++pExtendAttr->nFieldCount;
			pField = pShift;
			if(pField)
			{
				pField[0] = ',';
				++pField;
			}
		}

		((CMdbTableDefine*)pExtendAttr->pTableDefine)->AddIndex(sDbName, this);
		if(pExtendAttr->pPrimaryIndex)
			((CMdbIndexDefine*)pExtendAttr->pPrimaryIndex)->AddDetail(sDbName, this);
	}

	return bRet;
}

//-----------------------------------------------------
// CMdbDefine
//-----------------------------------------------------
CMdbDefine::CMdbDefine(const char* sDbName)
{
	m_sDbName = sDbName;
}

CMdbDefine::~CMdbDefine()
{
	if(m_oMdbDef.GetSize())
	{
		CRbTreeNode* idx = m_oMdbDef.First();
		CRbTreeNode* end = m_oMdbDef.End();
		for(; idx!=end; idx=m_oMdbDef.GetNext(idx))
		{
			CMdbItemDef& item = m_oMdbDef.GetItem(idx);
			switch(item.nType)
			{
			case MDB_TABLE_DEFINE:
				((CMdbTableDefine&)item.oItemDef.oTabDef).~CMdbTableDefine();
				break;
			case MDB_INDEX_DEFINE:
				((CMdbIndexDefine&)item.oItemDef.oIdxDef).~CMdbIndexDefine();
				break;
			}
		}
		m_oMdbDef.Clear();
	}
}

const char* CMdbDefine::GetDbName()
{
	return m_sDbName;
}

CMdbTableDef* CMdbDefine::GetTableDefine(const char* sTableName)
{
	CMdbTableDef * pTableDef = NULL;
	CRbTreeNode* idx = m_oMdbDef.Find(CString(sTableName));
	CRbTreeNode* end = m_oMdbDef.End();
	if(idx!=end)
	{
		CMdbItemDef& item = m_oMdbDef.GetItem(idx);
		if(item.nType == MDB_TABLE_DEFINE)
			pTableDef = &item.oItemDef.oTabDef;
	}
	return pTableDef;
}

CMdbIndexDef* CMdbDefine::GetIndexDefine(const char* sIndexName)
{
	CMdbIndexDef * pIndexDef = NULL;
	CRbTreeNode* idx = m_oMdbDef.Find(CString(sIndexName));
	CRbTreeNode* end = m_oMdbDef.End();
	if(idx!=end)
	{
		CMdbItemDef& item = m_oMdbDef.GetItem(idx);
		if(item.nType == MDB_INDEX_DEFINE)
			pIndexDef = &item.oItemDef.oIdxDef;
	}
	return pIndexDef;
}

void CMdbDefine::MakeTableList(const char* sTableName)
{
	if(!m_oTableList.Empty())
		m_oTableList += ",";
	m_oTableList += sTableName;
}

void CMdbDefine::MakeIndexList(const char* sIndexName)
{
	if(!m_oIndexList.Empty())
		m_oIndexList += ",";
	m_oIndexList += sIndexName;
}

const char* CMdbDefine::GetTableList()
{
	return m_oTableList.GetStr();
}

const char* CMdbDefine::GetIndexList()
{
	return m_oIndexList.GetStr();
}

uint32 CMdbDefine::CreateTable(const char* sTableName, CMdbTableDefine *&pDefine)
{
	if(InvalidDbName((char*)sTableName))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbDefine::CreateTable(%s.%s): MDB_INVALID_NAME", m_sDbName, sTableName));
		return MDB_INVALID_NAME;
	}
	if(GetTableDefine(sTableName))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbDefine::CreateTable(%s.%s): MDB_TABLE_EXIST", m_sDbName, sTableName));
		return MDB_TABLE_EXIST;
	}
	CMdbItemDef &oItem = m_oMdbDef[sTableName];
	oItem.nType = MDB_TABLE_DEFINE;
	CMdbTableDefine &oDefine = *(CMdbTableDefine*)&oItem.oItemDef.oTabDef;
	new(&oDefine) CMdbTableDefine;
	MakeTableList(sTableName);
	pDefine = &oDefine;
	return MDB_SUCCESS;
}

uint32 CMdbDefine::CreateTable(CMdbTableDef* pDefine)
{
	char* sTableName = pDefine->sTableName;
	CMdbTableDefine* pRet = NULL;
	uint32 nRet = CreateTable(sTableName, pRet);
	if(!nRet)
		nRet = pRet->Initialize(this, pDefine);
	if(nRet && pRet)
		pRet->~CMdbTableDefine();
	return nRet;
}

uint32 CMdbDefine::CreateIndex(const char* sIndexName, CMdbIndexDefine *&pDefine)
{
	if(InvalidDbName((char*)sIndexName))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbDefine::CreateIndex(%s.%s): MDB_INVALID_NAME", m_sDbName, sIndexName));
		return MDB_INVALID_NAME;
	}
	if(GetIndexDefine(sIndexName))
	{
		FocpLog(FOCP_LOG_ERROR, ("CMdbDefine::CreateIndex(%s.%s): MDB_TABLE_EXIST", m_sDbName, sIndexName));
		return MDB_TABLE_EXIST;
	}
	CMdbItemDef &oItem = m_oMdbDef[sIndexName];
	oItem.nType = MDB_INDEX_DEFINE;
	CMdbIndexDefine &oDefine = *(CMdbIndexDefine*)&oItem.oItemDef.oIdxDef;
	new(&oDefine) CMdbIndexDefine;
	MakeIndexList(sIndexName);
	pDefine = &oDefine;
	return MDB_SUCCESS;
}

uint32 CMdbDefine::CreateIndex(CMdbIndexDef* pDefine)
{
	CMdbIndexDefine* pRet = NULL;
	char* sIndexName = pDefine->sIndexName;
	uint32 nRet = CreateIndex(sIndexName, pRet);
	if(!nRet)
		nRet = pRet->Initialize(this, pDefine);
	if(nRet && pRet)
		pRet->~CMdbIndexDefine();
	return nRet;
}

FOCP_END();
