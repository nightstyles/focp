
#include "RemoteRdbDef.hpp"
#include "RdbPara.hpp"

#ifndef _REMOTE_RDB_TABLE_HPP_
#define _REMOTE_RDB_TABLE_HPP_

FOCP_BEGIN();

class CRemoteRdbTable;
class CRemoteRdbAccess;

//------------------------------------------------------------------------
// CRemoteRdbTable
//------------------------------------------------------------------------
class CRemoteRdbTable
{
private:
	CRemoteTableDefine * m_pTabDef;

public:
	CRemoteRdbTable();
	~CRemoteRdbTable();

	uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	uint32 Query(uint64 nRowId, CSqlFilter& oFilter, CRecordSet &oResult);
	uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);

	uint32 Insert(CSqlParameter* pInsertAttr);
	uint32 Delete(CSqlParameterSet* pCondition, uint32 &nRemovedCount);
	uint32 Update(CSqlParameterSet* pCondition, CSqlParameter* pSetAttr, CRecordSet &oRollBack, uint32 &nModifiedCount);
	void Truncate();
};

//------------------------------------------------------------------------
// CRemoteRdbAccess
//------------------------------------------------------------------------
class CRemoteRdbAccess: public CRdbAccess
{
public:
	uint32 m_nLocked;
	CRemoteTableDefine * m_pTabDef;
	CRemoteRdbTable* m_pTable;
	CRwMutex* m_pLock;
	CSqlParameterSet m_oIdxAttr;
	CSqlParameter m_oInsertAttr;
	CSqlParameter m_oSetAttr;
	CRecordSet m_oGetAttr;
	CRecordSet m_oRollBack;
	CSqlFilter m_oFilter;
	void * m_pDb;

public:
	CRemoteRdbAccess(void* pDb, CRemoteTableDefine* pTabDef, CRwMutex* pLock);
	virtual ~CRdbTableAccess();

	virtual uint32 GetFieldCount();
	virtual uint32 GetFieldNo(const char* sFieldName);
	virtual bool IsValidField(uint32 nFieldNo);

	virtual CRdbFilter* GetResultFilter();
	virtual CRdbResultSet* GetResultSet();
	virtual CRdbParaSet* GetQueryPara();
	virtual CRdbPara* GetUpdatePara();
	virtual CRdbPara* GetInsertPara();

	virtual uint32 Query(uint32 nMaxRecordQuantity, uint32 nSkipCount);
	virtual uint32 Query(uint64 nRowId);
	virtual uint32 QueryCount(uint32 &nCount);
	virtual uint32 Exist(uint32& bExist);

	virtual uint32 Insert();
	virtual uint32 Delete();
	virtual uint32 Update(uint32 nMaxRollBackRecordQuantity);
	virtual void Truncate();

	virtual void Release();
};

FOCP_END();

#endif
