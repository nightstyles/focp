
#include "AFC.hpp"

#ifndef _Afd_TcpFile_Hpp_
#define _Afd_TcpFile_Hpp_

FOCP_BEGIN();

class CTcpFileInterface: public CFileInterface
{
	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);
};

class CTcpFile: public CBaseFile
{
	friend class CTcpServer;
protected:
	ulong m_hHandle;
	CFileName m_oFileName;
	bool m_bListened;

public:
	CTcpFile();
	virtual ~CTcpFile();

	virtual CFileInterface* GetInterface();

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong *pHandle=NULL);

	virtual uint32 GetType();

	virtual const CFileName& GetFileName();

	virtual void SetBuffer(uint32 nBufSize, uint32 nBufType=FOCP_BOTH_BUFFER);

	virtual bool Accept(CFile &oFile);

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut);
	virtual int32 Write(const void* pBuf, int32 nBufLen);
};

FOCP_END();

#endif
