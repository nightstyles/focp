
#include "RdbTab.hpp"

#ifndef _RDB_DB_HPP_
#define _RDB_DB_HPP_

FOCP_BEGIN();

class CRdbDataBase;
class CRdbDataBaseSystem;

//-----------------------------------------------------------------------------
// CRdbDataBase
//----------------------------------------------------------------------------
class CRdbDataBase: public CRdb
{
public:
	uint32 m_nDbNo;
	CMutex m_oMutex;
	CDataBaseDefine* m_pDataBaseDefine;
	CObjectLock<uint32> m_oTableLockTable;
	CRbMap<uint32, CRbMap<ulong, CRdbTableAccess*> > m_oAccessTable;

public:
	CRdbDataBase(CDataBaseDefine* pDefine, uint32 nDbNo);
	virtual ~CRdbDataBase();

	virtual CRdbAccess* QueryAccess(const char* sTableName);

	virtual uint32 CreateTable(CRdbTableDef* pDefine);
	virtual uint32 ModifyTable(CRdbTableDef* pDefine);
	virtual uint32 DropTable(const char* sTableName);
	
	virtual uint32 CreateIndex(CRdbIndexDef* pDefine);
	virtual uint32 DropIndex(const char* sIndexName);

	virtual CRdbTableDef* GetTableDefine(const char* sTableName);
	virtual void ReleaseTableDefine(const char* sTableName);

	virtual CRdbIndexDef* GetIndexDefine(const char* sIndexName);
	virtual void ReleaseIndexDefine(const char* sIndexName);

	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount);
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName);

	virtual void QueryTableList(CString &oList);
	
	virtual void Release();
	
	const char* GetLogName();	

public:
	// for CRdbDataBaseSystem
	uint32 Mount();
	void UnMount(bool bDestroy=false);
	// for CRdbTableAccessManager
	void DestroyTableAccess(uint32 nTableNo);
	void DestroyTableAccess();
	
public:
	uint32 CreateTable2(CRdbTableDef* pDefine);
	uint32 ModifyTable2(CRdbTableDef* pDefine);
	uint32 RemoveTable2(char* sTableName);
	uint32 CreateIndex2(CRdbIndexDef* pDefine);
	uint32 RemoveIndex2(char* sIndexName);
};

//-----------------------------------------------------------------------------
// CRdbDataBaseSystem
//----------------------------------------------------------------------------
class CRdbDataBaseSystem: public CRdbSystem
{
public:
	CMutex m_oMutex;
	CDataBaseDefineSet m_oDbSysDef;
	CObjectLock<uint32> m_oDbLockTable;

public:
	CRdbDataBaseSystem();
	virtual ~CRdbDataBaseSystem();

	virtual uint32 Startup(const char* sConfigPath, const char* sRdoPath, uint32 nLogFileSize, uint32 nLogFileNum, bool bRedo);
	virtual void Cleanup();

	virtual void Reset(const char* sDataBakPath, const char* sDataBakPath2);
	virtual uint32 Backup(const char* sDataBakPath, const char* sDataBakPath2);

	virtual uint32 CreateDataBase(const char* sDbName);
	virtual uint32 RemoveDataBase(const char* sDbName);

	virtual CRdb* QueryDataBase(const char* sDbName);

	virtual void QueryDataBaseList(CString &oDbList);

public:
	uint32 CreateDataBase2(const char* sDbName);
	uint32 RemoveDataBase2(const char* sDbName);
	uint32 CreateDataBaseSystemObject();
	uint32 Startup();
};

FOCP_END();

#endif
