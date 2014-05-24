
#include "AfsDef.hpp"

#ifndef _Afs_IpAddr_Hpp_
#define _Afs_IpAddr_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_IP_V4 = 0,
	FOCP_IP_V6 = 0x80000000,

	//IPV4属性定义
	FOCP_IP_AE = 0x07,//用作类属识别的掩码
	FOCP_IP_A = 0x01,
	FOCP_IP_B = 0x02,
	FOCP_IP_C = 0x03,
	FOCP_IP_D = 0x04,
	FOCP_IP_E = 0x05,

	FOCP_IP_UC = 0x08,//单播地址
	FOCP_IP_MC = 0x10,//组播地址
	FOCP_IP_BC = 0x20,//广播地址
	FOCP_IP_ANY = 0x40,//任意地址
	FOCP_IP_LB = 0x80,//回环地址
	FOCP_IP_PRIVATE = 0x100,//私有地址,不能用在互联网上
	FOCP_IP_SPECIFIC = 0x200,//专用地址,
	FOCP_IP_HOST = 0x400,//可以用作主机IP地址

	FOCP_IP_MINMASK_A = 0xFF000000,//A类最小掩码
	FOCP_IP_MINMASK_B = 0xFFFF0000,//A类最小掩码
	FOCP_IP_MINMASK_C = 0xFFFFFF00,//A类最小掩码,

	//广播地址类型
	FOCP_BCADDR_0 = 0,//受限
	FOCP_BCADDR_1 = 0,//指向网络的广播地址
	FOCP_BCADDR_2 = 0,//指向子网的广播地址
	FOCP_BCADDR_3 = 0,//指向所有子网的广播地址

	//IPv6地址属性
	FOCP_IPV6_SPECIFIC = 0x01, //保留地址
	FOCP_IPV6_NON = 0x02,//未分配地址
	FOCP_IPV6_NSAP = 0x03,//NSAP保留
	FOCP_IPV6_IPX = 0x04,//IPX保留
	FOCP_IPV6_GLOBAL = 0x05,//公网单播IP，
	FOCP_IPV6_LINK = 0x06,//链路本地单播IP，相当于IPV4的B类专用
	FOCP_IPV6_SITE = 0x07,//站点本地单播IP，相当于IPV4的私有地址
	FOCP_IPV6_MC = 0x08,//组播地址

	FOCP_IPV6_ANY = 0x10,//任意地址
	FOCP_IPV6_LB=0x20,//回环地址
	FOCP_IPV6_V4MAP = 0x40,//IPV4映射过来的IPV6地址
	FOCP_IPV6_V4 = 0x80,//兼容IPV4的IPV6地址
};

//IPV4
AFS_API uint32 GetIpType(uint32 nAddr);
AFS_API bool CheckIpMask(uint32 nAddr, uint32 nMask);
AFS_API bool GetBcAddr(uint32 nAddr, uint32 nMask, uint32 nBcType, uint32 &nBcAddr);
AFS_API bool IsBcAddr(uint32 nAddr, uint32 nMask);
AFS_API bool CheckHostIp(uint32 nAddr, uint32 nMask);
AFS_API bool AddIpv4Addr(const char* sIpV4, const char* sMaskV4, const char* sIfMac);
AFS_API bool DelIpv4Addr(const char* sIpV4);

AFS_API uint32 GetIpType(uint32 nAddr[4]);

struct CIpAddrInfo
{
	uint32 nType;
	uint32 nAddr[4];
	uint32 nZone;
	uint16 nPort;
};

class AFS_API CIpAddress
{
private:
	CIpAddrInfo m_oAddr;

public:
	CIpAddress();
	~CIpAddress();
	CIpAddress(const CIpAddress& oSrc);
	CIpAddress& operator=(const CIpAddress& oSrc);

	bool SetIp(const char* sIp, bool bUseDns=true, bool bWithPort=true, bool bWithZone=true);
	void GetIp(CString &oIp, bool bForceIpV6=false);
	void GetIpName(CString &oIpName);

	void SetIp(uint32 nIp, bool bMap=true);
	void SetIp(const uint32 nIp[4], uint32 nZone=0);
	void GetIpAddr(CIpAddrInfo & oIpAddr);
	bool GetIp(uint32 &nIp, bool &bMap);
	void GetIp(uint32 nIp[4]);

	const CIpAddrInfo* GetAddrInfo() const;

	uint16 GetPort();
	void SetPort(uint16 nPort);

	uint32 GetZone();
	bool SetZone(uint32 nZone);

	bool IsIpv4(bool &bMap);
	bool IsIpv6();
	bool IsAny();
	bool IsLoopBack();
	bool IsUniCast();
	bool IsMultiCast();
	bool IsBroadCast(uint32 nMaskLen);
	bool GetBroadCastAddr(uint32 &nBcAddr, uint32 nBcType, uint32 nMaskLen=0);
	bool IsHostIp();//检查该Ip地址是否可以作为单播地址绑定在网卡上。

	void GetMinMaxMask(uint16 &nMinMask, uint16 &nMaxMask);//返回掩码的最短长度或最长长度

	bool operator==(const CIpAddress& oSrc);
	bool operator!=(const CIpAddress& oSrc);
	bool operator<(const CIpAddress& oSrc);
	bool operator<=(const CIpAddress& oSrc);
	bool operator>(const CIpAddress& oSrc);
	bool operator>=(const CIpAddress& oSrc);

protected:
	void CumputeType();
	bool ParseIpV4(char* sIp, uint32 nLen, bool bWithPort);
	bool ParseIpV6(char* sIp, uint32 nLen, bool bWithPort, bool bWithZone);
	bool ParseDns(char* sName, uint32 nLen, bool bWithPort);
};

FOCP_END();

#endif
