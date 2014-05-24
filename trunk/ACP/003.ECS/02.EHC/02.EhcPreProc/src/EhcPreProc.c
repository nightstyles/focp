
#include "EhcPreProc.h"

/* cmode states */
#define CMcmt 1			/* in comment */
#define CMstr 2			/* in string */
#define CMchr 3			/* in character constant */
#define CMang 4			/* in < > */

//sign flag
#define USIGN  1
#define SIGN   2

#define REBUFF_INCREMENT   80

enum
{
    DEFAULT,
	DEFINE,
    ELIF,
	ELSE,
	ENDIF,
    IF,
	IFDEF,
	IFNDEF,
	INCLUDE,
	UNDEF,
	iERROR,
	PRAGMA
};

typedef struct EhcPreProcInt
{
    union v_t
	{
		ehc_long s;		/* signed value */
		ehc_ulong u;		/* ehc_uint value */
    }v;
    ehc_int type;
}EhcPreProcInt;

#define sval(x)  ((x).v.s)
#define uval(x)  ((x).v.u)
#define tval(x)  ((x).type)

enum
{
	LOR = 1, LAND, BOR,  XOR,  AND, EQ, NEQ,  LT, LEQ,
    GT, GEQ, LSHFT, RSHFT,PLUS, MINUS, TIMES, DIV, MOD
};

#define MoD  10
#define BUFSIZE   512
static const ehc_char * empty_string = " ";

#define SkipAll(a)	while(*(a))++a
#define ForwardSkipWhiteSpace(a)	while(*(a) && GetEhcInterface()->IsSpace(*(a)))++a
#define BackSkipWhiteSpace(a)	while(*(a) && GetEhcInterface()->IsSpace(*(a)))--a
#define SkipToChar(a,b)	while(*(a) && *(a) != b)++a
#define SkipNonWhiteSpaceChar(a)		while(*(a) && !GetEhcInterface()->IsSpace(*(a)))++a

static ehc_int GetChar(CEhcPreProc* pPreProc);
static ehc_void UnGetChar(CEhcPreProc* pPreProc);
static ehc_void WritePreChar(CEhcPreProc* pPreProc, ehc_char c);
static ehc_char* WriteProChar(CEhcPreProc* pPreProc, ehc_char c);
static ehc_void GetPreLine(CEhcPreProc* pPreProc);
static ehc_int ControlLine(CEhcPreProc* pPreProc);
static ehc_void Process(CEhcPreProc* pPreProc);
static ehc_char * CppExpand(CEhcPreProc* pPreProc, ehc_char *fld,ehc_char **end, ehc_int bot, ehc_int top);
static ehc_char * Process2(CEhcPreProc* pPreProc, ehc_char * line, ehc_int bot, ehc_int top);
static ehc_void DefineMacro(CEhcPreProc* pPreProc, ehc_char *def);
static ehc_void DoDefMacro(CEhcPreProc* pPreProc, ehc_char *s);
static ehc_void NewMacro(CEhcPreProc* pPreProc, CEhcVector *parms);
static ehc_void RemMacroID(CEhcPreProc* pPreProc, ehc_int k);
static ehc_void FreeMacro(EhcMacro * pMacro);
static ehc_int IsMacroID(CEhcPreProc* pPreProc, ehc_char *sMacroName);
static ehc_void Check4Res(CEhcPreProc* pPreProc, EhcMacro * pMacro);
static ehc_void Parameterise(CEhcPreProc* pPreProc, CEhcVector *parms);
static ehc_void MergeTokens(CEhcPreProc* pPreProc, EhcMacro *pMacro);
static ehc_int FindParm(CEhcPreProc* pPreProc, CEhcVector *parms,ehc_char *name, ehc_uint len);
static ehc_void ExprList(CEhcPreProc* pPreProc, CEhcVector * parms, ehc_char **p,ehc_int more);
static ehc_void DoPragma(CEhcPreProc* pPreProc, ehc_char *s);
static ehc_void ReplaceDefines(CEhcPreProc* pPreProc, ehc_char *S);
static ehc_void ReplaceIdentifiers(CEhcPreProc* pPreProc, ehc_char *S);
static ehc_int CppParse(CEhcPreProc* pPreProc, ehc_char *s);
static EhcPreProcInt IfExpr(CEhcPreProc* pPreProc, ehc_int k);
static EhcPreProcInt DoBinary(CEhcPreProc* pPreProc, ehc_int tk, EhcPreProcInt left, EhcPreProcInt right);
static EhcPreProcInt ExprUnary(CEhcPreProc* pPreProc);
static EhcPreProcInt GetNumber(CEhcPreProc* pPreProc);
static ehc_int GetTok(CEhcPreProc* pPreProc, ehc_int k);
static ehc_int GetCharConst(CEhcPreProc* pPreProc);

typedef struct CEhcKeyword
{
    ehc_char *id;
    ehc_int  token;
}CEhcKeyword;

static CEhcKeyword g_pEhcKeyWords[] =
{
	{"define", DEFINE},
	{"elif", ELIF},
	{"else", ELSE},
	{"endif", ENDIF},
	{"error", iERROR},
	{"if", IF},
	{"ifdef", IFDEF},
	{"ifndef", IFNDEF},
	{"include", INCLUDE},
	{"pragma", PRAGMA},
	{"undef", UNDEF}
};

static ehc_int IsKeyWord(ehc_char* id, CEhcKeyword* pKeywords, ehc_int n)
{
	ehc_int i;
    for(i=0; i<n; i++)
	{
		if(!StringCompare(pKeywords[i].id, id))
		   return pKeywords[i].token;
	}
    return 0;
}

static ehc_char *WriteChar(ehc_char *s, ehc_char c, ehc_int mod, ehc_int i)
{
	if(!(i%mod))
	{
		if(!s)
			s = (ehc_char *)GetEhcInterface()->Malloc(i + mod + 1);
		else
			s = (ehc_char *)GetEhcInterface()->Realloc(s, i + mod + 1);
	}
	s[i] = c;
	return s;
}

static ehc_void ReBuff(ehc_char **buf, ehc_int *len)
{
	*buf = (ehc_char *) GetEhcInterface()->Realloc(*buf, *len + REBUFF_INCREMENT);
	*len += REBUFF_INCREMENT;
}

static ehc_char * CreateCString(ehc_char * sStr)
{
	ehc_int i = 0;
	ehc_char *S=NULL;
	S = WriteChar(S,'\"', MoD, i++); /* " */
	while(*sStr)
	{
		if(*sStr == '\"' || *sStr == '\\')  /* " */
			S = WriteChar(S,'\\', MoD, i++);
		S = WriteChar(S, *sStr++, MoD, i++);
	}
	S = WriteChar(S, '\"', MoD, i++); /* " */
	S[i]='\0';
	return S;
}

static ehc_ulong GetHashCode(ehc_uchar *s)
{
	ehc_ulong t, h,i;
	h = 0;
	while(*s)
	{
		for(t = i=0;*s && i<32;i+=8)
			t |= (*s++ << i);
		h += t;
	}
	return h;
}

static void ClearMacro(EhcMacro *pMacro)
{
	FreeMacro(pMacro);
	GetEhcInterface()->Free(pMacro);
}

/*
* for processing -Dname[=value] switch
* def = name[=value]
*/
static ehc_void DefineMacro(CEhcPreProc* pPreProc, ehc_char *sDefine)
{
	if(sDefine)
	{
		ehc_char * sEqual;
		ehc_int nLen = StringLength(sDefine);
		ehc_char * sStr = (ehc_char*)GetEhcInterface()->Malloc(nLen+3);
		MemoryCopy(sStr, sDefine, nLen+1);
		for(sEqual = sStr; *sEqual && *sEqual != '='; ++sEqual)
			;
		if(*sEqual)
		{
			*sEqual = ' ';
			sStr[nLen] = 0;
		}
		else
		{
			sStr[nLen] = ' ';
			sStr[nLen+1] = '1';
			sStr[nLen+2] = 0;
		}
		DoDefMacro(pPreProc, sStr);
		GetEhcInterface()->Free(sStr);
	}
}

static ehc_void WritePreChar(CEhcPreProc* pPreProc, ehc_char c)
{
	*pPreProc->sPreLineCur++ = c;
	if (pPreProc->sPreLineCur >= pPreProc->sPreLine + pPreProc->nPreLineLen)
	{
		ehc_uint d = pPreProc->sPreLineCur - pPreProc->sPreLine;
		ReBuff(&pPreProc->sPreLine, (ehc_int*)&pPreProc->nPreLineLen);
		pPreProc->sPreLineCur = pPreProc->sPreLine + d;
	}
}

static ehc_char* WriteProChar(CEhcPreProc* pPreProc, ehc_char c)
{
	*pPreProc->sProLineCur++ = c;
	if(pPreProc->sProLineCur >= pPreProc->sProLine + pPreProc->nProLineLen)
	{
		ehc_uint d = pPreProc->sProLineCur - pPreProc->sProLine;
		ReBuff(&pPreProc->sProLine, (ehc_int*)&pPreProc->nProLineLen);
		pPreProc->sProLineCur = pPreProc->sProLine + d;
	}
	*pPreProc->sProLineCur = '\0';
	return pPreProc->sProLineCur - 1;
}

static ehc_void GetPreLine(CEhcPreProc* pPreProc)
{
	ehc_int c;
	ehc_int nLineComment = 0;
	pPreProc->sPreLineCur = pPreProc->sPreLine;
	pPreProc->sFileName = GetFileName(&pPreProc->oFileTable);
	pPreProc->nFileLineNo = GetLineNo(&pPreProc->oFileTable);
	while(1)
	{
		switch ((c = GetChar(pPreProc)))
		{
		case '\\':
			/* look for pPreProc->sPreLine continuation */
			switch ((c = GetChar(pPreProc)))
			{
			case '\0':
				PrintError(False, "unexpected end of file");
			case '\n':
				continue;
			}
			WritePreChar(pPreProc, '\\');

		default:
			/* pPreProc->nSkip leading white space*/
			if(!GetEhcInterface()->IsSpace(c) || pPreProc->sPreLineCur != pPreProc->sPreLine)
			{
				if(pPreProc->sPreLineCur == pPreProc->sPreLine)
				{
					pPreProc->sFileName = GetFileName(&pPreProc->oFileTable);
					pPreProc->nFileLineNo = GetLineNo(&pPreProc->oFileTable);
				}
				WritePreChar(pPreProc, (ehc_char)c);
			}
			continue;

		case '/':
			if (pPreProc->nCMode == 0)
			{
				/* check for comment*/
				if((c = GetChar(pPreProc)) == '*' || c == '/')
				{
					/* Allow for C++ style comments */
					if(c == '/')
						nLineComment = 1;
					/* strip comment out */
					c = GetChar(pPreProc);
					if(pPreProc->sPreLineCur != pPreProc->sPreLine)
						WritePreChar(pPreProc, ' ');  /* replace comment with space */
					while(c != '\0')
					{
						if(c == '\n')
						{
							if(nLineComment == 1)
							{
								UnGetChar(pPreProc);
								nLineComment = 0;
								break;
							}
						}
						if(c == '*' && !nLineComment)
						{
							if((c = GetChar(pPreProc)) == '/')
								break;
						}
						else if (c == '\\' && nLineComment)
						{
							/* allow for pPreProc->sPreLine splicing */
							c = GetChar(pPreProc); /* waste next ehc_char */
							c = GetChar(pPreProc);
						}
						else c = GetChar(pPreProc);
					}
				}
				else
				{
					WritePreChar(pPreProc, '/');
					if(c != '\0')
						WritePreChar(pPreProc, (ehc_char)c);
				}
			}
			else WritePreChar(pPreProc, '/');
			continue;

		case '\"': /* " */
			if (pPreProc->nCMode == 0)
				pPreProc->nCMode = CMstr;
			else if (pPreProc->nCMode == CMstr)
				pPreProc->nCMode = 0;
			WritePreChar(pPreProc, '\"');  /* " */
			continue;

		case '\'':
			if (pPreProc->nCMode == 0)
				pPreProc->nCMode = CMchr;
			else if (pPreProc->nCMode == CMchr)
				pPreProc->nCMode = 0;
			WritePreChar(pPreProc, '\'');
			continue;

		case '\n':
			if(pPreProc->sPreLineCur == pPreProc->sPreLine && GetFileCount(&pPreProc->oFileTable) > 1 )
				continue;
			if (pPreProc->nCMode == CMstr)
			{
				if (!pPreProc->nSkip)
					PrintError(False, "unbalanced \"");  /* " */
				else
					WritePreChar(pPreProc, '\"');  /* " */
				pPreProc->nCMode = 0;
			}
			else if (pPreProc->nCMode == CMchr)
			{
				if (!pPreProc->nSkip)
					PrintError(False, "unbalanced \'");
				else
					WritePreChar(pPreProc, '\'');
				pPreProc->nCMode = 0;
			}
			WritePreChar(pPreProc, '\n');
			break;

		case '\0':
			if(pPreProc->nIfLevel && GetFileCount(&pPreProc->oFileTable) == 2)
				PrintError(False, "unterminated `#if' conditional");
			if (pPreProc->sPreLineCur != pPreProc->sPreLine)
				break;
			if (GetFileCount(&pPreProc->oFileTable) >= 1)
			{
				PopFile(&pPreProc->oFileTable);
				if(GetFileCount(&pPreProc->oFileTable) >= 1)
					continue;
			}
			else
				WritePreChar(pPreProc, '\0');
			break;
		}
		break;
	}
	*pPreProc->sPreLineCur = '\0';
}

static ehc_int GetChar(CEhcPreProc* pPreProc)
{
	ehc_int nRet;
	if(!pPreProc->sLine || !pPreProc->sLine[0])
	{
		pPreProc->sLine = (ehc_char*)GetLineFromFileTable(&pPreProc->oFileTable);
		if(!pPreProc->sLine || !pPreProc->sLine[0])
			return '\0';
	}
	nRet = (ehc_int)(ehc_uchar)pPreProc->sLine[0];
	++pPreProc->sLine;
	return nRet;
}

static ehc_void UnGetChar(CEhcPreProc* pPreProc)
{
	if(pPreProc->sLine)
		--pPreProc->sLine;
	else
		pPreProc->sLine = (ehc_char*)"";
}

static ehc_int ControlLine(CEhcPreProc* pPreProc)
{
	ehc_int k;
	ehc_char key[25];
	ehc_char *s, *e;
	if (*pPreProc->sPreLine == '#')
	{
		s = pPreProc->sPreLine + 1;
		ForwardSkipWhiteSpace(s);
		if(!*s) /* test for null directive */
			return 1;

		for(k=0; GetEhcInterface()->IsAlpha(*s); k++)
			key[k] = *s++;
		key[k] = '\0';

		k = IsKeyWord(key, g_pEhcKeyWords, sizeof(g_pEhcKeyWords)/sizeof(CEhcKeyword));
		ForwardSkipWhiteSpace(s);

		switch (k)
		{
		case DEFINE:
			if (!pPreProc->nSkip)
			{
				if(*s)
					DoDefMacro(pPreProc, s);
				else
					PrintError(False, "empty '#define' directive");
			}
			break;

		case ELIF:
			if(pPreProc->nSkip && pPreProc->nSkip == pPreProc->nIfLevel)
			{
				if(CppParse(pPreProc, s))
					pPreProc->nSkip = 0;
			}
			else if(!pPreProc->nSkip && pPreProc->nIfLevel)
				pPreProc->nSkip = pPreProc->nIfLevel + 1;
			break;

		case ELSE:
			if (pPreProc->nIfLevel == 0)
				PrintError(False, "unmatched #else");
			else if (pPreProc->nSkip == pPreProc->nIfLevel)
				pPreProc->nSkip = 0;
			else if (pPreProc->nSkip == 0)
				pPreProc->nSkip = pPreProc->nIfLevel;
			break;

		case ENDIF:
			if (pPreProc->nIfLevel == 0)
				PrintError(False, "unmatched #endif");
			else
			{
				if (pPreProc->nSkip >= pPreProc->nIfLevel)
					pPreProc->nSkip = 0;
				--pPreProc->nIfLevel;
			}
			break;

		case IF:
			++pPreProc->nIfLevel;
			if(!pPreProc->nSkip)
			{
				if(*s)
				{
					if(!CppParse(pPreProc, s))
						pPreProc->nSkip = pPreProc->nIfLevel;
				}
				else
					PrintError(False, "empty '#if' directive");
			}
			break;
		case IFDEF:
		case IFNDEF:
			++pPreProc->nIfLevel;
			if (!pPreProc->nSkip)
			{
				if (GetEhcInterface()->IsAlpha(*s) || *s == '_')
				{
					e = s;
					SkipNonWhiteSpaceChar(s);
					*s = '\0';
					if (IsMacroID(pPreProc, e) > -1)
					{
						if (k == IFNDEF)
							pPreProc->nSkip = pPreProc->nIfLevel;
					}
					else if (k == IFDEF)
						pPreProc->nSkip = pPreProc->nIfLevel;
				}
				else
					PrintError(False, "illegal macro identifier");
			}
			break;

		case INCLUDE:
			if(pPreProc->nSkip)
				break;
			if(!*s)
			{
				PrintError(False, "empty '#include' directive");
				break;
			}
			if (*s == '\"') /* " */
				s++, pPreProc->nCMode = CMstr;
			else if (*s == '<')
				s++, pPreProc->nCMode = CMang;
			else
				pPreProc->nCMode = 0;
			e = s;

			if(!pPreProc->nCMode)
				SkipNonWhiteSpaceChar(s);
			else
			{
				while(*s && *s != '\"' && *s !='>')
					s++;
				if(*s) s++; /* move past the sentinel */
			}

			if (pPreProc->nCMode)
			{
				if (pPreProc->nCMode == CMstr && *(s - 1) != '\"')
					PrintError(False, "missing \"");
				else if (pPreProc->nCMode == CMang && *(s - 1) != '>')
					PrintError(False, "missing >");
				*--s = '\0';
			}
			else
			{/* token_sequence */
				pPreProc->sPreLineCur = pPreProc->sPreLine;
				while (e != s)
					*pPreProc->sPreLineCur++ = *e++;
				*pPreProc->sPreLineCur = '\0';
				Process(pPreProc);
				e = pPreProc->sProLine;
			}

			if(GetEhcInterface()->bSupportPreProcOut)
			{
				ehc_void* fp;
				ehc_int nBufLen;
				ehc_char sFileName2[EHC_MAX_PATH], sInfo[EHC_MAX_PATH+20];
				GetEhcInterface()->FormatPrint(sFileName2, "%s.p", pPreProc->sFileName);
				if(pPreProc->nCMode != CMang)
					GetEhcInterface()->FormatPrint(sInfo, "#include \"%s\"\n", e);
				else
					GetEhcInterface()->FormatPrint(sInfo, "#include <%s>\n", e);
				nBufLen = StringLength(sInfo);
				fp = GetEhcInterface()->OpenFile(sFileName2, "a+b");
				GetEhcInterface()->WriteFile(fp, sInfo, nBufLen);
				GetEhcInterface()->CloseFile(fp);
			}
	
			if(PushIncludeFile(&pPreProc->oFileTable, e, (pPreProc->nCMode != CMang)))
			{
				ehc_char sDebug[512];
				GetEhcInterface()->FormatPrint(sDebug, "open head file %s failure",e);
				PrintError(False, sDebug);
			}
			else if(GetEhcInterface()->bSupportPreProcOut)
			{
				ehc_void* fp;
				ehc_char sFileName2[EHC_MAX_PATH];
				GetEhcInterface()->FormatPrint(sFileName2, "%s.p", pPreProc->oFileTable.pFileTable->sFileName);
				fp = GetEhcInterface()->OpenFile(sFileName2, "wb");
				GetEhcInterface()->CloseFile(fp);
			}
			pPreProc->nCMode = 0;

		case UNDEF:
			if (!pPreProc->nSkip)
			{
				e = s;
				SkipNonWhiteSpaceChar(s);
				*s = '\0';
				k = IsMacroID(pPreProc, e);
				if (k > -1)
					RemMacroID(pPreProc, k);
			}
			break;

		case iERROR:
			if(!pPreProc->nSkip)
			{
				ehc_char *S = Process2(pPreProc, s, 0, 0);
				PrintError(False, S);
				if(S)
					GetEhcInterface()->Free(S);
			}
			break;

		case PRAGMA:
			if(!pPreProc->nSkip)
			{
				ehc_char *S = Process2(pPreProc, s, 0, 0);
				if(S)
				{
					DoPragma(pPreProc, S);
					GetEhcInterface()->Free(S);
				}
			}
			break;

		default:
			break;
		}
		return 1;
	}
	return pPreProc->nSkip;
}

static ehc_void Process(CEhcPreProc* pPreProc)
{
	ehc_char *S;
	*(pPreProc->sProLineCur = pPreProc->sProLine) = '\0';
	S = Process2(pPreProc, pPreProc->sPreLine, 0, 0);
	if(S)
	{
		ehc_char* sOld = S;
		while(*S)
			WriteProChar(pPreProc, *S++);
		GetEhcInterface()->Free(sOld);
	}
	pPreProc->nCMode = 0;
}

static ehc_char * Process2(CEhcPreProc* pPreProc, ehc_char * line, ehc_int bot, ehc_int top)
{
	ehc_int k = 0;
	ehc_char *p, *s, *S = NULL;
	ehc_char * lp = line;

	while (*lp)
	{
		if (!pPreProc->nCMode && (GetEhcInterface()->IsAlpha(*lp) || *lp == '_'))
		{
			s = lp;
			p = CppExpand(pPreProc, lp, &lp, bot, top);
			if(p)
			{
				s = p;
				while(*p)
					S = WriteChar(S, *p++, MoD, k++);
				GetEhcInterface()->Free(s);
			}
			else while(s != lp)
				S = WriteChar(S, *s++, MoD, k++);
		}
		else
		{
			if(*lp == '\'')
			{
				if (pPreProc->nCMode == 0)
					pPreProc->nCMode = CMchr;
				else if (pPreProc->nCMode == CMchr)
					pPreProc->nCMode = 0;
			}
			if (*lp == '\"')
			{	/* " */
				if (pPreProc->nCMode == 0)
					pPreProc->nCMode = CMstr;
				else if(pPreProc->nCMode == CMstr)
					pPreProc->nCMode = 0;
			}
			else if (*lp == '\\'  && (pPreProc->nCMode == CMstr || pPreProc->nCMode == CMchr) )
			{
				S = WriteChar(S, *lp++, MoD, k++);
				if (!*lp)	/* get second ehc_char */
					break;
			}
			S = WriteChar(S, *lp++, MoD, k++);
		}
	}
	if(S)
		S[k] = '\0';
	return S;
}

static ehc_char * CppExpand(CEhcPreProc* pPreProc, ehc_char *fld, ehc_char **end, ehc_int bot, ehc_int top)
{
	ehc_char word[128];
	ehc_int i = 0, k;
	ehc_char *p, *p2;
	EhcMacro *pMacro;
	ehc_char *S =NULL;
	CEhcVector parms;
	InitializeVector(&parms);/*ehc_char*/

	for(i=0;*fld && (GetEhcInterface()->IsAlnum(*fld) || *fld == '_');++i)
		word[i] = *fld++;
	word[i]=0;

	if(end)
		*end = fld;
	if ((k = IsMacroID(pPreProc, word)) < 0)
		return NULL;
	for(i=bot;i<top;i++)
	{
		if(k == pPreProc->nForbid[i])
			return NULL;
	}
	pPreProc->nForbid[top++] = k;
	ForwardSkipWhiteSpace(fld);
	pMacro = pPreProc->oMacros.pTable[k];
	if (pMacro->nParaNum > 0)
	{
		if (*fld != '(')
			return NULL; /**/
		else
		{ /* collect arguments */
			++fld;
			ExprList(pPreProc, &parms, &fld, 1);
		}
		if(end)
			*end = fld;
	}
	if (parms.nSize != (ehc_uint)pMacro->nParaNum)
		PrintError(False, "macro syntax error");
	else
	{
		ehc_char * t, *s;
		p = pMacro->sTokenSeq;
		/* Now substitute in arguments and
		* expand as necessary
		*/
		i = 0; S = NULL;
		while (*p)
		{
			if (*p < 0)
			{
				if(pMacro->sProtect && pMacro->sProtect[-*p] == 1)
					p2 = NULL;
				else  if((i && S[i-1] == '#') || (i >= 2 && S[i-2] == '#'))
				{
					p2 = CreateCString(parms.pTable[-*p-1]);
					if(S[i-1] == '#')
						i--;
					else
						i -= 2;
				}
				else
					p2 = Process2(pPreProc, parms.pTable[-*p-1],top,top);
				if(p2)
				{
					ehc_char *p = p2;
					while (*p2)
						S = WriteChar(S, *p2++, MoD, i++);
					GetEhcInterface()->Free(p);
				}
				else
				{
					p2 = parms.pTable[-*p-1];
					while (*p2)
						S = WriteChar(S, *p2++, MoD, i++);
				}
				p++;
			}
			else
				S = WriteChar(S, *p++, MoD, i++);
		}
		if(S)
		{ /* Now rescan macro definition */
			ehc_char *S2=NULL;
			ehc_int k = 0;
			if(pMacro->nParaNum > 0)do
			{
				/* catch possible new macro funcs */
				/* bit of hack, but seems to work */
				/* should really check also for */
				/* brackets in strings and ehc_char literals */
				while(*fld && GetEhcInterface()->IsSpace(*fld))
					fld++;
				while(*fld == '(')
				{
					ehc_int parens = 0;
					do
					{
						S = WriteChar(S,*fld,MoD,i++);
						if(*fld == '(')
							parens++;
						else if(*fld == ')')
							parens--;
						if(! *++fld && parens)
						{
							/* need more input */
							GetPreLine(pPreProc);
							fld = pPreProc->sPreLine;
						}
					} while(*fld && parens);
					if(parens)
						PrintError(False, "missing `)'");
					*end = fld;
				}
			} while(GetEhcInterface()->IsSpace(*fld));

			s = S;
			S[i] = 0;
			while(*s)
			{
				if (*s == '"')
				{
					if (!pPreProc->nCMode)
						pPreProc->nCMode = CMstr;
					else if (pPreProc->nCMode == CMstr)
						pPreProc->nCMode = 0;
				}
				else if (*s == '\'')
				{
					if (!pPreProc->nCMode)
						pPreProc->nCMode = CMchr;
					else if (pPreProc->nCMode == CMchr)
						pPreProc->nCMode = 0;
				}
				else if(*s == '\\')
					S2 = WriteChar(S2, *s++, MoD, k++);
				else if((GetEhcInterface()->IsAlpha(*s) || *s == '_') && !pPreProc->nCMode)
				{
					t = CppExpand(pPreProc, s, &p, bot, top);
					if(t)
					{
						ehc_char * h = t;
						while(*t)
							S2 = WriteChar(S2,*t++,MoD,k++);
						GetEhcInterface()->Free(h);
						s = p;
					}
					else while(s < p)
						S2 = WriteChar(S2,*s++,MoD,k++);
					continue;
				}
				S2 = WriteChar(S2,*s++,MoD,k++);
			}
			S2[k] = 0;
			GetEhcInterface()->Free(S);
			S = S2;
		}
	}
	ClearVector(&parms, GetEhcInterface()->Free);
	return S;
}

static ehc_void DoDefMacro(CEhcPreProc* pPreProc, ehc_char *s)
{
	CEhcVector parms; /*ehc_char**/
	InitializeVector(&parms);
	ForwardSkipWhiteSpace(s);
	pPreProc->oDefMacro.sMacroName = s;
	while (*s && !GetEhcInterface()->IsSpace(*s) && *s != '(')
		++s;
	if (*s == '(')
	{
		*s++ = '\0';
		ExprList(pPreProc, &parms, &s, 0);
	}
	else if(*s)
		*s++ = '\0';
	ForwardSkipWhiteSpace(s);
	pPreProc->oDefMacro.sTokenSeq = s;
	pPreProc->oDefMacro.nParaNum = parms.nSize;
	SkipAll(s);
	--s;
	BackSkipWhiteSpace(s);
	*++s = '\0';
	if(parms.nSize)
		Parameterise(pPreProc, &parms);
	NewMacro(pPreProc, &parms);
	ClearVector(&parms, GetEhcInterface()->Free);
}

static ehc_void NewMacro(CEhcPreProc* pPreProc, CEhcVector *parms)
{
	ehc_int k;
	k = IsMacroID(pPreProc, pPreProc->oDefMacro.sMacroName);
	if (k > -1)
	{
		EhcMacro *old;
		old = pPreProc->oMacros.pTable[k];
		if ((old->nParaNum != (ehc_int)parms->nSize) ||
			!((old->sTokenSeq == empty_string && !*pPreProc->oDefMacro.sTokenSeq) ||
			StringCompare(old->sTokenSeq, pPreProc->oDefMacro.sTokenSeq) == 0))
		{
			ehc_char sDebug[1024];
			GetEhcInterface()->FormatPrint(sDebug, "Re-declaration of macro %s", pPreProc->oDefMacro.sMacroName);
			PrintError(False, sDebug);
		}
		if(pPreProc->oDefMacro.sProtect)
			GetEhcInterface()->Free(pPreProc->oDefMacro.sProtect);
	}
	else
	{
		EhcMacro * pNewMacro;
		pPreProc->oDefMacro.sMacroName = StringDuplicate(pPreProc->oDefMacro.sMacroName);
		pPreProc->oDefMacro.nHashCode = GetHashCode((ehc_uchar *)pPreProc->oDefMacro.sMacroName);
		pPreProc->oDefMacro.sFileName = SaveString(&pPreProc->oStringTab, GetFileName(&pPreProc->oFileTable));
		if(*pPreProc->oDefMacro.sTokenSeq)
			pPreProc->oDefMacro.sTokenSeq = StringDuplicate(pPreProc->oDefMacro.sTokenSeq);
		else /* allow for empty macro */
			pPreProc->oDefMacro.sTokenSeq = (ehc_char*)empty_string;
		pPreProc->oDefMacro.nParaNum = parms->nSize;
		pNewMacro = (EhcMacro*)GetEhcInterface()->Malloc(sizeof(pPreProc->oDefMacro));
		pNewMacro[0] = pPreProc->oDefMacro;
		Push(&pPreProc->oMacros, pNewMacro);
	}
	pPreProc->oDefMacro.sProtect = pPreProc->oDefMacro.sMacroName = pPreProc->oDefMacro.sTokenSeq = NULL;
}

static ehc_void RemMacroID(CEhcPreProc* pPreProc, ehc_int k)
{
	if(pPreProc->oMacros.nSize > (ehc_uint)k)
	{
		EhcMacro *pMacro = pPreProc->oMacros.pTable[k];
		FreeMacro(pMacro);
		Remove(&pPreProc->oMacros, k, GetEhcInterface()->Free);
	}
}

static ehc_void FreeMacro(EhcMacro * pMacro)
{
	if (pMacro->sMacroName)
	{
		GetEhcInterface()->Free(pMacro->sMacroName);
		pMacro->sMacroName = NULL;
	}
	if (pMacro->sTokenSeq)
	{
		if(pMacro->sTokenSeq != empty_string)
			GetEhcInterface()->Free(pMacro->sTokenSeq);
		pMacro->sTokenSeq = NULL;
	}
	if(pMacro->sProtect)
		GetEhcInterface()->Free(pMacro->sProtect);
}

static ehc_int IsMacroID(CEhcPreProc* pPreProc, ehc_char *sMacroName)
{
	ehc_ulong nHashCode = GetHashCode((ehc_uchar *)sMacroName);
	ehc_int i, nCount = pPreProc->oMacros.nSize;
	for(i=0; i<nCount; ++i)
	{
		EhcMacro *pMacro = pPreProc->oMacros.pTable[i];
		if (nHashCode == pMacro->nHashCode && StringCompare(pMacro->sMacroName, sMacroName) == 0)
		{
			Check4Res(pPreProc, pMacro);
			return i;
		}
	}
	return -1;
}

static ehc_void Check4Res(CEhcPreProc* pPreProc, EhcMacro * pMacro)
{
	ehc_char str[20];
	ehc_char * itoa(ehc_int, ehc_char *s, ehc_int);
	ehc_int c = 0, q = 1;
	ehc_char * s = NULL;
	if(pMacro->sMacroName[0] == '_' && pMacro->sMacroName[1] == '_')
	{
		switch(pMacro->sMacroName[2])
		{
		case 'F':
			if(StringCompare(pMacro->sMacroName,"__FILE__") == 0)
			{
				c = 1;
				s = GetFileName(&pPreProc->oFileTable);
				if(StringCompare(s,pMacro->sTokenSeq) == 0)
					return;
			}
			break;
		case 'L':
			if(StringCompare(pMacro->sMacroName,"__LINE__")== 0)
			{
				GetEhcInterface()->FormatPrint(str,"%d", GetLineNo(&pPreProc->oFileTable));
				s = str;
				c = 1;
				q = 0;
			}
			break;
		case 'D':
		case 'T':
			if(StringCompare(pMacro->sMacroName,"__DATE__") == 0 ||
				StringCompare(pMacro->sMacroName,"__TIME__") == 0)
			{
				ehc_char ct[12];
				c = 1;
				s = str;
				if(pMacro->sMacroName[2] == 'D')
				{
					GetEhcInterface()->GetDate(ct);
					StringCopyN(str, ct, 11);
					str[11] = 0;
				} else
				{
					GetEhcInterface()->GetDate(ct);
					StringCopyN(str, ct, 8);
					str[8] = 0;
				}
			}
			break;
		}
		if(c && s)
		{
			ehc_char * p;
			GetEhcInterface()->Free(pMacro->sTokenSeq);
			p = pMacro->sTokenSeq = (ehc_char*)GetEhcInterface()->Malloc(StringLength(s) + 3);
			if(q)
				*p++ = '\"';
			while(*s)
				*p++ = *s++;
			if(q)
				*p++ ='\"';
			*p='\0';
		}
	}
}

static ehc_void Parameterise(CEhcPreProc* pPreProc, CEhcVector *parms)
{
	ehc_char *s, *sMacroName, *op;
	ehc_int i;

	pPreProc->nCMode = 0;
	op = s = pPreProc->oDefMacro.sTokenSeq;
	while(*s)
	{
		if (!pPreProc->nCMode && (GetEhcInterface()->IsAlpha(*s) || *s == '_'))
		{
			sMacroName = s++;
			while (GetEhcInterface()->IsAlnum(*s) || *s == '_')
				++s;
			if ((i = FindParm(pPreProc, parms, sMacroName, (ehc_uint) (s - sMacroName))) != 0)
				*op++ = -i;
			else while (sMacroName < s)
				*op++ = *sMacroName++;
		}
		else
		{
			if (*s == '"')
			{
				if (!pPreProc->nCMode)
					pPreProc->nCMode = CMstr;
				else if (pPreProc->nCMode == CMstr)
					pPreProc->nCMode = 0;
			}
			else if (*s == '\'')
			{
				if (!pPreProc->nCMode)
					pPreProc->nCMode = CMchr;
				else if (pPreProc->nCMode == CMchr)
					pPreProc->nCMode = 0;
			}
			else if (GetEhcInterface()->IsSpace(*s) && !pPreProc->nCMode)
			{
				do s++; while(*s && GetEhcInterface()->IsSpace(*s));
				*--s = ' ';
			}
			*op++ = *s++;
		}
	}
	*op = '\0';
	if (pPreProc->nCMode)
	{
		if (pPreProc->nCMode == CMstr)
			PrintError(False, "missing end \" in macro token sequence");
		else
			PrintError(False, "missing end ' in macro token sequence");
	}
	else MergeTokens(pPreProc, &pPreProc->oDefMacro);
}

static ehc_void MergeTokens(CEhcPreProc* pPreProc, EhcMacro *pMacro)
{
	ehc_char * s, *seq;
	ehc_int left, right;

	if(pMacro->nParaNum)
		pMacro->sProtect = (ehc_char*)GetEhcInterface()->Malloc(pMacro->nParaNum + 1);

	s = seq  = pMacro->sTokenSeq;
	while(*s)
	{
		if(!pPreProc->nCMode && *s == '#' && *(s+1) == '#')
		{
			ehc_int d = (ehc_int)(s - seq) - 1;
			while(d >= 0 && seq[d] > 0 && GetEhcInterface()->IsSpace(seq[d]))
				d--;
			if(d < 0)
				PrintError(False, "macro definition begins with ##");
			left = d;
			d = (ehc_int)(s - seq) + 2;
			while(seq[d] > 0 && GetEhcInterface()->IsSpace(seq[d]))
				d++;
			if(!seq[d])
				PrintError(False, "macro definition ends with ##");
			right = d;
			s = seq + left + 1;
			if(seq[left] < 0)
				pMacro->sProtect[-seq[left]] = 1;
			if(seq[right] < 0)
				pMacro->sProtect[-seq[right]] = 1;
			while(seq[left] != 0)
				seq[++left] = seq[right++];
		}
		else if (*s == '"')
		{
			if (!pPreProc->nCMode)
				pPreProc->nCMode = CMstr;
			else if (pPreProc->nCMode == CMstr)
				pPreProc->nCMode = 0;
		}
		else if (*s == '\'')
		{
			if (!pPreProc->nCMode)
				pPreProc->nCMode = CMchr;
			else if (pPreProc->nCMode == CMchr)
				pPreProc->nCMode = 0;
		}
		s++;
	}
}

static ehc_int FindParm(CEhcPreProc* pPreProc, CEhcVector *parms,ehc_char *name, ehc_uint len)
{
	ehc_int i, nCount = parms->nSize;
	for(i=0; i<nCount; ++i)
	{
		ehc_char* v = (*parms).pTable[i];
		if(!StringCompareN(v, name, len))
			return i+1;
	}
	return 0;
}

static ehc_void ExprList(CEhcPreProc* pPreProc, CEhcVector *parms, ehc_char **p,ehc_int more)
{
	ehc_int c = 0, b = 0;
	ehc_char *s = *p;

	if(!pPreProc->pStr)
		pPreProc->pStr = (ehc_char*)GetEhcInterface()->Malloc(BUFSIZE);
	while (1)
	{
		for (; *s; ++s)
		{
			if(GetEhcInterface()->IsSpace(*s) && !pPreProc->nCMode)
			{
				while(*s && GetEhcInterface()->IsSpace(*s)) s++;
				if(!*s)
					break;
				s--;
			}
			if(c == (ehc_int)pPreProc->nSZ)
			{
				pPreProc->nSZ += BUFSIZE;
				pPreProc->pStr = (ehc_char*)GetEhcInterface()->Realloc(pPreProc->pStr,pPreProc->nSZ);
			}
			pPreProc->pStr[c++] = *s;
			switch (*s)
			{
			case '\\':
				pPreProc->pStr[c++] = *++s; /*get next ehc_char */
				continue;
			case '{': case '(':
				if (pPreProc->nCMode == 0)
					++b;
				continue;
			case ',':
				if (pPreProc->nCMode || b)
					continue;
				--c;
				break;
			case '}': case ')':
				if(b)
				{
					b--;
					continue;
				}
				if (pPreProc->nCMode)
					continue;
				--c;
				break;
			case '\'':
				switch (pPreProc->nCMode)
				{
				case 0:
					pPreProc->nCMode = CMchr;
				case CMstr:
					continue;
				}
				pPreProc->nCMode = 0;
				continue;
			case '\"':  /* " */
				switch (pPreProc->nCMode)
				{
				case 0:
					pPreProc->nCMode = CMstr;
				case CMchr:
					continue;
				}
				pPreProc->nCMode = 0;
				continue;
			default:
				continue;
			} /* end switch */
			break;
		} /* end for loop */

		if(!b && *s)
		{
			pPreProc->pStr[c] = '\0';
			/*strip leading white space */
			c = 0;
			while(pPreProc->pStr[c] && GetEhcInterface()->IsSpace(pPreProc->pStr[c]))
				c++;
			Push(parms, StringDuplicate(pPreProc->pStr + c));
		}
		if(!*s && more)
		{ /*need more input*/
			GetPreLine(pPreProc);
			if(c && !GetEhcInterface()->IsSpace(pPreProc->pStr[c-1]) && !GetEhcInterface()->IsSpace(*pPreProc->sPreLine))
				pPreProc->pStr[c++] = ' ';  /* need white space */
			s = pPreProc->sPreLine;
			continue;
		}
		if (*s == ')')
			break;
		if (*s != ',')
		{
			PrintError(False, "illegal macro definition");
			break;
		}
		c = 0;
		s++;
	} /* end while */
	*p = ++s;
	if(pPreProc->nSZ > (BUFSIZE << 2))
	{
		pPreProc->nSZ = BUFSIZE;
		pPreProc->pStr = (ehc_char*)GetEhcInterface()->Realloc(pPreProc->pStr,pPreProc->nSZ);
	}
}

static ehc_void DoPragma(CEhcPreProc* pPreProc, ehc_char *s)
{
}

static ehc_void ReplaceDefines(CEhcPreProc* pPreProc, ehc_char *S)
{
	ehc_char str[50];
	ehc_int cmode = 0;
	ehc_char *p;
	p = S;
	while(*S != '\0')
	{
		if(!cmode && (GetEhcInterface()->IsAlpha(*S) || *S == '_'))
		{
			if(S[0] == 'd' && S[1] == 'e' &&
				S[2] == 'f' && S[3] == 'i' &&
				S[4] == 'n' && S[5] == 'e' &&
				S[6] == 'd' && !GetEhcInterface()->IsAlpha(S[7]) &&
				S[7] != '_')
			{
				ehc_int br = 0;
				ehc_int i;
				S+=7;
				ForwardSkipWhiteSpace(S);
				if(*S=='(')
				{
					S++;
					br = 1;
					ForwardSkipWhiteSpace(S);
				}
				i = 0;
				while(i < 50 && (GetEhcInterface()->IsAlpha(*S) || *S == '_' || GetEhcInterface()->IsDigit(*S)))
					str[i++] = *S++;
				str[i] = '\0';
				if(br)
				{
					ForwardSkipWhiteSpace(S);
					if(*S != ')')
						PrintError(False, "missing ')'");
					else
						S++;
				}
				if(str[0] != '\0')
				{
					if(IsMacroID(pPreProc, str) > -1)
						*p = '1';
					else
						*p = '0';
					++p;
				} else
					PrintError(False, "missing identifier");
				continue;
			}
			do
			*p++ = *S++;
			while(GetEhcInterface()->IsAlpha(*S) || *S == '_' );
			continue;
		} if(*S == '\'')
			cmode = !cmode;
		*p++ = *S++;
	}
	*p = '\0';
}

static ehc_void ReplaceIdentifiers(CEhcPreProc* pPreProc, ehc_char *S)
{
	ehc_int i;
	ehc_char str[50];
	ehc_char *p;
	ehc_int cmode = 0;

	p = S;

	while(1)
	{
		while(GetEhcInterface()->IsSpace(*S))
			*p++ = *S++;
		if(!*S)
			return;
		if(!GetEhcInterface()->IsAlpha(*S) && *S != '_')
		{
			if(GetEhcInterface()->IsDigit(*S) || *S == '\'')
			{ /* skip throu numbers or literals*/
				while(*S && !GetEhcInterface()->IsSpace(*S))
					*p++ = *S++;
			}
			else while(*S && !GetEhcInterface()->IsSpace(*S) && *S != '_' && GetEhcInterface()->IsPunct(*S))
				*p++ = *S++;
			continue;
		}
		if(!cmode)
		{
			i = 0;
			while(i < 50 && (GetEhcInterface()->IsAlpha(*S) || *S == '_' || GetEhcInterface()->IsDigit(*S)) )
				str[i++] = *S++;
			str[i] = '\0';
			if(StringCompare(str,"sizeof") == 0)
				PrintError(False, "illegal sizeof operator");
			else
				*p++ = '0';
		}
		else
			*p++ = *S++;
	}
}

static ehc_int CppParse(CEhcPreProc* pPreProc, ehc_char *s)
{
	EhcPreProcInt res;
	pPreProc->pS = s;
	ReplaceDefines(pPreProc, pPreProc->pS);
	pPreProc->pS = s  = Process2(pPreProc, pPreProc->pS, 0, 0);
	ReplaceIdentifiers(pPreProc, pPreProc->pS);
	res = IfExpr(pPreProc, 0);
	if(s)
		GetEhcInterface()->Free(s);
	return (uval(res))?1:0;
}

static EhcPreProcInt IfExpr(CEhcPreProc* pPreProc, ehc_int k)
{
	EhcPreProcInt res;
	ehc_int k1, tk;
	res = ExprUnary(pPreProc);
	for(k1 = 10; k1 >= k; k1--)
		while((tk = GetTok(pPreProc, k1)))
			res = DoBinary(pPreProc, tk, res, IfExpr(pPreProc, k1>8?k1:k1+1));
		return res;
}

#define eval(a,l,r,op)\
{\
	if(tval(l) == SIGN)\
	sval(a) = sval(l) op sval(r);\
	else\
	uval(a) = uval(l) op uval(r);\
}

static EhcPreProcInt DoBinary(CEhcPreProc* pPreProc, ehc_int tk, EhcPreProcInt left, EhcPreProcInt right)
{
	EhcPreProcInt r;
	if(tval(left) == USIGN || tval(right) == USIGN)
		tval(r) = tval(left) = tval(right) = USIGN;
	else
		tval(r) = tval(left) = tval(right) = SIGN;
	switch(tk)
	{
	case BOR: eval(r,left,right, | ); break;
	case XOR: eval(r,left,right, ^ ); break;
	case AND: eval(r,left,right, & ); break;
	case LT:  eval(r,left,right, < ); break;
	case LEQ: eval(r,left,right, <= ); break;
	case EQ:  eval(r,left,right,  == ); break;
	case NEQ: eval(r,left,right,  != ); break;
	case GT:  eval(r,left,right, > ); break;
	case GEQ: eval(r,left,right, >= ); break;
	case LOR: eval(r,left,right, || ); break;
	case LAND: eval(r,left,right, && ); break;
	case LSHFT: eval(r,left,right, << ); break;
	case RSHFT: eval(r,left,right, >> ); break;
	case PLUS: eval(r,left,right, + ); break;
	case MINUS: eval(r,left,right, - ); break;
	case TIMES: eval(r,left,right, * ); break;
	case DIV:
		if(sval(right))
		{
			eval(r,left,right, / );
		}
		else
		{
			PrintError(False, "divide or mod by zero");
			sval(r) = 0;
		}
		break;
	case MOD:
		if(sval(right))
		{
			eval(r,left,right, % );
		}
		else
		{
			PrintError(False, "divide or mod by zero");
			sval(r) = 0;
		}
		break;
	}
	return r;
}

static ehc_int GetTok(CEhcPreProc* pPreProc, ehc_int k)
{
	ehc_int TK = 0;
	while(GetEhcInterface()->IsSpace(*pPreProc->pS))
		pPreProc->pS++;

	switch(k)
	{
	case 1: /* LOR */
		if(*pPreProc->pS == '|' && *(pPreProc->pS+1) == '|')
		{
			pPreProc->pS+=2;
			TK = LOR;
		}
		break;
	case 2: /* LAND */
		if(*pPreProc->pS == '&' && *(pPreProc->pS+1) == '&')
		{
			pPreProc->pS+=2;
			TK = LAND;
		}
	case 3: /* BOR */
		if(*pPreProc->pS == '|' && *(pPreProc->pS+1) != '|')
		{
			pPreProc->pS++;
			TK = BOR;
		}
		break;
	case 4: /* XOR */
		if(*pPreProc->pS == '^')
		{
			pPreProc->pS++;
			TK = GEQ;
		}
		break;
	case 5: /* AND */
		if(*pPreProc->pS == '&' && *(pPreProc->pS+1) != '&')
		{
			pPreProc->pS++;
			TK = GEQ;
		}
		break;
	case 6: /* EQ, NEQ */
		if(*pPreProc->pS == '=' && *(pPreProc->pS+1) == '=')
		{
			pPreProc->pS+=2;
			TK = EQ;
		}
		else if(*pPreProc->pS == '!' && *(pPreProc->pS+1) == '=')
		{
			pPreProc->pS+=2;
			TK = NEQ;
		}
		break;
	case 7: /* LT, LEQ, GT, GEQ */
		if(*pPreProc->pS == '<')
		{
			pPreProc->pS++;
			if(*pPreProc->pS == '=')
			{
				pPreProc->pS++;
				TK = LEQ;
			}
			else TK = LT;
		}
		else if(*pPreProc->pS == '>')
		{
			pPreProc->pS++;
			if(*pPreProc->pS == '=')
			{
				pPreProc->pS++;
				TK = GEQ;
			}
			else TK = GT;
		}
		break;
	case 8:  /* LSHFT, RSHFT */
		if(*pPreProc->pS == '<' && *(pPreProc->pS+1) == '<')
		{
			pPreProc->pS+=2;
			TK = LSHFT;
		}
		else if(*pPreProc->pS == '>' && *(pPreProc->pS+1) == '>')
		{
			pPreProc->pS+=2;
			TK = RSHFT;
		}
		break;
	case 9: /* PLUS, MINUS */
		if(*pPreProc->pS == '-')
		{
			pPreProc->pS++;
			TK = MINUS;
		}
		else if(*pPreProc->pS == '+')
		{
			pPreProc->pS++;
			TK = PLUS;
		}
		break;
	case 10: /* TIMES, DIV, MOD */
		if(*pPreProc->pS == '*')
		{
			pPreProc->pS++;
			TK = TIMES;
		}
		else if(*pPreProc->pS == '/')
		{
			pPreProc->pS++;
			TK = DIV;
		}
		else if(*pPreProc->pS == '%')
		{
			pPreProc->pS++;
			TK = MOD;
		}
		break;
	}
	return TK;
}

static ehc_int GetOct(ehc_int x)
{
	return x>='0'&&x<='7'? x-'0':-1;
}

static ehc_int GetHex(ehc_int x)
{
	if (x >= '0' && x <= '9')
		x -= '0';
	else if (x >= 'a' && x <= 'f')
		x = x - 'a' + 10;
	else if (x >= 'A' && x <= 'F')
		x = x - 'A' + 10;
	else
		x = -1;
	return x;
}

static ehc_int GetDec(ehc_int x)
{
	return x >= '0' && x <= '9' ? x-'0':-1;
}

static EhcPreProcInt GetNumber(CEhcPreProc* pPreProc)   /* collect hex, octal and decimal integers */
{
	ehc_int (*f)(ehc_int x);
	ehc_int radix,val;

	EhcPreProcInt res = {{0},SIGN};

	if(*pPreProc->pS == '0')
	{
		pPreProc->pS++;
		if(*pPreProc->pS == 'x' || *pPreProc->pS == 'X')
		{ /* get hex number */
			pPreProc->pS++;
			radix = 16;
			f = GetHex;
		}
		else
		{ /* get octal number */
			radix = 8;
			f = GetOct;
		}
	}
	else
	{ /* get decimal number */
		radix = 10;
		f = GetDec;
	}
	while((val = (*f)(*pPreProc->pS++)) >= 0)
		uval(res) = uval(res) * radix + val;
	pPreProc->pS--;

	if(uval(res) > MAX_EHC_LONG)
		tval(res) = USIGN;

	/* check for prefix */
	if(*pPreProc->pS=='u' || *pPreProc->pS=='U')
	{
		pPreProc->pS++;
		tval(res) = USIGN;
	}
	if(*pPreProc->pS=='l' || *pPreProc->pS=='L')
		pPreProc->pS++;
	return res;
}

static ehc_int GetCharConst(CEhcPreProc* pPreProc)
{
	ehc_int c;
	switch (*pPreProc->pS)
	{
	case 'n': c = '\n'; break; /* newline */
	case 't': c = '\t'; break; /* tabspace */
	case 'v': c = '\v'; break; /* vertical tab */
	case 'b': c = '\b'; break; /* backspace */
	case 'r': c = '\r'; break; /* carriage return */
	case 'f': c = '\f'; break; /* formfeed */
	case 'a': c = '\a'; break; /* bell */
	case '\\': c = '\\'; break; /* backslash */
	case '\'': c = '\''; break; /* single quote */
	case '"': c = '\"'; break; /* double quote */
	case 'x':			/* string of hex characters */
	case 'X':
		{
			ehc_int i, val = 0;
			pPreProc->pS++;
			while ((i = GetHex(*pPreProc->pS)) > -1)
			{
				pPreProc->pS++;
				val = val * 16 + i;
			}
			if (val > 255)
				PrintError(False, "illegal character hex value");
			c = val;
		}
		break;
	default:
		if (GetEhcInterface()->IsDigit(*pPreProc->pS))
		{	/* treat as octal characters */
			ehc_int i, val = 0;
			while ((i = GetOct(*pPreProc->pS)) > -1)
			{
				val = val * 8 + i;
				pPreProc->pS++;
			}
			if (val > 255)
				PrintError(False, "illegal character octal value");
			c = val;
		}
		else
		{
			ehc_char sDebug[64];
			GetEhcInterface()->FormatPrint(sDebug, "Illegal character escape sequence `\\%c'", *pPreProc->pS);
			PrintError(False, sDebug);
			c = *pPreProc->pS++;
		}
		break;
	}
	return c;
}

static EhcPreProcInt ExprUnary(CEhcPreProc* pPreProc)
{
	EhcPreProcInt res;
	while(GetEhcInterface()->IsSpace(*pPreProc->pS))
		pPreProc->pS++;
	if(GetEhcInterface()->IsDigit(*pPreProc->pS))
	{
		res = GetNumber(pPreProc);
	}
	else if( *pPreProc->pS == '(')
	{
		pPreProc->pS++;
		res = IfExpr(pPreProc, 0);
		if(*pPreProc->pS != ')')
			PrintError(False, "unbalanced parenthesis");
		pPreProc->pS++;
	}
	else if(*pPreProc->pS == '!')
	{
		pPreProc->pS++;
		res = ExprUnary(pPreProc);
		uval(res) = !uval(res);
	}
	else if(*pPreProc->pS == '-')
	{
		pPreProc->pS++;
		if(*pPreProc->pS == '-')
			PrintError(False, "-- not allowed in operand of #if");
		res = ExprUnary(pPreProc);
		tval(res) = SIGN;
		sval(res) = -(long)(uval(res));
	}
	else if(*pPreProc->pS == '+')
	{
		pPreProc->pS++;
		if(*pPreProc->pS == '+')
			PrintError(False, "++ not allowed in operand of #if");
		res = ExprUnary(pPreProc);
	}
	else if(*pPreProc->pS == '~')
	{
		pPreProc->pS++;
		res = ExprUnary(pPreProc);
		uval(res) = ~uval(res);
	}
	else if(*pPreProc->pS == '\'')
	{ /* ehc_char constants */
		pPreProc->pS++;
		if(*pPreProc->pS == '\\')
		{
			pPreProc->pS++;
			uval(res) = GetCharConst(pPreProc);
		}
		else
			uval(res) = *pPreProc->pS++;
		if(*pPreProc->pS != '\'')
			PrintError(False, "missing closing single quote '");
		else
			pPreProc->pS++;
		tval(res) = SIGN;
	}
	else
		PrintError(False, "illegal constant expression");
	return res;
}

ehc_int InitializePreProcessor(CEhcPreProc* pPreProc, ehc_char* sFileName, ehc_int nArgc, ehc_char* sArgv[])
{
	ehc_int i;
	ehc_char * sRet;

	InitializeFileTable(&pPreProc->oFileTable);
	if(PushFile(&pPreProc->oFileTable, sFileName))
	{
		ClearFileTable(&pPreProc->oFileTable);
		return 1;
	}
	InitStringTable(&pPreProc->oStringTab, False);
	InitializeVector(&pPreProc->oMacros);
	pPreProc->sLine = NULL;
	pPreProc->sPreLineCur = NULL;
	pPreProc->sPreLine = NULL;
	pPreProc->nPreLineLen = 0;
	pPreProc->sProLineCur = NULL;
	pPreProc->sProLine = NULL;
	pPreProc->nProLineLen = 0;
	pPreProc->sFileName = NULL;
	pPreProc->nFileLineNo = 0;
	pPreProc->nIfLevel = 0;
	pPreProc->nSkip = 0;
	pPreProc->nCMode = 0;
	MemorySet(&pPreProc->oDefMacro, 0, sizeof(pPreProc->oDefMacro));
	pPreProc->nSZ = 0;
	pPreProc->pStr = NULL;
	pPreProc->pS = NULL;

	DefineMacro(pPreProc, "__FILE__=emp");
	DefineMacro(pPreProc, "__LINE__=emp");
	DefineMacro(pPreProc, "__DATE__=emp");
	DefineMacro(pPreProc, "__TIME__=emp");
	DefineMacro(pPreProc, "__STDC__=1");

	for(i=1; i<nArgc; ++i)
	{
		if(!MemoryCompare(sArgv[i], "-I", 2))
		{
			sRet = sArgv[i] + 2;
			if(!sRet[0])
			{
				++i;
				if(i < nArgc)
					sRet = sArgv[i];
				else
					sRet = NULL;
			}
			if(sRet)
				InsertSearchPath(&pPreProc->oFileTable, sRet);
		}
		else if(!MemoryCompare(sArgv[i], "-D", 2))
		{
			sRet = sArgv[i] + 2;
			if(!sRet[0])
			{
				++i;
				if(i < nArgc)
					sRet = sArgv[i];
				else
					sRet = NULL;
			}
			if(sRet)
				DefineMacro(pPreProc, sRet);
		}
	}

	return 0;
}

ehc_void ClearPreProcessor(CEhcPreProc* pPreProc)
{
	if(pPreProc->sPreLine)
		GetEhcInterface()->Free(pPreProc->sPreLine);
	if(pPreProc->sProLine)
		GetEhcInterface()->Free(pPreProc->sProLine);
	if(pPreProc->pStr)
		GetEhcInterface()->Free(pPreProc->pStr);
	ClearVector(&pPreProc->oMacros, ClearMacro);
	ClearFileTable(&pPreProc->oFileTable);
	ClearStringTable(&pPreProc->oStringTab);
}

CEhcCodeLine* GetProcLine(CEhcPreProc* pPreProc)
{
	if(pPreProc->sPreLine)
		GetEhcInterface()->Free(pPreProc->sPreLine);
	if(pPreProc->sProLine)
		GetEhcInterface()->Free(pPreProc->sProLine);
	pPreProc->sPreLine = (ehc_char *) GetEhcInterface()->Malloc(REBUFF_INCREMENT);
	pPreProc->sProLine = (ehc_char *) GetEhcInterface()->Malloc(REBUFF_INCREMENT);
	pPreProc->nPreLineLen = REBUFF_INCREMENT;
	pPreProc->nProLineLen = REBUFF_INCREMENT;
	pPreProc->sPreLineCur = pPreProc->sPreLine;
	pPreProc->sProLineCur = pPreProc->sProLine;
	while(1)
	{
		GetPreLine(pPreProc);//读取预处理前的行
		if(!ControlLine(pPreProc))//预处理行
		{
			Process(pPreProc);//宏替换处理
			break;
		}
	}
	pPreProc->oCodeLine.sLine = pPreProc->sProLine;
	pPreProc->oCodeLine.sFileName = pPreProc->sFileName;
	pPreProc->oCodeLine.nFileLineNo = pPreProc->nFileLineNo;
	return &pPreProc->oCodeLine;
}
