
#include "MdbIdx.hpp"

#ifndef _MDB_TAB_HPP_
#define _MDB_TAB_HPP_

FOCP_BEGIN();

/*
锁设计：读写锁（表），记录锁（数据表）
	Delete/Truncate/Insert->写锁
	Query->读锁，记录锁
	Update->修改唯一性索引的，写锁，读锁。
*/
struct CBeforeTriggerItem
{
	FBeforeTrigger Trigger;
	void* pContext;
	uint32 nCallerMask;
};
struct CAfterTriggerItem
{
	FAfterTrigger Trigger;
	void* pContext;
	uint32 nCallerMask;
};
struct CQueryTriggerItem
{
	FQueryTrigger Trigger;
	void* pContext;
	uint32 nCallerMask;
};
class CMdbTable
{
public:
	CMdbTableDef* m_pTabDef;
	CMdbRowIndex m_oRowIndex;
	uint32 m_nIndexCount;
	CMdbAbstractIndex** m_pIndexTable;

	CSingleList<CBeforeTriggerItem> m_oInsertBeforeTriggerList;
	CSingleList<CBeforeTriggerItem> m_oDeleteBeforeTriggerList;
	CSingleList<CBeforeTriggerItem> m_oUpdateBeforeTriggerList;
	CSingleList<CBeforeTriggerItem> m_oTruncateBeforeTriggerList;
	CSingleList<CAfterTriggerItem> m_oInsertAfterTriggerList;
	CSingleList<CAfterTriggerItem> m_oDeleteAfterTriggerList;
	CSingleList<CAfterTriggerItem> m_oUpdateAfterTriggerList;
	CSingleList<CAfterTriggerItem> m_oTruncateAfterTriggerList;
	CSingleList<CQueryTriggerItem> m_oQueryTriggerList;

public:
	CMdbTable(CMdbTableDef* pTabDef);
	~CMdbTable();

	void AddIndex(CMdbAbstractIndex* pIndex);

	uint32 Query(CMdbSqlParaSet* pCondition, CMdbSqlFilter& oFilter, CMdbRecordSet &oResult, uint32 nSkipCount, CMdbAbstractIndex* pIndex, bool bAsc);
	uint32 Query(CMdbSqlParaSet * pCondition, uint32 &nCount);
	uint32 Exist(CMdbSqlParaSet * pCondition, uint32& bExist);

	uint32 Insert(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlPara* pInsertAttr);
	uint32 Delete(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, uint32 &nRemovedCount);
	uint32 Update(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, CMdbSqlPara* pSetAttr, uint32 &nModifiedCount);

	void Truncate(CMdbAccess* pAccess, uint32 nCaller);

	void RegInsertBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegInsertAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegDeleteBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegDeleteAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegUpdateBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegUpdateAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegTruncateBeforeTrigger(FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegTruncateAfterTrigger(FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	void RegQueryTrigger(FQueryTrigger Trigger, void* pContext, uint32 nCallerMask);

	bool ProcessBeforeTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CBeforeTriggerItem>* pTriggerList);
	void ProcessAfterTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CAfterTriggerItem>* pTriggerList);
	bool ProcessQueryTrigger(CMdbAccess* pAccess, uint32 nCaller, uint32 nMode, uint32 nPageSize, uint32 &nSkipCount);

private:
	friend uint32 DeleteMdbRecord(CMdbAbstractIndex* pIndex, CMdbDataRecord* pDataRecord, void* pPara, bool &bDeleted);
	uint32 Delete2(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, uint32 &nRemovedCount);
	void Delete(CMdbAccess* pAccess, uint32 nCaller, CMdbDataRecord* pDataRecord);

	friend uint32 UpdateMdbRecord(CMdbAbstractIndex* pIndex, CMdbDataRecord* pDataRecord, void* pPara, bool &bDeleted);
	uint32 Update2(CMdbAccess* pAccess, uint32 nCaller, CMdbSqlParaSet* pCondition, CMdbSqlPara* pSetAttr, uint32 &nModifiedCount);
	uint32 Update(CMdbDataRecord* pDataRecord, CMdbSqlPara* pSetAttr, CMdbDataRecord *&pRollBack, uint32* pModifiedCount,
				  bool bNeedDetach, bool bNeedRollBack);
	void RollBack(CMdbDataRecord *pRollBack);

	CMdbAbstractIndex* IndexChoice(CMdbSqlParaSet* pCondition);
	uint32 GetFieldAttrTable(uint32* pFlag, uint32* pIsRange, CMdbSqlParaSet* pCondition);

	void FillIndexFilter(CMdbSqlFilter& oFilter);
};

//------------------------------------------------------------------------
// CMdbDataAccess
//------------------------------------------------------------------------
class CMdbDataAccess: public CMdbTableAccess
{
private:
	CMdbTable* m_pTable;

public:
	CMdbDataAccess(CMdb* pDb, CMdbTableDef* pTabDef, CMdbTable* pTable);
	virtual ~CMdbDataAccess();

	virtual uint32 Insert(uint32 nCaller);
	virtual uint32 Update(uint32* pModifiedCount, uint32 nCaller);
	virtual uint32 Delete(uint32* pDeletedCount, uint32 nCaller);
	virtual uint32 Truncate(uint32 nCaller);
	virtual uint32 Query(uint32 nPageSize, uint32 nSkipCount, uint32 nCaller);
	virtual uint32 Count(uint32 &nCount, uint32 nCaller);
	virtual uint32 Exist(uint32& bExist, uint32 nCaller);
};

//------------------------------------------------------------------------
// CLocalMdb
//------------------------------------------------------------------------
struct CMdbTriggerItem
{
	FMdbTrigger Trigger;
	void* pContext;
	uint32 nCallerMask;
};
class CLocalMdb: public CMdb, public CMdbLocalInterface
{
private:
	CMdbDefine m_oMdbDef;
	CRbMap<CString, CMdbTable*, CNameCompare> m_oTables;
	CRbMap<CString, CMdbAbstractIndex*, CNameCompare> m_oIndexs;

public:
	CMutex m_oMutex;
	CSingleList<CMdbTriggerItem> m_oInsertTriggerList;
	CSingleList<CMdbTriggerItem> m_oDeleteTriggerList;
	CSingleList<CMdbTriggerItem> m_oUpdateTriggerList;
	CSingleList<CMdbTriggerItem> m_oTruncateTriggerList;

public:
	CLocalMdb(const char* sDbName);
	virtual ~CLocalMdb();

	virtual CMdbAccess* CreateAccess(const char* sTableName);
	virtual CMdbTableDef* GetTableDefine(const char* sTableName);
	virtual const char* GetTableList();

	virtual CMdbIndexDef* GetIndexDefine(const char* sIndexName);
	virtual const char* GetIndexList();
	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount);
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName);

	virtual CMdbLocalInterface* GetLocalInterface();

	virtual uint32 CreateTable(CMdbTableDef* pTableDefine);
	virtual uint32 CreateIndex(CMdbIndexDef* pIndexDefine);
	virtual void RegInsertBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegInsertAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegDeleteBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegDeleteAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegUpdateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegUpdateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegTruncateBeforeTrigger(const char* sTableName, FBeforeTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegTruncateAfterTrigger(const char* sTableName, FAfterTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegQueryTrigger(const char* sTableName, FQueryTrigger Trigger, void* pContext, uint32 nCallerMask);

	virtual void RegInsertDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegDeleteDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegUpdateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask);
	virtual void RegTruncateDbTrigger(FMdbTrigger Trigger, void* pContext, uint32 nCallerMask);

	void ProcessTrigger(CMdbAccess* pAccess, uint32 nCaller, CSingleList<CMdbTriggerItem>* pTriggerList);
};

FOCP_END();

#endif
