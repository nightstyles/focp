
#include "MdbSvr.hpp"
#include "MdbLsn.hpp"
#include "MdbError.hpp"

static FOCP_NAME::CMutex g_oMutex;
static bool g_bInitError = false;

FOCP_C_BEGIN();

MDBSVR_API void* CreateMdbServer()
{
	g_oMutex.Enter();
	if(!g_bInitError)
	{
		g_bInitError = true;
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_PACKAGE_ERROR, "MDB_PACKAGE_ERROR");
	}
	g_oMutex.Leave();
	return new FOCP_NAME::CMdbSvrModule;
}

MDBSVR_API void DestroyMdbServer(void* pMdbServer)
{
	delete ((FOCP_NAME::CMdbSvrModule*)pMdbServer);
}

FOCP_C_END();
