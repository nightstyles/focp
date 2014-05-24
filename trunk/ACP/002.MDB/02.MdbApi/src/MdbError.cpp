
#include "MdbError.hpp"
#include "AFC.hpp"

FOCP_BEGIN();

static CMutex g_oErrorMutex;
static CRbMap<uint32, const char*> m_oErrorTable;

struct CMdbErrorInit
{
	CMdbErrorInit()
	{
		m_oErrorTable[MDB_INVALID_INPUT] = "MDB_INVALID_INPUT";
		m_oErrorTable[MDB_INVALID_NAME] = "MDB_INVALID_NAME";
		m_oErrorTable[MDB_INVALID_TYPE] = "MDB_INVALID_TYPE";
		m_oErrorTable[MDB_INVALID_SIZE] = "MDB_INVALID_SIZE";
		m_oErrorTable[MDB_REPEAT_FIELD] = "MDB_REPEAT_FIELD";
		m_oErrorTable[MDB_DB_EXIST] = "MDB_DB_EXIST";
		m_oErrorTable[MDB_DB_NOTEXIST] = "MDB_DB_NOTEXIST";
		m_oErrorTable[MDB_DB_BUSY] = "MDB_DB_BUSY";
		m_oErrorTable[MDB_DB_NOSELECTED] = "MDB_DB_NOSELECTED";
		m_oErrorTable[MDB_TABLE_EXIST] = "MDB_TABLE_EXIST";
		m_oErrorTable[MDB_TABLE_NOTEXIST] = "MDB_TABLE_NOTEXIST";
		m_oErrorTable[MDB_RECORD_TOOLONG] = "MDB_RECORD_TOOLONG";
		m_oErrorTable[MDB_RECORD_TOOMANY] = "MDB_RECORD_TOOMANY";
		m_oErrorTable[MDB_INDEX_EXIST] = "MDB_INDEX_EXIST";
		m_oErrorTable[MDB_INDEX_NOTEXIST] = "MDB_INDEX_NOTEXIST";
		m_oErrorTable[MDB_INDEX_TOOMANY] = "MDB_INDEX_TOOMANY";
		m_oErrorTable[MDB_DETAIL_EXIST] = "MDB_DETAIL_EXIST";
		m_oErrorTable[MDB_DETAIL_NOTEXIST] = "MDB_DETAIL_NOTEXIST";
		m_oErrorTable[MDB_FIELD_NOTEXIST] = "MDB_FIELD_NOTEXIST";
		m_oErrorTable[MDB_FIELD_TOOMANY] = "MDB_FIELD_TOOMANY";
		m_oErrorTable[MDB_FIELD_ISNULL] = "MDB_FIELD_ISNULL";
		m_oErrorTable[MDB_INVALID_FOREIGN_INDEX] = "MDB_INVALID_FOREIGN_INDEX";
		m_oErrorTable[MDB_INDEXFIELD_TOOMANY] = "MDB_INDEXFIELD_TOOMANY";
		m_oErrorTable[MDB_INVALID_INDEXFIELD] = "MDB_INVALID_INDEXFIELD";
		m_oErrorTable[MDB_INVALID_INDEX_ARITHMETIC] = "MDB_INVALID_INDEX_ARITHMETIC";
		m_oErrorTable[MDB_TRAVEL_FAILURE] = "MDB_TRAVEL_FAILURE";
		m_oErrorTable[MDB_INVALID_COND] = "MDB_INVALID_COND";
		m_oErrorTable[MDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE] = "MDB_RECORD_NOTEXIST_IN_PRIMARY_TABLE";
		m_oErrorTable[MDB_UNIQUE_INDEX_CONFLICT] = "MDB_UNIQUE_INDEX_CONFLICT";
		m_oErrorTable[MDB_RECORD_NOT_EXIST] = "MDB_RECORD_NOT_EXIST";
		m_oErrorTable[MDB_TRY_UPDATE_PRIMARYKEY] = "MDB_TRY_UPDATE_PRIMARYKEY";
		m_oErrorTable[MDB_TRIGGER_CHECKFAILURE] = "MDB_TRIGGER_CHECKFAILURE";
		m_oErrorTable[MDB_INVALID_INTERFACE] = "MDB_INVALID_INTERFACE";
		m_oErrorTable[MDB_NONEMPTY_TABLE] = "MDB_NONEMPTY_TABLE";
		m_oErrorTable[MDB_FORBIT_ACCESS] = "MDB_FORBIT_ACCESS";
	}
};

static CMdbErrorInit g_oInitor;

MCI_API bool RegMdbError(uint32 nCode, const char* sInfo)
{
	if(nCode == 0 || sInfo == NULL || sInfo[0] == '\0')
		return false;
	bool bRet = true;
	CRbTreeNode* pEnd = m_oErrorTable.End();
	g_oErrorMutex.Enter();
	CRbTreeNode* pIt = m_oErrorTable.Find(nCode);
	if(pIt == pEnd)
		m_oErrorTable[nCode] = sInfo;
	else
		bRet = false;
	g_oErrorMutex.Leave();
	return bRet;
}

MCI_API const char* GetMdbError(uint32 nCode)
{
	const char* sInfo = NULL;
	if(nCode)
	{
		CRbTreeNode* pEnd = m_oErrorTable.End();
		g_oErrorMutex.Enter();
		CRbTreeNode* pIt = m_oErrorTable.Find(nCode);
		if(pIt != pEnd)
			sInfo = m_oErrorTable.GetItem(pIt);
		g_oErrorMutex.Leave();
	}
	return sInfo;
}

FOCP_END();
