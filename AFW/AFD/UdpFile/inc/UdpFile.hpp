
#include "AFC.hpp"

#ifndef _Afd_UdpFile_Hpp_
#define _Afd_UdpFile_Hpp_

FOCP_BEGIN();

class CUdpFileInterface: public CFileInterface
{
	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);
};

class CUdpFile: public CBaseFile
{
private:
	ulong m_hHandle;
	CFileName m_oFileName;
	bool m_bConnected, m_bIsMultiCast;

public:
	CUdpFile();
	virtual ~CUdpFile();

	virtual CFileInterface* GetInterface();

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong * pHandle=NULL);

	virtual uint32 GetType();

	virtual const CFileName& GetFileName();

	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut);
	virtual int32 Write(const void* pBuf, int32 nBufLen);

	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CFileName &oFileName, uint32 nTimeOut);
	virtual int32 ReadFrom(void* pBuf, int32 nBufLen, CIpAddr& oIpAddr, uint32 nTimeOut);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CFileName& oFileName);
	virtual int32 WriteTo(const void* pBuf, int32 nBufLen, const CIpAddr& oIpAddr);//???????
};

FOCP_END();

#endif
