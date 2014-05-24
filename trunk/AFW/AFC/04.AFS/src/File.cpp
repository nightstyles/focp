
#include "File.hpp"
#include "FileSystem.hpp"

#ifndef WINDOWS
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

FOCP_BEGIN();

bool CFileName::Parse(const char* sFileName)
{
	oProtocol.Clear();
	oConnectName.Clear();
	oBindName.Clear();
	char* sSp = CString::StringOfString(sFileName, "://");
	if(!sSp || sSp == sFileName)
	{//缺省为本地磁盘文件
		oProtocol = "disk";
		if(sSp)
			sFileName += 3;
	}
	else
	{
		oProtocol.Assign(sFileName, sSp-sFileName);
		sFileName = sSp + 3;
	}
	sSp = CString::StringOfString(sFileName, "|");
	if(sSp)
	{
		uint32 nLen = sSp-sFileName;
		if(nLen)
			oConnectName.Assign(sFileName, sSp-sFileName);
		oBindName = sSp+1;
	}
	else
		oConnectName = sFileName;
	return true;
}

CFileInterface::CFileInterface()
	:CInterface(CFileInterface::GetInterfaceManager())
{
}

CFileInterface::~CFileInterface()
{
}

const char* CFileInterface::GetProtocol()
{
	return NULL;
}

CBaseFile* CFileInterface::CreateFile()
{
	return NULL;
}

void CFileInterface::DestroyFile(CBaseFile* pFile)
{
}

const char* CFileInterface::GetInterfaceName()
{
	return GetProtocol();
}

struct A
{
	CInterfaceManager oManager;
	A():oManager("FileInterfaces")
	{
	}
};

CInterfaceManager* CFileInterface::GetInterfaceManager()
{
	return &CSingleInstance<A>::GetInstance()->oManager;
}

CBaseFile::CBaseFile()
{
	m_nStatus = FOCP_FILE_CLOSED;
}

CBaseFile::~CBaseFile()
{
	Close();
}

CFileInterface* CBaseFile::GetInterface()
{
	return NULL;
}

int32 CBaseFile::Open(const CFileName& oFileName, const char* sOption)
{
	return FOCP_FILE_PROTOCOL_ERROR;
}

void CBaseFile::Close(ulong* pHandle)
{
}

uint32 CBaseFile::GetStatus()
{
	return m_nStatus;
}

void CBaseFile::SetBuffer(uint32 nBufSize, uint32 nBufType)
{
}

int32 CBaseFile::Read(void* pBuf, int32 nBufLen, uint32 nTimeOut)
{
	return -1;
}

int32 CBaseFile::Write(const void* pBuf, int32 nBufLen)
{
	return -1;
}

int32 CBaseFile::ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut)
{
	return -1;
}

int32 CBaseFile::ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut)
{
	return -1;
}

int32 CBaseFile::WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName)
{
	return -1;
}

int32 CBaseFile::WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr)
{
	return -1;
}

bool CBaseFile::Accept(CFile &oFile)
{
	return false;
}

int32 CBaseFile::GetPosition()
{
	return 0;
}

void CBaseFile::SetPosition(int32 nPos)
{
}

void CBaseFile::Seek(uint32 nOrigin, int32 nOffset)
{
}

void CBaseFile::Truncate()
{
}

bool CBaseFile::Lock(uint32 nLock, int32 nSize)
{
	return false;
}

void CBaseFile::Flush()
{
}

uint32 CBaseFile::GetOpenOption(const char* sOption, uint32 *pListenCount)
{
	uint32 nRet = 0;
	bool bFilled = false;
	if(sOption)
	{
		while(*sOption)
		{
			switch(*sOption)
			{
			case 'r':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_SHARE_READ;
				nRet &= ~(uint32)FOCP_FILE_OPTION_LISTEN;
				break;
			case 'w':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE;
				nRet |= FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
				nRet &= ~(uint32)FOCP_FILE_OPTION_LISTEN;
				break;
			case 'c':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE | FOCP_FILE_OPTION_CREATE;
				nRet |= FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_DESTROY|FOCP_FILE_OPTION_APPEND|FOCP_FILE_OPTION_NEW|FOCP_FILE_OPTION_LISTEN);
				break;
			case 'd':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE | FOCP_FILE_OPTION_DESTROY;
				nRet |= FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_APPEND|FOCP_FILE_OPTION_NEW|FOCP_FILE_OPTION_LISTEN);
				break;
			case 'a':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE | FOCP_FILE_OPTION_APPEND;
				nRet |= FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_DESTROY|FOCP_FILE_OPTION_NEW|FOCP_FILE_OPTION_LISTEN);
				break;
			case 'n':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE | FOCP_FILE_OPTION_NEW;
				nRet |= FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_DESTROY|FOCP_FILE_OPTION_APPEND|FOCP_FILE_OPTION_LISTEN);
				break;
			case 'l':
				bFilled = true;
				if(pListenCount)
					pListenCount[0] = 5;
				if(sOption[1] == ':')
				{
					uint32 nListenCount;
					CFormatString oString;
					oString.Bind((char*)sOption+2);
					if(oString.Scan("%u", &nListenCount) == 1 && pListenCount)
						pListenCount[0] = nListenCount;
					sOption+=oString.GetAlignPos()+1;
				}
				nRet |= FOCP_FILE_OPTION_LISTEN;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_READ|FOCP_FILE_OPTION_WRITE|
								  FOCP_FILE_OPTION_CREATE|FOCP_FILE_OPTION_DESTROY|
								  FOCP_FILE_OPTION_APPEND|
								  FOCP_FILE_OPTION_SHARE_READ|FOCP_FILE_OPTION_SHARE_WRITE);
				break;
			case 't':
				bFilled = true;
				if(pListenCount)
					pListenCount[0] = 3;
				if(sOption[1] == ':')
				{
					uint32 nListenCount;
					CFormatString oString;
					oString.Bind((char*)sOption+2);
					if(oString.Scan("%u", &nListenCount) == 1 && pListenCount)
						pListenCount[0] = nListenCount;
					sOption+=oString.GetAlignPos()+1;
				}
				nRet |= FOCP_FILE_OPTION_TTL;
				break;
			case 'm':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_LOOP;
				break;
			case 'e':
				bFilled = true;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_SHARE_READ|FOCP_FILE_OPTION_SHARE_WRITE|FOCP_FILE_OPTION_LISTEN);
				break;
			case 's':
				bFilled = true;
				nRet |= FOCP_FILE_OPTION_SHARE_READ;
				nRet &= ~(uint32)(FOCP_FILE_OPTION_SHARE_WRITE|FOCP_FILE_OPTION_LISTEN);
				break;
			}
			++sOption;
		}
	}
	if(bFilled == false)
		nRet = FOCP_FILE_OPTION_READ | FOCP_FILE_OPTION_WRITE | FOCP_FILE_OPTION_SHARE_READ | FOCP_FILE_OPTION_SHARE_WRITE;
	return nRet;
}

void CBaseFile::SetStatus(uint32 nStatus)
{
	m_nStatus = nStatus;
}

CFile::CFile(CBaseFile* pFile, bool bOwned)
{
	m_pBaseFile = NULL;
	m_bOwned = true;
	SetStatus(FOCP_FILE_NORMAL);
	Redirect(pFile, bOwned);
}

CFile::CFile(const char* sFileName, const char* sOption)
{
	m_pBaseFile = NULL;
	m_bOwned = true;
	SetStatus(FOCP_FILE_NORMAL);
	Open(sFileName, sOption);
}

CFile::~CFile()
{
	Redirect(NULL, true);
}

CFileInterface* CFile::GetInterface()
{
	if(m_pBaseFile == NULL)
		return NULL;
	return m_pBaseFile->GetInterface();
}

int32 CFile::Open(const char* sFileName, const char* sOption)
{
	CFileName oFileName;
	if(oFileName.Parse(sFileName))
		return Open(oFileName, sOption);
	return FOCP_FILE_PROTOCOL_ERROR;
}

int32 CFile::Open(const CFileName& oFileName, const char* sOption)
{
	CFileInterface* pInterface = (CFileInterface*)CFileInterface::GetInterfaceManager()->QueryInterface(oFileName.oProtocol.GetStr());
	if(pInterface == NULL)
		return FOCP_FILE_PROTOCOL_ERROR;

	CBaseFile* pFile = pInterface->CreateFile();
	int32 nRet = pFile->Open(oFileName, sOption);

	if(nRet)
		pInterface->DestroyFile(pFile);
	else
		Redirect(pFile);

	return nRet;
}

void CFile::Close(ulong * pHandle)
{
	if(m_pBaseFile == NULL)
		return;
	m_pBaseFile->Close(pHandle);
}

void CFile::Redirect(CBaseFile* pFile, bool bOwned)
{
	if(pFile == (CBaseFile*)this)
		return;

	DestroyFile();

	if(pFile == NULL)
		return;

	m_pBaseFile = pFile;
	m_bOwned = bOwned;
}

const CFileName& CFile::GetFileName()
{
	return m_pBaseFile->GetFileName();
}

uint32 CFile::GetType()
{
	if(m_pBaseFile == NULL)
		return FOCP_INVALID_FILE;
	return m_pBaseFile->GetType();
}

uint32 CFile::GetStatus()
{
	if(m_pBaseFile == NULL)
		return FOCP_FILE_CLOSED;
	return m_pBaseFile->GetStatus();
}

void CFile::SetStatus(uint32 nStatus)
{
	if(m_pBaseFile)
		m_pBaseFile->SetStatus(nStatus);
}

void CFile::SetBuffer(uint32 nBufSize, uint32 nBufType)
{
	if(m_pBaseFile)
		m_pBaseFile->SetBuffer(nBufSize, nBufType);
}

int32 CFile::Read(void* pBuf, int32 nBufLen, uint32 nTimeOut)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	int32 nRet = m_pBaseFile->Read(pBuf, nBufLen, nTimeOut);
	return nRet;
}

int32 CFile::Write(const void* pBuf, int32 nBufLen)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	return m_pBaseFile->Write(pBuf, nBufLen);
}

int32 CFile::ReadFrom(void* pBuf, int32 nBufLen, CString &oFileName, uint32 nTimeOut)
{
	CFileName oFileName2;
	int32 nRet = ReadFrom(pBuf, nBufLen, oFileName2, nTimeOut);
	if(nRet)
		return nRet;
	oFileName.Clear();
	CStringFormatter oFormatter(&oFileName);
	oFormatter.Print("%s://", oFileName2.oProtocol.GetStr());
	oFormatter.Print("%s", oFileName2.oConnectName.GetStr());
	if(!oFileName2.oBindName.Empty())
		oFormatter.Print("|%s", oFileName2.oBindName.GetStr());
	return nRet;
}

int32 CFile::WriteTo(const void* pBuf, int32 nBufLen, const char* sFileName)
{
	CFileName oFileName;
	if(oFileName.Parse(sFileName))
		return WriteTo(pBuf, nBufLen, oFileName);
	return 0;
}

int32 CFile::ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	return m_pBaseFile->ReadFrom(pBuf, nBufLen, oFileName, nTimeOut);
}

int32 CFile::ReadFrom(void* pBuf, int32 nBufLen, CIpAddr &oFileName, uint32 nTimeOut)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	return m_pBaseFile->ReadFrom(pBuf, nBufLen, oFileName, nTimeOut);
}

int32 CFile::WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	return m_pBaseFile->WriteTo(pBuf, nBufLen, oFileName);
}

int32 CFile::WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr)
{
	if(nBufLen <= 0 || !pBuf)
		return 0;
	if(m_pBaseFile == NULL)
		return -1;
	int32 nRest = 0x7FFFFFFF - m_pBaseFile->GetPosition();
	if(nRest < nBufLen)
		nBufLen = nRest;
	if(nBufLen <= 0)
		return 0;
	return m_pBaseFile->WriteTo(pBuf, nBufLen, oIpAddr);
}

int32 CFile::Read(CMemoryStream& oStream, int32 nReadSize, uint32 nTimeOut)
{
	if(nReadSize <= 0)
		return 0;
	if(oStream.GetSize() == oStream.GetPosition())
	{
		if(!oStream.ExtendSize(nReadSize))
			return 0;
	}
	uint32 nBlockSize;
	char* pBuf = oStream.GetBuf(nBlockSize);
	if(nBlockSize > (uint32)nReadSize)
		nBlockSize = nReadSize;
	int32 nRet = Read(pBuf, nBlockSize, nTimeOut);//???????
	if(nRet > 0)
		oStream.Seek(nRet);
	return nRet;
}

int32 CFile::Write(CMemoryStream& oStream, int32 nWriteSize)
{
	if(nWriteSize <= 0)
		return 0;
	uint32 nRest = oStream.GetSize() - oStream.GetPosition();
	if(nRest < (uint32)nWriteSize)
		nWriteSize = nRest;
	int32 nRet, nOldCopySize = nWriteSize;
	while(nWriteSize)
	{
		uint32 nBlockSize;
		char* pBuf = oStream.GetBuf(nBlockSize);
		if(nBlockSize > (uint32)nWriteSize)
			nBlockSize = nWriteSize;
		nRet = Write(pBuf, nBlockSize);
		if(nRet < 0)
		{
			if(nWriteSize == nOldCopySize)
				return nRet;
			break;
		}
		if(nRet == 0)
			break;
		nWriteSize -= nRet;
		oStream.Seek(nRet);
	}
	return nOldCopySize - nWriteSize;
}

bool CFile::Accept(CFile &oFile)
{
	if(m_pBaseFile == NULL)
		return false;
	return m_pBaseFile->Accept(oFile);
}

int32 CFile::GetPosition()
{
	if(m_pBaseFile == NULL)
		return 0;
	return m_pBaseFile->GetPosition();
}

void CFile::SetPosition(int32 nPos)
{
	if(nPos < 0)
		nPos = 0;
	if(m_pBaseFile)
		m_pBaseFile->SetPosition(nPos);
}

void CFile::Seek(uint32 nOrigin, int32 nOffset)
{
	if(m_pBaseFile)
		m_pBaseFile->Seek(nOrigin, nOffset);
}

void CFile::Truncate()
{
	if(m_pBaseFile)
		m_pBaseFile->Truncate();
}

bool CFile::Lock(uint32 nLock, int32 nSize)
{
	if(m_pBaseFile == NULL)
		return false;
	return m_pBaseFile->Lock(nLock, nSize);
}

void CFile::Flush()
{
	if(m_pBaseFile == NULL)
		return;
	m_pBaseFile->Flush();
}

void CFile::DestroyFile(bool bLock)
{
	if(m_pBaseFile)
	{
		if(m_bOwned)
			GetInterface()->DestroyFile(m_pBaseFile);
		m_pBaseFile = NULL;
	}
	m_bOwned = true;
}

void CFile::Swap(CFile& oSrc)
{
	::FOCP_NAME::Swap(m_pBaseFile, oSrc.m_pBaseFile);
	::FOCP_NAME::Swap(m_bOwned, oSrc.m_bOwned);
}

FOCP_END();

#ifdef WINDOWS
#include <io.h>
#include <sys/locking.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/file.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

FOCP_BEGIN();

bool CFile::GetHostName(char sHostName[256])
{
	if(gethostname(sHostName, sizeof(sHostName)))
		return false;
	return true;
}

bool CFile::GetHostIpList(CString &oHostIpList)
{
	uint32 i;
	struct hostent *hptr;
	char cHostName[256];
	if (gethostname(cHostName, sizeof(cHostName)))
	{
		FocpLog(FOCP_LOG_ERROR, ("gethostname failure"));
		return false;
	}
	oHostIpList.Clear();
	SystemLock();
	hptr = gethostbyname(cHostName);
	if (NULL == hptr)
	{
		SystemUnLock();
		FocpLog(FOCP_LOG_ERROR, ("gethostbyname failure"));
		return false;
	}
	CStringFormatter oFormatter(&oHostIpList);
	for(i=0; hptr->h_addr_list[i]; i++)
	{
		uint8* pIp = (uint8*)(hptr->h_addr_list[i]);
		oFormatter.Print(";%u8.%u8.%u8.%u8", pIp[0], pIp[1], pIp[2], pIp[3]);
	}
	SystemUnLock();
	oHostIpList.Append(";");
	return true;
}

bool CFile::CheckHostIp(uint32 nIp)
{
	if(nIp == (uint32)(-1))
		return false;
	CIpAddrList oAddrList;
	if(!GetIpAddrList("*", oAddrList))
		return false;
	uint32 nSize = oAddrList.oAddrList.GetSize();
	for(uint32 i=0; i<nSize; ++i)
	{
		uint32 nTmp = oAddrList.oAddrList[i];
		if(nTmp == nIp)
			return true;
	}
	return false;
}

bool CFile::CheckHostIp(const char* sIpAddr)
{
	uint32 nIp = GetIpAddr(sIpAddr);
	if(nIp == (uint32)(-1))
		return false;
	return CheckHostIp(nIp);
}

#if defined(WINDOWS) || defined(CYGWIN_NT)

bool GetWinMac(char sMac[7], bool bWalk);
bool CFile::GetMacAddress(char sMac[7], bool bWalk)
{
	return GetWinMac(sMac, bWalk);
}

#else

static int getmac_one(int no, char* buf)
{
	struct ifreq ifr;

	int fd=socket(PF_INET, SOCK_DGRAM, 0);
	if(fd < 0)
		return 1;

	CBinary::MemorySet(&ifr, 0, sizeof(ifr));

	CFormatString oStr;
	oStr.Print("eth%d", no);

	CString::StringCopy(ifr.ifr_name, oStr.GetStr());

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close(fd);
		return 1;
	}

	CBinary::MemoryCopy(buf, &ifr.ifr_hwaddr.sa_data, 6);
	close(fd);

	return 0;
}

bool CFile::GetMacAddress(char sMac[7], bool bWalk)
{
	sMac[6] = '\0';
	for(int i=0; i<100; ++i)
	{
		if(!getmac_one(i, sMac))
			return true;
		if(!bWalk)
			break;
	}
	return false;
}

#endif

ulong CFile::GetHostId()
{
#ifdef UNIX
	return gethostid();
#else
	static uint32 nHostId;
	static bool bInit = false;
	if(bInit)
		return nHostId;
	SystemLock();
	if(!bInit)
	{
		char* out = (char*)&nHostId;
		char sMac[20];
		if(!GetMacAddress(sMac))
		{
			nHostId = 0xFFFFFFFF;
			bInit = true;
			SystemUnLock();
			return nHostId;
		}
		static uint8 sSecret[10] = {0x7E,0xA6,0xD7,0xB1,0xC3,0xF2,0xE3,0x04,0xBE,0xCA};
		char md5Output[16];
		CBinary::MemoryCopy(sMac+6, sSecret, 10);
		GetMd5((uint8*)md5Output, (uint8*)sMac, 16);
		if(CBinary::U32Code(1) != 1)
		{
			out[0] = md5Output[1] + md5Output[6] + md5Output[11];
			out[1] = md5Output[3] + md5Output[8] + md5Output[13];
			out[2] = md5Output[5] + md5Output[10] + md5Output[15];
			out[3] = md5Output[0] & 0x7F;
		}
		else
		{
			out[3] = md5Output[1] + md5Output[6] + md5Output[11];
			out[2] = md5Output[3] + md5Output[8] + md5Output[13];
			out[1] = md5Output[5] + md5Output[10] + md5Output[15];
			out[0] = md5Output[0] & 0x7F;
		}
		bInit = true;
	}
	SystemUnLock();
	return nHostId;
#endif
}

bool CFile::GetIpAddrList(const char* sFileName, CIpAddrList& oAddrList)
{
	char * sPort;
	oAddrList.nPort = 0;
	CFormatString oFileName;
	sPort = CString::CharOfString(sFileName, ':');
	if(sPort)
	{
		char c;
		oFileName.Assign(sPort+1);
		if(oFileName.Scan("%u16", &oAddrList.nPort) != 1)
		{
			FocpLog(FOCP_LOG_ERROR, ("Invalid Port"));
			return false;
		}
		if(!oFileName.GetChar(c))
		{
			FocpLog(FOCP_LOG_ERROR, ("Invalid Port"));
			return false;
		}
		oFileName.Assign(sFileName, sPort-sFileName);
		sFileName = oFileName.GetStr();
	}
	oAddrList.oAddrList.Clear();
	if(!sFileName[0])
		return true;
	uint32 nAddr = inet_addr(sFileName);
	if(nAddr != (uint32)(-1))
		oAddrList.oAddrList.Insert((uint32)(-1), nAddr);
	else
	{
		struct hostent *hptr;
		char sHostName[256];
		if(sFileName[0] == '*')
		{
			gethostname(sHostName, 200);
			sFileName = sHostName;
		}
		SystemLock();
		hptr = gethostbyname(sFileName);
		if (NULL == hptr)
		{
			SystemUnLock();
			FocpLog(FOCP_LOG_ERROR, ("gethostbyname failure"));
			return false;
		}
		for(uint32 i=0; hptr->h_addr_list[i]; i++)
			oAddrList.oAddrList.Insert((uint32)(-1), *(uint32*)(hptr->h_addr_list[i]));
		SystemUnLock();
	}
	return true;
}

uint32 CFile::GetIpAddr(const char* sIpAddr)
{
	uint32 nAddr = inet_addr(sIpAddr);
	if(nAddr == (uint32)(-1))
	{
		struct hostent *hptr;
		char sHostName[256];
		if(sIpAddr[0] == '*')
		{
			gethostname(sHostName, 200);
			sIpAddr = sHostName;
		}
		SystemLock();
		hptr = gethostbyname(sIpAddr);
		if (NULL == hptr)
		{
			SystemUnLock();
			return -1;
		}
		nAddr = *(uint32*)(hptr->h_addr_list[0]);
		SystemUnLock();
	}
	return nAddr;
}

void CFile::GetIpFileName(const CIpAddr& oAddr, CString &oIpFileName, bool bFriendly)
{
	if(bFriendly)
	{
		SystemLock();
		struct hostent *hptr = gethostbyaddr((const char*)&oAddr.nAddr, 4, AF_INET);
		if(hptr && hptr->h_name && hptr->h_name[0])
			oIpFileName = hptr->h_name;
		SystemUnLock();
	}
	if(oIpFileName.Empty())
	{
		uint8* pAddr = (uint8*)&oAddr.nAddr;
		CStringFormatter(&oIpFileName).Print("%u8.%u8.%u8.%u8", pAddr[0], pAddr[1], pAddr[2], pAddr[3]);
	}
	if(oAddr.nPort)
		CStringFormatter(&oIpFileName, oIpFileName.GetSize()).Print(":%u16", oAddr.nPort);
}

bool CFile::IsMulticastAddr(uint32 nAddr)
{
	bool bIsMultiCast = false;
	uint32 nMinMultiCastAddr=0, nMaxMultiCastAddr=0xFFFFFFFF;
	*(uint8*)&nMinMultiCastAddr = 224;
	*(uint8*)&nMaxMultiCastAddr = 239;
	nMinMultiCastAddr = CBinary::U32Code(nMinMultiCastAddr);
	nMaxMultiCastAddr = CBinary::U32Code(nMaxMultiCastAddr);
	nAddr = CBinary::U32Code(nAddr);
	if(nMaxMultiCastAddr > nMinMultiCastAddr)
	{
		if(nAddr >= nMinMultiCastAddr && nAddr <= nMaxMultiCastAddr)
			bIsMultiCast = true;
	}
	else
	{
		if(nAddr >= nMaxMultiCastAddr && nAddr <= nMinMultiCastAddr)
			bIsMultiCast = true;
	}
	return bIsMultiCast;
}

bool CFile::IsMulticastAddr(const char* sAddr)
{
	return IsMulticastAddr((uint32)inet_addr(sAddr));
}

//格式化读，返回读的项数。
//-1:IO错误, -3:参数错误
AFS_API int32 Scan(const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = ScanV(sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

struct CStdInFormatter
{
	CFileFormatter oFormatter;
	CStdInFormatter():oFormatter(&FocpStdIn)
	{
	}
};

static CMutex g_oStdInMutex;

AFS_API int32 ScanV(const char* sFormat, CVaList& pArgList)
{
	g_oStdInMutex.Enter();
	int32 nRet = CSingleInstance<CStdInFormatter>::GetInstance()->oFormatter.ScanV(sFormat, pArgList);
	g_oStdInMutex.Leave();
	return nRet;
}

//格式化写，返回写入的总长度。
//-1:IO错误, -2:IO中断或满, -3:参数错误
AFS_API int32 Print(const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = PrintV(sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

AFS_API int32 PrintError(const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = PrintErrorV(sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

struct CStdOutFormatter
{
	CFileFormatter oFormatter;
	CStdOutFormatter():oFormatter(&FocpStdOut)
	{
	}
};

static CMutex g_oStdOutMutex;

AFS_API int32 PrintV(const char* sFormat, CVaList& pArgList)
{
	g_oStdOutMutex.Enter();
	int32 nRet = CSingleInstance<CStdOutFormatter>::GetInstance()->oFormatter.PrintV(sFormat, pArgList);
	g_oStdOutMutex.Leave();
	return nRet;
}

struct CStdErrFormatter
{
	CFileFormatter oFormatter;
	CStdErrFormatter():oFormatter(&FocpStdErr)
	{
	}
};

AFS_API int32 PrintErrorV(const char* sFormat, CVaList& pArgList)
{
	g_oStdOutMutex.Enter();
	int32 nRet = CSingleInstance<CStdErrFormatter>::GetInstance()->oFormatter.PrintV(sFormat, pArgList);
	g_oStdOutMutex.Leave();
	return nRet;
}

AFS_API int32 PrintEx(bool bError, const char* sFormat, ...)
{
	CVaList pArgList;
	VaStart(pArgList, sFormat);
	int32 nRet = PrintExV(bError, sFormat, pArgList);
	VaEnd(pArgList);
	return nRet;
}

AFS_API int32 PrintExV(bool bError, const char* sFormat, CVaList& pArgList)
{
	return (bError)?PrintErrorV(sFormat, pArgList):PrintV(sFormat, pArgList);
}

CFileFormatter::CFileFormatter(CFile* pFile, int32 nBufSize, bool bText, CFormatterMethod* pMethod)
	:CFormatter(pMethod)
{
	if(nBufSize<=0)
		nBufSize = 256;
	m_pFile = pFile;
	m_nBufSize = nBufSize;
	m_oReadBuffer.m_sBuffer = new char[nBufSize];
	m_oReadBuffer.m_nCount = 0;
	m_oReadBuffer.m_nOffset = 0;
	m_oWriteBuffer.m_sBuffer = new char[nBufSize];
	m_oWriteBuffer.m_nCount = 0;
	m_oWriteBuffer.m_nOffset = 0;
	m_bText = bText;
	m_bHaveCr = false;
}

CFileFormatter::~CFileFormatter()
{
	Flush();
	delete[] m_oReadBuffer.m_sBuffer;
	delete[] m_oWriteBuffer.m_sBuffer;
}

//0：正常，-1：IO异常，-2：读到尾部
int32 CFileFormatter::ReadChar(char &nChar)
{
	int32 nRet;
	if(m_oReadBuffer.m_nOffset < m_oReadBuffer.m_nCount)
	{
		nChar = m_oReadBuffer.m_sBuffer[m_oReadBuffer.m_nOffset];
		++m_oReadBuffer.m_nOffset;
		return 0;
	}
	m_oReadBuffer.m_nOffset = 0;
	//0:文件结束【除非pBuf为NULL或nBufLen为0】，-1:IO异常，>0:返回长度
	m_oReadBuffer.m_nCount = m_pFile->Read(m_oReadBuffer.m_sBuffer, m_nBufSize);
	switch(m_oReadBuffer.m_nCount)
	{
	case 0:
		nRet = -2;
		break;
	case -1:
		nRet = -1;
		break;
	default:
		nRet = 0;
		nChar = m_oReadBuffer.m_sBuffer[m_oReadBuffer.m_nOffset];
		++m_oReadBuffer.m_nOffset;
		break;
	}
	return nRet;
}

//0：正常，-1：IO异常
int32 CFileFormatter::WriteChar(char nChar)
{
#ifdef WINDOWS
	if(nChar == '\n' && m_bText && !m_bHaveCr)
		WriteChar('\r');
#endif
	if(m_bText)
	{
		if(nChar == '\r')
		{
			if(m_bHaveCr)
				return 0;
			m_bHaveCr = true;
		}
		else
			m_bHaveCr = false;
	}
	if(m_oWriteBuffer.m_nCount >= m_nBufSize)
		Flush();
	m_oWriteBuffer.m_sBuffer[m_oWriteBuffer.m_nCount] = nChar;
	++m_oWriteBuffer.m_nCount;
	return 0;
}

void CFileFormatter::Flush()
{
	if(m_oWriteBuffer.m_nCount > 0)
	{
		char* sBuffer = m_oWriteBuffer.m_sBuffer;
		int32 nCount = m_oWriteBuffer.m_nCount;
		while(nCount)
		{
			int32 nRet = m_pFile->Write(sBuffer, nCount);
			if(nRet <= 0)
			{
				m_nSuccess = nRet;
				break;
			}
			nCount -= nRet;
			sBuffer += nRet;
		}
		m_nFlushCount += m_oWriteBuffer.m_nCount - nCount;
		m_oWriteBuffer.m_nCount = 0;
	}
}

CFormatFile::CFormatFile(int32 nBufSize, bool bText, CFormatterMethod* pMethod)
	:CFile(), CFileFormatter((CFile*)this, nBufSize, bText, pMethod)
{
}

CFormatFile::~CFormatFile()
{
}

void CFormatFile::Close(ulong * pHandle)
{
	CFileFormatter::Flush();
	CFile::Close(pHandle);
}


FOCP_END();
