
#include "TcpFile.hpp"

FOCP_BEGIN();

static CTcpFileInterface g_oTcpFileInterface;

const char* CTcpFileInterface::GetProtocol()
{
	return "tcp";
}

CBaseFile* CTcpFileInterface::CreateFile()
{
	return new CTcpFile;
}

void CTcpFileInterface::DestroyFile(CBaseFile* pFile)
{
	if(pFile)
		delete pFile;
}

FOCP_END();

#if defined(WINDOWS) || defined(CYGWIN_NT)
	#include <winsock2.h>
	#include <mswsock.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <errno.h>
#endif

FOCP_BEGIN();

CTcpFile::CTcpFile()
{
	m_hHandle = (ulong)(-1);
	m_oFileName.oProtocol = "tcp";
	m_bListened = false;
}

CTcpFile::~CTcpFile()
{
	Close();
}

CFileInterface* CTcpFile::GetInterface()
{
	return &g_oTcpFileInterface;
}

int32 CTcpFile::Open(const CFileName& oFileName, const char* sOption)
{
	CIpAddr oIpAddr;
	CIpAddrList oBindAddrList;
	CIpAddrList oConnectAddrList;
	uint32 i, nListenCount;

	uint32 nOption = GetOpenOption(sOption, &nListenCount);

	if(!(nOption & FOCP_FILE_OPTION_LISTEN) && !(nOption & FOCP_FILE_OPTION_READ) && !(nOption & FOCP_FILE_OPTION_WRITE))
		return FOCP_FILE_OPTION_ERROR;

	if(nOption & (FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_DESTROY|FOCP_FILE_OPTION_APPEND|FOCP_FILE_OPTION_NEW))
		return FOCP_FILE_OPTION_ERROR;

	uint32 nBindSize=0;

	if(oFileName.oBindName.Empty())
	{
		if(nOption & FOCP_FILE_OPTION_LISTEN)
			return FOCP_FILE_BINDNAME_ERROR;
	}
	else
	{
		if(!CFile::GetIpAddrList(oFileName.oBindName.GetStr(), oBindAddrList))
			return FOCP_FILE_BINDNAME_ERROR;
		nBindSize = oBindAddrList.oAddrList.GetSize();
		if(!nBindSize)
			return FOCP_FILE_BINDNAME_ERROR;
	}

	uint32 nConnectSize=0;
	if(oFileName.oConnectName.Empty())
	{
		if(!(nOption & FOCP_FILE_OPTION_LISTEN))
			return FOCP_FILE_CONNECTNAME_ERROR;
	}
	else if(nOption & FOCP_FILE_OPTION_LISTEN)
		return FOCP_FILE_CONNECTNAME_ERROR;
	else
	{
		if(!CFile::GetIpAddrList(oFileName.oConnectName.GetStr(), oConnectAddrList))
			return FOCP_FILE_CONNECTNAME_ERROR;
		nConnectSize = oConnectAddrList.oAddrList.GetSize();
		if(!nConnectSize)
			return FOCP_FILE_CONNECTNAME_ERROR;
	}

#if defined(WINDOWS) || defined(CYGWIN_NT)
	m_hHandle = (ulong)WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
	m_hHandle = (ulong)socket(AF_INET, SOCK_STREAM, 0);
#endif
	if(m_hHandle == (ulong)(-1))
		return FOCP_FILE_OTHER_ERROR;
	sockaddr_in oBindAddr;
	if(nBindSize)
	{
		i=1;
		oBindAddr.sin_family = AF_INET;
		oBindAddr.sin_port = CBinary::U16Code(oBindAddrList.nPort);
#if defined(WINDOWS) || defined(CYGWIN_NT)
		setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_REUSEADDR, (char *)(&i), sizeof(i));
#else
		setsockopt((int32)m_hHandle, SOL_SOCKET, SO_REUSEADDR, (char *)(&i), sizeof(i));
#endif
		for(i=0; i<nBindSize; ++i)
		{
			oBindAddr.sin_addr.s_addr = oBindAddrList.oAddrList[i];
#if defined(WINDOWS) || defined(CYGWIN_NT)
			int32 nRet = bind((SOCKET)m_hHandle, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr));
#else
			int32 nRet = bind((int32)m_hHandle, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr));
#endif
			if(nRet == 0)
				break;
		}
		if(i>=nBindSize)
		{
#if defined(WINDOWS) || defined(CYGWIN_NT)
			closesocket((SOCKET)m_hHandle);
#else
			close((int32)m_hHandle);
#endif
			m_hHandle = (ulong)(-1);
			return FOCP_FILE_BIND_ERROR;
		}
		if(nOption & FOCP_FILE_OPTION_LISTEN)
		{
#if defined(WINDOWS) || defined(CYGWIN_NT)
			if(listen((SOCKET)m_hHandle, nListenCount))
#else
			if(listen((int32)m_hHandle, nListenCount))
#endif
			{
#if defined(WINDOWS) || defined(CYGWIN_NT)
				closesocket((SOCKET)m_hHandle);
#else
				close((int32)m_hHandle);
#endif
				m_hHandle = (ulong)(-1);
				return FOCP_FILE_LISTEN_ERROR;
			}
			m_bListened = true;
#if defined(WINDOWS) || defined(CYGWIN_NT)
			CIocpServer::GetInstance()->Listen(m_hHandle);
#endif
		}
	}
	if(nConnectSize)
	{
		sockaddr_in oConnAddr;
		oConnAddr.sin_family = AF_INET;
		oConnAddr.sin_port = CBinary::U16Code(oConnectAddrList.nPort);
		oIpAddr.nPort = oConnectAddrList.nPort;
		for(i=0; i<nConnectSize; ++i)
		{
			int32 nRet;
			oConnAddr.sin_addr.s_addr = oConnectAddrList.oAddrList[i];
#if defined(WINDOWS) || defined(CYGWIN_NT)
			nRet = connect((SOCKET)m_hHandle, (const struct sockaddr*)(void*)&oConnAddr, sizeof(oConnAddr));
#else
			nRet = connect((int32)m_hHandle, (const struct sockaddr*)(void*)&oConnAddr, sizeof(oConnAddr));
#endif
			if(nRet == 0)
				break;
		}
		if(i>=nConnectSize)
		{
			Close();
			return FOCP_FILE_CONNECT_ERROR;
		}
		m_oFileName.oConnectName = oFileName.oConnectName;
	}
	if(nBindSize)
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		int32 nAddrLen = sizeof(oBindAddr);
		getsockname((SOCKET)m_hHandle, (struct sockaddr*)(void*)&oBindAddr, &nAddrLen);
#else
		socklen_t nAddrLen = sizeof(oBindAddr);
		getsockname((int32)m_hHandle, (struct sockaddr*)(void*)&oBindAddr, &nAddrLen);
#endif
		oIpAddr.nAddr = oBindAddr.sin_addr.s_addr;
		oIpAddr.nPort = CBinary::U16Code(oBindAddr.sin_port);
		CFile::GetIpFileName(oIpAddr, m_oFileName.oBindName);
	}
	SetStatus(FOCP_FILE_NORMAL);
	return 0;
}

void CTcpFile::Close(ulong *pHandle)
{
	if(pHandle)
		pHandle[0] = m_hHandle;
	if(m_hHandle != (ulong)(-1))
	{
		if(pHandle == NULL)
		{
#if defined(WINDOWS) || defined(CYGWIN_NT)
			shutdown((int32)m_hHandle, SD_BOTH);
			closesocket((SOCKET)m_hHandle);
#else
			shutdown((int32)m_hHandle, SHUT_RDWR);
			close((int32)m_hHandle);
#endif
		}
		m_hHandle = (ulong)(-1);
		SetStatus(FOCP_FILE_CLOSED);
		m_bListened = false;
	}
}

bool CTcpFile::Accept(CFile &oFile)
{
	CIpAddr oIpAddr;
	if(m_hHandle == (ulong)(-1))
		return false;
	if(m_bListened == false)
		return false;
	CTcpFile* pFile = new CTcpFile;

#if defined(WINDOWS) || defined(CYGWIN_NT)
	pFile->m_hHandle = CIocpServer::GetInstance()->Accept(m_hHandle, oIpAddr);
#else
loop:
	sockaddr_in oFromAddr;
	socklen_t nSockLen = sizeof(oFromAddr);
	pFile->m_hHandle = (ulong)accept((int32)m_hHandle, (sockaddr*)(void*)&oFromAddr, &nSockLen);
	if(pFile->m_hHandle == (ulong)(-1) && errno == EINTR)
		goto loop;
#endif
	if(pFile->m_hHandle == (ulong)(-1))
	{
		SetStatus(FOCP_FILE_BAD);
		delete pFile;
		return false;
	}
#if defined(WINDOWS) || defined(CYGWIN_NT)
#else
	oIpAddr.nAddr = oFromAddr.sin_addr.s_addr;
	oIpAddr.nPort = CBinary::U16Code(oFromAddr.sin_port);
#endif
	CFile::GetIpFileName(oIpAddr, pFile->m_oFileName.oConnectName);
	pFile->SetStatus(FOCP_FILE_NORMAL);
	oFile.Redirect(pFile, true);
	return true;
}

uint32 CTcpFile::GetType()
{
	if(m_hHandle == (ulong)(-1))
		return FOCP_INVALID_FILE;
	if(m_bListened)
		return FOCP_LISTEN_FILE|FOCP_STREAM_FILE;
	else
		return FOCP_CONNECT_FILE|FOCP_STREAM_FILE;
}

const CFileName& CTcpFile::GetFileName()
{
	return m_oFileName;
}

int32 CTcpFile::Read(void* pBuf, int32 nBufLen, uint32 nTimeOut)
{
	if(m_hHandle == (ulong)(-1))
		return -1;
	if(m_bListened)
		return 0;
	if(nTimeOut != 0xFFFFFFFF)
	{
		fd_set rset;
		struct timeval tm;
		tm.tv_sec = nTimeOut/1000;
		tm.tv_usec = (nTimeOut%1000)*1000;
		FD_ZERO(&rset);
#if defined(WINDOWS) || defined(CYGWIN_NT)
		FD_SET((SOCKET)m_hHandle, &rset);
		if(select((SOCKET)m_hHandle+1, &rset, NULL, NULL, &tm))
			return -2;
#else
		FD_SET((int32)m_hHandle, &rset);
		if(!select((int32)m_hHandle+1, &rset, NULL, NULL, &tm))
			return -2;
#endif
	}
	int32 nRet;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = recv((SOCKET)m_hHandle, (char*)pBuf, nBufLen, 0);
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
#else
loop:
	nRet = recv((int32)m_hHandle, pBuf, nBufLen, 0);
	if(nRet < 0)
	{
		if(errno == EINTR)
			goto loop;
		SetStatus(FOCP_FILE_BAD);
	}
#endif
	if(nRet == 0)
		SetStatus(FOCP_FILE_BROKEN);
	return nRet;
}

int32 CTcpFile::Write(const void* pBuf, int32 nBufLen)
{
	if(m_hHandle == (ulong)(-1))
		return -1;
	if(m_bListened)
		return 0;
	int32 nRet;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = send((SOCKET)m_hHandle, (const char*)pBuf, nBufLen, 0);
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
#else
loop:
	nRet = send((int32)m_hHandle, pBuf, nBufLen, 0);
	if(nRet < 0)
	{
		if(errno == EINTR)
			goto loop;
		SetStatus(FOCP_FILE_BAD);
	}
#endif
	return nRet;
}

void CTcpFile::SetBuffer(uint32 nBufSize, uint32 nBufType)
{
	if(nBufType & FOCP_READ_BUFFER)
#if defined(WINDOWS) || defined(CYGWIN_NT)
		setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_RCVBUF, (char *)&nBufSize, sizeof(nBufSize));
#else
		setsockopt((int32)m_hHandle, SOL_SOCKET, SO_RCVBUF, (char *)&nBufSize, sizeof(nBufSize));
#endif

	if(nBufType & FOCP_WRITE_BUFFER)
#if defined(WINDOWS) || defined(CYGWIN_NT)
		setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, sizeof(nBufSize));
#else
		setsockopt((int32)m_hHandle, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, sizeof(nBufSize));
#endif
}

FOCP_END();
