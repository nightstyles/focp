
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
	//以下选项可能被同时包含
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
	FOCP_FILE_LOCK = 1,//加锁文件
	FOCP_FILE_TLOCK = 2,//尝试加锁文件，如果已经加锁，就失败
	FOCP_FILE_ULOCK = 3,//解锁文件
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

//依据CBaseFile实现的具体文件驱动器不需要考虑并发访问的安全问题
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
	// r=读,对于存储文件而言，文件不存在会失败
	// w=写,对于存储文件而言，文件不存在会失败，并可携带多个子属性
	//	c=文件不存在，将创建
	//	d=文件存在，将清空数据
	//  a=以追加方式写
	//	n=创建新文件，文件存在将失败
	// l[:n]=侦听选项，如果没有设置sBindName，会返回失败，可指定侦听数量。
	// e=独占模式，缺省是共享读写模式
	// s=共享读模式，缺省是共享读写模式
	// t[:n]=组播TTL选项
	// m=组播回环标识
	//返回码：0成功，错误码如下
	// -1: 协议不存在
	// -2: 无效绑定名
	// -3: 绑定失败
	// -4: 无效连接名
	// -5: 连接失败
	// -6: 选项错误
	// -7: 其它错误
	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong * pHandle=NULL);

	virtual uint32 GetType() = 0;
	virtual uint32 GetStatus();//驱动函数不需要实现。通过SetStatus设置

	virtual const CFileName& GetFileName() = 0;

	//对于存储文件而言，读写缓冲是一体的，所以类型为FOCP_BOTH_BUFFER。
	//对于管道文件而言，读写缓冲是分离的，当指定为FOCP_BOTH_BUFFER，两个缓冲都设置为同样大小。
	//	可以使用FOCP_READ_BUFFER、FOCP_WRITE_BUFFER分别设置不同的缓冲大小。
	//如果不支持缓冲的，忽略操作
	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);

	//面向连接的文件，支持如下读写。格式化器选择这两个读写函数
	//0:文件结束【除非pBuf为NULL或nBufLen为0】，-1:IO异常，-2:超时，>0:返回长度
	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut=0xFFFFFFFF);
	//0:文件中断、磁盘满等因素，-1:IO异常，>0:返回长度
	virtual int32 Write(const void* pBuf, int32 nBufLen);

	//面向非连接的文件，支持如下读写
	//0:文件结束【除非pBuf为NULL或nBufLen为0】，-1:IO异常，-2:超时，>0:返回长度
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut=0xFFFFFFFF);
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut=0xFFFFFFFF);
	//0:文件中断、磁盘满等因素，-1:IO异常，>0:返回长度
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr);//并发安全

	//面向侦听的文件，支持如下函数
	virtual bool Accept(CFile &oFile);

	//对于存储文件而言，支持如下随机访问能力
	virtual int32 GetPosition();
	virtual void SetPosition(int32 nPos);
	virtual void Seek(uint32 nOrigin, int32 nOffset);

	//对于存储文件而言，支持如下特殊能力
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

	//sFileName的格式为：PROTOCOL '://' [ CONNECTNAME ] [ '|' [ BINDNAME ] ]
	// 例如：tcp://www.newpost.com:2020|192.168.1.101:3333
	//			udp://|0.0.0.0:4444
	//				如果连接地址是一个主播地址，那么绑定地址就是接口地址。
	//				sOption可添加t[:n]指定TTL，r将支持组播回环。
	//			disk:///home/caowenke/G4/FiberTest/cfg/FiberConfig.dat
	int32 Open(const char* sFileName, const char* sOption=NULL);

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong * pHandle=NULL);//并发安全

	void Redirect(CBaseFile* pFile, bool bOwned=true);

	virtual const CFileName& GetFileName();//并发安全

	virtual uint32 GetType();//并发安全
	virtual uint32 GetStatus();//并发安全

	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);//并发安全

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut=0xFFFFFFFF);//并发安全
	virtual int32 Write(const void* pBuf, int32 nBufLen);//并发安全

	int32 ReadFrom(void* pBuf, int32 nBufLen, CString &oFileName, uint32 nTimeOut=0xFFFFFFFF);//并发安全
	int32 WriteTo(const void* pBuf, int32 nBufLen, const char* sFileName);//并发安全

	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut=0xFFFFFFFF);//并发安全
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut=0xFFFFFFFF);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName);//并发安全
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr);//并发安全

	int32 Read(CMemoryStream& oStream, int32 nReadSize, uint32 nTimeOut=0xFFFFFFFF);//并发安全
	int32 Write(CMemoryStream& oStream, int32 nWriteSize);//并发安全

	//面向侦听的文件，支持如下函数
	virtual bool Accept(CFile &oFile);

	virtual int32 GetPosition();//并发安全
	virtual void SetPosition(int32 nPos);//并发安全
	virtual void Seek(uint32 nOrigin, int32 nOffset);//并发安全

	virtual void Truncate();//并发安全
	virtual bool Lock(uint32 nLock, int32 nSize);//并发安全
	virtual void Flush();

	void Swap(CFile& oSrc);

	static uint32 GetIpAddr(const char* sIpAddr);
	static bool GetIpAddrList(const char* sFileName, CIpAddrList& oAddrList);
	static void GetIpFileName(const CIpAddr& oAddr, CString &oIpFileName, bool bFriendly=false);
	static bool IsMulticastAddr(uint32 nAddr);//检查该地址是否可以作为主播地址
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

//格式化读，返回读的项数。
//-1:IO错误, -3:参数错误
AFS_API int32 Scan(const char* sFormat, ...);
AFS_API int32 ScanV(const char* sFormat, CVaList& pArgList);

//格式化写，返回写入的总长度。
//-1:IO错误, -2:IO中断或满, -3:参数错误
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
