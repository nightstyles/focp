
#include "AFC.hpp"

#ifndef _MDB_SVR_HPP_
#define _MDB_SVR_HPP_

#if defined(MDBLSN_EXPORTS)
#define MDBSVR_API FOCP_EXPORT
#else
#define MDBSVR_API FOCP_IMPORT
#endif

FOCP_C_BEGIN();

MDBSVR_API void* CreateMdbServer();
MDBSVR_API void DestroyMdbServer(void* pMdbServer);

FOCP_C_END();

#endif
