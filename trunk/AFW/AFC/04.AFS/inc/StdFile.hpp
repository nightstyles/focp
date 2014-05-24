
#include "File.hpp"

#ifndef _Afs_StdFile_Hpp_
#define _Afs_StdFile_Hpp_

FOCP_BEGIN();

class CStdFileInterface: public CFileInterface
{
public:
	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);
};

class CStdFile: public CBaseFile
{
	FOCP_FORBID_COPY(CStdFile);
private:
	int32 m_hHandle;
	CFileName m_oFileName;

public:
	CStdFile();
	virtual ~CStdFile();

	virtual CFileInterface* GetInterface();

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close();

	virtual uint32 GetType();

	virtual const CFileName& GetFileName();

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut);
	virtual int32 Write(const void* pBuf, int32 nBufLen);
};

class CMemoryFileInterface: public CFileInterface
{
public:
	virtual const char* GetProtocol();
	virtual CBaseFile* CreateFile();
	virtual void DestroyFile(CBaseFile* pFile);
};

class CMemoryFile: public CBaseFile
{
	FOCP_FORBID_COPY(CMemoryFile);
private:
	CMemoryStream* m_pStream;
	CFileName m_oFileName;
	bool m_bRead, m_bWrite;

public:
	CMemoryFile();
	virtual ~CMemoryFile();

	virtual CFileInterface* GetInterface();

	virtual int32 Open(const CFileName& oFileName, const char* sOption=NULL);
	virtual void Close();

	virtual uint32 GetType();

	virtual const CFileName& GetFileName();

	virtual int32 Read(void* pBuf, int32 nBufLen, uint32 nTimeOut);
	virtual int32 Write(const void* pBuf, int32 nBufLen);
};

FOCP_END();

#endif
