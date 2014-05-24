
#include "VmmStorage.hpp"

FOCP_BEGIN();

#ifdef MSVC
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000i64)
#else
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000ULL)
#endif

const char* GetShellPath(char* sShellPath)
{
	CString::StringCopy(sShellPath, CFilePathInfo::GetInstance()->GetDir());
	CDiskFileSystem::GetInstance()->GetOsPathName(sShellPath);
	return sShellPath;
}

CVirtualStorage::CVirtualStorage()
:CVmmSwapper(this)
{
	m_nDeviceQuantity = 0;
	for(uint32 i=0; i<VMM_DEVICE_CAP; ++i)
		m_pDevices[i] = NULL;
	m_sConfigPath[0] = 0;
	m_sDevicePath[0] = 0;
	m_nDiskSpaceAddr = 0;
	m_nMemorySpaceAddr = 0;
}

CVirtualStorage::~CVirtualStorage()
{
}

void CVirtualStorage::CleanStorage()
{
	if(m_nMemorySpaceAddr)
	{
		void * pSpace = GetGlobalSpace(1);
		SetVirtualSpaceAddress(pSpace, 0);
		m_nMemorySpaceAddr = 0;
	}
	if(m_nDiskSpaceAddr)
	{
		void * pSpace = GetGlobalSpace(0);
		SetVirtualSpaceAddress(pSpace, 0);
		m_nDiskSpaceAddr = 0;
	}
	CommitAllPages(true);
	for(uint32 i=0; i<m_nDeviceQuantity; ++i)
	{
		m_pDevices[i]->Flush();
		delete m_pDevices[i];
	}
	SetVmmControlStatus(VMM_CONTROL_DIRTY, 0);
	CleanSwapper();
}

void CVirtualStorage::FlushAll(bool bFlushAll)
{
	CommitAllPages(bFlushAll);
	for(uint32 i=0; i<m_nDeviceQuantity; ++i)
		m_pDevices[i]->Flush();
	SetVmmControlStatus(VMM_CONTROL_DIRTY, 0);
}

int32 CVirtualStorage::Read(uint64 nAddr, void* pMem, int32 nMemSize)
{
#ifdef VMM_COMPLEX_MAP
	return m_pDevices[nAddr>>32]->Read((uint32)nAddr, (char*)pMem, nMemSize);
#else
	return m_pDevices[(nAddr>>32)&0x7FFFFFFF]->Read((uint32)nAddr, (char*)pMem, nMemSize);
#endif
}

int32 CVirtualStorage::Write(uint64 nAddr, void* pMem, int32 nMemSize)
{
#ifdef VMM_COMPLEX_MAP
	return m_pDevices[nAddr>>32]->Write((uint32)nAddr, (char*)pMem, nMemSize);
#else
	return m_pDevices[(nAddr>>32)&0x7FFFFFFF]->Write((uint32)nAddr, (char*)pMem, nMemSize);
#endif
}

void* CVirtualStorage::GetMemoryAddress(uint64 nAddr)
{
	if(nAddr&VMM_MEMORY_FLAG)
#ifdef VMM_COMPLEX_MAP
		return m_pDevices[nAddr>>32]->m_pPages + (nAddr&0x7FFFFFFF);
#else
		return GET_MEMORY_ADDR(nAddr);
#endif
	return NULL;
}

uint32 CVirtualStorage::GetVirtualDeviceCount()
{
	return m_nDeviceQuantity;
}

uint64 CVirtualStorage::AllocatePage(int32 bInMemory)
{
	register uint64 nAddr = VMM_INVALID_ADDR64;

	register bool bMemory = bInMemory?true:false;

	for(register uint32 i=0; i<m_nDeviceQuantity; ++i)
	{
		register CVirtualDevice* pDevice = m_pDevices[i];
		register bool bMemoryDevice = (pDevice->m_nDeviceAddr&VMM_MEMORY_FLAG)?true:false;
		if(bMemoryDevice == bMemory)
		{
			nAddr = pDevice->AllocatePage();
			if(nAddr != VMM_INVALID_ADDR64)
				break;
		}
	}

	return nAddr;
}

void CVirtualStorage::DeAllocatePage(uint64 nPageAddr)
{
#ifdef VMM_COMPLEX_MAP
	m_pDevices[nPageAddr>>32]->DeAllocatePage(nPageAddr);
#else
	m_pDevices[(nPageAddr>>32)&0x7FFFFFFF]->DeAllocatePage(nPageAddr);
#endif
}

uint64 CVirtualStorage::AllocateBlock(uint64 nPageAddr)
{
	register uint64 nBlockAddr;
#if VMM_SAFE_ENABLED
	if(nPageAddr & 0xFFFF)
		Abort();
#endif
	if(nPageAddr & VMM_MEMORY_FLAG)
	{
		register CVirtualPageHead* pHead = GET_MEMORY_OBJECT(CVirtualPageHead, nPageAddr);
		CVirtualPage oPage(nPageAddr, pHead);
		nBlockAddr = oPage.Allocate();
	}
	else
	{
		register CVirtualCachePage* pPage = MapPage(nPageAddr);
		CVirtualPage oPage(nPageAddr, pPage);
		nBlockAddr = oPage.Allocate();
		UnMapPage(pPage);
	}
	return nBlockAddr;
}

bool CVirtualStorage::DeAllocateBlock(uint64 nBlockAddr, uint32 &nBlockSize)
{
	register uint64 nPageAddr;
	if(nBlockAddr & VMM_MEMORY_FLAG)
	{
#ifdef VMM_COMPLEX_MAP
		nPageAddr = ComputePageAddr(nBlockAddr);
#else
		register uint32 nHeadAddr = (uint32)m_pDevices[nBlockAddr>>32]->m_pPages;
		nPageAddr = ComputePageAddr(nBlockAddr - nHeadAddr) + nHeadAddr;
#endif
		register CVirtualPageHead* pHead = GET_MEMORY_OBJECT(CVirtualPageHead, nPageAddr);
		CVirtualPage oPage(nPageAddr, pHead);
		return oPage.Deallocate(nBlockAddr, nBlockSize);
	}
	else
	{
		nPageAddr = ComputePageAddr(nBlockAddr);
		register CVirtualCachePage* pPage = MapPage(nPageAddr);
		CVirtualPage oPage(nPageAddr, pPage);
		bool bRet = oPage.Deallocate(nBlockAddr, nBlockSize);
		UnMapPage(pPage);
		return bRet;
	}
}

uint32 CVirtualStorage::GetVirtualDeviceInfo(uint32 nDeviceNo, CVirtualDeviceInfo* pInfo)
{
	CVirtualDevice* pDevice = NULL;
	if(nDeviceNo < m_nDeviceQuantity)
		pDevice = m_pDevices[nDeviceNo];
	if(!pDevice)
		return 1;
	pInfo->nDeviceSize = pDevice->m_oFile.GetSize();
	pInfo->nFreeSpace = pDevice->GetFreeSize();
	CString::StringCopy(pInfo->sDeviceName, pDevice->m_sFile);
	return 0;
}

uint32 CVirtualStorage::CreateMemoryDevice(uint32 nSize)
{
	m_oMutex.Enter();
	if(m_nDeviceQuantity == VMM_DEVICE_CAP)
	{
		m_oMutex.Leave();
		FocpError(("CVirtualStorage::CreateMemoryDevice() failure: too many devices"));
		return 1;
	}
	CVirtualDevice* pDevice = new(std::nothrow) CVirtualDevice(m_nDeviceQuantity, nSize);
	if(!pDevice)
	{
		m_oMutex.Leave();
		FocpError(("CVirtualStorage::CreateMemoryDevice() failure: memory lack"));
		return 1;
	}
	if(!pDevice->m_oFile.IsValidFile())
	{
		m_oMutex.Leave();
		delete pDevice;
		FocpError(("CVirtualStorage::CreateMemoryDevice() failure: invalid device"));
		return 1;
	}
	m_pDevices[m_nDeviceQuantity] = pDevice;
	++m_nDeviceQuantity;
	if(!m_nMemorySpaceAddr)
	{
		void * pSpace = GetGlobalSpace(1);
		CreateVirtualSpace(pSpace, 1);
		m_nMemorySpaceAddr = GetVirtualSpaceAddress(pSpace);
	}
	m_oMutex.Leave();
	SaveConfig();
	return 0;
}

uint32 CVirtualStorage::CreateFileDevice(const char* sFileName, uint32 nSize)
{
	CFormatString oFileName;
	if(CString::CharOfString(sFileName, '/') || CString::CharOfString(sFileName, '\\'))
	{
		FocpError(("CVirtualStorage::CreateMemoryDevice(%s): cann't include '/' and '\\'", sFileName));
		return 1;
	}
	oFileName.Print("%s/%s", m_sDevicePath, sFileName);
	m_oMutex.Enter();
	if(m_nDeviceQuantity == VMM_DEVICE_CAP)
	{
		m_oMutex.Leave();
		FocpError(("CVirtualStorage::CreateMemoryDevice() failure: too many devices"));
		return 1;
	}
	CVirtualDevice* pDevice = new(std::nothrow) CVirtualDevice(m_nDeviceQuantity, oFileName.GetStr(), nSize, true);
	if(!pDevice)
	{
		m_oMutex.Leave();
		FocpError(("CVirtualStorage::CreateFileDevice(%s) failure: memory lack", oFileName.GetStr()));
		return 1;
	}
	if(!pDevice->m_oFile.IsValidFile())
	{
		m_oMutex.Leave();
		delete pDevice;
		FocpError(("CVirtualStorage::CreateFileDevice(%s) failure: invalid device", oFileName.GetStr()));
		return 1;
	}
	m_pDevices[m_nDeviceQuantity] = pDevice;
	++m_nDeviceQuantity;
	if(!m_nDiskSpaceAddr)
	{
		uint64 nFirstPageAddr = pDevice->AllocatePage();
		uint8* pFirstPage = new uint8[VMM_PAGE_SIZE];
		CBinary::MemorySet(pFirstPage, 0, VMM_PAGE_SIZE);
		pDevice->Write((uint32)nFirstPageAddr, (char*)pFirstPage, VMM_PAGE_SIZE);
		delete[] pFirstPage;
		void * pSpace = GetGlobalSpace(0);
		CreateVirtualSpace(pSpace, 0);
		m_nDiskSpaceAddr = GetVirtualSpaceAddress(pSpace);
	}
	m_oMutex.Leave();
	SaveConfig();
	return 0;
}

bool CVirtualStorage::GetSystemPage(uint64 &nPageAddr)
{
	for(uint32 i=0; i<VMM_DEVICE_CAP; ++i)
	{
		CVirtualDevice* pDevice = m_pDevices[i];
		if(!pDevice)
			break;
		if(pDevice->m_nDeviceAddr&VMM_MEMORY_FLAG)
			continue;
		nPageAddr = pDevice->m_nDeviceAddr;
		return true;
	}
	return false;
}

void CVirtualStorage::SaveConfig()
{
	CFormatString oFileName;
	CFormatString oCommand;

	char* sShellPath = new char[FOCP_MAX_PATH];
	char* sConfigPath = new char[FOCP_MAX_PATH];

	GetShellPath(sShellPath);
	CString::StringCopy(sConfigPath, m_sConfigPath);
	CDiskFileSystem::GetInstance()->GetOsPathName(sConfigPath);

	m_oMutex.Enter();
#ifdef WINDOWS
	oCommand.Print("%s/mv.exe -f %s/vmm.cfg %s/vmm.cfg.bak", sShellPath, sConfigPath, sConfigPath);
#else
	oCommand.Print("mv -f %s/vmm.cfg %s/vmm.cfg.bak", sConfigPath, sConfigPath);
#endif
	System(oCommand.GetStr());
	oFileName.Print("disk://%s/vmm.cfg", m_sConfigPath);
	CFile oFile;
	if(oFile.Open(oFileName.GetStr(), "c"))
	{
		m_oMutex.Leave();
		FocpError(("CVirtualMemoryManager::SaveConfig() open vmm config file failure"));
		delete[] sShellPath;
		delete[] sConfigPath;
		return;
	}
	oFile.Write(&m_nDiskSpaceAddr, sizeof(m_nDiskSpaceAddr));
	oFile.Write(&m_nDeviceQuantity, sizeof(m_nDeviceQuantity));
	for(uint32 i=0; i<m_nDeviceQuantity; ++i)
	{
		CVirtualDevice* pDevice = m_pDevices[i];
		uint32 nDeviceSize = pDevice->m_oFile.GetSize();
		oFile.Write(&nDeviceSize, 4);
		oFile.Write(pDevice->m_sFile, FOCP_MAX_PATH);
	}
	oCommand.Clear();
#ifdef WINDOWS
	oCommand.Print("%s/rm.exe -f %s/vmm.cfg.bak", sShellPath, sConfigPath);
#else
	oCommand.Print("rm -f %s/vmm.cfg.bak", sConfigPath);
#endif
	System(oCommand.GetStr());
	m_oMutex.Leave();
	delete[] sShellPath;
	delete[] sConfigPath;
}

uint32 CVirtualStorage::Load(const char* sConfilePath, const char* sDevicePath, const char* sBackupPath)
{
	char* sShellPath = new char[FOCP_MAX_PATH];
	char* sConfigPath2 = new char[FOCP_MAX_PATH];
	char* sDevicePath2 = new char[FOCP_MAX_PATH];

	CString::StringCopy(m_sConfigPath, sConfilePath);
	CString::StringCopy(m_sDevicePath, sDevicePath);

	CString::StringCopy(sConfigPath2, m_sConfigPath);
	CDiskFileSystem::GetInstance()->GetOsPathName(sConfigPath2);
	CString::StringCopy(sDevicePath2, sDevicePath);
	CDiskFileSystem::GetInstance()->GetOsPathName(sDevicePath2);

	GetShellPath(sShellPath);

	if(sBackupPath)
	{
		CFormatString oCommand;
#ifdef WINDOWS
		oCommand.Print("%s/rm.exe -f %s/*", sShellPath, sDevicePath2);
#else
		oCommand.Print("rm -rf %s/*", sDevicePath2);
#endif
		System(oCommand.GetStr());
	}

	bool bOld = false;
	CFormatString oFileName;
	oFileName.Print("disk://%s/vmm.cfg.bak", m_sConfigPath);
	CFile oFile;
	if(!oFile.Open(oFileName.GetStr(), "w"))
		bOld = true;
	else
	{
		oFileName.Clear();
		oFileName.Print("disk://%s/vmm.cfg", m_sConfigPath);
		if(oFile.Open(oFileName.GetStr(), "c"))
		{
			FocpError(("CVirtualMemoryManager::Load() open vmm config file failure"));
			delete[] sShellPath;
			delete[] sConfigPath2;
			delete[] sDevicePath2;
			return 1;
		}
	}
	m_nDiskSpaceAddr = 0;
	if(oFile.Read(&m_nDiskSpaceAddr, sizeof(m_nDiskSpaceAddr)) != sizeof(m_nDiskSpaceAddr))
		m_nDiskSpaceAddr = 0;
	uint32 nCount = 0;
	if(oFile.Read(&nCount, sizeof(nCount)) != sizeof(nCount))
		nCount = 0;
	bool bHasMemoryDevice = false;
	char * pBuf = NULL;
	if(sBackupPath)
	{
		pBuf = new(std::nothrow) char[1048576];
		if(!pBuf)
		{
			delete[] sShellPath;
			delete[] sConfigPath2;
			delete[] sDevicePath2;
			FocpError(("CVirtualMemoryManager::Load() no enough memnory"));
			return 1;
		}
	}
	bool bFirst = true;
	uint8* pFirstPage = new uint8[VMM_PAGE_SIZE];
	CBinary::MemorySet(pFirstPage, 0, VMM_PAGE_SIZE);
	for(uint32 i=0; i<nCount; ++i)
	{
		char sFile[FOCP_MAX_PATH];
		uint32 nDeviceSize;
		oFile.Read(&nDeviceSize, 4);
		oFile.Read(sFile, FOCP_MAX_PATH);
		CVirtualDevice* pDevice;
		if(sFile[0])
			pDevice = new(std::nothrow) CVirtualDevice(m_nDeviceQuantity, sFile, nDeviceSize, sBackupPath!=NULL);
		else
			pDevice = new(std::nothrow) CVirtualDevice(m_nDeviceQuantity, nDeviceSize);
		if(!pDevice)
		{
			if(pBuf)
				delete[] pBuf;
			delete[] pFirstPage;
			delete[] sShellPath;
			delete[] sConfigPath2;
			delete[] sDevicePath2;
			FocpError(("CVirtualStorage::Load(%s) failure: memory lack", sFile));
			return 1;
		}
		if(!pDevice->m_oFile.IsValidFile())
		{
			if(pBuf)
				delete[] pBuf;
			delete[] pFirstPage;
			delete[] sShellPath;
			delete[] sConfigPath2;
			delete[] sDevicePath2;
			delete pDevice;
			FocpError(("CVirtualStorage::Load(%s) failure", sFile));
			return 1;
		}
		m_pDevices[m_nDeviceQuantity] = pDevice;
		++m_nDeviceQuantity;
		if(!sFile[0])
			bHasMemoryDevice = true;
		else if(sBackupPath)
		{
			oFileName.Clear();
			oFileName.Print("%s/%s", sBackupPath, sFile + CString::StringLength(m_sDevicePath) + 1);
			uint32 nRet = pDevice->RestoreFrom(oFileName.GetStr(), pBuf);
			if(nRet == 2)
			{
				if(bFirst)
				{
					uint64 nFirstPageAddr = pDevice->AllocatePage();
					pDevice->Write((uint32)nFirstPageAddr, (char*)pFirstPage, VMM_PAGE_SIZE);					
					void * pSpace = GetGlobalSpace(0);
					CreateVirtualSpace(pSpace, 0);
					m_nDiskSpaceAddr = GetVirtualSpaceAddress(pSpace);
				}
			}
			else if(nRet)
			{
				if(pBuf)
					delete[] pBuf;
				delete[] pFirstPage;
				delete[] sShellPath;
				delete[] sConfigPath2;
				delete[] sDevicePath2;
				delete pDevice;
				return 1;
			}
			else if(bFirst)
				SetVirtualSpaceAddress(GetGlobalSpace(0), m_nDiskSpaceAddr);
			bFirst = false;
		}
		else
		{
			if(bFirst)
				SetVirtualSpaceAddress(GetGlobalSpace(0), m_nDiskSpaceAddr);
			bFirst = false;
		}
	}
	delete[] pFirstPage;
	if(bOld)
	{
		CFormatString oCommand;
#ifdef WINDOWS
		oCommand.Print("%s/cp.exe -f %s/vmm.cfg.bak %s/vmm.cfg", sShellPath, sConfigPath2, sConfigPath2);
#else
		oCommand.Print("cp -f %s/vmm.cfg.bak %s/vmm.cfg", sConfigPath2, sConfigPath2);
#endif
		System(oCommand.GetStr());
		oCommand.Clear();
#ifdef WINDOWS
		oCommand.Print("%s/rm.exe -f %s/vmm.cfg.bak", sShellPath, sConfigPath2);
#else
		oCommand.Print("rm -f %s/vmm.cfg.bak", sConfigPath2);
#endif
		System(oCommand.GetStr());
	}
	if(pBuf)
		delete[] pBuf;
	if(bHasMemoryDevice)
	{
		void * pSpace = GetGlobalSpace(1);
		CreateVirtualSpace(pSpace, 1);
		m_nMemorySpaceAddr = GetVirtualSpaceAddress(pSpace);
	}
	delete[] sShellPath;
	delete[] sConfigPath2;
	delete[] sDevicePath2;
	return 0;
}

uint32 CVirtualStorage::BackupTo(const char* sBackupPath)
{
	char * pBuf = new char[1048576];
	for(uint32 i=0; i<m_nDeviceQuantity; ++i)
	{
		CVirtualDevice* pDevice = m_pDevices[i];
		if(pDevice->m_sFile[0])
		{
			CFormatString oFileName;
			oFileName.Print("%s/%s", sBackupPath, pDevice->m_sFile + CString::StringLength(m_sDevicePath) + 1);
			if(pDevice->BackupTo(oFileName.GetStr(), pBuf))
			{
				delete[] pBuf;
				return 1;
			}
		}
	}
	delete[] pBuf;
	return 0;
}

FOCP_END();
