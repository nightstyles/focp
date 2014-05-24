/****************************************************************************
 *
 *  Copyright  (c)  2002,  UTStarcom, Inc.
 *          All Rights Reserved.
 *
 *  Subsystem:  TS
 *  Created:    Jacky Cao 2007/06/04
 *
 ****************************************************************************/

#ifndef _SQL_API_HPP_
#define _SQL_API_HPP_

#include "AFC.hpp"

FOCP_BEGIN();

#if defined(SQLAPI_EXPORTS)
#define SQL_API FOCP_EXPORT
#else
#define SQL_API FOCP_IMPORT
#endif

typedef void (*FSqlBufferSize)(void* pTerminal, uint32 * pReadSize, uint32 * pWriteSize);
typedef void (*FSqlPuts)(void* pTerminal, const char* sLine);
typedef const char* (*FSqlGets)(void* pTerminal);

struct SQL_API CSqlTerminal
{
	void* pTerminal;
	FSqlPuts PutLine;
	FSqlGets GetLine;
	FSqlBufferSize BufferSize;
};

SQL_API void* CreateSqlEnv(CSqlTerminal* pTerminal, char* sDataBakPath, char* sDataBakPath2);
SQL_API void DestroySqlEnv(void* pSqlEnv);
SQL_API void ExecSql(void* pSqlEnv);

FOCP_END();

#endif
