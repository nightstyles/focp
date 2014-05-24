
#include "VmmFile.hpp"

#ifndef _VMM_DEVICE_HPP_
#define _VMM_DEVICE_HPP_

FOCP_BEGIN();

#ifdef MSVC
#define VMM_INVALID_ADDR64 0xFFFFFFFFFFFFFFFFi64
#else
#define VMM_INVALID_ADDR64 0xFFFFFFFFFFFFFFFFULL
#endif
#define VMM_MAX_PAGECOUNT (VMM_MAX_FILESIZE>>VMM_PAGE_BIT)
#define VMM_MAGIC_CODE 0xABCDEF01

struct CVirtualDeviceHead
{
	uint32 nMagic;
	uint32 nTotalPage;
	uint32 nFreePage;
	uint32 nAllocator;
	uint8 pTable[4096];
	uint32 nMaxMountPage;
};

#define VMM_DEVICE_HEADSIZE sizeof(CVirtualDeviceHead)

class CVirtualDevice
{
public:
	CMutex m_oMutex;
	CVirtualDeviceHead* m_pHead;
	CVirtualFile m_oFile;
	uint64 m_nDeviceAddr;
	uint32 m_nMod;
	uint32 m_nDirty;
	uint32 m_nChange;
	char m_sFile[FOCP_MAX_PATH];
	uint16 * m_pSwappTable;
	uint8* m_pPages;

public:
	CVirtualDevice(uint32 nDeviceNo, uint32 nSize);
	CVirtualDevice(uint32 nDeviceNo, const char* sFileName, uint32 nSize, bool bCreate);
	~CVirtualDevice();

	uint32 GetFreeSize();

	int32 Read(uint32 nAddr, char* pBuf, int32 nReadSize);
	int32 Write(uint32 nAddr, char* pBuf, int32 nWriteSize);

	void MountPage(uint32 nPageId, char* pBuf);
	void CommitPage(uint32 nPageId, char* pBuf);

	void* GetMemoryAddr(uint32 nAddr);
	void Flush();

	uint64 AllocatePage();
	void DeAllocatePage(uint64 nPageAddr);

	uint32 BackupTo(const char* sBackupFileName, char* pBuf);
	uint32 RestoreFrom(const char* sBackupFileName, char* pBuf);

private:
	void CreateDevice(bool bCreate);
};

FOCP_END();

#endif
