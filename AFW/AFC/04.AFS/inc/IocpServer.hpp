
#include "File.hpp"

#ifndef _Afs_IocpServer_Hpp_
#define _Afs_IocpServer_Hpp_

FOCP_BEGIN();

class AFS_API CIpcoQueue
{
	FOCP_FORBID_COPY(CIpcoQueue);
	struct CMsgItem
	{
		uint32 hLink;
		CMemoryStream oStream;//当为空时表示该链路有问题，需要关闭
		CMsgItem* pNext;
	};
private:
	CMutex m_oMutex;
	CEvent m_oEvent;
	CBaseSingleList<CMsgItem> m_oMsgQueue;

public:
	CIpcoQueue();
	virtual ~CIpcoQueue();

	//返回0：超时
	//返回>0: 返回消息大小（小于等于nBufLen，表示读完）
	int32 ReadFrom(void* pBuf, int32 nBufLen, uint32 &hLink);
	bool ReadFrom(CMemoryStream& oStream, uint32 &hLink);

protected:
	void PushMsg(uint32 hLink, CMemoryStream &oStream);
};

class AFS_API CStreamAssembler
{
	FOCP_FORBID_COPY(CStreamAssembler);
public:
	CStreamAssembler();
	virtual ~CStreamAssembler();

	//如果oMsg非空表示，已解析出一个完整消息，CIocpServer将把该消息放入CIpcoQueue中
	//返回未装配的大小，如果非0，且oMsg为空，表示装配出错
	virtual int32 OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg);

	virtual void UnBindLink();
};

struct CIocpLinkItem;
class AFS_API CIocpServer: public CCooperateFunction, public CIpcoQueue
{
	friend struct CSingleInstance<CIocpServer>;
public:
	struct CLinkContainer
	{
		CEvent oEvent;
		CBaseSingleList<CIocpLinkItem> oContainer;

		CLinkContainer();

		CIocpLinkItem* AllocLink();
		void FreeLink(CIocpLinkItem* pLink);
		void Clear();
	};

	struct CLinkMap
	{
#if defined(WINDOWS) || defined(CYGWIN_NT)
		CCooperator oThread;
		CEvent oEvent;
		CBaseDoubleList<CIocpLinkItem> oTable;
#else
		CRbMap<ulong, CIocpLinkItem*> oTable;
#endif

		CLinkMap(CCooperateFunction* pFun);

#if defined(WINDOWS) || defined(CYGWIN_NT)
		CIocpLinkItem* AllocLink();
		void PushLink(CIocpLinkItem* pLink);
		void RemoveLink(CIocpLinkItem* pLink);
#else
		void PushLink(ulong hFile, CIocpLinkItem* pLink);
		void RemoveLink(ulong hFile);
		CIocpLinkItem* GetLinkItem(int32 hFile);
#endif
		void Clear();
	};

private:
	//IocpHandle
	CMutex m_oIocpMutex;
	ulong m_hIocpHandle;
	//Container
	CMutex m_oMutex;
	uint32 m_nMaxLink;
	CIocpLinkItem * m_pLinks;
	CLinkContainer m_oContainer;
	//Map
	CLinkMap m_oLinkMap;

	CIocpServer();
public:
	~CIocpServer();

	static CIocpServer* GetInstance();

	bool Initialize(uint16 nMaxLink);
	void Cleanup();

#if defined(WINDOWS) || defined(CYGWIN_NT)
	void Start();
	void Stop(bool bBlock);
#endif

	uint32 AllocLink();
	void FreeLink(uint32 hLink);
	bool BindLink(uint32 hLink, ulong hHandle, CStreamAssembler* pAssembler);
	int32 SendTo(uint32 hLink, const void* pBuf, int32 nBufLen);
	CStreamAssembler* GetAssember(uint32 hLink);

#if defined(WINDOWS) || defined(CYGWIN_NT)
	void Listen(ulong hHandle);
	ulong Accept(ulong hHandle, CIpAddr &oFromAddr);
#endif

protected:
	virtual void StopNotify(CCooperator* pCooperator);
	virtual void MainProc(CCooperator* pCooperator, bool &bRunning);
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private:
	CStreamAssembler* CloseLink(CIocpLinkItem* pLink);
	CStreamAssembler* ReadLink(CIocpLinkItem* pLink, uint32 nBytes);
};

FOCP_END();

#endif
