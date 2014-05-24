
#include "SqlEnv.hpp"

FOCP_BEGIN();

SQL_API void* CreateSqlEnv(CSqlTerminal* pTerminal)
{
	return new CSqlEnv(pTerminal);
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
