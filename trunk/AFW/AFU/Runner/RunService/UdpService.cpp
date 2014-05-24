
#include "../BaseDef.hpp"

static unsigned short g_nUdpServicePort = FOCP_RUNNER_UDPSERVICEPORT;
static SOCKET g_nUdpSocket = INVALID_SOCKET;

#ifndef WINDOWS
extern "C" CThreadReturn FOCP_CALL WaitProcessThread(void* pPara)
{
	while(true)
	{
		wait(NULL);
	}
	return (CThreadReturn)0;
}
#endif

void UdpService()
{
#ifdef WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup(wVersionRequested, &wsaData);
#endif

	g_nUdpServicePort = GetDefaultUdpServicePort();
	g_nUdpSocket = socket(AF_INET, SOCK_DGRAM, 0);

	int breuse;
	breuse = 1;
	setsockopt(g_nUdpSocket, SOL_SOCKET, SO_REUSEADDR, (char *)(&breuse), sizeof(int));

	sockaddr_in oBindAddr;
	memset(&oBindAddr, 0, sizeof(oBindAddr));
	oBindAddr.sin_family = AF_INET;
	oBindAddr.sin_port = htons(g_nUdpServicePort);
	oBindAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if(bind(g_nUdpSocket, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr)))
	{
		closesocket(g_nUdpSocket);
#ifdef WINDOWS
		WSACleanup();
#endif
		return;
	}

#ifndef WINDOWS
	if(!CreateThread(WaitProcessThread, NULL, 1024))
	{
		closesocket(g_nUdpSocket);
		return;
	}
#endif
	
	char* pMsg= new char[4096];
	while(true)
	{
		sockaddr_in oFrom;
		socklen_t nLen = sizeof(oFrom);
		int nRet = recvfrom(g_nUdpSocket, pMsg, 4096, 0, (struct sockaddr*)(void*)&oFrom, &nLen);
		if(nRet <= 0)
			break;
		int nArgc;
		memcpy(&nArgc, pMsg, 4);
		char** sArgv = new char*[nArgc+1];
		sArgv[nArgc] = NULL;
		char* pShift = pMsg + 4;
		for(int i=0; i<nArgc; ++i)
		{
			sArgv[i] = pShift;
			pShift += strlen(pShift)+1;
		}
#ifdef WINDOWS
		_spawnv(_P_DETACH, sArgv[0], sArgv);
#else
		int fd, fdtablesize;
		if(vfork() == 0)
		{
			fdtablesize = getdtablesize();
			for(fd = 3; fd < fdtablesize; fd++)
				close(fd);
			if (setsid() < 0)
				setpgid(getpid(), getpid());
			execv(sArgv[0], sArgv);
			_exit(0);
		}
#endif
		delete[] sArgv;
	}
	delete[] pMsg;
	closesocket(g_nUdpSocket);
#ifdef WINDOWS
	WSACleanup();
#endif
}
