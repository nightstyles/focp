%{
#include "SqlEnv.hpp"

#include <stdlib.h>

void CopyName(char* d, const char* s);

FOCP_BEGIN();

#define YYPARSE_PARAM lexer

void yyerror(const char* msg);

%}

%pure-parser

%union {
	bool 	b;
	int 	i;
	char* 	sv;
	char	nam[FOCP_NAME::RDB_NAME_MAXLEN+1];
	double 	d;
};

%token <nam> IDENTIFIER
%token TINT8 TINT16 TINT32 TINT64 
%token TUINT8 TUINT16 TUINT32 TUINT64
%token TFLOAT TDOUBLE
%token <sv> TSTRING CSTRING VSTRING VCSTRING
%token RAW VRAW
%token TSIZE RECSIZE
%token DEFAULT BACKUP
%token NOT TNULL
%token <sv> TNUMBER
%token <sv> XSTRING
%token UNIQUE FOREIGN
%token RBTREE NTREE HASH
%token REPEAT
%token CREATE
%token DEVICE
%token DATABASE
%token TABLE
%token TEMPORARY
%token MODIFY
%token DROP
%token INDEX
%token ON BY
%token INSERT
%token INTO
%token VALUES
%token COMMENT
%token USE
%token TDELETE
%token FROM
%token WHERE
%token AND
%token UPDATE
%token SET
%token <nam> VCOUNT
%token TRUNCATE
%token SELECT
%token DESC
%token QUIT
%token HELP
%token <sv> BATCH
%token GE LE EQ MT LT
%token TSQLERROR

%type <i> basetype
%type <sv> value
%type <i> operator
%type <i> devicesize

%start stmtlist

%%

def_field: IDENTIFIER
	{
		if(pDb)
			pDb->m_oTable.AddField($1);
	}
;

def_notnull: /* empty */
| NOT TNULL
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->null = true;
		}
	}
;

def_size: TSIZE EQ TNUMBER
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->len = strtod($3, NULL);
		}
		free($3);
	}
;

def_recsize: /* empty */
| RECSIZE EQ TNUMBER
	{
		if(pDb)
		{ 
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->recsize = strtod($3, NULL);
		}
		free($3);
	}
;

def_default: /* empty */
| DEFAULT EQ XSTRING
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->defval = $3;
		}
		free($3);
	}
;

field: def_field def_datatype def_notnull def_default ;

fields: /* empty */
| field
| fields comment
| fields ',' field
;

basetype: TINT8 
	{ $$ = RDB_INT8_FIELD; }
| TINT16
	{ $$ = RDB_INT16_FIELD; }
| TINT32 
	{ $$ = RDB_INT32_FIELD; }
| TINT64 
	{ $$ = RDB_INT64_FIELD; }
| TUINT8 
	{ $$ = RDB_UINT8_FIELD; }
| TUINT16 
	{ $$ = RDB_UINT16_FIELD; }
| TUINT32 
	{ $$ = RDB_UINT32_FIELD; }
| TUINT64
	{ $$ = RDB_UINT64_FIELD; }
| TFLOAT 
	{ $$ = RDB_FLOAT_FIELD; }
| TDOUBLE
	{ $$ = RDB_DOUBLE_FIELD; }
;

def_datatype: basetype
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = $1;
		}
	}
| TSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_CHAR_FIELD;
		}
	}
| CSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_LCHAR_FIELD;
		}
	}
| RAW def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_RAW_FIELD;
		}
	}
| VSTRING def_size def_recsize
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_VARCHAR_FIELD;
		}
	}
| VCSTRING def_size def_recsize
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_VARLCHAR_FIELD;
		}
	}
| VRAW def_size def_recsize
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_oTable.GetCurrentField();
			if(pField)
				pField->type = RDB_VARRAW_FIELD;
		}
	}
;

create_memorydevice: CREATE TEMPORARY DEVICE TNUMBER ';'
	{
		CreateMemoryDevice((uint32)atoi($4));
		pSqlEnv->Flush();
		free($4)
	}
;

create_filedevice: CREATE DEVICE XSTRING devicesize ';'
	{
		CreateFileDevice($4, $3);
		pSqlEnv->Flush();
		free($3);		
	}
;

devicesize: /* empty */
	{
		$$ = 1;
	}
| TNUMBER
	{
		$$ = atoi($1);
		free($1);
	}
;

usedatabase: USE IDENTIFIER ';'
	{
		pSqlEnv->SelectDataBase($2);
		pSqlEnv->Flush();
		pDb = pSqlEnv->GetDataBase();
	}
;

createdatabase: CREATE DATABASE IDENTIFIER ';'
	{
		pSqlEnv->CreateDataBase($3);
		pSqlEnv->Flush();
	}
;

dropdatabase: DROP DATABASE IDENTIFIER ';'
	{
		pSqlEnv->RemoveDataBase($3);
		pSqlEnv->Flush();
	}
;

createtable0: CREATE TABLE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->CreateSqlTable($3);
	}
'(' fields ')' ';'
;

createtable: createtable0
	{
		if(pDb)
			pDb->CreateTable();
		pSqlEnv->Flush();
	}
;

modifytable0: MODIFY TABLE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->CreateSqlTable($3);
	}
'(' fields ')' ';'
;

modifytable: modifytable0
	{
		if(pDb)
			pDb->CreateTable(true);
		pSqlEnv->Flush();
	}
;

droptable: DROP TABLE IDENTIFIER ';'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->DropTable($3);
		pSqlEnv->Flush();
	}
;

insfld: IDENTIFIER
	{
		if(pDb)
			pDb->m_oInsert.AddField($1);
	}
;

insflds: insfld
| insflds ',' insfld
;

insfldlist: /* empty */
	{ }
| '(' insflds ')'
;

value: TNUMBER
	{ $$ = $1; }
| XSTRING
	{ $$ = $1; } 
;

insvalue0: /* empty */
| value
	{
		if(pDb)
			pDb->m_oInsert.AddValue($1);
		free($1);
	}
;

insvalue: insvalue0
| insvalue ',' insvalue0
;

insert0: INSERT INTO IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else 
			pDb->CreateSqlInsert($3);
	}
insfldlist VALUES '(' insvalue ')' ';'
;

insert: insert0
	{
		if(pDb)
			pDb->InsertRecord();
		pSqlEnv->Flush();
	}
;

delete0: TDELETE FROM IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->CreateSqlDelete($3);
	}
;

delete1: delete0 ';'
| delete0 WHERE wherecond ';'
;

wherecond0: IDENTIFIER operator value
	{
		if(pDb)
		{
			pDb->m_pWhere->AddCond($1, $2, $3);
			free($3);
		}
	}
;

wherecond: wherecond0
| wherecond logicop wherecond0
;

logicop: AND
;

operator: GE
	{ $$ = RDB_SQLPARA_OPERATOR_MOREEQUAL; }
| LE
	{ $$ = RDB_SQLPARA_OPERATOR_LESSEQUAL; }
| LT
	{ $$ = RDB_SQLPARA_OPERATOR_LESS; }
| MT
	{ $$ = RDB_SQLPARA_OPERATOR_MORE; }
| EQ
	{ $$ = RDB_SQLPARA_OPERATOR_EQUAL; }
;

delete: delete1
	{
		if(pDb)
			pDb->DeleteRecord();
		pSqlEnv->Flush();
	}
;

update0: UPDATE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->CreateSqlUpdate($2);
	}
SET setexpr
;

update1: update0 ';'
| update0 WHERE wherecond ';'
;

setexpr0: IDENTIFIER EQ value
	{
		if(pDb)
			pDb->m_oUpdate.AddSetPara($1, $3);
		free($3);
	}
;

setexpr: setexpr0
| setexpr ',' setexpr0
;

update: update1
	{
		if(pDb)
			pDb->UpdateRecord();
		pSqlEnv->Flush();
	}
;

truncate: TRUNCATE TABLE IDENTIFIER ';'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->TruncateTable($3);
		pSqlEnv->Flush();
	}
;

select0: SELECT
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->PrepareSqlSelect();
	}
selflds FROM IDENTIFIER
	{
		if(pDb)
			pDb->CreateSqlSelect($5);
	}
;

select1: select0 ';'
| select0 WHERE wherecond ';'
;

select: select1
	{
		if(pDb)
			pDb->QueryTable();
		pSqlEnv->Flush();
	}
;

selspecfld: IDENTIFIER 
	{
		if(pDb)
			pDb->m_oSelect.AddField($1);
	}
| selspecfld ',' IDENTIFIER
	{
		if(pDb)
			pDb->m_oSelect.AddField($3);
	}
;

selflds: '*' | selspecfld ;

comment: COMMENT
;

desc: DESC TABLE IDENTIFIER ';'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->DescTable($3);
		pSqlEnv->Flush();
	}
| DESC TABLE '*' ';'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->DescTable();
		pSqlEnv->Flush();
	}
| DESC DATABASE '*' ';'
	{
		pSqlEnv->DescAllDataBase();
		pSqlEnv->Flush();
	}
;

createindex0: CREATE indexattr foreignattr INDEX IDENTIFIER ON IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
		{
			pDb->m_oIndex.oIndexName = $5;
			pDb->m_oIndex.oTableName = $7;
			pDb->m_oIndex.oIndexFieldTable.Clear();
		}
	}
'(' indexfld ')' indexalgo ';'
;

createindex: createindex0
	{
		if(pDb)
			pDb->CreateIndex();
		pSqlEnv->Flush();
	}
;

indexattr: /* empty */
	{
		if(pDb)
			pDb->m_oIndex.nQualifier = RDB_COMMON_INDEX;
	}
| UNIQUE
	{
		if(pDb)
			pDb->m_oIndex.nQualifier = RDB_UNIQUE_INDEX;
	}
;

foreignattr: /* empty */
	{
		if(pDb)
			pDb->m_oIndex.oPrimaryIndex = "";
	}
| FOREIGN '(' IDENTIFIER ')'
	{
		if(pDb)
			pDb->m_oIndex.oPrimaryIndex = $3;
	}
;

hashrate: /* empty */
	{
		if(pDb)
			pDb->m_oIndex.nHashRate = 125;
	}
| '(' TNUMBER ')'
	{
		if(pDb)
			pDb->m_oIndex.nHashRate = strtod($2, NULL);
		free($2);
	}
;

indexalgo: /* empty */
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = RDB_RBTREE_INDEX;
	}
| BY RBTREE
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = RDB_RBTREE_INDEX;
	}
| BY NTREE
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = RDB_NTREE_INDEX;
	}
| BY HASH hashrate
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = RDB_HASH_INDEX;
	}
;

indexfld0: IDENTIFIER
	{
		if(pDb)
			pDb->m_oIndex.oIndexFieldTable.Insert((uint32)(-1), $1);
	}
;

indexfld: indexfld0
| indexfld ',' indexfld0
;

dropindex: DROP INDEX IDENTIFIER ';'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->DropIndex($3);
		pSqlEnv->Flush();
	}
;

quit: QUIT
	{
		int x = 1;
		if(x)
			return 0;
	}
;

help: HELP
	{
		pSqlEnv->Help();
		pSqlEnv->Flush();
	}
;

batch: BATCH
	{
		pSqlEnv->ExecuteFile($1);
		free($1);
	}
;

backup: BACKUP
	{
		pSqlEnv->Backup();
	}
;

repeat: REPEAT TNUMBER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("there isn't a database selected"));
		else
			pDb->SetRepeat(atoi($2));
		pSqlEnv->Flush();
		free($2);
	}
;

stmt: ';'
| create_memorydevice
| create_filedevice
| createdatabase
| usedatabase
| dropdatabase
| createtable
| modifytable
| droptable
| createindex
| dropindex
| insert
| delete
| update
| truncate
| select
| desc
| comment
| quit
| help
| batch
| backup
| repeat
| error ';'
	{
		FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("invalid sql command"));
		pSqlEnv->Flush();
		yyclearin;
		yyerrok;
	}
;

stmtlist: stmt | stmtlist stmt

%%
void yyerror(const char*)
{
}

FOCP_END();

void CopyName(char* d, const char* s)
{
	FOCP_NAME::CString::StringCopy(d, s, FOCP_NAME::RDB_NAME_MAXLEN);
	d[FOCP_NAME::RDB_NAME_MAXLEN] = '\0';
}
