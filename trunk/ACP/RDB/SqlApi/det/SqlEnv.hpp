
#include "SqlApi.hpp"
#include "VmmApi.hpp"

#ifndef _SQL_ENV_HPP_
#define _SQL_ENV_HPP_

#if defined(RDB_SUPPORT_DISK_DB) && defined(RDB_SUPPORT_MEMORY_DB)
#include "SqlYacc.3.hpp"
#elif defined(RDB_SUPPORT_MEMORY_DB)
#include "SqlYacc.1.hpp"
#elif defined(RDB_SUPPORT_DISK_DB)
#include "SqlYacc.2.hpp"
#endif

#include "FlexLexer.h"
#include "SqlBuf.hpp"
using namespace std;

FOCP_BEGIN();

class CSqlEnv;

//-----------------------------------------------------------------
// CSqlField
//-----------------------------------------------------------------
struct CSqlField
{
	CString name;
	int seq;
	int type;
	int len;
	int recsize;
	bool null;
	CString defval;

	CSqlField();
};

struct CSqlGetFieldName
{
	static const CString* GetKey(const CSqlField& oData);
};

//-----------------------------------------------------------------
// CSqlTable
//-----------------------------------------------------------------
class CSqlTable
{
	friend class CSqlDataBase;
private:
	CRbTree<CString,CSqlField,CSqlGetFieldName,CNameCompare> m_oFieldTable;
	uint32 m_nFldNo;
	CString m_oName;
	uint32 m_nStorage;
	CSqlField* m_pField;

public:
	CSqlTable();

#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
	void Bind(const CString& oName, uint32 nStorageType);
#else
	void Bind(const CString& oName);
#endif
	CSqlField* AddField(const CString& oName);
	CSqlField* GetCurrentField();
};

//-----------------------------------------------------------------
// CSqlOperator
//-----------------------------------------------------------------
class CSqlOperator
{
	friend class CSqlDataBase;
protected:
	CString m_oName;
	CRdbAccess * m_pAccess;

public:
	CSqlOperator();
	virtual ~CSqlOperator();

	virtual void Bind(const CString &oName, CRdbAccess * pAccess);
	void UnBind();
};

//-----------------------------------------------------------------
// CSqlInsert
//-----------------------------------------------------------------
class CSqlInsert: public CSqlOperator
{
	friend class CSqlDataBase;
private:
	CRdbPara * m_pInsertAttr;
	CVector<CString> m_oFieldTable;
	uint32 m_nFldNo;
	CSqlEnv* m_pEnv;

public:
	CSqlInsert(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CRdbAccess * pAccess);
	bool AddValue(const CString& oVal);
	void AddField(const CString& oName);
};

//-----------------------------------------------------------------
// CSqlWhere
//-----------------------------------------------------------------
class CSqlWhere: public CSqlOperator
{
protected:
	CRdbParaSet* m_pWhere;
	CSqlEnv* m_pEnv;

public:
	CSqlWhere(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CRdbAccess * pAccess);
	bool AddCond(const CString& oName, uint32 nOp, const CString& oVal);
};

//-----------------------------------------------------------------
// CSqlDelete
//-----------------------------------------------------------------
typedef CSqlWhere CSqlDelete;

//-----------------------------------------------------------------
// CSqlUpdate
//-----------------------------------------------------------------
class CSqlUpdate: public CSqlWhere
{
	friend class CSqlDataBase;
private:
	CRdbPara* m_pSetAttr;

public:
	CSqlUpdate(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CRdbAccess * pAccess);
	bool AddSetPara(const CString& oName, const CString& oVal);
};

//-----------------------------------------------------------------
// CSqlSelect
//-----------------------------------------------------------------
class CSqlSelect: public CSqlWhere
{
	friend class CSqlDataBase;
private:
	CRdbResultSet* m_pRecordSet;
	CVector<CString> m_oFieldTable;

public:
	CSqlSelect(CSqlEnv * pEnv);

	void Clear();

	void AddField(const CString& oName);

	uint32 GetResultCount();
	CRdbResult* GetRecord(uint32 nRow);
};

//-----------------------------------------------------------------
// CSqlIndex
//-----------------------------------------------------------------
class CSqlIndex
{
public:
	CSqlIndex();

	uint32 nQualifier, nArithmetic, nHashRate;
	CString oIndexName, oTableName, oPrimaryIndex;
	CVector<CString> oIndexFieldTable;
};

//-----------------------------------------------------------------
// CSqlDataBase
//-----------------------------------------------------------------
class CSqlDataBase
{
	friend class CSqlEnv;
	friend int yyparse(void* lexer);
private:
	CSqlEnv* m_pEnv;
	CRdb * m_pDb;
	CSqlTable m_oTable;
	CSqlIndex m_oIndex;
	CSqlInsert m_oInsert;
	CSqlDelete m_oDelete;
	CSqlUpdate m_oUpdate;
	CSqlSelect m_oSelect;
	CSqlWhere* m_pWhere;
	uint32 m_nMaxRollBack;
	uint32 m_nResultSize;
	uint32 m_nRepeat;

public:
	CSqlDataBase(CSqlEnv* pEnv);
	~CSqlDataBase();

	void Bind(CRdb* pDb);

	void SetRepeat(uint32 nRepeat);

#if defined(RDB_SUPPORT_MEMORY_DB) && defined(RDB_SUPPORT_DISK_DB)
	uint32 CreateSqlTable(const CString& oName, uint32 nStorageType);
#else
	uint32 CreateSqlTable(const CString& oName);
#endif
	uint32 CreateTable(bool bModify=false);
	uint32 DropTable(const CString& oName);

	uint32 CreateSqlIndex();
	uint32 CreateIndex();
	uint32 DropIndex(const CString& oName);

	uint32 CreateSqlInsert(const CString& oName);
	uint32 InsertRecord();

	uint32 CreateSqlDelete(const CString& oName);
	uint32 DeleteRecord();

	void SetMaxRollBack(uint32 nMaxRollBack);
	uint32 CreateSqlUpdate(const CString& oName);
	uint32 UpdateRecord();

	uint32 TruncateTable(const CString& oName);

	void SetResultSize(uint32 nResultSize);
	void PrepareSqlSelect();
	uint32 CreateSqlSelect(const CString& oName);
	CString GetFieldList();
	uint32 QueryTable();
	CString GetRecordText(CRdbResult* pRecord, CString& oFieldList);

	uint32 DescTable(char* sTableName = NULL);
	CString GetTypeName(uint32 nType, uint32 nLen, uint32 nRecSize);
	CString GetArithmeticName(uint32 nArithmetic, uint32 nHashRate);
};

//-----------------------------------------------------------------
// CSqlReadWriteObject
//-----------------------------------------------------------------
struct CFileTerminal
{
	CFile fp;
	CString oLine;
};
struct CFileTerminalList
{
	CSqlTerminal* pTerminal;
	CFileTerminalList* pNext;
};

class CSqlReadWriteObject
{
private:
	CSqlTerminal* m_pTerminal;
	char * m_sReadBuf;
	uint32 m_nReadSize;
	uint32 m_nReadBufSize, m_nWriteBufSize;	
	CVector<CString> m_oText;
	CFileTerminalList* m_pTerminalList;
	CSqlEnv* m_pEnv;

public:
	CSqlReadWriteObject(CSqlTerminal* pTerminal, CSqlEnv* pEnv);
	~CSqlReadWriteObject();

	int write(const char* s, uint32 size);
	int read(char* s, uint32 size);
	int readbufsize();
	int writebufsize();

	bool PushFile(char* sFileName);

	void Flush(bool bCarriage=true);
};

//-----------------------------------------------------------------
// CSqlEnv
//-----------------------------------------------------------------
typedef CRwStream<char, CSqlReadWriteObject> CSqlStream;
class CSqlEnv: public yyFlexLexer
{
private:
	CSqlDataBase m_oDataBase;
	CRwStream<> m_oOutStream;
	CSqlTerminal m_oTerminal;
	CSqlReadWriteObject m_oRwObj;
	CSqlStream m_oStream;
	char* m_sDataBakPath, * m_sDataBakPath2;

public:
	CSqlEnv(CSqlTerminal* pTerminal, char* sDataBakPath, char* sDataBakPath2);
	virtual ~CSqlEnv();

	CSqlDataBase* GetDataBase();

	CSqlStream& GetStream();

	uint32 CreateDataBase(const CString& oName);
	uint32 RemoveDataBase(const CString& oName);
	uint32 SelectDataBase(const CString& oName);
	uint32 DescAllDataBase();
	void Backup();

	void Help();

	void Flush(bool bCarriage=true);

	void Interpret();

	void ExecuteFile(char* sScriptFile);
};

void SqlLog(void* pSqlEnv, uint32 nLogLevel, const char* sFormat, ...);

FOCP_END();

#endif
