
#include "FileSystem.hpp"

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

#ifdef WINDOWS
#include <direct.h>
#else
#include <unistd.h>
#endif

FOCP_BEGIN();

static void ToughFileTime(const char* sOldFile, const char* sNewFile);

CDiskFileSystem::CDiskFileSystem()
{
	CBinary::MemorySet(m_sCurrentDirectory, 0, FOCP_MAX_PATH);
#ifdef CYGWIN_NT
	if(RunInWindows())
	{
		CString::StringCopy(m_sCurrentDirectory, "/cygdrive");
		GetProgramFileName(m_sCurrentDirectory+9);
		GetAfcPathName(m_sCurrentDirectory+9);
	}
	else
#endif
	{
		GetProgramFileName(m_sCurrentDirectory);
		GetAfcPathName(m_sCurrentDirectory);
	}
	uint32 nLen = CString::StringLength(m_sCurrentDirectory);
	while(nLen)
	{
		--nLen;
		if(m_sCurrentDirectory[nLen] == '/')
		{
			m_sCurrentDirectory[nLen] = '\0';
			break;
		}
	}
	SetCurrentDirectory(m_sCurrentDirectory);
}

CDiskFileSystem::~CDiskFileSystem()
{
}

CDiskFileSystem* CDiskFileSystem::GetInstance()
{
	return CSingleInstance<CDiskFileSystem>::GetInstance();
}

bool CDiskFileSystem::CheckFileName(const char* sName)
{
	//斜线、反斜线、冒号、星号、问号、竖线、大于号、小于号、双引号、分号
	const char* s = "/\\:*?|><\";";
	char * s2 = CString::SetOfString(sName, s);//strcspn
	if(s2 && s2[0])
		return false;
	return true;
}

const char* CDiskFileSystem::GetCurrentDirectory()
{
	return m_sCurrentDirectory;
}

bool CDiskFileSystem::SetCurrentDirectory(const char* sDirectory)
{
	CPathDetailInfo oDetailInfo;
	char sPath[FOCP_MAX_PATH];
	if(!GetFullPath(sDirectory, sPath))
		return false;
	if(!GetOsPathName(sPath, &oDetailInfo))
		return false;
	if(oDetailInfo.bExist == false)
		return false;
	if(oDetailInfo.sFilePart)
		return false;
	m_oMutex.Enter();
	chdir(sPath);
	GetAfcPathName(sPath);
	CString::StringCopy(m_sCurrentDirectory, sPath);
	m_oMutex.Leave();
	return true;
}

enum
{
	FOCP_PATH_SECTION_ROOT,
	FOCP_PATH_SECTION_CURRENT,
	FOCP_PATH_SECTION_PREV,
	FOCP_PATH_SECTION_COMMON,
};

bool CDiskFileSystem::GetPathSection(const char* &sAfcPath, const char* sEnd, CString &oName, uint32 &nType)
{
	if(sAfcPath[0] == '/' || sAfcPath[0] == '\\')
	{
		nType = FOCP_PATH_SECTION_ROOT;
		sAfcPath++;
		return true;
	}
	if(sAfcPath[0] == '.')
	{
		if(sAfcPath[1] == '/' || sAfcPath[1] == '\\')
		{
			nType = FOCP_PATH_SECTION_CURRENT;
			sAfcPath += 2;
			return true;
		}
		if(sAfcPath[1] == '.' && (sAfcPath[2] == '/' || sAfcPath[2] == '\\'))
		{
			nType = FOCP_PATH_SECTION_PREV;
			sAfcPath += 3;
			return true;
		}
	}
	while(sAfcPath != sEnd)
	{
		switch(sAfcPath[0])
		{
		case ':': case '*': case '?': case '|': case '>': case '<': case '\"': case ';':
			return false;
		}
		if(sAfcPath[0] == '\\' || sAfcPath[0] == '/')
		{
			++sAfcPath;
			break;
		}
		oName += sAfcPath[0];
		++sAfcPath;
	}
	if(oName.Empty())
		return false;
	if(!oName.Compare("."))
		nType = FOCP_PATH_SECTION_CURRENT;
	else if(!oName.Compare(".."))
		nType = FOCP_PATH_SECTION_PREV;
	else
		nType = FOCP_PATH_SECTION_COMMON;
	return true;
}

static char* GetPrevPath(char* pRoot)
{
	char* s = CString::CharOfString(pRoot, '/', true);
	if(s == NULL)
		s = pRoot;
	s[0] = '\0';
	return s;
}

////////////////////////////////////////
//全路径格式:
// Windows:
//	/c
//	/c/aa
// CYGWIN_NT:
//	/cygdrive/c
//	/cygdrive/c/aa
// UNIX:
//	/
//	/aa
////////////////////////////////////////
bool CDiskFileSystem::GetFullPath(const char* sAfcPath, char* pBuf, CPathDetailInfo* pDetailInfo)
{
	bool bMeetRoot = false;
#ifdef WINDOWS
	bool bMeetDirver = false;
#endif

	while(sAfcPath[0] && CString::IsSpace(sAfcPath[0]))
		++sAfcPath;
	if(!sAfcPath[0])
		return false;
	const char* sEnd = sAfcPath;
	while(sEnd[0])
		++sEnd;
	--sEnd;
	while(CString::IsSpace(sEnd[0]))
		--sEnd;
	++sEnd;

	char* pShift = pBuf;
	char* sRoot = pBuf;
#ifdef WINDOWS
	sRoot += 2;
#elif defined(CYGWIN_NT)
	if(!CString::StringCompare(sAfcPath, "/cygdrive/", false, 10))
	{
		CString::StringCopy(pShift, sAfcPath, 11);
		if(pShift[10] >= 'A' && pShift[10] <= 'Z')
		{
			pShift[10] -= 'A';
			pShift[10] += 'a';
		}
		if(pShift[10] < 'a' || pShift[10] > 'z')
			return false;
		pShift += 11;
		sRoot += 11;
		sAfcPath += 11;
		if(sAfcPath[0] == '\\' || sAfcPath[0] == '/')
			++sAfcPath;
		else if(sAfcPath != sEnd)
			return false;
		bMeetRoot = true;
	}
#endif

	uint32 nLen;
	pShift[0] = '\0';
	while(sAfcPath != sEnd)
	{
		uint32 nType;
		CString oName;
		if(!GetPathSection(sAfcPath, sEnd, oName, nType))
			return false;
		if(!bMeetRoot)
		{
			bMeetRoot = true;
			if(nType != FOCP_PATH_SECTION_ROOT)
			{
				m_oMutex.Enter();
				CString::StringCopy(pShift, m_sCurrentDirectory);
				m_oMutex.Leave();
				pShift += CString::StringLength(pShift);
#ifdef WINDOWS
				bMeetDirver = true;
#endif
			}
		}
		switch(nType)
		{
		case FOCP_PATH_SECTION_PREV:
#ifdef WINDOWS
			if(!bMeetDirver)
				return false;
#endif
			pShift = GetPrevPath(sRoot);
			break;
		case FOCP_PATH_SECTION_COMMON:
			nLen = oName.GetSize();
			if(pShift - pBuf + nLen + 1 >= FOCP_MAX_PATH)
				return false;
			pShift[0] = '/';
			CString::StringCopy(pShift+1, oName.GetStr());
#ifdef WINDOWS
			if(!bMeetDirver)
			{
				if(pShift[1] >= 'A' && pShift[1] <= 'Z')
				{
					pShift[1] -= 'A';
					pShift[1] += 'a';
				}
				if(pShift[1] < 'a' || pShift[1] > 'z')
					return false;
				if(pShift[2])
					return false;
				bMeetDirver = true;
			}
#endif
			pShift += nLen + 1;
			break;
		}
	}
	if(!bMeetRoot)
		return false;
#ifdef WINDOWS
	if(!bMeetDirver)
	{
		pShift[0] = '/';
		m_oMutex.Enter();
		pShift[1] = m_sCurrentDirectory[1];
		m_oMutex.Leave();
		pShift[2] = '\0';
	}
#else
	if(pShift == pBuf)
	{
		pShift[0] = '/';
		pShift[1] = '\0';
	}
#endif
	if(pDetailInfo)
	{
		if(!GetOsPathName(pBuf, pDetailInfo))
			return false;
		GetAfcPathName(pBuf);
	}
	return true;
}

bool CDiskFileSystem::GetOsPathNameEx(void* pDir, char* sDirectory, char* sFileName, CPathDetailInfo* pDetailInfo)
{
	const char* sFileName2;
	bool bIsDirectory;
#ifdef WINDOWS
	char* sSeparator = (char*)"\\";
#else
	char* sSeparator = (char*)"/";
#endif

	while((sFileName2 = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
	{
		if(!CString::StringCompare(sFileName2, ".") || !CString::StringCompare(sFileName2, ".."))
			continue;
		uint32 nLen = CString::StringLength(sFileName2);
		if(!CBinary::MemoryCompare(sFileName, sFileName2, nLen, false) && (sFileName[nLen]==sSeparator[0] || sFileName[nLen]=='\0'))
		{
			CBinary::MemoryCopy(sFileName, sFileName2, nLen);
			if(bIsDirectory)
			{
				if(sFileName[nLen]=='\0')
				{
					pDetailInfo->bExist = true;
					pDetailInfo->sFilePart = NULL;
					return true;
				}
				sFileName += nLen + 1;
				if(sDirectory[CString::StringLength(sDirectory)-1] != sSeparator[0])
					CString::StringCatenate(sDirectory, sSeparator);
				CString::StringCatenate(sDirectory, sFileName2);
				void * pDir2 = FOCP_NAME::OpenDirectory(sDirectory);
				if(pDir2 == NULL)
					return false;
				bool bRet = GetOsPathNameEx(pDir2, sDirectory, sFileName, pDetailInfo);
				FOCP_NAME::CloseDirectory(pDir2);
				return bRet;
			}
			if(sFileName[nLen] == sSeparator[0])
				return false;
			pDetailInfo->bExist = true;
			pDetailInfo->sFilePart = sFileName;
			return true;
		}
	}
	pDetailInfo->bExist = false;
	pDetailInfo->sFilePart = sFileName;
	return true;
}

bool CDiskFileSystem::GetOsPathName(char* sAfcFullPath, CPathDetailInfo* pDetailInfo)
{
	char* pShift;

	if(sAfcFullPath[0] != '/' && sAfcFullPath[0] != '\\')
		return false;
#ifdef WINDOWS
	sAfcFullPath[0] = sAfcFullPath[1];
	if(sAfcFullPath[0] >= 'a')
	{
		sAfcFullPath[0] -= 'a';
		sAfcFullPath[0] += 'A';
	}
	if(sAfcFullPath[0] < 'A' || sAfcFullPath[0] > 'Z')
		return false;
	sAfcFullPath[1] = ':';
	pShift = sAfcFullPath;
	while(*pShift)
	{
		if(*pShift == '/')
			*pShift = '\\';
		++pShift;
	}
	if(sAfcFullPath[2] && sAfcFullPath[2] != '\\')
		return false;
#else
	pShift = sAfcFullPath;
	while(*pShift)
	{
		if(*pShift == '\\')
			*pShift = '/';
		++pShift;
	}
#endif

#ifdef CYGWIN_NT
	bool bCygdrive = !CString::StringCompare(sAfcFullPath, "/cygdrive/", false, 10);
	if(bCygdrive)
	{
		if(sAfcFullPath[10] >= 'A' && sAfcFullPath[10] <= 'Z')
		{
			sAfcFullPath[10] -= 'A';
			sAfcFullPath[10] += 'a';
		}
		if(sAfcFullPath[10] < 'a' || sAfcFullPath[10] > 'z')
			return false;
		if(sAfcFullPath[11] && sAfcFullPath[11] != '/')
			return false;
	}
#endif

	if(pDetailInfo == NULL)
		return true;

	char sRoot[FOCP_MAX_PATH];
#ifdef WINDOWS
	sRoot[0] = sAfcFullPath[0];
	sRoot[1] = ':';
	sRoot[2] = 0;
	pShift = sAfcFullPath + 2;
	if(sAfcFullPath[2])
		++pShift;
#elif defined(CYGWIN_NT)
	if(bCygdrive)
	{
		CString::StringCopy(sRoot, sAfcFullPath, 11);
		pShift = sAfcFullPath + 11;
		sRoot[11] = '/';
		sRoot[12] = '\0';
		if(sAfcFullPath[11])
			++pShift;
	}
	else
	{
		sRoot[0] = '/';
		sRoot[1] = 0;
		pShift = sAfcFullPath + 1;
	}
#else
	sRoot[0] = '/';
	sRoot[1] = 0;
	pShift = sAfcFullPath + 1;
#endif

	void * pDir = FOCP_NAME::OpenDirectory(sRoot);
	if(pDir == NULL)
	{
		pDetailInfo->bExist = false;
		pDetailInfo->sFilePart = sAfcFullPath;
		return true;
	}
	if(!pShift[0])
	{
		FOCP_NAME::CloseDirectory(pDir);
		pDetailInfo->bExist = true;
		pDetailInfo->sFilePart = NULL;
		return true;
	}
	bool bRet = GetOsPathNameEx(pDir, sRoot, pShift, pDetailInfo);
	FOCP_NAME::CloseDirectory(pDir);
	return bRet;
}

void* CDiskFileSystem::OpenDirectory(const char* sDirectory)
{
	CPathDetailInfo oDetailInfo;
	char sPath[FOCP_MAX_PATH];
	if(!GetFullPath(sDirectory, sPath))
		return NULL;
	if(!GetOsPathName(sPath, &oDetailInfo))
		return NULL;
	if(oDetailInfo.bExist == false)
		return NULL;
	if(oDetailInfo.sFilePart)
		return NULL;
	return FOCP_NAME::OpenDirectory(sPath);
}

void CDiskFileSystem::CloseDirectory(void* pDirectory)
{
	FOCP_NAME::CloseDirectory(pDirectory);
}

const char* CDiskFileSystem::ReadDirectory(void* pDirectory, bool &bIsDirectory)
{
	return FOCP_NAME::ReadDirectory(pDirectory, bIsDirectory);
}

void CDiskFileSystem::RewindDirectory(void* pDirectory)
{
	FOCP_NAME::RewindDirectory(pDirectory);
}

char CDiskFileSystem::GetOsPathSeparator()
{
#ifdef WINDOWS
	return '\\';
#else
	return '/';
#endif
}

bool CDiskFileSystem::CreateDirectory(const char* sDirectory)
{
	CPathDetailInfo oDetailInfo;
	char sPath[FOCP_MAX_PATH];
	char nSeparator = GetOsPathSeparator();

	if(!GetFullPath(sDirectory, sPath))
		return false;
	if(!GetOsPathName(sPath, &oDetailInfo))
		return false;
	if(oDetailInfo.bExist)
	{
		if(oDetailInfo.sFilePart)
			return false;
		return true;
	}
	if(oDetailInfo.sFilePart == NULL)
		return false;

	char* pShift = oDetailInfo.sFilePart;
	while(true)
	{
		char* pNext = CString::CharOfString(pShift, nSeparator);
		if(pNext)
			*pNext = '\0';
#ifdef WINDOWS
		if(mkdir(sPath))
#else
		if(mkdir(sPath, 0755))
#endif
			return false;
		if(pNext == NULL)
			break;
		pShift = pNext + 1;
		*pNext = nSeparator;
	}

	return true;
}

bool CDiskFileSystem::RemovePath(const char* sPathName, bool bRecursive)
{
	CPathDetailInfo oDetailInfo;
	char sPath[FOCP_MAX_PATH];

	if(!GetFullPath(sPathName, sPath))
		return false;

	if(!GetOsPathName(sPath, &oDetailInfo))
		return false;

	if(oDetailInfo.bExist == false)
		return false;

	if(oDetailInfo.sFilePart)
	{
		if(unlink(sPath))
			return false;
	}
	else
	{
		if(rmdir(sPath))
		{
			if(bRecursive)
			{
				char sFile[FOCP_MAX_PATH];
				return RecurseRemoveDirectory(sPath, sFile);
			}
			return false;
		}
	}
	return true;
}

bool CDiskFileSystem::RecurseRemoveDirectory(char* sPath, char* sFile)
{
	bool bIsDirectory;
	const char* sFileName;

	void* pDir = FOCP_NAME::OpenDirectory(sPath);
	if(pDir == NULL)
		return false;

	char nSeparator = GetOsPathSeparator();
	char* sEnd = sPath + CString::StringLength(sPath);

	while((sFileName = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
	{
		if(!CString::StringCompare(sFileName, ".") || !CString::StringCompare(sFileName, ".."))
			continue;
		sprintf(sFile, "%s%c%s", sPath, nSeparator, sFileName);
		if(bIsDirectory)
		{
			if(rmdir(sFile))
			{
				CString::StringCopy(sPath, sFile);
				if(RecurseRemoveDirectory(sPath, sFile) == false)
				{
					FOCP_NAME::CloseDirectory(pDir);
					return false;
				}
				*sEnd = '\0';
			}
		}
		else
		{
			if(unlink(sFile))
			{
				FOCP_NAME::CloseDirectory(pDir);
				return false;
			}
		}
	}
	FOCP_NAME::CloseDirectory(pDir);

	if(rmdir(sPath))
		return false;
	return true;
}

bool CDiskFileSystem::RenamePath(const char* sOldPathName, const char* sNewPathName)
{
	CPathDetailInfo oDetailInfo1, oDetailInfo2;
	char sPath1[FOCP_MAX_PATH], sPath2[FOCP_MAX_PATH];

	if(!GetFullPath(sOldPathName, sPath1))
		return false;

	if(!GetFullPath(sNewPathName, sPath2))
		return false;

	if(!GetOsPathName(sPath1, &oDetailInfo1))
		return false;

	if(!GetOsPathName(sPath2, &oDetailInfo2))
		return false;

	if(oDetailInfo1.bExist == false)
		return false;

	if(oDetailInfo2.bExist)
	{
		if(((oDetailInfo1.sFilePart == NULL) && oDetailInfo2.sFilePart) ||
				(oDetailInfo1.sFilePart && (oDetailInfo2.sFilePart==NULL)))
			return false;
	}

#if defined(WINDOWS)
	char* sPath;
	struct stat oStat1;
	if(oDetailInfo1.sFilePart)//移动文件
	{
		struct stat oStat2;
		if(oDetailInfo2.bExist)
		{
			if(stat(sPath2, &oStat2))
				return false;
			if(!(oStat2.st_mode & _S_IWRITE))
				return false;
		}
		if(stat(sPath1, &oStat1))
			return false;
		if(!(oStat1.st_mode & _S_IWRITE))
			return false;
		sPath = sPath1 + CString::StringLength(sPath1) - 1;
		while(*sPath != '\\')
			--sPath;
		*sPath = '\0';
		if(stat(sPath1, &oStat1))
			return false;
		if(!(oStat1.st_mode & (_S_IWRITE|_S_IEXEC)))
			return false;
		*sPath = '\\';
		if(CopyFile(sPath1, sPath2) == false)
			return false;
	}
	else //移动目录
	{
		sPath = sPath1 + CString::StringLength(sPath1) - 1;
		while(*sPath != '\\')
			--sPath;
		*sPath = '\0';
		if(stat(sPath1, &oStat1))
			return false;
		if(!(oStat1.st_mode & (_S_IWRITE|_S_IEXEC)))
			return false;
		*sPath = '\\';
		if(sPath1[0] == sPath2[0])
		{
			if(!MoveDirectory(sPath1, sPath2, oDetailInfo2.bExist) == false)
				return false;
		}
		else
		{
			char sBuf1[FOCP_MAX_PATH], sBuf2[FOCP_MAX_PATH];
			if(!CopyDirectory(sPath1, sPath2, sBuf1, sBuf2, oDetailInfo2.bExist) == false)
				return false;
		}
	}
	if(unlink(sPath1))
		return false;
	return true;
#else
	return (0==rename(sPath1, sPath2));
#endif
}

bool CDiskFileSystem::CopyPath(const char* sOldPathName, const char* sNewPathName)
{
	CPathDetailInfo oDetailInfo1, oDetailInfo2;
	char sPath1[FOCP_MAX_PATH], sPath2[FOCP_MAX_PATH];

	if(!GetFullPath(sOldPathName, sPath1))
		return false;

	if(!GetFullPath(sNewPathName, sPath2))
		return false;

	if(!GetOsPathName(sPath1, &oDetailInfo1))
		return false;

	if(!GetOsPathName(sPath2, &oDetailInfo2))
		return false;

	if(oDetailInfo1.bExist == false)
		return false;

	if(oDetailInfo2.bExist)
	{
		if(((oDetailInfo1.sFilePart == NULL) && oDetailInfo2.sFilePart) ||
				(oDetailInfo1.sFilePart && (oDetailInfo2.sFilePart==NULL)))
			return false;
	}

	if(oDetailInfo1.sFilePart)//复制文件
		return CopyFile(sPath1, sPath2);

	char sBuf1[FOCP_MAX_PATH], sBuf2[FOCP_MAX_PATH];
	return CopyDirectory(sPath1, sPath2, sBuf1, sBuf2, oDetailInfo2.bExist, true);
}

bool CDiskFileSystem::CopyFile(const char* sOldFile, const char* sNewFile)
{
	//这里不使用CFile,因为已经寻找到了操作系统的文件，再使用CFile有点罗嗦。
	FILE* fp1 = fopen(sOldFile, "rb");
	if(fp1 == NULL)
		return false;
	FILE* fp2;
	bool bNewFileExist = false;
	fp2 = fopen(sNewFile, "rb");
	if(fp2)
	{
		bNewFileExist = true;
		fclose(fp2);
	}
	fp2 = fopen(sNewFile, "wb");
	if(fp2 == NULL)
	{
		fclose(fp1);
		return false;
	}
	char* s = new char[1048576];
	uint32 nReadLen = 0, nWriteLen = 1;
	while(!feof(fp1))
	{
		nReadLen = fread(s, 1048576, 1, fp1);
		if(nReadLen <= 0)
			break;
		nWriteLen = fwrite(s, nReadLen, 1, fp2);
		if(nWriteLen <= 0)
			break;
	}
	fclose(fp1);
	fclose(fp2);
	delete[] s;
	if(nReadLen < 0)
	{
		if(!bNewFileExist)
			unlink(sNewFile);
		return false;
	}
	if(nWriteLen <= 0)
	{
		if(!bNewFileExist)
			unlink(sNewFile);
		return false;
	}
	if(!bNewFileExist)
		ToughFileTime(sOldFile, sNewFile);
	return true;
}

bool CDiskFileSystem::CopyDirectory(char* sOldDirectory, char* sNewDirectory, char* sBuf1, char* sBuf2, bool bExist, bool bForce)
{
	void * pDir;
	const char* sFileName;
	bool bIsDirectory;

#ifdef WINDOWS
	char nSeparator = '\\';
#else
	char nSeparator = '/';
#endif

	if(!bExist)
	{
#ifdef WINDOWS
		if(mkdir(sNewDirectory))
#else
		if(mkdir(sNewDirectory, 0755))
#endif
			return false;
	}
	else if(bForce == false)
	{
		pDir = FOCP_NAME::OpenDirectory(sNewDirectory);
		if(pDir == NULL)
			return false;
		while((sFileName = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
		{
			if(!CString::StringCompare(sFileName, ".") || !CString::StringCompare(sFileName, ".."))
				continue;
			FOCP_NAME::CloseDirectory(pDir);
			return false;
		}
		FOCP_NAME::CloseDirectory(pDir);
	}
	pDir = FOCP_NAME::OpenDirectory(sOldDirectory);
	if(pDir == NULL)
		return false;
	char* sEnd1 = sOldDirectory + CString::StringLength(sOldDirectory);
	char* sEnd2 = sNewDirectory + CString::StringLength(sNewDirectory);
	while((sFileName = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
	{
		if(!CString::StringCompare(sFileName, ".") || !CString::StringCompare(sFileName, ".."))
			continue;
		sprintf(sBuf1, "%s%c%s", sOldDirectory, nSeparator, sFileName);
		sprintf(sBuf2, "%s%c%s", sNewDirectory, nSeparator, sFileName);
		if(bIsDirectory)
		{
			bool bExist2 = false;
			if(bExist)
			{
				void* pDir2 = FOCP_NAME::OpenDirectory(sBuf2);
				if(pDir == NULL)
					bExist2 = false;
				else
				{
					bExist2 = true;
					FOCP_NAME::CloseDirectory(pDir2);
				}
			}
			if(!CopyDirectory(sBuf1, sBuf2, sOldDirectory, sNewDirectory, bExist2, true))
			{
				FOCP_NAME::CloseDirectory(pDir);
				return false;
			}
			CString::StringCopy(sOldDirectory, sBuf1);
			CString::StringCopy(sNewDirectory, sBuf2);
			*sEnd1 = '\0';
			*sEnd2 = '\0';
		}
		else
		{
			if(!CopyFile(sBuf1, sBuf2))
			{
				FOCP_NAME::CloseDirectory(pDir);
				return false;
			}
		}
	}

	FOCP_NAME::CloseDirectory(pDir);
	return true;
}

bool CDiskFileSystem::AccessPath(const char* sPathName, uint32 nMode, bool &bDirectory)
{
	struct stat oStat;
	CPathDetailInfo oDetailInfo;
	char sPath[FOCP_MAX_PATH];

	if(!GetFullPath(sPathName, sPath))
		return false;

	if(!GetOsPathName(sPath, &oDetailInfo))
		return false;

	if(oDetailInfo.bExist == false)
		return false;

	if(oDetailInfo.sFilePart)
		bDirectory = false;
	else
		bDirectory = true;

	if(stat(sPath, &oStat))
		return false;

	if(nMode & FOCP_FILEACCESS_WRITABLE)
	{
#ifdef WINDOWS
		if(!(oStat.st_mode & _S_IWRITE))
			return false;
#else
		if(!(oStat.st_mode & S_IWUSR))
			return false;
#endif
	}

	if(nMode & FOCP_FILEACCESS_EXECUTABLE)
	{
#ifdef WINDOWS
		if(!(oStat.st_mode & _S_IEXEC))
			return false;
#else
		if(!(oStat.st_mode & S_IXUSR))
			return false;
#endif
	}

	return true;
}

bool CDiskFileSystem::SearchFile(const char* sEnvVariable, const char* sFileName, char* pBuf, const char* sPrefixName, const char* sPostfixName)
{
	const char* sSearchPathList = GetEnvVar(sEnvVariable);
#ifdef WINDOWS
	if(!CString::StringCompare(sEnvVariable, "path", false))
		return SearchFileEx(sSearchPathList, sFileName, pBuf, ';', sPrefixName, sPostfixName);
#endif
	return SearchFileEx(sSearchPathList, sFileName, pBuf, ':', sPrefixName, sPostfixName);
}

bool CDiskFileSystem::SearchFileEx(const char* sSearchPathList, const char* sFileName, char* pBuf, char cSeparator, const char* sPrefixName, const char* sPostfixName)
{
	void * pDir;
	uint32 nLen;
	bool bRet = false;
	bool bIsDirectory;
	const char* sFileName2, *pShift;
	CPathDetailInfo oDetailInfo;
	char sSearchPath[FOCP_MAX_PATH], sSearchFullPath[FOCP_MAX_PATH];

	if(sPrefixName == NULL)
		sPrefixName = "";
	if(sPostfixName == NULL)
		sPostfixName = "";

	m_oMutex.Enter();
	CString::StringCopy(sSearchFullPath, m_sCurrentDirectory);
	m_oMutex.Leave();
	goto first;
	while(sSearchPathList)
	{
		pShift = (const char*)CString::CharOfString(sSearchPathList, cSeparator);
		if(pShift)
		{
			nLen = pShift - sSearchPathList;
			CBinary::MemoryCopy(sSearchPath, sSearchPathList, nLen);
			sSearchPath[nLen] = 0;
			sSearchPathList = pShift + 1;
		}
		else
		{
			CString::StringCopy(sSearchPath, sSearchPathList);
			sSearchPathList = NULL;
		}
		if(!GetFullPath(sSearchPath, sSearchFullPath))
			continue;
first:
		if(!GetOsPathName(sSearchFullPath, &oDetailInfo))
			continue;
		if(oDetailInfo.bExist == false)
			continue;
		if(oDetailInfo.sFilePart)
			continue;
		pDir = FOCP_NAME::OpenDirectory(sSearchFullPath);
		if(pDir == NULL)
			continue;
		while((sFileName2 = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
		{
			if(!CString::StringCompare(sFileName2, ".") || !CString::StringCompare(sFileName2, ".."))
				continue;
			if(bIsDirectory)
				continue;
			if(!CString::StringCompare(sFileName2, sFileName, false))
			{
				bRet = true;
				break;
			}
			sprintf(sSearchPath, "%s%s", sPrefixName, sFileName);
			if(sPrefixName[0])
			{
				if(!CString::StringCompare(sFileName2, sSearchPath, false))
				{
					bRet = true;
					break;
				}
			}
			if(sPostfixName[0])
			{
				CString::StringCatenate(sSearchPath, sPostfixName);
				if(!CString::StringCompare(sFileName2, sSearchPath, false))
				{
					bRet = true;
					break;
				}
				if(sPrefixName[0] && !CString::StringCompare(sFileName2, sSearchPath+CString::StringLength(sPrefixName), false))
				{
					bRet = true;
					break;
				}
			}
		}
		if(bRet)
		{
			GetAfcPathName(sSearchFullPath);
			sprintf(pBuf, "%s/%s", sSearchFullPath, sFileName2);
			FOCP_NAME::CloseDirectory(pDir);
			break;
		}
		FOCP_NAME::CloseDirectory(pDir);
	}
	return bRet;
}

bool CDiskFileSystem::MoveDirectory(char* sOldDirectory, char* sNewDirectory, bool bExist)
{
	void * pDir;
	const char* sFileName;
	bool bIsDirectory;
	char sBuf1[FOCP_MAX_PATH], sBuf2[FOCP_MAX_PATH];

#ifdef WINDOWS
	char nSeparator = '\\';
#else
	char nSeparator = '/';
#endif

	if(!bExist)
	{
#ifdef WINDOWS
		if(mkdir(sNewDirectory))
#else
		if(mkdir(sNewDirectory, 0755))
#endif
			return false;
	}

	pDir = FOCP_NAME::OpenDirectory(sOldDirectory);
	if(pDir == NULL)
		return false;

	while((sFileName = FOCP_NAME::ReadDirectory(pDir, bIsDirectory)))
	{
		if(!CString::StringCompare(sFileName, ".") || !CString::StringCompare(sFileName, ".."))
			continue;
		sprintf(sBuf1, "%s%c%s", sOldDirectory, nSeparator, sFileName);
		sprintf(sBuf2, "%s%c%s", sNewDirectory, nSeparator, sFileName);
		if(!RenamePath(sBuf1, sBuf2))
		{
			FOCP_NAME::CloseDirectory(pDir);
			return false;
		}
	}

	FOCP_NAME::CloseDirectory(pDir);
	return true;
}

char* CDiskFileSystem::GetAfcPathName(char* sOsFullPath)
{
	if(sOsFullPath[1] == ':')
	{
		sOsFullPath[1] = sOsFullPath[0];
		sOsFullPath[0] = '\\';
		char* pShift = sOsFullPath;
		while(*pShift)
		{
			if(*pShift == '\\')
				*pShift = '/';
			++pShift;
		}
		if(sOsFullPath[0] >= 'A')
		{
			sOsFullPath[0] -= 'A';
			sOsFullPath[0] += 'a';
		}
	}
	return sOsFullPath;
}

void CDiskFileSystem::GetTmpPath(char* sAfcPath)
{
	if(RunInWindows())
	{
		sAfcPath[0] = '\0';
		const char* sTmp = GetEnvVar("TEMP");
		if(!sTmp)
			sTmp = GetEnvVar("TMP");
#ifdef WINDOWS
		CString::StringCopy(sAfcPath, sTmp);
		sAfcPath[1] = sAfcPath[0];
		sAfcPath[0] = '/';
		if(sAfcPath[1] >= 'A')
		{
			sAfcPath[1] -= 'A';
			sAfcPath[1] += 'a';
		}
#elif defined(CYGWIN_NT)
		CString::StringCopy(sAfcPath, "/cygdrive");
		CString::StringCopy(sAfcPath+9, sTmp);
		sAfcPath[10] = sAfcPath[9];
		sAfcPath[9] = '/';
		if(sAfcPath[1] >= 'A')
		{
			sAfcPath[10] -= 'A';
			sAfcPath[10] += 'a';
		}
#else
		sAfcPath[0] = '\0';
		return;
#endif
		char* pShift = sAfcPath;
		while(*pShift)
		{
			if(pShift[0] == '\\')
				pShift[0] = '/';
			++pShift;
		}
	}
	else
		CString::StringCopy(sAfcPath, "/tmp");
}

CFilePathInfo::CFilePathInfo(const char* sPath)
{
	CPathDetailInfo oDetail;
	char sTmp[FOCP_MAX_PATH+9] = {0};
	m_sPath[0] = m_sName[0] = m_sHome[0] = m_sDir[0] = '\0';
	CDiskFileSystem* pFileSystem = CDiskFileSystem::GetInstance();
	if(sPath == NULL)
	{
#ifdef WINDOWS
		GetProgramFileName(sTmp);
		pFileSystem->GetAfcPathName(sTmp);
#elif defined(CYGWIN_NT)
		if(RunInWindows())
		{
			CString::StringCopy(sTmp, "/cygdrive");
			GetProgramFileName(sTmp+9);
			pFileSystem->GetAfcPathName(sTmp+9);
		}
		else
			GetProgramFileName(sTmp);
#else
		GetProgramFileName(sTmp);
#endif
	}
	else
		CString::StringCatenate(sTmp, sPath);
	pFileSystem->GetFullPath(sTmp, m_sPath, &oDetail);
	CString::StringCopy(m_sDir, m_sPath);
	if(oDetail.sFilePart)
	{
		CString::StringCopy(m_sName, oDetail.sFilePart);
#if defined(WINDOWS) || defined(CYGWIN_NT)
		uint32 i=CString::StringLength(m_sName);
		if(i>4 && !CString::StringCompare(m_sName+i-4, ".exe", false))
			m_sName[i-4] = '\0';
#endif
		m_sDir[oDetail.sFilePart - m_sPath - 1] = '\0';
	}
	CString::StringCopy(sTmp, m_sDir);
	CString::StringCatenate(sTmp, "/../");
	pFileSystem->GetFullPath(sTmp, m_sHome, &oDetail);
}

CFilePathInfo* CFilePathInfo::GetInstance()
{
	return CSingleInstance<CFilePathInfo>::GetInstance();
}

const char* CFilePathInfo::GetPath()//file full name
{
	return m_sPath;
}

const char* CFilePathInfo::GetName()//file name without the suffix
{
	return m_sName;
}

const char* CFilePathInfo::GetDir()//file's directory
{
	return m_sDir;
}

const char* CFilePathInfo::GetHome()//the file directory's parent directory, maybe same with GetDir();
{
	return m_sHome;
}


FOCP_END();

#ifdef WINDOWS
#include <windows.h>
#endif

FOCP_BEGIN();
static void ToughFileTime(const char* sOldFile, const char* sNewFile)
{
#ifdef WINDOWS
	uint32 dwDesiredAccess = 0;
	uint32 dwShareMode = 0;
	uint32 dwCreationDisposition = 0;
	uint32 dwFlagsAndAttributes = 0;
	FILETIME aTime, cTime, mTime;
	HANDLE hOld, hNew;

	dwDesiredAccess = 0;
	dwShareMode = 0;
	dwCreationDisposition = 0;
	dwFlagsAndAttributes = 0;
	dwDesiredAccess |= GENERIC_READ;
	dwShareMode |= FILE_SHARE_READ;
	dwCreationDisposition = OPEN_EXISTING;
	dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
	dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;

	hOld = CreateFile(sOldFile, dwDesiredAccess, dwShareMode, NULL,
					  dwCreationDisposition, dwFlagsAndAttributes, NULL);

	if(INVALID_HANDLE_VALUE == hOld)
		return;

	dwDesiredAccess = 0;
	dwShareMode = 0;
	dwCreationDisposition = 0;
	dwFlagsAndAttributes = 0;
	dwDesiredAccess |= GENERIC_WRITE;
	dwShareMode |= FILE_SHARE_READ;
	dwCreationDisposition = OPEN_EXISTING;
	dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
	dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

	hNew = CreateFile(sNewFile, dwDesiredAccess, dwShareMode, NULL,
					  dwCreationDisposition, dwFlagsAndAttributes, NULL);

	if(INVALID_HANDLE_VALUE == hNew)
	{
		CloseHandle(hOld);
		return;
	}
	if(GetFileTime(hOld, &cTime, &aTime, &mTime))
		SetFileTime(hNew, &cTime, &aTime, &mTime);

	CloseHandle(hOld);
	CloseHandle(hNew);

#else
	CString oCmd("touch -r ");
	oCmd += sOldFile;
	oCmd += " ";
	oCmd += sNewFile;
	system(oCmd.GetStr());
#endif
}

FOCP_END();
