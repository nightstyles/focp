
#include "MdbAccess.hpp"

#ifndef _MDB_REP_HPP_
#define _MDB_REP_HPP_

#if defined(MDBREP_EXPORTS)
#define MDBREP_API FOCP_EXPORT
#else
#define MDBREP_API FOCP_IMPORT
#endif

FOCP_C_BEGIN();

MDBREP_API bool MdbTransfer();
MDBREP_API bool ConfigMdbDomain(const char* sDbName, FOCP_NAME::uint32 nDomain);
MDBREP_API bool CreateMdbReplicator(const char* sDbName, const char* sTableName, const char* sTransferIdx, bool bTableReplicative, bool bWithout, const char* sFields, const char* sCond);
MDBREP_API bool RegisterMdbTransfer();
MDBREP_API void CleanupMdbReplication();

FOCP_C_END();

FOCP_BEGIN();

/*
	create replicator on dbname [in domain];
	create replicator on db.tablename[[!](fields)] [where cond];
*/
struct CMdbReplicateTableAttr
{
	bool bReplicativeTable, bReplicativeFields;
	CMdbIndexDef* pTransferIdx;
	CMdbParaSet* pWhere;
};

enum
{
	//REPLICATE
	MDB_REPLICATE_INSERT = 0x9988,
	MDB_REPLICATE_DELETE = 0x8877,
	MDB_REPLICATE_UPDATE = 0x7766,
	MDB_REPLICATE_TRUNCATE = 0x6655,
	//TRANSFER
	MDB_TRANSFER_INSERT_REQUEST = 0x99887766,
	MDB_TRANSFER_INSERT_RESPONSE = 0x88776655,
};

class CMdbReplicator:
	public CAsmPlugIn, //处理数据复制消息
	public CAcmTcpModule,//处理数据传输请求
	public CAcmTcpClientContext, //发起出具传输请求
	public CMdbExtPlugIn
{
private:
	CRbMap<CMdb*, CAcmSequenceModule*> m_oMdbUdpTable;
	CRbMap< CAcmSequenceModule*, CSingleList<CMdb*> > m_oUdpMdbTable;

public:
	CMdbReplicator();
	virtual ~CMdbReplicator();

	//外部接口
	static CMdbReplicator* GetInstance();
	bool Transfer();
	bool ConfigDomain(uint32 nDomain, const char* sDbName);
	bool CreateReplicator(const char* sDbName, const char* sTableName, const char* sTransferIdx,
						  bool bTableReplicative, const char* sFields, bool bWithout, const char* sCond);
	bool RegTransferServer();
	void Cleanup();

	//CAcmTcpModule
	virtual void ProcessAcmModuleMsg(CAcmTcpLink* pLink, uint32 nCmd, CMemoryStream& oStream);

	//CAcmTcpClientContext
	virtual bool OnReConnect(CAcmTcpClient* pClient);
	virtual uint32 OnLogin(CAcmTcpClient* pClient, bool bReLogin);
	virtual void ProcessMsg(CAcmTcpClient* pClient, CTcpHead& oHead, CMemoryStream &oStream);

	//CMdbExtPlugIn
	virtual void OnFree(void* pAttr, uint32 nAttrType/*0=字段属性，1表属性，2索引属性*/);

protected:
	virtual void ProcessAsmPlugInMsg(CAcmSequenceModule* pModule, CAsmDataMsg& oMsg);

private:
	bool WriteString(CMemoryStream &oStream, const char* sStr);
	bool ReadString(CMemoryStream &oStream, CString &oStr);
	//Trigger
	void OnInsert(CMdbAccess* pAccess, uint32 nCaller);
	void OnDelete(CMdbAccess* pAccess, uint32 nCaller);
	void OnUpdate(CMdbAccess* pAccess, uint32 nCaller);
	void OnTruncate(CMdbAccess* pAccess, uint32 nCaller);
	bool OnDeleteBefore(CMdbAccess* pAccess, uint32 nCaller);
	bool OnUpdateBefore(CMdbAccess* pAccess, uint32 nCaller);

	static void OnInsert(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnDelete(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnUpdate(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static void OnTruncate(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static bool OnDeleteBefore(CMdbAccess* pAccess, uint32 nCaller, void* pContext);
	static bool OnUpdateBefore(CMdbAccess* pAccess, uint32 nCaller, void* pContext);

	//CAsmPlugIn
	void ProcessRepInsert(CAsmDataMsg& oMsg);
	void ProcessRepDelete(CAsmDataMsg& oMsg);
	void ProcessRepUpdate(CAsmDataMsg& oMsg);
	void ProcessRepTruncate(CAsmDataMsg& oMsg);
	//CAcmTcpModule
	void TransferTable(CAcmTcpLink* pLink, CMdb* pDb, char* sTableName);
	void TransferRecord(CAcmTcpLink* pLink, CMdb* pDb, CMdbTableAccess* pAccess, CMdbRecord* pRecord, CMdbSqlFilter* pFilter);
	//CreateReplicator
	void SkipWhiteSpace(const char* &pStr);
	uint32 GetIdentifier(const char* &pStr, CString &oIdentifier);
	uint32 GetOperator(const char* &pStr, uint32 &nOp);
	uint32 GetValue(const char* &pStr, CString &oValue);
	void NewSet(CMdbSqlParaSet* pParaSet);
	void AddCond(CMdbSqlParaSet* pParaSet, uint32 nCol, uint32 nOp, const CString& oVal);
	//Transfer
	void SendTransferRequest(CAcmTcpClient* pClient, CMdb* pDb);
};

FOCP_END();

#endif
