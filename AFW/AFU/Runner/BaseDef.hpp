
#if defined(WINDOWS)
	#if _WIN32_WINNT < 0x0400
		#define _WIN32_WINNT 0x0400
	#endif
	#include <windows.h>
	#include <process.h>
	#include <io.h>
	#include <fcntl.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
  #include <arpa/inet.h>
	#include <errno.h>
	#include <stdlib.h>
	#include <sys/wait.h>
	#include <pthread.h>
	#include <sys/time.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#ifndef FOCP_CALL
#if defined(WINDOWS)
#define FOCP_CALL __stdcall
#else
#define FOCP_CALL
#endif
#endif

#ifndef WINDOWS
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define closesocket close
typedef void* CThreadReturn;
#else
#define _dup2 dup2
typedef unsigned int CThreadReturn;
#endif

#ifndef __FOCP__GNU__
typedef int socklen_t;
#endif

#define FOCP_RUNNER_UDPSERVICEPORT 1981
#define FOCP_RUNNER_TCPSERVICEPORT 1982

extern "C"
{
	typedef CThreadReturn (FOCP_CALL *FThreadProc)(void*);
}

//Thread
bool CreateThread(FThreadProc ThreadProc, void* pContext, unsigned int nStackSize);
char** GetArgv(const char* sCmdLine);
void MoveArgv(int argc, char* argv[], int nRemove);
unsigned short GetDefaultTcpServicePort();
unsigned short GetDefaultUdpServicePort();
bool TcpSend(SOCKET nSock, char *pBuf, int nBufLen);
bool TcpRecv(SOCKET nSock, char *pBuf, int nBufLen);
void ShutDown(SOCKET nSock);
void GetCmdLine(int argc, char*argv[], char* sCmdLine);
