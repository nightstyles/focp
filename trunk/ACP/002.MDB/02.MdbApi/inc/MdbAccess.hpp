
#include "MdbApi.hpp"

#ifndef _MDB_ACCESS_HPP_
#define _MDB_ACCESS_HPP_

FOCP_BEGIN();

enum
{
	MDB_ATTR_EXTSIZE = 10,

	MDB_EXTEND_PLUGIN_REPLICATOR = 0,
	MDB_EXTEND_PLUGIN_STORAGER = 1,
};

struct CMdbFieldAttr
{
	uint32 nOffset, nSize;
	void* pExtAttr[MDB_ATTR_EXTSIZE];//供后续扩展控制用，默认为0
};

struct CMdbTableAttr
{
	uint32 nRecordSize;
	uint32 nVarFieldCount;
	uint32* pVarFieldList;
	uint32 nIndexCount;
	void* pExtAttr[MDB_ATTR_EXTSIZE];//供后续扩展控制用，默认为0
	CMdbIndexDef** pIndexDefineSet;
	CMdbIndexDef* pPrimeIndex;
	CMdbIndexDef* pForeignIndex;
	void* pInstance;
};

struct CMdbIndexAttr
{
	CMdbTableDef* pTableDefine;
	CMdbIndexDef * pPrimaryIndex;
	uint32 nDetailIndexCount;
	CMdbIndexDef** pDetailIndexs;
	uint32 nFieldCount;
	uint32* pFields;
	void* pInstance;
	void* pExtAttr[MDB_ATTR_EXTSIZE];//供后续扩展控制用，默认为0
};

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
#define MDB_RECORD_MAXSIZE 4096

#define MDB_FLG_L(x) ((x)>>5)
#define MDB_FLG_B(x) (1<<((x)&31))

#define GetAtomX(type, pField) (*(type*)pField->m_pData)
#define GetAtomX0(type) (*(type*)m_pData)
#define FillField() ((*m_pFlag) |= m_nBit)
#define FillFieldA(pField) ((*pField->m_pFlag) |= pField->m_nBit)
#define IS_NULL() (((*m_pFlag) & m_nBit)?false:true)
#define IS_NULL_A(pField) (((*pField->m_pFlag) & pField->m_nBit)?false:true)
#define IS_SET_FIELD(pObj, nField) ((nField<pObj->m_pTabDef->nFieldCount) && (pObj->m_pFlag[MDB_FLG_L(nField)] & MDB_FLG_B(nField)))
#define IS_SET_FIELD0(oObj, nField) ((nField<oObj.m_pTabDef->nFieldCount) && (oObj.m_pFlag[MDB_FLG_L(nField)] & MDB_FLG_B(nField)))

struct CMdbRaw;

class CMdbField;
class CMdbRecord;

class CMdbRecordSet;
class CMdbSqlFilter;
class CMdbSqlPara;
class CMdbSqlParaSet;

//-----------------------------------------------------
// CMdbRaw
//-----------------------------------------------------
struct CMdbRaw
{
	uint32 nSize;
	char sData[1];
};

uint32 MCI_API GetFieldFlagCount(CMdbTableDef* pTabDef);

//-----------------------------------------------------------------------------
// CMdbField
//-----------------------------------------------------------------------------
class MCI_API CMdbField: public CMdbData
{
public:
	uint32 m_nType;
	uint32 m_nSize;
	uint32* m_pFlag;
	uint32 m_nFlag;
	uint32 m_nBit;
	void* m_pData;

public:
	~CMdbField();
	CMdbField();
	CMdbField(const CMdbField& oSrc);
	CMdbField& operator=(const CMdbField& oSrc);

	void Initialize(uint32 nType, uint32 nSize,
					void* pRecordData, uint32 nFieldNo, uint32 nOffset);

	uint32 GetStringSize();
	void SetNull();

	void GetAsString(char * pString);
	void SetString(register const char * v);
	void SetRaw(register const void * v, register uint32 nLen);
	void SetFromString(register const char* v);
	void SetFromField(register CMdbField* pValue, uint32 nOp=MDB_SQLPARA_OPERATOR_EQUAL);

	int32 Compare(CMdbField* pValue);
	uint32 GetHashValue();

	bool IsCoincident(CMdbField* pValue, uint32 nOpr);

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);

	virtual uint32 GetType();
	virtual bool IsNull();
	virtual int8 GetInt8();
	virtual int16 GetInt16();
	virtual int32 GetInt32();
	virtual int64 GetInt64();
	virtual uint8 GetUInt8();
	virtual uint16 GetUInt16();
	virtual uint32 GetUInt32();
	virtual uint64 GetUInt64();
	virtual float GetFloat();
	virtual double GetDouble();
	virtual char* GetString(register uint32 * pStrLen);
	virtual void* GetRaw(register uint32 &nRawSize);
	virtual CDate GetDate();
	virtual CTime GetTime();
	virtual CDateTime GetDateTime();

	void* GetData(uint32 &nSize);
};

//-----------------------------------------------------------------------------
// CMdbRecord
//-----------------------------------------------------------------------------
class MCI_API CMdbRecord: public CMdbResult
{
public:
	CMdbTableDef* m_pTabDef;
	uint8* m_pRecordData;//flag+data
	CMdbField* m_pFieldTable;
	bool m_bOwner;

public:
	CMdbRecord(CMdbTableDef * pTabDef, uint8* pRecordData=NULL);
	virtual ~CMdbRecord();

	CMdbField* GetField(register uint32 nFieldNo);
	void GetField(register uint32 nFieldNo, CMdbField& oField);

	virtual uint32 GetType(register uint32 nFieldNo);
	virtual bool IsNull(register uint32 nFieldNo);
	void SetNull(register uint32 nFieldNo);

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
	virtual CDate GetDate(register uint32 nFieldNo);
	virtual CTime GetTime(register uint32 nFieldNo);
	virtual CDateTime GetDateTime(register uint32 nFieldNo);

	virtual uint32 GetStringSize(register uint32 nFieldNo);
	virtual void GetAsString(register uint32 nFieldNo, register char * pString);

	void SetInt8(register uint32 nFieldNo, register int8 v);
	void SetInt16(register uint32 nFieldNo, register int16 v);
	void SetInt32(register uint32 nFieldNo, register int32 v);
	void SetInt64(register uint32 nFieldNo, register int64 v);
	void SetUInt8(register uint32 nFieldNo, register uint8 v);
	void SetUInt16(register uint32 nFieldNo, register uint16 v);
	void SetUInt32(register uint32 nFieldNo, register uint32 v);
	void SetUInt64(register uint32 nFieldNo, register uint64 v);
	void SetFloat(register uint32 nFieldNo, register float v);
	void SetDouble(register uint32 nFieldNo, register double v);
	void SetString(register uint32 nFieldNo, register const char * v);
	void SetRaw(register uint32 nFieldNo, register const void * v, uint32 nLen);
	void SetDate(register uint32 nFieldNo, const CDate& oDate);
	void SetTime(register uint32 nFieldNo, const CTime& oTime);
	void SetDateTime(register uint32 nFieldNo, const CDateTime& oDateTime);
	void SetFromString(register uint32 nFieldNo, register const char* v);

	void CopyFrom(CMdbSqlFilter &oFilter, register CMdbRecord* pRecord);

	int32 Compare(register CMdbRecord* pRecord, uint32 nFieldCount, uint32 *pFields);

	void Clear();

	bool Write(CMemoryStream & oStream, CMdbSqlFilter &oFilter);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CMdbRecordSet
//-----------------------------------------------------------------------------
class MCI_API CMdbRecordSet: public CMdbResultSet
{
public:
	CMdbTableDef * m_pTabDef;
	uint32 m_nSetSize;
	uint32 m_nRecordCount;
	CMdbRecord** m_pRecordSet;

public:
	CMdbRecordSet(CMdbTableDef * pTabDef);
	virtual ~CMdbRecordSet();

	void PreAlloc();

	CMdbRecord* AllocRecord();
	void PopRecord();

	uint32 GetRecordSetSize();
	void SetRecordSetSize(uint32 nSize);

	uint32 GetRecordCount();
	CMdbRecord* GetRecord(uint32 nRecNo);

	void Clear();

	virtual uint32 GetResultCount();
	virtual CMdbResult* GetResult(uint32 nRecordNo);

	void Sort(bool bAsc, uint32 nFieldCount, uint32* pFields, uint32 nSegCount=0, uint32* pSeg=NULL);

	bool Write(CMemoryStream & oStream, CMdbSqlFilter &oFilter);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CMdbSqlFilter
//-----------------------------------------------------------------------------
class MCI_API CMdbSqlFilter: public CMdbFilter
{
public:
	CMdbTableDef * m_pTabDef;
	uint32 m_nFlgCount, m_nParaCount;
	uint32* m_pParaTable;
	uint32* m_pFlag;

public:
	CMdbSqlFilter(CMdbTableDef* pTabDef);
	virtual ~CMdbSqlFilter();

	virtual void SetField(uint32 nFieldNo);
	virtual void Clear();

	void SetAllField();

	bool IsSetField(uint32 nFieldNo);

	uint32 GetParaCount();
	uint32 GetPara(uint32 nParaIdx);

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CMdbSqlPara
//-----------------------------------------------------------------------------
struct CMdbCondField
{
	CMdbField* pField;
	uint32 nOperator;
};

class MCI_API CMdbSqlPara: public CMdbPara
{
public:
	struct CParaItem
	{
		uint32 nFieldNo;
		uint32 nOperator;
	};
	CMdbRecord m_oRecord;
	CMdbTableDef * m_pTabDef;
	uint32 m_nFlgCount, m_nParaCount;
	CParaItem* m_pParaTable;
	uint32* m_pFlag;
	bool m_bError;

public:
	CMdbSqlPara(CMdbTableDef * pTabDef);
	virtual ~CMdbSqlPara();

	CMdbRecord& GetRecord();

	bool HaveError();

	bool IsSetField(uint32 nFieldNo);
	uint32 SetField(uint32 nFieldNo, uint32 nOperator);

	void SetAllField(uint32 nOperator);

	void SetFrom(CMdbSqlPara* pPara);

	uint32 GetParaCount();
	uint32 GetPara(uint32 nParaIdx, uint32 *pOperator, uint32 * pFieldNo);

	void GetFilter(CMdbSqlFilter &oFilter);

	bool IsCoincident(uint32 nFieldNo, CMdbField* pField);
	bool IsCoincident(CMdbRecord& oRecord);

	uint32 GetHashValue(uint32 nFieldNo, uint32& nHashValue, uint32 &nHashCount);

	CMdbField* GetCondField(uint32 nFieldNo, uint32& nOperator);
	bool GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen);

	bool Write(CMemoryStream & oStream);
	bool WriteForRep(CMemoryStream & oStream, uint32 &nParaCount);

	bool Read(CMemoryStream & oStream);
	bool ReadFilterAndRecord(CMemoryStream & oStream);

	// for CMdbPara
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
	virtual void SetDate(register uint32 nFieldNo, const CDate& oDate, register uint32 nOperator);
	virtual void SetTime(register uint32 nFieldNo, const CTime& oTime, register uint32 nOperator);
	virtual void SetDateTime(register uint32 nFieldNo, const CDateTime& oDateTime, register uint32 nOperator);
	virtual void SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator);

	void SetFrom(register uint32 nFieldNo, register CMdbField* pField, register uint32 nOperator);
	void SetFrom(CMdbIndexDef* pIdxDef, CMdbRecord* pRecord);
	bool IncludeIndexField(CMdbIndexDef* pIdxDef);

	virtual void Clear();
};

//-----------------------------------------------------------------------------
// CMdbSqlParaSet
//-----------------------------------------------------------------------------
class MCI_API CMdbSqlParaSet: public CMdbParaSet
{
public:
	struct CParaItem
	{
		bool bAtom;
		void * pPara;
	};
	CMdbTableDef* m_pTabDef;
	uint32 m_nParaCount;
	CParaItem * m_pPataSet;
	bool m_bAnd;

public:
	CMdbSqlParaSet(CMdbTableDef* pTabDef, bool bAnd=false);
	virtual ~CMdbSqlParaSet();

	bool HaveError();

	void SetFrom(CMdbSqlParaSet* pParaSet);

	virtual CMdbPara* AddPara();
	virtual CMdbParaSet* AddParaSet();

	virtual uint32 GetParaCount();
	virtual void* GetPara(uint32 nParaIdx, bool &bParaSet);
	virtual void Clear();

	bool IsEmpty();

	void GetFilter(CMdbSqlFilter &oFilter);

	bool IsCoincident(uint32 nFieldNo, CMdbField* pField);
	bool IsCoincident(CMdbRecord& oRecord);

	uint32 GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount);

	bool GetCondField(uint32 nFieldNo, CMdbCondField& oMinField, CMdbCondField& oMaxField);

	bool GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen);

	bool Insert2Update(CMdbSqlPara* pInsert, CMdbSqlPara* pSet);

	bool SetFromWhere(const char* sCond);

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);

private:
	bool GetCondField(CMdbCondField &oCondField, CMdbCondField& oMinField, CMdbCondField& oMaxField);
	//for SetFromWhere
	void AddCond(uint32 nFieldNo, uint32 nOp, const CString& oVal);
	uint32 GetValue(const char* &pStr, CString &oValue);
	uint32 GetOperator(const char* &pStr, uint32 &nOp);
	uint32 GetIdentifier(const char* &pStr, CString &oIdentifier);
	void SkipWhiteSpace(const char* &pStr);
	uint32 GetFieldNo(const char* sFieldName);
};

struct MCI_API CMdbTableAccess: public CMdbAccess
{
	CMdbTableDef* m_pTabDef;
	CMdbSqlParaSet m_oIdxAttr;
	CMdbSqlPara m_oInsertAttr;
	CMdbSqlPara m_oSetAttr;
	CMdbRecordSet m_oGetAttr;
	CMdbSqlFilter m_oFilter;
	CMdbIndexDef* m_pOrderByIndex;
	bool m_bAscOrder;
	CMdb * m_pDb;
	CMdbTableAccess* m_pNext;

	CMdbTableAccess(CMdb* pDb, CMdbTableDef* pTabDef);
	virtual ~CMdbTableAccess();

	virtual uint32 GetFieldNo(const char* sFieldName);
	virtual uint32 GetFieldCount();

	virtual CMdbFilter* GetResultFilter();
	virtual CMdbResultSet* GetResultSet();
	virtual CMdbParaSet* GetQueryPara();
	virtual CMdbPara* GetUpdatePara();
	virtual CMdbPara* GetInsertPara();

	virtual bool SetOrderBy(const char* sIndexName, bool bAsc);

	virtual CMdbTableDef* GetTableDefine();

	virtual void Clear();
	virtual void Release();
	virtual const char* GetTableName();
	virtual CMdb* GetMdb();
};

class MCI_API CMdbExtPlugIn
{
private:
	uint32 m_nPlugIn;

public:
	CMdbExtPlugIn(uint32 nPlugInType);
	virtual ~CMdbExtPlugIn();

	virtual void OnFree(void* pAttr, uint32 nAttrType/*0=字段属性，1表属性，2索引属性*/) = 0;

	static CMdbExtPlugIn* GetPlugIn(uint32 nPlugInType);

private:
	CMdbExtPlugIn(const CMdbExtPlugIn&);
	CMdbExtPlugIn& operator=(const CMdbExtPlugIn&);
};

class MCI_API CMdbFirstNode
{
public:
	CMdbFirstNode();
	virtual ~CMdbFirstNode();

	static CMdbFirstNode* GetInstance();
	virtual bool OnFirstNode(uint32 nDomain);
	virtual void OnOtherNode(uint32 nDomain);

private:
	CMdbFirstNode(const CMdbFirstNode&);
	CMdbFirstNode& operator=(const CMdbFirstNode&);
};

FOCP_END();

#endif
