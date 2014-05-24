
#include "AfsDef.hpp"
#include "DynamicLibrary.hpp"

#ifndef _Afs_File_Hpp_
#define _Afs_File_Hpp_

FOCP_BEGIN();

enum
{
	FOCP_INVALID_FILE = 0,

	FOCP_STORAGE_FILE = 1,
	FOCP_LISTEN_FILE = 2,
	FOCP_CONNECT_FILE = 3,
	FOCP_NONCONNECT_FILE = 4,
	//����ѡ����ܱ�ͬʱ����
	FOCP_STREAM_FILE = 16,
	FOCP_PACKAGE_FILE = 32,
};

enum
{
	FOCP_FILE_NORMAL = 0,
	FOCP_FILE_CLOSED = 1,
	FOCP_FILE_BROKEN = 2,
	FOCP_FILE_BAD = 3,
};

enum
{
	FOCP_FILE_LOCK = 1,//�����ļ�
	FOCP_FILE_TLOCK = 2,//���Լ����ļ�������Ѿ���������ʧ��
	FOCP_FILE_ULOCK = 3,//�����ļ�
};

enum
{
	FOCP_READ_BUFFER = 1,
	FOCP_WRITE_BUFFER = 2,
	FOCP_BOTH_BUFFER=3
};

enum
{
	FOCP_FILE_OPTION_READ = 1,
	FOCP_FILE_OPTION_WRITE = 2,
	FOCP_FILE_OPTION_CREATE = 4,
	FOCP_FILE_OPTION_DESTROY = 8,
	FOCP_FILE_OPTION_APPEND = 16,
	FOCP_FILE_OPTION_NEW = 32,
	FOCP_FILE_OPTION_LISTEN = 64,
	FOCP_FILE_OPTION_SHARE_READ = 128,
	FOCP_FILE_OPTION_SHARE_WRITE = 256,
	FOCP_FILE_OPTION_TTL = 1024,
	FOCP_FILE_OPTION_LOOP = 2048,
};

enum
{
	FOCP_FILE_PROTOCOL_ERROR = -1,
	FOCP_FILE_BINDNAME_ERROR = -2,
	FOCP_FILE_BIND_ERROR = -3,
	FOCP_FILE_LISTEN_ERROR = -4,
	FOCP_FILE_CONNECTNAME_ERROR = -5,
	FOCP_FILE_CONNECT_ERROR = -6,
	FOCP_FILE_OPTION_ERROR = -7,
	FOCP_FILE_OTHER_ERROR = -8,
};

enum
{
	FOCP_SEEK_SET = 0,
	FOCP_SEEK_CUR = 1,
	FOCP_SEEK_END = 2,
};

class CFileName;
class CFileInterface;
class CBaseFile;
class CFile;

class AFS_API CFileName
{
public:
	CString oProtocol;
	CString oBindName;
	CString oConnectName;

	bool Parse(const char* sFileName);
};

class AFS_API CFileInterface: public CInterface
{
public:
	CFileInterface();
	virtual ~CFileInterface();

	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);

	static CInterfaceManager* GetInterfaceManager();

protected:
	virtual const char* GetInterfaceName();
};

struct AFS_API CIpAddr
{
	uint32 nAddr;
	uint16 nPort;
};

struct AFS_API CIpAddrList
{
	CVector<uint32> oAddrList;
	uint16 nPort;
};

//����CBaseFileʵ�ֵľ����ļ�����������Ҫ���ǲ������ʵİ�ȫ����
class AFS_API CBaseFile
{
	FOCP_FORBID_COPY(CBaseFile);
	friend class CFile;
	friend class CCompletePort;
private:
	uint32 m_nStatus;

public:
	CBaseFile();
	virtual ~CBaseFile();

	virtual CFileInterface* GetInterface();

	//sOption:
	// r=��,���ڴ洢�ļ����ԣ��ļ������ڻ�ʧ��
	// w=д,���ڴ洢�ļ����ԣ��ļ������ڻ�ʧ�ܣ�����Я�����������
	//	c=�ļ������ڣ�������
	//	d=�ļ����ڣ����������
	//  a=��׷�ӷ�ʽд
	//	n=�������ļ����ļ����ڽ�ʧ��
	// l[:n]=����ѡ����û������sBindName���᷵��ʧ�ܣ���ָ������������
	// e=��ռģʽ��ȱʡ�ǹ����дģʽ
	// s=�����ģʽ��ȱʡ�ǹ����дģʽ
	// t[:n]=�鲥TTLѡ��
	// m=�鲥�ػ���ʶ
	//�����룺0�ɹ�������������
	// -1: Э�鲻����
	// -2: ��Ч����
	// -3: ��ʧ��
	// -4: ��Ч������
	// -5: ����ʧ��
	// -6: ѡ�����
	// -7: ��������
	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong * pHandle=NULL);

	virtual uint32 GetType() = 0;
	virtual uint32 GetStatus();//������������Ҫʵ�֡�ͨ��SetStatus����

	virtual const CFileName& GetFileName() = 0;

	//���ڴ洢�ļ����ԣ���д������һ��ģ���������ΪFOCP_BOTH_BUFFER��
	//���ڹܵ��ļ����ԣ���д�����Ƿ���ģ���ָ��ΪFOCP_BOTH_BUFFER���������嶼����Ϊͬ����С��
	//	����ʹ��FOCP_READ_BUFFER��FOCP_WRITE_BUFFER�ֱ����ò�ͬ�Ļ����С��
	//�����֧�ֻ���ģ����Բ���
	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);

	//�������ӵ��ļ���֧�����¶�д����ʽ����ѡ����������д����
	//0:�ļ�����������pBufΪNULL��nBufLenΪ0����-1:IO�쳣��-2:��ʱ��>0:���س���
	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut=0xFFFFFFFF);
	//0:�ļ��жϡ������������أ�-1:IO�쳣��>0:���س���
	virtual int32 Write(const void* pBuf, int32 nBufLen);

	//��������ӵ��ļ���֧�����¶�д
	//0:�ļ�����������pBufΪNULL��nBufLenΪ0����-1:IO�쳣��-2:��ʱ��>0:���س���
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut=0xFFFFFFFF);
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut=0xFFFFFFFF);
	//0:�ļ��жϡ������������أ�-1:IO�쳣��>0:���س���
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr);//������ȫ

	//�����������ļ���֧�����º���
	virtual bool Accept(CFile &oFile);

	//���ڴ洢�ļ����ԣ�֧�����������������
	virtual int32 GetPosition();
	virtual void SetPosition(int32 nPos);
	virtual void Seek(uint32 nOrigin, int32 nOffset);

	//���ڴ洢�ļ����ԣ�֧��������������
	virtual void Truncate();
	virtual bool Lock(uint32 nLock, int32 nSize);
	virtual void Flush();

protected:
	uint32 GetOpenOption(const char* sOption, uint32 *pListenCount=NULL);
	virtual void SetStatus(uint32 nStatue);
};

class AFS_API CFile: public CBaseFile
{
	FOCP_FORBID_COPY(CFile);
	friend class CFileName;
private:
	CBaseFile* m_pBaseFile;
	bool m_bOwned;

public:
	CFile(const char* sFileName, const char* sOption=NULL);
	CFile(CBaseFile* pFile=NULL, bool bOwned=true);
	virtual ~CFile();

	virtual CFileInterface* GetInterface();

	//sFileName�ĸ�ʽΪ��PROTOCOL '://' [ CONNECTNAME ] [ '|' [ BINDNAME ] ]
	// ���磺tcp://www.newpost.com:2020|192.168.1.101:3333
	//			udp://|0.0.0.0:4444
	//				������ӵ�ַ��һ��������ַ����ô�󶨵�ַ���ǽӿڵ�ַ��
	//				sOption�����t[:n]ָ��TTL��r��֧���鲥�ػ���
	//			disk:///home/caowenke/G4/FiberTest/cfg/FiberConfig.dat
	int32 Open(const char* sFileName, const char* sOption=NULL);

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong * pHandle=NULL);//������ȫ

	void Redirect(CBaseFile* pFile, bool bOwned=true);

	virtual const CFileName& GetFileName();//������ȫ

	virtual uint32 GetType();//������ȫ
	virtual uint32 GetStatus();//������ȫ

	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);//������ȫ

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut=0xFFFFFFFF);//������ȫ
	virtual int32 Write(const void* pBuf, int32 nBufLen);//������ȫ

	int32 ReadFrom(void* pBuf, int32 nBufLen, CString &oFileName, uint32 nTimeOut=0xFFFFFFFF);//������ȫ
	int32 WriteTo(const void* pBuf, int32 nBufLen, const char* sFileName);//������ȫ

	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut=0xFFFFFFFF);//������ȫ
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut=0xFFFFFFFF);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName);//������ȫ
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr);//������ȫ

	int32 Read(CMemoryStream& oStream, int32 nReadSize, uint32 nTimeOut=0xFFFFFFFF);//������ȫ
	int32 Write(CMemoryStream& oStream, int32 nWriteSize);//������ȫ

	//�����������ļ���֧�����º���
	virtual bool Accept(CFile &oFile);

	virtual int32 GetPosition();//������ȫ
	virtual void SetPosition(int32 nPos);//������ȫ
	virtual void Seek(uint32 nOrigin, int32 nOffset);//������ȫ

	virtual void Truncate();//������ȫ
	virtual bool Lock(uint32 nLock, int32 nSize);//������ȫ
	virtual void Flush();

	void Swap(CFile& oSrc);

	static uint32 GetIpAddr(const char* sIpAddr);
	static bool GetIpAddrList(const char* sFileName, CIpAddrList& oAddrList);
	static void GetIpFileName(const CIpAddr& oAddr, CString &oIpFileName, bool bFriendly=false);
	static bool IsMulticastAddr(uint32 nAddr);//���õ�ַ�Ƿ������Ϊ������ַ
	static bool IsMulticastAddr(const char* sAddr);
	static bool GetHostName(char sHostName[256]);
	static bool GetHostIpList(CString &oHostIpList);
	static bool CheckHostIp(uint32 nIp);
	static bool CheckHostIp(const char* sIpAddr);
	static ulong GetHostId();
	static bool GetMacAddress(char sMac[7], bool bWalk=true);

protected:
	virtual void SetStatus(uint32 nStatue);

private:
	void DestroyFile(bool bLock=true);
};

AFS_API CFile& GetStdIn();
AFS_API CFile& GetStdOut();
AFS_API CFile& GetStdErr();

//��ʽ���������ض���������
//-1:IO����, -3:��������
AFS_API int32 Scan(const char* sFormat, ...);
AFS_API int32 ScanV(const char* sFormat, CVaList& pArgList);

//��ʽ��д������д����ܳ��ȡ�
//-1:IO����, -2:IO�жϻ���, -3:��������
AFS_API int32 Print(const char* sFormat, ...);
AFS_API int32 PrintError(const char* sFormat, ...);
AFS_API int32 PrintV(const char* sFormat, CVaList& pArgList);
AFS_API int32 PrintErrorV(const char* sFormat, CVaList& pArgList);
AFS_API int32 PrintEx(bool bError, const char* sFormat, ...);
AFS_API int32 PrintExV(bool bError, const char* sFormat, CVaList& pArgList);

class AFS_API CFileFormatter: public CFormatter
{
	struct CFileBuffer
	{
		char* m_sBuffer;
		int32 m_nCount;
		int32 m_nOffset;
	};
private:
	CFile* m_pFile;
	int32 m_nBufSize;
	CFileBuffer m_oReadBuffer;
	CFileBuffer m_oWriteBuffer;
	bool m_bText, m_bHaveCr;

public:
	CFileFormatter(CFile* pFile, int32 nBufSize=4096, bool bText=true, CFormatterMethod* pMethod=NULL);
	virtual ~CFileFormatter();

protected:
	virtual int32 ReadChar(char &nChar);
	virtual int32 WriteChar(char nChar);
	virtual void Flush();
};

class AFS_API CFormatFile: public CFile, public CFileFormatter
{
public:
	CFormatFile(int32 nBufSize=4096, bool bText=true, CFormatterMethod* pMethod=NULL);
	virtual ~CFormatFile();

	virtual void Close(ulong * pHandle=NULL);//???????

//	virtual void Flush();
};

FOCP_END();

#define FocpStdIn (FOCP_NAME::GetStdIn())
#define FocpStdOut (FOCP_NAME::GetStdOut())
#define FocpStdErr (FOCP_NAME::GetStdErr())

#endif
