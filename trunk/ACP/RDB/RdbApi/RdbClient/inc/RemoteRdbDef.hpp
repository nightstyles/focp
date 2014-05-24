
#include "RdbApi.hpp"

#ifndef _REMOTE_RDB_DEF_h_
#define _REMOTE_RDB_DEF_h_

FOCP_BEGIN();

class CRemoteFieldDefine;
class CRemoteTableDefine;
class CRemoteDataBaseDefine;
class CRemoteDataBaseDefineSet;

enum
{
	RDB_TABLE_DEFINE, 
	RDB_INDEX_DEFINE
};

//-----------------------------------------------------
// CRemoteRdbNameTable
//-----------------------------------------------------
class CRemoteRdbNameTable
{
private:
	CHashMap<CString, uint32, CNameCompare, CNameHashArithmetic> m_oHashTable;

public:
	CRemoteRdbNameTable();
	~CRemoteRdbNameTable();

	uint32 FindName(char* sName);
	bool InsertName(char* sName, uint32 nIndex);
	void Erase(char* sName);
	uint32 GetSize();
	void Clear();
};

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
#define RDB_FLG_L(x) ((x)>>5)
#define RDB_FLG_B(x) (1<<((x)&31))

template<typename T> T RemoteAbs(T& t)
{
	if(t>(T)0)
		return t;
	return -t;
}

bool InvalidRemoteDbName(char* sName);
const char* GetRemoteErrorInfo(uint32 nCode);

//-----------------------------------------------------
// CRemoteFieldDefine
//-----------------------------------------------------
class CRemoteFieldDefine
{
public:
	CRdbFieldDef* m_pBaseAttr;
	uint32 m_nOffset, m_nFieldNo, m_nSize;
	CRemoteTableDefine* m_pTableDefine;

public:
	CRemoteFieldDefine(CRemoteTableDefine* pTableDefine);
	~CRemoteFieldDefine();

	CRdbFieldDef * GetBaseAttr();
	uint32 GetFieldOffset();
	uint32 GetSizeInRecord();
	uint32 GetFieldNo();

	const char* GetLogName();
};

//-----------------------------------------------------
// CRemoteTableDefine
//-----------------------------------------------------
class CRemoteTableDefine
{
public:
	CRdbTableDef* m_pBaseAttr;
	CRemoteDataBaseDefine* m_pDataBaseDefine;
	CRemoteFieldDefine** m_pFieldDefineSet;
	CRemoteRdbNameTable m_oNameTable;
	uint32 m_nRecordSize;
	uint32 m_nFieldCount;
	uint32 m_nIndexCount;

	uint32 m_nVarFieldCount;
	uint32* m_pVarFieldList;

public:
	CRemoteTableDefine(CRemoteDataBaseDefine* pDataBaseDefine);
	~CRemoteTableDefine();

	CRdbTableDef * GetBaseAttr();

	uint32 GetFieldCount();

	bool IsValidField(uint32 nFieldNo);

	uint32 GetFieldFlagCount();
	uint32 GetRecordSize();

	CRemoteFieldDefine* GetFieldDefine(uint32 nFieldNo);
	uint32 GetFieldNo(char* sFieldName);

	static uint32 ComputeFieldRecSize(CRdbFieldDef* pFieldDefine);
	static uint32 ComputeFieldOffset(uint32 nOffset, CRdbFieldDef* pFieldDefine);
	
	uint32 LoadFromStream(CMemoryStream& oStream);

	const char* GetLogName();

private:
	void Clear();
	uint32 AddFieldDefine(CRdbFieldDef* pFieldDefine);
};

//-----------------------------------------------------
// CRemoteDataBaseDefine
//-----------------------------------------------------
class CRemoteDataBaseDefine
{
private:
	void* m_pInstance;
	uint64 m_nDbAddr;
	CRdbDef * m_pBaseAttr;
	CRemoteTableDefine** m_pTableDefineSet;
	CRemoteRdbNameTable m_oNameTable;
	CIndexDefineSet m_oIndexDefineSet;
	char m_sLogName[RDB_NAME_MAXLEN+1];

public:
	CRemoteDataBaseDefine();
	~CRemoteDataBaseDefine();

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


FOCP_END();
