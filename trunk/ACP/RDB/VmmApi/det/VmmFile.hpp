
#include "VmmApi.hpp"

#ifndef _VMM_FILE_HPP_
#define _VMM_FILE_HPP_

FOCP_BEGIN();

#define VMM_PAGE_SIZE 65536
#define VMM_PAGE_BIT 16
#define VMM_MAX_FILESIZE 0x7FFFFFFF

class CVirtualFile
{
public:
	struct CVirtualDiskFile
	{
		uint32 nSize;
		CFile nFile;
		uint32 bDirty;

		CVirtualDiskFile();
		~CVirtualDiskFile();
	};

	struct CVirtualMemFile
	{
		uint32 nSize;
		uint8* pPages;

		CVirtualMemFile();
		~CVirtualMemFile();
	};

public:
	CVirtualMemFile* m_pMemFile;
	CVirtualDiskFile* m_pDiskFile;

public:
	CVirtualFile(uint32 nSize);
	CVirtualFile(const char* sFileName, uint32 nSize, bool bCreate);

	~CVirtualFile();

	void Close();

	bool IsValidFile();

	uint32 GetSize();

	int32 Read(uint32 nAddr, char* pBuf, int32 nReadSize);
	int32 Write(uint32 nAddr, char* pBuf, int32 nWriteSize);

	int32 Expand(uint32 nSize);

	void* GetMemoryAddr(uint32 nAddr);
	void Flush();
};

FOCP_END();

#endif
