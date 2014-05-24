
#include "AFC.hpp"

#ifndef _RDB_API_h_
#define _RDB_API_h_

#if defined(RDBAPI_EXPORTS)
#define RDB_API FOCP_EXPORT
#else
#define RDB_API FOCP_IMPORT
#endif

#if !defined(RDB_SUPPORT_MEMORY_DB) && !defined(RDB_SUPPORT_DISK_DB) && !defined(RDB_SUPPORT_REMOTE_DB)
#error Missing the macro 'RDB_SUPPORT_MEMORY_DB' or 'RDB_SUPPORT_DISK_DB' or 'RDB_SUPPORT_REMOTE_DB'
#endif

FOCP_BEGIN();

enum
{
// max value define
	RDB_NAME_MAXLEN = 64,			// the name's max length
	RDB_MAX_INDEX_FIELD_NUM = 8,	// the max quantity of the index fields
	RDB_MAX_FIELD_NUM = 4096,		// the max valid field quantity of the table
	RDB_MAX_INDEX_NUM = 32,			// the max index quantity of the table

// sql para operator define
	RDB_SQLPARA_OPERATOR_LESS=1,		// <
	RDB_SQLPARA_OPERATOR_EQUAL=2,		// =, ==
	RDB_SQLPARA_OPERATOR_MORE=4,		// >
	RDB_SQLPARA_OPERATOR_LESSEQUAL=3,	// <=
	RDB_SQLPARA_OPERATOR_MOREEQUAL=6,	// >=
	RDB_SQLPARA_OPERATOR_NOTEQUAL=5,	// !=
	RDB_SQLPARA_OPERATOR_INC=8,			// ++
	RDB_SQLPARA_OPERATOR_DEC=16,		// --
	RDB_SQLPARA_OPERATOR_BITAND=32,		// &=
	RDB_SQLPARA_OPERATOR_BITOR=64,		// |=
	RDB_SQLPARA_OPERATOR_BITNOT=128,	// ~=
	RDB_SQLPARA_OPERATOR_BITXOR=256,	// ^=

// table's storage attribute define
	RDB_MEMORY_TABLE = 1,				// memory table
	RDB_FILE_TABLE = 2,					// file table

// field's data type define
	RDB_INT8_FIELD = 1,			// int8
	RDB_INT16_FIELD = 2,		// int16		
	RDB_INT32_FIELD = 3,		// int32
	RDB_INT64_FIELD = 4,		// int64
	RDB_UINT8_FIELD = 5,		// uint8
	RDB_UINT16_FIELD = 6,		// uint16
	RDB_UINT32_FIELD = 7,		// uint32
	RDB_UINT64_FIELD = 8,		// uint64
	RDB_FLOAT_FIELD = 9,		// float
	RDB_DOUBLE_FIELD = 10,		// double
	RDB_CHAR_FIELD = 11,		// string
	RDB_VARCHAR_FIELD = 12,		// variable string
	RDB_RAW_FIELD = 13,			// raw
	RDB_VARRAW_FIELD = 14,		// variable raw
	RDB_LCHAR_FIELD = 15,		// lower case string
	RDB_VARLCHAR_FIELD = 16,	// variable lower case string

// index qualifier attribute define
	RDB_COMMON_INDEX = 0,		// common index (not unique)
	RDB_UNIQUE_INDEX = 1,		// unique index

// index arithmetic attribute define
	RDB_RBTREE_INDEX = 1,		// rb-tree index
	RDB_NTREE_INDEX = 2,		// nt-tree index
	RDB_HASH_INDEX = 3,			// hash index

// rdb error code
	RDB_SUCCESS = 0,			// success code;
	RDB_BACKUP_FAILURE,			// backup database system failure
	RDB_CREATE_REDO_FAILURE,	// create redo system failure
	RDB_WRITERDO_FAILURE,		// write redo failure
	RDB_INVALID_INPUT,			// invalid parameter
	RDB_INVALID_NAME,			// invalid name: dbname, table name, field name, and so on
	RDB_INVALID_STORAGE,		// invalid storage type of the table
	RDB_INVALID_TYPE,			// invalid data type of the field
	RDB_INVALID_SIZE,			// invalid data length of the field
	RDB_LACK_MEMORY,			// temporary lack memory space
	RDB_LACK_STORAGE,			// lack storage space
	RDB_REPEAT_FIELD,			// redefine field
	RDB_DB_EXIST,				// the database exist in the system;
	RDB_DB_NOTEXIST,			// the database not exist in the system
	RDB_DB_BUSY,				// the database is busy
	RDB_DB_NOSELECTED,			// there is not any database selected
	RDB_DB_MOUNTED,				// the database is mounted
	RDB_DB_NOTMOUNTED,			// the database isnot mounted
	RDB_TABLE_EXIST,			// the table exists in the database system
	RDB_TABLE_NOTEXIST,			// not exist the table in the database system
	RDB_RECORD_TOOLONG,			// the record cann't be more than VMM_MAX_MAPSIZE
	RDB_VIEW_EXIST,				// the view exists in the database system
	RDB_VIEW_NOTEXIST,			// the view not exist in the database system
	RDB_INDEX_EXIST,			// the index exists in the database system
	RDB_INDEX_NOTEXIST,			// the index not exists in the database system
	RDB_INDEX_TOOMANY,			// the index is too many
	RDB_DETAIL_EXIST,			// exist the detail table
	RDB_DETAIL_NOTEXIST,		// not exist the detail table
	RDB_FIELD_NOTEXIST,			// the field not exist
	RDB_FIELD_TOOMANY,			// the field is too many
	RDB_FIELD_ISNULL,			// the field is null
	RDB_REFUSE_MODIFY,			// refuse modify define
	RDB_REFUSE_RRDB,			// refuse remove the database
	RDB_INVALID_FOREIGN_INDEX,	// invalid foreign index
	RDB_INDEXFIELD_TOOMANY,		// index field too many
	RDB_INVALID_INDEXFIELD,		// index index field
	RDB_TRAVEL_FAILURE,			// tavel table failure
	RDB_INVALID_COND,			// invalid query condition
	RDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE,	// the new record not exist in the primary table;
	RDB_UNIQUE_INDEX_CONFLICT,	// the new record has been existed in the table;
	RDB_RECORD_NOT_EXIST,		// the record not exist in the table;
	RDB_OPENFILE_FAILURE,		// open file failure
	RDB_MAX_ECODE				// extend error code must be inserted before RDB_MAX_ECODE
};

struct RDB_API CRdbFieldDef
{
	char* sFieldName, *sDefault;
	uint32 nType, nLen, bNotNull, nRecSize;
	int32 nJob;					//internal field
};

struct RDB_API CRdbTableDef
{
	char* sTableName;
	uint32 nFieldCount;
	CRdbFieldDef* pFieldDefines;
	uint32 nStorage;
	int32 nMaxJob;				//internal field
	uint32 nTableNo;			//internal field
	uint64 nStorageAddr;		//internal field
};

struct RDB_API CRdbIndexDef
{
	char *sIndexName, *sTableName, *sPrimaryIndex;
	char* pFieldList;			//with comma separator
	uint32 nQualifier, nArithmetic, nHashRate;
	uint32 nIndexNo;			//internal field
	uint64 nStorageAddr;		//internal field
};

class RDB_API CRdbResult
{
private:
	CRdbResult(const CRdbResult& oSrc);
	CRdbResult& operator=(const CRdbResult& oSrc);
	
protected:
	CRdbResult();
	virtual ~CRdbResult();

public:
	virtual uint32 GetType(register uint32 nFieldNo);

	virtual bool IsNull(register uint32 nFieldNo);
	virtual int8 GetInt8(register uint32 nFieldNo);	
	virtual int16 GetInt16(register uint32 nFieldNo);
	virtual int32 GetInt32(register uint32 nFieldNo);
	virtual int64 GetInt64(register uint32 nFieldNo);
	virtual uint8 GetUInt8(register uint32 nFieldNo);
	virtual uint16 GetUInt16(register uint32 nFieldNo);
	virtual uint32 GetUInt32(register uint32 nFieldNo);
	virtual uint64 GetUInt64(register uint32 nFieldNo);
	virtual float GetFloat(register uint32 nFieldNo);
	virtual double GetDouble(register uint32 nFieldNo);
	virtual char* GetString(register uint32 nFieldNo, register uint32 * pStrLen);
	virtual void* GetRaw(register uint32 nFieldNo, register uint32 &nRawSize);

	virtual uint32 GetStringSize(register uint32 nFieldNo);
	virtual void GetAsString(register uint32 nFieldNo, register char * pString);
};

class RDB_API CRdbResultSet
{
private:
	CRdbResultSet(const CRdbResultSet& oSrc);
	CRdbResultSet& operator=(const CRdbResultSet& oSrc);
	
protected:
	CRdbResultSet();
	virtual ~CRdbResultSet();

public:
	virtual uint32 GetResultCount();
	virtual CRdbResult* GetResult(uint32 nRecordNo);
};

class RDB_API CRdbPara
{
private:
	CRdbPara(const CRdbPara& oSrc);
	CRdbPara& operator=(const CRdbPara& oSrc);
	
protected:
	CRdbPara();
	virtual ~CRdbPara();

public:
	virtual uint32 GetType(register uint32 nFieldNo);
	virtual void SetNull(register uint32 nFieldNo, uint32 nOperator);

	virtual void SetInt8(register uint32 nFieldNo, register int8 v, register uint32 nOperator);
	virtual void SetInt16(register uint32 nFieldNo, register int16 v, register uint32 nOperator);
	virtual void SetInt32(register uint32 nFieldNo, register int32 v, register uint32 nOperator);
	virtual void SetInt64(register uint32 nFieldNo, register int64 v, register uint32 nOperator);
	virtual void SetUInt8(register uint32 nFieldNo, register uint8 v, register uint32 nOperator);
	virtual void SetUInt16(register uint32 nFieldNo, register uint16 v, register uint32 nOperator);
	virtual void SetUInt32(register uint32 nFieldNo, register uint32 v, register uint32 nOperator);
	virtual void SetUInt64(register uint32 nFieldNo, register uint64 v, register uint32 nOperator);
	virtual void SetFloat(register uint32 nFieldNo, register float v, register uint32 nOperator);
	virtual void SetDouble(register uint32 nFieldNo, register double v, register uint32 nOperator);
	virtual void SetString(register uint32 nFieldNo, register const char * v, register uint32 nOperator);
	virtual void SetRaw(register uint32 nFieldNo, register const void * v, register uint32 nLen, register uint32 nOperator);
	virtual void SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator);

	virtual void Clear();
};

class RDB_API CRdbParaSet
{
private:
	CRdbParaSet(const CRdbParaSet& oSrc);
	CRdbParaSet& operator=(const CRdbParaSet& oSrc);
	
protected:
	CRdbParaSet();
	virtual ~CRdbParaSet();

public:
	virtual CRdbPara* AddPara();
	virtual CRdbParaSet* AddParaSet();
	
	virtual uint32 GetParaCount();
	virtual void* GetPara(uint32 nParaIdx, bool &bParaSet);

	virtual void Clear();
};

class RDB_API CRdbFilter
{
private:
	CRdbFilter(const CRdbFilter& oSrc);
	CRdbFilter& operator=(const CRdbFilter& oSrc);
	
protected:
	CRdbFilter();
	virtual ~CRdbFilter();

public:
	virtual void SetField(uint32 nFieldNo);
	virtual void Clear();
};

class RDB_API CRdbAccess
{
private:
	CRdbAccess(const CRdbAccess& oSrc);
	CRdbAccess& operator=(const CRdbAccess& oSrc);
	
protected:
	CRdbAccess();
	virtual ~CRdbAccess();

public:
	virtual uint32 GetFieldNo(const char* sFieldName);
	virtual uint32 GetFieldCount();

	virtual bool IsValidField(uint32 nFieldNo);

	virtual CRdbFilter* GetResultFilter();
	virtual CRdbResultSet* GetResultSet();
	virtual CRdbParaSet* GetQueryPara();
	virtual CRdbPara* GetUpdatePara();
	virtual CRdbPara* GetInsertPara();

	virtual uint32 Insert();
	virtual uint32 Update(uint32 nMaxRollBack);
	virtual uint32 Delete();
	virtual void Truncate();
	virtual uint32 Query(uint32 nResultSetSize, uint32 nSkipCount);
	virtual uint32 Query(uint64 nRowId);
	virtual uint32 Query(uint32 &nCount);
	virtual uint32 Exist(uint32& bExist);
	
	virtual void Release();
};

class RDB_API CRdb
{
private:
	CRdb(const CRdb& oSrc);
	CRdb& operator=(const CRdb& oSrc);
	
protected:
	CRdb();
	virtual ~CRdb();

public:
	virtual CRdbAccess* QueryAccess(const char* sTableName);

	virtual uint32 CreateTable(CRdbTableDef* pTableDefine);
	virtual uint32 ModifyTable(CRdbTableDef* pTableDefine);
	virtual uint32 DropTable(const char* sTableName);

	virtual CRdbTableDef* GetTableDefine(const char* sTableName);
	virtual void ReleaseTableDefine(const char* sTableName);

	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount);
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName);

	virtual uint32 CreateIndex(CRdbIndexDef* pIndexDefine);
	virtual uint32 DropIndex(const char* sIndexName);

	virtual CRdbIndexDef* QueryIndexDefine(const char* sIndexName);
	virtual void ReleaseIndexDefine(const char* sIndexName);

	virtual void QueryTableList(CString &oList);

	virtual void Release();
};

#if defined(RDB_SUPPORT_MEMORY_DB)  || defined(RDB_SUPPORT_DISK_DB)
class RDB_API CRdbSystem
{
private:
	CRdbSystem(const CRdbSystem& oSrc);
	CRdbSystem& operator=(const CRdbSystem& oSrc);	

protected:
	CRdbSystem();
	virtual ~CRdbSystem();

public:
	static CRdbSystem* GetInstance();

	virtual uint32 Startup(const char* sConfigPath, const char* sRdoPath, uint32 nLogFileSize, uint32 nLogFileNum, bool bRedo);
	virtual void Cleanup();

	virtual void Reset(const char* sDataBakPath, const char* sDataBakPath2);
	virtual uint32 Backup(const char* sDataBakPath, const char* sDataBakPath2);

	virtual CRdb* QueryDataBase(const char* sDbName);

	virtual uint32 CreateDataBase(const char* sDbName);
	virtual uint32 DestroyDataBase(const char* sDbName);

	virtual void QueryDataBaseList(CString& oDbList);
};
#endif

#if defined(RDB_SUPPORT_REMOTE_DB)
class RDB_API CRemoteRdbSystem
{
private:
	CRemoteRdbSystem(const CRemoteRdbSystem& oSrc);
	CRemoteRdbSystem& operator=(const CRemoteRdbSystem& oSrc);

protected:
	CRemoteRdbSystem();

public:
	virtual ~CRemoteRdbSystem();

	static CRemoteRdbSystem* NewInstance();

	virtual uint32 Startup(const char* sDbHost);
	virtual void Cleanup();

	virtual uint32 GetStorageMode();

	virtual CRdb* QueryDataBase(const char* sDbName);

	virtual uint32 CreateDataBase(const char* sDbName);
	virtual uint32 DestroyDataBase(const char* sDbName);

	virtual void QueryDataBaseList(CString& oDbList);
};
#endif

FOCP_END();

#endif
