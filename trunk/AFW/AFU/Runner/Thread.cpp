
#include "BaseDef.hpp"

bool CreateThread(FThreadProc ThreadProc, void* pContext, unsigned int nStackSize)
{
#ifdef WINDOWS
	DWORD nThreadId;
	HANDLE hThread = (void*)CreateThread(NULL, nStackSize, (LPTHREAD_START_ROUTINE)ThreadProc, pContext, 0, &nThreadId);
	if(hThread == NULL)
		return false;
	CloseHandle(hThread);
#else
	pthread_t nThreadId;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(nStackSize == 0)
		nStackSize = 1048576;
	pthread_attr_setstacksize(&attr, nStackSize);
	if(0 != pthread_create(&nThreadId, &attr, ThreadProc, pContext))
	{
		pthread_attr_destroy(&attr);
		return false;
	}
	pthread_attr_destroy(&attr);
#endif
	return true;
}

#define NULCHAR    '\0'
#define SPACECHAR  ' '
#define TABCHAR    '\t'
#define DQUOTECHAR '\"'
#define SLASHCHAR  '\\'

void ParseCmdline(const char* sCmdLine, char **argv, char *args, int *pArgc, int *nChars)
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
			
		} while ( c != SPACECHAR && c != NULCHAR && c != TABCHAR );
		
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

char** GetArgv(const char* sCmdLine)
{
	int nArgc, nChars;
	ParseCmdline(sCmdLine, NULL, NULL, &nArgc, &nChars);
	char** sArgv = (char**)malloc(nArgc * sizeof(char *) + nChars * sizeof(char));
	ParseCmdline(sCmdLine, sArgv, (char*)sArgv + nArgc*sizeof(char*), &nArgc, &nChars);
	return sArgv;
}

void MoveArgv(int argc, char* argv[], int nRemove)
{
	argc -= nRemove;
	memmove(argv, argv+nRemove, argc*sizeof(void*));
	argv[argc] = NULL;
}

unsigned short GetDefaultTcpServicePort()
{
	unsigned short nPort = FOCP_RUNNER_TCPSERVICEPORT;
	const char* s = (const char*)getenv("FOCP_RUNNER_TCPSERVICEPORT");
	if(s)
	{
		int d;
		if(sscanf(s, "%d", &d) == 1)
		{
			if(d > 0 && d < 65536)
				nPort = (unsigned short)d;
		}
	}
	return nPort;
}

unsigned short GetDefaultUdpServicePort()
{
	unsigned short nPort = FOCP_RUNNER_UDPSERVICEPORT;
	const char* s = (const char*)getenv("FOCP_RUNNER_UDPSERVICEPORT");
	if(s)
	{
		int d;
		if(sscanf(s, "%d", &d) == 1)
		{
			if(d > 0 && d < 65536)
				nPort = (unsigned short)d;
		}
	}
	return nPort;
}

bool TcpSend(SOCKET nSock, char *pBuf, int nBufLen)
{
	while(nBufLen)
	{
		int nRet = send(nSock, pBuf, nBufLen, 0);
		if(nRet <= 0)
			return false;
		nBufLen -= nRet;
		pBuf += nRet;
	}
	return true;
}

bool TcpRecv(SOCKET nSock, char *pBuf, int nBufLen)
{
	while(nBufLen)
	{
		int nRet = recv(nSock, pBuf, nBufLen, 0);
		if(nRet <= 0)
			return false;
		nBufLen -= nRet;
		pBuf += nRet;
	}
	return true;
}

void ShutDown(SOCKET nSock)
{
#ifdef WINDOWS
	shutdown(nSock,SD_BOTH);
#else
	shutdown(nSock,SHUT_RDWR);
#endif
}

void GetCmdLine(int argc, char*argv[], char* sCmdLine)
{
	int i;
	sCmdLine[0] = 0;
	if(strchr(argv[0], ' ') || strchr(argv[0], '\t'))
	{
		strcat(sCmdLine, "\"");
		strcat(sCmdLine, argv[0]);
		strcat(sCmdLine, "\"");
	}
	else
		strcat(sCmdLine, argv[0]);
	char* s = sCmdLine + strlen(sCmdLine);
	for(i=1; i<argc; ++i)
	{
		s[0] = ' ';
		++s;
		char* arg = argv[i];
		bool bHaveSpace = (strchr(argv[0], ' ') || strchr(argv[0], '\t'));
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
