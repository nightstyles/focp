
#include "AFC.hpp"

#ifndef _MDB_CLI_HPP_
#define _MDB_CLI_HPP_

#if defined(MDBCON_EXPORTS)
#define RDB_API FOCP_EXPORT
#else
#define RDB_API FOCP_IMPORT
#endif

FOCP_C_BEGIN();

RDB_API void* CreateMdbClient(const char* sDbList, const char* sServerAddr, FOCP_NAME::uint16 nServerPort);
RDB_API void DestroyMdbClient(void* pMdbClient);
RDB_API FOCP_NAME::uint32 StartMdbClient(void* pMdbClient);
RDB_API void StopMdbClient(void* pMdbClient, bool bBlock);

FOCP_C_END();

#endif
