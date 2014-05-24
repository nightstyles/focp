
#include "RdbApi.hpp"

#ifndef _RDB_UTILITY_HPP_
#define _RDB_UTILITY_HPP_

FOCP_BEGIN();

//-----------------------------------------------------
// RDB record max size
//-----------------------------------------------------
#define RDB_RECORD_MAXSIZE 4096

//-----------------------------------------------------
// CRdbValue & CRawData
//-----------------------------------------------------
struct CRdbValue
{
	int8 i8;
	int16 i16;
	int32 i32;
	int64 i64;
	uint8 u8;
	uint16 u16;
	uint32 u32;
	uint64 u64;
	float f32;
	double f64;
	char* s;
};

struct CRawData
{
	uint32 nSize;
	char sData[1];
};


//-----------------------------------------------------
// CRdbNameTable
//-----------------------------------------------------
class CRdbNameTable
{
private:
	CHashMap<CString, uint32, CNameCompare, CNameHashArithmetic> m_oHashTable;

public:
	CRdbNameTable();
	~CRdbNameTable();

	uint32 FindName(char* sName);
	bool InsertName(char* sName, uint32 nIndex);
	void Erase(char* sName);
	uint32 GetSize();
	void Clear();
};

//-----------------------------------------------------
// CBaseFieldDefine
//-----------------------------------------------------
struct CBaseFieldDefine
{
	CRdbFieldDef* m_pBaseAttr;
	uint32 m_nOffset, m_nSize;	
};

//-----------------------------------------------------
// CBaseTableDefine
//-----------------------------------------------------
struct CBaseTableDefine
{
	uint32 m_nRecordSize;
	uint32 m_nFieldCount;
	uint32 m_nVarFieldCount;
	uint32* m_pVarFieldList;
	CBaseFieldDefine** m_pFieldDefineSet;

	CBaseTableDefine();
	virtual ~CBaseTableDefine();

	virtual const char* GetLogName() = 0;

	bool IsValidField(uint32 nFieldNo);
	uint32 GetFieldFlagCount();
};

//-----------------------------------------------------
// base utility
//-----------------------------------------------------
#define RDB_FLG_L(x) ((x)>>5)
#define RDB_FLG_B(x) (1<<((x)&31))

#define GetAtomX(type, pField) (*(type*)pField->m_pData)
#define GetAtomX0(type) (*(type*)m_pData)
#define FillField() ((*m_pFlag) |= m_nBit)
#define FillFieldA(pField) ((*pField->m_pFlag) |= pField->m_nBit)
#define IS_NULL() (((*m_pFlag) & m_nBit)?false:true)
#define IS_NULL_A(pField) (((*pField->m_pFlag) & pField->m_nBit)?false:true)
#define IS_SET_FIELD(pObj, nField) ((nField<pObj->m_nFieldCount) && (pObj->m_pFlag[RDB_FLG_L(nField)] & RDB_FLG_B(nField)))
#define IS_SET_FIELD0(oObj, nField) ((nField<oObj.m_nFieldCount) && (oObj.m_pFlag[RDB_FLG_L(nField)] & RDB_FLG_B(nField)))

template<typename T> T Abs(T& t)
{
	if(t>(T)0)
		return t;
	return -t;
}

bool InvalidDbName(char* sName);
const char* GetErrorInfo(uint32 nCode);

const char* GetLogName();

uint32 CorrectFieldAttr(const char* sLogName, CRdbFieldDef* pFieldDefine);

char* CloneString(const char* sLogName, char* sName);
bool CloneFieldDef(const char* sLogName, CRdbFieldDef* &pDstFieldDefines, CRdbFieldDef* pSrcFieldDefines, uint32 nFieldCount);
bool CloneTableDef(const char* sLogName, CRdbTableDef & oDstTableDef, CRdbTableDef & oSrcTableDef);
bool CloneIndexDef(const char* sLogName, CRdbIndexDef & oDstIndexDef, CRdbIndexDef & oSrcIndexDef);

void FreeFieldDefine(CRdbFieldDef& oFieldDef);
void FreeTableDefine(CRdbTableDef& oDefine);
void FreeIndexDefine(CRdbIndexDef& oDefine);

bool PackTableDefine(CMemoryStream & oStream, CRdbTableDef& oDefine);
bool UnPackTableDefine(CMemoryStream & oStream, CRdbTableDef& oDefine);
bool PackIndexDefine(CMemoryStream & oStream, CRdbIndexDef &oDefine);
bool UnPackIndexDefine(CMemoryStream & oStream, CRdbIndexDef &oDefine);

FOCP_END();

#endif
