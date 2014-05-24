
#include "IocpServer.hpp"

#if defined(WINDOWS) || defined(CYGWIN_NT)
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <errno.h>
#include <signal.h>
#endif

#include <stdio.h>

FOCP_BEGIN();

CIpcoQueue::CIpcoQueue()
	:m_oMsgQueue(FocpFieldOffset(CMsgItem, pNext))
{
	m_oEvent.Reset();
}

CIpcoQueue::~CIpcoQueue()
{
	CMsgItem* pMsg;
	while((pMsg = m_oMsgQueue.Pop()))
		delete pMsg;
}

void CIpcoQueue::PushMsg(uint32 hLink, CMemoryStream &oStream)
{
	CMsgItem* pMsg = new CMsgItem;
	pMsg->hLink = hLink;
	pMsg->oStream.Swap(oStream);
	pMsg->oStream.SetPosition(0);
	m_oMutex.Enter();
	if(!m_oMsgQueue.GetSize())
		m_oEvent.Set();
	m_oMsgQueue.Push(pMsg);
	m_oMutex.Leave();
}

int32 CIpcoQueue::ReadFrom(void* pBuf, int32 nBufLen, uint32& hLink)
{
	int32 nRet = 0;
	hLink = 0xFFFFFFFF;
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	CMsgItem* pMsg = m_oMsgQueue.First();
	if(pMsg)
	{
		hLink = pMsg->hLink;
		nRet = (int32)pMsg->oStream.Read(pBuf, nBufLen);
		if(nRet == 0)
			nRet = -1;
		else
		{
			uint32 nRest = pMsg->oStream.GetSize() - pMsg->oStream.GetPosition();
			if(nRest == 0)
			{
				m_oMsgQueue.Pop();
				delete pMsg;
				if(!m_oMsgQueue.GetSize())
					m_oEvent.Reset();
			}
			nRet += nRest;
		}
	}
	m_oMutex.Leave();
	return nRet;
}

bool CIpcoQueue::ReadFrom(CMemoryStream& oStream, uint32 &hLink)
{
	bool bRet = false;
	hLink = 0xFFFFFFFF;
	oStream.SetPosition(0);
	oStream.Truncate();
	m_oEvent.Wait(1000);
	m_oMutex.Enter();
	CMsgItem* pMsg = m_oMsgQueue.Pop();
	if(pMsg)
	{
		if(!m_oMsgQueue.GetSize())
			m_oEvent.Reset();
		hLink = pMsg->hLink;
		oStream.Swap(pMsg->oStream);
		delete pMsg;
		bRet = true;
	}
	m_oMutex.Leave();
	return bRet;
}

CStreamAssembler::CStreamAssembler()
{
}

CStreamAssembler::~CStreamAssembler()
{
}

int32 CStreamAssembler::OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg)
{
	oMsg.SetPosition(0);
	oMsg.Truncate();
	oMsg.Write((void*)sBuf, nBufLen);
	return 0;
}

void CStreamAssembler::UnBindLink()
{
	delete (CStreamAssembler*)this;
}

struct CIocpLinkItem
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	uint32 bAccept;
	OVERLAPPED oOverlapped;
	WSABUF oDataBuf;
	CIocpLinkItem* pPrev;
#endif
	ulong hFile;
	uint32 nStatus;//0=FREE, 1=IDLE, 2=BIND, 3=BUSY(Windows)
	uint16 nCounter;
	CStreamAssembler* pAssembler;
	char sBuf[1024];
	CIocpLinkItem* pNext;
	CIocpLinkItem()
	{
		hFile = (ulong)(-1);
		nStatus = 0;
		nCounter = 0;
		pAssembler = NULL;
		pNext = NULL;
#if defined(WINDOWS) || defined(CYGWIN_NT)
		pPrev = NULL;
		bAccept = 0;
#endif
	}
};

CIocpServer::CLinkContainer::CLinkContainer():
	oContainer(FocpFieldOffset(CIocpLinkItem, pNext))
{
	oEvent.Reset();
}

CIocpLinkItem* CIocpServer::CLinkContainer::AllocLink()
{
	CIocpLinkItem* pLink = oContainer.Pop();
	if(pLink)
	{
		if(oContainer.GetSize() == 0)
			oEvent.Reset();
		pLink->pNext = NULL;
	}
	return pLink;
}

void CIocpServer::CLinkContainer::FreeLink(CIocpLinkItem* pLink)
{
	if(oContainer.GetSize() == 0)
		oEvent.Set();
	oContainer.Push(pLink);
}

void CIocpServer::CLinkContainer::Clear()
{
	oEvent.Reset();
	oContainer.Clear();
}

CIocpServer::CLinkMap::CLinkMap(CCooperateFunction* pFun)
#if defined(WINDOWS) || defined(CYGWIN_NT)
	:oThread(pFun), oTable(FocpFieldOffset(CIocpLinkItem, pPrev), FocpFieldOffset(CIocpLinkItem, pNext))
#endif
{
}

void CIocpServer::CLinkMap::Clear()
{
	oTable.Clear();
#if defined(WINDOWS) || defined(CYGWIN_NT)
	oEvent.Reset();
#endif
}

#if defined(WINDOWS) || defined(CYGWIN_NT)
CIocpLinkItem* CIocpServer::CLinkMap::AllocLink()
{
	CIocpLinkItem* pLink = oTable.Pop();
	if(pLink && oTable.GetSize() == 0)
		oEvent.Reset();
	return pLink;
}

void CIocpServer::CLinkMap::PushLink(CIocpLinkItem* pLink)
{
	if(oTable.GetSize() == 0)
		oEvent.Set();
	oTable.Push(pLink);
}

void CIocpServer::CLinkMap::RemoveLink(CIocpLinkItem* pLink)
{
	oTable.Remove(pLink);
	if(oTable.GetSize() == 0)
		oEvent.Reset();
}

#else
void CIocpServer::CLinkMap::PushLink(ulong hFile, CIocpLinkItem* pLink)
{
	oTable[hFile] = pLink;
}

void CIocpServer::CLinkMap::RemoveLink(ulong hFile)
{
	oTable.Remove(hFile);
}

CIocpLinkItem* CIocpServer::CLinkMap::GetLinkItem(int32 hFile)
{
	CIocpLinkItem* pLink = NULL;
	CRbTreeNode* pIt = oTable.Find(hFile);
	if(pIt != oTable.End())
		pLink = oTable.GetItem(pIt);
	return pLink;
}

#endif

CIocpServer::CIocpServer():m_oLinkMap(this)
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	m_hIocpHandle = 0;
#else
	m_hIocpHandle = (ulong)(-1);
#endif
	m_nMaxLink = 0;
	m_pLinks = NULL;
}

CIocpServer::~CIocpServer()
{
	Cleanup();
}

CIocpServer* CIocpServer::GetInstance()
{
	return CSingleInstance<CIocpServer>::GetInstance();
}

bool CIocpServer::Initialize(uint16 nMaxLink)
{
	if(!nMaxLink)
		nMaxLink = 100;
	m_nMaxLink = nMaxLink;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	m_hIocpHandle = (ulong)CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if(m_hIocpHandle == 0)
	{
		FocpError(("%s", "Initialize complete port failure"));
		return false;
	}
#else
	m_hIocpHandle = (ulong)epoll_create(m_nMaxLink);
	if(m_hIocpHandle == (ulong)(-1))
	{
		FocpError(("%s", "Initialize complete port failure"));
		return false;
	}
#endif
	m_pLinks = new CIocpLinkItem[m_nMaxLink];
	for(uint32 i=0; i<m_nMaxLink; ++i)
		m_oContainer.FreeLink(m_pLinks+i);
	return true;
}

void CIocpServer::Cleanup()
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(m_hIocpHandle)
	{
		CloseHandle((HANDLE)m_hIocpHandle);
		m_hIocpHandle = 0;
	}
#else
	if(m_hIocpHandle != (ulong)(-1))
	{
		close((int32)m_hIocpHandle);
		m_hIocpHandle = (ulong)(-1);
	}
#endif
	m_oContainer.Clear();
	m_oLinkMap.Clear();
	if(m_pLinks)
	{
		delete[] m_pLinks;
		m_pLinks = NULL;
		m_nMaxLink = 0;
	}
}

#if defined(WINDOWS) || defined(CYGWIN_NT)
void CIocpServer::Start()
{
	m_oLinkMap.oThread.Start();
}

void CIocpServer::Stop(bool bBlock)
{
	m_oLinkMap.oThread.Stop(bBlock);
}

#endif

uint32 CIocpServer::AllocLink()
{
	uint32 nRet = 0xFFFFFFFF;
	m_oContainer.oEvent.Wait(1000);
	m_oMutex.Enter();
	CIocpLinkItem* pLink = m_oContainer.AllocLink();
	if(pLink)
	{
		uint32 nIdx = pLink - m_pLinks;
		++pLink->nCounter;
		pLink->nStatus = 1;
		nRet = (nIdx << 16)|pLink->nCounter;
	}
	m_oMutex.Leave();
	return nRet;
}

#if defined(WINDOWS) || defined(CYGWIN_NT)

typedef BOOL (PASCAL FAR * LPFN_ACCEPTEX)(
	IN SOCKET sListenSocket,
	IN SOCKET sAcceptSocket,
	IN PVOID lpOutputBuffer,
	IN DWORD dwReceiveDataLength,
	IN DWORD dwLocalAddressLength,
	IN DWORD dwRemoteAddressLength,
	OUT LPDWORD lpdwBytesReceived,
	IN LPOVERLAPPED lpOverlapped
);
#define WSAID_ACCEPTEX {0xb5367df1,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

typedef VOID (PASCAL FAR * LPFN_GETACCEPTEXSOCKADDRS)(
	IN PVOID lpOutputBuffer,
	IN DWORD dwReceiveDataLength,
	IN DWORD dwLocalAddressLength,
	IN DWORD dwRemoteAddressLength,
	OUT struct sockaddr **LocalSockaddr,
	OUT LPINT LocalSockaddrLength,
	OUT struct sockaddr **RemoteSockaddr,
	OUT LPINT RemoteSockaddrLength
);
#define WSAID_GETACCEPTEXSOCKADDRS {0xb5367df2,0xcbac,0x11cf,{0x95,0xca,0x00,0x80,0x5f,0x48,0xa1,0x92}}

CMutex g_oAcceptMutex;
static bool g_bAccept = false;
static LPFN_ACCEPTEX AcceptEx = NULL;
static LPFN_GETACCEPTEXSOCKADDRS GetAcceptSockAddr = NULL;

struct  CIocpAcceptOvl
{
	uint32 bAccept;//1，设置为1表示做Accept操作
	OVERLAPPED oOverlapped;
	char sBuf[2*(sizeof(sockaddr_in)+16)];
	ulong hHandle;
	CIpAddr* pAddr;
	CEvent oEvent;

	CIocpAcceptOvl(CIpAddr * aAddr)
	{
		bAccept = 1;
		hHandle = (ulong)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		CBinary::MemorySet(&oOverlapped, 0, sizeof(OVERLAPPED));
		pAddr = aAddr;
	}

	~CIocpAcceptOvl()
	{
		if(hHandle != (ulong)(-1))
		{
			closesocket((SOCKET)hHandle);
			hHandle = (ulong)(-1);
		}
	}
};

void CIocpServer::Listen(ulong hHandle)
{
	CreateIoCompletionPort((HANDLE)hHandle, (HANDLE)m_hIocpHandle, hHandle, 0);
}

static void DoAccept(CIocpAcceptOvl* pOverlap)
{
	sockaddr_in * pClientAddr = NULL;
	sockaddr_in * pLocalAddr = NULL;
	int32 nRemoteLen=sizeof(sockaddr_in), nLocalLen = sizeof(sockaddr_in);
	GetAcceptSockAddr(pOverlap->sBuf, 0, sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16,
					  (LPSOCKADDR*)&pLocalAddr, &nLocalLen, (LPSOCKADDR*)&pClientAddr, &nRemoteLen);
	pOverlap->pAddr->nPort = CBinary::U16Code(pClientAddr->sin_port);
	pOverlap->pAddr->nAddr = pClientAddr->sin_addr.s_addr;
}

ulong CIocpServer::Accept(ulong hHandle, CIpAddr &oAddr)
{
	if(!g_bAccept)
	{
		g_oAcceptMutex.Enter();
		if(!g_bAccept)
		{
			uint32 nRecvBytes = 0;
			GUID oAcceptGuid = WSAID_ACCEPTEX;
			if(SOCKET_ERROR == WSAIoctl((SOCKET)hHandle, SIO_GET_EXTENSION_FUNCTION_POINTER,
										&oAcceptGuid, sizeof(oAcceptGuid), &AcceptEx, sizeof(AcceptEx), (DWORD*)&nRecvBytes, NULL, NULL))
			{
				g_oAcceptMutex.Leave();
				return (ulong)(-1);
			}
			nRecvBytes = 0;
			GUID oGetAddrGuid = WSAID_GETACCEPTEXSOCKADDRS;
			if(SOCKET_ERROR == WSAIoctl((SOCKET)hHandle, SIO_GET_EXTENSION_FUNCTION_POINTER,
										&oGetAddrGuid, sizeof(oGetAddrGuid), &GetAcceptSockAddr, sizeof(GetAcceptSockAddr), (DWORD*)&nRecvBytes, NULL, NULL))
			{
				g_oAcceptMutex.Leave();
				return (ulong)(-1);
			}
			g_bAccept = true;
		}
		g_oAcceptMutex.Leave();
	}
	oAddr.nAddr = 0;
	oAddr.nPort = 0;
	CIocpAcceptOvl oOvl(&oAddr);
	uint32 nRecvBytes = 0;
	if(AcceptEx((SOCKET)hHandle, (SOCKET)oOvl.hHandle, oOvl.sBuf, 0, sizeof(sockaddr_in)+16, sizeof(sockaddr_in)+16, (DWORD*)&nRecvBytes, &oOvl.oOverlapped))
		DoAccept(&oOvl);
	else if(WSAGetLastError() == WSA_IO_PENDING)
		oOvl.oEvent.Wait();
	if(oAddr.nAddr)
	{
		hHandle = oOvl.hHandle;
		oOvl.hHandle = (ulong)(-1);
		return hHandle;
	}
	return (ulong)(-1);
}

#endif


bool CIocpServer::BindLink(uint32 hLink, ulong hHandle, CStreamAssembler* pAssembler)
{
	if(hHandle == (ulong)(-1) || !pAssembler)
		return false;
	uint16 nCounter = (uint16)hLink;
	uint32 nIdx = hLink >> 16;
	if(nIdx >= m_nMaxLink)
		return false;
	CIocpLinkItem* pLink = m_pLinks + nIdx;
	m_oMutex.Enter();
	if(pLink->nCounter != nCounter || pLink->nStatus != 1)
	{
		m_oMutex.Leave();
		return false;
	}
	pLink->hFile = hHandle;
	pLink->pAssembler = pAssembler;
	pLink->nStatus = 2;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	m_oLinkMap.PushLink(pLink);
	CreateIoCompletionPort((HANDLE)hHandle, (HANDLE)m_hIocpHandle, hHandle, 0);
#else
	epoll_event oOverlapped;
	CBinary::MemorySet(&oOverlapped, 0, sizeof(oOverlapped));
	oOverlapped.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;// | EPOLLET;
	oOverlapped.data.fd = (int32)hHandle;
//	fcntl((int32)m_hIocpHandle, F_SETFL, fcntl((int32)hHandle, F_GETFL, 0) | O_NONBLOCK);
	epoll_ctl((int32)m_hIocpHandle, EPOLL_CTL_ADD, (int32)hHandle, &oOverlapped);
	m_oLinkMap.PushLink(hHandle, pLink);
#endif
	m_oMutex.Leave();
	return true;
}

int32 CIocpServer::SendTo(uint32 hLink, const void* pBuf, int32 nBufLen)
{
	if(pBuf == NULL)
		return 0;
	if(nBufLen <= 0)
		return nBufLen;
	uint16 nCounter = (uint16)hLink;
	uint32 nIdx = hLink >> 16;
	if(nIdx >= m_nMaxLink)
		return -1;
	CIocpLinkItem* pLink = m_pLinks + nIdx;
	m_oMutex.Enter();
	if(pLink->nCounter != nCounter || pLink->nStatus < 2)
	{
		m_oMutex.Leave();
		return -1;
	}
	ulong hFile = pLink->hFile;
	m_oMutex.Leave();
	if(hFile == (ulong)(-1))
		return -1;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	int32 nRet = send((SOCKET)hFile, (const char*)pBuf, nBufLen, 0);
#else
	int32 nRet = send((int32)hFile, (const char*)pBuf, nBufLen, 0);
#endif
	return nRet;
}

CStreamAssembler* CIocpServer::GetAssember(uint32 hLink)
{
	uint16 nCounter = (uint16)hLink;
	uint32 nIdx = hLink >> 16;
	if(nIdx >= m_nMaxLink)
		return NULL;
	CIocpLinkItem* pLink = m_pLinks + nIdx;
	m_oMutex.Enter();
	if(pLink->nCounter != nCounter || !pLink->nStatus)
	{
		m_oMutex.Leave();
		return NULL;
	}
	CStreamAssembler* Assembler = pLink->pAssembler;
	m_oMutex.Leave();
	return Assembler;
}

void CIocpServer::FreeLink(uint32 hLink)
{
	uint16 nCounter = (uint16)hLink;
	uint32 nIdx = hLink >> 16;
	if(nIdx >= m_nMaxLink)
		return;
	CIocpLinkItem* pLink = m_pLinks + nIdx;
	m_oMutex.Enter();
	if(pLink->nCounter != nCounter || !pLink->nStatus)
	{
		m_oMutex.Leave();
		return;
	}
#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(pLink->nStatus == 3)
	{
		closesocket((SOCKET)pLink->hFile);
		pLink->hFile = (ulong)(-1);
		CStreamAssembler* pAssembler = pLink->pAssembler;
		pLink->pAssembler = NULL;
		m_oMutex.Leave();
		if(pAssembler)
			pAssembler->UnBindLink();
		return;
	}
#endif
	CStreamAssembler* pAssembler = CloseLink(pLink);
	m_oMutex.Leave();
	if(pAssembler)
		pAssembler->UnBindLink();
}

#if defined(WINDOWS) || defined(CYGWIN_NT)
static uint32 g_nExitCode;
#endif

void CIocpServer::StopNotify(CCooperator* pCooperator)
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(pCooperator != &m_oLinkMap.oThread)
		PostQueuedCompletionStatus((HANDLE)m_hIocpHandle, 0, (ULONG_PTR)&g_nExitCode, NULL);
#else
	pCooperator->Kill(SIGUSR1);//发送一个激活的信号，以便让epoll_wait线程能获得CPU.
#endif
}

void CIocpServer::MainProc(CCooperator* pCooperator, bool &bRunning)
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
#else
	CCooperator::OpenSignal(SIGUSR1);
#endif
	while(bRunning)
		ProcessOnce(pCooperator, bRunning);
}

void CIocpServer::ProcessOnce(CCooperator* pCooperator, bool &bRunning)
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(pCooperator == &m_oLinkMap.oThread)
	{
		m_oLinkMap.oEvent.Wait(1000);

		m_oMutex.Enter();
		CIocpLinkItem* pLink = m_oLinkMap.AllocLink();
		if(pLink)
			pLink->nStatus = 3;
		m_oMutex.Leave();
		if(pLink == NULL)
			return;

		uint32 nFlags = 0;
		uint32 nRecvBytes = 0;
		CBinary::MemorySet(&pLink->oOverlapped, 0, sizeof(OVERLAPPED));
		pLink->oDataBuf.len = 1024;
		pLink->oDataBuf.buf = pLink->sBuf;
		if(WSARecv((SOCKET)pLink->hFile, &pLink->oDataBuf, 1, (DWORD*)&nRecvBytes,
				   (DWORD*)&nFlags, &pLink->oOverlapped, NULL) == SOCKET_ERROR)
		{
			if(WSA_IO_PENDING == WSAGetLastError())
				return;
			CStreamAssembler* pAssembler = NULL;
			m_oMutex.Enter();
			pAssembler = CloseLink(pLink);
			m_oMutex.Leave();
			if(pAssembler)
				pAssembler->UnBindLink();
		}
		else
		{
			CStreamAssembler* pAssembler = NULL;
			m_oMutex.Enter();
			if(nRecvBytes == 0)
				pAssembler = CloseLink(pLink);
			else
				pAssembler = ReadLink(pLink, nRecvBytes);
			m_oMutex.Leave();
			if(pAssembler)
				pAssembler->UnBindLink();
		}
	}
	else
	{
		uint32 nBytes = 0;
		ULONG_PTR pKey = 0;
		LPOVERLAPPED pOverlapped;
		m_oIocpMutex.Enter();
		if(bRunning == false)
		{
			m_oIocpMutex.Leave();
			return;
		}
		BOOL bResult = GetQueuedCompletionStatus((HANDLE)m_hIocpHandle, (DWORD*)&nBytes, &pKey, &pOverlapped, INFINITE);
		m_oIocpMutex.Leave();
		if(pKey == (ULONG_PTR)&g_nExitCode)
		{
			bRunning = false;
			return;
		}
		if(pOverlapped == NULL)
			return;
		CIocpLinkItem* pLink = (CIocpLinkItem*)((char*)pOverlapped - FocpFieldOffset(CIocpLinkItem, oOverlapped));
		if(pLink->bAccept)
		{
			CIocpAcceptOvl *pAccept = (CIocpAcceptOvl*)(void*)pLink;
			if(bResult)
				DoAccept(pAccept);
			pAccept->oEvent.Set();
		}
		else
		{
			CStreamAssembler* pAssembler = NULL;
			m_oMutex.Enter();
			if(!bResult || !nBytes)
				pAssembler = CloseLink(pLink);
			else
				pAssembler = ReadLink(pLink, nBytes);
			m_oMutex.Leave();
			if(pAssembler)
				pAssembler->UnBindLink();
		}
	}
#else
	epoll_event oEvents[500];
	m_oIocpMutex.Enter();
	if(bRunning == false)
	{
		m_oIocpMutex.Leave();
		return;
	}
	int32 nEventCount = epoll_wait((int32)m_hIocpHandle, oEvents, 500, 5000);
	m_oIocpMutex.Leave();
	for(int32 i=0; i<nEventCount; ++i)
	{
		CIocpLinkItem* pLink = NULL;
		CStreamAssembler* pAssembler = NULL;
		if(oEvents[i].events & EPOLLIN)
		{
			m_oMutex.Enter();
			pLink = m_oLinkMap.GetLinkItem(oEvents[i].data.fd);
			if(pLink && pLink->nStatus == 2)
			{
				int32 nBytes = recv(oEvents[i].data.fd, pLink->sBuf, 1024, 0);
				if(nBytes > 0)
					pAssembler = ReadLink(pLink, nBytes);
				else
					pAssembler = CloseLink(pLink);
			}
			m_oMutex.Leave();
		}
		else
		{
			m_oMutex.Enter();
			pLink = m_oLinkMap.GetLinkItem(oEvents[i].data.fd);
			if(pLink && pLink->nStatus == 2)
				pAssembler = CloseLink(pLink);
			m_oMutex.Leave();
		}
		if(pLink == NULL)
		{
			/*
						epoll_event oOverlapped;
						CBinary::MemorySet(&oOverlapped, 0, sizeof(oOverlapped));
						oOverlapped.data.fd = oEvents[i].data.fd;
						oOverlapped.events = 0;
			*/
			epoll_ctl((int32)m_hIocpHandle, EPOLL_CTL_DEL, oEvents[i].data.fd, NULL);//&oOverlapped);
		}
		else
		{
			epoll_event oOverlapped;
			CBinary::MemorySet(&oOverlapped, 0, sizeof(oOverlapped));
			oOverlapped.events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT;// | EPOLLET;
			oOverlapped.data.fd = oEvents[i].data.fd;
			//	fcntl((int32)m_hIocpHandle, F_SETFL, fcntl((int32)hHandle, F_GETFL, 0) | O_NONBLOCK);
			epoll_ctl((int32)m_hIocpHandle, EPOLL_CTL_MOD, oEvents[i].data.fd, &oOverlapped);
		}
		if(pAssembler)
			pAssembler->UnBindLink();
	}
	return;
#endif
}

CStreamAssembler* CIocpServer::CloseLink(CIocpLinkItem* pLink)
{
	if(pLink->nStatus == 2)
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		m_oLinkMap.RemoveLink(pLink);
#else
		m_oLinkMap.RemoveLink(pLink->hFile);
#endif
	}
	if(pLink->hFile != (ulong)(-1))
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		closesocket((SOCKET)pLink->hFile);
#else
		epoll_event oOverlapped;
		CBinary::MemorySet(&oOverlapped, 0, sizeof(oOverlapped));
		oOverlapped.data.fd = (int32)pLink->hFile;
		oOverlapped.events = 0;
		epoll_ctl((int32)m_hIocpHandle, EPOLL_CTL_DEL, (int32)pLink->hFile, &oOverlapped);
		shutdown((int32)pLink->hFile, SHUT_RDWR);
		close((int32)pLink->hFile);
#endif
		pLink->hFile = (ulong)(-1);
	}
	CStreamAssembler* pAssembler = pLink->pAssembler;
	pLink->pAssembler = NULL;
	pLink->nStatus = 0;
	m_oContainer.FreeLink(pLink);
	return pAssembler;
}

CStreamAssembler* CIocpServer::ReadLink(CIocpLinkItem* pLink, uint32 nBytes)
{
	CMemoryStream oMsg;
	char * sBuf = pLink->sBuf;
	CStreamAssembler* pAssembler;
	while(nBytes)
	{
		int32 nLen = pLink->pAssembler->OnAssembler(sBuf, nBytes, oMsg);
		uint32 nSize = oMsg.GetSize();
		if(nSize)
		{
			uint32 hLink = pLink - m_pLinks;
			hLink <<= 16;
			hLink |= pLink->nCounter;
			PushMsg(hLink, oMsg);
		}
		else if(nLen)
			break;
		sBuf += nBytes - nLen;
		nBytes = nLen;
	}
	if(nBytes)
		pAssembler = CloseLink(pLink);
	else
	{
		pAssembler = NULL;
#if defined(WINDOWS) || defined(CYGWIN_NT)
		pLink->nStatus = 2;
		m_oLinkMap.PushLink(pLink);
#endif
	}
	return pAssembler;
}

FOCP_END();
