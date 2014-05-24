
#include "RdbDb.hpp"

FOCP_BEGIN();

CRdbResult::CRdbResult()
{
}

CRdbResult::~CRdbResult()
{
}

uint32 CRdbResult::GetType(register uint32 nFieldNo)
{
	return 0;
}

bool CRdbResult::IsNull(register uint32 nFieldNo)
{
	return true;
}

int8 CRdbResult::GetInt8(register uint32 nFieldNo)
{
	return (int8)0;
}

int16 CRdbResult::GetInt16(register uint32 nFieldNo)
{
	return (int16)0;
}

int32 CRdbResult::GetInt32(register uint32 nFieldNo)
{
	return (int32)0;
}

int64 CRdbResult::GetInt64(register uint32 nFieldNo)
{
	return (int64)0;
}

uint8 CRdbResult::GetUInt8(register uint32 nFieldNo)
{
	return (uint8)0;
}

uint16 CRdbResult::GetUInt16(register uint32 nFieldNo)
{
	return (uint16)0;
}

uint32 CRdbResult::GetUInt32(register uint32 nFieldNo)
{
	return (uint32)0;
}

uint64 CRdbResult::GetUInt64(register uint32 nFieldNo)
{
	return (uint64)0;
}

float CRdbResult::GetFloat(register uint32 nFieldNo)
{
	return (float)0.0;
}

double CRdbResult::GetDouble(register uint32 nFieldNo)
{
	return (double)0.0;
}

char* CRdbResult::GetString(register uint32 nFieldNo, register uint32 * pStrLen)
{
	return (char*)NULL;
}

void* CRdbResult::GetRaw(register uint32 nFieldNo, register uint32 &nRawSize)
{
	return (void*)NULL;
}

uint32 CRdbResult::GetStringSize(register uint32 nFieldNo)
{
	return (uint32)0;
}

void CRdbResult::GetAsString(register uint32 nFieldNo, register char * pString)
{
}

CRdbResultSet::CRdbResultSet()
{
}

CRdbResultSet::~CRdbResultSet()
{
}

uint32 CRdbResultSet::GetResultCount()
{
	return 0;
}

CRdbResult* CRdbResultSet::GetResult(uint32 nRecordNo)
{
	return NULL;
}

CRdbPara::CRdbPara()
{
}

CRdbPara::~CRdbPara()
{
}


uint32 CRdbPara::GetType(register uint32 nFieldNo)
{
	return 0;
}

void CRdbPara::SetNull(register uint32 nFieldNo, uint32 nOperator)
{
}

void CRdbPara::SetInt8(register uint32 nFieldNo, register int8 v, register uint32 nOperator)
{
}

void CRdbPara::SetInt16(register uint32 nFieldNo, register int16 v, register uint32 nOperator)
{
}

void CRdbPara::SetInt32(register uint32 nFieldNo, register int32 v, register uint32 nOperator)
{
}

void CRdbPara::SetInt64(register uint32 nFieldNo, register int64 v, register uint32 nOperator)
{
}

void CRdbPara::SetUInt8(register uint32 nFieldNo, register uint8 v, register uint32 nOperator)
{
}

void CRdbPara::SetUInt16(register uint32 nFieldNo, register uint16 v, register uint32 nOperator)
{
}

void CRdbPara::SetUInt32(register uint32 nFieldNo, register uint32 v, register uint32 nOperator)
{
}

void CRdbPara::SetUInt64(register uint32 nFieldNo, register uint64 v, register uint32 nOperator)
{
}

void CRdbPara::SetFloat(register uint32 nFieldNo, register float v, register uint32 nOperator)
{
}

void CRdbPara::SetDouble(register uint32 nFieldNo, register double v, register uint32 nOperator)
{
}

void CRdbPara::SetString(register uint32 nFieldNo, register const char * v, register uint32 nOperator)
{
}

void CRdbPara::SetRaw(register uint32 nFieldNo, register const void * v, register uint32 nLen, register uint32 nOperator)
{
}

void CRdbPara::SetFromString(register uint32 nFieldNo, register const char* v, register uint32 nOperator)
{
}

void CRdbPara::Clear()
{
}

CRdbParaSet::CRdbParaSet()
{
}

CRdbParaSet::~CRdbParaSet()
{
}


CRdbPara* CRdbParaSet::AddPara()
{
	return NULL;
}

CRdbParaSet* CRdbParaSet::AddParaSet()
{
	return NULL;
}


uint32 CRdbParaSet::GetParaCount()
{
	return 0;
}

void* CRdbParaSet::GetPara(uint32 nParaIdx, bool &bParaSet)
{
	return NULL;
}

void CRdbParaSet::Clear()
{
}

CRdbFilter::CRdbFilter()
{
}

CRdbFilter::~CRdbFilter()
{
}

void CRdbFilter::SetField(uint32 nFieldNo)
{
}

void CRdbFilter::Clear()
{
}

CRdbAccess::CRdbAccess()
{
}

CRdbAccess::~CRdbAccess()
{
}

uint32 CRdbAccess::GetFieldNo(const char* sFieldName)
{
	return 0;
}

uint32 CRdbAccess::GetFieldCount()
{
	return 0;
}

bool CRdbAccess::IsValidField(uint32 nFieldNo)
{
	return false;
}

CRdbFilter* CRdbAccess::GetResultFilter()
{
	return NULL;
}

CRdbResultSet* CRdbAccess::GetResultSet()
{
	return NULL;
}

CRdbParaSet* CRdbAccess::GetQueryPara()
{
	return NULL;
}

CRdbPara* CRdbAccess::GetUpdatePara()
{
	return NULL;
}

CRdbPara* CRdbAccess::GetInsertPara()
{
	return NULL;
}

uint32 CRdbAccess::Insert()
{
	return 0;
}

uint32 CRdbAccess::Update(uint32 nMaxRollBack)
{
	return 0;
}

uint32 CRdbAccess::Delete()
{
	return 0;
}

uint32 CRdbAccess::Exist(uint32& bExist)
{
	return 0;
}

void CRdbAccess::Truncate()
{
}

uint32 CRdbAccess::Query(uint32 nResultSetSize, uint32 nSkipCount)
{
	return 0;
}

uint32 CRdbAccess::Query(uint64 nRowId)
{
	return 0;
}

uint32 CRdbAccess::Query(uint32 &nCount)
{
	return 0;
}

void CRdbAccess::Release()
{
}

CRdb::CRdb()
{
}

CRdb::~CRdb()
{
}

CRdbAccess* CRdb::QueryAccess(const char* sTableName)
{
	return NULL;
}

uint32 CRdb::CreateTable(CRdbTableDef* pTableDefine)
{
	return 0;
}

uint32 CRdb::ModifyTable(CRdbTableDef* pTableDefine)
{
	return 0;
}

uint32 CRdb::DropTable(const char* sTableName)
{
	return 0;
}

CRdbTableDef* CRdb::GetTableDefine(const char* sTableName)
{
	return NULL;
}

void CRdb::ReleaseTableDefine(const char* sTableName)
{
}

uint32 CRdb::GetIndexCount(const char* sTableName, uint32 & nCount)
{
	return 0;
}

uint32 CRdb::GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName)
{
	return 0;
}

uint32 CRdb::CreateIndex(CRdbIndexDef* pIndexDefine)
{
	return 0;
}

uint32 CRdb::DropIndex(const char* sIndexName)
{
	return 0;
}

CRdbIndexDef* CRdb::QueryIndexDefine(const char* sIndexName)
{
	return NULL;
}

void CRdb::ReleaseIndexDefine(const char* sIndexName)
{
}

void CRdb::QueryTableList(CString &oList)
{
}

void CRdb::Release()
{
}

CRdbSystem::CRdbSystem()
{
}

CRdbSystem::~CRdbSystem()
{
}

uint32 CRdbSystem::Startup(const char* sConfigPath, const char* sRdoPath, uint32 nLogFileSize, uint32 nLogFileNum, bool bRedo)
{

	return 0;
}

void CRdbSystem::Cleanup()
{
}

void CRdbSystem::Reset(const char* sDataBakPath, const char* sDataBakPath2)
{
}

uint32 CRdbSystem::Backup(const char* sDataBakPath, const char* sDataBakPath2)
{
	return 0;
}

CRdb* CRdbSystem::QueryDataBase(const char* sDbName)
{
	return NULL;
}

uint32 CRdbSystem::CreateDataBase(const char* sDbName)
{
	return 0;
}

uint32 CRdbSystem::DestroyDataBase(const char* sDbName)
{
	return 0;
}

void CRdbSystem::QueryDataBaseList(CString& oDbList)
{
}

FOCP_END();
