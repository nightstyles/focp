
#include "AcmDef.hpp"

#ifndef _ACM_TELNET_HPP_
#define _ACM_TELNET_HPP_

/*
telnet
NAME                    CODE                MEANING
NULL                    0                   �޶���
Line Feed (LF)          10                  ���У�X���겻��
Carriage Return (CR)    13                  �س�����X�����0
BELL (BEL)              7                   ����
Back Space (BS)         8                   ����
Horizontal Tab (HT)     9                   X��TAB
Vertical Tab (VT)       11                  Y��TAB
Form Feed (FF)          12                  �Ƶ���һҳ��X�᲻��
1B = ESC
F1                      1B 4F 50 (OP)
F2                      1B 4F 51 (OQ)
F3                      1B 4F 52 (OR)
F4                      1B 4F 53 (OS)
��                      1B 4F 41 (OA)
��                      1B 4F 42 (OB)
��                      1B 4F 43 (OC)
��                      1B 4F 44 (OD)
m��nΪ����������ʱΪASCII
ESC[m;nH                                    ��궨�ڵ�m�е�n��
ESC[m;nf                                    ��궨�ڵ�m�е�n��
ESC[nA                                      �������n��
ESC[nB                                      �������n��
ESC[nC                                      �������n��
ESC[nD                                      �������n��
ESC[s                                       ���浱ǰ���λ��
ESC[u                                       �ָ��ղŹ��λ��
ESC[J                                       �����Ļ���ӵ�ǰλ�õ���Ļ���·���
ESC[K                                       �����ǰ�Ұ��У�����ǰλ�ã�
*/

FOCP_BEGIN();

class CAcmTelnetServer;

struct CTelnetContext
{
	int32 n;
	uint8 c;
	char cmd[4096], *oldcmd;
	int32 l, oldl, is_telnet_option, skip, esc;
	int32 cursor, insertmode;
};

//CFiber�����ǵ�һ������
class ACM_API CAcmTelnetLink: public CFiber, public CStreamAssembler, public CCmdSession
{
	friend class CAcmTelnetServer;
private:
	CCmdSystem* m_pCmdSystem;
	CTelnetContext m_oContext;
	bool m_bPrompt;
	char m_cPrompt;
	uint8 m_sBuf[1024];
	int32 m_nBufLen, m_nIdx;
	CAcmTelnetServer* m_pServer;
	uint32 m_hLink;
	bool * m_pRunning;

	CMutex m_oMutex;
	CEvent m_oEvent;
	uint32 m_nPos;
	CMemoryStream m_oStream;
	CHistoryCmd m_oHistoryCmd;

public:
	CAcmTelnetLink();
	virtual ~CAcmTelnetLink();

	void Initialize(uint32 hLink, CAcmTelnetServer* pServer, CCmdSystem* pCmdSystem);

	//for fiber
	virtual uint32 GetClassId() const;
	virtual int32 GetWorkerGroupId();
	virtual void Reset();

	//for CCmdSession
	virtual void Print(const char *sFormat, ...);
	virtual void PrintV(const char* sFormat, CVaList& pArgList);
	virtual void ClearScreen();
	virtual const char* ReadLine(bool bPrompt, char cPrompt);

	//for CStreamAssembler
	virtual int32 OnAssembler(const char* sBuf, int32 nBufLen, CMemoryStream& oMsg);
	virtual void UnBindLink();

protected:
	//for fiber
	virtual void MainProc(bool &bRunning);
	virtual void StopNotify();

private:
	int32 ReadChar(const bool &bRunning, uint8 & c);
	bool ReadLine(const bool &bRunning, bool bPrompt);
	void ClearLine(char* cmd, int32 l, int32 cursor);
	static void History(CCmdSystem* pCmdSystem, CCmdSession* pSession, const char *sCmdLine, const char* sCmdArg);
};

class ACM_API CAcmTelnetServer: public CCooperateFunction
{
	friend class CAcmTelnetLink;
private:
	CFile m_oLsnTcp;
	CIocpServer* m_pIocpServer;
	CCmdSystem* m_pCmdSystem;
	CCooperator m_oLsnThread;
	uint32 m_hLink;

public:
	CAcmTelnetServer();
	virtual ~CAcmTelnetServer();

	bool Initialize(uint16 nServerPort);
	void Cleanup();

	void Start();
	void Stop(bool bBlock = true);

protected:
	virtual void ProcessOnce(CCooperator* pCooperator, bool &bRunning);
};

FOCP_END();

#endif
