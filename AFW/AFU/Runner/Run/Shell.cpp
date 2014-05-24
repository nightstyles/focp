
#include "../BaseDef.hpp"

class CThreadEvent
{
private:
	void* m_pEvent;

	CThreadEvent(const CThreadEvent&);
	CThreadEvent& operator=(const CThreadEvent&);
public:
	CThreadEvent(bool bAutoReset=false)
	{
#ifdef WINDOWS
		m_pEvent = CreateEvent(NULL, bAutoReset?FALSE:TRUE, FALSE, NULL);
#endif

#ifdef UNIX
		pthread_event_t * pEvent = new pthread_event_t;
		pEvent->bmanual = bAutoReset?0:1;
		pEvent->bvalue = 0;
		pthread_mutex_init(&(pEvent->mutex), NULL);
		pthread_cond_init(&(pEvent->cond), NULL);
		m_pEvent = pEvent;
#endif
	}

	~CThreadEvent()
	{
#ifdef WINDOWS
		CloseHandle((HANDLE)m_pEvent);
		m_pEvent = NULL;
#endif

#ifdef UNIX
		pthread_event_t * pEvent = (pthread_event_t*)m_pEvent;
		pthread_cond_destroy(&(pEvent->cond));
		pthread_mutex_destroy(&(pEvent->mutex));
		delete pEvent;
		m_pEvent = NULL;
#endif
	}

	void Set()
	{
#ifdef WINDOWS
		SetEvent((HANDLE)m_pEvent);
#endif

#ifdef UNIX
		pthread_event_t * pEvent = (pthread_event_t*)m_pEvent;
		if(!pEvent->bvalue)
		{
			pthread_mutex_lock(&(pEvent->mutex));
			if(!pEvent->bvalue)
			{
				pEvent->bvalue = 1;
				pthread_cond_signal(&(pEvent->cond));
			}
			pthread_mutex_unlock(&(pEvent->mutex));
		}
#endif
	}

	void Reset()
	{
#ifdef WINDOWS
		ResetEvent((HANDLE)m_pEvent);
#endif

#ifdef UNIX
		pthread_event_t * pEvent = (pthread_event_t*)m_pEvent;
		if(pEvent->bvalue)
		{
			pthread_mutex_lock(&(pEvent->mutex));
			if(pEvent->bvalue)
			{
				timespec tt;
				tt.tv_sec = time(NULL);
				tt.tv_nsec = 0;
				pthread_cond_timedwait(&(pEvent->cond), &(pEvent->mutex), &tt);
				pEvent->bvalue = 0;
			}
			pthread_mutex_unlock(&(pEvent->mutex));
		}
#endif
	}

	bool Wait(unsigned int nTimeOut=0xFFFFFFFF)
	{
#ifdef WINDOWS
		return WAIT_OBJECT_0 == WaitForSingleObject((HANDLE)m_pEvent, nTimeOut);
#endif

#ifdef UNIX
		timespec tt;
		unsigned int nRet;
		pthread_event_t * pEvent;

		if(nTimeOut != (unsigned int)(-1))
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			tt.tv_sec = tv.tv_sec;
			tt.tv_nsec = tv.tv_usec*1000;
			tt.tv_sec += nTimeOut/1000;
			tt.tv_nsec += (nTimeOut%1000)*1000000;
			if(tt.tv_nsec >= 1000000000)
			{
				tt.tv_nsec -= 1000000000;
				tt.tv_sec++;
			}
		}

		pEvent = (pthread_event_t*)m_pEvent;
		pthread_mutex_lock(&(pEvent->mutex));

		if(pEvent->bvalue)
		{
			if(pEvent->bmanual == 0)
				pEvent->bvalue = 0;
			pthread_mutex_unlock(&(pEvent->mutex));
			return true;
		}

		nRet = 0;
		while(pEvent->bvalue == 0)
		{
			if(nTimeOut == (unsigned int)(-1))
				nRet = pthread_cond_wait(&(pEvent->cond), &(pEvent->mutex));
			else
				nRet = pthread_cond_timedwait(&(pEvent->cond), &(pEvent->mutex), &tt);
			if(nRet && errno != EINTR)
				break;
		}
		if(pEvent->bvalue)
		{
			nRet = 0;
			if(pEvent->bmanual == 0)
				pEvent->bvalue = 0;
			else
				pthread_cond_signal(&(pEvent->cond));
		}
		pthread_mutex_unlock(&(pEvent->mutex));

		return (nRet == 0);
#endif
	}
};

//Run
//Run -b1971 [-r] x.exe
//Run -a1971 [-r] x.exe
//Run -a1971
static unsigned short g_nVirtualPort = 0;
static unsigned short g_nBindPort = 0;
static unsigned short g_nTcpServicePort = FOCP_RUNNER_TCPSERVICEPORT;
static unsigned short g_nUdpServicePort = FOCP_RUNNER_UDPSERVICEPORT;
static bool g_bNeedConnect = false;
static bool g_bLoop;
static int g_nAppIdx = 0;
static int g_nPortIdx = 0;
static SOCKET g_nUdpSocket = INVALID_SOCKET;
static SOCKET g_nTcpSocket = INVALID_SOCKET;
static FILE* StdInPipe[2];

static char sStdInBuf[1024];

static CThreadEvent g_oReadEvent(true);

void InitializeStdInPipe()
{
	int fd[2];
#ifdef WINDOWS
	_pipe(fd, 512, _O_TEXT);
#else
	pipe(fd);
#endif
	StdInPipe[0] = fdopen(fd[0], "rt");
	StdInPipe[1] = fdopen(fd[1], "wt");
	setvbuf(StdInPipe[1], NULL, _IONBF, 0);
	setvbuf(StdInPipe[0], sStdInBuf, _IOLBF, 1024);
}

void BreakStdInPipe()
{
	if(StdInPipe[1])
	{
		fclose(StdInPipe[1]);
		StdInPipe[1] = NULL;
	}
}

void ResetStdInPipe()
{
	BreakStdInPipe();
	fclose(StdInPipe[0]);
	InitializeStdInPipe();
}

extern "C" CThreadReturn FOCP_CALL StdInRecvThread(void*)
{
	char sBuf[1024];
	while(true)
	{
		g_oReadEvent.Wait(0xFFFFFFFF);
		gets(sBuf);
		if(sBuf[0] == '\0')
		{
			if(g_nTcpSocket == INVALID_SOCKET)
			{
				printf("#");
				fflush(stdout);
			}
			g_oReadEvent.Set();
			continue;
		}
		strcat(sBuf, "\r\n");
		if(StdInPipe[1])
			fprintf(StdInPipe[1], "%s", sBuf);
	}
	return (CThreadReturn)0;
}

extern "C" CThreadReturn FOCP_CALL SocketRecvThread(void*)
{
	char pBuf[1024];

	while(true)
	{
		if(!TcpRecv(g_nTcpSocket, (char*)pBuf, 4))
		{
			if(g_nTcpSocket != INVALID_SOCKET)
			{
				closesocket(g_nTcpSocket);
				g_nTcpSocket = INVALID_SOCKET;
			}
			break;
		}
		unsigned short nMode = ((unsigned short*)pBuf)[0];
		if(nMode != 3 && nMode != 4)//StdOut/StdErr Msg
		{
			if(g_nTcpSocket != INVALID_SOCKET)
			{
				closesocket(g_nTcpSocket);
				g_nTcpSocket = INVALID_SOCKET;
			}
			break;
		}
		unsigned short nSize = ((unsigned short*)pBuf)[1];
		if(nSize == 0 || nSize > 1024)//invalid StdIn Msg Len
		{
			if(g_nTcpSocket != INVALID_SOCKET)
			{
				closesocket(g_nTcpSocket);
				g_nTcpSocket = INVALID_SOCKET;
			}
			break;
		}
		if(!TcpRecv(g_nTcpSocket, (char*)pBuf, (int)nSize))
		{
			if(g_nTcpSocket != INVALID_SOCKET)
			{
				closesocket(g_nTcpSocket);
				g_nTcpSocket = INVALID_SOCKET;
			}
			break;
		}
		if(nMode == 3)
			write(1, pBuf, nSize);
		else
			write(2, pBuf, nSize);
	}
	return (CThreadReturn)0;
}

bool ApplyVirtualConnect()
{
	unsigned short pBuf[2];
	pBuf[0] = 5;//ApplyVirtualConnect
	pBuf[1] = g_nVirtualPort;
	if(!TcpSend(g_nTcpSocket, (char*)pBuf, 4))
		return false;
	if(!TcpRecv(g_nTcpSocket, (char*)pBuf, 4))
		return false;
	if(pBuf[0] != 6)//ApplyVirtualConnect Response
		return false;
	if(pBuf[1] != 1)//ApplyVirtualConnect failure
		return false;
	return true;
}

void sig_fatal (int sig)
{
	signal(sig, sig_fatal);
	if(g_nTcpSocket != INVALID_SOCKET)
	{
		closesocket(g_nTcpSocket);
		g_nTcpSocket = NULL;
		BreakStdInPipe();
	}
	else
		_exit(0);
}

void ListApplication()
{
	g_nTcpServicePort = GetDefaultTcpServicePort();
	g_nTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in oToAddr;
	memset(&oToAddr, 0, sizeof(oToAddr));
	oToAddr.sin_family = AF_INET;
	oToAddr.sin_port = htons(g_nTcpServicePort);
	oToAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if(connect(g_nTcpSocket, (const struct sockaddr*)(void*)&oToAddr, sizeof(oToAddr)))
	{
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
		return;
	}
	unsigned short nCmd[2];
	nCmd[0] = 5;//ApplyVirtualConnect
	nCmd[1] = g_nVirtualPort;
	if(!TcpSend(g_nTcpSocket, (char*)nCmd, 4))
	{
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
		return;
	}
	while(true)
	{
		char pBuf[1024];
		int nRet = recv(g_nTcpSocket, pBuf, 1023, 0);
		if(nRet<=0)
			break;
		pBuf[nRet] = 0;
		printf("%s", pBuf);
	}
	ShutDown(g_nTcpSocket);
	closesocket(g_nTcpSocket);
	g_nTcpSocket = INVALID_SOCKET;
}

void StartApplication(int argc, char* argv[])
{
	int i, d;

	for(i=1; i<argc; ++i)
	{
		if(!strcmp(argv[i], "-r"))
			g_bLoop = true;
		else if(!strncmp(argv[i], "-a", 2) && !g_nVirtualPort)
		{
			if(sscanf(argv[i]+2, "%d", &d) == 1 && d>=0 && d<65536)
			{
				g_nVirtualPort = (unsigned short)d;
				g_bNeedConnect = true;
				g_nPortIdx = i;
			}
		}
		else if(!strncmp(argv[i], "-b", 2) && !g_nVirtualPort)
		{
			if(sscanf(argv[i]+2, "%d", &d) == 1 && d>=0 && d<65536)
			{
				g_nVirtualPort = (unsigned short)d;
				g_bNeedConnect = false;
				g_nPortIdx = i;
			}
		}
		else
		{
			g_nAppIdx = i;
			break;
		}
	}

	if(g_nVirtualPort == 0 || g_nAppIdx == 0)
		return;

	g_nUdpServicePort = GetDefaultUdpServicePort();
	g_nUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(g_bNeedConnect)
	{
		sockaddr_in oBindAddr;
		memset(&oBindAddr, 0, sizeof(oBindAddr));
		oBindAddr.sin_family = AF_INET;
		oBindAddr.sin_port = 0;
		oBindAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		if(bind(g_nUdpSocket, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr)))
		{
			closesocket(g_nUdpSocket);
			g_nUdpSocket = INVALID_SOCKET;
			return;
		}
		socklen_t nLen = sizeof(oBindAddr);
		getsockname(g_nUdpSocket, (struct sockaddr*)(void*)&oBindAddr, &nLen);
		g_nBindPort = htons(oBindAddr.sin_port);
	}
	//组织发送创建子进程的消息。
	sockaddr_in oToAddr;
	memset(&oToAddr, 0, sizeof(oToAddr));
	oToAddr.sin_family = AF_INET;
	oToAddr.sin_port = htons(g_nUdpServicePort);
	oToAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	char* pMsg= new char[4096];
	char* pShift = pMsg;
	int nArgc = argc - g_nAppIdx + 2;//argv[0], -v
#ifdef WINDOWS
	++nArgc;//-h
#endif
	if(g_bLoop)
		++nArgc;
	if(g_bNeedConnect)
		++nArgc;
	memcpy(pShift, &nArgc, sizeof(nArgc));
	pShift += sizeof(nArgc);
	strcpy(pShift, argv[0]);
	if(strstr(argv[0], ".exe"))
		pShift += strlen(pShift) - 7;
	else
		pShift += strlen(pShift) - 3;
#ifdef WINDOWS
	strcpy(pShift, "RunShell.exe");
#else
	strcpy(pShift, "RunShell");
#endif
	pShift += strlen(pShift)+1;
	if(g_bLoop)
	{
		strcpy(pShift, "-r");
		pShift += strlen(pShift)+1;
	}
	if(g_bNeedConnect)
	{
		strcpy(pShift, "-c");
		pShift += strlen(pShift);
		sprintf(pShift, "%d", (int)g_nBindPort);
		pShift += strlen(pShift)+1;
	}
	strcpy(pShift, argv[g_nPortIdx]);
	pShift[1] = 'v';
	pShift += strlen(pShift)+1;
#ifdef WINDOWS
	strcpy(pShift, "-h");
	pShift += strlen(pShift)+1;
#endif
	for(i=g_nAppIdx; i<argc; ++i)
	{
		strcpy(pShift, argv[i]);
		pShift += strlen(pShift)+1;
	}
	if(pShift-pMsg > 4096)
	{
		closesocket(g_nUdpSocket);
		g_nUdpSocket = INVALID_SOCKET;
		return;
	}
	sendto(g_nUdpSocket, pMsg, pShift-pMsg, 0, (const struct sockaddr*)(void*)&oToAddr, sizeof(oToAddr));
	delete[] pMsg;
	char bMaybe;
	if(g_bNeedConnect)
	{
		sockaddr_in oFrom;
		socklen_t nLen = sizeof(oFrom);

		fd_set rset;
		struct timeval tm;
		FD_ZERO(&rset);
		FD_SET(g_nUdpSocket, &rset);
		int maxid;
		maxid = g_nUdpSocket+1;
		tm.tv_sec = 5;
		tm.tv_usec = 0;
		if(!select(maxid, &rset, NULL, NULL, &tm))
		{
			closesocket(g_nUdpSocket);
			g_nUdpSocket = INVALID_SOCKET;
			return;
		}

		if(recvfrom(g_nUdpSocket, &bMaybe, 1, 0, (struct sockaddr*)(void*)&oFrom, &nLen) != 1 || bMaybe!='1')
		{
			closesocket(g_nUdpSocket);
			g_nUdpSocket = INVALID_SOCKET;
			return;
		}

	}
	closesocket(g_nUdpSocket);
	g_nUdpSocket = INVALID_SOCKET;
	if(g_bNeedConnect)
	{
		g_nTcpServicePort = GetDefaultTcpServicePort();
		g_nTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in oToAddr;
		memset(&oToAddr, 0, sizeof(oToAddr));
		oToAddr.sin_family = AF_INET;
		oToAddr.sin_port = htons(g_nTcpServicePort);
		oToAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
		if(connect(g_nTcpSocket, (const struct sockaddr*)(void*)&oToAddr, sizeof(oToAddr)))
		{
			closesocket(g_nTcpSocket);
			g_nTcpSocket = INVALID_SOCKET;
			return;
		}
		if(!ApplyVirtualConnect())
		{
			ShutDown(g_nTcpSocket);
			closesocket(g_nTcpSocket);
			g_nTcpSocket = INVALID_SOCKET;
			return;
		}
		if(!CreateThread(SocketRecvThread, NULL, 4096))
		{
			ShutDown(g_nTcpSocket);
			closesocket(g_nTcpSocket);
			g_nTcpSocket = INVALID_SOCKET;
			return;
		}
		char* pMsg= new char[1028];
		*(unsigned short*)pMsg = 2;//StdIn Msg
		while(true)
		{
			pMsg[4] = '\0';
			g_oReadEvent.Set();
			fgets(pMsg+4, 1028-4, StdInPipe[0]);
			if(pMsg[4] == '\0')
			{
				if(StdInPipe[1] == NULL)
					break;
				continue;
			}
			//strcat(pMsg+4, "\r\n");
			int nLen = strlen(pMsg+4);
			if(nLen > 1024)
				break;
			if(nLen > 0)
			{
				*(unsigned short*)(pMsg+2) = (unsigned short)nLen;
				if(!TcpSend(g_nTcpSocket, pMsg, nLen+4))
					break;
			}
		}
		delete[] pMsg;
		ShutDown(g_nTcpSocket);
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
	}
}

void ConnectApplication(int argc, char* argv[])
{
	int d;

	if(sscanf(argv[1], "%d", &d) == 1 && d>=0 && d<65536)
		g_nVirtualPort = (unsigned short)d;

	if(g_nVirtualPort == 0)
		return;

	g_nTcpServicePort = GetDefaultTcpServicePort();
	g_nTcpSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in oToAddr;
	memset(&oToAddr, 0, sizeof(oToAddr));
	oToAddr.sin_family = AF_INET;
	oToAddr.sin_port = htons(g_nTcpServicePort);
	oToAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if(connect(g_nTcpSocket, (const struct sockaddr*)(void*)&oToAddr, sizeof(oToAddr)))
	{
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
		return;
	}
	if(!ApplyVirtualConnect())
	{
		ShutDown(g_nTcpSocket);
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
		return;
	}
	if(!CreateThread(SocketRecvThread, NULL, 4096))
	{
		ShutDown(g_nTcpSocket);
		closesocket(g_nTcpSocket);
		g_nTcpSocket = INVALID_SOCKET;
		return;
	}
	char* pMsg= new char[1028];
	*(unsigned short*)pMsg = 2;//StdIn Msg
	while(true)
	{
		pMsg[4] = '\0';
		g_oReadEvent.Set();
		fgets(pMsg+4, 1028-4, StdInPipe[0]);
		if(pMsg[4] == '\0')
		{
			if(StdInPipe[1] == NULL)
				break;
			continue;
		}
		//strcat(pMsg+4, "\r\n");
		int nLen = strlen(pMsg+4);
		if(nLen >= 1024)
			break;
		if(nLen > 0)
		{
			*(unsigned short*)(pMsg+2) = (unsigned short)nLen;
			if(!TcpSend(g_nTcpSocket, pMsg, nLen+4))
				break;
		}
	}
	delete[] pMsg;
	ShutDown(g_nTcpSocket);
	closesocket(g_nTcpSocket);
	g_nTcpSocket = INVALID_SOCKET;
}

#ifndef WINDOWS
int strcmpi(const char *s1, const char *s2)
{
	int c;
	static int s[256];
	static int i=0;
	if(!i)
	{
		i=1;
		int j;
		for(j=0; j<256; ++j)s[j] = j;
		for(j=65; j<91; ++j)s[j] += 32;
	}

	if(!s1 || !s2) return 0;
	while( !(c = (s[*(unsigned char*)s1] - s[*(unsigned char*)s2])) )
	{
		if(!(*s1))break;
		s1 ++;
		s2 ++;
	}
	if(c<0) return -1;
	else if(c>0)return 1;
	return 0;
}
#endif

int main(int argc, char* argv[])
{
#ifdef WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup(wVersionRequested, &wsaData);
#else
	signal(SIGQUIT, sig_fatal);
#endif
	signal(SIGINT, sig_fatal);
	signal(SIGTERM, sig_fatal);

	InitializeStdInPipe();

	CreateThread(StdInRecvThread, NULL, 4096);

	char pBuf[1024];

	while(true)
	{
		printf("#");
		pBuf[0] = '\0';
		g_oReadEvent.Set();
		fgets(pBuf, 1024, StdInPipe[0]);
		if(pBuf[0] == 0)
			continue;
		strchr(pBuf, '\r')[0] = '\0';
		char** sArgv = GetArgv(pBuf);
		int nArgc = 0;
		while(sArgv[nArgc])
			++nArgc;
		if(nArgc)
		{
			if(!strcmpi(sArgv[0], "Help"))
			{
				printf("Help -- print the help information\n");
				printf("List -- list all applications in the job management system\n");
				printf("Start [-r] [-aPort|-bPort] application-command-line\n");
				printf("\tstart the specified application into the job management system\n");
				printf("\t[-r] -- auto restart the application after the application exited\n");
				printf("\t[-aPort] -- start the applicaiton, and interact with it\n");
				printf("\t[-bPort] -- start the applicaiton, and can use the 'Connect' command to interact with it\n");
				printf("Connect Port\n");
				printf("\tconnect to the specified application in the job management system\n");
				printf("Clear\n");
				printf("\tclear the screen\n");
				printf("Ctrl^C -- Exit the interaction with the application or system\n");
			}
			else if(!strcmpi(sArgv[0], "List"))
				ListApplication();
			else if(!strcmpi(sArgv[0], "Start"))
			{
				sArgv[0] = argv[0];
				StartApplication(nArgc, sArgv);
			}
			else if(!strcmpi(sArgv[0], "Connect"))
				ConnectApplication(nArgc, sArgv);
			else if(!strcmpi(sArgv[0], "Clear"))
			{
#ifdef WINDOWS
				system("cls");
#else
				system("clear");
#endif
			}
		}
		free(sArgv);

		g_nVirtualPort = 0;
		g_nBindPort = 0;
		g_nTcpServicePort = FOCP_RUNNER_TCPSERVICEPORT;
		g_nUdpServicePort = FOCP_RUNNER_UDPSERVICEPORT;
		g_bNeedConnect = false;
		g_bLoop = false;
		g_nAppIdx = 0;
		g_nPortIdx = 0;

		if(g_nTcpSocket != INVALID_SOCKET)
		{
			ShutDown(g_nTcpSocket);
			closesocket(g_nTcpSocket);
			g_nTcpSocket = INVALID_SOCKET;
		}
		if(g_nUdpSocket != INVALID_SOCKET)
		{
			closesocket(g_nUdpSocket);
			g_nUdpSocket = INVALID_SOCKET;
		}

		ResetStdInPipe();
	}

#ifdef WINDOWS
	WSACleanup();
#endif
	return 0;
}
