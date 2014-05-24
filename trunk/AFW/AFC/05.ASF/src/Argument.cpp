
#include "Argument.hpp"

FOCP_BEGIN();

CArguments::CArguments()
{
	m_nArgc = 0;
	m_sArgv = NULL;
	m_sCmdLine[0] = 0;
	m_bOwnArgv = false;
}

CArguments::~CArguments()
{
	if(m_bOwnArgv && m_sArgv)
		CMalloc::Free(m_sArgv);
	m_sArgv = NULL;
}

CArguments* CArguments::GetInstance()
{
	return CSingleInstance<CArguments>::GetInstance();
}

//95511-2-9 sos
void CArguments::SetArgv(int32 argc, char*argv[], bool bIgnoreArgv0)
{
	int32 i;
	m_nArgc = argc;
	m_sArgv = (char**)argv;
	m_sCmdLine[0] = 0;
	if(bIgnoreArgv0)
	{
		const char* sAppPath = CFilePathInfo::GetInstance()->GetPath();
		argv[0] = (char*)sAppPath;
	}
	if(CString::CharOfString(argv[0], ' ') || CString::CharOfString(argv[0], '\t'))
	{
		CString::StringCatenate(m_sCmdLine, "\"");
		CString::StringCatenate(m_sCmdLine, argv[0]);
		CString::StringCatenate(m_sCmdLine, "\"");
	}
	else
		CString::StringCatenate(m_sCmdLine, argv[0]);
	char* s = m_sCmdLine + CString::StringLength(m_sCmdLine);
	for(i=1; i<m_nArgc; ++i)
	{
		s[0] = ' ';
		++s;
		char* arg = argv[i];
		bool bHaveSpace = (CString::CharOfString(argv[0], ' ') || CString::CharOfString(argv[0], '\t'));
		if(bHaveSpace)
		{
			s[0] = '\"';
			++s;
		}
		while(arg[0])
		{
			switch(arg[0])
			{
			case '\"':
			case '\\':
			case ':':
			case '|':
			case '>':
			case '<':
				s[0] = '\\';
				++s;
				s[0] = arg[0];
				++s;
				break;
			default:
				s[0] = arg[0];
				++s;
				break;
			}
			++arg;
		}
		if(bHaveSpace)
		{
			s[0] = '\"';
			++s;
		}
	}
	s[0] = '\0';
}

int32 CArguments::GetArgc()
{
	return m_nArgc;
}

char* const* CArguments::GetArgv()
{
	return (char* const*)m_sArgv;
}

const char* CArguments::GetCmdLine()
{
	return m_sCmdLine;
}

int32 CArguments::GetOption(char c, char** pArg)
{
	for(int32 i=1; i<m_nArgc; ++i)
	{
		char* s = m_sArgv[i];
		if(s[0] == '-' && s[1]==c)
		{
			if(pArg)
			{
				if(s[2])
					*pArg = s+2;
				else
					*pArg = NULL;
			}
			return i;
		}
	}
	return 0;
}

char CArguments::WalkOption(int32 &nIdx, char* &pArg)
{
	if(nIdx == 0)
		nIdx = 1;
	for(; nIdx<m_nArgc; ++nIdx)
	{
		char* s = m_sArgv[nIdx];
		if(s[0] == '-' && s[1])
		{
			if(s[2])
				pArg = s+2;
			else
				pArg = NULL;
			++nIdx;
			return s[1];
		}
	}
	return '\0';
}

#define NULCHAR    '\0'
#define SPACECHAR  ' '
#define TABCHAR    '\t'
#define DQUOTECHAR '\"'
#define SLASHCHAR  '\\'

void CArguments::ParseCmdline(const char* sCmdLine, char **argv, char *args, int32 *pArgc, int32 *nChars)
{
	const char *p;
	unsigned char c;
	int inquote;                    /* 1 = inside quotes */
	int copychar;                   /* 1 = copy char to *args */
	unsigned numslash;              /* num of backslashes seen */
	int flag = 0;

	*nChars = 0;
	*pArgc = 1;                   /* the program name at least */

	/* first scan the program name, copy it if the char is not blank, and count the bytes */
	while(sCmdLine[flag] == SPACECHAR || sCmdLine[flag] == TABCHAR)
		flag ++;
	p = sCmdLine + flag;

	if (argv)
		*argv++ = args;//program name

	/*
	A quoted program name is handled here. The handling is much
	simpler than for other arguments. Basically, whatever lies
	between the leading double-quote and next one, or a terminal null
	character is simply accepted. Fancier handling is not required
	because the program name must be a legal NTFS/HPFS file name.
	Note that the double-quote characters are not copied, nor do they
	contribute to nChars.
	*/
	if ( *p == DQUOTECHAR )
	{
		/*
		scan from just past the first double-quote through the next
		double-quote, or up to a null, whichever comes first
		*/
		while ( (*(++p) != DQUOTECHAR) && (*p != NULCHAR) )
		{
			++*nChars;
			if(args)
				*args++ = *p;
		}
		/* append the terminating null */
		++*nChars;
		if ( args )
			*args++ = NULCHAR;

		/* if we stopped on a double-quote (usual case), skip over it */
		if ( *p == DQUOTECHAR )
			p++;
	}
	else
	{
		/* Not a quoted program name */
		do
		{
			++*nChars;
			if (args)
				*args++ = *p;
			c = (unsigned char) *p++;

		}
		while ( c != SPACECHAR && c != NULCHAR && c != TABCHAR );

		if ( c == NULCHAR )
		{
			p--;
		}
		else
		{
			if (args)
				*(args-1) = NULCHAR;
		}
	}

	inquote = 0;

	/* loop on each argument */
	for(;;)
	{
		if ( *p )
		{
			while (*p == SPACECHAR || *p == TABCHAR)
				++p;
		}

		if (*p == NULCHAR)
			break;              /* end of args */

		/* scan an argument */
		if (argv)
			*argv++ = args;     /* store ptr to arg */
		++*pArgc;

		/* loop through scanning one argument */
		for (;;)
		{
			copychar = 1;
			/* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
			2N+1 backslashes + " ==> N backslashes + literal "
			N backslashes ==> N backslashes */
			numslash = 0;
			while (*p == SLASHCHAR)
			{
				/* count number of backslashes for use below */
				++p;
				++numslash;
			}
			if(*p == DQUOTECHAR)
			{
				/* if 2N backslashes before, start/end quote, otherwise copy literally */
				if (numslash % 2 == 0)
				{
					if (inquote)
					{
						if (p[1] == DQUOTECHAR)
							p++; /* Double quote inside quoted string */
						else /* skip first quote char and copy second */
							copychar = 0;
					}
					else
						copychar = 0;       /* don't copy quote */
					inquote = !inquote;
				}
				numslash /= 2;          /* divide numslash by two */
			}
			/* copy slashes */
			while (numslash--)
			{
				if (args)
					*args++ = SLASHCHAR;
				++*nChars;
			}

			/* if at end of arg, break loop */
			if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
				break;

			/* copy character into argument */
			if (copychar)
			{
				if (args)
					*args++ = *p;
				++*nChars;
			}
			++p;
		}

		/* null-terminate the argument */

		if (args)
			*args++ = NULCHAR;          /* terminate string */
		++*nChars;
	}

	/* We put one last argument in -- a null ptr */
	if (argv)
		*argv++ = NULL;
	++*pArgc;
}

void CArguments::SetCmdLine(const char* sCmdLine)
{
	int32 nArgc, nChars;
	ParseCmdline(sCmdLine, NULL, NULL, &nArgc, &nChars);
	m_sArgv = (char**)CMalloc::Malloc(nArgc * sizeof(char *) + nChars * sizeof(char));
	ParseCmdline(sCmdLine, m_sArgv, (char*)m_sArgv + nArgc*sizeof(char*), &nArgc, &nChars);
	m_nArgc = nArgc - 1;
	m_bOwnArgv = true;
	m_sCmdLine[0] = 0;
	CString::StringCatenate(m_sCmdLine, sCmdLine);
}

FOCP_END();
