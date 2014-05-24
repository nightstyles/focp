
#include "UdpFile.hpp"

FOCP_BEGIN();

static CUdpFileInterface g_oUdpFileInterface;

const char* CUdpFileInterface::GetProtocol()
{
	return "udp";
}

CBaseFile* CUdpFileInterface::CreateFile()
{
	return new CUdpFile;
}

void CUdpFileInterface::DestroyFile(CBaseFile* pFile)
{
	if(pFile)
		delete pFile;
}

FOCP_END();

#if defined(WINDOWS) || defined(CYGWIN_NT)
	#include <winsock2.h>
#else
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <errno.h>
	#include <netdb.h>
#endif

FOCP_BEGIN();

CUdpFile::CUdpFile()
{
	m_hHandle = (ulong)(-1);
	m_oFileName.oProtocol = "udp";
	m_bConnected = false;
	m_bIsMultiCast = false;
}

CUdpFile::~CUdpFile()
{
}

CFileInterface* CUdpFile::GetInterface()
{
	return &g_oUdpFileInterface;
}

int32 CUdpFile::Open(const CFileName& oFileName, const char* sOption)
{
	CIpAddr oIpAddr;
	CIpAddrList oBindAddrList;
	CIpAddrList oConnectAddrList;
	uint32 nTTL;

	uint32 nOption = GetOpenOption(sOption, &nTTL);

	if(!(nOption & FOCP_FILE_OPTION_READ) && !(nOption & FOCP_FILE_OPTION_WRITE))
		return FOCP_FILE_OPTION_ERROR;

	if(nOption & (FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_DESTROY|FOCP_FILE_OPTION_APPEND|FOCP_FILE_OPTION_NEW))
		return FOCP_FILE_OPTION_ERROR;

	if(nOption & FOCP_FILE_OPTION_LISTEN)
		return FOCP_FILE_OPTION_ERROR;

	bool bAllowLoop = false;
	if(nOption & FOCP_FILE_OPTION_LOOP)
		bAllowLoop = true;

	if(!(nOption & FOCP_FILE_OPTION_TTL))
		nTTL = 1;

	uint32 nBindSize=0;
	oBindAddrList.nPort = 0;
	if(!oFileName.oBindName.Empty())
	{
		if(!CFile::GetIpAddrList(oFileName.oBindName.GetStr(), oBindAddrList))
			return FOCP_FILE_BINDNAME_ERROR;
		nBindSize = oBindAddrList.oAddrList.GetSize();
		if(!nBindSize)
			return FOCP_FILE_BINDNAME_ERROR;
	}

	uint32 nConnectSize=0;
	if(!oFileName.oConnectName.Empty())
	{
		if(!CFile::GetIpAddrList(oFileName.oConnectName.GetStr(), oConnectAddrList))
			return FOCP_FILE_CONNECTNAME_ERROR;
		nConnectSize = oConnectAddrList.oAddrList.GetSize();
		if(!nConnectSize)
			return FOCP_FILE_CONNECTNAME_ERROR;
		m_bIsMultiCast = CFile::IsMulticastAddr(oConnectAddrList.oAddrList[0]);
	}

#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(m_bIsMultiCast)
		m_hHandle = (ulong)WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF|WSA_FLAG_OVERLAPPED);
	else
		m_hHandle = (ulong)socket(AF_INET, SOCK_DGRAM, 0);
#else
	m_hHandle = (ulong)socket(AF_INET, SOCK_DGRAM, 0);
#endif

	if(m_hHandle == (ulong)(-1))
		return FOCP_FILE_OTHER_ERROR;
	if(m_bIsMultiCast)
	{
		uint32 nReuse = 1;
		sockaddr_in oConnAddr,oBindAddr;

		uint32 nHost = nBindSize?oBindAddrList.oAddrList[0]:INADDR_ANY;
		if(nHost == INADDR_ANY)
		{
			struct hostent * pHost =  gethostbyname(NULL);
			if(!pHost)
			{
#if defined(WINDOWS) || defined(CYGWIN_NT)
				closesocket((SOCKET)m_hHandle);
#else
				close((int32)m_hHandle);
#endif
				m_hHandle = (ulong)(-1);
				return FOCP_FILE_BIND_ERROR;
			}
			nHost = ((in_addr*)pHost->h_addr_list[0])->s_addr;
		}

		oConnAddr.sin_family = AF_INET;
		oConnAddr.sin_port = CBinary::U16Code(oConnectAddrList.nPort);
		oConnAddr.sin_addr.s_addr = oConnectAddrList.oAddrList[0];
		oBindAddr.sin_family = AF_INET;
		oBindAddr.sin_port = oConnAddr.sin_port;
		oBindAddr.sin_addr.s_addr = INADDR_ANY;//如果绑定nHost,SIO_MULTIPOINT_LOOPBACK能失效.

#if defined(WINDOWS) || defined(CYGWIN_NT)
		DWORD cbRet;
		char achOutbuf[32];
		int32 nRet = setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_REUSEADDR, (char *)(&nReuse), sizeof(nReuse));
		if(!nRet) nRet = bind((SOCKET)m_hHandle, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr));
		if(!nRet)
		{
			if(SOCKET_ERROR == WSAIoctl((SOCKET)m_hHandle, SIO_MULTICAST_SCOPE, &nTTL, sizeof(nTTL), achOutbuf,8,&cbRet,NULL,NULL))
				nRet = -1;
		}
		if(!nRet)
		{
			cbRet = 32;
			if(SOCKET_ERROR == WSAIoctl((SOCKET)m_hHandle, SIO_MULTIPOINT_LOOPBACK, &bAllowLoop, sizeof(bAllowLoop), achOutbuf,32,&cbRet,NULL,NULL))
				nRet = -1;
		}
		//windows缺接口地址设置
		if(!nRet && INVALID_SOCKET == WSAJoinLeaf((SOCKET)m_hHandle, (struct sockaddr *)(&oConnAddr), sizeof(struct sockaddr), NULL,NULL,NULL,NULL,JL_BOTH))
			nRet = -1;
#else
		struct ip_mreq oItfAddr;
		CBinary::MemorySet(&oItfAddr, 0, sizeof(oItfAddr));
		CBinary::MemoryCopy(&(oItfAddr.imr_multiaddr), &(oConnAddr.sin_addr), sizeof(in_addr));
		oItfAddr.imr_interface.s_addr = nHost;
		int32 nRet = setsockopt((int32)m_hHandle, SOL_SOCKET, SO_REUSEADDR, (char *)(&nReuse), sizeof(nReuse));
		if(!nRet) nRet = setsockopt((int32)m_hHandle, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&oItfAddr, sizeof(oItfAddr));
		if(!nRet) nRet = bind((int32)m_hHandle, (const struct sockaddr*)(void*)&oBindAddr, sizeof(oBindAddr));
		if(!nRet) nRet = setsockopt((int32)m_hHandle, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&nTTL, sizeof(nTTL));
		if(!nRet) nRet = setsockopt((int32)m_hHandle, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&bAllowLoop, sizeof(bAllowLoop));
#endif
		if(nRet)
		{
#if defined(WINDOWS) || defined(CYGWIN_NT)
			closesocket((SOCKET)m_hHandle);
#else
			close((int32)m_hHandle);
#endif
			m_hHandle = (ulong)(-1);
			return FOCP_FILE_BIND_ERROR;
		}
		oIpAddr.nAddr = nHost;
		oIpAddr.nPort = oBindAddrList.nPort;
		CFile::GetIpFileName(oIpAddr, m_oFileName.oBindName);
		oIpAddr.nAddr = oConnAddr.sin_addr.s_addr;
		oIpAddr.nPort = oConnectAddrList.nPort;
		CFile::GetIpFileName(oIpAddr, m_oFileName.oConnectName);
	}
	else
	{
		if(nBindSize)
		{
			uint32 i;
			sockaddr_in oBindAddr;
			CBinary::MemorySet(&oBindAddr, 0, sizeof(oBindAddr));
			oBindAddr.sin_family = AF_INET;
			oBindAddr.sin_port = CBinary::U16Code(oBindAddrList.nPort);
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
#if defined(WINDOWS) || defined(CYGWIN_NT)
			int32 nAddrLen = sizeof(oBindAddr);
			getsockname((SOCKET)m_hHandle, (struct sockaddr*)(void*)&oBindAddr, &nAddrLen);
#else
			socklen_t nAddrLen = sizeof(oBindAddr);
			getsockname((int32)m_hHandle, (struct sockaddr*)(void*)&oBindAddr, &nAddrLen);
#endif
			oIpAddr.nAddr = oBindAddr.sin_addr.s_addr;
			oIpAddr.nPort = oBindAddrList.nPort;
			CFile::GetIpFileName(oIpAddr, m_oFileName.oBindName);
		}
		if(nConnectSize)
		{
			uint32 i;
			sockaddr_in oConnAddr;
			CBinary::MemorySet(&oConnAddr, 0, sizeof(oConnAddr));
			oConnAddr.sin_family = AF_INET;
			oConnAddr.sin_port = CBinary::U16Code(oConnectAddrList.nPort);
			for(i=0; i<nConnectSize; ++i)
			{
				oConnAddr.sin_addr.s_addr = oConnectAddrList.oAddrList[i];
				//因为Udp不是真正的链接，所以不需要考虑异步的问题
#if defined(WINDOWS) || defined(CYGWIN_NT)
				int32 nRet = connect((SOCKET)m_hHandle, (const struct sockaddr*)(void*)&oConnAddr, sizeof(oConnAddr));
#else
				int32 nRet = connect((int32)m_hHandle, (const struct sockaddr*)(void*)&oConnAddr, sizeof(oConnAddr));
#endif
				if(nRet == 0)
				{
					m_bConnected = true;
					break;
				}
			}
			if(i>=nConnectSize)
			{
#if defined(WINDOWS) || defined(CYGWIN_NT)
				closesocket((SOCKET)m_hHandle);
#else
				close((int32)m_hHandle);
#endif
				m_hHandle = (ulong)(-1);
				return FOCP_FILE_CONNECT_ERROR;
			}
			oIpAddr.nAddr = oConnAddr.sin_addr.s_addr;
			oIpAddr.nPort = oConnectAddrList.nPort;
			CFile::GetIpFileName(oIpAddr, m_oFileName.oConnectName);
		}
	}
	SetStatus(FOCP_FILE_NORMAL);
	return 0;
}

void CUdpFile::Close(ulong *pHandle)
{
	if(pHandle)
		pHandle[0] = m_hHandle;
	if(m_hHandle != (ulong)(-1))
	{
		if(pHandle == NULL)
		{
#if defined(WINDOWS) || defined(CYGWIN_NT)
			shutdown((SOCKET)m_hHandle, SD_BOTH);
			closesocket((SOCKET)m_hHandle);
#else
			shutdown((int32)m_hHandle, SHUT_RDWR);
			close((int32)m_hHandle);
#endif
		}
		m_hHandle = (ulong)(-1);
		SetStatus(FOCP_FILE_CLOSED);
		m_bConnected = false;
	}
}

uint32 CUdpFile::GetType()
{
	if(m_hHandle == (ulong)(-1))
		return FOCP_INVALID_FILE;
	return FOCP_NONCONNECT_FILE | FOCP_PACKAGE_FILE;
}

const CFileName& CUdpFile::GetFileName()
{
	return m_oFileName;
}

int32 CUdpFile::Read(void* pBuf, int32 nBufLen, uint32 nTimeOut)
{
	if(m_hHandle == (ulong)(-1))
		return -1;
	if(!m_bConnected)
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
#else
		FD_SET((int32)m_hHandle, &rset);
		if(select((int32)m_hHandle+1, &rset, NULL, NULL, &tm))
#endif
			return -2;
	}
	int32 nRet;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = recv((SOCKET)m_hHandle, (char*)pBuf, nBufLen, 0);
#else
loop:
	nRet = recv((int32)m_hHandle, (char*)pBuf, nBufLen, 0);
	if(nRet < 0 && errno == EINTR)
		goto loop;
#endif
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
	return nRet;
}

int32 CUdpFile::Write(const void* pBuf, int32 nBufLen)
{
	if(m_hHandle == (ulong)(-1))
		return -1;
	if(!m_bConnected)
		return 0;
	int32 nRet;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = send((SOCKET)m_hHandle, (const char*)pBuf, nBufLen, 0);
#else
loop:
	nRet = send((int32)m_hHandle, (const char*)pBuf, nBufLen, 0);
	if(nRet < 0 && errno == EINTR)
		goto loop;
#endif
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
	return nRet;
}

int32 CUdpFile::ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut)
{
	int32 nRet;
	sockaddr_in oFrom;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	int32 nLen;
#else
	socklen_t nLen;
#endif

	if(m_hHandle == (ulong)(-1))
		return -1;

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
#else
		FD_SET((int32)m_hHandle, &rset);
		if(select((int32)m_hHandle+1, &rset, NULL, NULL, &tm))
#endif
			return -2;
	}
	nLen = sizeof(oFrom);
	CBinary::MemorySet(&oFrom, 0, sizeof(oFrom));
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = recvfrom((SOCKET)m_hHandle, (char*)pBuf, nBufLen, 0, (struct sockaddr*)(void*)&oFrom, &nLen);
#else
loop:
	nRet = recvfrom((int32)m_hHandle, (char*)pBuf, nBufLen, 0, (struct sockaddr*)(void*)&oFrom, &nLen);
	if(nRet < 0 && errno == EINTR)
		goto loop;
#endif
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
	else if(nRet > 0)
	{
		oIpAddr.nAddr = oFrom.sin_addr.s_addr;
		oIpAddr.nPort = CBinary::U16Code(oFrom.sin_port);
	}
	return nRet;
}

int32 CUdpFile::ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut)
{
	CIpAddr oIpAddr = {0, 0};
	int32 nRet = ReadFrom(pBuf, nBufLen, oIpAddr, nTimeOut);
	oFileName.oProtocol = "udp";
	oFileName.oBindName.Clear();
	CFile::GetIpFileName(oIpAddr, oFileName.oConnectName);
	return nRet;
}

int32 CUdpFile::WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName)
{
	if(m_hHandle == (ulong)(-1))
		return -1;
	if(oFileName.oProtocol.Compare("udp", false))
		return 0;
	if(oFileName.oConnectName.Empty())
		return 0;
	CIpAddrList oAddrList;
	if(!CFile::GetIpAddrList(oFileName.oConnectName.GetStr(), oAddrList))
		return 0;
	if(!oAddrList.oAddrList.GetSize())
		return 0;
	CIpAddr oIpAddr;
	oIpAddr.nAddr = oAddrList.oAddrList[0];
	oIpAddr.nPort = oAddrList.nPort;
	return WriteTo(pBuf, nBufLen, oIpAddr);
}

int32 CUdpFile::WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr)
{
	sockaddr_in oTo;
	CBinary::MemorySet(&oTo, 0, sizeof(oTo));
	oTo.sin_family = AF_INET;
	oTo.sin_port = CBinary::U16Code(oIpAddr.nPort);
	oTo.sin_addr.s_addr = oIpAddr.nAddr;
	int32 nRet;
#if defined(WINDOWS) || defined(CYGWIN_NT)
	nRet = sendto((SOCKET)m_hHandle, (const char*)pBuf, nBufLen, 0, (struct sockaddr*)(void*)&oTo, sizeof(oTo));
#else
loop:
	nRet = sendto((int32)m_hHandle, (const char*)pBuf, nBufLen, 0, (struct sockaddr*)(void*)&oTo, sizeof(oTo));
	if(nRet < 0 && errno == EINTR)
		goto loop;
#endif
	if(nRet < 0)
		SetStatus(FOCP_FILE_BAD);
	return nRet;
}

void CUdpFile::SetBuffer(uint32 nBufSize, uint32 nBufType)
{
#if defined(WINDOWS) || defined(CYGWIN_NT)
	if(nBufType & FOCP_READ_BUFFER)
		setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_RCVBUF, (char *)&nBufSize, sizeof(nBufSize));
	if(nBufType & FOCP_WRITE_BUFFER)
		setsockopt((SOCKET)m_hHandle, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, sizeof(nBufSize));
#else

	if(nBufType & FOCP_READ_BUFFER)
		setsockopt((int32)m_hHandle, SOL_SOCKET, SO_RCVBUF, (char *)&nBufSize, sizeof(nBufSize));
	if(nBufType & FOCP_WRITE_BUFFER)
		setsockopt((int32)m_hHandle, SOL_SOCKET, SO_SNDBUF, (char *)&nBufSize, sizeof(nBufSize));
#endif
}

FOCP_END();
