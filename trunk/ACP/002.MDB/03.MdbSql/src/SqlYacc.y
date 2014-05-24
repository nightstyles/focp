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
	char	nam[FOCP_NAME::MDB_NAME_MAXLEN+1];
	double 	d;
};

%token <nam> IDENTIFIER
%token TINT8 TINT16 TINT32 TINT64
%token TUINT8 TUINT16 TUINT32 TUINT64
%token TFLOAT TDOUBLE TDATE TTIME TDATETIME
%token <sv> TSTRING CSTRING VSTRING VCSTRING
%token RAW VRAW
%token TSIZE
%token DEFAULT
%token NOT TNULL
%token <sv> TNUMBER
%token <sv> XSTRING
%token UNIQUE FOREIGN
%token RBTREE HASH
%token REPEAT CAPACITY
%token CREATE
%token DATABASE
%token TABLE
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
%token OR
%token UPDATE
%token SET
%token TRUNCATE
%token SELECT
%token DESC
%token QUIT
%token HELP
%token ORDERBY
%token ASC
%token DEC
%token <sv> BATCH
%token GE LE EQ MT LT NE PE SE AE OE XE BN ME DE MOD
%token TSQLERROR

%type <i> basetype
%type <sv> value
%type <i> operator

%start stmtlist

%%

def_field: IDENTIFIER
	{
		if(pDb)
			pDb->m_pCurrentTable->AddField($1);
	}
;

def_notnull: /* empty */
| NOT TNULL
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->null = true;
		}
	}
;

def_size: TSIZE EQ TNUMBER
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->len = (int)strtod($3, NULL);
		}
		free($3);
	}
;

def_default: /* empty */
| DEFAULT EQ XSTRING
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
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
	{ $$ = MDB_INT8_FIELD; }
| TINT16
	{ $$ = MDB_INT16_FIELD; }
| TINT32
	{ $$ = MDB_INT32_FIELD; }
| TINT64
	{ $$ = MDB_INT64_FIELD; }
| TUINT8
	{ $$ = MDB_UINT8_FIELD; }
| TUINT16
	{ $$ = MDB_UINT16_FIELD; }
| TUINT32
	{ $$ = MDB_UINT32_FIELD; }
| TUINT64
	{ $$ = MDB_UINT64_FIELD; }
| TFLOAT
	{ $$ = MDB_FLOAT_FIELD; }
| TDOUBLE
	{ $$ = MDB_DOUBLE_FIELD; }
| TDATE
	{ $$ = MDB_DATE_FIELD; }
| TTIME
	{ $$ = MDB_TIME_FIELD; }
| TDATETIME
	{ $$ = MDB_DATETIME_FIELD; }
;

def_datatype: basetype
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = $1;
		}
	}
| TSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_CHAR_FIELD;
		}
	}
| CSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_LCHAR_FIELD;
		}
	}
| RAW def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_RAW_FIELD;
		}
	}
| VSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARCHAR_FIELD;
		}
	}
| VCSTRING def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARLCHAR_FIELD;
		}
	}
| VRAW def_size
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARRAW_FIELD;
		}
	}
| IDENTIFIER
	{
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
			{
				CSqlField* pAliasType = pDb->m_oAliasTable.FindField($1);
				if(pAliasType)
				{
					pField->type = pAliasType->type;
					pField->len = pAliasType->len;
					pField->null = pAliasType->null;
					pField->defval = pAliasType->defval;
				}
				else
				{
					FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("'%s' is invalid type", $1));
					pField->type = MDB_INT32_FIELD;
				}
			}
		}
	}
;

usedatabase: USE IDENTIFIER
	{
		pSqlEnv->SelectDataBase($2);
		pSqlEnv->Flush();
		pDb = pSqlEnv->GetDataBase();
	}
;

table_capacity: /* empty */
	{
		if(pDb)
			pDb->m_pCurrentTable->SetMaxRecordNum(0xFFFFFFFF);
	}
| CAPACITY EQ TNUMBER
	{
		if(pDb)
			pDb->m_pCurrentTable->SetMaxRecordNum(atoi($3));
		free($3);
	}
;

createtable0: CREATE TABLE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlTable($3);
	}
'(' fields ')' table_capacity
;

createtable: createtable0
	{
		if(pDb)
			pDb->CreateTable();
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
| TNULL
	{
		if(pDb)
			pDb->m_oInsert.AddValue("");
	}
;

insvalue: insvalue0
| insvalue ',' insvalue0
;

insert0: INSERT INTO IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlInsert($3);
	}
insfldlist VALUES '(' insvalue ')'
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
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlDelete($3);
	}
;

delete1: delete0
| delete0 WHERE wherecond
;

wherecond_base: IDENTIFIER operator value
	{
		if(pDb)
		{
			pDb->m_pWhere->AddCond($1, $2, $3);
			free($3);
		}
	}
| IDENTIFIER operator TNULL
	{
		if(pDb)	pDb->m_pWhere->AddCond($1, $2, "");
	}
;

wherecond_and: wherecond_base
| wherecond_and AND wherecond_base
;

wherecond: wherecond_and
| wherecond OR {pDb->m_pWhere->NewSet();} wherecond_and
;

operator: GE { $$ = MDB_SQLPARA_OPERATOR_MOREEQUAL; }
| LE { $$ = MDB_SQLPARA_OPERATOR_LESSEQUAL; }
| LT { $$ = MDB_SQLPARA_OPERATOR_LESS; }
| MT { $$ = MDB_SQLPARA_OPERATOR_MORE; }
| EQ { $$ = MDB_SQLPARA_OPERATOR_EQUAL; }
| NE { $$ = MDB_SQLPARA_OPERATOR_NOTEQUAL; }
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
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlUpdate($2);
	}
SET setexpr
;

update1: update0
| update0 WHERE wherecond
;

setexpr0:
 IDENTIFIER EQ BN
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, "", MDB_SQLPARA_OPERATOR_BITNOT);
 }
|IDENTIFIER EQ value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_EQUAL);
	free($3);
 }
|IDENTIFIER EQ TNULL
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, "", MDB_SQLPARA_OPERATOR_EQUAL);
 }
|IDENTIFIER PE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_ADD);
	free($3);
 }
|IDENTIFIER SE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_SUB);
	free($3);
 }
|IDENTIFIER ME value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_MUL);
	free($3);
 }
|IDENTIFIER DE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_DIV);
	free($3);
 }
|IDENTIFIER MOD value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_MOD);
	free($3);
 }
|IDENTIFIER AE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_BITAND);
	free($3);
 }
|IDENTIFIER OE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_BITOR);
	free($3);
 }
|IDENTIFIER XE value
 {
	if(pDb)
		pDb->m_oUpdate.AddSetPara($1, $3, MDB_SQLPARA_OPERATOR_BITXOR);
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

truncate: TRUNCATE TABLE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->TruncateTable($3);
		pSqlEnv->Flush();
	}
;

select0: SELECT
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->PrepareSqlSelect();
	}
selflds FROM IDENTIFIER
	{
		if(pDb)
			pDb->CreateSqlSelect($5);
	}
;

select1: select0
| select0 WHERE wherecond
| select0 orderby_sentence
| select0 WHERE wherecond orderby_sentence
;

orderby_sentence: ORDERBY IDENTIFIER { pDb->OrderBy($2); }
| ORDERBY IDENTIFIER ASC { pDb->OrderBy($2, true); }
| ORDERBY IDENTIFIER DEC { pDb->OrderBy($2, false); }
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

desc: DESC TABLE IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->DescTable($3);
		pSqlEnv->Flush();
	}
| DESC TABLE '*'
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->DescTable();
		pSqlEnv->Flush();
	}
| DESC DATABASE '*'
	{
		pSqlEnv->DescAllDataBase();
		pSqlEnv->Flush();
	}
;

createindex0: CREATE indexattr foreignattr INDEX IDENTIFIER ON IDENTIFIER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
		{
			pDb->m_oIndex.oIndexName = $5;
			pDb->m_oIndex.oTableName = $7;
			pDb->m_oIndex.oIndexFieldTable.Clear();
		}
	}
'(' indexfld ')' indexalgo
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
			pDb->m_oIndex.nQualifier = MDB_COMMON_INDEX;
	}
| UNIQUE
	{
		if(pDb)
			pDb->m_oIndex.nQualifier = MDB_UNIQUE_INDEX;
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
			pDb->m_oIndex.nHashRate = (uint32)strtod($2, NULL);
		free($2);
	}
;

indexalgo: /* empty */
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_RBTREE_INDEX;
	}
| BY RBTREE
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_RBTREE_INDEX;
	}
| BY HASH hashrate
	{
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_HASH_INDEX;
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

repeat: REPEAT TNUMBER
	{
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->SetRepeat(atoi($2));
		pSqlEnv->Flush();
		free($2);
	}
;

stmt: ';'
| usedatabase ';'
| createtable ';'
| field ';'
| createindex ';'
| insert ';'
| delete ';'
| update ';'
| truncate ';'
| select ';'
| desc ';'
| comment ';'
| quit ';'
| help ';'
| batch ';'
| repeat ';'
| error
	{
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("invalid sql command"));
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
	FOCP_NAME::CString::StringCopy(d, s, FOCP_NAME::MDB_NAME_MAXLEN);
	d[FOCP_NAME::MDB_NAME_MAXLEN] = '\0';
}
