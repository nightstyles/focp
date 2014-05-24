
#include "AFC.hpp"

#ifndef _MDB_HPP_
#define _MDB_HPP_

#if defined(MDBLOC_EXPORTS)
#define MDB_API FOCP_EXPORT
#else
#define MDB_API FOCP_IMPORT
#endif

FOCP_C_BEGIN();

bool MDB_API CreateMemoryDataBase(const char* sDbName);

FOCP_C_END();

#endif
