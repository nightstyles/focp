
#include "AcmDef.hpp"

#ifndef _ACM_TCP_HPP_
#define _ACM_TCP_HPP_

FOCP_BEGIN();

struct CTcpHead
{
	uint32 nSize;
	uint32 nMagic;
	uint32 nModule;//协议族
	uint32 nCmd;
};

class CAcmTcpServer;

class ACM_API CAcmTcpLink: public CStreamAssembler
{
	friend class CAcmTcpServer;
private:
	uint32 m_hLink;
	CMemoryStream m_oStream;
	uint32 m_nSize;
	CAcmTcpServer* m_pServer;
	CAcmTcpLink *m_pNext, *m_pPrev;
	uint32 m_nStatus;//0=Free,1=AUTH,2=INIT,3=WORKING
	bool m_bHello;
	CMutex m_oMutex;

public:
	CAcmTcpLink(CAcmTcpServer* pServer);
	virtual ~CAcmTcpLink();

	uint32 GetStatus();
	void Send(CMemoryStream &oStream);
	void Close();

	void CreateMsgHead(CMemoryStream &oStream, uint32 nMagic, uint32 nModule, uint32 nCmd);
	void BuildMsgHead(CMemoryStream &oStream);

	virtual int32 OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg);
	virtual void UnBindLink();

private:
	bool SendStream(CMemoryStream &oStream);
	void Hello(uint32 nMagic);
};

class ACM_API CAcmTcpModule
{
	FOCP_FORBID_COPY(CAcmTcpModule);
	friend class CAcmTcpServer;
public:
	CAcmTcpModule();
	virtual ~CAcmTcpModule();

	virtual void ProcessAcmModuleMsg(CAcmTcpLink* pLink, uint32 nCmd, CMemoryStream& oStream) = 0;
};

class ACM_API CAcmTcpServer: public CCooperateFunction
{
	friend class CAcmTcpLink;
private:
	CFile m_oLsnTcp;
	CIocpServer* m_pIocpServer;
	CCooperator m_oLsnThread, m_oHelloThread;
	CThreadPool m_oProcThreadPool;
	CMutex m_oMutex;
	CObjectRegistry<CAcmTcpModule, uint32> m_oModules;
	CBaseSingleList<CAcmTcpLink> m_oContainer;
	CBaseDoubleList<CAcmTcpLink> m_oRunning;
	uint32 m_nMagic, m_hLink;
	CAcmTcpLink* m_pLink;
	uint16 m_nServerPort;

public:
	CAcmTcpServer();
	virtual ~CAcmTcpServer();

	static CAcmTcpServer* GetInstance();

	//应用注册的模块,应用模块不能为0xABCDEFAB
	void RegisterModule(uint32 nModule, CAcmTcpModule* pModule);
	void DeRegisterModule(uint32 nModule);
	CAcmTcpModule* QueryModule(uint32 nModule);
	void ReleaseModule(uint32 nModule);

	uint32 Initialize(uint16 nServerPort, uint32 nWorkerNum=100, uint32 nMagic=ACM_TCP_MAGIC);
	void Cleanup();

	uint16 GetPort();

	uint32 Start(bool bNeedHello = true);
	void Stop(bool bBlock = true);

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private:
	CAcmTcpLink* AllocLink();
	void FreeLink(CAcmTcpLink* pLink);
	void CloseAll();
	void DestroyAllLinks();
	void Hello();
	bool ProcessLogin(CFile* pFile);
	void ProcessMsg(uint32 hLink, CMemoryStream &oStream);
};

class CAcmTcpClient;
class ACM_API CAcmTcpClientContext
{
	FOCP_FORBID_COPY(CAcmTcpClientContext);
public:
	CAcmTcpClientContext();
	virtual ~CAcmTcpClientContext();

	virtual void ProcessMsg(CAcmTcpClient* pClient, CTcpHead& oHead, CMemoryStream &oStream) = 0;
	virtual uint32 OnLogin(CAcmTcpClient* pClient, bool bReLogin) = 0;
	virtual bool OnReConnect(CAcmTcpClient* pClient);
};

enum
{
	FOCP_SESSION_BREAK=3
};

class ACM_API CAcmTcpClient: public CCooperateFunction, public CSyncCaller
{
private:
	CFile m_oTcp;
	CMutex m_oMutex;
	CString m_oServerAddr;
	uint16 m_nServerPort;
	uint32 m_nMagic, m_nStatus;
	bool m_bConnected, m_bLogin, m_bHello, m_bFailure, m_bConnectFailure;
	CCooperator m_oRecvThread, m_oConnThread, m_oHelloThread;
	CAcmTcpClientContext* m_pContext;

public:
	CAcmTcpClient(CAcmTcpClientContext* pContext);
	virtual ~CAcmTcpClient();

	void Initialize(uint32 nServerAddr, uint16 nServerPort, uint32 nMagic=ACM_TCP_MAGIC);
	void Initialize(const char* sServerAddr, uint16 nServerPort, uint32 nMagic=ACM_TCP_MAGIC);
	void Cleanup();

	bool Start(bool bNeedHello = true);
	void Stop(bool bBlock=true);

	bool Send(char* sMsg, uint32 nSize, bool bHello=false);
	bool Send(CMemoryStream& oStream, bool bHello=false);

	const char* GetServerAddr();
	uint16 GetServerPort();
	uint32 GetMagic();

	void CreateMsgHead(CMemoryStream &oStream, uint32 nMagic, uint32 nModule, uint32 nCmd);
	void BuildMsgHead(CMemoryStream &oStream);

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);

private:
	bool Recv(char* sMsg, uint32 nSize);
	void Connect(bool bReConnect);
	void Hello();
	uint32 Login(bool bFirst);
	void FinishInitialize();
	void ReturnLogin(CMemoryStream &oStream);
	void ProcessMsg(CTcpHead& oHead, CMemoryStream &oStream);
};

FOCP_END();

#endif

