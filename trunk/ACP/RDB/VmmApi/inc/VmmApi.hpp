
#include "AFC.hpp"

#ifndef _VMM_API_HPP_
#define _VMM_API_HPP_

#if defined(VMMAPI_EXPORTS)
#define VMM_API FOCP_EXPORT
#else
#define VMM_API FOCP_IMPORT
#endif

FOCP_BEGIN();

#define VMM_SAFE_ENABLED 0
#ifdef FOCP_64BIT
#define VMM_COMPLEX_MAP
#endif

#ifdef VMM_COMPLEX_MAP
#define VMM_MEMORY_FLAG 0x80000000
#else
#ifdef MSVC
#define VMM_MEMORY_FLAG 0x8000000000000000i64
#else
#define VMM_MEMORY_FLAG 0x8000000000000000ULL
#endif
#endif

/* control status */
#define VMM_CONTROL_DIRTY 0
#define VMM_CONTROL_BACKUP 1
VMM_API int32 InitializeVmmControler(const char* sControlFile, uint32 nStatusNum);
VMM_API uint8 GetVmmControlStatus(uint32 nIdx);
VMM_API void SetVmmControlStatus(uint32 nIdx, uint8 nStatus);

/* Startup & Cleanup */
VMM_API int32 StartupVmm(const char* sConfilePath, const char* sDevicePath, const char* sBackupPath, uint32 nSwapSize, uint32 nAutoIncPageSize);
VMM_API void CleanupVmm();
VMM_API uint32 BackupVmm(const char* sBackupPath);
VMM_API bool GetSystemPage(uint64 * pPageAddr);

/*
	Device Management Function
	The nDeviceSize and nFreeSpace's unit is mega-bytes
*/
typedef struct CVirtualDeviceInfo{uint32 nDeviceSize, nFreeSpace;char sDeviceName[256];}CVirtualDeviceInfo;
VMM_API uint32 CreateMemoryDevice(uint32 nSize);
VMM_API uint32 CreateFileDevice(uint32 nSize, const char* sFileName);
VMM_API uint32 GetVirtualDeviceCount();
VMM_API uint32 GetVirtualDeviceInfo(uint32 nDeviceNo, CVirtualDeviceInfo* pInfo);

/* Direct Read & Write Device Space */
VMM_API int32 DirectReadVmm(uint64 nAddr, char* pMem, int32 nSize);
VMM_API int32 DirectWriteVmm(uint64 nAddr, char* pMem, int32 nSize);
VMM_API void* GetMemoryAddress(uint64 nAddr);
VMM_API int32 IsMemoryAddr(uint64 nAddr);
#define IS_MEMORY_ADDR(nAddr) ((nAddr&VMM_MEMORY_FLAG)?1:0)
#ifdef VMM_COMPLEX_MAP
#define GET_MEMORY_ADDR(nAddr) GetMemoryAddress(nAddr)
#define GET_MEMORY_OBJECT(T, nAddr) (T*)GetMemoryAddress(nAddr)
#else
#define GET_MEMORY_ADDR(nAddr) ((void*)(uint32)(nAddr))
#define GET_MEMORY_OBJECT(T, nAddr) ((T*)(uint32)(nAddr))
#endif

/* Virtual Space Management Function */
VMM_API void* NewVirtualSpace();
VMM_API void DeleteVirtualSpace(void* pSpace);

VMM_API bool CreateVirtualSpace(void* pSpace, int32 bInMemory);
VMM_API void DestroyVirtualSpace(void* pSpace);

VMM_API uint64 GetVirtualSpaceAddress(void* pSpace);
VMM_API void SetVirtualSpaceAddress(void* pSpace, uint64 nAddr);

VMM_API int32 VirtualSpaceInMemory(void* pSpace);

VMM_API uint64 AllocateFromVirtualSpace(void* pSpace, uint32 nSize);
VMM_API void DeAllocateIntoVirtualSpace(void* pSpace, uint64 nAddr);
VMM_API void ClearVirtualSpace(void* pSpace);

VMM_API void* GetGlobalSpace(int32 bInMemory);

/* Global Space's alloc & free */
VMM_API uint64 vmalloc(uint32 size, int32 bInMemory);
VMM_API void vfree(uint64 nAddr);

/* storage mapping function */
VMM_API void* vmap(uint64 nAddr);
VMM_API void unvmap(void* pMem);
VMM_API void vflush(void* pMem);

VMM_API void vquery(uint64 nAddr, char* pMem, uint32 nSize);
VMM_API void vcommit(uint64 nAddr, char* pMem, uint32 nSize);
VMM_API void vmemset(uint64 nAddr, int32 c, uint32 nSize);
VMM_API void vmemcpy(uint64 nDst, uint64 nSrc, uint32 nSize);

VMM_API void FlushVmm();

FOCP_END();

#endif
