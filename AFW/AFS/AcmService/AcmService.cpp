
#include "AFC.hpp"

#include <time.h>

FOCP_BEGIN();

class CAcmService;

struct CAcmDomain
{
	uint32 nNode;
	uint32 nDomainType;
	uint32 nSerialAbility;
	uint32 nSerialCapacity;
	uint32 nSerialMaxAck;
	uint32 nMcItfAddr;
	uint32 nMultiCastAddr;
	uint16 nMultiCastPort;
	uint8 nMultiCastTTL;
	uint8 bAllowLoop;
	uint16 nMcItfPort;
	CRbMap<uint32, CIpAddr> oUniCastInfo;
	CAcmUdp* pUdp;
	CAcmSequenceModule* pSerialModule;
};

static void AcmCmdFunc_LsDomain(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
static void AcmCmdFunc_LsSerial(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
static void AcmCmdFunc_LsToken(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);

class CAcmService: public CService, public CAcmContext, public CAsmNodeWalker, public CAcmTokenWalker
{
private:
	CRbMap<uint32, CAcmDomain> m_oDomainInfo;
	CAcmTokenModule* m_pTokenMoudle;
	CAcmVipModule* m_pVipModule;
	uint32 m_nWalkSize;
	CCmdSession* m_pSession;
	CAcmTcpServer* m_pTcp;
	CEvent m_oEvent;
	bool m_bNodeRepeat;

public:
	CAcmService()
	{
		m_bNodeRepeat = false;
		m_pTokenMoudle = NULL;
		m_pVipModule = NULL;
		m_pTcp = NULL;
	}

	virtual const char* GetServiceName()
	{
		return "AcmService";
	}

	void SetSession(CCmdSession* pSession)
	{
		m_nWalkSize = 0;
		m_pSession = pSession;
	}

	uint32 GetWalkSize()
	{
		return m_nWalkSize;
	}

	virtual bool HaveAbility(uint32 nOption)
	{
		if(nOption == ACM_TCPSVR_ABILITY)
			return m_pTcp!=NULL;
		return true;
	}

protected:
	virtual void OnNodeRepeat(CAcmUdp* pUdp)
	{
		uint32 nState = GetState();
		uint32 nDomain = pUdp->GetDomain();
		uint32 nNode = pUdp->GetNode();
		if(nState < FOCP_SERVICE_STARTED)
		{
			m_bNodeRepeat = true;
			m_oEvent.Set();
			FocpLog(FOCP_LOG_ERROR, ("The current node(nDomain=%u, nNode=%u) is repeated with the network environment", nDomain, nNode));
		}
		else
			FocpLog(FOCP_LOG_ERROR, ("The visitor node(nDomain=%u, nNode=%u) is repeated with the current node", nDomain, nNode));
	}

	virtual void OnAddNode(CAcmUdp* pUdp, uint32 nNode, uint32 nIp, uint16 nPort)
	{
		uint32 nDomain = pUdp->GetDomain();
		CAcmDomain & oDomain = m_oDomainInfo[nDomain];
		if(oDomain.nDomainType == 2)//µ¥²¥
		{
			CRbTreeNode* pIt = oDomain.oUniCastInfo.Find(nNode);
			if(pIt == oDomain.oUniCastInfo.End())
			{
				CIpAddr & oAddr = oDomain.oUniCastInfo[nNode];
				oAddr.nAddr = nIp;
				oAddr.nPort = nPort;
				CTextAccess oAccess;
				CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();
				if(!pConfigSystem->OpenConfig(oAccess, "AcmUniCast"))
					return;
				oAccess.OpenColVal();
				{
					CFormatString oIdx;
					oIdx.Print("%u", nDomain);
					oAccess.SetColVal("AcmDomain", oIdx.GetStr());
				}
				{
					CFormatString oIdx;
					oIdx.Print("%u", nNode);
					oAccess.SetColVal("Node", oIdx.GetStr());
				}
				{
					CString oIdx;
					CIpAddr oIp = {nIp, 0};
					CFile::GetIpFileName(oIp, oIdx);
					oAccess.SetColVal("UniCastAddr", oIdx.GetStr());
				}
				{
					CFormatString oIdx;
					oIdx.Print("%u16", nPort);
					oAccess.SetColVal("UniCastPort", oIdx.GetStr());
				}
				oAccess.Insert(true);
			}
		}
	}

	virtual void OnWalk(uint32 nNode, const char* sAddr)
	{
		m_pSession->Print(" Node = %u Addr=%s\r\n", nNode, sAddr);
		++m_nWalkSize;
	}

	virtual void OnWalk(uint32 nDomain, uint32 nNode, const CAsmNode& oNode)
	{
		CString oDate;
		struct tm *tloc;
#ifndef WINDOWS
		struct tm tt;
		localtime_r((const time_t*)&oNode.nTime, &tt);
		tloc = &tt;
#else
		tloc = localtime((const time_t*)&oNode.nTime);
#endif
		CStringFormatter(&oDate).Print("%04d-%02d-%02d %02d:%02d:%02d",
			1900+tloc->tm_year,tloc->tm_mon+1,tloc->tm_mday,
			tloc->tm_hour,tloc->tm_min,tloc->tm_sec);

		CIpAddr oAddr;
		CString oFileName;
		CAcmUdp* pUdp = CAcmUdp::QueryUdp(nDomain);
		pUdp->GetNodeAddr(nNode, oAddr);
		CFile::GetIpFileName(oAddr, oFileName);

		m_pSession->Print(" Node[%u].Addr(%s) startup at %s, %s, %s\r\n", nNode, oFileName.GetStr(), oDate.GetStr(),
			oNode.bOnLine?"online ":"offline", oNode.bServering?"active":"inactive");

		++m_nWalkSize;
	}

	virtual void OnWalk(uint32 nToken, uint32 nDomain, uint32 nOwner)
	{
		m_pSession->Print(" Token '%u' is in domain '%u', and its owner is %u\r\n", nToken, nDomain, nOwner);
		++m_nWalkSize;
	}

	virtual bool OnInitialize()
	{
		uint32 nLength;
		const char* sVal;
		CTextAccess oAccess;
		CInitConfigSystem* pConfigSystem = CInitConfigSystem::GetInstance();

		if(pConfigSystem->OpenConfig(oAccess, "AcmTcp", true))
		{
			uint32 nPort, nWorkerNum;
			oAccess.OpenIdxVal();
			if(oAccess.Query())
			{
				sVal = oAccess.GetVal("LsnPort", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmTcp.LsnPort' is invalid"));
					return false;
				}
				nPort = CString::Atoi(sVal);
				if(nPort >= 65536)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmTcp.LsnPort(%u)' is invalid", nPort));
					return false;
				}
				sVal = oAccess.GetVal("WorkerNum", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmTcp.WorkerNum' is invalid"));
					return false;
				}
				nWorkerNum = CString::Atoi(sVal);
				if(nPort)
				{
					m_pTcp = CAcmTcpServer::GetInstance();
					if(m_pTcp->Initialize(nPort, nWorkerNum))
						return false;
				}
			}
		}

		if(!pConfigSystem->OpenConfig(oAccess, "AcmApplication"))
			return false;
		oAccess.OpenIdxVal();
		while(oAccess.Query())
		{
			uint32 nAcmDomain, nNode;
			sVal = oAccess.GetVal("AcmDomain", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmApplication.AcmDomain' is invalid"));
				return false;
			}
			nAcmDomain = CString::Atoi(sVal);
			sVal = oAccess.GetVal("Node", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmApplication[%u].Node' is invalid", nAcmDomain));
				return false;
			}
			nNode = CString::Atoi(sVal);
			if(nNode == (uint32)(-1))
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmApplication[%u].Node(%u)' is invalid", nAcmDomain, nNode));
				return false;
			}
			CRbTreeNode* pIt = m_oDomainInfo.Find(nAcmDomain);
			if(pIt != m_oDomainInfo.End())
			{
				CAcmDomain & oDomain = m_oDomainInfo.GetItem(pIt);
				if(oDomain.nNode != nNode)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmApplication[%u] is repeated defined", nAcmDomain));
					return false;
				};
			}
			else
			{
				CAcmDomain & oDomain = m_oDomainInfo[nAcmDomain];
				oDomain.nNode = nNode;
				oDomain.nDomainType = 0;
				oDomain.pUdp = NULL;
				oDomain.pSerialModule = NULL;
			}
		}
		if(m_oDomainInfo.GetSize() == 0)
		{
			FocpLog(FOCP_LOG_SYSLOG, ("AcmApplication is empty"));
			return true;
		}
		if(!pConfigSystem->OpenConfig(oAccess, "AcmDomain"))
			return false;
		oAccess.OpenIdxVal();
		bool bHaveAcmMultiCast = false, bHaveAcmUniCast = false;
		CRbTreeNode* pIt = m_oDomainInfo.First();
		CRbTreeNode* pEnd = m_oDomainInfo.End();
		for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
		{
			uint32 nAcmDomain = m_oDomainInfo.GetKey(pIt);
			CAcmDomain & oDomain = m_oDomainInfo.GetItem(pIt);
			CFormatString oIdx;
			oIdx.Print("%u", nAcmDomain);
			oAccess.OpenIdxVal();
			if(!oAccess.SetIdxVal("AcmDomain", oIdx.GetStr()))
			{
				FocpLog(FOCP_LOG_ERROR, ("AcmDomain.AcmDomain isn't existed"));
				return false;
			}
			if(!oAccess.Query())
			{
				FocpLog(FOCP_LOG_ERROR, ("AcmDomain[%u] isn't existed", nAcmDomain));
				return false;
			}
			sVal = oAccess.GetVal("DomainType", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].DomainType' is invalid", nAcmDomain));
				return false;
			}
			oDomain.nDomainType = CString::Atoi(sVal);
			if(oDomain.nDomainType == 1)
				bHaveAcmMultiCast = true;
			else if(oDomain.nDomainType == 2)
				bHaveAcmUniCast = true;
			else
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].DomainType(%u) is invalid", nAcmDomain, oDomain.nDomainType));
				return false;
			}
			sVal = oAccess.GetVal("SerialAbility", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].SerialAbility' is invalid", nAcmDomain));
				return false;
			}
			oDomain.nSerialAbility = CString::Atoi(sVal);
			if(oDomain.nSerialAbility > 6 || (oDomain.nDomainType == 1 && (oDomain.nSerialAbility & ASM_SUPPORT_ACK)))
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].SerialAbility(%u) is invalid", nAcmDomain, oDomain.nSerialAbility));
				return false;
			}
			sVal = oAccess.GetVal("SerialCapacity", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].SerialCapacity' is invalid", nAcmDomain));
				return false;
			}
			oDomain.nSerialCapacity = CString::Atoi(sVal);
			sVal = oAccess.GetVal("SerialMaxAck", nLength);
			if(!sVal || sVal[nLength-1])
			{
				FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].SerialMaxAck' is invalid", nAcmDomain));
				return false;
			}
			oDomain.nSerialMaxAck = CString::Atoi(sVal);
		}
		if(bHaveAcmMultiCast)
		{
			if(!pConfigSystem->OpenConfig(oAccess, "AcmMultiCast"))
				return false;
			pIt = m_oDomainInfo.First();
			for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
			{
				CAcmDomain & oDomain = m_oDomainInfo.GetItem(pIt);
				if(oDomain.nDomainType == 2)
					continue;
				uint32 nPort, nAcmDomain = m_oDomainInfo.GetKey(pIt);
				CFormatString oIdx;
				oAccess.OpenIdxVal();
				oIdx.Print("%u", nAcmDomain);
				if(!oAccess.SetIdxVal("AcmDomain", oIdx.GetStr()))
				{
					FocpLog(FOCP_LOG_ERROR, ("AcmMultiCast.AcmDomain isn't existed"));
					return false;
				}
				if(!oAccess.Query())
				{
					FocpLog(FOCP_LOG_ERROR, ("AcmMultiCast[%u] isn't existed", nAcmDomain));
					return false;
				}
				sVal = oAccess.GetVal("MultiCastAddr", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastAddr' is invalid", nAcmDomain));
					return false;
				}
				oDomain.nMultiCastAddr = CFile::GetIpAddr(sVal);
				if(!CFile::IsMulticastAddr(oDomain.nMultiCastAddr))
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastAddr(%s) is invalid", nAcmDomain, sVal));
					return false;
				}
				sVal = oAccess.GetVal("MultiCastPort", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastPort' is invalid", nAcmDomain));
					return false;
				}
				nPort = CString::Atoi(sVal);
				if(nPort >= 65536 || nPort == 0)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastPort(%s) is invalid", nAcmDomain, sVal));
					return false;
				}
				oDomain.nMultiCastPort = nPort;
				sVal = oAccess.GetVal("MultiCastTTL", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastTTL' is invalid", nAcmDomain));
					return false;
				}
				nPort = CString::Atoi(sVal);
				if(nPort >= 256 || nPort == 0)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].MultiCastTTL(%s) is invalid", nAcmDomain, sVal));
					return false;
				}
				oDomain.nMultiCastTTL = nPort;
				sVal = oAccess.GetVal("AllowLoopBack", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].AllowLoopBack' is invalid", nAcmDomain));
					return false;
				}
				nPort = CString::Atoi(sVal);
				if(nPort)
					oDomain.bAllowLoop = 1;
				else
					oDomain.bAllowLoop = 0;
				sVal = oAccess.GetVal("InterfaceAddr", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].InterfaceAddr' is invalid", nAcmDomain));
					return false;
				}
				oDomain.nMcItfAddr = CFile::GetIpAddr(sVal);
				if(!CFile::CheckHostIp(oDomain.nMcItfAddr))
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].InterfaceAddr(%s)' is invalid", nAcmDomain, sVal));
					return false;
				}
				sVal = oAccess.GetVal("InterfacePort", nLength);
				if(!sVal || sVal[nLength-1])
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].InterfacePort' is invalid", nAcmDomain));
					return false;
				}
				nPort = CString::Atoi(sVal);
				if(nPort >= 65536)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmDomain[%u].InterfacePort(%s)' is invalid", nAcmDomain, sVal));
					return false;
				}
				oDomain.nMcItfPort = nPort;
			}
		}
		if(bHaveAcmUniCast)
		{
			if(!pConfigSystem->OpenConfig(oAccess, "AcmUniCast"))
				return false;
			pIt = m_oDomainInfo.First();
			for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
			{
				CAcmDomain & oDomain = m_oDomainInfo.GetItem(pIt);
				if(oDomain.nDomainType == 1)
					continue;
				uint32 nAcmDomain = m_oDomainInfo.GetKey(pIt);
				CFormatString oIdx;
				oAccess.OpenIdxVal();
				oIdx.Print("%u", nAcmDomain);
				if(!oAccess.SetIdxVal("AcmDomain", oIdx.GetStr()))
				{
					FocpLog(FOCP_LOG_ERROR, ("AcmUniCast.AcmDomain isn't existed"));
					return false;
				}
				while(oAccess.Query())
				{
					sVal = oAccess.GetVal("Node", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u].Node is invalid", nAcmDomain));
						return false;
					}
					uint32 nNode = CString::Atoi(sVal);
					CRbTreeNode* pNode = oDomain.oUniCastInfo.Find(nNode);
					if(pNode != oDomain.oUniCastInfo.End())
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u].Node(%u) is repeated defined", nAcmDomain, nNode));
						return false;
					}
					CIpAddr & oIpAddr = oDomain.oUniCastInfo[nNode];
					sVal = oAccess.GetVal("UniCastAddr", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u].UniCastAddr is invalid", nAcmDomain, nNode));
						return false;
					}
					oIpAddr.nAddr = CFile::GetIpAddr(sVal);
					if(CFile::IsMulticastAddr(oIpAddr.nAddr))
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u].UniCastAddr(%s) is invalid", nAcmDomain, nNode, sVal));
						return false;
					}
					if(nNode == oDomain.nNode && !CFile::CheckHostIp(oIpAddr.nAddr))
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u].UniCastAddr(%s) is invalid", nAcmDomain, nNode, sVal));
						return false;
					}
					sVal = oAccess.GetVal("UniCastPort", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u].UniCastPort is invalid", nAcmDomain, nNode));
						return false;
					}
					uint32 nPort = CFile::GetIpAddr(sVal);
					if(nPort >= 65536)
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u].UniCastPort(%u) is invalid", nAcmDomain, nNode, nPort));
						return false;
					}
					oIpAddr.nPort = nPort;
				}
				if(oDomain.oUniCastInfo.GetSize() == 0)
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u] isn't existed", nAcmDomain));
					return false;
				}
				if(oDomain.oUniCastInfo.Find(oDomain.nNode) == oDomain.oUniCastInfo.End())
				{
					FocpLog(FOCP_LOG_ERROR, ("The config 'AcmUniCast[%u-%u] isn't existed", nAcmDomain, oDomain.nNode));
					return false;
				}
			}
		}
		pIt = m_oDomainInfo.First();
		for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
		{
			CAcmUdp* pUdp;
			uint32 nDomain = m_oDomainInfo.GetKey(pIt);
			CAcmDomain& oDomain = m_oDomainInfo.GetItem(pIt);
			if(oDomain.nDomainType == 1)//¶à²¥
			{
				pUdp = new CAcmUdp(this, nDomain, oDomain.nNode, true, true);
				pUdp->AddNode(oDomain.nNode, oDomain.nMcItfAddr, oDomain.nMcItfPort);
				if(!pUdp->InitMultiCast(oDomain.nMultiCastPort, oDomain.nMultiCastAddr,
					oDomain.nMultiCastTTL, oDomain.bAllowLoop?true:false))
				{
					delete pUdp;
					CString oAddrName;
					CIpAddr oIpAddr = {oDomain.nMultiCastAddr, oDomain.nMultiCastPort};
					CFile::GetIpFileName(oIpAddr, oAddrName);
					FocpLog(FOCP_LOG_ERROR, ("Initialize AcmMultiCast(Domain=%u, Node=%u, MultiCastAddr=%s) failure", nDomain, oDomain.nNode, oAddrName.GetStr()));
					return false;
				}
			}
			else
			{
				uint16 nUniPort = oDomain.oUniCastInfo[oDomain.nNode].nPort;
				pUdp = new CAcmUdp(this, nDomain, oDomain.nNode, false, true);
				if(!pUdp->InitUniCast(nUniPort))
				{
					delete pUdp;
					FocpLog(FOCP_LOG_ERROR, ("Initialize AcmUniCast(Domain=%u, Node=%u, UniCastPort=%u16) failure", nDomain, oDomain.nNode, nUniPort));
					return false;
				}
				CRbTreeNode* pNode = oDomain.oUniCastInfo.First();
				CRbTreeNode* pEnd2 = oDomain.oUniCastInfo.End();
				for(; pNode!=pEnd2; pNode=oDomain.oUniCastInfo.GetNext(pNode))
				{
					CIpAddr &oAddr = oDomain.oUniCastInfo.GetItem(pNode);
					if(!pUdp->AddNode(oDomain.oUniCastInfo.GetKey(pNode), oAddr.nAddr, oAddr.nPort))
					{
						delete pUdp;
						return false;
					}
				}
			}
			oDomain.pUdp = pUdp;
			if(oDomain.nSerialAbility)
				oDomain.pSerialModule = new CAcmSequenceModule(pUdp, oDomain.nSerialAbility, oDomain.nSerialCapacity, oDomain.nSerialMaxAck);
		}

		if(m_oDomainInfo.GetSize())
		{
			m_pTokenMoudle = CAcmTokenModule::GetInstance(false);
			if(pConfigSystem->OpenConfig(oAccess, "AcmVip"))
			{
				oAccess.OpenIdxVal();
				while(oAccess.Query())
				{
					CIpAddress oAddr;
					sVal = oAccess.GetVal("Domain", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Domain' is invalid"));
						return false;
					}
					uint32 nDomain = CString::Atoi(sVal);
					if(CAcmUdp::QueryUdp(nDomain) == NULL)
						continue;
					sVal = oAccess.GetVal("Vip", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Vip' is invalid"));
						return false;
					}
					if(!oAddr.SetIp(sVal, false, false, false))
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Vip' is invalid"));
						return false;
					}
					CString oVip(sVal);
					sVal = oAccess.GetVal("Mask", nLength);
					if(sVal)
					{
						if(sVal[nLength-1])
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Mask' is invalid"));
							return false;
						}
						if(!oAddr.SetIp(sVal, false, false, false))
						{
							FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Mask' is invalid"));
							return false;
						}
					}
					CString oMask(sVal);
					sVal = oAccess.GetVal("IfMac", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.IfMac' is invalid"));
						return false;
					}
					CString oIfMac(sVal);
					sVal = oAccess.GetVal("Default", nLength);
					if(!sVal || sVal[nLength-1])
					{
						FocpLog(FOCP_LOG_ERROR, ("The config 'AcmVip.Default' is invalid"));
						return false;
					}
					uint32 nDefault = CString::Atoi(sVal);
					DelIpv4Addr(oVip.GetStr());
					if(!m_pVipModule)
						m_pVipModule = CAcmVipModule::GetInstance();
					if(!m_pVipModule->AddToken(nDomain, oVip.GetStr(), oMask.GetStr(), oIfMac.GetStr(), nDefault))
					{
						FocpError(("Add vip(%s/%s on %s) failure", oVip.GetStr(), oMask.GetStr(), oIfMac.GetStr()));
						return false;
					}
				}
			}
		}

		CCmdSystem* pCmdSys = CCmdSystem::GetInstance();
		pCmdSys->RegisterCmd("/Acm/LsDomain", "LsDomain [DomainId] <CR>:\r\n\t list the domain information", AcmCmdFunc_LsDomain);
		pCmdSys->RegisterCmd("/Acm/LsSerial", "LsSerial [DomainId] <CR>:\r\n\t list the domain serial information", AcmCmdFunc_LsSerial);
		pCmdSys->RegisterCmd("/Acm/LsToken", "LsToken <CR>:\r\n\t list token information", AcmCmdFunc_LsToken);

		return true;
	}

	virtual bool OnStart()
	{
		if(m_pTcp)
			m_pTcp->Start();
		if(m_oDomainInfo.GetSize())
		{
			m_oEvent.Reset();
			CRbTreeNode* pIt = m_oDomainInfo.First();
			CRbTreeNode* pEnd = m_oDomainInfo.End();
			for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
			{
				CAcmDomain& oDomain = m_oDomainInfo.GetItem(pIt);
				if(oDomain.pSerialModule)
					oDomain.pSerialModule->Start();
				oDomain.pUdp->Start();
			}
			if(m_pTokenMoudle)
			{
				if(m_pVipModule && !m_pVipModule->Start())
					return false;
				m_pTokenMoudle->Start();
			}
			m_oEvent.Wait(3000);
			if(m_bNodeRepeat)
				return false;
		}
		return true;
	}

	virtual void OnStop()
	{
		if(m_pTcp)
			m_pTcp->Stop(false);
		if(m_oDomainInfo.GetSize())
		{
			CRbTreeNode* pIt = m_oDomainInfo.First();
			CRbTreeNode* pEnd = m_oDomainInfo.End();
			for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
			{
				CAcmDomain& oDomain = m_oDomainInfo.GetItem(pIt);
				oDomain.pUdp->Stop(false);
				if(oDomain.pSerialModule)
					oDomain.pSerialModule->Stop(false);
			}
			if(m_pVipModule)
				m_pVipModule->Stop();
			if(m_pTokenMoudle)
				m_pTokenMoudle->Stop();
			pIt = m_oDomainInfo.First();
			for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
			{
				CAcmDomain& oDomain = m_oDomainInfo.GetItem(pIt);
				oDomain.pUdp->Stop();
				if(oDomain.pSerialModule)
					oDomain.pSerialModule->Stop();
			}
		}
		if(m_pTcp)
			m_pTcp->Stop();
	}

	virtual void OnCleanup()
	{
		if(m_pTokenMoudle)
		{
			delete m_pTokenMoudle;
			m_pTokenMoudle = NULL;
		}
		CRbTreeNode* pIt = m_oDomainInfo.First();
		CRbTreeNode* pEnd = m_oDomainInfo.End();
		for(; pIt!=pEnd; pIt=m_oDomainInfo.GetNext(pIt))
		{
			CAcmDomain& oDomain = m_oDomainInfo.GetItem(pIt);
			if(oDomain.pSerialModule)
				delete oDomain.pSerialModule;
			delete oDomain.pUdp;
		}
		m_oDomainInfo.Clear();
		if(m_pTcp)
		{
			m_pTcp->Cleanup();
			m_pTcp = NULL;
		}
	}
};

static void AcmCmdFunc_LsDomain(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CArguments oArgs;
	oArgs.SetCmdLine(sCmdLine);
	int32 nArgc = oArgs.GetArgc();
	if(nArgc == 1)
	{
		CVector<uint32> oDomains;
		CAcmUdp::GetAllDomains(oDomains);
		uint32 nSize = oDomains.GetSize();
		pSession->Print("There are %u domains:\r\n", nSize);
		for(uint32 i=0; i<nSize; ++i)
		{
			uint32 nDomain = oDomains[i];
			pSession->Print("\tDomain:%u\r\n", nDomain);
		}
		pSession->Print("\r\n");
	}
	else if(nArgc == 2)
	{
		const char* sArg = oArgs.GetArgv()[1];
		uint32 nDomain = CString::Atoi(sArg);
		CAcmUdp* pUdp = CAcmUdp::QueryUdp(nDomain);
		if(pUdp == NULL)
			pSession->Print("There isn't the domain '%u'\r\n", nDomain);
		else
		{
			if(pUdp->IsMultiCast())
			{
				CIpAddr oAddr;
				CString oFileName;
				uint32 nTTL = pUdp->GetMultiCastAddr(oAddr);
				CFile::GetIpFileName(oAddr, oFileName);
				pSession->Print("The domain %u is multicast domain, and the multicast addr is %s, and the TTL is %u\r\n", nDomain, oFileName.GetStr(), nTTL);
			}
			else
				pSession->Print("The domain %u is unicast domain\r\n", nDomain);
			CAcmService* pService = (CAcmService*)CServiceManager::GetInstance()->QueryService("AcmService");
			pService->SetSession(pSession);
			pUdp->Walk();
			uint32 nSize = pService->GetWalkSize();
			pSession->Print("There are %u nodes in the domain %u\r\n", nSize, nDomain);
		}
	}
}

static void AcmCmdFunc_LsSerial(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CArguments oArgs;
	oArgs.SetCmdLine(sCmdLine);
	int32 nArgc = oArgs.GetArgc();
	if(nArgc == 1)
		pSession->Print("This lack of domain parameters\r\n");
	else
	{
		const char* sArg = oArgs.GetArgv()[1];
		uint32 nDomain = CString::Atoi(sArg);
		CAcmUdp* pUdp = CAcmUdp::QueryUdp(nDomain);
		if(pUdp == NULL)
			pSession->Print("There isn't the domain '%u'\r\n", nDomain);
		else
		{
			CAcmSequenceModule* pModule = (CAcmSequenceModule*)pUdp->QueryModule(ACM_SERIAL_MODULE);
			if(pModule == NULL)
				pSession->Print("This domain %u doesn't support the serial function\r\n", nDomain);
			else
			{
				bool bSupportSend = pModule->SupportSend();
				bool bSupportRecv = pModule->SupportRecv();
				bool bSupportAck = pModule->SupportUniCastSend();
				uint32 nCapacity = 0, nRecvCapacity, nSendCapacity, nRecvSize, nSendSize;
				nRecvCapacity = pModule->GetRecvCapacity(nRecvSize);
				nSendCapacity = pModule->GetRecvCapacity(nSendSize);
				if(nRecvCapacity)
					nCapacity = nRecvCapacity;
				else if(nSendCapacity)
					nCapacity = nSendCapacity;
				pSession->Print("This domain %u", nDomain);
				if(bSupportSend)
				{
					pSession->Print(" supports serial send");
					if(bSupportAck)
						pSession->Print("(ack)");
				}
				if(bSupportRecv)
				{
					if(bSupportSend)
						pSession->Print(", and serial receive");
					else
						pSession->Print(" supports serial receive");
				}
				pSession->Print("\r\nThe capacity is %u\r\n", nCapacity);
				if(bSupportAck)
					pSession->Print("the send queue is %u\r\n", nSendSize);
				if(bSupportRecv)
					pSession->Print("the receive queue is %u\r\n", nRecvSize);

				CAcmService* pService = (CAcmService*)CServiceManager::GetInstance()->QueryService("AcmService");
				pService->SetSession(pSession);
				pModule->Walk(*pService);
				uint32 nSize = pService->GetWalkSize();
				pSession->Print("There are %u nodes in the serial domain %u\r\n", nSize, nDomain);
				pUdp->ReleaseModule(ACM_SERIAL_MODULE);
			}
		}
	}
}

static void AcmCmdFunc_LsToken(CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg)
{
	CAcmTokenModule* pModule = CAcmTokenModule::GetInstance();
	if(pModule == NULL)
		pSession->Print("The system doesn't support the token function\r\n");
	else
	{
		CAcmService* pService = (CAcmService*)CServiceManager::GetInstance()->QueryService("AcmService");
		pService->SetSession(pSession);
		pModule->Walk(*pService);
		uint32 nSize = pService->GetWalkSize();
		pSession->Print("There are %u tokens in the system\r\n", nSize);
	}
}

static CAcmService g_oAcmService;

FOCP_END();
