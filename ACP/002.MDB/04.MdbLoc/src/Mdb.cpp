
#include "Mdb.hpp"

#include "MdbTab.hpp"

FOCP_C_BEGIN();

bool MDB_API CreateMemoryDataBase(const char* sDbName)
{
	return (new FOCP_NAME::CLocalMdb(sDbName))!=NULL;
}

FOCP_C_END();
