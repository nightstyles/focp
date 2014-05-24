
#include "RdbIdx.hpp"

#ifndef _RDB_TAB_HPP_
#define _RDB_TAB_HPP_

FOCP_BEGIN();

class CRdbTable;
class CRdbTableAccess;
extern CRedoLog* g_pRedo;

//------------------------------------------------------------------------
// CRdbTable
//------------------------------------------------------------------------
class CRdbDataBase;
class CRdbTable
{
public:
	uint64 m_nThis;
	CTableDefine* m_pTabDef;
	CTableSpace* m_pTableSpace;
	CDataTable* m_pDataTable;
	CRdbRowIndex* m_pRowIndex;
	uint32 m_nIndexCount;
	CAbstractIndex** m_pIndexTable;
	CRwMutex m_oLock;

public:
	CRdbTable();
	~CRdbTable();

	uint64 GetThis();
	CRdbTable& SetThis(CTableDefine* pTabDef);

	bool CreateObject(CTableDefine* pTabDef);
	void DestroyObject();

	uint32 GetIndexCount();
	CAbstractIndex* GetIndex(uint32 nIndexNo);

	void DropIndex(uint32 nIndexNo);
	uint32 CreateNewIndex(uint32 nIndexNo);

	uint32 Query(CSqlParameterSet* pCondition, CSqlFilter& oFilter, CRecordSet &oResult, uint32 nSkipCount);
	uint32 Query(uint64 nRowId, CSqlFilter& oFilter, CRecordSet &oResult);
	uint32 QueryCount(CSqlParameterSet * pCondition, uint32 &nCount);
	uint32 Exist(CSqlParameterSet * pCondition, uint32& bExist);

	uint32 Insert(CSqlParameter* pInsertAttr);
	uint32 Delete(CSqlParameterSet* pCondition, uint32 &nRemovedCount);
	uint32 Update(CSqlParameterSet* pCondition, CSqlParameter* pSetAttr, CRecordSet &oRollBack, uint32 &nModifiedCount);
	void Truncate();

	bool IsEmptyTable();

private:
	friend uint32 DeleteRdbRecord(CAbstractIndex* pIndex, CDataTable::CIterator& oIt, CRecord* pRecord, void*);
	friend uint32 UpdateRdbRecord(CAbstractIndex* pIndex, CDataTable::CIterator& oIt, CRecord* pRecord, void* pPara);
	void Construction(CTableDefine* pTabDef);
	void Distruction();
	CAbstractIndex* IndexChoice(CSqlParameterSet* pCondition);
	uint32 GetFieldAttrTable(uint32* pFlag, uint32* pIsRange, CSqlParameterSet* pCondition);
	void Delete(CDataTable::CIterator& oIt, CRecord* pRecord);
	uint32 Update(CDataTable::CIterator& oIt, CRecord* pRecord, CSqlParameter* pSetAttr, CRecordSet* pRollBack, uint32* pModifiedCount, bool bNeedDetach, CSqlFilter& oFilter);
	void RollBack(CRdbRecord& oCurDbRecord, CRecordSet* pRollBack, uint32 nLastRecordNo, uint32 nLastIndexNo, CSqlFilter& oFilter, CSqlParameter* pSetAttr);
	void FillIndexFilter(CSqlFilter& oFilter);
};

//------------------------------------------------------------------------
// CRdbTableAccess
//------------------------------------------------------------------------
class CRdbTableAccess: public CRdbAccess
{
public:
	uint32 m_nLocked;
	CTableDefine * m_pTabDef;
	CRdbTable* m_pTable;
	CRwMutex* m_pLock;
	CSqlParameterSet m_oIdxAttr;
	CSqlParameter m_oInsertAttr;
	CSqlParameter m_oSetAttr;
	CRecordSet m_oGetAttr;
	CRecordSet m_oRollBack;
	CSqlFilter m_oFilter;
	void * m_pDb;

public:
	CRdbTableAccess(void* pDb, CTableDefine* pTabDef, CRwMutex* pLock);
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

	void ReTruncate(char* sRedo, uint32 nSize);
	void ReInsert(char* sRedo, uint32 nSize);
	void ReDelete(char* sRedo, uint32 nSize);
	void ReUpdate(char* sRedo, uint32 nSize);

private:
	uint32 WriteRedoForTruncate();
	uint32 WriteRedoForInsert();
	uint32 WriteRedoForDelete();
	uint32 WriteRedoForUpdate(uint32 nMaxRollBackRecordQuantity);
};

FOCP_END();

#endif
