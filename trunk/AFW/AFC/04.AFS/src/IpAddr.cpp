
#include "IpAddr.hpp"

#ifndef WINDOWS
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#else
//	#include <winsock2.h>
//	#include <ws2tcpip.h>
//	#include <iphlpapi.h>
//	#include <stdio.h>
#endif

FOCP_BEGIN();

#ifdef LINUX

static bool GetIfMac(char* sIfName, int32 nSock, char sMac[])
{
	struct ifreq ifr;

	CBinary::MemorySet(&ifr, 0, sizeof(ifr));

	CString::StringCopy(ifr.ifr_name, sIfName);

	if (ioctl(nSock, SIOCGIFHWADDR, &ifr) < 0)
		return false;

	CBinary::MemoryCopy(sMac, &ifr.ifr_hwaddr.sa_data, 6);

	return 6;
}

static uint32 GetIfIndex(uint8 sMac[], int32 nSock, char sIfName[])
{
	uint32 nRet = (uint32)-1;

	int32 len, lastlen, position;
	char *buf, *cptr;
	struct ifconf ifc;
	struct ifreq *ifr;

	CIntegerSet<uint32> oSet(1, 65535);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	while(1)
	{
		buf = new char[len];
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(nSock, SIOCGIFCONF, &ifc) < 0)
		{
			if (errno != EINVAL || lastlen != 0)
			{
				delete[] buf;
				return nRet;
			}
		}
		else
		{
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		delete[] buf;
	}
	int n;
	ifr = ifc.ifc_req;
	sIfName[0] = '\0';
	for (n = ifc.ifc_len / sizeof (struct ifreq); --n >= 0; ifr++)
	{
		cptr = CString::CharOfString(ifr->ifr_name,':');
		if(cptr == NULL)
		{
			char sMac2[6];
			if(GetIfMac(ifr->ifr_name, nSock, sMac2) && !CBinary::MemoryCompare(sMac2, sMac, 6))
			{
				CString::StringCopy(sIfName, ifr->ifr_name);
				break;
			}
		}
	}
	if(!sIfName[0])
	{
		delete[] buf;
		return nRet;
	}
	for (n = ifc.ifc_len / sizeof (struct ifreq); --n >= 0; ifr++)
	{
		cptr = CString::CharOfString(ifr->ifr_name,':');
		if(cptr)
		{
			*cptr=0;
			if(CString::StringCompare(ifr->ifr_name, sIfName)==0 && cptr[1])
			{
				position = CString::Atoi(cptr+1);
				oSet.Set((uint32)position);
			}
		}
	}
	delete[] buf;
	if(!oSet.Alloc(nRet))
		nRet = (uint32)(-1);
	return nRet;
}

static bool GetIfName(uint32 nIpAddr, int32 nSock, char* sIfName)
{
	int32 len, lastlen;
	char *buf;
	struct ifconf ifc;
	struct ifreq *ifr;

	sIfName[0] = '\0';
	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	while(1)
	{
		buf = new char[len];
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(nSock, SIOCGIFCONF, &ifc) < 0)
		{
			if (errno != EINVAL || lastlen != 0)
			{
				delete[] buf;
				return false;
			}
		}
		else
		{
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		delete[] buf;
	}
	int n;
	ifr = ifc.ifc_req;
	for (n = ifc.ifc_len / sizeof (struct ifreq); --n >= 0; ifr++)
	{
		if(nIpAddr == *(uint32*)&((struct sockaddr_in*)&ifr->ifr_addr)->sin_addr)
		{
			CString::StringCopy(sIfName, ifr->ifr_name);
			delete[] buf;
			return true;
		}
	}
	delete[] buf;
	return false;
}

#endif

AFS_API uint32 GetIpType(uint32 nAddr)
{
	uint32 nRet = FOCP_IP_V4;
	uint8* pAddr = (uint8*)&nAddr;
	if(pAddr[0] <= 127)
	{
		nRet |= FOCP_IP_A;
		if(pAddr[0] == 0)
			nRet |= FOCP_IP_ANY | FOCP_IP_SPECIFIC;
		else
		{
			nRet |= FOCP_IP_UC;
			if(pAddr[0] == 127)
				nRet |= FOCP_IP_LB | FOCP_IP_SPECIFIC;
			else
			{
				nRet |= FOCP_IP_HOST;
				if(pAddr[0] == 10)
					nRet |= FOCP_IP_PRIVATE;
			}
		}
		if(pAddr[1] == 255 || pAddr[2] == 255 || pAddr[3] == 255)
		{
			nRet |= FOCP_IP_BC;
			nRet &= ~FOCP_IP_HOST;
		}
	}
	else if(pAddr[0] <= 191)
	{
		nRet |= FOCP_IP_B | FOCP_IP_UC | FOCP_IP_HOST;
		if(pAddr[0] == 172 && pAddr[1] >= 16 && pAddr[1] <= 31)
			nRet |= FOCP_IP_PRIVATE;
		else if(pAddr[0] == 169 && pAddr[1] == 254)
			nRet |= FOCP_IP_SPECIFIC;
		if(pAddr[2] == 255 || pAddr[3] == 255)
		{
			nRet |= FOCP_IP_BC;
			nRet &= ~FOCP_IP_HOST;
		}
	}
	else if(pAddr[0] <= 223)
	{
		nRet |= FOCP_IP_C | FOCP_IP_UC | FOCP_IP_HOST;
		if(pAddr[0] == 192 && pAddr[1] == 168)
			nRet |= FOCP_IP_PRIVATE;
		if(pAddr[3] == 255)
		{
			nRet |= FOCP_IP_BC;
			nRet &= ~FOCP_IP_HOST;
		}
	}
	else if(pAddr[0] <= 239)
	{
		nRet |= FOCP_IP_D | FOCP_IP_MC;
		uint32 nAddr2 = CBinary::U32Code(nAddr);
		if(nAddr2 <= 0xE00000FF)
			nRet |= FOCP_IP_SPECIFIC;
		else if(pAddr[0] == 239)
			nRet |= FOCP_IP_PRIVATE;
	}
	else
	{
		nRet |= FOCP_IP_E;
		if(nAddr == (uint32)(-1))
			nRet |= FOCP_IP_BC;
	}
	return nRet;
}

AFS_API bool CheckIpMask(uint32 nAddr, uint32 nMask)
{
	uint32 nType = GetIpType(nAddr), nBit=1, n=0;
	nMask = CBinary::U32Code(nMask);
	switch(nType & FOCP_IP_AE)
	{
	case FOCP_IP_A:
		if(nMask < FOCP_IP_MINMASK_A)
			return false;
		while(!(nMask & nBit))
		{
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		while(nMask & nBit)
		{
			nBit <<= 1;
			++n;
			if(n >= 24)
				break;
		}
		if(n == 24)
			return true;
		break;
	case FOCP_IP_B:
		if(nMask < FOCP_IP_MINMASK_B)
			return false;
		while(!(nMask & nBit))
		{
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		while(nMask & nBit)
		{
			nBit <<= 1;
			++n;
			if(n >= 16)
				break;
		}
		if(n == 16)
			return true;
		break;
	case FOCP_IP_C:
		if(nMask < FOCP_IP_MINMASK_C)
			return false;
		while(!(nMask & nBit))
		{
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		while(nMask & nBit)
		{
			nBit <<= 1;
			++n;
			if(n >= 8)
				break;
		}
		if(n == 8)
			return true;
		break;
	}
	return false;
}

AFS_API bool GetBcAddr(uint32 nAddr, uint32 nMask, uint32 nBcType, uint32 &nBcAddr)
{
	if(nBcType == FOCP_BCADDR_0)
	{
		nBcAddr = (uint32)(-1);
		return true;
	}
	uint32 nNet, nType = GetIpType(nAddr), n=0, nBit=1, nHost=0;
	switch(nType & FOCP_IP_AE)
	{
	case FOCP_IP_A:
		nNet = nAddr & FOCP_IP_MINMASK_A;
		if(nBcType == FOCP_BCADDR_1 || nBcType == FOCP_BCADDR_3)
		{
			nBcAddr = (uint32)(-1);
			nBcAddr &= ~FOCP_IP_MINMASK_A;
			nBcAddr |= nNet;
			return true;
		}
		while(!(nMask & nBit))
		{
			nHost |= nBit;
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		nBcAddr = nHost | (nAddr & (~nHost));
		return true;
	case FOCP_IP_B:
		nNet = nAddr & FOCP_IP_MINMASK_B;
		if(nBcType == FOCP_BCADDR_1 || nBcType == FOCP_BCADDR_3)
		{
			nBcAddr = (uint32)(-1);
			nBcAddr &= ~FOCP_IP_MINMASK_B;
			nBcAddr |= nNet;
			return true;
		}
		while(!(nMask & nBit))
		{
			nHost |= nBit;
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		nBcAddr = nHost | (nAddr & (~nHost));
		return true;
	case FOCP_IP_C:
		nNet = nAddr & FOCP_IP_MINMASK_C;
		if(nBcType == FOCP_BCADDR_1 || nBcType == FOCP_BCADDR_3)
		{
			nBcAddr = (uint32)(-1);
			nBcAddr &= ~FOCP_IP_MINMASK_C;
			nBcAddr |= nNet;
			return true;
		}
		while(!(nMask & nBit))
		{
			nHost |= nBit;
			nBit <<= 1;
			++n;
		}
		if(n == 0)
			return false;
		nBcAddr = nHost | (nAddr & (~nHost));
		return true;
	}
	return false;
}

AFS_API bool IsBcAddr(uint32 nAddr, uint32 nMask)
{
	uint32 nBcAddr;
	if(GetIpType(nAddr) & FOCP_IP_BC)
		return true;
	return (GetBcAddr(nAddr, nMask, FOCP_BCADDR_2, nBcAddr) && (nAddr == nBcAddr));
}

AFS_API bool CheckHostIp(uint32 nAddr, uint32 nMask)
{
	uint32 nBcAddr;
	uint32 nType = GetIpType(nAddr);
	if(!(nType & FOCP_IP_HOST))
		return false;
	if(!GetBcAddr(nAddr, nMask, FOCP_BCADDR_2, nBcAddr))
		return false;
	return (nAddr != nBcAddr);
}

AFS_API uint32 GetIpType(uint32 nAddr[4])
{
	uint32 nRet = FOCP_IP_V6;
	uint32 nAddr0 = CBinary::U32Code(nAddr[0]);
	uint32 nBit = (nAddr0 >> 22);
	if(nBit == 0x3FA)
		nRet |= FOCP_IPV6_LINK;
	else if(nBit == 0x3FB)
		nRet |= FOCP_IPV6_SITE;
	else
	{
		nBit >>= 1;//nAddr0 >> 23
		if(nBit == 0x1FC)
			nRet |= FOCP_IPV6_NON;
		else
		{
			nBit >>= 1;//nAddr0 >> 24
			if(nBit == 0xFF)
				nRet |= FOCP_IPV6_MC;
			else if(nBit == 1)
				nRet |= FOCP_IPV6_NON;
			else if(nBit == 0)
			{
				nRet |= FOCP_IPV6_SPECIFIC;
				if(nAddr0 == 0 && nAddr[1] == 0)
				{
					if(nAddr[2] == 0)
					{
						nRet |= FOCP_IPV6_V4;//兼容地址
						if(nAddr[3] == 0)
							nRet |= FOCP_IPV6_ANY;
						else if(1 == CBinary::U32Code(nAddr[3]))
							nRet |= FOCP_IPV6_LB;
					}
					else if(0xFFFF == CBinary::U32Code(nAddr[2]))
						nRet |= FOCP_IPV6_V4MAP;
				}
			}
			else
			{
				nBit >>= 1;//nAddr0 >> 25
				if(1 == nBit)
					nRet |= FOCP_IPV6_NSAP;
				else if(2 == nBit)
					nRet |= FOCP_IPV6_IPX;
				else if( (3 == nBit) || (0x7E == nBit) )
					nRet |= FOCP_IPV6_NON;
				else
				{
					nBit >>= 1;//nAddr0 >> 26
					if(0x3E == nBit)
						nRet |= FOCP_IPV6_NON;
					else
					{
						nBit >>= 1;//nAddr0 >> 27
						if( (1 == nBit) || (0x1E == nBit) )
							nRet |= FOCP_IPV6_NON;
						else
						{
							nBit >>= 1;//nAddr0 >> 28
							if( (1 == nBit) || (0xE == nBit) )
								nRet |= FOCP_IPV6_NON;
							else
							{
								nBit >>= 1;//nAddr0 >> 29
								if(1 == nBit)
									nRet |= FOCP_IPV6_GLOBAL;
								else if(nBit >= 2 && nBit <= 6)
									nRet |= FOCP_IPV6_NON;
							}
						}
					}
				}
			}
		}
	}
	return nRet;
}

CIpAddress::~CIpAddress()
{
}

CIpAddress::CIpAddress()
{
	m_oAddr.nAddr[0] = m_oAddr.nAddr[1] = m_oAddr.nAddr[2] = m_oAddr.nAddr[3] = 0;
	m_oAddr.nPort = 0;
	m_oAddr.nZone = 0;
	CumputeType();
}

CIpAddress::CIpAddress(const CIpAddress& oSrc)
	:m_oAddr(oSrc.m_oAddr)
{
}

CIpAddress& CIpAddress::operator=(const CIpAddress& oSrc)
{
	if(this != &oSrc)
		m_oAddr = oSrc.m_oAddr;
	return *this;
}

void CIpAddress::SetIp(uint32 nIp, bool bMap)
{
	m_oAddr.nAddr[3] = nIp;
	if(bMap)
		m_oAddr.nAddr[2] = CBinary::U32Code(0xFFFF);
	else
		m_oAddr.nAddr[2] = 0;
	m_oAddr.nAddr[0] = m_oAddr.nAddr[1] = 0;
	CumputeType();
}

void CIpAddress::SetIp(const uint32 nIp[4], uint32 nZone)
{
	m_oAddr.nAddr[0] = nIp[0];
	m_oAddr.nAddr[1] = nIp[1];
	m_oAddr.nAddr[2] = nIp[2];
	m_oAddr.nAddr[3] = nIp[3];
	m_oAddr.nZone = nZone;
	CumputeType();
}

void CIpAddress::GetIpAddr(CIpAddrInfo & oIpAddr)
{
	oIpAddr = m_oAddr;
}

bool CIpAddress::GetIp(uint32 &nIp, bool &bMap)
{
	if(m_oAddr.nType & FOCP_IP_V6)
		return false;
	if(m_oAddr.nAddr[2] == 0)
		bMap = false;
	else
		bMap = true;
	nIp = m_oAddr.nAddr[3];
	return true;
}

void CIpAddress::GetIp(uint32 nIp[4])
{
	nIp[0] = m_oAddr.nAddr[0];
	nIp[1] = m_oAddr.nAddr[1];
	nIp[2] = m_oAddr.nAddr[2];
	nIp[3] = m_oAddr.nAddr[3];
}

uint16 CIpAddress::GetPort()
{
	return m_oAddr.nPort;
}

void CIpAddress::SetPort(uint16 nPort)
{
	m_oAddr.nPort = nPort;
}

uint32 CIpAddress::GetZone()
{
	return m_oAddr.nZone;
}

bool CIpAddress::SetZone(uint32 nZone)
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(!(m_oAddr.nType & FOCP_IPV6_LB))
		{
			m_oAddr.nZone = nZone;
			return true;
		}
	}
	return false;
}

bool CIpAddress::IsIpv4(bool &bMap)
{
	if(m_oAddr.nType & FOCP_IP_V6)
		return false;
	if(m_oAddr.nAddr[2] == 0)
		bMap = false;
	else
		bMap = true;
	return true;
}

bool CIpAddress::IsIpv6()
{
	if(m_oAddr.nType & FOCP_IP_V6)
		return true;
	return false;
}

bool CIpAddress::IsAny()
{
	if(m_oAddr.nType & FOCP_IP_ANY)
		return true;
	return false;
}

bool CIpAddress::IsLoopBack()
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(m_oAddr.nType & FOCP_IPV6_LB)
			return true;
	}
	else if(m_oAddr.nType & FOCP_IP_LB)
		return true;
	return false;
}

bool CIpAddress::IsUniCast()
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(m_oAddr.nType & (FOCP_IPV6_LB|FOCP_IPV6_GLOBAL|FOCP_IPV6_LINK|FOCP_IPV6_SITE))
			return true;
	}
	else if(m_oAddr.nType & FOCP_IP_UC)
		return true;
	return false;
}

bool CIpAddress::IsMultiCast()
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(m_oAddr.nType & FOCP_IPV6_MC)
			return true;
	}
	else if(m_oAddr.nType & FOCP_IP_MC)
		return true;
	return false;
}

bool CIpAddress::IsBroadCast(uint32 nMaskLen)
{
	if(m_oAddr.nType & FOCP_IP_V6)
		return false;
	uint16 nMinMask, nMaxMask;
	GetMinMaxMask(nMinMask, nMaxMask);
	if(nMaskLen < nMinMask || nMaskLen > nMaxMask)
		return false;
	uint32 nMaskIp = (uint32)(-1);
	nMaskIp >>= (32 - nMaskLen);
	nMaskIp <<= nMaskLen;
	return IsBcAddr(m_oAddr.nAddr[3], nMaskIp);
}

bool CIpAddress::GetBroadCastAddr(uint32 &nBcAddr, uint32 nBcType, uint32 nMaskLen)
{
	if(m_oAddr.nType & FOCP_IP_V6)
		return false;
	uint16 nMinMask, nMaxMask;
	GetMinMaxMask(nMinMask, nMaxMask);
	if(!nMaskLen)
		nMaskLen = nMaskLen;
	else if(nMaskLen < nMinMask || nMaskLen > nMaxMask)
		return false;
	uint32 nMaskIp = (uint32)(-1);
	nMaskIp >>= (32 - nMaskLen);
	nMaskIp <<= nMaskLen;
	return GetBcAddr(m_oAddr.nAddr[3], nMaskIp, nBcType, nBcAddr);
}

bool CIpAddress::IsHostIp()
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(m_oAddr.nType & (FOCP_IPV6_GLOBAL|FOCP_IPV6_LINK|FOCP_IPV6_SITE))
			return true;
		return false;
	}
	uint16 nMinMask, nMaxMask;
	GetMinMaxMask(nMinMask, nMaxMask);
	uint32 nMask = -1;
	nMask >>= (32 - nMinMask);
	nMask <<= nMinMask;
	return CheckHostIp(m_oAddr.nAddr[3], nMask);
}

const CIpAddrInfo* CIpAddress::GetAddrInfo() const
{
	return &m_oAddr;
}

void CIpAddress::CumputeType()
{
	uint32 nType = GetIpType(m_oAddr.nAddr);
	if(nType & FOCP_IPV6_V4MAP)
		nType = GetIpType(m_oAddr.nAddr[3]);
	else if(nType & FOCP_IP_V6)
	{
		if(!(nType & FOCP_IPV6_LB))
			nType = GetIpType(m_oAddr.nAddr[3]);
	}
	m_oAddr.nType = nType;
	if(!(nType & FOCP_IP_V6))
		m_oAddr.nZone = 0;
}

void CIpAddress::GetMinMaxMask(uint16 &nMinMask, uint16 &nMaxMask)
{
	if(m_oAddr.nType & FOCP_IP_V6)
	{
		if(m_oAddr.nType & (FOCP_IPV6_LB|FOCP_IPV6_NON))
			nMinMask = nMaxMask = 0;
		else if(m_oAddr.nType & FOCP_IPV6_SPECIFIC)
		{
			nMinMask = 8;
			nMaxMask = 127;
		}
		else if(m_oAddr.nType & (FOCP_IPV6_NSAP|FOCP_IPV6_IPX))
		{
			nMinMask = 7;
			nMaxMask = 127;
		}
		else if(m_oAddr.nType & FOCP_IPV6_GLOBAL)
		{
			nMinMask = 3;
			nMaxMask = 127;
		}
		else if(m_oAddr.nType & (FOCP_IPV6_LINK|FOCP_IPV6_SITE))
		{
			nMinMask = 10;
			nMaxMask = 127;
		}
		else if(m_oAddr.nType & FOCP_IPV6_MC)
		{
			nMinMask = 8;
			nMaxMask = 127;
		}
		else
			nMinMask = nMaxMask = 0;
	}
	else switch(m_oAddr.nType & FOCP_IP_AE)
		{
		default:
			nMinMask = nMaxMask = 0;
			break;
		case FOCP_IP_A:
			nMinMask = 8;
			nMaxMask = 31;
			break;
		case FOCP_IP_B:
			nMinMask = 16;
			nMaxMask = 31;
			break;
		case FOCP_IP_C:
			nMinMask = 24;
			nMaxMask = 31;
			break;
		}
}

bool CIpAddress::SetIp(const char* sIp, bool bUseDns, bool bWithPort, bool bWithZone)
{
	if(!sIp || !sIp[0])
		return false;
	char* pStart = CString::TokenOfString(sIp);
	if(!pStart)
		return false;
	char* pEnd = (char*)sIp;
	if(CString::TokenOfString(sIp))
		return false;
	uint32 nLen = pEnd-pStart;
	if(!nLen)
		return false;
	CString oIp(pStart, nLen);
	sIp = oIp.GetStr();
	CIpAddrInfo oAddr = m_oAddr;//备份，当出错时需要还原
	if(ParseIpV4((char*)sIp, nLen, bWithPort))
		return true;
	if(ParseIpV6((char*)sIp, nLen, bWithPort, bWithZone))
		return true;
	if(bUseDns && ParseDns((char*)sIp, nLen, bWithPort))
		return true;
	m_oAddr = oAddr;
	return false;
}

//A.B.C.D:P [A.B.C.D]:P A.B.C.D/P [A.B.C.D]/P A.B.C.D [A.B.C.D]
bool CIpAddress::ParseIpV4(char* sIp, uint32 nLen, bool bWithPort)
{
	CString oLine;
	uint32 nVal[4];
	CFormatBinary oFmt((uint8*)sIp, nLen);
	if(4 == oFmt.Scan("%u.%u.%u.%u", nVal, nVal+1, nVal+2, nVal+3))
	{
		if( (nVal[0] > 255) || (nVal[1] > 255) || (nVal[2] > 255) || (nVal[3] > 255) )
			return false;
		m_oAddr.nAddr[0] = m_oAddr.nAddr[1] = 0;
		m_oAddr.nAddr[2] = CBinary::U32Code(0xFFFF);
		uint8* pAddr = (uint8*)&m_oAddr.nAddr[3];
		pAddr[0] = (uint8)nVal[0];
		pAddr[1] = (uint8)nVal[1];
		pAddr[2] = (uint8)nVal[2];
		pAddr[3] = (uint8)nVal[3];
	}
	else if(4 == oFmt.Scan("[%u.%u.%u.%u", nVal, nVal+1, nVal+2, nVal+3))
	{
		char c = '\0';
		oFmt.GetChar(c);
		if(c != ']')
			return false;
		if( (nVal[0] > 255) || (nVal[1] > 255) || (nVal[2] > 255) || (nVal[3] > 255) )
			return false;
		m_oAddr.nAddr[0] = m_oAddr.nAddr[1] = 0;
		m_oAddr.nAddr[2] = CBinary::U32Code(0xFFFF);
		uint8* pAddr = (uint8*)&m_oAddr.nAddr[3];
		pAddr[0] = (uint8)nVal[0];
		pAddr[1] = (uint8)nVal[1];
		pAddr[2] = (uint8)nVal[2];
		pAddr[3] = (uint8)nVal[3];
	}
	if(bWithPort)
	{
		if( (1 == oFmt.Scan(":%u", nVal)) || (1 == oFmt.Scan("/%u", nVal)) )
		{
			if(nVal[0] > 65535)
				return false;
			m_oAddr.nPort = (uint16)nVal[0];
		}
	}
	if(1 == oFmt.Scan("%r", &oLine))
		return false;
	m_oAddr.nType = GetIpType(m_oAddr.nAddr[3]);
	m_oAddr.nZone = 0;
	return true;
}

// [ipv6]:port [ipv6]/port ipv6/port           [ipv6] ipv6
bool CIpAddress::ParseIpV6(char* sIp, uint32 nLen, bool bWithPort, bool bWithZone)
{
//	char c;
	char* pShift;
	CString oLine;
	uint32 nVal[8];//, nZone=0;
	if(CString::CharOfString(sIp, '+') || CString::CharOfString(sIp, '-') ||
			CString::CharOfString(sIp, 'X') || CString::CharOfString(sIp, 'x') )
		return false;
	if(sIp[0] == '[')
	{
		++sIp;
		pShift = CString::CharOfString(sIp, ']');
		if(!pShift)
			return false;
		pShift[0] = 0;
		nLen = pShift - sIp;
		++pShift;
		if(bWithPort)
		{
			if(pShift[1] == ':' || pShift[1] == '/')
			{
				++pShift;
				if(1 != StringScan(pShift, "%u%r", nVal, &oLine))
					return false;
				if(nVal[0] > 65535)
					return false;
				m_oAddr.nPort = (uint16)nVal[0];
			}
			else if(1 == StringScan(pShift, "%r", &oLine))
				return false;
		}
		else if(1 == StringScan(pShift, "%r", &oLine))
			return false;
	}
	else if(bWithPort)
	{
		pShift = CString::CharOfString(sIp, '/');
		if(pShift)
		{
			pShift[0] = 0;
			nLen = pShift - sIp;
			++pShift;
			if(1 != StringScan(pShift, "%u%r", nVal, &oLine))
				return false;
			if(nVal[0] > 65535)
				return false;
			m_oAddr.nPort = (uint16)nVal[0];
		}
	}
	m_oAddr.nZone = 0;
	if(bWithZone)
	{
		pShift = CString::CharOfString(sIp, '%');
		if(pShift)
		{
			pShift[0] = 0;
			nLen = pShift - sIp;
			++pShift;
			if(1 != StringScan(pShift, "%u%r", nVal, &oLine))
				return false;
			m_oAddr.nZone = nVal[0];
		}
	}

	nVal[4] = nVal[5] = nVal[6] = nVal[7] = 0;
	char* sPeriod = CString::StringOfString(sIp, ".");
	if(sPeriod)
	{
		if(m_oAddr.nZone)
			return false;
		m_oAddr.nAddr[0] = m_oAddr.nAddr[1] = 0;
		while(sPeriod != sIp)
		{
			if(sPeriod[0] == ':')
				break;
			--sPeriod;
		}
		if(sPeriod == sIp)
			return false;
		++sPeriod;
		if(sIp+7 == sPeriod)
		{
			if(CBinary::MemoryCompare(sIp, "::FFFF:", 7))
				return false;
			m_oAddr.nAddr[2] = CBinary::U32Code(0xFFFF);
		}
		else if(sIp + 2 == sPeriod)
		{
			if(CBinary::MemoryCompare(sIp, "::", 2))
				return false;
			m_oAddr.nAddr[2] = 0;
		}
		else
			return false;
		if(4 != StringScan(sPeriod, "%u.%u.%u.%u%r", nVal, nVal+1, nVal+2, nVal+3, &oLine))
			return false;
		if( (nVal[0] > 255) || (nVal[1] > 255) || (nVal[2] > 255) || (nVal[3] > 255) )
			return false;
		uint8* pAddr = (uint8*)&m_oAddr.nAddr[3];
		pAddr[0] = (uint8)nVal[0];
		pAddr[1] = (uint8)nVal[1];
		pAddr[2] = (uint8)nVal[2];
		pAddr[3] = (uint8)nVal[3];
		m_oAddr.nType = GetIpType(m_oAddr.nAddr[3]);
		return true;
	}
	nVal[0] = nVal[1] = nVal[2] = nVal[3] = 0;

	CFormatBinary oFmt((uint8*)sIp, nLen);
	char* sDoubleColon = CString::StringOfString(sIp, "::");
	if(sDoubleColon)
	{
		if(CString::StringOfString(sDoubleColon+2, "::"))
			return false;
		sDoubleColon[0] = '\0';
	}
	uint32 nIdx = 0;
	bool bNeedColon = false;
	if(sDoubleColon != sIp)while(nIdx < 8)
		{
			if(bNeedColon)
			{
				if(1 != oFmt.Scan(":%x", nVal+nIdx))
					break;
			}
			else
			{
				bNeedColon = true;
				if(1 != oFmt.Scan("%x", nVal+nIdx))
					break;
			}
			if(nVal[nIdx] > 65535)
				return false;
			++nIdx;
		}
	if(1 == oFmt.Scan("%r", &oLine))
		return false;
	if(sDoubleColon)
	{
		if(sPeriod != sDoubleColon+1)
		{
			char c;
			uint32 nMid = nIdx;
			oFmt.GetChar(c);//跳过先前的'\0'字符
			while(nIdx < 8)
			{
				if(1 != oFmt.Scan(":%x", nVal+nIdx))
					break;
				if(nVal[nIdx] > 65535)
					return false;
				++nIdx;
			}
			if(1 == oFmt.Scan("%r", &oLine))
				return false;
			uint32 nSet = 0;
			ArrayCopy(nVal+nMid+8-nIdx, nVal+nMid, nIdx-nMid);
			ArraySet(nVal+nMid, nSet, 8-nIdx);
		}
		nIdx = 8;
	}
	if(nIdx != 8)
		return false;
	m_oAddr.nAddr[0] = CBinary::U32Code((nVal[0] << 16) | nVal[1]);
	m_oAddr.nAddr[1] = CBinary::U32Code((nVal[2] << 16) | nVal[3]);
	m_oAddr.nAddr[2] = CBinary::U32Code((nVal[4] << 16) | nVal[5]);
	m_oAddr.nAddr[3] = CBinary::U32Code((nVal[6] << 16) | nVal[7]);
	uint32 nType = GetIpType(m_oAddr.nAddr);
	if(nType & FOCP_IPV6_V4MAP)
		nType = GetIpType(m_oAddr.nAddr[3]);
	else if(nType & FOCP_IPV6_V4)
	{
		if(!(nType & FOCP_IPV6_LB))
			nType = GetIpType(m_oAddr.nAddr[3]);
	}
	m_oAddr.nType = nType;
	if(nType & FOCP_IP_V6)
	{
		if(nType & FOCP_IPV6_LB)
			m_oAddr.nZone = 0;
	}
	else
		m_oAddr.nZone = 0;
	return true;
}

bool CIpAddress::operator==(const CIpAddress& oSrc)
{
	return (0 == CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

bool CIpAddress::operator!=(const CIpAddress& oSrc)
{
	return (0 != CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

bool CIpAddress::operator<(const CIpAddress& oSrc)
{
	return (0 > CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

bool CIpAddress::operator<=(const CIpAddress& oSrc)
{
	return (0 >= CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

bool CIpAddress::operator>(const CIpAddress& oSrc)
{
	return (0 < CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

bool CIpAddress::operator>=(const CIpAddress& oSrc)
{
	return (0 <= CBinary::MemoryCompare(m_oAddr.nAddr, oSrc.m_oAddr.nAddr, sizeof(m_oAddr.nAddr)));
}

void CIpAddress::GetIp(CString &oIp, bool bForceIpV6)
{
	oIp.Clear();
	CStringFormatter oFmt(&oIp);
	if(!(m_oAddr.nType & FOCP_IP_V6) && !bForceIpV6)
	{
		uint8* pAddr = (uint8*)&m_oAddr.nAddr[3];
		oFmt.Print("%u.%u.%u.%u", (uint32)pAddr[0], (uint32)pAddr[1], (uint32)pAddr[2], (uint32)pAddr[3]);
		if(m_oAddr.nPort)
			oFmt.Print(":%u", (uint32)m_oAddr.nPort);
	}
	else
	{
		uint16 * pAddr = (uint16*)m_oAddr.nAddr;
		if(m_oAddr.nPort)
			oFmt.Print("[");
		oFmt.Print("%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X", (uint32)CBinary::U16Code(pAddr[0]), (uint32)CBinary::U16Code(pAddr[1]), (uint32)CBinary::U16Code(pAddr[2]), (uint32)CBinary::U16Code(pAddr[3]),
				   (uint32)CBinary::U16Code(pAddr[4]), (uint32)CBinary::U16Code(pAddr[5]), (uint32)CBinary::U16Code(pAddr[6]), (uint32)CBinary::U16Code(pAddr[7]));
		if(m_oAddr.nZone)
			oFmt.Print("%%%u", m_oAddr.nZone);
		if(m_oAddr.nPort)
			oFmt.Print("]:%u", (uint32)m_oAddr.nPort);
	}
}

//注意windows.h的包含位置，不能太靠前，以防SetPort函数被重命名
FOCP_END();

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
//	#include <stdio.h>
#endif

FOCP_BEGIN();

bool CIpAddress::ParseDns(char* sName, uint32 nLen, bool bWithPort)
{
	CString oLine;
	if(bWithPort)
	{
		char* pShift = CString::CharOfString(sName, ':');
		if(pShift)
		{
			uint32 nVal;
			CFormatBinary oFmt((uint8*)pShift, nLen - (pShift - sName));
			if(1 != oFmt.Scan(":%u%r", &nVal, &oLine))
				return false;
			if(nVal > 65535)
				return false;
			m_oAddr.nPort = (uint16)nVal;
			*pShift = '\0';
		}
	}

	CSystemLock s;
	struct hostent * pHost = gethostbyname(sName);
#if defined(UNIX) && defined(AF_INET6) && !defined(CYGWIN_NT)
	if(!pHost)
	{
		char sBuf[2048]; // TODO: Too big buffer?
		int32 nErr = 0;
		struct hostent oHostBuf;
#if defined(__sun)
		pHost = gethostbyname_r(sName, AF_INET6, &oHostBuf, sBuf, 2048, &pHost, &nErr);
#else
		if (gethostbyname2_r(sName, AF_INET6, &oHostBuf, sBuf, 2048, &pHost, &nErr))
			pHost = NULL;
#endif
		if(!pHost)
			return false;
	}
#endif
	if(!pHost->h_addr_list[0])
		return false;
#ifdef AF_INET6
	if(pHost->h_addrtype == AF_INET6)
#else
	if(pHost->h_length == 16)
#endif
		SetIp((uint32*)pHost->h_addr_list[0]);
	else if(pHost->h_addrtype == AF_INET)
		SetIp(*(uint32*)pHost->h_addr_list[0]);
	else
		return false;
	return true;
}

void CIpAddress::GetIpName(CString &oIpName)
{
	CSystemLock s;
	struct hostent * pHost = NULL;
	if(m_oAddr.nType & FOCP_IP_V6)
	{
#ifdef AF_INET6
		pHost = gethostbyaddr((const char*)m_oAddr.nAddr, 16, AF_INET6);
#endif
	}
	else
		pHost = gethostbyaddr((const char*)(m_oAddr.nAddr+3), 4, AF_INET);
	if(pHost)
		oIpName = pHost->h_name;
	else
		GetIp(oIpName);
}

AFS_API bool AddIpv4Addr(const char* sIpV4, const char* sMaskV4, const char* sMacAddr)
{
	uint8 sMac[6];
	if(6 != StringScan(sMacAddr, "%x8-%x8-%x8-%x8-%x8-%x8", sMac, sMac+1, sMac+2, sMac+3, sMac+4, sMac+5))
		return false;
#ifdef WINDOWS
	uint32 iaIPAddress;
	uint32 iaIPMask;
	ulong NTEContext = 0;
	ulong NTEInstance = 0;
	iaIPAddress = inet_addr(sIpV4);
	if (iaIPAddress == INADDR_NONE)
		return false;
	iaIPMask = inet_addr(sMaskV4);
	if (iaIPMask == INADDR_NONE)
		return false;
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO*)CMalloc::Malloc(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
		return false;
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		CMalloc::Free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)CMalloc::Malloc(ulOutBufLen);
		if (pAdapterInfo == NULL)
			return false;
	}
	if (NO_ERROR == GetAdaptersInfo(pAdapterInfo, &ulOutBufLen))
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			if(!CBinary::MemoryCompare(pAdapter->Address, sMac, 6))
			{
				uint32 nIfIndex = pAdapter->Index;
				CMalloc::Free(pAdapterInfo);
				uint32 nRet = AddIPAddress(iaIPAddress, iaIPMask, nIfIndex, &NTEContext, &NTEInstance);
				if(nRet != NO_ERROR)
				{
					/*					char * lpMsgBuf = NULL;
										if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL, nRet, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL))
										{
											printf("\tError: %s", lpMsgBuf);
											LocalFree(lpMsgBuf);
										}*/
					return false;
				}
				return true;
			}
			pAdapter = pAdapter->Next;
		}
	}
	CMalloc::Free(pAdapterInfo);
	return false;
#elif defined(LINUX)
	uint32 iaIPAddress;
	uint32 iaIPMask;
	uint32 iaBroadAddr;
	iaIPAddress = inet_addr(sIpV4);
	if (iaIPAddress == INADDR_NONE)
		return false;
	iaIPMask = inet_addr(sMaskV4);
	if (iaIPMask == INADDR_NONE)
		return false;
	iaBroadAddr = (iaIPAddress & iaIPMask) | (~iaIPMask);

	struct ifreq Ifr;
	struct sockaddr_in Alias;
	struct sockaddr_in *sin;

	char sIfName[256];
	int32 nSock = socket(PF_INET, SOCK_DGRAM, 0);
	uint32 nIfIndex = GetIfIndex(sMac, nSock, sIfName);
	if(nIfIndex == (uint32)(-1))
	{
		close(nSock);
		return false;
	}

	StringPrint(Ifr.ifr_name, "%s:%u", sIfName, nIfIndex);

	CBinary::MemorySet(&Alias, 0, sizeof(Alias));
	Alias.sin_addr.s_addr = iaIPAddress;
	Alias.sin_family = AF_INET;

	CBinary::MemoryCopy(&(Ifr.ifr_addr), &Alias, sizeof(Alias));
	if (ioctl(nSock, SIOCSIFADDR, &Ifr, sizeof(struct ifreq)) == -1)
	{
		close(nSock);
		return false;
	}

	if (sMaskV4[0] == '\0')
	{
		close(nSock);
		return true;
	}

	sin = (struct sockaddr_in *)&Ifr.ifr_netmask;
	CBinary::MemorySet(sin, 0, sizeof(Ifr.ifr_netmask));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = iaIPMask;
	if (ioctl(nSock, SIOCSIFNETMASK, &Ifr) <0)
	{
		close(nSock);
		return false;
	}

	sin = (struct sockaddr_in *)&Ifr.ifr_broadaddr;
	CBinary::MemorySet(sin, 0, sizeof(Ifr.ifr_broadaddr));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = iaBroadAddr;
	if (ioctl(nSock, SIOCSIFBRDADDR, &Ifr) <0)
	{
		close(nSock);
		return false;
	}

	return true;

#else
#error Support for AddIpv4Addr currently only Linux and Windows
	return false;
#endif
}

AFS_API bool DelIpv4Addr(const char* sIpV4)
{
#ifdef WINDOWS
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *) CMalloc::Malloc(sizeof (IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
		return false;
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		CMalloc::Free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) CMalloc::Malloc(ulOutBufLen);
		if (pAdapterInfo == NULL)
			return false;
	}
	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			IP_ADDR_STRING* pIpAddressList = &pAdapter->IpAddressList;
			while(pIpAddressList)
			{
				if(!CString::StringCompare(pIpAddressList->IpAddress.String, sIpV4))
				{
					ulong nContext = pIpAddressList->Context;
					CMalloc::Free(pAdapterInfo);
					uint32 nRet = DeleteIPAddress(nContext);
					if(nRet != NO_ERROR)
					{
						/*						char * lpMsgBuf = NULL;
												if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
													NULL, nRet, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) & lpMsgBuf, 0, NULL))
												{
													printf("\tError: %s", lpMsgBuf);
													LocalFree(lpMsgBuf);
												}*/
						return false;
					}
					return true;
				}
				pIpAddressList = pIpAddressList->Next;
			}
			pAdapter = pAdapter->Next;
		}
	}
	CMalloc::Free(pAdapterInfo);
	return true;
#elif defined(LINUX)
	struct ifreq lifr;

	uint32 iaIPAddress = inet_addr(sIpV4);
	if (iaIPAddress == INADDR_NONE)
		return false;

	CBinary::MemorySet(&lifr, 0, sizeof(lifr));
	int32 nSock = socket(PF_INET, SOCK_DGRAM, 0);
	if(!GetIfName(iaIPAddress, nSock, lifr.ifr_ifrn.ifrn_name))
	{
		close(nSock);
		return false;
	}
	if (ioctl(nSock, SIOCGIFFLAGS, (char*) &lifr) < 0)
	{
		close(nSock);
		return false;
	}
	// if the vip has been down, nothing to do
	if (!(lifr.ifr_ifru.ifru_flags & IFF_UP) )
	{
		close(nSock);
		return true;
	}
	// down the vitural IP, maybe related with OS
	lifr.ifr_ifru.ifru_flags &= ~IFF_UP;
	if (ioctl(nSock, SIOCSIFFLAGS, (char*) &lifr) < 0)
	{
		close(nSock);
		return false;
	}
	return true;
#else
#error Support for DelIpv4Addr currently only Linux and Windows
	return false;
#endif
}

FOCP_END();
