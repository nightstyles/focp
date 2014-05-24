
#ifndef _REMOTE_RDB_HPP_
#define _REMOTE_RDB_HPP_

class RDB_API CRemoteDataBase: public CRdb
{
private:
	CMutex m_oMutex;
	uint32 m_nCounter;
	uint32 m_bValid;
	const char* m_sDbName;
	CRemoteDataBaseDefine* m_pDataBaseDefine;
	CRbMap<uint32, CRbMap<ulong, CRdbTableAccess*> > m_oAccessTable;
	CObjectLock<uint32> m_oTableLockTable;

public:
	CRemoteDataBase();
	virtual ~CRemoteDataBase();

	virtual CRdbAccess* QueryAccess(const char* sTableName);

	virtual uint32 CreateTable(CRdbTableDef* pTableDefine);
	virtual uint32 ModifyTable(CRdbTableDef* pTableDefine);
	virtual uint32 DropTable(const char* sTableName);

	virtual CRdbTableDef* GetTableDefine(const char* sTableName);
	virtual void ReleaseTableDefine(const char* sTableName);

	virtual uint32 GetIndexCount(const char* sTableName, uint32 & nCount);
	virtual uint32 GetIndex(const char* sTableName, uint32 nIdx, char* pIndexName);

	virtual uint32 CreateIndex(CRdbIndexDef* pIndexDefine);
	virtual uint32 DropIndex(const char* sIndexName);

	virtual CRdbIndexDef* QueryIndexDefine(const char* sIndexName);
	virtual void ReleaseIndexDefine(const char* sIndexName);

	virtual void QueryTableList(CString &oList);

	virtual void Release();
};

class RDB_API CRemoteDataBaseSystem: public CRemoteRdbSystem
{
private:
	CMutex m_oMutex;
	CMap<CString, CRemoteDataBase*> m_oDataBases;

public:
	CRemoteDataBaseSystem();
	virtual ~CRemoteDataBaseSystem();

	virtual uint32 Startup(const char* sDbHost);
	virtual void Cleanup();

	virtual uint32 GetStorageMode();

	virtual CRdb* QueryDataBase(const char* sDbName);

	virtual uint32 CreateDataBase(const char* sDbName);
	virtual uint32 DestroyDataBase(const char* sDbName);

	virtual void QueryDataBaseList(CString& oDbList);
};

#endif
