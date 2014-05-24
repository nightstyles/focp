
#include "VmmSwap.hpp"

#ifndef _VMM_STORAGE_HPP_
#define _VMM_STORAGE_HPP_

FOCP_BEGIN();

#ifndef VMM_DEVICE_CAP
#define VMM_DEVICE_CAP 8192
#endif

class CVirtualStorage: public CVmmSwapper
{
public:
	CMutex m_oMutex;
	uint32 m_nDeviceQuantity;
	CVirtualDevice* m_pDevices[VMM_DEVICE_CAP];
	char m_sConfigPath[FOCP_MAX_PATH], m_sDevicePath[FOCP_MAX_PATH];
	uint64 m_nDiskSpaceAddr;
	uint64 m_nMemorySpaceAddr;

public:
	CVirtualStorage();
	virtual ~CVirtualStorage();

	uint32 BackupTo(const char* sBackupPath);

	void CleanStorage();
	uint32 Load(const char* sConfilePath, const char* sDevicePath, const char* sBackupPath);

	int32 Read(uint64 nAddr, void* pMem, int32 nMemSize);
	int32 Write(uint64 nAddr, void* pMem, int32 nMemSize);

	void* GetMemoryAddress(uint64 nAddr);

	uint32 CreateMemoryDevice(uint32 nSize);
	uint32 CreateFileDevice(const char* sFileName, uint32 nSize);

	uint32 GetVirtualDeviceCount();
	uint32 GetVirtualDeviceInfo(uint32 nDeviceNo, CVirtualDeviceInfo* pInfo);

	uint64 AllocatePage(int32 bInMemory);
	void DeAllocatePage(uint64 nPageAddr);

	uint64 AllocateBlock(uint64 nPageAddr);
	bool DeAllocateBlock(uint64 nBlockAddr, uint32 &nBlockSize);

	bool GetSystemPage(uint64 &nPageAddr);

	void FlushAll(bool bFlushAll=false);

	void SaveConfig();
};

FOCP_END();

#endif
