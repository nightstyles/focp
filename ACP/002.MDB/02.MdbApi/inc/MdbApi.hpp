
#include "AFC.hpp"

#ifndef _FOCP_MDBAPI_HPP_
#define _FOCP_MDBAPI_HPP_

#if defined(MCI_EXPORTS)
#define MCI_API FOCP_EXPORT
#else
#define MCI_API FOCP_IMPORT
#endif

FOCP_BEGIN();

enum
{
// max value define
	MDB_NAME_MAXLEN = 64,			// the name's max length
	MDB_MAX_INDEX_FIELD_NUM = 8,	// the max quantity of the index fields
	MDB_MAX_FIELD_NUM = 4096,		// the max valid field quantity of the table
	MDB_MAX_INDEX_NUM = 32,			// the max index quantity of the table

// sql para operator define
	MDB_SQLPARA_OPERATOR_LESS=1,		// <
	MDB_SQLPARA_OPERATOR_EQUAL=2,		// =, ==
	MDB_SQLPARA_OPERATOR_MORE=4,		// >
	MDB_SQLPARA_OPERATOR_LESSEQUAL=3,	// <=
	MDB_SQLPARA_OPERATOR_MOREEQUAL=6,	// >=
	MDB_SQLPARA_OPERATOR_NOTEQUAL=5,	// !=

	MDB_SQLPARA_OPERATOR_ADD=20,		// +=
	MDB_SQLPARA_OPERATOR_SUB=21,		// -=
	MDB_SQLPARA_OPERATOR_MUL=22,		// *=
	MDB_SQLPARA_OPERATOR_DIV=23,		// /=
	MDB_SQLPARA_OPERATOR_MOD=24,		// %=
	MDB_SQLPARA_OPERATOR_BITAND=30,		// &=
	MDB_SQLPARA_OPERATOR_BITOR=31,		// |=
	MDB_SQLPARA_OPERATOR_BITXOR=32,		// ^=
	MDB_SQLPARA_OPERATOR_BITNOT=33,		// =~

// field's data type define
	MDB_INT8_FIELD = 1,			// int8
	MDB_INT16_FIELD = 2,		// int16
	MDB_INT32_FIELD = 3,		// int32
	MDB_INT64_FIELD = 4,		// int64
	MDB_UINT8_FIELD = 5,		// uint8
	MDB_UINT16_FIELD = 6,		// uint16
	MDB_UINT32_FIELD = 7,		// uint32
	MDB_UINT64_FIELD = 8,		// uint64
	MDB_FLOAT_FIELD = 9,		// float
	MDB_DOUBLE_FIELD = 10,		// double
	MDB_CHAR_FIELD = 11,		// string
	MDB_LCHAR_FIELD = 12,		// lower case string
	MDB_RAW_FIELD = 13,			// raw
	MDB_VARCHAR_FIELD = 14,		// variable length string
	MDB_VARLCHAR_FIELD = 15,		// variable length lower case string
	MDB_VARRAW_FIELD = 16,			// variable length raw
	MDB_DATE_FIELD = 17,
	MDB_TIME_FIELD = 18,
	MDB_DATETIME_FIELD = 19,

// index qualifier attribute define
	MDB_COMMON_INDEX = 0,		// common index (not unique)
	MDB_UNIQUE_INDEX = 1,		// unique index

// index arithmetic attribute define
	MDB_RANGE_INDEX = 1,
	MDB_EQUAL_INDEX = 2,
//	MDB_PREFIX_INDEX = 4,

	MDB_RBTREE_INDEX = 0x80000000 | MDB_RANGE_INDEX | MDB_EQUAL_INDEX, // rb-tree index
	MDB_HASH_INDEX = 0x40000000 | MDB_EQUAL_INDEX, // hash index
//	MDB_NTREE_INDEX = 0x20000000 | MDB_PREFIX_INDEX, // nt-tree index

// Caller定义
	MDB_APP_CALLER = 1,
	MDB_REPLICATE_CALLER = 2,
	MDB_TRANSFER_CALLER = 4,
	MDB_LOAD_CALLER = 8,
	MDB_STO_CALLER = 16
};

struct CMdbFieldAttr;
struct CMdbFieldDef
{
	char* sFieldName, *sDefault;
	uint32 nType, nLen, bNotNull;
	CMdbFieldAttr* pExtendAttr;//内部字段
};

struct CMdbTableAttr;
struct CMdbTableDef
{
	char* sTableName;
	CMdbFieldDef* pFieldDefines;
	uint32 nMaxRecordNum, nFieldCount;
	CMdbTableAttr* pExtendAttr;//内部字段;
};

struct CMdbIndexAttr;
struct CMdbIndexDef
{
	char *sIndexName, *sTableName, *sPrimaryIndex;
	char *pFieldList;	//with comma separator
	uint32 nQualifier, nArithmetic, nHashRate;
	CMdbIndexAttr* pExtendAttr;//内部字段;
};

MCI_API bool InvalidDbName(char* sName);

MCI_API const char* GetMdbError(uint32 nCode);

struct MCI_API CMdbData
{
	virtual uint32 GetType() = 0;
	virtual bool IsNull() = 0;
	virtual int8 GetInt8() = 0;
	virtual int16 GetInt16() = 0;
	virtual int32 GetInt32() = 0;
	virtual int64 GetInt64() = 0;
	virtual uint8 GetUInt8() = 0;
	virtual uint16 GetUInt16() = 0;
	virtual uint32 GetUInt32() = 0;
	virtual uint64 GetUInt64() = 0;
	virtual float GetFloat() = 0;
	virtual double GetDouble() = 0;
	virtual char* GetString(register uint32 * pStrLen) = 0;
	virtual void* GetRaw(register uint32 &nRawSize) = 0;
	virtual CDate GetDate() = 0;
	virtual CTime GetTime() = 0;
	virtual CDateTime GetDateTime() = 0;
};

struct MCI_API CMdbResult
{
	virtual uint32 GetType(register uint32 nFieldNo) = 0;

	virtual bool IsNull(register uint32 nFieldNo) = 0;
	virtual int8 GetInt8(register uint32 nFieldNo) = 0;
	virtual int16 GetInt16(register uint32 nFieldNo) = 0;
	virtual int32 GetInt32(register uint32 nFieldNo) = 0;
	virtual int64 GetInt64(register uint32 nFieldNo) = 0;
	virtual uint8 GetUInt8(register uint32 nFieldNo) = 0;
	virtual uint16 GetUInt16(register uint32 nFieldNo) = 0;
	virtual uint32 GetUInt32(register uint32 nFieldNo) = 0;
	virtual uint64 GetUInt64(register uint32 nFieldNo) = 0;
	virtual float GetFloat(register uint32 nFieldNo) = 0;
	virtual double GetDouble(register uint32 nFieldNo) = 0;
	virtual char* GetString(register uint32 nFieldNo, register uint32 * pStrLen) = 0;
	virtual void* GetRaw(register uint32 nFieldNo, register uint32 &nRawSize) = 0;
	virtual CDate GetDate(register uint32 nFieldNo) = 0;
	virtual CTime GetTime(register uint32 nFieldNo) = 0;
	virtual CDateTime GetDateTime(register uint32 nFieldNo) = 0;

	virtual uint32 GetStringSize(register uint32 nFieldNo) = 0;
	virtual void GetAsString(register uint32 nFieldNo, register char * pString) = 0;

	void GetNumber(void* pVal, register uint32 nFieldNo);
};

struct MCI_API CMdbResultSet
{
	virtual uint32 GetResultCount() = 0;
	virtual CMdbResult* GetResult(uint32 nRecordNo) = 0;
};

struct MCI_API CMdbPara
{
	virtual uint32 GetType(register uint32 nFieldNo) = 0;
	virtual void SetNull(register uint32 nFieldNo, uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;

	virtual void SetInt8(register uint32 nFieldNo, register int8 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetInt16(register uint32 nFieldNo, register int16 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetInt32(register uint32 nFieldNo, register int32 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetInt64(register uint32 nFieldNo, register int64 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetUInt8(register uint32 nFieldNo, register uint8 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetUInt16(register uint32 nFieldNo, register uint16 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetUInt32(register uint32 nFieldNo, register uint32 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetUInt64(register uint32 nFieldNo, register uint64 v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetFloat(register uint32 nFieldNo, register float v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetDouble(register uint32 nFieldNo, register double v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetString(register uint32 nFieldNo, register const char * v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetRaw(register uint32 nFieldNo, register const void * v, register uint32 nLen, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetDate(register uint32 nFieldNo, const CDate& oDate, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetTime(register uint32 nFieldNo, const CTime& oTime, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetDateTime(register uint32 nFieldNo, const CDateTime& oDateTime, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator=MDB_SQLPARA_OPERATOR_EQUAL) = 0;
	virtual void Clear() = 0;

	void SetNumber(register uint32 nFieldNo, register void* pVal, register uint32 nOperator);
};

/*
 only support three level structure:
	1. The top level: is the entrance of SQL where.
	2. The first sub-level: must be CMdbParaSet, the relation between CMdbParaSet is 'OR'.
	3. The second sub-level: must be CMdbPara, the relation between CMdbPara is 'AND'.
*/
struct MCI_API CMdbParaSet
{
	virtual CMdbPara* AddPara() = 0;
	virtual CMdbParaSet* AddParaSet() = 0;

	virtual uint32 GetParaCount() = 0;
	virtual void* GetPara(uint32 nParaIdx, bool &bParaSet) = 0;

	virtual void Clear() = 0;
};

struct MCI_API CMdbFilter
{
	virtual void SetField(uint32 nFieldNo) = 0;
	virtual void Clear() = 0;
};

class CMdb;

struct MCI_API CMdbAccess
{
	virtual uint32 GetFieldNo(const char* sFieldName) = 0;
	virtual uint32 GetFieldCount() = 0;

	virtual CMdbFilter* GetResultFilter() = 0;
	virtual CMdbResultSet* GetResultSet() = 0;
	virtual CMdbParaSet* GetQueryPara() = 0;
	virtual CMdbPara* GetUpdatePara() = 0;
	virtual CMdbPara* GetInsertPara() = 0;

	virtual CMdbTableDef* GetTableDefine() = 0;

	virtual bool SetOrderBy(const char* sIndexName, bool bAsc=true) = 0;

	virtual uint32 Insert(uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Update(uint32* pModifiedCount=NULL, uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Delete(uint32* pDeletedCount=NULL, uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Truncate(uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Query(uint32 nPageSize, uint32 nSkipCount, uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Count(uint32 &nCount, uint32 nCaller=MDB_APP_CALLER) = 0;
	virtual uint32 Exist(uint32& bExist, uint32 nCaller=MDB_APP_CALLER) = 0;

	virtual void Clear() = 0;
	virtual void Release() = 0;
	virtual const char* GetTableName() = 0;

	virtual CMdb* GetMdb() = 0;
};

typedef bool (*FBeforeTrigger)(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
typedef void (*FAfterTrigger)(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
typedef void (*FMdbTrigger)(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
typedef bool (*FQueryTrigger)(CMdbAccess* pAccess, uint32 nCaller, void* pContext, uint32 nMode, uint32 nPageSize, uint32 &nSkipCount);
typedef bool (*FStorageTrigger)(CMdbAccess* pAccess, CMdbResult* pRecord);

struct MCI_API CMdbLocalInterface
{
	virtual uint32 CreateTable(CMdbTableDef* pTableDefine) = 0;
	virtual uint32 CreateIndex(CMdbIndexDef* pIndexDefine) = 0;

	virtual void RegInsertBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegInsertAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegDeleteBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegDeleteAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegUpdateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegUpdateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegTruncateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegTruncateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegQueryTrigger(const char* sTableName, FQueryTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;

	virtual void RegInsertDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegDeleteDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegUpdateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
	virtual void RegTruncateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask=MDB_APP_CALLER) = 0;
};

class MCI_API CMdb
{
	friend void MCI_API RemoveAllMdb();

private:
	CMdb(const CMdb& oSrc);
	CMdb& operator=(const CMdb& oSrc);

private:
	const char* m_sDbName;

protected:
	virtual ~CMdb();

public:
	CMdb(const char* sName);

	static CMdb* GetMdb(const char* sDbName);
	static const char* GetDbList();
	static void RemoveAll();
	static void CreateCppCode();
	const char* GetDbName();

	virtual CMdbAccess* CreateAccess(const char* sTableName) = 0;
	virtual CMdbTableDef* GetTableDefine(const char* sTableName) = 0;
	virtual const char* GetTableList() = 0;

	virtual CMdbIndexDef* GetIndexDefine(const char* sIndexName) = 0;
	virtual const char* GetIndexList() = 0;
	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount) = 0;
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName) = 0;

	virtual CMdbLocalInterface* GetLocalInterface();

	CMdbAccess* QueryAccess(const char* sTableName);
};

FOCP_EXPORT void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger);
FOCP_EXPORT void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger);

#define MDB_SYSDB_NAME "SysDb"

//-------------------------------------------------------------------
//interface for auto-code
//-------------------------------------------------------------------
struct MCI_API CMdbRawVal
{
	void* pVal;
	uint32 nLen;

	CMdbRawVal(void* v, uint32 nLen);
	CMdbRawVal(const CMdbRawVal& o);
	CMdbRawVal& operator=(const CMdbRawVal& o);
};

template<typename TData, uint32 nCol> class CMdbUpdateVal
{
public:
	template<typename TValueType> class CFloatVal
	{
	private:
		CMdbPara* m_pPara;
		CFloatVal(const CFloatVal<TValueType> &o);
		CFloatVal<TValueType>& operator=(const CFloatVal<TValueType> &o);

	public:
		inline CFloatVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CFloatVal<TValueType>& operator=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CFloatVal<TValueType>& operator+=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CFloatVal<TValueType>& operator-=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline CFloatVal<TValueType>& operator++()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CFloatVal<TValueType>& operator--()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline CFloatVal<TValueType>& operator*=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MUL);
			return *this;
		}

		inline CFloatVal<TValueType>& operator/=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_DIV);
			return *this;
		}

		inline CFloatVal<TValueType>& operator%=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MOD);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	template<typename TValueType> class CIntVal
	{
	private:
		CMdbPara* m_pPara;
		CIntVal(const CIntVal<TValueType> &o);
		CIntVal<TValueType>& operator=(const CIntVal<TValueType> &o);

	public:
		inline CIntVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CIntVal<TValueType>& operator=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CIntVal<TValueType>& operator+=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CIntVal<TValueType>& operator-=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline CIntVal<TValueType>& operator++()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CIntVal<TValueType>& operator--()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline CIntVal<TValueType>& operator*=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MUL);
			return *this;
		}

		inline CIntVal<TValueType>& operator/=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_DIV);
			return *this;
		}

		inline CIntVal<TValueType>& operator%=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MOD);
			return *this;
		}

		inline CIntVal<TValueType>& operator&=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_BITAND);
			return *this;
		}

		inline CIntVal<TValueType>& operator|=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_BITOR);
			return *this;
		}

		inline CIntVal<TValueType>& operator^=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_BITXOR);
			return *this;
		}

		inline CIntVal<TValueType>& operator~()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_BITNOT);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	class CRawVal
	{
	private:
		CMdbPara* m_pPara;
		CRawVal(const CRawVal &o);
		CRawVal& operator=(const CRawVal &o);

	public:
		inline CRawVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CRawVal& operator=(const CMdbRawVal& oVal)
		{
			m_pPara->SetRaw(nCol, oVal.pVal, oVal.nLen, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CRawVal& operator+=(const CMdbRawVal& oVal)
		{
			m_pPara->SetRaw(nCol, oVal.pVal, oVal.nLen, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	class CStringVal
	{
	private:
		CMdbPara* m_pPara;
		CStringVal(const CStringVal &o);
		CStringVal& operator=(const CStringVal &o);

	public:
		inline CStringVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CStringVal& operator=(const char* pStr)
		{
			m_pPara->SetString(nCol, pStr, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CStringVal& operator=(const CString& oStr)
		{
			m_pPara->SetString(nCol, oStr.GetStr(), MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CStringVal& operator+=(const char* pStr)
		{
			m_pPara->SetString(nCol, pStr, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CStringVal& operator+=(const CString& oStr)
		{
			m_pPara->SetString(nCol, oStr.GetStr(), MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	template<typename TValueType> class CTimeValue
	{
	private:
		CMdbPara* m_pPara;
		CTimeValue(const CTimeValue<TValueType> &o);
		CTimeValue<TValueType>& operator=(const CTimeValue<TValueType> &o);

	public:
		inline CTimeValue()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CTimeValue<TValueType>& operator=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CTimeValue<TValueType>& operator+=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CTimeValue<TValueType>& operator-=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline CTimeValue<TValueType>& operator++()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_ADD);
			return *this;
		}

		inline CTimeValue<TValueType>& operator--()
		{
			TValueType v(1);
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_SUB);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	template<typename TValueType FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<int8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<int16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<int32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<int64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<uint8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<uint16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<uint32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CIntVal<uint64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<float FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CFloatVal<float> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<double FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CFloatVal<double> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDate> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDateTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CString FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CStringVal TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<void FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CRawVal TValue;
	};

	typedef typename CHelper<TData>::TValue TValue;
};

template<typename TData, uint32 nCol> class CMdbInsertVal
{
public:
	template<typename TValueType> class CNumberVal
	{
	private:
		CMdbPara* m_pPara;
		CNumberVal(const CNumberVal<TValueType> &o);
		CNumberVal<TValueType>& operator=(const CNumberVal<TValueType> &o);

	public:
		inline CNumberVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CNumberVal<TValueType>& operator=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	class CRawVal
	{
	private:
		CMdbPara* m_pPara;
		CRawVal(const CRawVal &o);
		CRawVal& operator=(const CRawVal &o);

	public:
		inline CRawVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CRawVal& operator=(const CMdbRawVal &oVal)
		{
			m_pPara->SetRaw(nCol, oVal.pVal, oVal.nLen, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	class CStringVal
	{
	private:
		CMdbPara* m_pPara;
		CStringVal(const CStringVal &o);
		CStringVal& operator=(const CStringVal &o);

	public:
		inline CStringVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CStringVal& operator=(const char * pStr)
		{
			m_pPara->SetString(nCol, pStr, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CStringVal& operator=(const CString &oStr)
		{
			m_pPara->SetString(nCol, oStr.GetStr(), MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	template<typename TValueType> class CTimeValue
	{
	private:
		CMdbPara* m_pPara;
		CTimeValue(const CTimeValue<TValueType> &o);
		CTimeValue<TValueType>& operator=(const CTimeValue<TValueType> &o);

	public:
		inline CTimeValue()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CTimeValue<TValueType>& operator=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline void SetNull()
		{
			m_pPara->SetNull(nCol);
		}
	};

	template<typename TValueType FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<float FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<float> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<double FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<double> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDate> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDateTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CString FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CStringVal TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<void FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CRawVal TValue;
	};

	typedef typename CHelper<TData>::TValue TValue;
};

template<typename TData, uint32 nCol> class CMdbCondVal
{
public:
	template<typename TValueType> class CNumberVal
	{
	private:
		CMdbPara* m_pPara;
		CNumberVal(const CNumberVal<TValueType> &o);
		CNumberVal<TValueType>& operator=(const CNumberVal<TValueType> &o);

	public:
		inline CNumberVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CNumberVal<TValueType>& operator==(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CNumberVal<TValueType>& operator<(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_LESS);
			return *this;
		}

		inline CNumberVal<TValueType>& operator>(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MORE);
			return *this;
		}

		inline CNumberVal<TValueType>& operator<=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_LESSEQUAL);
			return *this;
		}

		inline CNumberVal<TValueType>& operator>=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_MOREEQUAL);
			return *this;
		}

		inline CNumberVal<TValueType>& operator!=(TValueType v)
		{
			m_pPara->SetNumber(nCol, &v, MDB_SQLPARA_OPERATOR_NOTEQUAL);
			return *this;
		}

		inline void SetNull(bool bIsNull=true)
		{
			m_pPara->SetNull(nCol, bIsNull?MDB_SQLPARA_OPERATOR_EQUAL:MDB_SQLPARA_OPERATOR_NOTEQUAL);
		}
	};

	class CRawVal
	{
	private:
		CMdbPara* m_pPara;
		CRawVal(const CRawVal &o);
		CRawVal& operator=(const CRawVal &o);

	public:
		inline CRawVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CRawVal& operator==(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CRawVal& operator<(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_LESS);
			return *this;
		}

		inline CRawVal& operator>(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_MORE);
			return *this;
		}

		inline CRawVal& operator<=(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_LESSEQUAL);
			return *this;
		}

		inline CRawVal& operator>=(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_MOREEQUAL);
			return *this;
		}

		inline CRawVal& operator!=(const CMdbRawVal& v)
		{
			m_pPara->SetRaw(nCol, v.pVal, v.nLen, MDB_SQLPARA_OPERATOR_NOTEQUAL);
			return *this;
		}

		inline void SetNull(bool bIsNull=true)
		{
			m_pPara->SetNull(nCol, bIsNull?MDB_SQLPARA_OPERATOR_EQUAL:MDB_SQLPARA_OPERATOR_NOTEQUAL);
		}
	};

	class CStringVal
	{
	private:
		CMdbPara* m_pPara;
		CStringVal(const CStringVal &o);
		CStringVal& operator=(const CStringVal &o);

	public:
		inline CStringVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CStringVal& operator==(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CStringVal& operator==(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CStringVal& operator<(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_LESS);
			return *this;
		}

		inline CStringVal& operator<(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_LESS);
			return *this;
		}

		inline CStringVal& operator>(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_MORE);
			return *this;
		}

		inline CStringVal& operator>(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_MORE);
			return *this;
		}

		inline CStringVal& operator<=(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_LESSEQUAL);
			return *this;
		}

		inline CStringVal& operator<=(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_LESSEQUAL);
			return *this;
		}

		inline CStringVal& operator>=(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_MOREEQUAL);
			return *this;
		}

		inline CStringVal& operator>=(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_MOREEQUAL);
			return *this;
		}

		inline CStringVal& operator!=(const char* v)
		{
			m_pPara->SetString(nCol, v, MDB_SQLPARA_OPERATOR_NOTEQUAL);
			return *this;
		}

		inline CStringVal& operator!=(const CString& v)
		{
			m_pPara->SetString(nCol, v.GetStr(), MDB_SQLPARA_OPERATOR_NOTEQUAL);
			return *this;
		}

		inline void SetNull(bool bIsNull=true)
		{
			m_pPara->SetNull(nCol, bIsNull?MDB_SQLPARA_OPERATOR_EQUAL:MDB_SQLPARA_OPERATOR_NOTEQUAL);
		}
	};

	template<typename TValueType> class CTimeValue
	{
	private:
		CMdbPara* m_pPara;
		CTimeValue(const CTimeValue<TValueType> &o);
		CTimeValue<TValueType>& operator=(const CTimeValue<TValueType> &o);

	public:
		inline CTimeValue()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbPara* pPara)
		{
			m_pPara = pPara;
		}

		inline CTimeValue<TValueType>& operator==(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_EQUAL);
			return *this;
		}

		inline CTimeValue<TValueType>& operator<(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_LESS);
			return *this;
		}

		inline CTimeValue<TValueType>& operator>(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_MORE);
			return *this;
		}

		inline CTimeValue<TValueType>& operator<=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_LESSEQUAL);
			return *this;
		}

		inline CTimeValue<TValueType>& operator>=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_MOREEQUAL);
			return *this;
		}

		inline CTimeValue<TValueType>& operator!=(const TValueType& v)
		{
			m_pPara->SetNumber(nCol, (void*)&v, MDB_SQLPARA_OPERATOR_NOTEQUAL);
			return *this;
		}

		inline void SetNull(bool bIsNull=true)
		{
			m_pPara->SetNull(nCol, bIsNull?MDB_SQLPARA_OPERATOR_EQUAL:MDB_SQLPARA_OPERATOR_NOTEQUAL);
		}
	};

	template<typename TValueType FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<float FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<float> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<double FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<double> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDate> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CTimeValue<CDateTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CString FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CStringVal TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<void FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CRawVal TValue;
	};

	typedef typename CHelper<TData>::TValue TValue;
};

template<typename TData, uint32 nCol> class CMdbResultVal
{
public:
	template<typename TValueType> class CNumberVal
	{
	private:
		CMdbResult* m_pPara;
		CNumberVal(const CNumberVal<TValueType> &o);
		CNumberVal<TValueType>& operator=(const CNumberVal<TValueType> &o);

	public:
		inline CNumberVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbResult* pPara)
		{
			m_pPara = pPara;
		}

		inline operator TValueType()
		{
			TValueType v;
			m_pPara->GetNumber(&v, nCol);
			return v;
		}

		inline bool IsNull()
		{
			return m_pPara->IsNull(nCol);
		}
	};

	class CRawVal
	{
	private:
		CMdbResult* m_pPara;
		CRawVal(const CRawVal &o);
		CRawVal& operator=(const CRawVal &o);

	public:
		inline CRawVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbResult* pPara)
		{
			m_pPara = pPara;
		}

		inline operator CMdbRawVal()
		{
			uint32 n;
			void* p = m_pPara->GetRaw(nCol, n);
			return CMdbRawVal(p, n);
		}

		inline void* Get(uint32 &nLen)
		{
			return m_pPara->GetRaw(nCol, nLen);
		}

		inline bool IsNull()
		{
			return m_pPara->IsNull(nCol);
		}
	};

	class CStringVal
	{
	private:
		CMdbResult* m_pPara;
		CStringVal(const CStringVal &o);
		CStringVal& operator=(const CStringVal &o);

	public:
		inline CStringVal()
		{
			m_pPara = NULL;
		}

		inline void Bind(CMdbResult* pPara)
		{
			m_pPara = pPara;
		}

		inline const char* Get(uint32 &nLen)
		{
			return m_pPara->GetString(nCol, &nLen);
		}

		inline operator CString()
		{
			uint32 n;
			char* s = m_pPara->GetString(nCol, &n);
			return CString(s, n);
		}

		inline bool IsNull()
		{
			return m_pPara->IsNull(nCol);
		}
	};

	template<typename TValueType FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<int64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<int64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint8 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint8> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint16 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint16> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint32 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint32> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<uint64 FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<uint64> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<float FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<float> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<double FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<double> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDate FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<CDate> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<CTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CDateTime FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CNumberVal<CDateTime> TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<CString FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CStringVal TValue;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<void FOCP_FAKE_TYPE_ARG(S)>
	{
		typedef CRawVal TValue;
	};

	typedef typename CHelper<TData>::TValue TValue;
};

template<uint32 nCol> class CMdbSelect
{
private:
	CMdbFilter* m_pFilter;
	CMdbSelect(const CMdbSelect<nCol> &o);
	CMdbSelect<nCol>& operator=(const CMdbSelect<nCol> &o);

public:
	inline CMdbSelect()
	{
		m_pFilter = NULL;
	}

	inline void Bind(CMdbFilter* pFilter)
	{
		m_pFilter = pFilter;
	}

	inline void Select()
	{
		m_pFilter->SetField(nCol);
	}
};

FOCP_END();

#endif
