
#ifdef WINDOWS
#include "AdvWin32.h"
#endif

#include "../BaseDef.hpp"

#ifdef WINDOWS

#ifdef MSVC
#pragma warning(disable: 4001)
#endif

#if defined(_MIPS_) || defined(_ALPHA_)
#define STACKPTR(Context) (Context.IntSp)
#elif defined(_X86_)
#define STACKPTR(Context) (Context.Esp)
#else
#error only support MIPS,ALPHA,X86 CPU
#endif

PVOID AllocProcessMemory(HANDLE hProcess, DWORD dwNumBytes)
{
	CONTEXT Context;
	DWORD dwThreadId, dwNumBytesXferred, dwError;
	HANDLE hThread;
	HINSTANCE hinstKrnl = GetModuleHandle("Kernel32");
	PVOID pvMem = NULL;
	MEMORY_BASIC_INFORMATION mbi;
	BOOL fOK = FALSE;

	hThread = CreateRemoteThread(hProcess, NULL, dwNumBytes+sizeof(HANDLE),
								 (LPTHREAD_START_ROUTINE)GetProcAddress(hinstKrnl, "ExitThread"),
								 0, CREATE_SUSPENDED, &dwThreadId);

	if(hThread == NULL)
	{
		dwError = GetLastError();
		goto end;
	}
	Context.ContextFlags = CONTEXT_CONTROL;
	if(!GetThreadContext(hThread, &Context))
		goto end;
	if(sizeof(mbi) != VirtualQueryEx(hProcess, (DWORD*)STACKPTR(Context)-1, &mbi, sizeof(mbi)))
		goto end;
	pvMem = (PVOID)mbi.BaseAddress;
	fOK = WriteProcessMemory(hProcess, pvMem, &hThread, sizeof(hThread), &dwNumBytesXferred);
	if(!fOK)
		goto end;
	pvMem = (PVOID)((PHANDLE)pvMem+1);

end:
	if(!fOK)
	{
		if(hThread)
		{
			ResumeThread(hThread);
			CloseHandle(hThread);
		}
		pvMem = NULL;
	}

	return pvMem;
}

BOOL FreeProcessMemory(HANDLE hProcess, PVOID pvMem)
{
	BOOL fOK;
	HANDLE hThread;
	DWORD dwNumBytes;

	pvMem = (PVOID)((PHANDLE)pvMem-1);
	fOK = ReadProcessMemory(hProcess, pvMem, &hThread, sizeof(hThread), &dwNumBytes);
	if(fOK)
	{
		if(ResumeThread(hThread) == 0xFFFFFFFF)
			fOK = FALSE;
		CloseHandle(hThread);
	}

	return fOK;
}

#endif

#ifdef WINDOWS
extern "C" {
	__declspec(dllimport) HWND WINAPI GetConsoleWindow(void);
}
#endif

#ifndef WINDOWS
struct pthread_event_t
{
	int bmanual;
	int bvalue;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};
#endif

#ifndef WINDOWS
extern "C" CThreadReturn FOCP_CALL WaitProcessThread(void* pPara)
{
	while(true)
	{
		wait(NULL);
	}
	return (CThreadReturn)0;
}
static char* sAppName;
#endif

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

struct CProcessPipes
{
	int StdInPipe[2];
	int StdOutPipe[2];
	int StdErrPipe[2];
};

static CProcessPipes g_oPipes;
static SOCKET g_nTcpClientSocket = INVALID_SOCKET;
static bool g_bLoop = false;
static bool g_bBackGround = false;
static bool g_bHide = false;
static unsigned short g_nVirtualPort = 0;
static unsigned short g_nClientPort = 0;
static int g_nStdFiles[3];
static CThreadEvent g_oStdOutEvent;

int Pipe(int fd[2]);
bool InitializeProcessPipe();
void BindProcessPipe();
void UnBindProcessPipe();
void CleanProcessPipe(bool bCleanParent, bool bCleanChild);
bool CreateTcpClientThread();
bool ConnectToTcpServer(unsigned short nVirtualPort, char* argv[]);
void DisConnectToTcpServer();
unsigned short GetDefaultTcpServicePort();

bool ApplyVirtualBind(char* argv[]);
void NotifyClient(unsigned short nClientPort, bool bMaybe);

extern "C" CThreadReturn FOCP_CALL SocketRecvThread(void*);
extern "C" CThreadReturn FOCP_CALL StdOutRecvThread(void*);
extern "C" CThreadReturn FOCP_CALL StdErrRecvThread(void*);

int Pipe(int fd[2])
{
#ifdef WINDOWS
	return _pipe(fd, 512, _O_TEXT);
#else
	return pipe(fd);
#endif
}

bool InitializeProcessPipe()
{
	if(Pipe(g_oPipes.StdInPipe))
		return false;
	if(Pipe(g_oPipes.StdOutPipe))
		return false;
	if(Pipe(g_oPipes.StdErrPipe))
		return false;

	return true;
}

void BindProcessPipe()
{
	fflush(stdout);
	for(int i=0; i<3; ++i)
		g_nStdFiles[i] = dup(i);
	dup2(g_oPipes.StdInPipe[0], 0);
	dup2(g_oPipes.StdOutPipe[1], 1);
	dup2(g_oPipes.StdErrPipe[1], 2);
	CleanProcessPipe(false, true);
}

void UnBindProcessPipe()
{
	for(int i=0; i<3; ++i)
	{
		dup2(g_nStdFiles[i], i);
		close(g_nStdFiles[i]);
	}
}

void CleanProcessPipe(bool bCleanParent, bool bCleanChild)
{
	if(g_oPipes.StdInPipe[0]>=0 && bCleanChild)
	{
		close(g_oPipes.StdInPipe[0]);
		g_oPipes.StdInPipe[0] = -1;
	}
	if(g_oPipes.StdInPipe[1]>=0 && bCleanParent)
	{
		close(g_oPipes.StdInPipe[1]);
		g_oPipes.StdInPipe[1] = -1;
	}
	if(g_oPipes.StdOutPipe[0]>=0 && bCleanParent)
	{
		close(g_oPipes.StdOutPipe[0]);
		g_oPipes.StdOutPipe[0] = -1;
	}
	if(g_oPipes.StdOutPipe[1]>=0 && bCleanChild)
	{
		close(g_oPipes.StdOutPipe[1]);
		g_oPipes.StdOutPipe[1] = -1;
	}
	if(g_oPipes.StdErrPipe[0]>=0 && bCleanParent)
	{
		close(g_oPipes.StdErrPipe[0]);
		g_oPipes.StdErrPipe[0] = -1;
	}
	if(g_oPipes.StdErrPipe[1]>=0 && bCleanChild)
	{
		close(g_oPipes.StdErrPipe[1]);
		g_oPipes.StdErrPipe[1] = -1;
	}
}

bool CreateTcpClientThread()
{
	if(!CreateThread(SocketRecvThread, NULL, 4096))
		return false;

	if(!CreateThread(StdOutRecvThread, NULL, 4096))
		return false;

	if(!CreateThread(StdErrRecvThread, NULL, 4096))
		return false;

	return true;
}

bool ConnectToTcpServer(unsigned short nVirtualPort, char* argv[])
{
	g_nTcpClientSocket = socket(AF_INET, SOCK_STREAM, 0);

	if(INVALID_SOCKET == g_nTcpClientSocket)
		return false;

	if(!InitializeProcessPipe())
	{
		closesocket(g_nTcpClientSocket);
		CleanProcessPipe(true, true);
		return false;
	}

	unsigned short nTcpServicePort = GetDefaultTcpServicePort();

	sockaddr_in oConnectAddr;
	memset(&oConnectAddr, 0, sizeof(oConnectAddr));
	oConnectAddr.sin_family = AF_INET;
	oConnectAddr.sin_port = htons(nTcpServicePort);
	oConnectAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(g_nTcpClientSocket, (const struct sockaddr*)(void*)&oConnectAddr, sizeof(oConnectAddr)))
	{
		closesocket(g_nTcpClientSocket);
		CleanProcessPipe(true, true);
		return false;
	}

	if(!ApplyVirtualBind(argv))
	{
		closesocket(g_nTcpClientSocket);
		CleanProcessPipe(true, true);
		return false;
	}

	BindProcessPipe();
	CreateTcpClientThread();

	return true;
}

bool ApplyVirtualBind(char* argv[])
{
	int argc;
	char* sCmdLine;
	argc = 0;
	sCmdLine = new char[1024];
	while(argv[argc])
		++argc;
	GetCmdLine(argc, argv, sCmdLine+6);

	unsigned short* pBuf = (unsigned short*)sCmdLine;
	pBuf[0] = 0;//ApplyVirtualBind
	pBuf[1] = g_nVirtualPort;
	pBuf[2] = strlen(sCmdLine+6)+1;
	if(!TcpSend(g_nTcpClientSocket, (char*)pBuf, pBuf[2]+6))
	{
		delete[] sCmdLine;
		return false;
	}
	if(!TcpRecv(g_nTcpClientSocket, (char*)pBuf, 4))
	{
		delete[] sCmdLine;
		return false;
	}
	if(pBuf[0] != 1)//ApplyVirtualBind Response
	{
		delete[] sCmdLine;
		return false;
	}
	if(pBuf[1] != 1)//VirtualBind failure
	{
		delete[] sCmdLine;
		return false;
	}
	delete[] sCmdLine;
	return true;
}

extern "C" CThreadReturn FOCP_CALL SocketRecvThread(void*)
{
	char pBuf[1024];

	while(true)
	{
		if(!TcpRecv(g_nTcpClientSocket, (char*)pBuf, 4))
			break;
		if(((unsigned short*)pBuf)[0] != 2)//StdIn Msg
			break;
		unsigned short nSize = ((unsigned short*)pBuf)[1];
		if(nSize == 0 || nSize > 1024)//invalid StdIn Msg Len
			break;
		if(!TcpRecv(g_nTcpClientSocket, (char*)pBuf, (int)nSize))
			break;
		if(g_oPipes.StdInPipe[1] >= 0)
			write(g_oPipes.StdInPipe[1], pBuf, (int)nSize);
	}
	return (CThreadReturn)0;
}

extern "C" CThreadReturn FOCP_CALL StdOutRecvThread(void*)
{
	char pBuf[1024];
	unsigned short pMsg[2] = {3, 0};//StdOut Msg
	while(true)
	{
		g_oStdOutEvent.Wait(1000);
		int nSize = (int)read(g_oPipes.StdOutPipe[0], pBuf, 1024);
		if(nSize <= 0)
			continue;
		pMsg[1] = (unsigned short)nSize;
		if(!TcpSend(g_nTcpClientSocket, (char*)pMsg, 4))
			break;
		if(!TcpSend(g_nTcpClientSocket, (char*)pBuf, nSize))
			break;
	}
	return (CThreadReturn)0;
}

extern "C" CThreadReturn FOCP_CALL StdErrRecvThread(void*)
{
	char pBuf[1024];
	unsigned short pMsg[2] = {4, 0};//StdErr Msg
	while(true)
	{
		g_oStdOutEvent.Wait(1000);
		int nSize = (int)read(g_oPipes.StdErrPipe[0], pBuf, 1024);
		if(nSize <= 0)
			continue;
		pMsg[1] = (unsigned short)nSize;
		if(!TcpSend(g_nTcpClientSocket, (char*)pMsg, 4))
			break;
		if(!TcpSend(g_nTcpClientSocket, (char*)pBuf, nSize))
			break;
	}
	return (CThreadReturn)0;
}

void NotifyClient(unsigned short nClientPort, bool bMaybe)
{
	sockaddr_in oToAddr;

	SOCKET nUdpClientSocket = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&oToAddr, 0, sizeof(oToAddr));
	oToAddr.sin_family = AF_INET;
	oToAddr.sin_port = htons(nClientPort);
	oToAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	char x = bMaybe?'1':'0';

	sendto(nUdpClientSocket, &x, 1, 0, (const struct sockaddr*)(void*)&oToAddr, sizeof(oToAddr));

	closesocket(nUdpClientSocket);
}

void DisConnectToTcpServer()
{
	SOCKET nSock = g_nTcpClientSocket;
	g_nTcpClientSocket = INVALID_SOCKET;
	if(nSock != INVALID_SOCKET)
		closesocket(nSock);
}

#ifndef WINDOWS
void CloseNotUsedFiles()
{
	int fd, fdtablesize;
	fdtablesize = getdtablesize();
	for(fd = 3; fd < fdtablesize; fd++)
	{
		if(g_oPipes.StdInPipe[0] != fd && g_oPipes.StdInPipe[1] != fd &&
				g_oPipes.StdOutPipe[0] != fd && g_oPipes.StdOutPipe[1] != fd &&
				g_oPipes.StdErrPipe[0] != fd && g_oPipes.StdErrPipe[1] != fd)
			close(fd);
	}
}
#endif

void Run(char* argv[])
{
	if(g_nVirtualPort)
	{
#ifdef WINDOWS
		WORD wVersionRequested;
		WSADATA wsaData;
		wVersionRequested = MAKEWORD( 2, 2 );
		WSAStartup(wVersionRequested, &wsaData);
#endif
		bool bMaybe = ConnectToTcpServer(g_nVirtualPort, argv);
		if(g_nClientPort)
			NotifyClient(g_nClientPort, bMaybe);
		if(bMaybe == false)
		{
			UnBindProcessPipe();
			return;
		}
	}
#ifndef WINDOWS
	g_oStdOutEvent.Set();
	if(g_nVirtualPort)
	{
		char s[100];
		char* ldp = getenv("LD_PRELOAD");
		sprintf(s, "LD_PRELOAD=%s%s%s", ldp?ldp:"",ldp?":":"", sAppName);
		char* x = s + strlen(s) - 8;
		strcpy(x, "libSetBuf.so");
		putenv(s);
	}
	while(true)
	{
		int pid;
		if(0 == (pid=vfork()))
		{
			CloseNotUsedFiles();
			if (setsid() < 0)
				setpgid(getpid(), getpid());
			execv(argv[0], argv);
			_exit(0);
		}
		else
		{
			if(g_nVirtualPort)
				UnBindProcessPipe();
			if(!g_bBackGround)
				waitpid(pid, NULL, 0);
			if(g_nVirtualPort)
			{
				g_oStdOutEvent.Reset();
				CleanProcessPipe(true, false);
			}
		}
		if(!g_bLoop || g_bBackGround)
			break;
		if(g_nVirtualPort)
		{
			InitializeProcessPipe();
			BindProcessPipe();
			g_oStdOutEvent.Set();
		}
	}
#else
	while(true)
	{
		DWORD nCreation;
		STARTUPINFO oStart;
		PROCESS_INFORMATION oInfo;
		int argc;
		char* sCmdLine;

		argc = 0;
		sCmdLine = new char[512];
		while(argv[argc])
			++argc;
		GetCmdLine(argc, argv, sCmdLine);

		nCreation = 0;
		memset(&oStart,0,sizeof(oStart));
		oStart.cb = sizeof(oStart);

		if(g_bBackGround)
			nCreation |= DETACHED_PROCESS;
		else
		{
			oStart.dwFlags = STARTF_USESHOWWINDOW;
			if(g_bHide)
				oStart.wShowWindow = SW_HIDE;
			else
				oStart.wShowWindow = SW_SHOWDEFAULT;
			if(g_nVirtualPort)
				nCreation |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION* pInfo = &oInfo;//g_bBackGround?(PROCESS_INFORMATION*)NULL:&oInfo;

		if(CreateProcess(NULL, sCmdLine, NULL, NULL, TRUE, nCreation, NULL, NULL, &oStart, pInfo))
		{
			if(g_nVirtualPort)
			{
				DWORD nXferred;
				HINSTANCE hinstKrnl = GetModuleHandle("Kernel32");
				char* sEnd = argv[0]+strlen(argv[0])-1;
				while(sEnd!=argv[0])
				{
					if(sEnd[0] != '/' || sEnd[0] != '\\' || sEnd[0] != ':')
						--sEnd;
					else
					{
						++sEnd;
						break;
					}
				}
				int nLen = sEnd - argv[0];
				PVOID pMem = AllocProcessMemory(pInfo->hProcess, nLen + 11);//"SetBuf.dll";
				WriteProcessMemory(pInfo->hProcess, pMem, argv[0], nLen, &nXferred);
				WriteProcessMemory(pInfo->hProcess, (char*)pMem+nLen, "SetBuf.dll", 11, &nXferred);
				HANDLE hThread = CreateRemoteThread(pInfo->hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(hinstKrnl, "LoadLibraryA"), pMem, 0, &nXferred);
				WaitForSingleObject(hThread, 0xFFFFFFFF);
				CloseHandle(hThread);
				FreeProcessMemory(pInfo->hProcess, pMem);
				ResumeThread(pInfo->hThread);
			}
			delete[] sCmdLine;
			if(g_nVirtualPort)
				UnBindProcessPipe();
			if(!g_bBackGround)
			{
				WaitForSingleObject(pInfo->hProcess, -1);
				CloseHandle(pInfo->hProcess);
				CloseHandle(pInfo->hThread);
			}
			if(g_nVirtualPort)
			{
				g_oStdOutEvent.Reset();
				CleanProcessPipe(true, false);
			}
		}
		else
		{
			delete[] sCmdLine;
			UnBindProcessPipe();
			CleanProcessPipe(true, false);
			break;
		}
		if(!g_bLoop || g_bBackGround)
			break;
		if(g_nVirtualPort)
		{
			InitializeProcessPipe();
			BindProcessPipe();
			g_oStdOutEvent.Set();
		}
	};
#endif
	if(g_nVirtualPort)
	{
		DisConnectToTcpServer();
#ifdef WINDOWS
		WSACleanup();
#endif
	}
}

int main(int argc, char* argv[])
{
	//-r,-a,-h,-v,-c,x.exe
	int i, j;

#ifndef WINDOWS
	sAppName = argv[0];
#endif

	g_oPipes.StdInPipe[0] = g_oPipes.StdInPipe[1] = -1;
	g_oPipes.StdOutPipe[0] = g_oPipes.StdOutPipe[1] = -1;
	g_oPipes.StdErrPipe[0] = g_oPipes.StdErrPipe[1] = -1;

	for(i=1; i<argc; ++i)
	{
		if(!strcmp(argv[i], "-r"))
			g_bLoop = true;
		else if(!strncmp(argv[i], "-v", 2))
		{
			int d;
			if(sscanf(argv[i]+2, "%d", &d) == 1 && d>0 && d<65536)
				g_nVirtualPort = (unsigned short)d;
		}
		else if(!strncmp(argv[i], "-c", 2))
		{
			int d;
			if(sscanf(argv[i]+2, "%d", &d) == 1 && d>0 && d<65536)
				g_nClientPort = (unsigned short)d;
		}
		else if(!strcmp(argv[i], "-a"))
		{
			g_bBackGround = true;
			j = i;
		}
		else if(!strcmp(argv[i], "-h"))
			g_bHide = true;
		else
			break;
	}

	if(g_nVirtualPort)
		g_oStdOutEvent.Set();

	if(i<argc)
	{
#ifdef WINDOWS
		if(g_bBackGround)//后台运行
		{
			if(g_bLoop)
			{
				argv[j] = (char*)"-h";
				_spawnv(_P_DETACH, argv[0], argv);
				return 0;
			}
		}
		else if(g_bHide && g_nVirtualPort)
		{
			HWND hActive = GetForegroundWindow();
			AllocConsole();
			ShowWindow(GetConsoleWindow(), SW_HIDE);
			SetForegroundWindow(hActive);
		}
#else
		if(g_bBackGround)//后台运行
		{
			if(g_bLoop)
			{
				if(vfork())
					return 0;
				CloseNotUsedFiles();
				if (setsid() < 0)
					setpgid(getpid(), getpid());
				g_bBackGround = false;
			}
		}
#endif
		MoveArgv(argc, argv, i);
		Run(argv);
		_exit(0);
	}
	return 0;
}
