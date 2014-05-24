
#include "AFC.hpp"

#ifndef _Afd_DiskFile_Hpp_
#define _Afd_DiskFile_Hpp_

FOCP_BEGIN();

class CDiskFileInterface: public CFileInterface
{
	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);
};

class CDiskFile: public CBaseFile
{
private:
	ulong m_hHandle;
	CFileName m_oFileName;

public:
	CDiskFile();
	virtual ~CDiskFile();

	virtual CFileInterface* GetInterface();

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close(ulong* pHandle);

	virtual uint32 GetType();

	virtual const CFileName& GetFileName();

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut);
	virtual int32 Write(const void* pBuf, int32 nBufLen);

	virtual int32 GetPosition();
	virtual void SetPosition(int32 nPos);
	virtual void Seek(uint32 nOrigin, int32 nOffset);//Origin:0=cur，1=tail，2=head
	
	virtual void Truncate();//无法实现异步
	virtual bool Lock(uint32 nLock, int32 nSize);//无法实现异步
	virtual void Flush();

	virtual bool ReadWriteCritical();
};

FOCP_END();

#endif
