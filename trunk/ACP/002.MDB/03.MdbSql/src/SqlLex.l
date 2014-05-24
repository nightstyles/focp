
%{
#include <string.h>
void CopyName(char* d, const char* s);
%}

IDENTIFIER	[a-zA-Z_][a-zA-Z_0-9]*
WS		[ \t\n\r]+
TNUMBER		[+-]?([0-9]+|[0-9]+\.[0-9]+)
XSTRING		\'(\'\'|[^\'\n])*(\'|\n)
BATCH		batch[ \t\n\r]*{XSTRING}[ \t\n\r]*;[^\n]*

%x STRING0

%%
capacity	{return CAPACITY;}
repeat		{return REPEAT;}
use		{return USE;}
create		{return CREATE;}
database	{return DATABASE;}
table		{return TABLE;}
index		{return INDEX;}
on		{return ON;}
by		{return BY;}
unique		{return UNIQUE;}
foreign		{return FOREIGN;}
rbtree		{return RBTREE;}
hash		{return HASH;}
insert		{return INSERT;}
into		{return INTO;}
values		{return VALUES;}
delete		{return TDELETE;}
where		{return WHERE;}
and		{return AND;}
or		{return OR;}
update		{return UPDATE;}
set		{return SET;}
truncate	{return TRUNCATE;}
select		{return SELECT;}
from		{return FROM;}
desc		{return DESC;}
int8		{return TINT8;}
int16		{return TINT16;}
int32		{return TINT32;}
int64		{return TINT64;}
uint8		{return TUINT8;}
uint16		{return TUINT16;}
uint32		{return TUINT32;}
uint64		{return TUINT64;}
float		{return TFLOAT;}
double		{return TDOUBLE;}
date		{return TDATE;}
time		{return TTIME;}
datetime	{return TDATETIME;}
string		{return TSTRING;}
cstring		{return CSTRING;}
vstring		{return VSTRING;}
vcstring	{return VCSTRING;}
raw		{return RAW;}
vraw		{return VRAW;}
not		{return NOT;}
null		{return TNULL;}
size		{return TSIZE;}
default		{return DEFAULT;}
quit		{return QUIT;}
help		{return HELP;}
orderby		{return ORDERBY;}
asc		{return ASC;}
dec		{return DEC;}
"--".*		{return COMMENT;}
">="		{return GE;}
"<="		{return LE;}
"!="		{return NE;}
"+="		{return PE;}
"-="		{return SE;}
"*="		{return ME;}
"/="		{return DE;}
"%="		{return MOD;}
"&="		{return AE;}
"|="		{return OE;}
"^="		{return XE;}
"~"		{return BN;}
"("		{return '(';}
")"		{return ')';}
";"		{return ';';}
"="		{return EQ;}
","		{return ',';}
">"		{return MT;}
"<"		{return LT;}
"*"		{return '*';}
"."		{return '.';}
{TNUMBER}	{			
			yylval->sv = strdup(yytext);
			return TNUMBER;
		}
{XSTRING}	{			
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
{IDENTIFIER}	{{CopyName(yylval->nam, yytext); return IDENTIFIER;}}
{BATCH}		{//batch{WS}{XSTRING}{WS};[^\n]*
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
%%

int yywrap()
{
	return 1;
}
