
#include "AFC.hpp"

#ifndef _MDB_LSN_HPP_
#define _MDB_LSN_HPP_

FOCP_BEGIN();

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
	MDB_QUERYEXIST_REQ,

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
	MDB_QUERYEXIST_RESP,

	//Error;
	MDB_PACKAGE_ERROR = 301,
};

class CMdbSvrModule: public CAcmTcpModule
{
public:
	CMdbSvrModule();
	virtual ~CMdbSvrModule();

protected:
	virtual void ProcessAcmModuleMsg(CAcmTcpLink* pLink, uint32 nCmd, CMemoryStream& oStream);

private:
	bool ReadString(CMemoryStream &oStream, CString *pStr);
	void WriteString(CMemoryStream &oStream, const char* sStr);
	void ProcessGetDbList(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessGetTableList(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessGetIndexList(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessGetTableDef(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessGetIndexDef(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessInsert(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessUpdate(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessDelete(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessTruncate(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessQuery(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessQueryCount(CAcmTcpLink* pLink, CMemoryStream &oStream);
	void ProcessQueryExist(CAcmTcpLink* pLink, CMemoryStream &oStream);
};

FOCP_END();

#endif
