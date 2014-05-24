
/*
MDB�־û�������Ʒ�����
��1�����ñ�׼��ϵ�ʹ������ݿ�ϵͳ��ΪMDB�����ݳ־û��洢�豸���Դ�ʵ��MDB�ĳ־û���
��2��MDB���ϵ�ʹ������ݿ�ԽӲ��ñ�׼��ODBC�ӿڣ��Դ˱�������Ľӿڹ�����
��3����Ҫ���ı���Ҫ��չһ�����ԣ���֧�ֳ־û���Ҫ��
	��A����һ������չ���ԣ�
		���ݿ��������������ͼ����Ϊ�ձ�ʾ���ڴ������ͬ��
		����������MDB����ʱ�����������������Ĳ��ּ�¼
		�洢�������������ʱ�������洢���������ļ�¼
		���������������ݱ仯ʱ�������ж��Ƿ���Ҫ�������ڴ���
		����������
		����Ϊ֧��ODBC����չ�����ԡ�
	��B���ֶ���չ���ԣ�
		���ݿ��ֶ�����Ϊ�ձ�ʾ���ڴ���ֶ�����ͬ���64���ַ���
		����Ϊ֧��ODBC����չ�����ԡ�
��4���־û�֧�ֵĲ�����
	���롢�޸ġ�ɾ���������
��5�����һ��ϵͳ���Դ洢�־û�������Ϣ��ϵͳ��ʱɨ����¼����������ݱ��ͨ��ODBC���õ�
	�������ݿ�ϵͳ�С�
��6���¼���ṹ�������£�
	����������������������ʱ��[���ֶ���������]��
	��A�����롢�޸ġ�ɾ��������Լ�¼����(�����ǿ�)������Ǳ���(����Ϊ��)��
	��B���������������ʱ����ɾ���ñ�֮ǰ�����в���������������¼���
	��C����ɾ����������ʱ��
		��a��������ھ��¼�����ɾ�����¼���
		��b��������¼�Ϊ�����������Ҫ���Ե�ǰ�¼���
		��c��������뵱ǰɾ���¼���
	��D���������������ʱ��
		��a������¼����д���ɾ���¼���ɾ���ü�¼�������޸��¼���
		��b���������һ���µĲ����¼���
	��E�����޸Ĳ�������ʱ��������λ�ڼ�¼����ϸ�����ֶΡ�
		��a������ɼ�¼���ϴ洢���������¼�¼�����ϴ洢������תɾ�����̡�
		��b������ɼ�¼�����ϴ洢���������¼�¼���ϴ洢������ת�������̡�
		��c������޸��˴洢��������������ɾ��������¼���
		��d������¼����д��ڲ��������ɾ���ü�¼�������²��룬�¼����Ͳ��䡣
		��e������¼����д����޸Ĳ�����ɾ���ü�¼�������²��룬�¼����Ͳ��䡣
		��f����������޸��¼�
	��F��������/�޸��¼�����ʱ��
		��a�������¼�����ϻ������������Cache��¼��
��7��һ��ACM����԰���������ݿ⣬һ�����ݿ�ֻ������ĳһ���ض���ACM����������ݸ��Ƽ�����ͬ��
	һ�����ݿ��Ӧһ��ODBC��·����ͬODBC��·����ָ��ͬһ����Դ��
	һ�����ݿ��Ӧһ���¼���
	һ������ֻ����һ���ڵ��ܽ��¼����е��¼������ڴ������ݿ�ϵͳ��ͨ�����ƾ���������ʵ�֡�
	�¼���֧�����ݸ�����������ô�����ĸ��㾺�������񣬶�����ȷ�İ����ݸ��µ��������ݿ��С�
��8���¼������������������ģ�ͣ�
	��A��Ӧ�ñ����仯ʱ����After�������У������¼���ת(6)��(BCDE)���̡�
	��B������ڵ�ɨ����¼���һ����¼����������⣬���ɹ���ɾ�����¼���¼��
��9���¼����������ֶ����Լ����
	��������MDB SQL��WHERE�������ı���ʽ���������256�ֽڣ������������¼��־�����������¼���
��10��֧������ʱ�����ݼ��ء�
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

	bool Connect();//������
	void Disconnect();

	//�������ӳɹ�����øú������Գ�ʼ��������ԡ�
	bool InitializeTable(const char* sDbName, CMdbTableDef* pTabDef);

	bool IsBroken();//ִ����COdbcSentence::Exec������øú��������·״̬��

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

	//Start֮ǰ�������顣
	bool Load();

	void Start();
	void Stop(bool bBlock=true);

	void StartStorage();
	void StopStorage();

	CMdbAccess* GetEventAccess();

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private: //����������������
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
	virtual void OnFree(void* pAttr, uint32 nAttrType/*0=�ֶ����ԣ�1�����ԣ�2��������*/);

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
