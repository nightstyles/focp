
#include "SqlApi.hpp"

#ifndef _SQL_ENV_HPP_
#define _SQL_ENV_HPP_

#ifdef MSVC
#pragma warning(disable:4065)
#endif

#include "SqlYacc.hpp"
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
	uint32 m_nMaxRecordNum;
	CString m_oName;
	CSqlField* m_pField;

public:
	CSqlTable();

	void Bind(const CString& oName);

	void SetMaxRecordNum(uint32 nMaxRecordNum);

	CSqlField* AddField(const CString& oName);
	CSqlField* GetCurrentField();
	CSqlField* FindField(const CString& oName);
};

//-----------------------------------------------------------------
// CSqlOperator
//-----------------------------------------------------------------
class CSqlOperator
{
	friend class CSqlDataBase;
protected:
	CString m_oName;
	CMdbAccess * m_pAccess;
	bool m_bError;

public:
	CSqlOperator();
	virtual ~CSqlOperator();

	virtual void Bind(const CString &oName, CMdbAccess * pAccess);
	void UnBind();
};

//-----------------------------------------------------------------
// CSqlInsert
//-----------------------------------------------------------------
class CSqlInsert: public CSqlOperator
{
	friend class CSqlDataBase;
private:
	CMdbPara * m_pInsertAttr;
	CVector<CString> m_oFieldTable;
	uint32 m_nFldNo;
	CSqlEnv* m_pEnv;

public:
	CSqlInsert(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CMdbAccess * pAccess);
	bool AddValue(const CString& oVal);
	void AddField(const CString& oName);
};

//-----------------------------------------------------------------
// CSqlWhere
//-----------------------------------------------------------------
class CSqlWhere: public CSqlOperator
{
protected:
	CMdbParaSet* m_pWhere;
	CSqlEnv* m_pEnv;

public:
	CSqlWhere(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CMdbAccess * pAccess);
	void NewSet();
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
	CMdbPara* m_pSetAttr;

public:
	CSqlUpdate(CSqlEnv * pEnv);

	virtual void Bind(const CString &oName, CMdbAccess * pAccess);
	bool AddSetPara(const CString& oName, const CString& oVal, uint32 nOp);
};

//-----------------------------------------------------------------
// CSqlSelect
//-----------------------------------------------------------------
class CSqlSelect: public CSqlWhere
{
	friend class CSqlDataBase;
private:
	CMdbResultSet* m_pRecordSet;
	CVector<CString> m_oFieldTable;

public:
	CSqlSelect(CSqlEnv * pEnv);

	void Clear();

	void AddField(const CString& oName);

	uint32 GetResultCount();
	CMdbResult* GetRecord(uint32 nRow);
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
	CMdb * m_pDb;
	CSqlTable m_oTable, m_oAliasTable, *m_pCurrentTable;
	CSqlIndex m_oIndex;
	CSqlInsert m_oInsert;
	CSqlDelete m_oDelete;
	CSqlUpdate m_oUpdate;
	CSqlSelect m_oSelect;
	CSqlWhere* m_pWhere;
	uint32 m_nResultSize;
	uint32 m_nRepeat;

public:
	CSqlDataBase(CSqlEnv* pEnv);
	~CSqlDataBase();

	void Bind(CMdb* pDb);

	void SetRepeat(uint32 nRepeat);

	uint32 CreateSqlTable(const CString& oName);
	uint32 CreateTable();

	uint32 CreateSqlIndex();
	uint32 CreateIndex();

	uint32 CreateSqlInsert(const CString& oName);
	uint32 InsertRecord();

	uint32 CreateSqlDelete(const CString& oName);
	uint32 DeleteRecord();

	uint32 CreateSqlUpdate(const CString& oName);
	uint32 UpdateRecord();

	uint32 TruncateTable(const CString& oName);

	void OrderBy(const CString& oIdxName, bool bAsc=true);

	void SetResultSize(uint32 nResultSize);
	void PrepareSqlSelect();
	uint32 CreateSqlSelect(const CString& oName);
	CString GetFieldList();
	uint32 QueryTable();
	CString GetRecordText(CMdbResult* pRecord, CString& oFieldList);

	uint32 DescTable(char* sTableName = NULL);
	CString GetTypeName(uint32 nType, uint32 nLen);
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

public:
	CSqlEnv(CSqlTerminal* pTerminal);
	virtual ~CSqlEnv();

	CSqlDataBase* GetDataBase();

	CSqlStream& GetStream();

	uint32 SelectDataBase(const CString& oName);
	uint32 DescAllDataBase();

	void Help();

	void Flush(bool bCarriage=true);

	void Interpret();

	void ExecuteFile(char* sScriptFile);
};

FOCP_END();

#endif
