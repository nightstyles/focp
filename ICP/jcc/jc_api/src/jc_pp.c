
#include "jc_pp.h"
#include "jc_keyword.h"

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

typedef struct JcPreProcInt
{
    union v_t
	{
		jc_long s;		/* signed value */
		jc_ulong u;		/* jc_uint value */
    }v;
    jc_int type;
}JcPreProcInt;

#define sval(x)  ((x).v.s)
#define uval(x)  ((x).v.u)
#define tval(x)  ((x).type)

enum
{
	LOR = 1, LAND, BOR,  XOR,   AND,    EQ,  NEQ,  LT, LEQ,
    GT, GEQ, LSHFT, RSHFT,PLUS, MINUS, TIMES, DIV, MOD
};

#define MoD  10
#define BUFSIZE   512
static const jc_char * empty_string = " ";

#define SkipAll(a)	while(*(a))++a
#define ForwardSkipWhiteSpace(a)	while(*(a) && g_oInterface.IsSpace(*(a)))++a
#define BackSkipWhiteSpace(a)	while(*(a) && g_oInterface.IsSpace(*(a)))--a
#define SkipToChar(a,b)	while(*(a) && *(a) != b)++a
#define SkipNonWhiteSpaceChar(a)		while(*(a) && !g_oInterface.IsSpace(*(a)))++a

static jc_void PutError(CJcPreProc* pPreProc, jc_char* s);
static jc_int GetChar(CJcPreProc* pPreProc);
static jc_void UnGetChar(CJcPreProc* pPreProc);
static jc_void WritePreChar(CJcPreProc* pPreProc, jc_char c);
static jc_char* WriteProChar(CJcPreProc* pPreProc, jc_char c);
static jc_void GetPreLine(CJcPreProc* pPreProc);
static jc_int ControlLine(CJcPreProc* pPreProc);
static jc_void Process(CJcPreProc* pPreProc);
static jc_char * CppExpand(CJcPreProc* pPreProc, jc_char *fld,jc_char **end, jc_int bot, jc_int top);
static jc_char * Process2(CJcPreProc* pPreProc, jc_char * line, jc_int bot, jc_int top);
static jc_void DefineMacro(CJcPreProc* pPreProc, jc_char *def);
static jc_void DoDefMacro(CJcPreProc* pPreProc, jc_char *s);
static jc_void NewMacro(CJcPreProc* pPreProc, CJcVector *parms);
static jc_void RemMacroID(CJcPreProc* pPreProc, jc_int k);
static jc_void FreeMacro(JcMacro * pMacro);
static jc_int IsMacroID(CJcPreProc* pPreProc, jc_char *sMacroName);
static jc_void Check4Res(CJcPreProc* pPreProc, JcMacro * pMacro);
static jc_void Parameterise(CJcPreProc* pPreProc, CJcVector *parms);
static jc_void MergeTokens(CJcPreProc* pPreProc, JcMacro *pMacro);
static jc_int FindParm(CJcPreProc* pPreProc, CJcVector *parms,jc_char *name, jc_uint len);
static jc_void ExprList(CJcPreProc* pPreProc, CJcVector * parms, jc_char **p,jc_int more);
static jc_void DoPragma(CJcPreProc* pPreProc, jc_char *s);
static jc_void ReplaceDefines(CJcPreProc* pPreProc, jc_char *S);
static jc_void ReplaceIdentifiers(CJcPreProc* pPreProc, jc_char *S);
static jc_int CppParse(CJcPreProc* pPreProc, jc_char *s);
static JcPreProcInt IfExpr(CJcPreProc* pPreProc, jc_int k);
static JcPreProcInt DoBinary(CJcPreProc* pPreProc, jc_int tk, JcPreProcInt left, JcPreProcInt right);
static JcPreProcInt ExprUnary(CJcPreProc* pPreProc);
static JcPreProcInt GetNumber(CJcPreProc* pPreProc);
static jc_int GetTok(CJcPreProc* pPreProc, jc_int k);
static jc_int GetCharConst(CJcPreProc* pPreProc);

static CJcKeyword g_pJcKeyWords[] =
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

static jc_char *WriteChar(jc_char *s, jc_char c, jc_int mod, jc_int i)
{
	if(!(i%mod))
	{
		if(!s)
			s = (jc_char *)g_oInterface.Malloc(i + mod + 1);
		else
			s = (jc_char *)g_oInterface.Realloc(s, i + mod + 1);
	}
	s[i] = c;
	return s;
}

static jc_void ReBuff(jc_char **buf, jc_int *len)
{
	*buf = (jc_char *) g_oInterface.Realloc(*buf, *len + REBUFF_INCREMENT);
	*len += REBUFF_INCREMENT;
}

static jc_char * CreateCString(jc_char * sStr)
{
	jc_int i = 0;
	jc_char *S=NULL;
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

static jc_ulong GetHashCode(jc_uchar *s)
{
	jc_ulong t, h,i;
	h = 0;
	while(*s)
	{
		for(t = i=0;*s && i<32;i+=8)
			t |= (*s++ << i);
		h += t;
	}
	return h;
}

static jc_void PutError(CJcPreProc* pPreProc, jc_char* s)
{
	CompileError(pPreProc->pErrorSystem, 0, "%s", s);
}

static void JC_CALL ClearMacro(JcMacro *pMacro)
{
	FreeMacro(pMacro);
	g_oInterface.Free(pMacro);
}

/*
* for processing -Dname[=value] switch
* def = name[=value]
*/
static jc_void DefineMacro(CJcPreProc* pPreProc, jc_char *sDefine)
{
	if(sDefine)
	{
		jc_char * sEqual;
		jc_int nLen = StringLength(sDefine);
		jc_char * sStr = (jc_char*)g_oInterface.Malloc(nLen+3);
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
		g_oInterface.Free(sStr);
	}
}

static jc_void WritePreChar(CJcPreProc* pPreProc, jc_char c)
{
	*pPreProc->sPreLineCur++ = c;
	if (pPreProc->sPreLineCur >= pPreProc->sPreLine + pPreProc->nPreLineLen)
	{
		jc_uint d = pPreProc->sPreLineCur - pPreProc->sPreLine;
		ReBuff(&pPreProc->sPreLine, (jc_int*)&pPreProc->nPreLineLen);
		pPreProc->sPreLineCur = pPreProc->sPreLine + d;
	}
}

static jc_char* WriteProChar(CJcPreProc* pPreProc, jc_char c)
{
	*pPreProc->sProLineCur++ = c;
	if(pPreProc->sProLineCur >= pPreProc->sProLine + pPreProc->nProLineLen)
	{
		jc_uint d = pPreProc->sProLineCur - pPreProc->sProLine;
		ReBuff(&pPreProc->sProLine, (jc_int*)&pPreProc->nProLineLen);
		pPreProc->sProLineCur = pPreProc->sProLine + d;
	}
	*pPreProc->sProLineCur = '\0';
	return pPreProc->sProLineCur - 1;
}

static jc_void GetPreLine(CJcPreProc* pPreProc)
{
	jc_int c;
	jc_int nLineComment = 0;
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
			case EOF:
				PutError(pPreProc, "unexpected end of file");
			case '\n':
				continue;
			}
			WritePreChar(pPreProc, '\\');

		default:
			/* pPreProc->nSkip leading white space*/
			if(!g_oInterface.IsSpace(c) || pPreProc->sPreLineCur != pPreProc->sPreLine)
			{
				if(pPreProc->sPreLineCur == pPreProc->sPreLine)
				{
					pPreProc->sFileName = GetFileName(&pPreProc->oFileTable);
					pPreProc->nFileLineNo = GetLineNo(&pPreProc->oFileTable);
				}
				WritePreChar(pPreProc, (jc_char)c);
			}
			continue;

		case '/':
			if (pPreProc->nCMode == 0)
			{
				/* check for comment*/
				if((c = GetChar(pPreProc)) == '*' || c == '/')
				{
					jc_int i = 0;
					/* Allow for C++ style comments */
					if(c == '/')
						nLineComment = 1;
					/* strip comment out */
					c = GetChar(pPreProc);
					if(pPreProc->sPreLineCur != pPreProc->sPreLine)
						WritePreChar(pPreProc, ' ');  /* replace comment with space */
					while(c != EOF)
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
							c = GetChar(pPreProc); /* waste next jc_char */
							c = GetChar(pPreProc);
						}
						else c = GetChar(pPreProc);
					}
				}
				else
				{
					WritePreChar(pPreProc, '/');
					if(c != EOF)
						WritePreChar(pPreProc, (jc_char)c);
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
					PutError(pPreProc, "unbalanced \"");  /* " */
				else
					WritePreChar(pPreProc, '\"');  /* " */
				pPreProc->nCMode = 0;
			}
			else if (pPreProc->nCMode == CMchr)
			{
				if (!pPreProc->nSkip)
					PutError(pPreProc, "unbalanced \'");
				else
					WritePreChar(pPreProc, '\'');
				pPreProc->nCMode = 0;
			}
			WritePreChar(pPreProc, '\n');
			break;

		case EOF:
			if(pPreProc->nIfLevel && GetFileCount(&pPreProc->oFileTable) == 2)
				PutError(pPreProc, "unterminated `#if' conditional");
			if (pPreProc->sPreLineCur != pPreProc->sPreLine)
				break;
			if (GetFileCount(&pPreProc->oFileTable) >= 1)
			{
				PopFile(&pPreProc->oFileTable);
				if(GetFileCount(&pPreProc->oFileTable) >= 1)
					continue;
			}
			else WritePreChar(pPreProc, EOF);
			break;
		}
		break;
	}
	*pPreProc->sPreLineCur = '\0';
}

static jc_int GetChar(CJcPreProc* pPreProc)
{
	jc_int nRet;
	if(!pPreProc->sLine || !pPreProc->sLine[0])
	{
		pPreProc->sLine = (jc_char*)GetLineFromFileTable(&pPreProc->oFileTable);
		if(!pPreProc->sLine || !pPreProc->sLine[0])
			return EOF;
	}
	nRet = (jc_int)(jc_uchar)pPreProc->sLine[0];
	++pPreProc->sLine;
	return nRet;
}

static jc_void UnGetChar(CJcPreProc* pPreProc)
{
	if(pPreProc->sLine)
		--pPreProc->sLine;
	else
		pPreProc->sLine = (jc_char*)"";
}

static jc_int ControlLine(CJcPreProc* pPreProc)
{
	jc_int k;
	jc_char key[25];
	jc_char *s, *e;
	if (*pPreProc->sPreLine == '#')
	{
		s = pPreProc->sPreLine + 1;
		ForwardSkipWhiteSpace(s);
		if(!*s) /* test for null directive */
			return 1;

		for(k=0; g_oInterface.IsAlpha(*s); k++)
			key[k] = *s++;
		key[k] = '\0';

		k = IsKeyWord(key, g_pJcKeyWords, sizeof(g_pJcKeyWords)/sizeof(CJcKeyword));
		ForwardSkipWhiteSpace(s);

		switch (k)
		{
		case DEFINE:
			if (!pPreProc->nSkip)
			{
				if(*s)
					DoDefMacro(pPreProc, s);
				else
					PutError(pPreProc, "empty '#define' directive");
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
				PutError(pPreProc, "unmatched #else");
			else if (pPreProc->nSkip == pPreProc->nIfLevel)
				pPreProc->nSkip = 0;
			else if (pPreProc->nSkip == 0)
				pPreProc->nSkip = pPreProc->nIfLevel;
			break;

		case ENDIF:
			if (pPreProc->nIfLevel == 0)
				PutError(pPreProc, "unmatched #endif");
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
					PutError(pPreProc, "empty '#if' directive");
			}
			break;
		case IFDEF:
		case IFNDEF:
			++pPreProc->nIfLevel;
			if (!pPreProc->nSkip)
			{
				if (g_oInterface.IsAlpha(*s) || *s == '_')
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
					PutError(pPreProc, "illegal macro identifier");
			}
			break;

		case INCLUDE:
			if(pPreProc->nSkip)
				break;
			if(!*s)
			{
				PutError(pPreProc, "empty '#include' directive");
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
					PutError(pPreProc, "missing \"");
				else if (pPreProc->nCMode == CMang && *(s - 1) != '>')
					PutError(pPreProc, "missing >");
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

			if(pPreProc->bSupportOutput)
			{
				jc_void* fp;
				jc_int nBufLen;
				jc_char sFileName2[JC_MAX_PATH], sInfo[JC_MAX_PATH+20];
				g_oInterface.FormatPrint(sFileName2, "%s.p", pPreProc->sFileName);
				if(pPreProc->nCMode != CMang)
					g_oInterface.FormatPrint(sInfo, "#include \"%s\"\n", e);
				else
					g_oInterface.FormatPrint(sInfo, "#include <%s>\n", e);
				nBufLen = StringLength(sInfo);
				fp = g_oInterface.OpenFile(sFileName2, "a+b");
				g_oInterface.WriteFile(fp, sInfo, nBufLen);
				g_oInterface.CloseFile(fp);
			}
	
			if(PushIncludeFile(&pPreProc->oFileTable, e, (pPreProc->nCMode != CMang)))
			{
				jc_char sDebug[512];
				g_oInterface.FormatPrint(sDebug, "open head file %s failure",e);
				PutError(pPreProc, sDebug);
			}
			else if(pPreProc->bSupportOutput)
			{
				jc_void* fp;
				jc_char sFileName2[JC_MAX_PATH];
				g_oInterface.FormatPrint(sFileName2, "%s.p", pPreProc->oFileTable.pFileTable->sFileName);
				fp = g_oInterface.OpenFile(sFileName2, "wb");
				g_oInterface.CloseFile(fp);
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
				jc_char *S = Process2(pPreProc, s, 0, 0);
				PutError(pPreProc, S);
				if(S)
					g_oInterface.Free(S);
			}
			break;

		case PRAGMA:
			if(!pPreProc->nSkip)
			{
				jc_char *S = Process2(pPreProc, s, 0, 0);
				if(S)
				{
					DoPragma(pPreProc, S);
					g_oInterface.Free(S);
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

static jc_void Process(CJcPreProc* pPreProc)
{
	jc_char *S;
	*(pPreProc->sProLineCur = pPreProc->sProLine) = '\0';
	S = Process2(pPreProc, pPreProc->sPreLine, 0, 0);
	if(S)
	{
		jc_char* sOld = S;
		while(*S)
			WriteProChar(pPreProc, *S++);
		g_oInterface.Free(sOld);
	}
	pPreProc->nCMode = 0;
}

static jc_char * Process2(CJcPreProc* pPreProc, jc_char * line, jc_int bot, jc_int top)
{
	jc_int k = 0;
	jc_char *p, *s, *S = NULL;
	jc_char * lp = line;

	while (*lp)
	{
		if (!pPreProc->nCMode && (g_oInterface.IsAlpha(*lp) || *lp == '_'))
		{
			s = lp;
			p = CppExpand(pPreProc, lp, &lp, bot, top);
			if(p)
			{
				s = p;
				while(*p)
					S = WriteChar(S, *p++, MoD, k++);
				g_oInterface.Free(s);
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
				if (!*lp)	/* get second jc_char */
					break;
			}
			S = WriteChar(S, *lp++, MoD, k++);
		}
	}
	if(S)
		S[k] = '\0';
	return S;
}

static jc_char * CppExpand(CJcPreProc* pPreProc, jc_char *fld, jc_char **end, jc_int bot, jc_int top)
{
	jc_char word[128];
	jc_int i = 0, k;
	jc_char *p, *p2;
	JcMacro *pMacro;
	jc_char *S =NULL;
	CJcVector parms;
	InitializeVector(&parms);/*jc_char*/

	for(i=0;*fld && (g_oInterface.IsAlnum(*fld) || *fld == '_');++i)
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
	if (parms.nSize != (jc_uint)pMacro->nParaNum)
		PutError(pPreProc, "macro syntax error");
	else
	{
		jc_char * t, *s;
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
					jc_char *p = p2;
					while (*p2)
						S = WriteChar(S, *p2++, MoD, i++);
					g_oInterface.Free(p);
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
			jc_char *S2=NULL;
			jc_int k = 0;
			if(pMacro->nParaNum > 0)do
			{
				/* catch possible new macro funcs */
				/* bit of hack, but seems to work */
				/* should really check also for */
				/* brackets in strings and jc_char literals */
				while(*fld && g_oInterface.IsSpace(*fld))
					fld++;
				while(*fld == '(')
				{
					jc_int parens = 0;
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
						PutError(pPreProc, "missing `)'");
					*end = fld;
				}
			} while(g_oInterface.IsSpace(*fld));

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
				else if((g_oInterface.IsAlpha(*s) || *s == '_') && !pPreProc->nCMode)
				{
					t = CppExpand(pPreProc, s, &p, bot, top);
					if(t)
					{
						jc_char * h = t;
						while(*t)
							S2 = WriteChar(S2,*t++,MoD,k++);
						g_oInterface.Free(h);
						s = p;
					}
					else while(s < p)
						S2 = WriteChar(S2,*s++,MoD,k++);
					continue;
				}
				S2 = WriteChar(S2,*s++,MoD,k++);
			}
			S2[k] = 0;
			g_oInterface.Free(S);
			S = S2;
		}
	}
	ClearVector(&parms, g_oInterface.Free);
	return S;
}

static jc_void DoDefMacro(CJcPreProc* pPreProc, jc_char *s)
{
	CJcVector parms; /*jc_char**/
	InitializeVector(&parms);
	ForwardSkipWhiteSpace(s);
	pPreProc->oDefMacro.sMacroName = s;
	while (*s && !g_oInterface.IsSpace(*s) && *s != '(')
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
	ClearVector(&parms, g_oInterface.Free);
}

static jc_void NewMacro(CJcPreProc* pPreProc, CJcVector *parms)
{
	jc_int k;
	k = IsMacroID(pPreProc, pPreProc->oDefMacro.sMacroName);
	if (k > -1)
	{
		JcMacro *old;
		old = pPreProc->oMacros.pTable[k];
		if ((old->nParaNum != (jc_int)parms->nSize) ||
			!((old->sTokenSeq == empty_string && !*pPreProc->oDefMacro.sTokenSeq) ||
			StringCompare(old->sTokenSeq, pPreProc->oDefMacro.sTokenSeq) == 0))
		{
			jc_char sDebug[1024];
			g_oInterface.FormatPrint(sDebug, "Re-declaration of macro %s", pPreProc->oDefMacro.sMacroName);
			PutError(pPreProc, sDebug);
		}
		if(pPreProc->oDefMacro.sProtect)
			g_oInterface.Free(pPreProc->oDefMacro.sProtect);
	}
	else
	{
		JcMacro * pNewMacro;
		pPreProc->oDefMacro.sMacroName = StringDuplicate(pPreProc->oDefMacro.sMacroName);
		pPreProc->oDefMacro.nHashCode = GetHashCode((jc_uchar *)pPreProc->oDefMacro.sMacroName);
		pPreProc->oDefMacro.sFileName = SaveString(&pPreProc->oStringTab, GetFileName(&pPreProc->oFileTable));
		if(*pPreProc->oDefMacro.sTokenSeq)
			pPreProc->oDefMacro.sTokenSeq = StringDuplicate(pPreProc->oDefMacro.sTokenSeq);
		else /* allow for empty macro */
			pPreProc->oDefMacro.sTokenSeq = (jc_char*)empty_string;
		pPreProc->oDefMacro.nParaNum = parms->nSize;
		pNewMacro = (JcMacro*)g_oInterface.Malloc(sizeof(pPreProc->oDefMacro));
		pNewMacro[0] = pPreProc->oDefMacro;
		Push(&pPreProc->oMacros, pNewMacro);
	}
	pPreProc->oDefMacro.sProtect = pPreProc->oDefMacro.sMacroName = pPreProc->oDefMacro.sTokenSeq = NULL;
}

static jc_void RemMacroID(CJcPreProc* pPreProc, jc_int k)
{
	if(pPreProc->oMacros.nSize > (jc_uint)k)
	{
		JcMacro *pMacro = pPreProc->oMacros.pTable[k];
		FreeMacro(pMacro);
		Remove(&pPreProc->oMacros, k, g_oInterface.Free);
	}
}

static jc_void FreeMacro(JcMacro * pMacro)
{
	if (pMacro->sMacroName)
	{
		g_oInterface.Free(pMacro->sMacroName);
		pMacro->sMacroName = NULL;
	}
	if (pMacro->sTokenSeq)
	{
		if(pMacro->sTokenSeq != empty_string)
			g_oInterface.Free(pMacro->sTokenSeq);
		pMacro->sTokenSeq = NULL;
	}
	if(pMacro->sProtect)
		g_oInterface.Free(pMacro->sProtect);
}

static jc_int IsMacroID(CJcPreProc* pPreProc, jc_char *sMacroName)
{
	jc_ulong nHashCode = GetHashCode((jc_uchar *)sMacroName);
	jc_int i, nCount = pPreProc->oMacros.nSize;
	for(i=0; i<nCount; ++i)
	{
		JcMacro *pMacro = pPreProc->oMacros.pTable[i];
		if (nHashCode == pMacro->nHashCode && StringCompare(pMacro->sMacroName, sMacroName) == 0)
		{
			Check4Res(pPreProc, pMacro);
			return i;
		}
	}
	return -1;
}

static jc_void Check4Res(CJcPreProc* pPreProc, JcMacro * pMacro)
{
	jc_char str[20];
	jc_char * itoa(jc_int, jc_char *s, jc_int);
	jc_int c = 0, q = 1;
	jc_char * s = NULL;
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
				g_oInterface.FormatPrint(str,"%d", GetLineNo(&pPreProc->oFileTable));
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
				jc_char ct[12];
				c = 1;
				s = str;
				if(pMacro->sMacroName[2] == 'D')
				{
					g_oInterface.GetDate(ct);
					StringCopyN(str, ct, 11);
					str[11] = 0;
				} else
				{
					g_oInterface.GetDate(ct);
					StringCopyN(str, ct, 8);
					str[8] = 0;
				}
			}
			break;
		}
		if(c && s)
		{
			jc_char * p;
			g_oInterface.Free(pMacro->sTokenSeq);
			p = pMacro->sTokenSeq = (jc_char*)g_oInterface.Malloc(StringLength(s) + 3);
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

static jc_void Parameterise(CJcPreProc* pPreProc, CJcVector *parms)
{
	jc_char *s, *sMacroName, *op;
	jc_int i;

	pPreProc->nCMode = 0;
	op = s = pPreProc->oDefMacro.sTokenSeq;
	while(*s)
	{
		if (!pPreProc->nCMode && (g_oInterface.IsAlpha(*s) || *s == '_'))
		{
			sMacroName = s++;
			while (g_oInterface.IsAlnum(*s) || *s == '_')
				++s;
			if ((i = FindParm(pPreProc, parms, sMacroName, (jc_uint) (s - sMacroName))) != 0)
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
			else if (g_oInterface.IsSpace(*s) && !pPreProc->nCMode)
			{
				do s++; while(*s && g_oInterface.IsSpace(*s));
				*--s = ' ';
			}
			*op++ = *s++;
		}
	}
	*op = '\0';
	if (pPreProc->nCMode)
	{
		if (pPreProc->nCMode == CMstr)
			PutError(pPreProc, "missing end \" in macro token sequence");
		else
			PutError(pPreProc, "missing end ' in macro token sequence");
	}
	else MergeTokens(pPreProc, &pPreProc->oDefMacro);
}

static jc_void MergeTokens(CJcPreProc* pPreProc, JcMacro *pMacro)
{
	jc_char * s, *seq;
	jc_int left, right;

	if(pMacro->nParaNum)
		pMacro->sProtect = (jc_char*)g_oInterface.Malloc(pMacro->nParaNum + 1);

	s = seq  = pMacro->sTokenSeq;
	while(*s)
	{
		if(!pPreProc->nCMode && *s == '#' && *(s+1) == '#')
		{
			jc_int d = (jc_int)(s - seq) - 1;
			while(d >= 0 && seq[d] > 0 && g_oInterface.IsSpace(seq[d]))
				d--;
			if(d < 0)
				PutError(pPreProc, "macro definition begins with ##");
			left = d;
			d = (jc_int)(s - seq) + 2;
			while(seq[d] > 0 && g_oInterface.IsSpace(seq[d]))
				d++;
			if(!seq[d])
				PutError(pPreProc, "macro definition ends with ##");
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

static jc_int FindParm(CJcPreProc* pPreProc, CJcVector *parms,jc_char *name, jc_uint len)
{
	jc_int i, nCount = parms->nSize;
	for(i=0; i<nCount; ++i)
	{
		jc_char* v = (*parms).pTable[i];
		if(!StringCompareN(v, name, len))
			return i+1;
	}
	return 0;
}

static jc_void ExprList(CJcPreProc* pPreProc, CJcVector *parms, jc_char **p,jc_int more)
{
	jc_int c = 0, b = 0;
	jc_char *s = *p;

	if(!pPreProc->pStr)
		pPreProc->pStr = (jc_char*)g_oInterface.Malloc(BUFSIZE);
	while (1)
	{
		for (; *s; ++s)
		{
			if(g_oInterface.IsSpace(*s) && !pPreProc->nCMode)
			{
				while(*s && g_oInterface.IsSpace(*s)) s++;
				if(!*s)
					break;
				s--;
			}
			if(c == (jc_int)pPreProc->nSZ)
			{
				pPreProc->nSZ += BUFSIZE;
				pPreProc->pStr = (jc_char*)g_oInterface.Realloc(pPreProc->pStr,pPreProc->nSZ);
			}
			pPreProc->pStr[c++] = *s;
			switch (*s)
			{
			case '\\':
				pPreProc->pStr[c++] = *++s; /*get next jc_char */
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
			while(pPreProc->pStr[c] && g_oInterface.IsSpace(pPreProc->pStr[c]))
				c++;
			Push(parms, StringDuplicate(pPreProc->pStr + c));
		}
		if(!*s && more)
		{ /*need more input*/
			GetPreLine(pPreProc);
			if(c && !g_oInterface.IsSpace(pPreProc->pStr[c-1]) && !g_oInterface.IsSpace(*pPreProc->sPreLine))
				pPreProc->pStr[c++] = ' ';  /* need white space */
			s = pPreProc->sPreLine;
			continue;
		}
		if (*s == ')')
			break;
		if (*s != ',')
		{
			PutError(pPreProc, "illegal macro definition");
			break;
		}
		c = 0;
		s++;
	} /* end while */
	*p = ++s;
	if(pPreProc->nSZ > (BUFSIZE << 2))
	{
		pPreProc->nSZ = BUFSIZE;
		pPreProc->pStr = (jc_char*)g_oInterface.Realloc(pPreProc->pStr,pPreProc->nSZ);
	}
}

static jc_void DoPragma(CJcPreProc* pPreProc, jc_char *s)
{
}

static jc_void ReplaceDefines(CJcPreProc* pPreProc, jc_char *S)
{
	jc_char str[50];
	jc_int cmode = 0;
	jc_char *p;
	p = S;
	while(*S != '\0')
	{
		if(!cmode && (g_oInterface.IsAlpha(*S) || *S == '_'))
		{
			if(S[0] == 'd' && S[1] == 'e' &&
				S[2] == 'f' && S[3] == 'i' &&
				S[4] == 'n' && S[5] == 'e' &&
				S[6] == 'd' && !g_oInterface.IsAlpha(S[7]) &&
				S[7] != '_')
			{
				jc_int br = 0;
				jc_int i;
				S+=7;
				ForwardSkipWhiteSpace(S);
				if(*S=='(')
				{
					S++;
					br = 1;
					ForwardSkipWhiteSpace(S);
				}
				i = 0;
				while(i < 50 && (g_oInterface.IsAlpha(*S) || *S == '_' || g_oInterface.IsDigit(*S)))
					str[i++] = *S++;
				str[i] = '\0';
				if(br)
				{
					ForwardSkipWhiteSpace(S);
					if(*S != ')')
						PutError(pPreProc, "missing ')'");
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
					PutError(pPreProc, "missing identifier");
				continue;
			}
			do
			*p++ = *S++;
			while(g_oInterface.IsAlpha(*S) || *S == '_' );
			continue;
		} if(*S == '\'')
			cmode = !cmode;
		*p++ = *S++;
	}
	*p = '\0';
}

static jc_void ReplaceIdentifiers(CJcPreProc* pPreProc, jc_char *S)
{
	jc_int i;
	jc_char str[50];
	jc_char *p;
	jc_int cmode = 0;

	p = S;

	while(1)
	{
		while(g_oInterface.IsSpace(*S))
			*p++ = *S++;
		if(!*S)
			return;
		if(!g_oInterface.IsAlpha(*S) && *S != '_')
		{
			if(g_oInterface.IsDigit(*S) || *S == '\'')
			{ /* skip throu numbers or literals*/
				while(*S && !g_oInterface.IsSpace(*S))
					*p++ = *S++;
			}
			else while(*S && !g_oInterface.IsSpace(*S) && *S != '_' && g_oInterface.IsPunct(*S))
				*p++ = *S++;
			continue;
		}
		if(!cmode)
		{
			i = 0;
			while(i < 50 && (g_oInterface.IsAlpha(*S) || *S == '_' || g_oInterface.IsDigit(*S)) )
				str[i++] = *S++;
			str[i] = '\0';
			if(StringCompare(str,"sizeof") == 0)
				PutError(pPreProc, "illegal sizeof operator");
			else
				*p++ = '0';
		}
		else
			*p++ = *S++;
	}
}

static jc_int CppParse(CJcPreProc* pPreProc, jc_char *s)
{
	JcPreProcInt res;
	pPreProc->pS = s;
	ReplaceDefines(pPreProc, pPreProc->pS);
	pPreProc->pS = s  = Process2(pPreProc, pPreProc->pS, 0, 0);
	ReplaceIdentifiers(pPreProc, pPreProc->pS);
	res = IfExpr(pPreProc, 0);
	if(s)
		g_oInterface.Free(s);
	return (uval(res))?1:0;
}

static JcPreProcInt IfExpr(CJcPreProc* pPreProc, jc_int k)
{
	JcPreProcInt res;
	jc_int k1, tk;
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

static JcPreProcInt DoBinary(CJcPreProc* pPreProc, jc_int tk, JcPreProcInt left, JcPreProcInt right)
{
	JcPreProcInt r;
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
			PutError(pPreProc, "divide or mod by zero");
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
			PutError(pPreProc, "divide or mod by zero");
			sval(r) = 0;
		}
		break;
	}
	return r;
}

static jc_int GetTok(CJcPreProc* pPreProc, jc_int k)
{
	jc_int TK = 0;
	while(g_oInterface.IsSpace(*pPreProc->pS))
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

static jc_int GetOct(jc_int x)
{
	return x>='0'&&x<='7'? x-'0':-1;
}

static jc_int GetHex(jc_int x)
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

static jc_int GetDec(jc_int x)
{
	return x >= '0' && x <= '9' ? x-'0':-1;
}

static JcPreProcInt GetNumber(CJcPreProc* pPreProc)   /* collect hex, octal and decimal integers */
{
	jc_int (*f)(jc_int x);
	jc_int radix,val;

	JcPreProcInt res = {{0},SIGN};

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

	if(uval(res) > MAX_JC_LONG)
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

static jc_int GetCharConst(CJcPreProc* pPreProc)
{
	jc_int c;
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
			jc_int i, val = 0;
			pPreProc->pS++;
			while ((i = GetHex(*pPreProc->pS)) > -1)
			{
				pPreProc->pS++;
				val = val * 16 + i;
			}
			if (val > 255)
				PutError(pPreProc, "illegal character hex value");
			c = val;
		}
		break;
	default:
		if (g_oInterface.IsDigit(*pPreProc->pS))
		{	/* treat as octal characters */
			jc_int i, val = 0;
			while ((i = GetOct(*pPreProc->pS)) > -1)
			{
				val = val * 8 + i;
				pPreProc->pS++;
			}
			if (val > 255)
				PutError(pPreProc, "illegal character octal value");
			c = val;
		}
		else
		{
			jc_char sDebug[64];
			g_oInterface.FormatPrint(sDebug, "Illegal character escape sequence `\\%c'", *pPreProc->pS);
			PutError(pPreProc, sDebug);
			c = *pPreProc->pS++;
		}
		break;
	}
	return c;
}

static JcPreProcInt ExprUnary(CJcPreProc* pPreProc)
{
	JcPreProcInt res;
	while(g_oInterface.IsSpace(*pPreProc->pS))
		pPreProc->pS++;
	if(g_oInterface.IsDigit(*pPreProc->pS))
	{
		res = GetNumber(pPreProc);
	}
	else if( *pPreProc->pS == '(')
	{
		pPreProc->pS++;
		res = IfExpr(pPreProc, 0);
		if(*pPreProc->pS != ')')
			PutError(pPreProc, "unbalanced parenthesis");
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
			PutError(pPreProc, "-- not allowed in operand of #if");
		res = ExprUnary(pPreProc);
		tval(res) = SIGN;
		sval(res) = -(long)(uval(res));
	}
	else if(*pPreProc->pS == '+')
	{
		pPreProc->pS++;
		if(*pPreProc->pS == '+')
			PutError(pPreProc, "++ not allowed in operand of #if");
		res = ExprUnary(pPreProc);
	}
	else if(*pPreProc->pS == '~')
	{
		pPreProc->pS++;
		res = ExprUnary(pPreProc);
		uval(res) = ~uval(res);
	}
	else if(*pPreProc->pS == '\'')
	{ /* jc_char constants */
		pPreProc->pS++;
		if(*pPreProc->pS == '\\')
		{
			pPreProc->pS++;
			uval(res) = GetCharConst(pPreProc);
		}
		else
			uval(res) = *pPreProc->pS++;
		if(*pPreProc->pS != '\'')
			PutError(pPreProc, "missing closing single quote '");
		else
			pPreProc->pS++;
		tval(res) = SIGN;
	}
	else
		PutError(pPreProc, "illegal constant expression");
	return res;
}

jc_int InitializePreProcessor(CJcPreProc* pPreProc, CJcErrorSystem* pErrorSystem, jc_char* sFileName)
{
	pPreProc->pErrorSystem = pErrorSystem;
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

	DefineMacro(pPreProc, "bool=unsigned char");
	DefineMacro(pPreProc, "true=1");
	DefineMacro(pPreProc, "false=0");

	return 0;
}

jc_void ClearPreProcessor(CJcPreProc* pPreProc)
{
	if(pPreProc->sPreLine)
		g_oInterface.Free(pPreProc->sPreLine);
	if(pPreProc->sProLine)
		g_oInterface.Free(pPreProc->sProLine);
	if(pPreProc->pStr)
		g_oInterface.Free(pPreProc->pStr);
	ClearVector(&pPreProc->oMacros, ClearMacro);
	ClearFileTable(&pPreProc->oFileTable);
	ClearStringTable(&pPreProc->oStringTab);
}

jc_void InitializePreProcessorCmdLine(CJcPreProc* pPreProc, jc_int nArgc, jc_char* sArgv[])
{
	jc_int i;
	jc_char * sRet;
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
}

CJcCodeLine* GetProcLine(CJcPreProc* pPreProc)
{
	if(pPreProc->sPreLine)
		g_oInterface.Free(pPreProc->sPreLine);
	if(pPreProc->sProLine)
		g_oInterface.Free(pPreProc->sProLine);
	pPreProc->sPreLine = (jc_char *) g_oInterface.Malloc(REBUFF_INCREMENT);
	pPreProc->sProLine = (jc_char *) g_oInterface.Malloc(REBUFF_INCREMENT);
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
