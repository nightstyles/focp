
%{
#include <string.h>
void CopyName(char* d, const char* s);
static int lex_test = 1;
%}

IDENTIFIER	[a-zA-Z_][a-zA-Z_0-9]*
WS			[ \t\n\r]+
TNUMBER		[+-]?([0-9]+|[0-9]+\.[0-9]+)
XSTRING		\'(\'\'|[^\'\n])*(\'|\n)
BATCH		batch[ \t\n\r]*{XSTRING}[ \t\n\r]*;[^\n]*

%x STRING0

%%
repeat			{if(lex_test)return REPEAT;}
use				{if(lex_test)return USE;}
create			{if(lex_test)return CREATE;}
database		{if(lex_test)return DATABASE;}
device			{if(lex_test)return DEVICE;}
table			{if(lex_test)return TABLE;}
modify			{if(lex_test)return MODIFY;}
drop			{if(lex_test)return DROP;}
index			{if(lex_test)return INDEX;}
on				{if(lex_test)return ON;}
by				{if(lex_test)return BY;}
unique			{if(lex_test)return UNIQUE;}
foreign			{if(lex_test)return FOREIGN;}
rbtree			{if(lex_test)return RBTREE;}
ntree			{if(lex_test)return NTREE;}
hash			{if(lex_test)return HASH;}
insert			{if(lex_test)return INSERT;}
into			{if(lex_test)return INTO;}
values			{if(lex_test)return VALUES;}
delete			{if(lex_test)return TDELETE;}
where			{if(lex_test)return WHERE;}
and				{if(lex_test)return AND;}
update			{if(lex_test)return UPDATE;}
set				{if(lex_test)return SET;}
truncate		{if(lex_test)return TRUNCATE;}
select			{if(lex_test)return SELECT;}
from			{if(lex_test)return FROM;}
desc			{if(lex_test)return DESC;}
int8			{if(lex_test)return TINT8;}
int16			{if(lex_test)return TINT16;}
int32			{if(lex_test)return TINT32;}
int64			{if(lex_test)return TINT64;}
uint8			{if(lex_test)return TUINT8;}
uint16			{if(lex_test)return TUINT16;}
uint32			{if(lex_test)return TUINT32;}
uint64			{if(lex_test)return TUINT64;}
float			{if(lex_test)return TFLOAT;}
double			{if(lex_test)return TDOUBLE;}
string			{if(lex_test)return TSTRING;}
cstring			{if(lex_test)return CSTRING;}
vstring			{if(lex_test)return VSTRING;}
vcstring		{if(lex_test)return VCSTRING;}
raw				{if(lex_test)return RAW;}
vraw			{if(lex_test)return VRAW;}
not				{if(lex_test)return NOT;}
null			{if(lex_test)return TNULL;}
size			{if(lex_test)return TSIZE;}
recsize			{if(lex_test)return RECSIZE;}
default			{if(lex_test)return DEFAULT;}
backup			{if(lex_test)return BACKUP;}
quit			{if(lex_test)return QUIT;}
help			{if(lex_test)return HELP;}
"$Count"		{if(lex_test){CopyName(yylval->nam, yytext); return VCOUNT;}}
"--".*			{if(lex_test)return COMMENT;}
">="			{if(lex_test)return GE;}
"<="			{if(lex_test)return LE;}
"("				{if(lex_test)return '(';}
")"				{if(lex_test)return ')';}
";"				{if(lex_test)return ';';}
"="				{if(lex_test)return EQ;}
","				{if(lex_test)return ',';}
">"				{if(lex_test)return MT;}
"<"				{if(lex_test)return LT;}
"*"				{if(lex_test)return '*';}
"."				{if(lex_test)return '.';}
{TNUMBER}		{
					if(lex_test)
					{
						yylval->sv = strdup(yytext);
						return TNUMBER;
					}
				}
{XSTRING}		{
					if(lex_test)
					{
						yylval->sv = strdup(yytext+1);
						unsigned int nLen = FOCP_NAME::CString::StringLength(yylval->sv);
						if(yylval->sv[nLen-1] == '\'')
							yylval->sv[nLen-1] = 0;
						char * sDst = yylval->sv;
						char * sSrc = yylval->sv;
						while(sSrc[0])
						{
							if(sSrc != sDst)
								*sDst = *sSrc;
							if(*sSrc == '\'')
								++sSrc;
							if(*sDst == '\n')
							{
								*sDst = 0;
								free(yylval->sv);
								return TSQLERROR;
							}
							++sSrc;
							++sDst;
						}
						*sDst = 0;
						return XSTRING;
					}
				}
{IDENTIFIER}	{if(lex_test){CopyName(yylval->nam, yytext); return IDENTIFIER;}}
{BATCH}			{//batch{WS}{XSTRING}{WS};[^\n]*
					if(lex_test)
					{
						char* s1 = FOCP_NAME::CString::CharOfString(yytext, '\'') + 1;
						char * sDst = s1;
						char * sSrc = s1;
						while(sSrc[0])
						{
							if(*sSrc == '\'')
							{								
								++sSrc;
								if(*sSrc != '\'')
									break;
							}
							if(sSrc != sDst)
								*sDst = *sSrc;
							if(*sDst == '\n')
							{
								*sDst = 0;
								return TSQLERROR;
							}
							++sSrc;
							++sDst;
						}
						*sDst = 0;
						yylval->sv = strdup(s1);
						s1 = FOCP_NAME::CString::CharOfString(sSrc, ';') + 1;
						for(; *s1; ++s1)if(!FOCP_NAME::CString::IsSpace(*s1))break;
						if(*s1)FocpCmdLogEx("SQL", FOCP_LOG_CLOSE, ("discard %s", s1));
						return BATCH;
					}
				}
%%

int yywrap()
{
	return 1;
}
