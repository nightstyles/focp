
#include "AfsDef.hpp"

#ifndef _Afs_IpAddr_Hpp_
#define _Afs_IpAddr_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_IP_V4 = 0,
	FOCP_IP_V6 = 0x80000000,

	//IPV4���Զ���
	FOCP_IP_AE = 0x07,//��������ʶ�������
	FOCP_IP_A = 0x01,
	FOCP_IP_B = 0x02,
	FOCP_IP_C = 0x03,
	FOCP_IP_D = 0x04,
	FOCP_IP_E = 0x05,

	FOCP_IP_UC = 0x08,//������ַ
	FOCP_IP_MC = 0x10,//�鲥��ַ
	FOCP_IP_BC = 0x20,//�㲥��ַ
	FOCP_IP_ANY = 0x40,//�����ַ
	FOCP_IP_LB = 0x80,//�ػ���ַ
	FOCP_IP_PRIVATE = 0x100,//˽�е�ַ,�������ڻ�������
	FOCP_IP_SPECIFIC = 0x200,//ר�õ�ַ,
	FOCP_IP_HOST = 0x400,//������������IP��ַ

	FOCP_IP_MINMASK_A = 0xFF000000,//A����С����
	FOCP_IP_MINMASK_B = 0xFFFF0000,//A����С����
	FOCP_IP_MINMASK_C = 0xFFFFFF00,//A����С����,

	//�㲥��ַ����
	FOCP_BCADDR_0 = 0,//����
	FOCP_BCADDR_1 = 0,//ָ������Ĺ㲥��ַ
	FOCP_BCADDR_2 = 0,//ָ�������Ĺ㲥��ַ
	FOCP_BCADDR_3 = 0,//ָ�����������Ĺ㲥��ַ

	//IPv6��ַ����
	FOCP_IPV6_SPECIFIC = 0x01, //������ַ
	FOCP_IPV6_NON = 0x02,//δ�����ַ
	FOCP_IPV6_NSAP = 0x03,//NSAP����
	FOCP_IPV6_IPX = 0x04,//IPX����
	FOCP_IPV6_GLOBAL = 0x05,//��������IP��
	FOCP_IPV6_LINK = 0x06,//��·���ص���IP���൱��IPV4��B��ר��
	FOCP_IPV6_SITE = 0x07,//վ�㱾�ص���IP���൱��IPV4��˽�е�ַ
	FOCP_IPV6_MC = 0x08,//�鲥��ַ

	FOCP_IPV6_ANY = 0x10,//�����ַ
	FOCP_IPV6_LB=0x20,//�ػ���ַ
	FOCP_IPV6_V4MAP = 0x40,//IPV4ӳ�������IPV6��ַ
	FOCP_IPV6_V4 = 0x80,//����IPV4��IPV6��ַ
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
	bool IsHostIp();//����Ip��ַ�Ƿ������Ϊ������ַ���������ϡ�

	void GetMinMaxMask(uint16 &nMinMask, uint16 &nMaxMask);//�����������̳��Ȼ������

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
