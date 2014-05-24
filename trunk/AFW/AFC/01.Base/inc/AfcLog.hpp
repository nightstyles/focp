
#include "AfcThread.hpp"
#include "List.hpp"

#ifdef WINDOWS
#include <windows.h>
#else
#include <netinet/in.h>
#endif

#include <stdio.h>

#ifndef _AFC_LOG_HPP_
#define _AFC_LOG_HPP_

/*
��־������Ʒ�����
1. ������AfcBaseģ�飬��Ϊ����ϵͳһ����������־������
2. ��־ģ��ĳ�ʼ�����ڲ��Զ����еģ������ⲿ�ر���뼶֧�֡�
3. ��־��ʼ���Ĳ���ȫ��ͨ�������������ݣ�Ϊ��2���ṩ�������ϡ�
3. ��־ģ��������ṩ����������
	void WriteLog(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, ...);
	void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog1(const char* sModuleName, uint32 nLevel, CString &oFileName, const char *sFunction, int nLine);
	void WriteLog2(const char* sLogInfo, ...);

	//sModuleNameΪNULL��'\0'��ʾȫ����־����
	void SetLogMode(const char* sModuleName, uint32 nMode);
	uint32 GetLogMode(const char* sModuleName);
	void SetLogLevel(const char* sModuleName, uint32 nLevel);
	uint32 GetLogLevel(const char* sModuleName);
	void SetLogFilter(const char* sModuleName, uint32 nLogFilter);
	uint32 GetLogFilter(const char* sModuleName);
	void SetLogHold(const char* sModuleName, bool bHold);
	bool GetLogHold(const char* sModuleName);

4. ��־��ʼ�������壺
	FocpLogLevel������ȫ�ֵ���־����ȱʡֵΪFOCP_LOG_ERROR.
		FOCP_LOG_SYSLOG=1, //����Ҫ��ӡ����־��������ʾ����
		FOCP_LOG_ERROR=2,  //������Ϣ
		FOCP_LOG_WARNING=3,//�澯��Ϣ
		FOCP_LOG_DEBUG=4,  //������Ϣ
		FOCP_LOG_CLOSE=5,  //
	FocpLogMode��������־��ӡ��ʽ����λ�ϣ���ȱʡΪ1
		FOCP_LOG_SCREEN=1, //��Ļ��ӡ��־
		FOCP_LOG_FILE=2, //���ļ��д�ӡ��־
			FocpLogFileSize, ������־�ļ�������С����λM����ȱʡ10.
			FocpLogFileNo����������ļ��ţ�ȱʡ999������001~999��
		FOCP_LOG_SERVER=4, //����־������־������
			FocpLogServerAddr��������־��������ַ��ȱʡΪ127.0.0.1
			FocpLogServerPort��������־�������˿ڣ�ȱʡΪ2269
	FocpLogFilter��������־����ģʽ����λ�ϣ���ȱʡΪ15
		FOCP_LOG_HOST=1, //����ӡ������Ϣ
		FOCP_LOG_DATE=2, //����ӡʱ����Ϣ
		FOCP_LOG_APPN=4, //����ӡӦ����Ϣ[����ʵ����]
		FOCP_LOG_MODU=8, //����ӡģ����Ϣ
		FOCP_LOG_SRCF=16,//����ӡ�ļ���Ϣ[�����к�]����������ñ�ʾ����ӡ������־ʱ�����У�����2���ո�
		FOCP_LOG_FUNC=32,//����ӡ������Ϣ����������ñ�ʾ����ӡ������־ʱ�����У�����2���ո�
5. ��־��������ƣ��������
*/

FOCP_BEGIN();

struct  CAfcLogAttr
{
	uint32 nMode;
	uint32 nLevel;
	uint32 nFilter;
};

struct CLogInfo
{
	char sDate[20];
	char sLogInfo[FOCP_LOG_MAX];
	const char* sAppName;
	const char* sModuleName;
	const char* sFileName;
	const char* sFunction;
	uint32 nLevel, nLine;
};

struct CLogPosInfo
{
	const char* sAppName;
	const char* sModuleName;
	const char* sFileName;
	const char* sFunction;
	uint32 nLevel, nLine;
};

struct CModuleName
{
	char* sName;

	~CModuleName();
	CModuleName(const char* s=NULL);
	CModuleName(const CModuleName& oSrc);
	CModuleName& operator=(const CModuleName& oSrc);
	CModuleName& operator=(const char* s);
	bool Empty();
};

struct CModuleNameCompare
{
	static int32 Compare(const CModuleName*pLeft, const CModuleName* pRight);
};

class CLogManager
{
private:
	CRbMap<CModuleName, CAfcLogAttr, CModuleNameCompare> m_oLogAttr;//��ģ�����
	CSingleList<CLogInfo> m_oLogBuffer;
	CLogPosInfo m_oLogPos;
	uint32 m_nLogFileSize;
	uint32 m_nLogFileNo;
	uint32 m_nDmn, m_nAin;
	char m_sHostIp[20];
	bool m_bHold, m_bLocked;

	char m_sHome[FOCP_MAX_PATH];
	char* m_sApp;
	char m_sName[FOCP_MAX_PATH];
	char m_sFileName[FOCP_MAX_PATH];
	FILE* m_pFile;
	uint32 m_nFileId;

#ifdef WINDOWS
	SOCKET m_nSocket;
#endif

#ifdef UNIX
	int32 m_nSocket;
#endif

	sockaddr_in m_oServerAddr;
	FOCP_DETAIL_NAME::CThreadMutex m_oMutex;

public:
	CLogManager();
	~CLogManager();

	void SetLogMode(const char* sModuleName, uint32 nMode);
	uint32 GetLogMode(const char* sModuleName);

	void SetLogLevel(const char* sModuleName, uint32 nLevel);
	uint32 GetLogLevel(const char* sModuleName);

	void SetLogFilter(const char* sModuleName, uint32 nLogFilter);
	uint32 GetLogFilter(const char* sModuleName);

	void SetLogInstance(uint32 nDmn, uint32 nAin);

	void WriteLogV(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog1(const char* sModuleName, uint32 nLevel, const char* sFileName, const char *sFunction, int nLine);
	void WriteLog2(const char* sLogInfo, CVaList& args);

	bool HaveModule(const char* sModuleName);
	void WalkLogModule(void *pPara, void(*OnWalk)(void*pPara, const char* sModule));

	void Lock();
	void UnLock();

private:
	CAfcLogAttr& GetLogAttr(const char* sModuleName);
	void GetLogDate(char sDate[20]);
	void GetFileDate(char sDate[20]);

	uint32 CreateLogMsg(CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG]);
	uint32 GetLogInfo(uint32 nFilter, CLogInfo& oLogInfo, char sLogInfo[FOCP_LOG_MAXMSG]);
	void CreateLogFile();
	bool NotAccess();

	void PrintScreen(uint32 nLevel, char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);
	void PrintFile(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);
	void PrintServer(char sLogInfo[FOCP_LOG_MAXMSG], uint32 nLen);

	void WriteLogV2(const char* sModuleName, uint32 nLevel, const char *sFileName, const char *sFunction, int nLine, const char* sLogInfo, CVaList& argptr);
	void WriteLog(CLogInfo &oLogInfo, bool bSupportNet=true);

	bool GetHostIpList(char sHostIp[20]);
};

FOCP_END();

#endif
