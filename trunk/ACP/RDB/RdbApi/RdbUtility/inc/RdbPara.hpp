
#include "RdbUtility.hpp"

#ifndef _RDB_PARA_HPP_
#define _RDB_PARA_HPP_

FOCP_BEGIN();

class CField;
class CRecord;
class CRdbField;
class CRdbRecord;
class CRecordSet;
class CSqlFilter;
class CSqlParameter;
class CSqlParameterSet;

//-----------------------------------------------------------------------------
// CField
//-----------------------------------------------------------------------------
class CField
{
public:
	uint32 m_nType;
	uint32 m_nSize;
	uint32* m_pFlag;
	uint32 m_nFlag;
	uint32 m_nBit;
	void* m_pData;
	CBaseTableDefine* m_pTabDef;

public:
	virtual ~CField();
	CField();
	CField(const CField& oSrc);
	CField& operator=(const CField& oSrc);

	void Initialize(uint32 nType, uint32 nSize, 
		void* pRecordData, uint32 nFieldNo, uint32 nOffset, CBaseTableDefine* pTabDef);

	uint32 GetStringSize();
	void SetNull();

	char* GetString(uint32 * pStrLen);
	void* GetRaw(uint32 &nRawSize);
	void GetAsString(char * pString);
	void SetString(register const char * v);
	void SetRaw(register const void * v, register uint32 nLen);
	void SetFromString(register const char* v);
	void SetFromField(register CField* pValue);

	int32 Compare(CField* pValue);
	uint32 GetHashValue();

	bool IsCoincident(CField* pValue, uint32 nOpr);

	const char* GetLogName();

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CRecord
//-----------------------------------------------------------------------------
class CRecord: public CRdbResult
{
public:
	uint64 m_nRowId;
	CBaseTableDefine* m_pTabDef;
	uint32 m_nFieldCount;
	uint8* m_pRecordData;//flag+data
	CField* m_pFieldTable;

public:
	CRecord(CBaseTableDefine * pTabDef);
	virtual ~CRecord();

	CField* GetField(register uint32 nFieldNo);

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
	void SetFromString(register uint32 nFieldNo, register const char* v);
	void SetFromField(register uint32 nFieldNo, register CRecord* pRecord);

	void Clear();

	uint64 GetRowId();
	void SetRowId(register uint64 nRowId);

	const char* GetLogName();

	bool Write(CMemoryStream & oStream, CSqlFilter &oFilter);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CRecordSet
//-----------------------------------------------------------------------------
class CRecordSet: public CRdbResultSet
{
public:
	CBaseTableDefine * m_pTabDef;
	uint32 m_nSetSize;
	uint32 m_nRecordCount;
	CRecord** m_pRecordSet;

public:
	CRecordSet(CBaseTableDefine * pTabDef);
	virtual ~CRecordSet();

	void PreAlloc();

	CRecord* AllocRecord();
	void PopRecord();

	uint32 GetRecordSetSize();
	void SetRecordSetSize(uint32 nSize);

	uint32 GetRecordCount();
	CRecord* GetRecord(uint32 nRecNo);

	void Clear();

	const char* GetLogName();

	virtual uint32 GetResultCount();
	virtual CRdbResult* GetResult(uint32 nRecordNo);
};

//-----------------------------------------------------------------------------
// CSqlFilter
//-----------------------------------------------------------------------------
class CSqlFilter: public CRdbFilter
{
public:
	CBaseTableDefine * m_pTabDef;
	uint32 m_nFieldCount, m_nFlgCount, m_nParaCount;
	uint32* m_pParaTable;
	uint32* m_pFlag;

public:
	CSqlFilter(CBaseTableDefine* pTabDef);
	virtual ~CSqlFilter();

	virtual void SetField(uint32 nFieldNo);
	virtual void Clear();
	
	void SetAllField();

	bool IsSetField(uint32 nFieldNo);

	uint32 GetParaCount();
	uint32 GetPara(uint32 nParaIdx);

	const char* GetLogName();

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);
};

//-----------------------------------------------------------------------------
// CSqlParameter
//-----------------------------------------------------------------------------
struct CCondField
{
	CField* pField;
	uint32 nOperator;
};
class CSqlParameter: public CRdbPara
{
public:
	struct CParaItem
	{
		uint32 nFieldNo;
		uint32 nOperator;
	};
	CRecord m_oRecord;
	CBaseTableDefine * m_pTabDef;
	uint32 m_nFieldCount, m_nFlgCount, m_nParaCount;
	CParaItem* m_pParaTable;
	uint32* m_pFlag;

public:
	CSqlParameter(CBaseTableDefine * pTabDef);
	virtual ~CSqlParameter();

	CRecord& GetRecord();

	bool IsSetField(uint32 nFieldNo);
	uint32 SetField(uint32 nFieldNo, uint32 nOperator);

	uint32 GetParaCount();
	uint32 GetPara(uint32 nParaIdx, uint32 *pOperator, uint32 * pFieldNo);

	void GetFilter(CSqlFilter &oFilter);

	bool IsCoincident(uint32 nFieldNo, CField* pField);
	bool IsCoincident(CRecord& oRecord);

	uint32 GetHashValue(uint32 nFieldNo, uint32& nHashValue, uint32 &nHashCount);

	CField* GetCondField(uint32 nFieldNo, uint32& nOperator);
	bool GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen);

	const char* GetLogName();

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);
	
	// for CRdbPara
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

//-----------------------------------------------------------------------------
// CSqlParameterSet
//-----------------------------------------------------------------------------
class CSqlParameterSet: public CRdbParaSet
{
public:
	struct CParaItem
	{
		bool bAtom;
		void * pPara;
	};
	CBaseTableDefine* m_pTabDef;
	uint32 m_nParaCount;
	CParaItem * m_pPataSet;

public:
	CSqlParameterSet(CBaseTableDefine* pTabDef);
	virtual ~CSqlParameterSet();

	virtual CRdbPara* AddPara();
	virtual CRdbParaSet* AddParaSet();

	virtual uint32 GetParaCount();
	virtual void* GetPara(uint32 nParaIdx, bool &bParaSet);
	virtual void Clear();

	void GetFilter(CSqlFilter &oFilter);

	bool IsCoincident(uint32 nFieldNo, CField* pField);
	bool IsCoincident(CRecord& oRecord);

	uint32 GetHashValue(uint32 nFieldNo, uint32 &nHashValue, uint32 &nHashCount);

	bool GetCondField(uint32 nFieldNo, CCondField& oMinField, CCondField& oMaxField);

	bool GetPrefixCondField(uint32 nFieldNo, char* &sStr, uint32 * pStrLen);

	const char* GetLogName();

	bool Write(CMemoryStream & oStream);
	bool Read(CMemoryStream & oStream);

private:
	bool GetCondField(CCondField &oCondField, CCondField& oMinField, CCondField& oMaxField);
};

FOCP_END();

#endif
