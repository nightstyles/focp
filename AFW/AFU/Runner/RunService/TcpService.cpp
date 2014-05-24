#include "../BaseDef.hpp"

#ifdef MSVC
#pragma warning(disable:4786)
#endif

#include <set>
#include <map>
#include <string>

using namespace std;

struct CStreamNode
{
	SOCKET nSock;
	string oCmdLine;
	set<SOCKET> oClients;
};

struct CClientThreadContext
{
	unsigned short bBegin;
	unsigned short nVirtualPort;
	SOCKET nClientSock;
	SOCKET nStreamSock;
};

struct CStreamThreadContext
{
	unsigned short bBegin;
	unsigned short nVirtualPort;
	SOCKET nSock;
};

#ifdef UNIX
struct CRITICAL_SECTION
{
	pthread_mutexattr_t attr;
	pthread_mutex_t mutex;
};
#endif

class CThreadMutex
{
private:
	void* m_pMutex;
	//下两个字段用于调试目的，可以查看当前锁被那个线程占用
	unsigned long m_nThreadId;//当m_nCounter>0时有效
	unsigned int m_nCounter;

	CThreadMutex(const CThreadMutex&);
	CThreadMutex& operator=(const CThreadMutex&);

public:
	CThreadMutex()
	{
#ifdef WINDOWS
		m_pMutex = new CRITICAL_SECTION;
		InitializeCriticalSection((CRITICAL_SECTION*)m_pMutex);
#endif

#ifdef UNIX
		CRITICAL_SECTION* pMutex = new CRITICAL_SECTION;
		pthread_mutexattr_init(&pMutex->attr);
		pthread_mutexattr_settype(&pMutex->attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&pMutex->mutex, &pMutex->attr);
		m_pMutex = pMutex;
#endif

		m_nThreadId = 0;
		m_nCounter = 0;
	}

	~CThreadMutex()
	{
		if(m_nCounter && m_nThreadId != GetCurrentThreadId())
			FocpAbort(("CThreadMutex::~CThreadMutex() failure")); ";
#ifdef WINDOWS
		DeleteCriticalSection((CRITICAL_SECTION*)m_pMutex);
		delete (CRITICAL_SECTION*)m_pMutex;
		m_pMutex = NULL;
#endif

#ifdef UNIX
		CRITICAL_SECTION* pMutex = (CRITICAL_SECTION*)m_pMutex;
		pthread_mutex_destroy(&pMutex->mutex);
		pthread_mutexattr_destroy(&pMutex->attr);
		delete pMutex;
		m_pMutex = NULL;
#endif
	}

	void Enter()
	{
#ifdef WINDOWS
		EnterCriticalSection((CRITICAL_SECTION*)m_pMutex);
#endif

#ifdef UNIX
		pthread_mutex_lock(&((CRITICAL_SECTION*)m_pMutex)->mutex);
#endif

		m_nThreadId = GetCurrentThreadId();
		++m_nCounter;
	}

	void Leave()
	{
		if(!m_nCounter || m_nThreadId != GetCurrentThreadId())
			FocpAbort(("CThreadMutex::Leave() failure"));";

		--m_nCounter;

#ifdef WINDOWS
		LeaveCriticalSection((CRITICAL_SECTION*)m_pMutex);
#endif

#ifdef UNIX
		pthread_mutex_unlock(&((CRITICAL_SECTION*)m_pMutex)->mutex);
#endif
	}

	static void Sleep(unsigned int nTimeOut)
	{
#ifdef UNIX
		timespec tt;
		tt.tv_sec = nTimeOut/1000;
		tt.tv_nsec = (nTimeOut%1000)*1000000;
		nanosleep(&tt,  NULL);
#endif

#ifdef WINDOWS
		::Sleep(nTimeOut);
#endif
	}

private:
	unsigned long GetCurrentThreadId()
	{
		unsigned long nThreadId = 0;
#ifdef WINDOWS
		DWORD &nTid = *(DWORD*)&nThreadId;
		nTid = ::GetCurrentThreadId();
#endif

#ifdef UNIX
		pthread_t &nTid = *(pthread_t*)&nThreadId;
		nTid = pthread_self();
#endif

		return nThreadId;
	}
};

static CThreadMutex g_oMutex;
static map<unsigned short, CStreamNode> g_oStreamTable;
static SOCKET g_nListenSocket = INVALID_SOCKET;
static unsigned short g_nTcpServicePort = FOCP_RUNNER_TCPSERVICEPORT;

static void ListProcessInfo(SOCKET nSock)
{
	g_oMutex.Enter();
	map<unsigned short, CStreamNode>::iterator idx = g_oStreamTable.begin();
	map<unsigned short, CStreamNode>::iterator end = g_oStreamTable.end();
	for(; idx != end; ++idx)
	{
		CStreamNode& oNode = idx->second;
		unsigned short nPort = idx->first;
		char sPort[10];
		sprintf(sPort, "%d", (int)nPort);
		string oLine(sPort);
		oLine += '\t';
		oLine += oNode.oCmdLine;
		oLine += '\n';
		TcpSend(nSock, (char*)oLine.c_str(), oLine.length());
	}
	g_oMutex.Leave();
	closesocket(nSock);
}

static void RemoveClient(unsigned short nVirtualPort, SOCKET nClientSock)
{
	g_oMutex.Enter();
	map<unsigned short, CStreamNode>::iterator idx = g_oStreamTable.find(nVirtualPort);
	if(idx != g_oStreamTable.end())
	{
		CStreamNode& oNode = idx->second;
		oNode.oClients.erase(nClientSock);
	}
	g_oMutex.Leave();
}

static void RemoveNode(unsigned short nVirtualPort, bool bClose=false)
{
	g_oMutex.Enter();
	map<unsigned short, CStreamNode>::iterator idx = g_oStreamTable.find(nVirtualPort);
	if(idx != g_oStreamTable.end())
	{
		CStreamNode& oNode = idx->second;
		set<SOCKET>::iterator it = oNode.oClients.begin();
		set<SOCKET>::iterator end = oNode.oClients.end();
		for(; it!=end; ++it)
		{
			SOCKET nSock = *it;
			ShutDown(nSock);
			closesocket(nSock);
		}
		if(bClose)
		{
			ShutDown(oNode.nSock);
			closesocket(oNode.nSock);
		}
		g_oStreamTable.erase(idx);
	}
	g_oMutex.Leave();
}

static CStreamNode* FindNode(unsigned short nVirtualPort)
{
	CStreamNode* pNode = NULL;
	g_oMutex.Enter();
	map<unsigned short, CStreamNode>::iterator idx = g_oStreamTable.find(nVirtualPort);
	if(idx != g_oStreamTable.end())
	{
		CStreamNode& oNode = idx->second;
		pNode = &oNode;
	}
	g_oMutex.Leave();
	return pNode;
}

static void CreateNode(unsigned short nVirtualPort, SOCKET nSock, char* sCmdLine)
{
	g_oMutex.Enter();
	CStreamNode& oNode = g_oStreamTable[nVirtualPort];
	oNode.nSock = nSock;
	oNode.oCmdLine = sCmdLine;
	g_oMutex.Leave();
}

extern "C" CThreadReturn FOCP_CALL ClientRecvThread(void* pPara)
{
	CClientThreadContext* pContext = (CClientThreadContext*)pPara;
	CClientThreadContext oContext = *pContext;
	pContext->bBegin = 1;
	char* pMsg= new char[1028];
	while(true)
	{
		if(!TcpRecv(oContext.nClientSock, (char*)pMsg, 4))
			break;
		if(((unsigned short*)pMsg)[0] != 2)//StdIn Msg
			break;
		unsigned short nSize = ((unsigned short*)pMsg)[1];
		if(nSize >= 1024)
			break;
		if(!TcpRecv(oContext.nClientSock, (char*)pMsg+4, (int)nSize))
			break;
		if(!TcpSend(oContext.nStreamSock, (char*)pMsg, 4+nSize))
			break;
	}
	delete[] pMsg;
	RemoveClient(oContext.nVirtualPort, oContext.nClientSock);
	ShutDown(oContext.nClientSock);
	closesocket(oContext.nClientSock);
	return (CThreadReturn)0;
}

extern "C" CThreadReturn FOCP_CALL StreamRecvThread(void* pPara)
{
	CStreamThreadContext* pContext = (CStreamThreadContext*)pPara;
	CStreamThreadContext oContext = *pContext;
	pContext->bBegin = 1;
	char* pMsg= new char[1024];
	CStreamNode* pNode = FindNode(oContext.nVirtualPort);
	while(true)
	{
		if(!TcpRecv(oContext.nSock, (char*)pMsg, 4))
			break;
		unsigned short nCmd[] = {((unsigned short*)pMsg)[0], ((unsigned short*)pMsg)[1]};
		if(nCmd[0] != 3 && nCmd[0] != 4)
			break;
		if(nCmd[1] == 0 || nCmd[1] > 1024)//invalid Msg Len
			break;
		if(!TcpRecv(oContext.nSock, (char*)pMsg, nCmd[1]))
			break;
		g_oMutex.Enter();
		set<SOCKET>::iterator idx = pNode->oClients.begin();
		set<SOCKET>::iterator end = pNode->oClients.end();
		for(; idx!=end; ++idx)
		{
			SOCKET nSock = *idx;
			TcpSend(nSock, (char*)nCmd, 4);
			TcpSend(nSock, (char*)pMsg, nCmd[1]);
		}
		g_oMutex.Leave();
	}
	delete[] pMsg;
	RemoveNode(oContext.nVirtualPort);
	ShutDown(oContext.nSock);
	closesocket(oContext.nSock);
	return (CThreadReturn)0;
}

void TcpService()
{
	sockaddr_in oFrom;
	unsigned short nCmd[2];
	CClientThreadContext oClient;
	CStreamThreadContext oStream;
	CStreamNode* pNode;

#ifdef WINDOWS
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );
	WSAStartup(wVersionRequested, &wsaData);
#endif

	g_nTcpServicePort = GetDefaultTcpServicePort();
	g_nListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	memset(&oFrom, 0, sizeof(oFrom));
	oFrom.sin_family = AF_INET;
	oFrom.sin_port = htons(g_nTcpServicePort);
	oFrom.sin_addr.s_addr = inet_addr("127.0.0.1");

	int breuse;
	breuse = 1;
	setsockopt(g_nListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)(&breuse), sizeof(int));

	if(bind(g_nListenSocket, (const struct sockaddr*)(void*)&oFrom, sizeof(oFrom)))
	{
		closesocket(g_nListenSocket);
#ifdef WINDOWS
		WSACleanup();
#endif
		return;
	}
	if(listen(g_nListenSocket, 5))
	{
		closesocket(g_nListenSocket);
#ifdef WINDOWS
		WSACleanup();
#endif
		return;
	}

	while(true)
	{
		socklen_t nLen = sizeof(oFrom);
		SOCKET nSock = accept(g_nListenSocket, (struct sockaddr*)(void*)&oFrom, &nLen);
		if(nSock == INVALID_SOCKET)
			break;
		fd_set rset;
		struct timeval tm;
		FD_ZERO(&rset);
		FD_SET(nSock, &rset);
		int maxid;
		maxid = nSock+1;
		tm.tv_sec = 5;
		tm.tv_usec = 0;
		if(!select(maxid, &rset, NULL, NULL, &tm))
		{
			closesocket(nSock);
			continue;
		}
		int nret = recv(nSock, (char*)nCmd, 4, 0);
		if(nret != 4)
		{
			closesocket(nSock);
			continue;
		}
		switch(nCmd[0])
		{
		case 0://ApplyVirtualBind
			oStream.nVirtualPort = nCmd[1];
			pNode = NULL;
			nCmd[0] = 1;//ApplyVirtualBind response;
			if(oStream.nVirtualPort == 0)
				nCmd[1] = 0;//ApplyVirtualBind response;
			else
			{
				pNode = FindNode(oStream.nVirtualPort);
				if(pNode)
					nCmd[1] = 0;//ApplyVirtualBind response;
				else
				{
					nCmd[1] = 1;//ApplyVirtualBind response;
					unsigned short nLen;
					if(!TcpRecv(nSock, (char*)&nLen, 2) || nLen <= 1)
						nCmd[1] = 0;
					else
					{
						char* sCmdLine = new char[nLen];
						if(!TcpRecv(nSock, sCmdLine, nLen))
							nCmd[1] = 0;
						else
						{
							CreateNode(oStream.nVirtualPort, nSock, sCmdLine);
							oStream.bBegin = 0;
							oStream.nSock = nSock;
							if(!CreateThread(StreamRecvThread, &oStream, 8192))
							{
								RemoveNode(oStream.nVirtualPort);
								nCmd[1] = 0;//ApplyVirtualBind response;
							}
							else while(oStream.bBegin == 0)
									CThreadMutex::Sleep(10);
						}
						delete[] sCmdLine;
					}
				}
			}
			TcpSend(nSock, (char*)nCmd, 4);
			if(nCmd[1] == 0)
			{
				ShutDown(nSock);
				closesocket(nSock);
			}
			break;

		case 5://ApplyVirtualConnect
			oClient.nVirtualPort = nCmd[1];
			pNode = NULL;
			nCmd[0] = 6;//ApplyVirtualConnect response;
			if(oClient.nVirtualPort == 0)
			{
				ListProcessInfo(nSock);
				break;
			}
			else
			{
				pNode = FindNode(oClient.nVirtualPort);
				if(pNode == NULL)
					nCmd[1] = 0;//ApplyVirtualConnect response;
				else
				{
					nCmd[1] = 1;//ApplyVirtualConnect response;
					oClient.bBegin = 0;
					oClient.nClientSock = nSock;
					oClient.nStreamSock = pNode->nSock;
					pNode->oClients.insert(nSock);
					if(!CreateThread(ClientRecvThread, &oClient, 8192))
					{
						RemoveClient(oClient.nVirtualPort, nSock);
						nCmd[1] = 0;//ApplyVirtualConnect response;
					}
					else while(oClient.bBegin == 0)
							CThreadMutex::Sleep(10);
				}
			}
			TcpSend(nSock, (char*)nCmd, 4);
			if(nCmd[1] == 0)
			{
				ShutDown(nSock);
				closesocket(nSock);
			}
			break;

		default:
			ShutDown(nSock);
			closesocket(nSock);
			break;
		}
	}
	closesocket(g_nListenSocket);
	g_oMutex.Enter();
	while(true)
	{
		map<unsigned short, CStreamNode>::iterator idx = g_oStreamTable.begin();
		if(idx == g_oStreamTable.end())
			break;
		RemoveNode(idx->first, true);
	}
	g_oMutex.Leave();
#ifdef WINDOWS
	WSACleanup();
#endif
}
