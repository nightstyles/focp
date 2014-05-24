
#include "MdbAccess.hpp"
#include "MdbDef.hpp"

#ifndef _MDB_CON_HPP_
#define _MDB_CON_HPP_

FOCP_BEGIN();

class CRemoteAccess;
class CRemoteMdb;
class CMdbConnector;

enum
{
	//nOp;
	MDB_GETDBLIST_REQ = 0X11111110,
	MDB_GETTABLELIST_REQ,
	MDB_GETINDEXLIST_REQ,
	MDB_GETTABLEDEF_REQ,
	MDB_GETINDEXDEF_REQ,
	MDB_INSERT_REQ,
	MDB_UPDATE_REQ,
	MDB_DELETE_REQ,
	MDB_TRUNCATE_REQ,
	MDB_QUERY_REQ,
	MDB_QUERYCOUNT_REQ,
	MDB_QUERYEXIT_REQ,

	MDB_GETDBLIST_RESP = 0X22222222,
	MDB_GETTABLELIST_RESP,
	MDB_GETINDEXLIST_RESP,
	MDB_GETTABLEDEF_RESP,
	MDB_GETINDEXDEF_RESP,
	MDB_INSERT_RESP,
	MDB_UPDATE_RESP,
	MDB_DELETE_RESP,
	MDB_TRUNCATE_RESP,
	MDB_QUERY_RESP,
	MDB_QUERYCOUNT_RESP,
	MDB_QUERYEXIT_RESP,

	// Error Code
	MDB_SESSION_BUSY = 300,
	MDB_PACKAGE_ERROR,
	MDB_SEND_ERROR,
	MDB_TIMEOUT_ERROR,
	MDB_INVALID_SESSION,
	MDB_SESSION_BREAK,
};

//-----------------------------------------------------
// CRemoteAccess
//-----------------------------------------------------
class CRemoteAccess: public CMdbTableAccess
{
public:
	uint32 m_nStatus, m_nCount;

	CRemoteAccess(CMdb* pDb, CMdbTableDef* pTabDef);
	virtual ~CRemoteAccess();

	virtual uint32 Insert(uint32 nCaller);
	virtual uint32 Update(uint32* pModifiedCount, uint32 nCaller);
	virtual uint32 Delete(uint32* pDeletedCount, uint32 nCaller);
	virtual uint32 Truncate(uint32 nCaller);
	virtual uint32 Query(uint32 nPageSize, uint32 nSkipCount, uint32 nCaller);
	virtual uint32 Count(uint32 &nCount, uint32 nCaller);
	virtual uint32 Exist(uint32& bExist, uint32 nCaller);
};

//-----------------------------------------------------
// CRemoteMdb
//-----------------------------------------------------
class CRemoteMdb: public CMdb
{
	friend class CMdbConnector;
	friend class CRemoteAccess;
private:
	CMdbConnector* m_pConnector;
	CMdbDefine m_oDbDef;
	CString m_oRemoteDbName;

public:
	CRemoteMdb(const char* sLocalDbName, const char* sRemoteDbName);
	virtual ~CRemoteMdb();

	virtual CMdbAccess* CreateAccess(const char* sTableName);
	virtual CMdbTableDef* GetTableDefine(const char* sTableName);
	virtual const char* GetTableList();

	virtual CMdbIndexDef* GetIndexDefine(const char* sIndexName);
	virtual const char* GetIndexList();
	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount);
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName);
};

//-----------------------------------------------------
// CMdbConnector
//-----------------------------------------------------
class CMdbConnector: public CAcmTcpClientContext
{
	struct CLoginContext
	{
		CString oRdbList;
		CString oLdbList;
		CString oAccessList;
		uint32 nStatus;
	};
private:
	CAcmTcpClient m_oTcp;
	CLoginContext* m_pLoginContext;
	uint32 m_nMagic;
	const char* m_sServerAddr;
	uint16 m_nServerPort;
	CString m_oSysdbAccess;
	bool m_bForce;

public:
	CMdbConnector();
	virtual ~CMdbConnector();

	bool Initialize(const char* sDbList, const char* sServerAddr, uint16 nServerPort);
	void Cleanup();

	bool Start();
	void Stop(bool bBlock=true);

	uint32 Insert(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess);
	uint32 Update(const char* sLocalDbName, const char* sRemoteDbName, uint32* pModifiedCount, CRemoteAccess* pAccess);
	uint32 Delete(const char* sLocalDbName, const char* sRemoteDbName, uint32* pDeletedCount, CRemoteAccess* pAccess);
	uint32 Truncate(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess);
	uint32 Query(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32 nPageSize, uint32 nSkipCount);
	uint32 Query(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32 &nCount);
	uint32 Exist(const char* sLocalDbName, const char* sRemoteDbName, CRemoteAccess* pAccess, uint32& bExist);

	virtual void ProcessMsg(CAcmTcpClient* pClient, CTcpHead& oHead, CMemoryStream &oStream);
	virtual uint32 OnLogin(CAcmTcpClient* pClient, bool bReLogin);

private:
	uint32 GetDbList();
	uint32 GetTableList(CRemoteMdb* pDb, const char* sAccessInfo);
	uint32 GetIndexList(CRemoteMdb* pDb);
	uint32 GetTableDefine(CRemoteMdb* pDb, const char* sTabName);
	uint32 GetIndexDefine(CRemoteMdb* pDb, const char* sIdxName);
	//Response Process
	bool ReadString(CMemoryStream &oStream, CString *pStr);
	bool WriteString(CMemoryStream &oStream, const char* sStr);
	void ReturnNameList(CMemoryStream &oStream);//GetDbList, GetTableList, GetIndexList
	void ReturnTableDefine(CMemoryStream &oStream);
	void ReturnIndexDefine(CMemoryStream &oStream);
	void ReturnModify(CMemoryStream &oStream);
	void ReturnQuery(CMemoryStream &oStream);
	void ReturnQueryCount(CMemoryStream &oStream);
	bool InitializeDbList(char* sDbList, bool bForce);
	bool GetAccessInfo(char* sDbList, CString& oAccessInfo);
};

FOCP_END();

#endif
