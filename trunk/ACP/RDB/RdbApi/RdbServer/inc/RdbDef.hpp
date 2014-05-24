
#include "RdbApi.hpp"
#include "VmmHash.hpp"
#include "VmmRbTree.hpp"
#include "RdbUtility.hpp"

#ifndef _RDB_DEF_h_
#define _RDB_DEF_h_

FOCP_BEGIN();

class CFieldDefine;
class CTableDefine;
class CTableDefineSet;
class CIndexDefine;
class CIndexDefineSet;
class CDataBaseDefine;
class CDataBaseDefineSet;

//-----------------------------------------------------
// base utility
//-----------------------------------------------------

enum
{
	RDB_TABLE_DEFINE, 
	RDB_INDEX_DEFINE
};

enum
{
	RDO_CREATE_DATABASE, 
	RDO_DROP_DATABASE,
	RDO_INSERT,
	RDO_UPDATE,
	RDO_DELETE,
	RDO_TRUNCATE,
	RDO_CREATE_TABLE,
	RDO_MODIFY_TABLE,
	RDO_DROP_TABLE,
	RDO_CREATE_INDEX,
	RDO_DROP_INDEX
};

struct CDbNameCompare: public CVirtualCompare<CString>
{
	virtual int32 Compare(CString&l, CString& r);
};

//-----------------------------------------------------
// CVirtualString
//-----------------------------------------------------
class CVirtualString
{
private:
	uint64 m_nThis;
	int32 m_bInMemory;

public:
	~CVirtualString();
	CVirtualString(int32 bInMemory = 0);
	CVirtualString(uint64 nThis, int32 bInMemory = 0);
	CVirtualString(CVirtualString& oSrc);
	CVirtualString& operator=(CVirtualString& oSrc);

	uint64 GetThis();
	CString& GetString(CString& oStr);
	bool SetString(CString& oStr);
	int32 IsMemoryObject();
};

//-----------------------------------------------------
// definition item structure extend from RdbApi.h
//-----------------------------------------------------
struct CRdbItemDef
{
	int32 nType;
	union tag_def_t
	{
		CRdbTableDef oTabDef;
		CRdbIndexDef oIdxDef;
	}oItemDef;
};

struct CRdbDef
{
	char* sDbName;
	uint32 nTableCount, nIndexCount;
	CRbMap<CString, CRdbItemDef, CNameCompare> oDbItemDefTable;
};

struct CRdbSysDef
{
	CRbMap<CString, CRdbDef, CNameCompare> oDbDefTable;
};

//-----------------------------------------------------
// definition item structure in file
//-----------------------------------------------------
struct CRdbFieldDefInFile
{
	uint64 nFieldName, nDefault;
	uint32 nType, nLen, bNotNull, nRecSize;
	int32 nJob;
};

struct CRdbTableDefInFile
{
	uint64 nTableName, nFieldDefines, nStorageAddr;
	uint32 nFieldCount;
	uint32 nStorage;
	int32 nMaxJob;
	uint32 nTableNo;
};

struct CRdbIndexDefInFile
{
	uint64 nIndexName, nTableName, nPrimaryIndex, nFieldList, nStorageAddr;
	uint32 nQualifier, nArithmetic, nHashRate;
	uint32 nIndexNo;
};

struct CRdbItemDefInFile
{
	union tag_def_in_file_t
	{
		CRdbTableDefInFile oTabDef;
		CRdbIndexDefInFile oIdxDef;
	}oItemDef;
	int32 nType;
};

struct CRdbDefInFile
{
	uint64 nDbName, nDbDef; // CDbItemDefTableInFile
	uint32 nTableCount, nIndexCount;
};

//-----------------------------------------------------
// CDbItemDefineTableInFile
//-----------------------------------------------------
enum {RDB_DBITEM_DEFINE_SIZE=sizeof(CRdbItemDefInFile)};
struct CGetDbItemKey: public CVirtualGetKey<CRdbItemDefInFile, CString>
{
	char* sLogName;
	virtual CString GetKey(CRdbItemDefInFile& oSrc);
};

struct CDbItemDefineAccess: public CVirtualAccess
{
	char* sLogName;
	CRdbItemDef& Read(CRdbItemDef& oObj, CRdbItemDefInFile& oDbDefInFile);
	bool Write(CRdbItemDef& oObj, CRdbItemDefInFile& oDbDefInFile);
	virtual void Clear(void* pBuf);
};

typedef CVmmRbTree<CRdbItemDefInFile, CString> CDbItemDefineTableInFile;

//-----------------------------------------------------
// CDbDefineAccess
//-----------------------------------------------------
enum {RDB_DB_DEFINE_SIZE=sizeof(CRdbDefInFile)};
struct CGetDbKey: public CVirtualGetKey<CRdbDefInFile, CString>
{
	CString GetKey(CRdbDefInFile& oSrc);
};

struct CDbDefineAccess: public CVirtualAccess
{
	CRdbDef& Read(CRdbDef& oObj, CRdbDefInFile& oDefInFile);
	bool Write(CRdbDef& oObj, CRdbDefInFile& oDefInFile);
	virtual void Clear(void* pBuf);
};

typedef CVmmRbTree<CRdbDefInFile, CString> CDbDefineTableInFile;

//-----------------------------------------------------
// CFieldDefine
//-----------------------------------------------------
class CFieldDefine: public CBaseFieldDefine
{
public:
	uint32 m_nDiskOffset, m_nFieldNo, m_nCounter, m_nDiskSize;
	CTableDefine* m_pTableDefine;

public:
	CFieldDefine(CTableDefine* pTableDefine);
	~CFieldDefine();

	CRdbFieldDef * GetBaseAttr();
	uint32 GetFieldOffset();
	uint32 GetFieldOffsetInFile();

	uint32 GetSizeInRecord();
	uint32 GetSizeInFileRecord();

	uint32 GetFieldNo();

	void Refer();
	void Release();
	uint32 GetReferenceCounter();
	
	const char* GetLogName();
};

//-----------------------------------------------------
// CTableDefine
//-----------------------------------------------------
class CTableDefine: public CBaseTableDefine
{
public:
	void * m_pInstance;
	CTableDefineSet* m_pTableSet;
	CRdbTableDef* m_pBaseAttr;
	CRdbNameTable m_oNameTable;
	uint32* m_pJobSize; // MemFieldCount & FileFieldCount
	uint32* m_pJobRecSize; // 
	uint32 m_nFileRecordSize;
	uint32 m_nIndexCount;
	CIndexDefine** m_pIndexDefineSet;

public:
	CTableDefine(CTableDefineSet* pTableSet);
	virtual ~CTableDefine();

	void* GetInstance();
	void SetInstance(void* pInstance);

	uint64 GetStorageAddr();
	void SetStorageAddr(uint64 nStorageAddr);

	CRdbTableDef * GetBaseAttr();

	uint32 GetFieldCount();
	uint32 GetFileFieldCount(int32 nJob);

	uint32 GetFileFieldFlagCount(int32 nJob);

	uint32 GetFileRecordHeadSize();
	uint32 GetFileRecordHeadSize(int32 nJob);

	uint32 GetRecordSize();
	uint32 GetFileRecordSize();
	uint32 GetFileRecordSize(int32 nJob);

	CFieldDefine* GetFieldDefine(uint32 nFieldNo);
	uint32 GetFieldNo(char* sFieldName);
	static uint32 ComputeFieldRecSize(CRdbFieldDef* pFieldDefine, uint32 nMemoryMode);
	static uint32 ComputeFieldOffset(uint32 nOffset, CRdbFieldDef* pFieldDefine, uint32 nMemoryMode);
	
	uint32 GetIndexCount();
	CIndexDefine* GetIndexDefine(uint32 nIndexNo);
	uint32 AddIndex(CIndexDefine* pIndexDefine);
	void DelIndex(CIndexDefine* pIndexDefine);

	bool IsIndexField(uint32 nFieldNo);
	uint32 GetIndexFieldNo(CIndexDefine* pIdxDef, uint32 nFieldNo);

	uint32 LoadFromDataDict(CRdbTableDef * pTabDef);

	CRdbTableDef* GetNewTableDefine(CRdbTableDef& oNewDef, bool bAllowModify);
	
	virtual const char* GetLogName();

private:
	void Clear();
	uint32 AddFieldDefine(CRdbFieldDef* pFieldDefine);
	bool ModifyFieldDef(uint32 nFieldNo, CRdbFieldDef& oRetFieldDef, CRdbFieldDef& oNewFieldDef, bool bAllowModify);
};

//-----------------------------------------------------
// CTableDefineSet
//-----------------------------------------------------
class CTableDefineSet
{
private:
	CDataBaseDefine * m_pDataBaseDefine;
	CTableDefine** m_pTableDefineSet;
	CRdbNameTable m_oNameTable;

public:
	CTableDefineSet(CDataBaseDefine* pDataBaseDefine);
	~CTableDefineSet();

	CDataBaseDefine* GetDataBaseDefine();
	uint32 GetTableCount();
	CTableDefine* GetTableDefine(uint32 nTableNo);
	uint32 GetTableNo(char* sTableName);

	uint32 LoadFromDataDict(CRdbDef & oDbDef);

	uint32 CreateTable(CRdbTableDef* pDefine);
	uint32 ModifyTable(CRdbTableDef* pDefine, bool bAllowModify);
	uint32 RemoveTable(char* sTableName);
	
	const char* GetLogName();

private:
	void Clear();
};

//-----------------------------------------------------
// CIndexDefine
//-----------------------------------------------------
class CIndexDefine
{
public:
	void* m_pInstance;
	CIndexDefineSet* m_pIndexSet;
	CRdbIndexDef* m_pBaseAttr;
	CTableDefine* m_pTableDefine;
	CIndexDefine * m_pPrimaryIndex;
	uint32 m_nDetailIndexCount;
	CIndexDefine** m_nDetailIndexs;	
	uint32 m_nFieldCount;
	uint32* m_pFields;

public:
	CIndexDefine(CIndexDefineSet* pIndexSet);
	~CIndexDefine();

	void* GetInstance();
	void SetInstance(void* pInstance);

	uint64 GetStorageAddr();
	void SetStorageAddr(uint64 nStorageAddr);

	CRdbIndexDef* GetBaseAttr();
	uint32 GetFieldCount();
	uint32 GetFieldNo(uint32 nFieldIdx);
	CTableDefine* GetTableDefine();
	CIndexDefine* GetPrimaryIndex();
	uint32 GetDetailIndexCount();
	CIndexDefine* GetDetailIndex(uint32 nDetailNo);
	
	uint32 LoadFromDataDict(CRdbIndexDef* pDefine, CTableDefineSet* pTableDefineSet, CIndexDefineSet* pIndexDefineSet);
	
	const char* GetLogName();

private:
	void Clear();
	uint32 AddDetail(CIndexDefine * pIndexDefine);
	void DelDetail(CIndexDefine * pIndexDefine);
};

//-----------------------------------------------------
// CIndexDefineSet
//-----------------------------------------------------
class CIndexDefineSet
{
private:
	CDataBaseDefine* m_pDataBaseDefine;
	CIndexDefine ** m_pIndexDefineSet;
	CRdbNameTable m_oNameTable;
	
public:
	CIndexDefineSet(CDataBaseDefine* pDataBaseDefine);
	~CIndexDefineSet();

	CDataBaseDefine* GetDataBaseDefine();
	uint32 GetIndexCount();
	CIndexDefine* GetIndexDefine(uint32 nIndexNo);
	uint32 GetIndexNo(char* sIdxName);

	uint32 LoadFromDataDict(CRdbDef & oDbDef, CTableDefineSet* pTableDefineSet);

	uint32 CreateIndex(CRdbIndexDef* pDefine, CTableDefineSet* pTableDefineSet);
	uint32 RemoveIndex(char* sIndexName);
	
	const char* GetLogName();

private:
	void Clear();
};

//-----------------------------------------------------
// CDataBaseDefine
//-----------------------------------------------------
class CDataBaseDefine
{
private:
	void* m_pInstance;
	uint64 m_nDbAddr;
	CRdbDef * m_pBaseAttr;
	CTableDefineSet m_oTableDefineSet;
	CIndexDefineSet m_oIndexDefineSet;
	char m_sLogName[RDB_NAME_MAXLEN+1];

public:
	CDataBaseDefine();
	~CDataBaseDefine();

	void* GetInstance();
	void SetInstance(void* pInstance);

	uint64 GetDataBaseAddr();
	char* GetDataBaseName();
	CRdbDef* GetBaseAttr();	
	CTableDefineSet* GetTableDefineSet();
	CIndexDefineSet* GetIndexDefineSet();

	uint32 LoadFromDataDict(uint64 nDbAddr, CRdbDef* pDefine);
	
	const char* GetLogName();
};

//-----------------------------------------------------
// CDataBaseDefineSet
//-----------------------------------------------------
class CDataBaseDefineSet
{
private:
	uint64 m_nDbsAddr;
	CString m_oDataBaseList;
	CRdbSysDef m_oDbsDef;
	uint32 m_nDataBaseCount;
	CDataBaseDefine** m_pDataBaseDefineSet;
	CRdbNameTable m_oNameTable;
	void* m_pLogObject;

public:
	CDataBaseDefineSet();
	~CDataBaseDefineSet();
	
	uint32 GetDataBaseCount();
	CDataBaseDefine* GetDataBaseDefine(uint32 nDataBaseNo);
	uint32 GetDataBaseNo(char* sDbName);

	uint32 LoadFromDataDict(uint64 nDbsAddr);

	uint64 CreateDataBaseSystem();

	uint32 CreateDataBase(char* sDbName);
	uint32 RemoveDataBase(char* sDbName);

	char* GetDataBaseList();

	uint64 GetDbsAddr();

	void Clear();

private:
	uint32 MountDataBase(char* sDbName);
	uint32 UnMountDataBase(char* sDbName);
	char* GetMountedDataBaseList();
};

FOCP_END();

#endif
