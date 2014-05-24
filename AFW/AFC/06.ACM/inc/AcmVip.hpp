
#include "AcmToken.hpp"

#ifndef _ACM_VIP_HPP_
#define _ACM_VIP_HPP_

FOCP_BEGIN();

class ACM_API CAcmVipToken: public CAcmToken
{
private:
	CString m_nVip;
	CString m_nMask;
	CString m_nIfName;

public:
	CAcmVipToken(uint32 nDomain, const char* sVip, const char* sMask, const char* sIfName, uint8 nWeight);
	virtual ~CAcmVipToken();

	const char* GetVip() const;

protected:
	virtual void OnRoleChange(bool bTakeUp);
};

class CAcmVipModule;

class ACM_API CAcmVipEvent
{
	friend class CAcmVipModule;
public:
	CAcmVipEvent();
	virtual ~CAcmVipEvent();

protected:
	virtual void OnEvent(CAcmVipToken* pToken, bool bTakeUp);
};

class ACM_API CAcmVipModule
{
	friend class CAcmVipToken;
private:
	CAcmTokenModule* m_pTokenModule;
	CMutex m_oMutex;
	bool m_bStarted;
	CRbMap<CString, CAutoPointer<CAcmVipToken> > m_oVipTokens;
	CSingleList<CAcmVipEvent*> m_oEvents;

public:
	CAcmVipModule();
	~CAcmVipModule();

	static CAcmVipModule* GetInstance();

	bool AddToken(uint32 nDomain, const char* sVip, const char* sMask, const char* sIfName, uint32 nDefaultNode);

	void RegisterEvent(CAcmVipEvent* pEvent);
	void DeRegisterEvent(CAcmVipEvent* pEvent);

	CAcmVipToken* QueryVip(const char* sVip);

	bool Start();
	void Stop();

private:
	void OnEvent(CAcmVipToken* pToken, bool bTakeUp);
};

FOCP_END();

#endif
