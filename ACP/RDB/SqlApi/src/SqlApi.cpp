
#include "SqlEnv.hpp"

FOCP_BEGIN();

SQL_API void* CreateSqlEnv(CSqlTerminal* pTerminal, char* sDataBakPath, char* sDataBakPath2)
{
	return new CSqlEnv(pTerminal, sDataBakPath, sDataBakPath2);
}

SQL_API void DestroySqlEnv(void* pSqlEnv)
{
	if(pSqlEnv)
		delete (CSqlEnv*)pSqlEnv;
}

SQL_API void ExecSql(void* pSqlEnv)
{
	((CSqlEnv*)pSqlEnv)->Interpret();
}

FOCP_END();
