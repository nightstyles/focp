
#include "VmmDevice.hpp"

FOCP_BEGIN();

CVirtualDevice::~CVirtualDevice()
{
	if(m_pHead)
	{
		if(!m_pPages)
			CMalloc::Free(m_pHead);
		m_pHead = NULL;
	}
	if(m_pSwappTable)
	{
		CMalloc::Free(m_pSwappTable);
		m_pSwappTable = NULL;
	}
}

CVirtualDevice::CVirtualDevice(uint32 nDeviceNo, uint32 nSize)
:m_oFile(nSize)
{
	m_nDeviceAddr = nDeviceNo;
	m_nDeviceAddr <<= 32;
	m_nDeviceAddr += VMM_MEMORY_FLAG;
	m_pHead = NULL;
	m_pPages = NULL;
	CBinary::MemorySet(m_sFile, 0, sizeof(m_sFile));
	m_nDirty = 0;
	m_nChange = 0;
	m_pSwappTable = NULL;
	if(m_oFile.IsValidFile())
		CreateDevice(true);
}

CVirtualDevice::CVirtualDevice(uint32 nDeviceNo, const char* sFileName, uint32 nSize, bool bCreate)
:m_oFile(sFileName, nSize, bCreate)
{
	m_nDeviceAddr = nDeviceNo;
	m_nDeviceAddr <<= 32;
	m_pHead = NULL;
	CBinary::MemorySet(m_sFile, 0, sizeof(m_sFile));
	CString::StringCopy(m_sFile, sFileName);
	m_nDirty = 0;
	m_nChange = 0;
	m_pSwappTable = NULL;
	m_pPages = NULL;
	if(m_oFile.IsValidFile())
		CreateDevice(bCreate);
}

void CVirtualDevice::CreateDevice(bool bCreate)
{
	if(bCreate)
	{
		if(m_nDeviceAddr & VMM_MEMORY_FLAG)
			m_pHead = (CVirtualDeviceHead*)m_oFile.GetMemoryAddr(0);
		else
		{
			m_pHead = (CVirtualDeviceHead*)CMalloc::Malloc(VMM_DEVICE_HEADSIZE);
			if(!m_pHead)
			{
				m_oFile.Close();
				CDiskFileSystem::GetInstance()->RemovePath(m_sFile);
				FocpError(("CVirtualDevice::CreateDevice(%s) failue: memory not enough", m_sFile));
				return;
			}
		}
		uint32 nUsableSize = m_oFile.GetSize() - VMM_DEVICE_HEADSIZE;
        CBinary::MemorySet(m_pHead, 0, VMM_DEVICE_HEADSIZE);
		m_pHead->nMaxMountPage = 0;
        m_pHead->nMagic = VMM_MAGIC_CODE;
		m_pHead->nFreePage = m_pHead->nTotalPage = (nUsableSize >> VMM_PAGE_BIT);
		m_nMod = ((m_pHead->nTotalPage-1)>>3)+1;
		if(m_nDeviceAddr & VMM_MEMORY_FLAG)
		{
			uint32 nMod = m_pHead->nTotalPage & 7;
			if(nMod)
			{
				static uint8 x[] = {1,3,7,15,31,63,127};
				m_pHead->pTable[m_nMod-1] = ~x[nMod-1];
			}
		}
		else
		{
			m_nDirty = 1;
			m_nChange = 1;
			SetVmmControlStatus(VMM_CONTROL_DIRTY, 1);
			m_nMod = 4096;
		}
	}
	else
	{
		m_pHead = (CVirtualDeviceHead*)CMalloc::Malloc(VMM_DEVICE_HEADSIZE);
		if(!m_pHead)
		{
			m_oFile.Close();
			CDiskFileSystem::GetInstance()->RemovePath(m_sFile);
			FocpError(("CVirtualDevice::OpenDevice(%s) failue: memory not enough", m_sFile));
			return;
		}
		int32 nRet = m_oFile.Read(0, (char*)m_pHead, VMM_DEVICE_HEADSIZE);
		int32 nSize = (m_pHead->nTotalPage<<VMM_PAGE_BIT) + VMM_DEVICE_HEADSIZE;
		if(nRet != VMM_DEVICE_HEADSIZE ||
			m_pHead->nMagic != VMM_MAGIC_CODE ||
			m_oFile.GetSize() != (uint32)nSize ||
			m_pHead->nAllocator >= m_pHead->nTotalPage || 
			m_pHead->nMaxMountPage > m_pHead->nTotalPage ||
			m_pHead->nFreePage > m_pHead->nTotalPage)
		{
			FocpError(("CVirtualDevice::OpenDevice(%s) failure: invalid virtual device file", m_sFile));
			m_oFile.Close();
			return;
		}
		m_nMod = 4096;
	}
	if(m_nDeviceAddr & VMM_MEMORY_FLAG)
		m_pPages = m_oFile.m_pMemFile->pPages + VMM_DEVICE_HEADSIZE;
	else
	{
		m_pSwappTable = (uint16*)CMalloc::Malloc(m_pHead->nTotalPage*sizeof(uint16));
		if(m_pSwappTable)
		{
			for(uint16 i=0; i<m_pHead->nTotalPage; ++i)
				m_pSwappTable[i] = 0xFFFF;
		}
		else
		{
			FocpError(("CVirtualDevice::CreateDevice(%s) failure: memory not enough", m_sFile));
			m_oFile.Close();
		}
	}
}

uint32 CVirtualDevice::GetFreeSize()
{
	m_oMutex.Enter();
	uint32 nSize = m_pHead->nFreePage << VMM_PAGE_BIT;
	m_oMutex.Leave();
	return nSize;
}

int32 CVirtualDevice::Read(uint32 nAddr, char* pBuf, int32 nReadSize)
{
	int32 nRet;
#ifndef VMM_COMPLEX_MAP
	if(m_pPages)
	{
		nRet = nReadSize;
		m_oMutex.Enter();
		CBinary::MemoryCopy(pBuf, (void*)nAddr, nReadSize);
		m_oMutex.Leave();
		return nRet;
	}
#endif
	uint32 nDeviceAddr = (nAddr&0x7FFFFFFF) + VMM_DEVICE_HEADSIZE;
	nRet = nReadSize;
	nRet = m_oFile.Read(nDeviceAddr, pBuf, nReadSize);
	m_oMutex.Leave();
	return nRet;
}

int32 CVirtualDevice::Write(uint32 nAddr, char* pBuf, int32 nWriteSize)
{
	int32 nRet;
#ifndef VMM_COMPLEX_MAP
	if(m_pPages)
	{
		nRet = nWriteSize;
		m_oMutex.Enter();
		CBinary::MemoryCopy((void*)nAddr, pBuf, nWriteSize);
		m_oMutex.Leave();
		return nRet;
	}
#endif
	uint32 nDeviceAddr = (nAddr&0x7FFFFFFF) + VMM_DEVICE_HEADSIZE;
	m_oMutex.Enter();
	uint32 nFileSize = m_oFile.GetSize();
	if(nDeviceAddr + nWriteSize > nFileSize)
		nWriteSize = nFileSize - nDeviceAddr;
	nRet = m_oFile.Write(nDeviceAddr, pBuf, nWriteSize);
	m_nChange = 1;
	m_oMutex.Leave();
	return nRet;
}

void CVirtualDevice::MountPage(uint32 nPageId, char* pBuf)
{
	m_oMutex.Enter();
	if(nPageId >= m_pHead->nMaxMountPage)
	{
		m_pHead->nMaxMountPage = nPageId + 1;
		m_nDirty = 1;
		m_nChange = 1;
		SetVmmControlStatus(VMM_CONTROL_DIRTY, 1);
	}
	else
	{
		uint32 nDeviceAddr = (nPageId << VMM_PAGE_BIT) + VMM_DEVICE_HEADSIZE;
		if(m_oFile.Read(nDeviceAddr, pBuf, VMM_PAGE_SIZE) != VMM_PAGE_SIZE)
			FocpAbort(("MountPage(%u) of the device %s failure", nPageId, m_sFile));
	}
	m_oMutex.Leave();
}

void CVirtualDevice::CommitPage(uint32 nPageId, char* pBuf)
{
	uint32 nDeviceAddr = (nPageId << VMM_PAGE_BIT) + VMM_DEVICE_HEADSIZE;
	m_oMutex.Enter();
	if(m_oFile.Write(nDeviceAddr, pBuf, VMM_PAGE_SIZE) != VMM_PAGE_SIZE)
		FocpAbort(("CommitPage(%u) of the device %s failure", nPageId, m_sFile));
	m_nChange = 1;
	m_oMutex.Leave();
}

void* CVirtualDevice::GetMemoryAddr(uint32 nAddr)
{
	if(m_pPages)
#ifdef VMM_COMPLEX_MAP
		return m_pPages + (nAddr&0x7FFFFFFF);
#else
		return (void*)nAddr;
#endif
	return NULL;
}

void CVirtualDevice::Flush()
{
	if(!m_pPages)
	{
		m_oMutex.Enter();
		if(m_nDirty)
		{
			m_oFile.Write(0, (char*)m_pHead, VMM_DEVICE_HEADSIZE);
			m_nDirty = 0;
		}
		if(m_nChange)
			m_oFile.Flush();
		m_oMutex.Leave();
	}
}

void SaveVmmConfig();
extern uint32 g_nAutoIncPageSize;

uint64 CVirtualDevice::AllocatePage()
{
	m_oMutex.Enter();
	if(!m_pHead->nFreePage)
	{
		if((m_nDeviceAddr & VMM_MEMORY_FLAG) || m_pHead->nTotalPage == 32767)
		{
			m_oMutex.Leave();
			return VMM_INVALID_ADDR64;
		}
	}
	uint8 nBit=1, iBit;
	uint8* pTable = m_pHead->pTable;
	uint32& nAllocator = m_pHead->nAllocator;
	if(nAllocator == m_nMod)
		nAllocator = 0;
	uint32 nPage;
	while(true)
	{
loop:
		if(pTable[nAllocator] != 0xFF)
		{
			for(iBit=0; iBit<8; ++iBit)
			{
				if(!(nBit & pTable[nAllocator]))
				{
					nPage = (nAllocator<<3) + iBit;
					if(nPage == m_pHead->nTotalPage)
					{
						if(m_pHead->nFreePage)
						{
							nAllocator = 0;
							goto loop;
						}
						uint32 nInc = g_nAutoIncPageSize;
						if(nInc + m_pHead->nTotalPage > 32767)
							nInc = 32767 - m_pHead->nTotalPage;
						uint16 * pNewSwappTable = (uint16*)CMalloc::Realloc(m_pSwappTable, (m_pHead->nTotalPage+nInc)*sizeof(uint16));
						if(!pNewSwappTable)
						{
							m_oMutex.Leave();
							return VMM_INVALID_ADDR64;
						}
						m_pSwappTable = pNewSwappTable;
						for(uint32 i=0; i<nInc; ++i)
							m_pSwappTable[m_pHead->nTotalPage+i] = 0xFFFF;
						if(m_oFile.Expand(nInc*65536))
						{
							nAllocator = 0;
							m_oMutex.Leave();
							return VMM_INVALID_ADDR64;
						}
						SaveVmmConfig();
						m_pHead->nFreePage += nInc;
						m_pHead->nTotalPage += nInc;
					}
					pTable[nAllocator] |= nBit;
					--m_pHead->nFreePage;
					if(!m_pPages)
					{
						m_nDirty = 1;
						m_nChange = 1;
						SetVmmControlStatus(VMM_CONTROL_DIRTY, 1);
					}
					break;
				}
				nBit <<= 1;
			}
			break;
		}
		++nAllocator;
		if(nAllocator == m_nMod)
			nAllocator = 0;
	}
	m_oMutex.Leave();
	uint32 nPageAddr = nPage << VMM_PAGE_BIT;
	uint64 nRet = m_nDeviceAddr + nPageAddr;
#ifndef VMM_COMPLEX_MAP
	if(m_pPages)
		nRet += (uint32)m_pPages;
#endif
	return nRet;
}

void CVirtualDevice::DeAllocatePage(uint64 nPageAddr)
{
#ifndef VMM_COMPLEX_MAP
	if(m_pPages)
		nPageAddr -= (uint32)m_pPages;
#endif
	uint32 nPage = ((uint32)(nPageAddr-m_nDeviceAddr));
	if(nPage & 0xFFFF)
		FocpAbort(("CVirtualDevice::DeAllocatePage(%u64) failure, device name is %s", nPageAddr, m_sFile));
	nPage >>= VMM_PAGE_BIT;
	uint32 nAllocator = (nPage>>3), iBit=(nPage&7);
	uint32 nBit = (1<<iBit);
	if(nPage >= m_pHead->nTotalPage)
		FocpAbort(("CVirtualDevice::DeAllocatePage(%u64) failure, device name is %s", nPageAddr, m_sFile));
	m_oMutex.Enter();
	if(m_pHead->pTable[nAllocator] & nBit)
	{
		m_pHead->pTable[nAllocator] &= ~nBit;
		++m_pHead->nFreePage;
		if(!m_pPages)
		{
			m_nDirty = 1;
			m_nChange = 1;
			SetVmmControlStatus(VMM_CONTROL_DIRTY, 1);
		}
	}
	else
		FocpAbort(("CVirtualDevice::DeAllocatePage(%u64) failure, device name is %s", nPageAddr, m_sFile));
	m_oMutex.Leave();
}

uint32 CVirtualDevice::BackupTo(const char* sBackupFileName, char * pBuf)
{
	if(!m_nChange)
		return 0;

	CFile oFile;
	CString oFileName("disk://");
	oFileName += sBackupFileName;
	sBackupFileName = oFileName.GetStr();

	if(oFile.Open(sBackupFileName, "wcd"))
	{
		FocpError(("CVirtualDevice::BackupTo(%s) failure: open file failure", sBackupFileName));
		return 1;
	}

	uint32 nAddr = 0;
	uint32 nTotalSize = VMM_DEVICE_HEADSIZE + m_pHead->nMaxMountPage * VMM_PAGE_SIZE;
	while(nTotalSize)
	{
		int nReadSize = nTotalSize;
		if(nReadSize > 1048576)
			nReadSize = 1048576;
		m_oFile.Read(nAddr, pBuf, nReadSize);
		if(nReadSize != oFile.Write(pBuf, nReadSize))
		{
			FocpError(("CVirtualDevice::BackupTo(%s) failure: write file failure", sBackupFileName));
			return 1;
		}
		nAddr += nReadSize;
		nTotalSize -= nReadSize;
	}

	return 0;
}

uint32 CVirtualDevice::RestoreFrom(const char* sBackupFileName, char * pBuf)
{
	CFile oFile;
	CString oFileName("disk://");
	oFileName += sBackupFileName;
	sBackupFileName = oFileName.GetStr();

	if(oFile.Open(sBackupFileName, "r"))
		return 2;
	if(VMM_DEVICE_HEADSIZE != oFile.Read(m_pHead, VMM_DEVICE_HEADSIZE))
	{
		FocpError(("CVirtualDevice::RestoreFrom(%s) failure: read file failure", sBackupFileName));
		return 1;
	}

	oFile.Seek(FOCP_SEEK_SET, 0);
	uint32 nTotalSize = m_pHead->nMaxMountPage * VMM_PAGE_SIZE;
	m_oFile.Write(0, (char*)m_pHead, VMM_DEVICE_HEADSIZE);
	uint32 nAddr = VMM_DEVICE_HEADSIZE;
	while(nTotalSize)
	{
		int nReadSize = nTotalSize;
		if(nReadSize > 1048576)
			nReadSize = 1048576;
		if(1 != oFile.Read(pBuf, nReadSize))
		{
			FocpError(("CVirtualDevice::RestoreFrom(%s) failure: read file failure", sBackupFileName));
			return 1;
		}
		m_oFile.Write(nAddr, pBuf, nReadSize);
		nAddr += nReadSize;
		nTotalSize -= nReadSize;
	}

	return 0;
}

FOCP_END();
