
#include "MdbCon.hpp"
#include "MdbCli.hpp"
#include "MdbError.hpp"

static FOCP_NAME::CMutex g_oMutex;
static bool g_bInitError = false;

FOCP_C_BEGIN();

RDB_API void* CreateMdbClient(const char* sDbList, const char* sServerAddr, FOCP_NAME::uint16 nServerPort)
{
	g_oMutex.Enter();
	if(!g_bInitError)
	{
		g_bInitError = true;
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_SESSION_BUSY, "MDB_SESSION_BUSY");
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_PACKAGE_ERROR, "MDB_PACKAGE_ERROR");
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_SEND_ERROR, "MDB_SEND_ERROR");
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_TIMEOUT_ERROR, "MDB_TIMEOUT_ERROR");
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_INVALID_SESSION, "MDB_INVALID_SESSION");
		FOCP_NAME::RegMdbError(FOCP_NAME::MDB_SESSION_BREAK, "MDB_SESSION_BREAK");
	}
	g_oMutex.Leave();
	FOCP_NAME::CMdbConnector* pClient = new FOCP_NAME::CMdbConnector;
	pClient->Initialize(sDbList, sServerAddr, nServerPort);
	return pClient;
}

RDB_API void DestroyMdbClient(void* pMdbClient)
{
	delete (FOCP_NAME::CMdbConnector*)pMdbClient;
}

RDB_API FOCP_NAME::uint32 StartMdbClient(void* pMdbClient)
{
	return ((FOCP_NAME::CMdbConnector*)pMdbClient)->Start();
}

RDB_API void StopMdbClient(void* pMdbClient, bool bBlock)
{
	((FOCP_NAME::CMdbConnector*)pMdbClient)->Stop(bBlock);
	if(bBlock)
		((FOCP_NAME::CMdbConnector*)pMdbClient)->Cleanup();
}

FOCP_C_END();
