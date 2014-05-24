
#include "RdbDef.hpp"

FOCP_BEGIN();

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
int32 CDbNameCompare::Compare(CString&l, CString& r)
{
	return l.Compare(r, false);
}

//---------------------------------------------------------
// CVirtualString
//---------------------------------------------------------
CVirtualString::~CVirtualString()
{
}

CVirtualString::CVirtualString(int32 bInMemory)
{
	m_bInMemory = bInMemory;
	m_nThis = 0;
}

CVirtualString::CVirtualString(uint64 nThis, int32 bInMemory)
{
	m_nThis = nThis;
	if(!nThis)
		m_bInMemory = bInMemory;
	else
		m_bInMemory = IsMemoryAddr(nThis);
}

CVirtualString::CVirtualString(CVirtualString& oSrc)
{
	m_nThis = oSrc.m_nThis;
	m_bInMemory = oSrc.m_bInMemory;
}

CVirtualString& CVirtualString::operator=(CVirtualString& oSrc)
{
	if(this != &oSrc)
	{
		m_nThis = oSrc.m_nThis;
		m_bInMemory = oSrc.m_bInMemory;
	}
	return *this;
}

uint64 CVirtualString::GetThis()
{
	return m_nThis;
}

CString& CVirtualString::GetString(CString& oStr)
{
	oStr.SetSize(0);
	if(m_nThis)
	{
		uint32 nLen;
		vquery(m_nThis, (char*)&nLen, 4);
		oStr.SetSize(nLen);
#if VMM_SAFE_ENABLED
		if(oStr.GetSize() != nLen)
			Abort();
#endif
		vquery(m_nThis+4, (char*)oStr.GetStr(), nLen);
	}
	return oStr;
}

bool CVirtualString::SetString(CString& oStr)
{
	uint64 nThis = 0;
	uint32 nLen = oStr.GetSize();
	if(nLen)
	{
		nThis = vmalloc(nLen+4, m_bInMemory);
		if(!nThis)
			return false;
		vcommit(nThis, (char*)&nLen, 4);
		vcommit(nThis+4, (char*)oStr.GetStr(), nLen);
	}
	if(m_nThis)
		vfree(m_nThis);
	m_nThis = nThis;
	return true;
}

int32 CVirtualString::IsMemoryObject()
{
	return m_bInMemory;
}

//---------------------------------------------------------
// Database Dictionary Function List
//---------------------------------------------------------
static uint64 SaveName(const char* sLogName, char* sName);
static char* ReadName(const char* sLogName, uint64 nAddr);

static void ClearFieldDefine(uint64 nAddr);
static void ReadFieldDefine(const char* sLogName, CRdbFieldDef* &pFieldDefines, uint64 nFieldDefines, uint32 nFieldCount);
static bool WriteFieldDefine(const char* sLogName, CRdbFieldDef* pFieldDefines, uint64& nFieldDefines, uint32 nFieldCount);

static void ClearTableDefine(void* pBuf);
static void ReadTableDefine(const char* sLogName, CRdbTableDef & oTableDef, CRdbTableDefInFile& oDefInFile);
static bool WriteTableDefine(const char* sLogName, CRdbTableDef & oTableDef, CRdbTableDefInFile& oDefInFile);
static CRdbFieldDef* GetFieldDefine(CRdbTableDef & oTableDef, char* sFieldName);

static void ClearIndexDefine(void* pBuf);
static void ReadIndexDefine(const char* sLogName, CRdbIndexDef & oIndexDef, CRdbIndexDefInFile& oDefInFile);
static bool WriteIndexDefine(const char* sLogName, CRdbIndexDef & oIndexDef, CRdbIndexDefInFile& oDefInFile);

static CRdbItemDef* GetDbItemDefine(CRdbDef & oDbDef, char* same);
static void NewTableDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbTableDef &oTabDef);
static void NewIndexDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbIndexDef &oIdxDef);
static void DelDbItemDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, char* sName);
static bool ModifyDbItemDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbItemDef& oDboDef);
static uint64 GetDbItemAddr(const char* sLogName, uint64 nDbAddr, char* sName);

static void ClearDataBaseDefine(void* pBuf);
static void FreeDataBaseDefine(CRdbDef & oDbDef);

static uint64 AllocDataBasesDefineSpace();
static void FreeDataBasesDefine(CRdbSysDef & oDbsDef);
static uint32 LoadDataBasesDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef);
static CRdbDef* GetDataBaseDefine(CRdbSysDef & oDbsDef, char* sDbName);
static void NewDataBaseDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef, char* sDbName);
static void DelDataBasesDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef, char* sDbName);
static uint64 GetDataBaseAddress(uint64 nDbsAddr, char* sDbName);

//---------------------------------------------------------
// SaveName & ReadName & CloneString
//---------------------------------------------------------
static uint64 SaveName(const char* sLogName, char* sName)
{
	uint64 nAddr = vmalloc(RDB_NAME_MAXLEN, 0);
	if(nAddr)
	{
		uint32 nLen = CString::StringLength(sName);
		nLen = (nLen == RDB_NAME_MAXLEN)?nLen:(nLen+1);
		vcommit(nAddr, sName, nLen);
	}
	else
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("SaveName(%s): RDB_LACK_STORAGE", sName));
	return nAddr;
}

static char* ReadName(const char* sLogName, uint64 nAddr)
{
	char sName[RDB_NAME_MAXLEN+1];
	sName[RDB_NAME_MAXLEN] = 0;
	vquery(nAddr, sName, RDB_NAME_MAXLEN);
	char* sRet = CloneString(sLogName, sName);
#if VMM_SAFE_ENABLED
	if(!sRet)
		Abort();
#endif
	return sRet;
}

//---------------------------------------------------------
// Field Define Interface
//---------------------------------------------------------
static void ClearFieldDefine(uint64 nAddr)
{
	CRdbFieldDefInFile oDefInFile;
	vquery(nAddr, (char*)&oDefInFile, sizeof(oDefInFile));
	vfree(oDefInFile.nFieldName);
	if(oDefInFile.nDefault)
	{
		CString oNullStr;
		CVirtualString oStr(oDefInFile.nDefault);
		oStr.SetString(oNullStr);
	}
}

static void ReadFieldDefine(const char* sLogName, CRdbFieldDef* &pFieldDefines, uint64 nFieldDefines, uint32 nFieldCount)
{
	pFieldDefines = NULL;
	if(nFieldCount)
	{
		pFieldDefines = new CRdbFieldDef[nFieldCount];
#if VMM_SAFE_ENABLED
		if(!pFieldDefines)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("ReadFieldDefine: RDB_LACK_MEMORY"));
			Abort();
		}
#endif
		for(uint32 i=0; i<nFieldCount; ++i)
		{
			CRdbFieldDefInFile oDefInFile;
			vquery(nFieldDefines+i*sizeof(oDefInFile), (char*)&oDefInFile, sizeof(oDefInFile));
			pFieldDefines[i].sFieldName = ReadName(sLogName, oDefInFile.nFieldName);
			pFieldDefines[i].sDefault = NULL;
			if(oDefInFile.nDefault)
			{
				CString s;
				CVirtualString oStr(oDefInFile.nDefault);
				oStr.GetString(s);
				pFieldDefines[i].sDefault = new char[s.GetSize()+1];
#if VMM_SAFE_ENABLED
				if(!pFieldDefines[i].sDefault)
				{
					FocpLogEx(sLogName, FOCP_LOG_ERROR, ("ReadFieldDefine: RDB_LACK_MEMORY"));
					Abort();
				}
#endif
				CString::StringCopy(pFieldDefines[i].sDefault, s.GetStr());
			}
			pFieldDefines[i].nJob = oDefInFile.nJob;
			pFieldDefines[i].nType = oDefInFile.nType;
			pFieldDefines[i].nLen = oDefInFile.nLen;
			pFieldDefines[i].bNotNull = oDefInFile.bNotNull;
			pFieldDefines[i].nRecSize = oDefInFile.nRecSize;
		}
	}
}

static bool WriteFieldDefine(const char* sLogName, CRdbFieldDef* pFieldDefines, uint64& nFieldDefines, uint32 nFieldCount)
{
	uint32 i = 0;
	nFieldDefines = 0;
	CRdbFieldDefInFile oDefInFile;
	if(nFieldCount)
	{
		nFieldDefines = vmalloc(nFieldCount*sizeof(oDefInFile), 0);
		if(!nFieldDefines)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteFieldDefine: RDB_LACK_STORAGE"));
			return false;
		}
		for(i=0; i<nFieldCount; ++i)
		{
			oDefInFile.nFieldName = SaveName(sLogName, pFieldDefines[i].sFieldName);
			if(!oDefInFile.nFieldName)
				goto error;
			oDefInFile.nDefault = 0;
			if(pFieldDefines[i].sDefault)
			{
				CVirtualString oStr;
				CString s = pFieldDefines[i].sDefault;
				if(oStr.SetString(s))
					oDefInFile.nDefault = oStr.GetThis();
				else
				{
					vfree(oDefInFile.nFieldName);
					goto error;
				}
			}
			oDefInFile.nJob = pFieldDefines[i].nJob;
			oDefInFile.nType = pFieldDefines[i].nType;
			oDefInFile.nLen = pFieldDefines[i].nLen;
			oDefInFile.bNotNull = pFieldDefines[i].bNotNull;
			oDefInFile.nRecSize = pFieldDefines[i].nRecSize;		
			vcommit(nFieldDefines+i*sizeof(oDefInFile), (char*)&oDefInFile, sizeof(oDefInFile));
		}
	}
	return true;
error:
	FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteFieldDefine: RDB_LACK_STORAGE"));
	for(uint32 j=0; j<i; ++j)
		ClearFieldDefine(nFieldDefines+j*sizeof(oDefInFile));
	vfree(nFieldDefines);
	nFieldDefines = 0;
	return false;
}

//---------------------------------------------------------
// Table Define Interface
//---------------------------------------------------------
static void ClearTableDefine(void* pBuf)
{
	CRdbTableDefInFile &oDefInFile = *(CRdbTableDefInFile*)pBuf;
	vfree(oDefInFile.nTableName);
	if(oDefInFile.nFieldDefines)
	{
		for(uint32 i=0; i<oDefInFile.nFieldCount; ++i)
			ClearFieldDefine(oDefInFile.nFieldDefines + i*sizeof(CRdbFieldDefInFile));
		vfree(oDefInFile.nFieldDefines);
	}
}

static void ReadTableDefine(const char* sLogName, CRdbTableDef & oTableDef, CRdbTableDefInFile& oDefInFile)
{
	oTableDef.sTableName = ReadName(sLogName, oDefInFile.nTableName);
	oTableDef.nFieldCount = oDefInFile.nFieldCount;
	oTableDef.nMaxJob = oDefInFile.nMaxJob;
	oTableDef.nStorage = oDefInFile.nStorage;
	oTableDef.nTableNo = oDefInFile.nTableNo;
	oTableDef.nStorageAddr = oDefInFile.nStorageAddr;
	ReadFieldDefine(sLogName, oTableDef.pFieldDefines, oDefInFile.nFieldDefines, oTableDef.nFieldCount);
}

static bool WriteTableDefine(const char* sLogName, CRdbTableDef & oTableDef, CRdbTableDefInFile& oDefInFile)
{
	CRdbTableDefInFile oDefInFile2;
	oDefInFile2.nTableName = SaveName(sLogName, oTableDef.sTableName);
	if(!oDefInFile2.nTableName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteTableDefine: RDB_LACK_STORAGE"));
		return false;
	}
	oDefInFile2.nFieldCount = oTableDef.nFieldCount;
	oDefInFile2.nMaxJob = oTableDef.nMaxJob;
	oDefInFile2.nStorage = oTableDef.nStorage;
	oDefInFile2.nTableNo = oTableDef.nTableNo;
	oDefInFile2.nStorageAddr = oTableDef.nStorageAddr;
	if(!WriteFieldDefine(sLogName, oTableDef.pFieldDefines, oDefInFile2.nFieldDefines, oTableDef.nFieldCount))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteTableDefine: RDB_LACK_STORAGE"));
		vfree(oDefInFile2.nTableName);
		return false;
	}
	oDefInFile = oDefInFile2;
	return true;
}

static CRdbFieldDef* GetFieldDefine(CRdbTableDef & oTableDef, char* sFieldName)
{
	for(uint32 i=0; i<oTableDef.nFieldCount; ++i)
	{
		if(!CString::StringCompare(oTableDef.pFieldDefines[i].sFieldName, sFieldName, false))
			return oTableDef.pFieldDefines + i;
	}
	return NULL;
}

//---------------------------------------------------------
// Index Define Interface
//---------------------------------------------------------
static void ClearIndexDefine(void* pBuf)
{
	CRdbIndexDefInFile &oDefInFile = *(CRdbIndexDefInFile*)pBuf;
	vfree(oDefInFile.nIndexName);
	vfree(oDefInFile.nTableName);
	if(oDefInFile.nPrimaryIndex)
		vfree(oDefInFile.nPrimaryIndex);
	if(oDefInFile.nFieldList)
	{
		CString oNullStr;
		CVirtualString oStr(oDefInFile.nFieldList);
		oStr.SetString(oNullStr);
	}
}

static void ReadIndexDefine(const char* sLogName, CRdbIndexDef & oIndexDef, CRdbIndexDefInFile& oDefInFile)
{
	oIndexDef.sIndexName = ReadName(sLogName, oDefInFile.nIndexName);
	oIndexDef.sTableName = ReadName(sLogName, oDefInFile.nTableName);
	oIndexDef.sPrimaryIndex = NULL;
	if(oDefInFile.nPrimaryIndex)
		oIndexDef.sPrimaryIndex = ReadName(sLogName, oDefInFile.nPrimaryIndex);
	oIndexDef.pFieldList = NULL;
	if(oDefInFile.nFieldList)
	{
		CString s;
		CVirtualString oStr(oDefInFile.nFieldList);
		oStr.GetString(s);
		oIndexDef.pFieldList = new char[s.GetSize()+1];
#if VMM_SAFE_ENABLED
		if(!oIndexDef.pFieldList)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("ReadIndexDefine: RDB_LACK_MEMORY"));
			Abort();
		}
#endif
		CString::StringCopy(oIndexDef.pFieldList, s.GetStr());
	}
	oIndexDef.nStorageAddr = oDefInFile.nStorageAddr;
	oIndexDef.nQualifier = oDefInFile.nQualifier;
	oIndexDef.nArithmetic = oDefInFile.nArithmetic;
	oIndexDef.nHashRate = oDefInFile.nHashRate;
	oIndexDef.nIndexNo = oDefInFile.nIndexNo;
}

static bool WriteIndexDefine(const char* sLogName, CRdbIndexDef & oIndexDef, CRdbIndexDefInFile& oDefInFile)
{
	CRdbIndexDefInFile oDefInFile2;
	oDefInFile2.nIndexName = SaveName(sLogName, oIndexDef.sIndexName);
	if(!oDefInFile2.nIndexName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteIndexDefine(%s): RDB_LACK_STORAGE", oIndexDef.sIndexName));
		return false;
	}
	oDefInFile2.nTableName = SaveName(sLogName, oIndexDef.sTableName);
	if(!oDefInFile2.nTableName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteIndexDefine(%s): RDB_LACK_STORAGE", oIndexDef.sIndexName));
		vfree(oDefInFile2.nIndexName);
		return false;
	}
	oDefInFile2.nPrimaryIndex = 0;
	if(oIndexDef.sPrimaryIndex)
	{
		oDefInFile2.nPrimaryIndex = SaveName(sLogName, oIndexDef.sPrimaryIndex);
		if(!oDefInFile2.nPrimaryIndex)
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteIndexDefine(%s): RDB_LACK_STORAGE", oIndexDef.sIndexName));
			vfree(oDefInFile2.nIndexName);
			vfree(oDefInFile2.nTableName);
			return false;
		}
	}
	oDefInFile2.nFieldList = 0;
	if(oIndexDef.pFieldList)
	{
		CVirtualString oStr;
		CString oFieldList(oIndexDef.pFieldList);
		if(!oStr.SetString(oFieldList))
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("WriteIndexDefine(%s): RDB_LACK_STORAGE", oIndexDef.sIndexName));
			vfree(oDefInFile2.nIndexName);
			vfree(oDefInFile2.nTableName);
			if(oDefInFile2.nPrimaryIndex)
				vfree(oDefInFile2.nPrimaryIndex);
			return false;
		}
		oDefInFile2.nFieldList = oStr.GetThis();
	}
	oDefInFile2.nStorageAddr = oIndexDef.nStorageAddr;
	oDefInFile2.nQualifier = oIndexDef.nQualifier;
	oDefInFile2.nArithmetic = oIndexDef.nArithmetic;
	oDefInFile2.nHashRate = oIndexDef.nHashRate;
	oDefInFile2.nIndexNo = oIndexDef.nIndexNo;
	oDefInFile = oDefInFile2;
	return true;
}

//---------------------------------------------------------
// Database Item Define Interface
//---------------------------------------------------------
static CRdbItemDef* GetDbItemDefine(CRdbDef & oDbDef, char* sName)
{
	CRdbItemDef * pDbItem = NULL;
	CRbTreeNode* idx = oDbDef.oDbItemDefTable.Find(CString(sName));
	CRbTreeNode* end = oDbDef.oDbItemDefTable.End();
	if(idx!=end)
	{
		CRdbItemDef& item = oDbDef.oDbItemDefTable.GetItem(idx);
		pDbItem = &item;
	}
	return pDbItem;
}

static void NewTableDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbTableDef &oTabDef)
{
	CRdbItemDef oDbItemDef;
	if(!CloneTableDef(sLogName, oDbItemDef.oItemDef.oTabDef, oTabDef))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("NewTableDefine(%s,%s): RDB_LACK_STORAGE", oDbDef.sDbName, oTabDef.sTableName));
		return;
	}
	char* sTableName = oDbItemDef.oItemDef.oTabDef.sTableName;
	oDbItemDef.nType = RDB_TABLE_DEFINE;
	bool bError = false;
	if( oDbDef.oDbItemDefTable.Insert(CString(sTableName), oDbItemDef) == oDbDef.oDbItemDefTable.End() )
		bError = true;
	else
	{
		CRdbItemDefInFile oTabDefInFile;
		CRdbDefInFile oDbDefInFile;

		if(!WriteTableDefine(sLogName, oDbItemDef.oItemDef.oTabDef, oTabDefInFile.oItemDef.oTabDef))
		{
			bError = true;
			oDbDef.oDbItemDefTable.Remove(CString(sTableName));
		}
		else
		{
			CDbItemDefineAccess oAccess;
			CVirtualAllocator oAllocator;
			CGetDbItemKey oGetKey;
			CDbNameCompare oCompare;
			oGetKey.sLogName = (char*)sLogName;
			oAccess.sLogName = (char*)sLogName;

			CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);
			vquery(nDbAddr, (char*)&oDbDefInFile, RDB_DB_DEFINE_SIZE);
			oItemInfile.SetThis(oDbDefInFile.nDbDef);
			oTabDefInFile.nType = oDbItemDef.nType;
			if(oItemInfile.Insert(oTabDefInFile) == oItemInfile.End())
			{
				bError = true;
				oDbDef.oDbItemDefTable.Remove(CString(sTableName));
				ClearTableDefine(&oTabDefInFile.oItemDef.oTabDef);
			}
		}
	}
	if(bError)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("NewTableDefine(%s,%s): RDB_LACK_STORAGE", oDbDef.sDbName, oTabDef.sTableName));
		FreeTableDefine(oDbItemDef.oItemDef.oTabDef);
	}
}

static void NewIndexDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbIndexDef &oIdxDef)
{
	CRdbItemDef oDbItemDef;
	if(!CloneIndexDef(sLogName, oDbItemDef.oItemDef.oIdxDef, oIdxDef))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("NewIndexDefine(%s,%s): RDB_LACK_STORAGE", oDbDef.sDbName, oIdxDef.sIndexName));
		return;
	}
	char* sIndexName = oDbItemDef.oItemDef.oIdxDef.sIndexName;
	oDbItemDef.nType = RDB_INDEX_DEFINE;
	bool bError = false;
	if(oDbDef.oDbItemDefTable.Insert(CString(sIndexName), oDbItemDef) == oDbDef.oDbItemDefTable.End())
		bError = true;
	else
	{
		CRdbItemDefInFile oIdxDefInFile;
		CRdbDefInFile oDbDefInFile;
		if(!WriteIndexDefine(sLogName, oDbItemDef.oItemDef.oIdxDef, oIdxDefInFile.oItemDef.oIdxDef))
		{
			bError = true;
			oDbDef.oDbItemDefTable.Remove(CString(sIndexName));
		}
		else
		{
			CDbItemDefineAccess oAccess;
			CVirtualAllocator oAllocator;
			CGetDbItemKey oGetKey;
			CDbNameCompare oCompare;
			oGetKey.sLogName = (char*)sLogName;
			oAccess.sLogName = (char*)sLogName;

			CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);
			vquery(nDbAddr, (char*)&oDbDefInFile, RDB_DB_DEFINE_SIZE);
			oItemInfile.SetThis(oDbDefInFile.nDbDef);
			oIdxDefInFile.nType = oDbItemDef.nType;
			if(oItemInfile.Insert(oIdxDefInFile) == oItemInfile.End())
			{
				bError = true;
				oDbDef.oDbItemDefTable.Remove(CString(sIndexName));
				ClearIndexDefine(&oIdxDefInFile.oItemDef.oIdxDef);
			}
		}
	}
	if(bError)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("NewIndexDefine(%s,%s): RDB_LACK_STORAGE", oDbDef.sDbName, oIdxDef.sIndexName));
		FreeIndexDefine(oDbItemDef.oItemDef.oIdxDef);
	}
}

static void DelDbItemDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, char* sName)
{
	CRbTreeNode* idx = oDbDef.oDbItemDefTable.Find(CString(sName));
	CRbTreeNode* end = oDbDef.oDbItemDefTable.End();
	if(idx!=end)
	{
		CRdbItemDef& item = oDbDef.oDbItemDefTable.GetItem(idx);
		switch(item.nType)
		{
		case RDB_TABLE_DEFINE:
			FreeTableDefine(item.oItemDef.oTabDef);
			break;
		case RDB_INDEX_DEFINE:
			FreeIndexDefine(item.oItemDef.oIdxDef);
			break;
		}

		CDbItemDefineAccess oAccess;
		CVirtualAllocator oAllocator;
		CGetDbItemKey oGetKey;
		CDbNameCompare oCompare;
		oGetKey.sLogName = (char*)sLogName;
		oAccess.sLogName = (char*)sLogName;

		CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);
		CRdbDefInFile oDbDefInFile;
		vquery(nDbAddr, (char*)&oDbDefInFile, RDB_DB_DEFINE_SIZE);

		oItemInfile.SetThis(oDbDefInFile.nDbDef);
		CString oName(sName);
		CDbItemDefineTableInFile::CIterator idx2 = oItemInfile.Find(oName);
		if(idx2 != oItemInfile.End())
			oItemInfile.Erase(idx2);
		oDbDef.oDbItemDefTable.Remove(idx);
	}
}

static uint64 GetDbItemAddr(const char* sLogName, uint64 nDbAddr, char* sName)
{
	CDbItemDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbItemKey oGetKey;
	CDbNameCompare oCompare;
	oGetKey.sLogName = (char*)sLogName;
	oAccess.sLogName = (char*)sLogName;

	CRdbDefInFile oDbDefInFile;
	CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	vquery(nDbAddr, (char*)&oDbDefInFile, RDB_DB_DEFINE_SIZE);

	oItemInfile.SetThis(oDbDefInFile.nDbDef);
	CString oName(sName);
	CDbItemDefineTableInFile::CIterator cidx = oItemInfile.Find(oName);
	if(cidx == oItemInfile.End())
		return 0;

	return cidx.GetThis();
}

static bool ModifyDbItemDefine(const char* sLogName, uint64 nDbAddr, CRdbDef & oDbDef, CRdbItemDef& oDbItemDef)
{
	char* sName;
	switch(oDbItemDef.nType)
	{
	case RDB_TABLE_DEFINE:
		sName = oDbItemDef.oItemDef.oTabDef.sTableName;
		break;
	case RDB_INDEX_DEFINE:
		sName = oDbItemDef.oItemDef.oIdxDef.sIndexName;
		break;
	}

	CDbItemDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbItemKey oGetKey;
	CDbNameCompare oCompare;
	oGetKey.sLogName = (char*)sLogName;
	oAccess.sLogName = (char*)sLogName;

	CRdbItemDefInFile oItemDefInFile;
	CRdbDefInFile oDbDefInFile;
	CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);

	vquery(nDbAddr, (char*)&oDbDefInFile, RDB_DB_DEFINE_SIZE);
	oItemInfile.SetThis(oDbDefInFile.nDbDef);

	if(!oAccess.Write(oDbItemDef, oItemDefInFile))
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("ModifyDbItemDefine(@s): RDB_LACK_STORAGE", sName));
		return false;
	}

	if(oItemInfile.Insert(oItemDefInFile, true) == oItemInfile.End())
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("ModifyDbItemDefine(@s): RDB_LACK_STORAGE", sName));
		return false;
	}

	CRdbItemDef& item = oDbDef.oDbItemDefTable[sName];
	switch(item.nType)
	{
	case RDB_TABLE_DEFINE:
		FreeTableDefine(item.oItemDef.oTabDef);
		break;
	case RDB_INDEX_DEFINE:
		FreeIndexDefine(item.oItemDef.oIdxDef);
		break;
	}
	item = oDbItemDef;
	return true;
}

//---------------------------------------------------------
// DataBase Define Interface
//---------------------------------------------------------
static void FreeDataBaseDefine(CRdbDef & oDbDef)
{
	delete[] oDbDef.sDbName;
	CRbTreeNode* idx = oDbDef.oDbItemDefTable.First();
	CRbTreeNode* end = oDbDef.oDbItemDefTable.End();
	for(; idx!=end; idx=oDbDef.oDbItemDefTable.GetNext(idx))
	{
		CRdbItemDef& item = oDbDef.oDbItemDefTable.GetItem(idx);
		switch(item.nType)
		{
		case RDB_TABLE_DEFINE:
			FreeTableDefine(item.oItemDef.oTabDef);
			break;
		case RDB_INDEX_DEFINE:
			FreeIndexDefine(item.oItemDef.oIdxDef);
			break;
		}
	}
}

static void ClearDataBaseDefine(void* pBuf)
{
	CDbItemDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbItemKey oGetKey;
	CDbNameCompare oCompare;
	char sLogName[RDB_NAME_MAXLEN+10];

	CRdbDefInFile &oDefInFile = *(CRdbDefInFile*)pBuf;

	char* sDbName = ReadName("Rdb", oDefInFile.nDbName);
	CString::StringCopy(sLogName, "Rdb::");
	CString::StringCatenate(sLogName, sDbName);
	delete[] sDbName;

	vfree(oDefInFile.nDbName);

	oGetKey.sLogName = (char*)sLogName;
	oAccess.sLogName = (char*)sLogName;

	CDbItemDefineTableInFile oItemInfile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oItemInfile.SetThis(oDefInFile.nDbDef);
	oItemInfile.DestroyObject();
}

static uint64 AllocDataBasesDefineSpace()
{
	CDbDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbKey oGetKey;
	CDbNameCompare oCompare;

	CDbDefineTableInFile oDbTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbTableInFile.CreateObject(0, 0);
	return oDbTableInFile.GetThis();
}

static void FreeDataBasesDefine(CRdbSysDef & oDbsDef)
{
	CRbTreeNode* idx = oDbsDef.oDbDefTable.First();
	CRbTreeNode* end = oDbsDef.oDbDefTable.End();
	for(; idx!=end; idx=oDbsDef.oDbDefTable.GetNext(idx))
	{
		CRdbDef & oDbDef = oDbsDef.oDbDefTable.GetItem(idx);
		FreeDataBaseDefine(oDbDef);
	}
	oDbsDef.oDbDefTable.Clear();
}

static uint32 LoadDataBasesDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef)
{
	CDbDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbKey oGetKey;
	CDbNameCompare oCompare;

	CRdbDef oDbDef;
	CDbDefineTableInFile oDbTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbTableInFile.SetThis(nDbsAddr);
	CDbDefineTableInFile::CIterator idx = oDbTableInFile.Begin();
	CDbDefineTableInFile::CIterator end = oDbTableInFile.End();
	for(; idx!=end; ++idx)
	{
		oDbDef.oDbItemDefTable.Clear();
		CRdbDefInFile &oDbDefInFile = idx.GetValue();
		oAccess.Read(oDbDef, oDbDefInFile);

		if(oDbsDef.oDbDefTable.Insert(CString(oDbDef.sDbName), oDbDef) == oDbsDef.oDbDefTable.End())
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("LoadDataBasesDefine(@s): RDB_LACK_MEMORY", oDbDef.sDbName));
			FreeDataBaseDefine(oDbDef);
			FreeDataBasesDefine(oDbsDef);
			return RDB_LACK_MEMORY;
		}
	}
	return RDB_SUCCESS;
}

static CRdbDef* GetDataBaseDefine(CRdbSysDef & oDbsDef, char* sDbName)
{
	CRdbDef* pDbDef = NULL;
	CRbTreeNode* idx = oDbsDef.oDbDefTable.Find(CString(sDbName));
	if(idx != oDbsDef.oDbDefTable.End())
	{
		CRdbDef & oDb = oDbsDef.oDbDefTable.GetItem(idx);
		pDbDef = &oDb;
	}
	return pDbDef;
}

static void NewDataBaseDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef, char* sDbName)
{
	CDbDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbKey oGetKey;
	CDbNameCompare oCompare;

	CRdbDef oDbDef;
	oDbDef.nTableCount = 0;
	oDbDef.nIndexCount = 0;
	oDbDef.sDbName = CloneString(GetLogName(), sDbName);
	if(!oDbDef.sDbName)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("NewDataBaseDefine(%s): RDB_LACK_MEMORY", sDbName));
		return;
	}
	if(oDbsDef.oDbDefTable.Insert(CString(oDbDef.sDbName), oDbDef) == oDbsDef.oDbDefTable.End())
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("NewDataBaseDefine(%s): RDB_LACK_MEMORY", sDbName));
		delete[] oDbDef.sDbName;
		return;
	}
	CDbDefineTableInFile oDbTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbTableInFile.SetThis(nDbsAddr);
	CRdbDefInFile oDbDefInFile;
	if(!oAccess.Write(oDbDef, oDbDefInFile))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("NewDataBaseDefine(%s): RDB_LACK_STORAGE", sDbName));
		oDbsDef.oDbDefTable.Remove(CString(sDbName));
		delete[] oDbDef.sDbName;
	}
	if(oDbTableInFile.Insert(oDbDefInFile) == oDbTableInFile.End())
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("NewDataBaseDefine(%s): RDB_LACK_STORAGE", sDbName));
		oDbsDef.oDbDefTable.Remove(CString(sDbName));
		delete[] oDbDef.sDbName;
	}
}

static void DelDataBasesDefine(uint64 nDbsAddr, CRdbSysDef & oDbsDef, char* sDbName)
{
	CDbDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbKey oGetKey;
	CDbNameCompare oCompare;

	CRbTreeNode* idx = oDbsDef.oDbDefTable.Find(CString(sDbName));
	CRbTreeNode* end = oDbsDef.oDbDefTable.End();
	if(idx != end)
	{
		CRdbDef & oDbDef = oDbsDef.oDbDefTable.GetItem(idx);
		FreeDataBaseDefine(oDbDef);
		oDbsDef.oDbDefTable.Remove(idx);

		CDbDefineTableInFile oDbTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
		oDbTableInFile.SetThis(nDbsAddr);
		CString oName(sDbName);
		CDbDefineTableInFile::CIterator cidx = oDbTableInFile.Find(oName);
		if(cidx != oDbTableInFile.End())
			oDbTableInFile.Erase(cidx);
	}
}

static uint64 GetDataBaseAddress(uint64 nDbsAddr, char* sDbName)
{
	CDbDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbKey oGetKey;
	CDbNameCompare oCompare;

	CDbDefineTableInFile oDbTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbTableInFile.SetThis(nDbsAddr);

	CString oName(sDbName);
	CDbDefineTableInFile::CIterator cidx = oDbTableInFile.Find(oName);
	if(cidx == oDbTableInFile.End())
		return 0;

	return cidx.GetThis();
}

//-----------------------------------------------------
// CDbItemDefineAccess
//-----------------------------------------------------
CString CGetDbItemKey::GetKey(CRdbItemDefInFile& oSrc)
{
	uint64 nAddr = oSrc.oItemDef.oTabDef.nTableName;
	char * sName = ReadName(sLogName, nAddr);
	CString oName(sName);
	delete[] sName;
	return oName;
}

CRdbItemDef& CDbItemDefineAccess::Read(CRdbItemDef& oObj, CRdbItemDefInFile& oDefInFile)
{
	oObj.nType = oDefInFile.nType;
	switch(oObj.nType)
	{
	case RDB_TABLE_DEFINE:
		ReadTableDefine(sLogName, oObj.oItemDef.oTabDef, oDefInFile.oItemDef.oTabDef);
		break;
	case RDB_INDEX_DEFINE:
		ReadIndexDefine(sLogName, oObj.oItemDef.oIdxDef, oDefInFile.oItemDef.oIdxDef);
		break;
	}
	return oObj;
}

bool CDbItemDefineAccess::Write(CRdbItemDef& oObj, CRdbItemDefInFile& oDefInFile)
{
	bool bRet = false;
	CRdbItemDefInFile oDefInFile2;
	oDefInFile2.nType = oObj.nType;
	switch(oObj.nType)
	{
	case RDB_TABLE_DEFINE:
		bRet = WriteTableDefine(sLogName, oObj.oItemDef.oTabDef, oDefInFile2.oItemDef.oTabDef);
		break;
	case RDB_INDEX_DEFINE:
		bRet = WriteIndexDefine(sLogName, oObj.oItemDef.oIdxDef, oDefInFile2.oItemDef.oIdxDef);
		break;
	}
	if(bRet)
		oDefInFile = oDefInFile2;
	return bRet;
}

void CDbItemDefineAccess::Clear(void* pBuf)
{
	CRdbItemDefInFile& oDef = *(CRdbItemDefInFile*)pBuf;
	switch(oDef.nType)
	{
	case RDB_TABLE_DEFINE:
		ClearTableDefine(&oDef.oItemDef.oTabDef);
		break;
	case RDB_INDEX_DEFINE:
		ClearIndexDefine(&oDef.oItemDef.oIdxDef);
		break;
	}
}

//-----------------------------------------------------
// CDbDefineAccess
//-----------------------------------------------------
CString CGetDbKey::GetKey(CRdbDefInFile& oSrc)
{
	uint64 nAddr = oSrc.nDbName;
	char * sName = ReadName("Rdb", nAddr);
	CString oName(sName);
	delete[] sName;
	return oName;
}

CRdbDef& CDbDefineAccess::Read(CRdbDef& oObj, CRdbDefInFile& oDbDefInFile)
{
	char sLogName[RDB_NAME_MAXLEN+10];
	oObj.sDbName = ReadName("Rdb", oDbDefInFile.nDbName);
	CString::StringCopy(sLogName, "Rdb::");
	CString::StringCatenate(sLogName, oObj.sDbName);

	CDbItemDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbItemKey oGetKey;
	CDbNameCompare oCompare;
	oGetKey.sLogName = (char*)sLogName;
	oAccess.sLogName = (char*)sLogName;

	CDbItemDefineTableInFile oDbItemTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbItemTableInFile.SetThis(oDbDefInFile.nDbDef);

	CDbItemDefineTableInFile::CIterator idx = oDbItemTableInFile.Begin();
	CDbItemDefineTableInFile::CIterator end = oDbItemTableInFile.End();
	oObj.oDbItemDefTable.Clear();
	for(; idx!=end; ++idx)
	{
		CRdbItemDef item;
		CRdbItemDefInFile& oItemInFile = idx.GetValue();
		oAccess.Read(item, oItemInFile);
		CString oKey = oGetKey.GetKey(oItemInFile);
#if VMM_SAFE_ENABLED
		if(oObj.oDbItemDefTable.insert(oKey, item) == oObj.oDbItemDefTable.End())
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CDbDefineAccess::Read(%s): RDB_LACK_MEMORY", oObj.sDbName));
			Abort();
		}
#else
		oObj.oDbItemDefTable.Insert(oKey, item);
#endif
	}

	oObj.nIndexCount = oDbDefInFile.nIndexCount;
	oObj.nTableCount = oDbDefInFile.nTableCount;

	return oObj;
}

bool CDbDefineAccess::Write(CRdbDef& oObj, CRdbDefInFile& oDbDefInFile)
{
	char sLogName[RDB_NAME_MAXLEN+10];
	CString::StringCopy(sLogName, "Rdb::");
	CString::StringCatenate(sLogName, oObj.sDbName);

	CRdbDefInFile oDbDefInFile2;
	oDbDefInFile2.nTableCount = oObj.nTableCount;
	oDbDefInFile2.nIndexCount = oObj.nIndexCount;
	oDbDefInFile2.nDbName = SaveName(sLogName, oObj.sDbName);
	if(!oDbDefInFile2.nDbName)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CDbDefineAccess::Write(@s): RDB_LACK_STORAGE", oObj.sDbName));
		return false;
	}

	CDbItemDefineAccess oAccess;
	CVirtualAllocator oAllocator;
	CGetDbItemKey oGetKey;
	CDbNameCompare oCompare;
	oGetKey.sLogName = (char*)sLogName;
	oAccess.sLogName = (char*)sLogName;

	CDbItemDefineTableInFile oDbItemTableInFile(&oAccess, &oGetKey, &oCompare, &oAllocator);
	oDbItemTableInFile.CreateObject(0, 0);
	oDbDefInFile2.nDbDef = oDbItemTableInFile.GetThis();
	if(!oDbDefInFile2.nDbDef)
	{
		FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CDbDefineAccess::Write(@s): RDB_LACK_STORAGE", oObj.sDbName));
		vfree(oDbDefInFile2.nDbName);
		return false;
	}

	CRdbItemDefInFile oItemInFile;
	CRbTreeNode* idx = oObj.oDbItemDefTable.First();
	CRbTreeNode* end = oObj.oDbItemDefTable.End();
	for(; idx!=end; idx=oObj.oDbItemDefTable.GetNext(idx))
	{
		CRdbItemDef &item = oObj.oDbItemDefTable.GetItem(idx);
		if(!oAccess.Write(item, oItemInFile))
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CDbDefineAccess::Write(@s): RDB_LACK_STORAGE", oObj.sDbName));
			oDbItemTableInFile.DestroyObject();
			vfree(oDbDefInFile.nDbName);
			return false;
		}
		if(oDbItemTableInFile.Insert(oItemInFile) == oDbItemTableInFile.End())
		{
			FocpLogEx(sLogName, FOCP_LOG_ERROR, ("CDbDefineAccess::Write(@s): RDB_LACK_STORAGE", oObj.sDbName));
			oDbItemTableInFile.DestroyObject();
			vfree(oDbDefInFile.nDbName);
			return false;
		}
	}
	oDbDefInFile = oDbDefInFile2;
	return true;
}

void CDbDefineAccess::Clear(void* pBuf)
{
	ClearDataBaseDefine(pBuf);
}

//-----------------------------------------------------
// CFieldDefine
//-----------------------------------------------------
CFieldDefine::CFieldDefine(CTableDefine* pTableDefine)
{
	m_pBaseAttr = NULL;
	m_nOffset = 0;
	m_nDiskOffset = 0;
	m_nCounter = 0;
	m_pTableDefine = pTableDefine;
}

CFieldDefine::~CFieldDefine()
{
	m_pBaseAttr = NULL;
	m_nOffset = 0;
	m_nDiskOffset = 0;
	m_nCounter = 0;
}

CRdbFieldDef * CFieldDefine::GetBaseAttr()
{
	return m_pBaseAttr;
}

uint32 CFieldDefine::GetFieldOffset()
{
	return m_nOffset;
}

uint32 CFieldDefine::GetFieldOffsetInFile()
{
	return m_nDiskOffset;
}

uint32 CFieldDefine::GetSizeInRecord()
{
	return m_nSize;
}

uint32 CFieldDefine::GetSizeInFileRecord()
{
	return m_nDiskSize;
}

uint32 CFieldDefine::GetFieldNo()
{
	return m_nFieldNo;
}

void CFieldDefine::Refer()
{
	++m_nCounter;
}

void CFieldDefine::Release()
{
	if(m_nCounter)
		--m_nCounter;
}

uint32 CFieldDefine::GetReferenceCounter()
{
	return m_nCounter;
}

const char* CFieldDefine::GetLogName()
{
	return m_pTableDefine->GetLogName();
}

//-----------------------------------------------------
// CTableDefine
//-----------------------------------------------------
CTableDefine::CTableDefine(CTableDefineSet* pTableSet)
{
	m_pTableSet = pTableSet;
	m_pInstance = NULL;
	m_pBaseAttr = NULL;
	m_pFieldDefineSet = NULL;
	m_pJobSize = NULL;
	m_pJobRecSize = NULL;
	m_nRecordSize = 0;
	m_nFileRecordSize = 0;
	m_nFieldCount = 0;
	m_nIndexCount = 0;
	m_pIndexDefineSet = NULL;
	m_nVarFieldCount = 0;
	m_pVarFieldList = NULL;
}

CTableDefine::~CTableDefine()
{
	Clear();
}

CRdbTableDef * CTableDefine::GetBaseAttr()
{
	return m_pBaseAttr;
}

uint32 CTableDefine::GetFieldCount()
{
	return m_nFieldCount;
}

uint32 CTableDefine::GetFileFieldCount(int32 nJob)
{
	if(nJob <= 0)
		return 0;
	if(nJob == m_pBaseAttr->nMaxJob)
		return m_nFieldCount;
	return m_pJobSize[nJob-1];
}

uint32 CTableDefine::GetFileFieldFlagCount(int32 nJob)
{
	uint32 nSize = RDB_FLG_L(GetFileFieldCount(nJob)-1) + 1;
	if(nSize & 1)nSize += 1;
	return nSize;
}

uint32 CTableDefine::GetRecordSize()
{
	return m_nRecordSize;
}

uint32 CTableDefine::GetFileRecordSize()
{
	return m_nFileRecordSize;
}

uint32 CTableDefine::GetFileRecordSize(int32 nJob)
{
	if(nJob <= 0)
		return 0;
	if(nJob == m_pBaseAttr->nMaxJob)
		return m_nFileRecordSize;
	return m_pJobRecSize[nJob-1] + (GetFileFieldFlagCount(nJob)<<2);
}

CFieldDefine* CTableDefine::GetFieldDefine(uint32 nFieldNo)
{
	if(nFieldNo < m_nFieldCount)
		return (CFieldDefine*)m_pFieldDefineSet[nFieldNo];
	return NULL;
}

uint32 CTableDefine::GetFieldNo(char* sFieldName)
{
	return m_oNameTable.FindName(sFieldName);
}

uint32 CTableDefine::GetIndexCount()
{
	return m_nIndexCount;
}

CIndexDefine* CTableDefine::GetIndexDefine(uint32 nIndexNo)
{
	if(nIndexNo < m_nIndexCount)
		return m_pIndexDefineSet[nIndexNo];
	return NULL;
}

uint32 CTableDefine::AddIndex(CIndexDefine* pIndexDefine)
{
	if(m_nIndexCount >= RDB_MAX_INDEX_NUM)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddIndex(%s): RDB_INDEX_TOOMANY", 
			pIndexDefine->GetBaseAttr()->sIndexName));
		return RDB_INDEX_TOOMANY;
	}
	++m_nIndexCount;
	CIndexDefine** pNewIndexDefineSet = (CIndexDefine**)CMalloc::Realloc(m_pIndexDefineSet, m_nIndexCount*sizeof(void*));
	if(!pNewIndexDefineSet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddIndex(%s): RDB_LACK_MEMORY", 
			pIndexDefine->GetBaseAttr()->sIndexName));
		return RDB_LACK_MEMORY;
	}
	m_pIndexDefineSet = pNewIndexDefineSet;
	m_pIndexDefineSet[m_nIndexCount-1] = pIndexDefine;
	return RDB_SUCCESS;
}

void CTableDefine::DelIndex(CIndexDefine* pIndexDefine)
{
	uint32 i;
	for(i=0; i<m_nIndexCount; ++i)
	{
		if(m_pIndexDefineSet[i] == pIndexDefine)
			break;
	}
	if(i<m_nIndexCount)
	{
		for(; i<m_nIndexCount-1; ++i)
			m_pIndexDefineSet[i] = m_pIndexDefineSet[i+1];
		--m_nIndexCount;
		if(!m_nIndexCount)
		{
			CMalloc::Free(m_pIndexDefineSet);
			m_pIndexDefineSet = NULL;
		}
	}
}

bool CTableDefine::IsIndexField(uint32 nFieldNo)
{
	for(uint32 i=0; i<m_nIndexCount; ++i)
	{
		CIndexDefine* pIndex = m_pIndexDefineSet[i];
		uint32 nFieldCount = pIndex->GetFieldCount();
		for(uint32 j=0; j<nFieldCount; ++j)
		{
			if(nFieldNo == pIndex->GetFieldNo(j))
				return true;
		}
	}
	return false;
}

uint32 CTableDefine::GetIndexFieldNo(CIndexDefine* pIdxDef, uint32 nFieldNo)
{
	uint32 nFieldCount = pIdxDef->GetFieldCount();
	for(uint32 j=0; j<nFieldCount; ++j)
	{
		if(nFieldNo == pIdxDef->GetFieldNo(j))
			return j;
	}
	return (uint32)(-1);
}

uint32 CTableDefine::LoadFromDataDict(CRdbTableDef * pTabDef)
{
	if(m_nFieldCount)
	{
		for(uint32 i=0; i<m_nFieldCount; ++i)
			delete m_pFieldDefineSet[i];
		CMalloc::Free(m_pFieldDefineSet);
		m_pFieldDefineSet = NULL;
		m_oNameTable.Clear();
		m_nFieldCount = 0;
	}
	m_nRecordSize = 0;
	m_nFileRecordSize = 0;
	if(m_pJobSize)
	{
		delete[] m_pJobSize;
		m_pJobSize = NULL;
	}
	if(m_pJobRecSize)
	{
		delete[] m_pJobRecSize;
		m_pJobRecSize = NULL;
	}
	if(m_pVarFieldList)
	{
		CMalloc::Free(m_pVarFieldList);
		m_pVarFieldList = NULL;
	}
	m_nVarFieldCount = 0;

	uint32 i, nHeadSize, jobid, nFileRecordSize;
	m_pBaseAttr = pTabDef;
	if(m_pBaseAttr->nFieldCount > RDB_MAX_FIELD_NUM)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::LoadFromDataDict(%s): RDB_FIELD_TOOMANY", pTabDef->sTableName));
		return RDB_FIELD_TOOMANY;
	}
	if(pTabDef->nMaxJob > 1)
	{
		m_pJobSize = new uint32[pTabDef->nMaxJob-1];
		if(!m_pJobSize)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::LoadFromDataDict(%s): RDB_LACK_MEMORY", pTabDef->sTableName));
			return RDB_LACK_MEMORY;
		}
		m_pJobRecSize = new uint32[pTabDef->nMaxJob-1];
		if(!m_pJobRecSize)
		{
			delete[] m_pJobSize;
			m_pJobSize = NULL;
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::LoadFromDataDict(%s): RDB_LACK_MEMORY", pTabDef->sTableName));
			return RDB_LACK_MEMORY;
		}
		for(i=0; i<(uint32)pTabDef->nMaxJob-1; ++i)
		{
			m_pJobSize[i] = 0;
			m_pJobRecSize[i] = 0;
		}
	}
	jobid = 0;
	nHeadSize = RDB_FLG_L(m_pBaseAttr->nFieldCount-1) + 1;
	if(nHeadSize&1) nHeadSize+=1;
	nHeadSize <<= 2;
	m_nRecordSize += nHeadSize;
	nFileRecordSize = 0;
	for(i=0; i<m_pBaseAttr->nFieldCount; ++i)
	{
		CRdbFieldDef* pDefine = m_pBaseAttr->pFieldDefines+i;
		uint32 nRet = AddFieldDefine(pDefine);
		if(nRet)
			return nRet;
		uint32 nCurJob = Abs(pDefine->nJob);
		if(nCurJob != jobid)
		{
			if(jobid)
			{
				for(; jobid<nCurJob; ++jobid)
				{
					m_pJobSize[jobid-1] = i;
					m_pJobRecSize[jobid-1] = nFileRecordSize;
				}
			}
			else
				jobid = nCurJob;
		}
		nFileRecordSize = m_nFileRecordSize;
	}
	m_nFileRecordSize += nHeadSize;
	if(m_nFileRecordSize > RDB_RECORD_MAXSIZE)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::LoadFromDataDict(%s): RDB_RECORD_TOOLONG(%u)", pTabDef->sTableName, m_nFileRecordSize));
		return RDB_RECORD_TOOLONG;
	}
	return RDB_SUCCESS;
}

uint32 CTableDefine::GetFileRecordHeadSize()
{
	return GetFieldFlagCount()<<2;
}

uint32 CTableDefine::GetFileRecordHeadSize(int32 nJob)
{
	return GetFileFieldFlagCount(nJob)<<2;
}

uint32 CTableDefine::AddFieldDefine(CRdbFieldDef* pDefine)
{
	CFieldDefine* pFieldDefine = new CFieldDefine(this);
	if(!pFieldDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddFieldDefine(%s): RDB_LACK_MEMORY", pDefine->sFieldName));
		return RDB_LACK_MEMORY;
	}
	pFieldDefine->m_pBaseAttr = pDefine;
	CFieldDefine** pNewFieldDefineSet = (CFieldDefine**)CMalloc::Realloc(m_pFieldDefineSet, (m_nFieldCount+1)*sizeof(void*));
	if(!pNewFieldDefineSet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddFieldDefine(%s): RDB_LACK_MEMORY", pDefine->sFieldName));
		delete pFieldDefine;
		return RDB_LACK_MEMORY;
	}
	m_pFieldDefineSet = (CBaseFieldDefine**)pNewFieldDefineSet;
	m_pFieldDefineSet[m_nFieldCount] = pFieldDefine;
	if(pDefine->nJob > 0)
	{
		if(m_oNameTable.FindName(pDefine->sFieldName) != (uint32)(-1))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddFieldDefine(%s): RDB_REPEAT_FIELD", pDefine->sFieldName));
			delete pFieldDefine;
			m_pFieldDefineSet[m_nFieldCount] = NULL;
			return RDB_REPEAT_FIELD;
		}
		if(!m_oNameTable.InsertName(pDefine->sFieldName, m_nFieldCount))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddFieldDefine(%s): RDB_LACK_MEMORY", pDefine->sFieldName));
			delete pFieldDefine;
			m_pFieldDefineSet[m_nFieldCount] = NULL;
			return RDB_LACK_MEMORY;
		}
	}
	pFieldDefine->m_nFieldNo = m_nFieldCount++;
	pFieldDefine->m_nOffset = ComputeFieldOffset(m_nRecordSize, pDefine, 1);
	pFieldDefine->m_nDiskOffset = ComputeFieldOffset(m_nFileRecordSize, pDefine, 0);
	pFieldDefine->m_nSize = ComputeFieldRecSize(pDefine, 1);
	pFieldDefine->m_nDiskSize = ComputeFieldRecSize(pDefine, 0);
	m_nRecordSize = pFieldDefine->m_nOffset + pFieldDefine->m_nSize;
	m_nFileRecordSize = pFieldDefine->m_nDiskOffset + pFieldDefine->m_nDiskSize;
	uint32 * pNewFieldList;
	switch(pDefine->nType)
	{
	case RDB_VARCHAR_FIELD:
	case RDB_VARLCHAR_FIELD:
	case RDB_VARRAW_FIELD:
		pNewFieldList = (uint32*)CMalloc::Realloc(m_pVarFieldList, (m_nVarFieldCount+1)*sizeof(uint32));
		if(!pNewFieldList)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::AddFieldDefine(%s): RDB_LACK_MEMORY", pDefine->sFieldName));
			delete pFieldDefine;
			m_pFieldDefineSet[m_nFieldCount] = NULL;
			--m_nFieldCount;
			return RDB_LACK_MEMORY;
		}
		m_pVarFieldList = pNewFieldList;
		m_pVarFieldList[m_nVarFieldCount] = pFieldDefine->m_nFieldNo;
	}
	return RDB_SUCCESS;
}

bool CTableDefine::ModifyFieldDef(uint32 nFieldNo, CRdbFieldDef& oRetFieldDef, CRdbFieldDef& oNewFieldDef, bool bAllowModify)
{
	if(!bAllowModify || ((CFieldDefine*)m_pFieldDefineSet[nFieldNo])->GetReferenceCounter())
	{// only allow to modify default value
		if(oRetFieldDef.nType != oNewFieldDef.nType)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): Refuse", oRetFieldDef.sFieldName));
			return false;
		}
		if(oRetFieldDef.nLen != oNewFieldDef.nLen)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): Refuse", oRetFieldDef.sFieldName));
			return false;
		}
		if(oRetFieldDef.bNotNull != oNewFieldDef.bNotNull)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): Refuse", oRetFieldDef.sFieldName));
			return false;
		}
		if(oRetFieldDef.nRecSize != oNewFieldDef.nRecSize)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): Refuse", oRetFieldDef.sFieldName));
			return false;
		}
		bool bSrcDefNil = (oRetFieldDef.sDefault==NULL);
		bool bDstDefNil = (oNewFieldDef.sDefault==NULL);
		if(bSrcDefNil != bDstDefNil)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): Refuse", oRetFieldDef.sFieldName));
			return false;
		}
		if(!bSrcDefNil && CString::StringCompare(oRetFieldDef.sDefault, oNewFieldDef.sDefault))
		{
			char * sNewDefault = CloneString(GetLogName(), oNewFieldDef.sDefault);
			if(!sNewDefault)
			{
				FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): RDB_LACK_MEMORY", 
					oRetFieldDef.sFieldName));
				return false;
			}
			delete[] oRetFieldDef.sDefault;
			oRetFieldDef.sDefault = sNewDefault;
		}
		return true;
	}
	oRetFieldDef.nType = oNewFieldDef.nType;
	oRetFieldDef.nLen = oNewFieldDef.nLen;
	oRetFieldDef.bNotNull = oNewFieldDef.bNotNull;
	oRetFieldDef.nRecSize = oNewFieldDef.nRecSize;
	if(oNewFieldDef.sDefault)
	{
		char * sNewDefault = CloneString(GetLogName(), oNewFieldDef.sDefault);
		if(!sNewDefault)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::ModifyFieldDef(%s): RDB_LACK_MEMORY", 
				oRetFieldDef.sFieldName));
			return false;
		}
		if(oRetFieldDef.sDefault)
			delete[] oRetFieldDef.sDefault;
		oRetFieldDef.sDefault = sNewDefault;
	}
	else
	{
		if(oRetFieldDef.sDefault)
			delete[] oRetFieldDef.sDefault;
		oRetFieldDef.sDefault = NULL;
	}
	return true;
}

CRdbTableDef* CTableDefine::GetNewTableDefine(CRdbTableDef& oNewDef, bool bAllowModify)
{
	CRdbTableDef& OldDef = m_pBaseAttr[0];
	CRdbTableDef* pRetDef = new CRdbTableDef;
	if(!pRetDef)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s): RDB_LACK_MEMORY", oNewDef.sTableName));
		return NULL;
	}
	pRetDef->sTableName = CloneString(GetLogName(), OldDef.sTableName);
	if(!pRetDef->sTableName)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s): RDB_LACK_MEMORY", oNewDef.sTableName));
		delete pRetDef;
		return NULL;
	}
	pRetDef->nFieldCount = 0;
	pRetDef->pFieldDefines = NULL;
	pRetDef->nMaxJob = OldDef.nMaxJob+1;
	uint32 i;
	for(i=0; i<OldDef.nFieldCount; ++i)
	{
		CRdbFieldDef* pRetFieldDef;
		CRdbFieldDef& oOldFieldDef = OldDef.pFieldDefines[i];
		if(!CloneFieldDef(GetLogName(), pRetFieldDef, &oOldFieldDef, 1))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)@CloneFieldDef(%s) failure", 
				oNewDef.sTableName, oOldFieldDef.sFieldName));
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		CRdbFieldDef* pNewFieldDef = FOCP_NAME::GetFieldDefine(oNewDef, pRetFieldDef->sFieldName);
		if(!pNewFieldDef)
		{
			if(((CFieldDefine*)m_pFieldDefineSet[i])->GetReferenceCounter())
			{
				FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s,%s): field is used", 
					oNewDef.sTableName, oOldFieldDef.sFieldName));
				FreeFieldDefine(pRetFieldDef[0]);
				delete pRetFieldDef;
				FreeTableDefine(pRetDef[0]);
				delete pRetDef;
				return NULL;
			}
			if(oOldFieldDef.nJob > 0)
				pRetFieldDef->nJob = -oOldFieldDef.nJob;
			else
				pRetFieldDef->nJob = oOldFieldDef.nJob;
		}
		else
		{
			pRetFieldDef->nJob = oOldFieldDef.nJob;
			CRdbFieldDef oNewFieldDef = pNewFieldDef[0];
			if(CorrectFieldAttr(GetLogName(), &oNewFieldDef))
			{
				FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)CorrectFieldAttr(%s) failure", 
					oNewDef.sTableName, oOldFieldDef.sFieldName));
				FreeFieldDefine(pRetFieldDef[0]);
				delete pRetFieldDef;
				FreeTableDefine(pRetDef[0]);
				delete pRetDef;
				return NULL;
			}
			if(!ModifyFieldDef(i, pRetFieldDef[0], oNewFieldDef, bAllowModify))
			{
				FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)ModifyFieldDef(%s) failure", 
					oNewDef.sTableName, oOldFieldDef.sFieldName));
				FreeFieldDefine(pRetFieldDef[0]);
				delete pRetFieldDef;
				FreeTableDefine(pRetDef[0]);
				delete pRetDef;
				return NULL;
			}
		}
		CRdbFieldDef* pNewFieldDefines = (CRdbFieldDef*)CMalloc::Realloc(pRetDef->pFieldDefines, 
			(pRetDef->nFieldCount+1)*sizeof(CRdbFieldDef));
		if(!pNewFieldDefines)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s): RDB_LACK_MEMORY", 
				oNewDef.sTableName));
			FreeFieldDefine(pRetFieldDef[0]);
			delete pRetFieldDef;
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		pRetDef->pFieldDefines = pNewFieldDefines;
		pRetDef->pFieldDefines[pRetDef->nFieldCount] = pRetFieldDef[0];
		delete pRetFieldDef;
		++pRetDef->nFieldCount;
	}
	for(i=0; i<oNewDef.nFieldCount; ++i)
	{
		CRdbFieldDef* pRetFieldDef;
		CRdbFieldDef oNewFieldDef = oNewDef.pFieldDefines[i];
		if(InvalidDbName(oNewFieldDef.sFieldName))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)@InvalidFieldName(%s)", 
				oNewDef.sTableName, oNewFieldDef.sFieldName));
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		CRdbFieldDef* pOldFieldDef = FOCP_NAME::GetFieldDefine(OldDef, oNewFieldDef.sFieldName);
		if(pOldFieldDef)
			continue;
		if(oNewFieldDef.bNotNull && !oNewFieldDef.sDefault)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)@InvalidField(%s)", 
				oNewDef.sTableName, oNewFieldDef.sFieldName));
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		if(!CloneFieldDef(GetLogName(), pRetFieldDef, &oNewFieldDef, 1))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s)CloneFieldDef(%s) failure", 
				oNewDef.sTableName, oNewFieldDef.sFieldName));
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		pRetFieldDef->nJob = pRetDef->nMaxJob;
		CRdbFieldDef* pNewFieldDefines = (CRdbFieldDef*)CMalloc::Realloc(pRetDef->pFieldDefines, 
			(pRetDef->nFieldCount+1)*sizeof(CRdbFieldDef));
		if(!pNewFieldDefines)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefine::GetNewTableDefine(%s): RDB_LACK_STORAGE", 
				oNewDef.sTableName));
			FreeFieldDefine(pRetFieldDef[0]);
			delete pRetFieldDef;
			FreeTableDefine(pRetDef[0]);
			delete pRetDef;
			return NULL;
		}
		pRetDef->pFieldDefines = pNewFieldDefines;
		pRetDef->pFieldDefines[pRetDef->nFieldCount] = pRetFieldDef[0];
		delete pRetFieldDef;
		++pRetDef->nFieldCount;
	}
	pRetDef->nStorage = OldDef.nStorage;
	pRetDef->nTableNo = OldDef.nTableNo;
	pRetDef->nStorageAddr = OldDef.nStorageAddr;
	return pRetDef;
}

void CTableDefine::Clear()
{
	if(m_nFieldCount)
	{
		for(uint32 i=0; i<m_nFieldCount; ++i)
			delete m_pFieldDefineSet[i];
		CMalloc::Free(m_pFieldDefineSet);
		m_pFieldDefineSet = NULL;
		m_oNameTable.Clear();
		m_nFieldCount = 0;
	}
	m_nRecordSize = 0;
	m_nFileRecordSize = 0;
	m_nIndexCount = 0;
	if(m_pIndexDefineSet)
	{
		CMalloc::Free(m_pIndexDefineSet);
		m_pIndexDefineSet = NULL;
	}
	if(m_pJobSize)
	{
		delete[] m_pJobSize;
		m_pJobSize = NULL;
	}
	if(m_pJobRecSize)
	{
		delete[] m_pJobRecSize;
		m_pJobRecSize = NULL;
	}
	if(m_pVarFieldList)
	{
		CMalloc::Free(m_pVarFieldList);
		m_pVarFieldList = NULL;
	}
	m_nVarFieldCount = 0;
}

uint32 CTableDefine::ComputeFieldRecSize(CRdbFieldDef* pFieldDefine, uint32 nMemoryMode)
{
	uint32 nSize = pFieldDefine->nLen;
	switch(pFieldDefine->nType)
	{
	case RDB_RAW_FIELD:
		nSize += sizeof(uint32);
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD: case RDB_VARRAW_FIELD:
		if(nMemoryMode)
			nSize = sizeof(void*);
		else
			nSize = sizeof(uint64) + sizeof(uint32);
		break;
	}
	return nSize;
}

uint32 CTableDefine::ComputeFieldOffset(uint32 nOffset, CRdbFieldDef* pFieldDefine, uint32 nMemoryMode)
{
	uint32 nModSize = pFieldDefine->nLen;
	switch(pFieldDefine->nType)
	{
	case RDB_CHAR_FIELD: case RDB_LCHAR_FIELD:
		return nOffset;
	case RDB_RAW_FIELD:
		nModSize = sizeof(uint32);
		break;
	case RDB_VARCHAR_FIELD: case RDB_VARLCHAR_FIELD: case RDB_VARRAW_FIELD:
		if(nMemoryMode)
			nModSize = sizeof(void*);
		else
			nModSize = sizeof(uint64);
		break;
	}

	return MakeAlign(nOffset, nModSize);
}

void* CTableDefine::GetInstance()
{
	return m_pInstance;
}

void CTableDefine::SetInstance(void* pInstance)
{
	m_pInstance = pInstance;
}

uint64 CTableDefine::GetStorageAddr()
{
	return m_pBaseAttr->nStorageAddr;
}

void CTableDefine::SetStorageAddr(uint64 nStorageAddr)
{
	m_pBaseAttr->nStorageAddr = nStorageAddr;
	uint64 nDbAddr = m_pTableSet->GetDataBaseDefine()->GetDataBaseAddr();
	uint64 nTableAddr = GetDbItemAddr(GetLogName(), nDbAddr, m_pBaseAttr->sTableName);
	vcommit(nTableAddr+16, (char*)&nStorageAddr, 8);
}

const char* CTableDefine::GetLogName()
{
	return m_pTableSet->GetLogName();
}

//-----------------------------------------------------
// CCTableDefineSet
//-----------------------------------------------------
CTableDefineSet::CTableDefineSet(CDataBaseDefine* pDataBaseDefine)
{
	m_pDataBaseDefine = pDataBaseDefine;
	m_pTableDefineSet = NULL;
}

CTableDefineSet::~CTableDefineSet()
{
	Clear();
}

CDataBaseDefine* CTableDefineSet::GetDataBaseDefine()
{
	return m_pDataBaseDefine;
}

uint32 CTableDefineSet::GetTableCount()
{
	return m_oNameTable.GetSize();
}

CTableDefine* CTableDefineSet::GetTableDefine(uint32 nTableNo)
{
	if(nTableNo >= m_pDataBaseDefine->GetBaseAttr()->nTableCount)
		return NULL;
	return m_pTableDefineSet[nTableNo];
}

uint32 CTableDefineSet::GetTableNo(char* sTableName)
{
	return m_oNameTable.FindName(sTableName);
}

uint32 CTableDefineSet::LoadFromDataDict(CRdbDef & oDbDef)
{
	char * sDbName = m_pDataBaseDefine->GetDataBaseName();

	uint32 nTableCount = oDbDef.nTableCount;
	m_pTableDefineSet = (CTableDefine**)CMalloc::Malloc(nTableCount*sizeof(void*));
	if(!m_pTableDefineSet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sDbName));
		return RDB_LACK_MEMORY;
	}
	for(uint32 i=0; i<nTableCount; ++i)
		m_pTableDefineSet[i] = NULL;
	CRbTreeNode* idx = oDbDef.oDbItemDefTable.First();
	CRbTreeNode* end = oDbDef.oDbItemDefTable.End();
	for(; idx!=end; idx=oDbDef.oDbItemDefTable.GetNext(idx))
	{
		CRdbItemDef& oTabDef = oDbDef.oDbItemDefTable.GetItem(idx);
		if(oTabDef.nType != RDB_TABLE_DEFINE)
			continue;
		char * sTableName = oTabDef.oItemDef.oTabDef.sTableName;
		CTableDefine* pTabDef = new CTableDefine(this);
		if(!pTabDef)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sTableName));
			return RDB_LACK_MEMORY;
		}
		uint32 nRet = pTabDef->LoadFromDataDict(&oTabDef.oItemDef.oTabDef);
		if(nRet)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::LoadFromDataDict(%s): failure", sTableName));
			delete pTabDef;
			return nRet;
		}
		if(!m_oNameTable.InsertName(sTableName, oTabDef.oItemDef.oTabDef.nTableNo))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sTableName));
			delete pTabDef;
			return RDB_LACK_MEMORY;
		}
		m_pTableDefineSet[oTabDef.oItemDef.oTabDef.nTableNo] = pTabDef;
	}
	return RDB_SUCCESS;
}

uint32 CTableDefineSet::ModifyTable(CRdbTableDef* pDefine, bool bAllowModify)
{
	char* sName = pDefine->sTableName;
	char* sDbName = m_pDataBaseDefine->GetDataBaseName();

	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CTableDefineSet::ModifyTable(%s, %s)", sDbName, sName));

	if(InvalidDbName(sName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_INVALID_NAME", sName));
		return RDB_INVALID_NAME;
	}

	uint32 nTableNo = GetTableNo(sName);
	CTableDefine * pTableDefine = GetTableDefine(nTableNo);
	if(!pTableDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_TABLE_NOTEXIST", sName));
		return RDB_TABLE_NOTEXIST;
	}
	pDefine = pTableDefine->GetNewTableDefine(pDefine[0], bAllowModify);
	if(!pDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_REFUSE_MODIFY", sName));
		return RDB_REFUSE_MODIFY;
	}
	if(pDefine->nFieldCount > RDB_MAX_FIELD_NUM)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_FIELD_TOOMANY", sName));
		FreeTableDefine(pDefine[0]);
		delete pDefine;
		return RDB_FIELD_TOOMANY;
	}
	uint64 nDbAddr = m_pDataBaseDefine->GetDataBaseAddr();
	CRdbDef& oDbDef = m_pDataBaseDefine->GetBaseAttr()[0];
	CRdbItemDef oTabDef;
	oTabDef.nType = RDB_TABLE_DEFINE;
	oTabDef.oItemDef.oTabDef = pDefine[0];
	if(!ModifyDbItemDefine(GetLogName(), nDbAddr, oDbDef, oTabDef))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_LACK_STORAGE", sName));
		FreeTableDefine(pDefine[0]);
		delete pDefine;
		return RDB_LACK_STORAGE;
	}
	delete pDefine;
	CRdbItemDef* pTabDef = GetDbItemDefine(oDbDef, sName);
	pDefine = &pTabDef->oItemDef.oTabDef;
#if VMM_SAFE_ENABLED
	uint32 nRet = pTableDefine->LoadFromDataDict(pDefine);
	if(nRet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::ModifyTable(%s): RDB_LACK_MEMORY", sName));
		Abort();
	}
#else
	pTableDefine->LoadFromDataDict(pDefine);
#endif
	return RDB_SUCCESS;
}

uint32 CTableDefineSet::CreateTable(CRdbTableDef* pDefine)
{
	char* sName = pDefine->sTableName;
	char* sDbName = m_pDataBaseDefine->GetDataBaseName();

	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CTableDefineSet::CreateTable(%s, %s)", sDbName, sName));

	uint32 nRet;
	if(InvalidDbName(sName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s): RDB_INVALID_NAME", sName));
		return RDB_INVALID_NAME;
	}
	uint32 nTableNo = GetTableNo(sName);
	CTableDefine * pTableDefine = GetTableDefine(nTableNo);
	if(pTableDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s): RDB_TABLE_EXIST", sName));
		return RDB_TABLE_EXIST;
	}
	uint32 &nTableCount = m_pDataBaseDefine->GetBaseAttr()->nTableCount;
	for(nTableNo=0; nTableNo<nTableCount; ++nTableNo)
		if(!m_pTableDefineSet[nTableNo])
			break;
	pDefine->nTableNo = nTableNo;
	uint64 nDbAddr = m_pDataBaseDefine->GetDataBaseAddr();
	CRdbDef& oDbDef = m_pDataBaseDefine->GetBaseAttr()[0];
	NewTableDefine(GetLogName(), nDbAddr, oDbDef, pDefine[0]);
	CRdbItemDef* pTabDef = GetDbItemDefine(oDbDef, sName);
	if(!pTabDef)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s): RDB_LACK_STORAGE", sName));
		return RDB_LACK_STORAGE;
	}
	pDefine = &pTabDef->oItemDef.oTabDef;
	pTableDefine = new CTableDefine(this);
	if(!pTableDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s): RDB_LACK_MEMORY", sName));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sName);
		return RDB_LACK_MEMORY;
	}
	nRet = pTableDefine->LoadFromDataDict(pDefine);
	if(nRet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s): %s", sName, GetErrorInfo(nRet)));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sName);
		delete pTableDefine;
		return nRet;
	}
	if(nTableNo >= nTableCount)
	{
		CTableDefine** pNewTableDefineSet = (CTableDefine**)CMalloc::Realloc(m_pTableDefineSet, (nTableCount+1)*sizeof(void*));
		if(!pNewTableDefineSet)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s, %s): RDB_LACK_MEMORY", m_pDataBaseDefine->GetDataBaseName(), sName));
			DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sName);
			delete pTableDefine;
			return RDB_LACK_MEMORY;
		}
		m_pTableDefineSet = pNewTableDefineSet;
		pNewTableDefineSet[nTableNo] = NULL;
	}
	if(!m_oNameTable.InsertName(sName, nTableNo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::CreateTable(%s, %s): RDB_LACK_MEMORY", sName));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sName);
		delete pTableDefine;
		return RDB_LACK_MEMORY;
	}
	m_pTableDefineSet[nTableNo] = pTableDefine;
	if(nTableNo >= nTableCount)
	{
		++nTableCount;
		vcommit(nDbAddr+16, (char*)&nTableCount, sizeof(uint32));
	}
	return RDB_SUCCESS;
}

uint32 CTableDefineSet::RemoveTable(char* sTableName)
{
	char* sDbName = m_pDataBaseDefine->GetDataBaseName();
	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CTableDefineSet::RemoveTable(%s, %s)", sDbName, sTableName));

	if(InvalidDbName(sTableName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::RemoveTable(%s): RDB_INVALID_NAME", sTableName));
		return RDB_INVALID_NAME;
	}
	uint32 i;
	uint32 nTableNo = GetTableNo(sTableName);
	CTableDefine * pTableDefine = GetTableDefine(nTableNo);
	if(!pTableDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::RemoveTable(%s): RDB_TABLE_NOTEXIST", sTableName));
		return RDB_TABLE_NOTEXIST;
	}
	uint32 nIndexCount = pTableDefine->GetIndexCount();
	for(i=0; i<nIndexCount; ++i)
	{
		CIndexDefine* pIndexDefine = pTableDefine->GetIndexDefine(i);
		if(pIndexDefine->GetDetailIndexCount())
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CTableDefineSet::RemoveTable(%s): RDB_DETAIL_EXIST", sTableName));
			return RDB_DETAIL_EXIST;
		}
	}
	while(pTableDefine->GetIndexCount())
	{
		CString oIndexName(pTableDefine->GetIndexDefine(0)->GetBaseAttr()->sIndexName);
		m_pDataBaseDefine->GetIndexDefineSet()->RemoveIndex((char*)oIndexName.GetStr());
	}
	uint64 nDbAddr = m_pDataBaseDefine->GetDataBaseAddr();
	CRdbDef& oDbDef = m_pDataBaseDefine->GetBaseAttr()[0];
	DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sTableName);
	delete pTableDefine;
	m_oNameTable.Erase(sTableName);
	m_pTableDefineSet[nTableNo] = NULL;
	return RDB_SUCCESS;
}

void CTableDefineSet::Clear()
{
	if(m_pTableDefineSet)
	{
		uint32 nTableCount = m_pDataBaseDefine->GetBaseAttr()->nTableCount;
		for(uint32 i=0; i<nTableCount; ++i)
		{
			if(m_pTableDefineSet[i])
				delete m_pTableDefineSet[i];
		}
		CMalloc::Free(m_pTableDefineSet);
		m_pTableDefineSet = NULL;
		m_oNameTable.Clear();
	}
}

const char* CTableDefineSet::GetLogName()
{
	return m_pDataBaseDefine->GetLogName();
}

//-----------------------------------------------------
// CIndexDefine
//-----------------------------------------------------
CIndexDefine::CIndexDefine(CIndexDefineSet* pIndexSet)
{
	m_pIndexSet = pIndexSet;
	m_pInstance = NULL;
	m_pBaseAttr = NULL;
	m_pTableDefine = NULL;
	m_pPrimaryIndex = NULL;
	m_nDetailIndexCount = 0;
	m_nDetailIndexs = NULL;
	m_nFieldCount = 0;
	m_pFields = NULL;
}

CIndexDefine::~CIndexDefine()
{
	Clear();
}

void* CIndexDefine::GetInstance()
{
	return m_pInstance;
}

void CIndexDefine::SetInstance(void* pInstance)
{
	m_pInstance = pInstance;
}

uint64 CIndexDefine::GetStorageAddr()
{
	return m_pBaseAttr->nStorageAddr;
}

void CIndexDefine::SetStorageAddr(uint64 nStorageAddr)
{
	m_pBaseAttr->nStorageAddr = nStorageAddr;
	uint64 nDbAddr = m_pIndexSet->GetDataBaseDefine()->GetDataBaseAddr();
	uint64 nIndexAddr = GetDbItemAddr(GetLogName(), nDbAddr, m_pBaseAttr->sIndexName);
	vcommit(nIndexAddr+32, (char*)&nStorageAddr, 8);
}

CRdbIndexDef* CIndexDefine::GetBaseAttr()
{
	return m_pBaseAttr;
}

uint32 CIndexDefine::GetFieldCount()
{
	return m_nFieldCount;
}

uint32 CIndexDefine::GetFieldNo(uint32 nFieldIdx)
{
	if(nFieldIdx < m_nFieldCount)
		return m_pFields[nFieldIdx];
	return (uint32)(-1);
}

CTableDefine* CIndexDefine::GetTableDefine()
{
	return m_pTableDefine;
}

CIndexDefine* CIndexDefine::GetPrimaryIndex()
{
	return m_pPrimaryIndex;;
}

uint32 CIndexDefine::GetDetailIndexCount()
{
	return m_nDetailIndexCount;
}

CIndexDefine* CIndexDefine::GetDetailIndex(uint32 nDetailNo)
{
	return (nDetailNo<m_nDetailIndexCount)?m_nDetailIndexs[nDetailNo]:NULL;
}

uint32 CIndexDefine::LoadFromDataDict(CRdbIndexDef* pDefine, CTableDefineSet* pTableDefineSet, CIndexDefineSet* pIndexDefineSet)
{
	m_pBaseAttr = pDefine;
	m_pTableDefine = pTableDefineSet->GetTableDefine(pTableDefineSet->GetTableNo(m_pBaseAttr->sTableName));
	m_pPrimaryIndex = NULL;
	if(m_pBaseAttr->sPrimaryIndex)
	{
		m_pPrimaryIndex = pIndexDefineSet->GetIndexDefine(pIndexDefineSet->GetIndexNo(m_pBaseAttr->sPrimaryIndex));
		if(m_pPrimaryIndex->GetBaseAttr()->nQualifier != RDB_UNIQUE_INDEX)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s): RDB_INVALID_FOREIGN_INDEX", pDefine->sIndexName));
			m_pPrimaryIndex = NULL;
			return RDB_INVALID_FOREIGN_INDEX;
		}
		if(m_pPrimaryIndex->AddDetail(this))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s): RDB_LACK_MEMORY", pDefine->sIndexName));
			m_pPrimaryIndex = NULL;
			return RDB_LACK_MEMORY;
		}
	}
	CString oFieldList(m_pBaseAttr->pFieldList);
	char*pShift, *pField = (char*)oFieldList.GetStr();
	while(pField)
	{
		pShift = (char*)CString::CharOfString(pField, ',');
		if(pShift)pShift[0] = 0;
		uint32* pFields = (uint32*)CMalloc::Realloc(m_pFields, (m_nFieldCount+1)<<2);
		if(!pFields)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s): RDB_LACK_MEMORY", pDefine->sIndexName));
			return RDB_LACK_MEMORY;
		}
		m_pFields = pFields;
		uint32 nFieldNo = m_pTableDefine->GetFieldNo(pField);
		CFieldDefine * pFieldDefine = m_pTableDefine->GetFieldDefine(nFieldNo);
		if(!pFieldDefine)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s,%s): RDB_INVALID_INDEXFIELD", pDefine->sIndexName, pField));
			return RDB_INVALID_INDEXFIELD;
		}
		CRdbFieldDef * pFieldBaseDefine = pFieldDefine->GetBaseAttr();
		if(pFieldBaseDefine->nType == RDB_VARCHAR_FIELD || pFieldBaseDefine->nType == RDB_VARRAW_FIELD || pFieldBaseDefine->nType == RDB_VARLCHAR_FIELD)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s,%s): RDB_INVALID_INDEXFIELD", pDefine->sIndexName, pField));
			return RDB_INVALID_INDEXFIELD;
		}
		if(m_pBaseAttr->nArithmetic == RDB_NTREE_INDEX)
		{
			uint32 nFieldType = pFieldDefine->GetBaseAttr()->nType;
			if(nFieldType != RDB_CHAR_FIELD)
			{
				FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s,%s): RDB_INVALID_INDEXFIELD", pDefine->sIndexName, pField));
				return RDB_INVALID_INDEXFIELD;
			}
		}
		m_pFields[m_nFieldCount] = nFieldNo;
		pFieldDefine->Refer();
		++m_nFieldCount;
		pField = pShift;
		if(pField)
			++pField;
		if(m_nFieldCount >= RDB_MAX_INDEX_FIELD_NUM || m_pBaseAttr->nArithmetic == RDB_NTREE_INDEX)
			break;
	}
	if(pField)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::LoadFromDataDict(%s): RDB_FIELD_TOOMANY", pDefine->sIndexName));
		return RDB_INDEXFIELD_TOOMANY;
	}
	return m_pTableDefine->AddIndex(this);
}

void CIndexDefine::Clear()
{
	if(m_nFieldCount)
	{
		for(uint32 i=0; i<m_nFieldCount; ++i)
			m_pTableDefine->GetFieldDefine(m_pFields[i])->Release();
		CMalloc::Free(m_pFields);
		m_pFields = NULL;
		m_nFieldCount = 0;
	}
	if(m_pTableDefine)
	{
		m_pTableDefine->DelIndex(this);
		m_pTableDefine = NULL;
	}
	if(m_pPrimaryIndex)
	{
		m_pPrimaryIndex->DelDetail(this);
		m_pPrimaryIndex = NULL;
	}
	if(m_nDetailIndexCount)
	{
		CMalloc::Free(m_nDetailIndexs);
		m_nDetailIndexs = NULL;
		m_nDetailIndexCount = 0;
	}
	m_pBaseAttr = NULL;
}

uint32 CIndexDefine::AddDetail(CIndexDefine * pIndexDefine)
{
	CIndexDefine** pNewDetailIndexs = (CIndexDefine**)CMalloc::Realloc(m_nDetailIndexs, (m_nDetailIndexCount+1)*sizeof(void*));
	if(!pNewDetailIndexs)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefine::AddDetail: RDB_LACK_MEMORY"));
		return RDB_LACK_MEMORY;
	}
	m_nDetailIndexs = pNewDetailIndexs;	
	m_nDetailIndexs[m_nDetailIndexCount] = pIndexDefine;
	++m_nDetailIndexCount;
	return RDB_SUCCESS;
}

void CIndexDefine::DelDetail(CIndexDefine * pIndexDefine)
{
	uint32 i;
	for(i=0; i<m_nDetailIndexCount; ++i)
	{
		if(m_nDetailIndexs[i] == pIndexDefine)
			break;
	}
	if(i<m_nDetailIndexCount)
	{
		for(; i<m_nDetailIndexCount-1; ++i)
			m_nDetailIndexs[i] = m_nDetailIndexs[i+1];	
		--m_nDetailIndexCount;
		if(!m_nDetailIndexCount)
		{
			CMalloc::Free(m_nDetailIndexs);
			m_nDetailIndexs = NULL;
		}
	}
}

const char* CIndexDefine::GetLogName()
{
	return m_pIndexSet->GetLogName();
}

//-----------------------------------------------------
// CIndexDefineSet
//-----------------------------------------------------
CIndexDefineSet::CIndexDefineSet(CDataBaseDefine* pDataBaseDefine)
{
	m_pDataBaseDefine = pDataBaseDefine;
	m_pIndexDefineSet = NULL;
}

CIndexDefineSet::~CIndexDefineSet()
{
	Clear();
}

CDataBaseDefine* CIndexDefineSet::GetDataBaseDefine()
{
	return m_pDataBaseDefine;
}

uint32 CIndexDefineSet::GetIndexCount()
{
	return m_oNameTable.GetSize();
}

CIndexDefine* CIndexDefineSet::GetIndexDefine(uint32 nIndexNo)
{	
	return (nIndexNo<m_pDataBaseDefine->GetBaseAttr()->nIndexCount)?m_pIndexDefineSet[nIndexNo]:NULL;
}

uint32 CIndexDefineSet::GetIndexNo(char* sIdxName)
{
	return m_oNameTable.FindName(sIdxName);
}

uint32 CIndexDefineSet::LoadFromDataDict(CRdbDef & oDbDef, CTableDefineSet* pTableDefineSet)
{
	char * sDbName = m_pDataBaseDefine->GetDataBaseName();

	uint32 nIndexCount = oDbDef.nIndexCount;
	m_pIndexDefineSet = (CIndexDefine**)CMalloc::Malloc(nIndexCount*sizeof(void*));
	if(!m_pIndexDefineSet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sDbName));
		return RDB_LACK_MEMORY;
	}
	for(uint32 i=0; i<nIndexCount; ++i)
		m_pIndexDefineSet[i] = NULL;
	CRbTreeNode* idx = oDbDef.oDbItemDefTable.First();
	CRbTreeNode* end = oDbDef.oDbItemDefTable.End();
	for(; idx!=end; idx=oDbDef.oDbItemDefTable.GetNext(idx))
	{
		CRdbItemDef& oIdxDef = oDbDef.oDbItemDefTable.GetItem(idx);
		if(oIdxDef.nType != RDB_INDEX_DEFINE)
			continue;
		char * sIndexName = oIdxDef.oItemDef.oIdxDef.sIndexName;
		CIndexDefine * pIdxDef = new CIndexDefine(this);
		if(!pIdxDef)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sIndexName));
			return RDB_LACK_MEMORY;
		}
		uint32 nRet = pIdxDef->LoadFromDataDict(&oIdxDef.oItemDef.oIdxDef, pTableDefineSet, this);
		if(nRet)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::LoadFromDataDict(%s): Failure", sIndexName));
			delete pIdxDef;
			return nRet;
		}
		if(!m_oNameTable.InsertName(sIndexName, oIdxDef.oItemDef.oIdxDef.nIndexNo))
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::LoadFromDataDict(%s): RDB_LACK_MEMORY", sIndexName));
			delete pIdxDef;
			return RDB_LACK_MEMORY;
		}
		m_pIndexDefineSet[oIdxDef.oItemDef.oIdxDef.nIndexNo] = pIdxDef;
	}
	return RDB_SUCCESS;
}

uint32 CIndexDefineSet::CreateIndex(CRdbIndexDef* pDefine, CTableDefineSet* pTableDefineSet)
{
	char * sDbName = m_pDataBaseDefine->GetDataBaseName();
	char* sIndexName = pDefine->sIndexName;

	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CIndexDefineSet::CreateIndex(%s, %s)", sDbName, sIndexName));

	if(InvalidDbName(sIndexName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): RDB_INVALID_NAME", sIndexName));
		return RDB_INVALID_NAME;
	}
	uint32 nIndexNo = GetIndexNo(sIndexName);
	CIndexDefine* pIndexDefine = GetIndexDefine(nIndexNo);
	if(pIndexDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): RDB_INDEX_EXIST", sIndexName));
		return RDB_INDEX_EXIST;
	}
	uint32 &nIndexCount = m_pDataBaseDefine->GetBaseAttr()->nIndexCount;
	for(nIndexNo=0; nIndexNo<nIndexCount; ++nIndexNo)
	{
		if(!m_pIndexDefineSet[nIndexNo])
			break;
	}
	pDefine->nIndexNo = nIndexNo;
	uint64 nDbAddr = m_pDataBaseDefine->GetDataBaseAddr();
	CRdbDef& oDbDef = m_pDataBaseDefine->GetBaseAttr()[0];
	NewIndexDefine(GetLogName(), nDbAddr, oDbDef, pDefine[0]);
	CRdbItemDef* pIdxDef = GetDbItemDefine(oDbDef, sIndexName);
	if(!pIdxDef)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): RDB_LACK_STORAGE", sIndexName));
		return RDB_LACK_STORAGE;
	}
	pDefine = &pIdxDef->oItemDef.oIdxDef;
	pIndexDefine = new CIndexDefine(this);
	if(!pIndexDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): RDB_LACK_MEMORY", sIndexName));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sIndexName);
		return RDB_LACK_MEMORY;
	}
	uint32 nRet = pIndexDefine->LoadFromDataDict(pDefine, pTableDefineSet, this);
	if(nRet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): %s", sIndexName, GetErrorInfo(nRet)));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sIndexName);
		delete pIndexDefine;
		return nRet;
	}
	if(nIndexNo >= nIndexCount)
	{
		CIndexDefine** pNewIndexDefineSet = (CIndexDefine**)CMalloc::Realloc(m_pIndexDefineSet, (nIndexCount+1)*sizeof(void*));
		if(!pNewIndexDefineSet)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s, %s): RDB_LACK_MEMORY", sIndexName));
			DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sIndexName);
			delete pIndexDefine;
			return RDB_LACK_MEMORY;
		}
		m_pIndexDefineSet = pNewIndexDefineSet;
		m_pIndexDefineSet[nIndexNo] = NULL;
	}
	if(!m_oNameTable.InsertName(sIndexName, nIndexNo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::CreateIndex(%s): RDB_LACK_MEMORY", sIndexName));
		DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sIndexName);
		delete pIndexDefine;
		return RDB_LACK_MEMORY;
	}
	m_pIndexDefineSet[nIndexNo] = pIndexDefine;
	if(nIndexNo >= nIndexCount)
	{
		++nIndexCount;
		vcommit(nDbAddr+20, (char*)&nIndexCount, sizeof(uint32));
	}
	return RDB_SUCCESS;
}

uint32 CIndexDefineSet::RemoveIndex(char* sIndexName)
{
	char * sDbName = m_pDataBaseDefine->GetDataBaseName();
	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CIndexDefineSet::RemoveIndex(%s, %s)", sDbName, sIndexName));

	if(InvalidDbName(sIndexName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::RemoveIndex(%s): RDB_INVALID_NAME", sIndexName));
		return RDB_INVALID_NAME;
	}
	uint32 nIndexNo = GetIndexNo(sIndexName);
	CIndexDefine* pIndexDefine = GetIndexDefine(nIndexNo);
	if(!pIndexDefine)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::RemoveIndex(%s): RDB_INDEX_NOTEXIST", sIndexName));
		return RDB_INDEX_NOTEXIST;
	}
	if(pIndexDefine->GetDetailIndexCount())
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CIndexDefineSet::RemoveIndex(%s): RDB_DETAIL_EXIST", sIndexName));
		return RDB_DETAIL_EXIST;
	}
	uint64 nDbAddr = m_pDataBaseDefine->GetDataBaseAddr();
	CRdbDef& oDbDef = m_pDataBaseDefine->GetBaseAttr()[0];
	DelDbItemDefine(GetLogName(), nDbAddr, oDbDef, sIndexName);
	delete pIndexDefine;
	m_oNameTable.Erase(sIndexName);
	m_pIndexDefineSet[nIndexNo] = NULL;
	return RDB_SUCCESS;
}

void CIndexDefineSet::Clear()
{
	if(m_pIndexDefineSet)
	{
		uint32 nIndexCount = m_pDataBaseDefine->GetBaseAttr()->nIndexCount;
		for(uint32 i=0; i<nIndexCount; ++i)
		{
			if(m_pIndexDefineSet[i])
				delete m_pIndexDefineSet[i];
		}
		CMalloc::Free(m_pIndexDefineSet);
		m_pIndexDefineSet = NULL;
		m_oNameTable.Clear();
	}
}

const char* CIndexDefineSet::GetLogName()
{
	return m_pDataBaseDefine->GetLogName();
}

//-----------------------------------------------------
// CDataBaseDefine
//-----------------------------------------------------
CDataBaseDefine::CDataBaseDefine():
m_oTableDefineSet(this),
m_oIndexDefineSet(this)
{
	m_pInstance = NULL;
	m_pBaseAttr = NULL;
	m_nDbAddr = 0;
	m_sLogName[0] = 0;
}

CDataBaseDefine::~CDataBaseDefine()
{
}

void* CDataBaseDefine::GetInstance()
{
	return m_pInstance;
}

void CDataBaseDefine::SetInstance(void* pInstance)
{
	m_pInstance = pInstance;
}

uint64 CDataBaseDefine::GetDataBaseAddr()
{
	return m_nDbAddr;
}

char* CDataBaseDefine::GetDataBaseName()
{
	return m_pBaseAttr->sDbName;
}

const char* CDataBaseDefine::GetLogName()
{
	return m_sLogName;
}

CRdbDef* CDataBaseDefine::GetBaseAttr()
{
	return m_pBaseAttr;
}

CTableDefineSet* CDataBaseDefine::GetTableDefineSet()
{
	return &m_oTableDefineSet;
}

CIndexDefineSet* CDataBaseDefine::GetIndexDefineSet()
{
	return &m_oIndexDefineSet;
}

uint32 CDataBaseDefine::LoadFromDataDict(uint64 nDbAddr, CRdbDef* pDefine)
{
	m_nDbAddr = nDbAddr;
	m_pBaseAttr = pDefine;
	CString::StringCopy(m_sLogName, "Rdb::");
	CString::StringCatenate(m_sLogName, m_pBaseAttr->sDbName);
	uint32 nRet = m_oTableDefineSet.LoadFromDataDict(m_pBaseAttr[0]);
	if(!nRet) nRet = m_oIndexDefineSet.LoadFromDataDict(m_pBaseAttr[0], &m_oTableDefineSet);
	if(nRet)
	{
		m_oIndexDefineSet.~CIndexDefineSet();
		m_oTableDefineSet.~CTableDefineSet();
		new(&m_oTableDefineSet) CTableDefineSet(this);
		new(&m_oIndexDefineSet) CIndexDefineSet(this);
	}
	return nRet;
}

//-----------------------------------------------------
// CDataBaseDefineSet
//-----------------------------------------------------
CDataBaseDefineSet::CDataBaseDefineSet()
{
	m_nDataBaseCount = 0;
	m_nDbsAddr = 0;
	m_pDataBaseDefineSet = NULL;
}

CDataBaseDefineSet::~CDataBaseDefineSet()
{
	Clear();
}

void CDataBaseDefineSet::Clear()
{
	if(m_nDataBaseCount)
	{
		for(uint32 i=0; i<m_nDataBaseCount; ++i)
		{
			if(m_pDataBaseDefineSet[i])
				delete m_pDataBaseDefineSet[i];
		}
		CMalloc::Free(m_pDataBaseDefineSet);
		m_pDataBaseDefineSet = NULL;
		m_nDataBaseCount = 0;
	}
	m_oNameTable.Clear();
	FreeDataBasesDefine(m_oDbsDef);
}

uint32 CDataBaseDefineSet::GetDataBaseCount()
{
	return m_nDataBaseCount;
}

CDataBaseDefine* CDataBaseDefineSet::GetDataBaseDefine(uint32 nDataBaseNo)
{
	if(nDataBaseNo >= m_nDataBaseCount)
		return NULL;
	return m_pDataBaseDefineSet[nDataBaseNo];
}

uint32 CDataBaseDefineSet::GetDataBaseNo(char* sDbName)
{
	return m_oNameTable.FindName(sDbName);
}

uint64 CDataBaseDefineSet::CreateDataBaseSystem()
{
	m_nDbsAddr = AllocDataBasesDefineSpace();
	if(!m_nDbsAddr)
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::CreateDataBaseSystem: RDB_LACK_STORAGE"));
	return m_nDbsAddr;
}

uint64 CDataBaseDefineSet::GetDbsAddr()
{
	return m_nDbsAddr;
}

uint32 CDataBaseDefineSet::LoadFromDataDict(uint64 nDbsAddr)
{
	m_nDbsAddr = nDbsAddr;

	uint32 nRet = LoadDataBasesDefine(m_nDbsAddr, m_oDbsDef);
	if(nRet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::LoadFromDataDict: Failure"));
		return nRet;
	}

	CRbTreeNode* idx = m_oDbsDef.oDbDefTable.First();
	CRbTreeNode* end = m_oDbsDef.oDbDefTable.End();
	m_oDataBaseList = "";
	for(; idx!=end; idx=m_oDbsDef.oDbDefTable.GetNext(idx))
	{
		char * sDataBaseName = (char*)m_oDbsDef.oDbDefTable.GetKey(idx).GetStr();
		nRet = MountDataBase(sDataBaseName);
		if(nRet)
		{
			CRbTreeNode* it = m_oDbsDef.oDbDefTable.First();
			for(; it!=idx; it=m_oDbsDef.oDbDefTable.GetNext(it))
			{
				sDataBaseName = (char*)m_oDbsDef.oDbDefTable.GetKey(it).GetStr();
				UnMountDataBase(sDataBaseName);
			}
			break;
		}
		if(!m_oDataBaseList.Empty())
			m_oDataBaseList += ",";
		m_oDataBaseList += sDataBaseName;
	}

	return nRet;
}

uint32 CDataBaseDefineSet::CreateDataBase(char* sDbName)
{
	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CDataBaseDefineSet::CreateDataBase(%s)", sDbName));

	if(InvalidDbName(sDbName) || !CString::StringCompare(sDbName, "Rdb", false))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::CreateDataBase(%s): RDB_INVALID_NAME", sDbName));
		return RDB_INVALID_NAME;
	}
	if(FOCP_NAME::GetDataBaseDefine(m_oDbsDef, sDbName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::CreateDataBase(%s): RDB_DB_EXIST", sDbName));
		return RDB_DB_EXIST;
	}
	NewDataBaseDefine(m_nDbsAddr, m_oDbsDef, sDbName);
	if(!FOCP_NAME::GetDataBaseDefine(m_oDbsDef, sDbName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::CreateDataBase(%s): RDB_LACK_STORAGE", sDbName));
		return RDB_LACK_STORAGE;
	}
	uint32 nRet = MountDataBase(sDbName);
	if(nRet)
	{
		RemoveDataBase(sDbName);
		return nRet;
	}
	CRbTreeNode* idx = m_oDbsDef.oDbDefTable.First();
	CRbTreeNode* end = m_oDbsDef.oDbDefTable.End();
	m_oDataBaseList = "";
	for(; idx!=end; idx=m_oDbsDef.oDbDefTable.GetNext(idx))
	{
		if(!m_oDataBaseList.Empty())
			m_oDataBaseList += ",";
		m_oDataBaseList += m_oDbsDef.oDbDefTable.GetKey(idx);
	}
	return RDB_SUCCESS;
}

uint32 CDataBaseDefineSet::RemoveDataBase(char* sDbName)
{
	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CDataBaseDefineSet::RemoveDataBase(%s)", sDbName));

	if(!FOCP_NAME::GetDataBaseDefine(m_oDbsDef, sDbName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::RemoveDataBase(%s): RDB_DB_NOTEXIST", sDbName));
		return RDB_DB_NOTEXIST;
	}
	if(m_oNameTable.FindName(sDbName) != (uint32)(-1))
		UnMountDataBase(sDbName);
	DelDataBasesDefine(m_nDbsAddr, m_oDbsDef, sDbName);
	if(FOCP_NAME::GetDataBaseDefine(m_oDbsDef, sDbName))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::RemoveDataBase(%s): RDB_REFUSE_RRdb", sDbName));
		return RDB_REFUSE_RRDB;
	}
	CRbTreeNode* idx = m_oDbsDef.oDbDefTable.First();
	CRbTreeNode* end = m_oDbsDef.oDbDefTable.End();
	m_oDataBaseList = "";
	for(; idx!=end; idx=m_oDbsDef.oDbDefTable.GetNext(idx))
	{
		if(!m_oDataBaseList.Empty())
			m_oDataBaseList += ",";
		m_oDataBaseList += m_oDbsDef.oDbDefTable.GetKey(idx);
	}
	return RDB_SUCCESS;
}

uint32 CDataBaseDefineSet::MountDataBase(char* sDbName)
{
	FocpLogEx(GetLogName(), FOCP_LOG_ERROR,  ("CDataBaseDefineSet::MountDataBase(%s)", sDbName));

	uint64 nDbAddr;
	if(m_oNameTable.FindName(sDbName) != (uint32)(-1))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): RDB_DB_MOUNTED", sDbName));
		return RDB_DB_MOUNTED;
	}
	CRdbDef* pDbDef = FOCP_NAME::GetDataBaseDefine(m_oDbsDef, sDbName);
	if(!pDbDef)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): RDB_DB_NOTEXIST", sDbName));
		return RDB_DB_NOTEXIST;
	}
	nDbAddr = GetDataBaseAddress(m_nDbsAddr, sDbName);
	CDataBaseDefine* pDb = new CDataBaseDefine;
	if(!pDb)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): RDB_LACK_MEMORY", sDbName));
		return RDB_LACK_MEMORY;
	}
	uint32 nRet = pDb->LoadFromDataDict(nDbAddr, pDbDef);
	if(nRet)
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): failure", sDbName));
		delete pDb;
		return nRet;
	}
	uint32 nDbNo, nSize = m_nDataBaseCount;
	for(nDbNo =0; nDbNo < nSize; ++nDbNo)
	{
		if(!m_pDataBaseDefineSet[nDbNo])
			break;
	}
	if(!m_oNameTable.InsertName(sDbName, nDbNo))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): RDB_LACK_MEMORY", sDbName));
		delete pDb;
		return RDB_LACK_MEMORY;
	}
	if(nDbNo >= nSize)
	{
		CDataBaseDefine** pNewSet = (CDataBaseDefine**)CMalloc::Realloc(m_pDataBaseDefineSet, (nSize+1)*sizeof(void*));
		if(!pNewSet)
		{
			FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::MountDataBase(%s): RDB_LACK_MEMORY", sDbName));
			m_oNameTable.Erase(sDbName);
			delete pDb;
			return RDB_LACK_MEMORY;
		}
		m_pDataBaseDefineSet = pNewSet;
		++m_nDataBaseCount;
	}
	m_pDataBaseDefineSet[nDbNo] = pDb;
	return RDB_SUCCESS;
}

uint32 CDataBaseDefineSet::UnMountDataBase(char* sDbName)
{
	uint32 nDbNo = m_oNameTable.FindName(sDbName);
	if(nDbNo == (uint32)(-1))
	{
		FocpLogEx(GetLogName(), FOCP_LOG_ERROR, ("CDataBaseDefineSet::UnMountDataBase(%s): RDB_DB_NOTMOUNTED", sDbName));
		return RDB_DB_NOTMOUNTED;
	}
	CDataBaseDefine* pDbDef = m_pDataBaseDefineSet[nDbNo];
	m_pDataBaseDefineSet[nDbNo] = NULL;
	m_oNameTable.Erase(sDbName);
	delete pDbDef;
	return RDB_SUCCESS;
}

char* CDataBaseDefineSet::GetDataBaseList()
{
	return (char*)m_oDataBaseList.GetStr();
}

FOCP_END();
