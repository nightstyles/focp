
/*
MDB持久化简易设计方案：
（1）采用标准关系型磁盘数据库系统作为MDB的数据持久化存储设备，以此实现MDB的持久化。
（2）MDB与关系型磁盘数据库对接采用标准的ODBC接口，以此避免大量的接口工作。
（3）需要入库的表需要扩展一定属性，以支持持久化需要：
	（A）表一级的扩展属性：
		数据库表名：可能是视图名，为空表示与内存表名相同。
		加载条件：MDB启动时仅仅加载满足条件的部分记录
		存储条件：数据入库时，仅仅存储满足条件的记录
		缓存条件：当数据变化时，用以判断是否需要保存在内存中
		主键索引：
		其它为支持ODBC而扩展的属性。
	（B）字段扩展属性：
		数据库字段名：为空表示与内存表字段名相同，最长64个字符。
		其它为支持ODBC而扩展的属性。
（4）持久化支持的操作：
	插入、修改、删除、清除。
（5）设计一张系统表以存储持久化操作信息，系统定时扫描该事件表，并把数据变更通过ODBC作用到
	磁盘数据库系统中。
（6）事件表结构定义如下：
	表名、主键、操作、操作时间[该字段用作调试]。
	（A）插入、修改、删除都是针对记录级的(主键非空)，清除是表级的(主键为空)。
	（B）当清除操作产生时，将删除该表之前的所有操作，并插入清除事件。
	（C）当删除操作产生时，
		（a）如果存在旧事件，则删除旧事件。
		（b）如果旧事件为插入操作，需要忽略当前事件。
		（c）否则插入当前删除事件。
	（D）当插入操作产生时，
		（a）如果事件表中存在删除事件，删除该记录，插入修改事件。
		（b）否则插入一个新的插入事件。
	（E）当修改操作产生时，仅仅定位于记录，不细化到字段。
		（a）如果旧记录符合存储条件，而新记录不符合存储条件，转删除流程。
		（b）如果旧记录不符合存储条件，而新记录符合存储条件，转插入流程。
		（c）如果修改了存储主键，将产生先删后插两个事件。
		（d）如果事件表中存在插入操作，删除该记录，并重新插入，事件类型不变。
		（e）如果事件表中存在修改操作，删除该记录，并重新插入，事件类型不变。
		（f）否则产生修改事件
	（F）当插入/修改事件作用时：
		（a）如果记录不符合缓存条件，清除Cache记录。
（7）一个ACM域可以包含多个数据库，一个数据库只能属于某一个特定的ACM域。这点与数据复制技术相同。
	一个数据库对应一条ODBC链路，不同ODBC链路可以指向同一数据源。
	一个数据库对应一张事件表。
	一个域中只能有一个节点能将事件表中的事件作用于磁盘数据库系统，通过令牌竞争机制来实现。
	事件表支持数据复制能力，那么无论哪个点竞得入库服务，都能正确的把数据更新到磁盘数据库中。
（8）事件表的生产者与消费者模型：
	（A）应用表发生变化时，在After触发器中，产生事件，转(6)的(BCDE)流程。
	（B）服务节点扫描该事件表一条记录，并将其入库，入库成功，删除该事件记录。
（9）事件表‘主键’字段设计约束：
	主键采用MDB SQL的WHERE条件的文本形式，最长不超过256字节，如果超长，记录日志，但不产生事件。
（10）支持启动时的数据加载。
*/

#include "MdbAccess.hpp"

#ifndef _MDB_STO_HPP_
#define _MDB_STO_HPP_

#ifdef WINDOWS
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>

#if defined(MDBSTO_EXPORTS)
#define MDBSTO_API FOCP_EXPORT
#else
#define MDBSTO_API FOCP_IMPORT
#endif

FOCP_C_BEGIN();

MDBSTO_API bool InitializeMdbStorageAttr(const char* sMdbName, const char* sMdbTabName,
		const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere,
		const char* sStorageIdx, const char* sFieldList);
MDBSTO_API bool InitializeMdbDataSource(const char* sMdbName, FOCP_NAME::uint32 nDomain, bool bSupportStorage,
										const char* sDSN, const char* sUser, const char* sPasswd,
										const char* sEventDb=MDB_SYSDB_NAME, const char* sEventTable=NULL);
MDBSTO_API void CleanupMdbStorage();
MDBSTO_API bool StartMdbStorage();
MDBSTO_API void StopMdbStorage(bool bBlock=true);
MDBSTO_API void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FOCP_NAME::FStorageTrigger StorageTrigger);
MDBSTO_API void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FOCP_NAME::FStorageTrigger CacheTrigger);

FOCP_C_END();

FOCP_BEGIN();

class COdbcConnect;
class COdbcTruncate;
class COdbcInsert;
class COdbcDelete;
class COdbcUpdate;
class COdbcSelect;
class CMdbStorager;
struct CMdbStorageTableAttr;
struct CMdbStorageFieldAttr;

class COdbcConnect
{
	friend class COdbcTruncate;
	friend class COdbcSelect;
	friend class COdbcInsert;
	friend class COdbcUpdate;
	friend class COdbcDelete;

private:
	void* m_pEnv;
	void* m_pDbc;
	bool m_bOpened;
	bool m_bConnected;
	bool m_bFirst;
	bool m_bBreak;
	CString m_oDsn, m_oUser, m_oPasswd;

public:
	COdbcConnect(void* pOdbcEnv);
	~COdbcConnect();

	bool Initialize(const char* pszDSN, const char* pszUName, const char* pszUPasswd);
	void Cleanup();

	bool Connect();//或重连
	void Disconnect();

	//初次连接成功后调用该函数用以初始化表格属性。
	bool InitializeTable(const char* sDbName, CMdbTableDef* pTabDef);

	bool IsBroken();//执行完COdbcSentence::Exec后，需调用该函数检测链路状态。

private:
	int16 GetValueType(CMdbFieldDef* pFieldDef, long& nBufSize);
	void OdbcError(void* hStmt, bool bWarning);
	bool BindSetField(void* pSentence, CMdbStorageFieldAttr* pFieldAttr, uint32 nCol, CMdbField* pField);
};

class COdbcTruncate
{
private:
	COdbcConnect* m_pDb;
	CMdbTableDef* m_pTabDef;
	void* m_pSentence;

public:
	COdbcTruncate(COdbcConnect* pDb, CMdbTableDef* pTabDef);
	~COdbcTruncate();

	void Exec();

private:
	bool PrepareSql();
};

class COdbcInsert
{
	friend class COdbcConnect;
private:
	COdbcConnect* m_pDb;
	CMdbTableDef* m_pTabDef;
	void* m_pSentence;

public:
	COdbcInsert(COdbcConnect* pDb, CMdbTableDef* pTabDef);
	~COdbcInsert();

	void Exec(CMdbRecord* pRecord);

private:
	bool PrepareSql();
	bool BindPara(CMdbRecord* pRecord);
};

class COdbcDelete
{
private:
	COdbcConnect* m_pDb;
	CMdbTableDef* m_pTabDef;
	void* m_pSentence;

public:
	COdbcDelete(COdbcConnect* pDb, CMdbTableDef* pTabDef);
	virtual ~COdbcDelete();

	void Exec(CMdbRecord* pRecord);

private:
	bool PrepareSql(CMdbRecord* pRecord);
	bool BindPara(CMdbRecord* pRecord);
};

class COdbcUpdate
{
private:
	COdbcConnect* m_pDb;
	CMdbTableDef* m_pTabDef;
	void* m_pSentence;

public:
	COdbcUpdate(COdbcConnect* pDb, CMdbTableDef* pTabDef);
	virtual ~COdbcUpdate();

	void Exec(CMdbRecord* pRecord);

private:
	bool PrepareSql(CMdbRecord* pRecord);
	bool BindPara(CMdbRecord* pRecord);
};

class COdbcSelect
{
	union CMdbValue
	{
		int8 i8;
		int16 i16;
		int32 i32;
		int64 i64;
		uint8 u8;
		uint16 u16;
		uint32 u32;
		uint64 u64;
		float f32;
		double f64;
		SQL_DATE_STRUCT oDate;
		SQL_TIME_STRUCT oTime;
		SQL_TIMESTAMP_STRUCT oDateTime;
		uint8 s[1];
	};
	struct XItem
	{
		//SQLINTEGER nLen;
		SQLLEN nLen;
		CMdbValue oValue;
	};
private:
	COdbcConnect* m_pDb;
	CMdbTableDef* m_pTabDef;
	void* m_pSentence;
	bool m_bError;
	uint32 m_nFieldCount, &m_nCount, *m_pFields;
	uint32 *m_pOffset, *m_pBufSize;
	SQLLEN m_nRecordSize;
	uint8* m_pBuf;
	CMdbTableAccess* m_pAccess;
	const char* m_sDbName;

public:
	COdbcSelect(COdbcConnect* pDb, CMdbTableDef* pTabDef, CMdb* pMdb, uint32 &nCount);
	virtual ~COdbcSelect();

	void Exec();
	bool HaveError();

private:
	bool PrepareSql();
	bool BindPara();
	uint32 GetItemSize(uint32 nFieldNo, uint32 &nAlignSize);
	uint32 ComputeBuffer(uint32& nBufSize, uint32 nFieldNo);
	bool BindCol(CMdbStorageFieldAttr* pFieldAttr, uint32 nBindCol);
};

class CMdbOdbc: public CCooperateFunction
{
private:
	COdbcConnect m_oLink;
	CMdb *m_pMdb, *m_pEventDb;
	CMdbAccess* m_pEventAccess;
	CCooperator m_oWorkerThread;
	CEvent m_oWorkEvent;
	uint32 m_nFields[4];
	bool m_bWork, m_bSupportStorage;

public:
	CMdbOdbc(void* pOdbcEnv);
	~CMdbOdbc();

	COdbcConnect* GetLink();

	bool Initialize(const char* sDSN, const char* sUser, const char* sPasswd, bool bSupportStorage,
					const char* sMdbName, const char* sEventDb=MDB_SYSDB_NAME, const char* sEventTable=NULL);
	void Cleanup();

	//Start之前做的事情。
	bool Load();

	void Start();
	void Stop(bool bBlock=true);

	void StartStorage();
	void StopStorage();

	CMdbAccess* GetEventAccess();

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private: //表名、主键、操作
	bool GetEvent(CString& oTableName, CString& oPrimaryKey, uint32 &nOp);
	void DelEvent(const CString& oTableName, const CString& oPrimaryKey);
	void ProcEvent(const CString& oTableName, const CString& oPrimaryKey, uint32 nOp);
};

struct CMdbStorageTableAttr
{
	CString oTableName;
	CString oLoadWhere;
	CMdbParaSet* pStorageWhere;
	CMdbParaSet* pCacheWhere;
	CMdbIndexDef* pStorageIdx;
	FStorageTrigger StorageTrigger;
	FStorageTrigger CacheTrigger;
};

struct CMdbStorageFieldAttr
{
	CString oFieldName;
	SQLSMALLINT nValueType, nParaType, nDecimalDigits;
	SQLUINTEGER nColumnSize;
//	SQLINTEGER nBufSize, nLen;
	SQLLEN nBufSize, nLen;
	SQL_DATE_STRUCT oDate;
	SQL_TIME_STRUCT oTime;
	SQL_TIMESTAMP_STRUCT oDateTime;
	bool bIdxField;
};

enum
{
	MDB_EVENT_INSERT = 1,
	MDB_EVENT_DELETE = 2,
	MDB_EVENT_UPDATE = 3,
	MDB_EVENT_TRUNCATE = 4,

	MDB_STORAGEKEY_MAXSIZE = 256,
	MDB_FASTFETCH_SIZE = 1024,
};

class CMdbStorageToken: public CAcmToken
{
public:
	CMdbStorageToken(uint32 nDomain);
	virtual ~CMdbStorageToken();

protected:
	virtual void OnRoleChange(bool bTakeUp);
};

class CMdbStorager: public CMdbExtPlugIn, public CMdbFirstNode
{
	friend class CMdbStorageToken;

	struct CMdbDomainItem
	{
		uint32 bLoaded;
		CSingleList<CString> oMdbList;
		CMdbStorageToken* pToken;
	};
private:
	void* m_pOdbcEnv;
	CRbMap<CString, CMdbOdbc*, CNameCompare> m_oLinks;
	CRbMap<uint32, CMdbDomainItem> m_oDomainMdbTable;

public:
	CMdbStorager();
	~CMdbStorager();

	static CMdbStorager* GetInstance();

	bool InitializeStorageAttr(const char* sMdbName, const char* sMdbTabName,
							   const char* sDbTabName, const char* sLoadWhere, const char* sStorageWhere, const char* sCacheWhere, 
							   const char* sStorageIdx, const char* sFieldList);
	bool InitializeDataSource(const char* sMdbName, uint32 nDomain, bool bSupportStorage,
							  const char* sDSN, const char* sUser, const char* sPasswd,
							  const char* sEventDb=MDB_SYSDB_NAME, const char* sEventTable=NULL);

	void SetStorageTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger StorageTrigger);
	void SetCacheTrigger(const char* sMdbName, const char* sMdbTabName, FStorageTrigger CacheTrigger);

	void Cleanup();

	virtual bool OnFirstNode(uint32 nDomain);
	virtual void OnOtherNode(uint32 nDomain);

	bool Start();
	void Stop(bool bBlock=true);

	//CMdbExtPlugIn
	virtual void OnFree(void* pAttr, uint32 nAttrType/*0=字段属性，1表属性，2索引属性*/);

private:
	void StartStorage(uint32 nDomain);
	void StopStorage(uint32 nDomain);

	static void OnInsertAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnDeleteAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnUpdateAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnTruncateAfter(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	void OnInsertAfter(CMdbAccess* pAccess, uint32 nCaller);
	void OnDeleteAfter(CMdbAccess* pAccess, uint32 nCaller);
	void OnUpdateAfter(CMdbAccess* pAccess, uint32 nCaller);
	void OnTruncateAfter(CMdbAccess* pAccess, uint32 nCaller);
	void CreatePrimaryKey(CString &oPrimaryKey, CMdbIndexDef* pStorageIdx, CMdbRecord* pRecord);
	void OnDeleteAfter(CMdbStorageTableAttr* pTabStoAttr, CMdbAccess* pAccess, CMdbRecord* pRecord);
	void OnInsertAfter(CMdbStorageTableAttr* pTabStoAttr, CMdbAccess* pAccess, CMdbRecord* pRecord);

	CMdbAccess* GetEventAccess(const char* sDbName);
};

FOCP_END();

#endif
