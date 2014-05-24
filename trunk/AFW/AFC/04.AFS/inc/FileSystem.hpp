
#include "AfsDef.hpp"

#ifndef _Afs_FileSystem_Hpp_
#define _Afs_FileSystem_Hpp_

FOCP_BEGIN();

//统一定义Windows和Unix两大主流操作系统中的文件系统。
//	【1】：路径分隔符采用斜线，而非反斜线【兼容反斜线】；去除Windows中的驱动器冒号。	=> Unix风格
//				 例如，C:\windows\system32\explore.exe => /C/windows\system32\explore.exe
//	【2】：路径名不区分大小写，但保留大小写的显示风格。	=> Windows风格
//	【3】：支持路径名中携带空白【头尾空白自动清除】。=> Windows风格
//	【4】：路径名中不能包含：斜线、反斜线、冒号、星号、问号、竖线、大于号、小于号、双引号、分号
enum
{
	FOCP_FILEACCESS_EXIST=0,
	FOCP_FILEACCESS_READABLE=1,
	FOCP_FILEACCESS_WRITABLE=2,
	FOCP_FILEACCESS_EXECUTABLE=4,
};

struct AFS_API CPathDetailInfo
{
	char* sFilePart;
	bool bExist;
};

class CLogManager;
class AFS_API CDiskFileSystem
{
	FOCP_FORBID_COPY(CDiskFileSystem);
	friend class CFilePathInfo;
	friend class CLogManager;
	friend struct CSingleInstance<CDiskFileSystem>;
private:
	CMutex m_oMutex;
	char m_sCurrentDirectory[FOCP_MAX_PATH];

	CDiskFileSystem();

public:
	~CDiskFileSystem();

	static CDiskFileSystem* GetInstance();

	static bool CheckFileName(const char* sName);

	char GetOsPathSeparator();

	const char* GetCurrentDirectory();
	bool SetCurrentDirectory(const char* sDirectory);

	//获得sAfcPath的全路径，注意需要保证pBuf足够长。
	//	解析成功，就返回真，否则返回假
	//如果pDetailInfo为空，将会检查sAfcPath自身的词法合法性。
	//如果pDetailInfo非空，会调用GetOsPathName检查路径的真实存在性。
	//	如果存在：是目录的话，sFilePart返回NULL，不是目录的话，将返回最短文件名。
	//	如果不存在：bExist返回false; sFilePart返回不存在的部分.
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
	bool GetFullPath(const char* sAfcPath, char* pBuf, CPathDetailInfo* pDetailInfo=NULL);

	//返回系统设定的临时目录
	void GetTmpPath(char* sAfcPath);

	//换sAfcFullPath算为Os路径，返回成功与失败
	//sAfcFullPath必须是格式正确的AFC路径【必须与GetFullPathName返回的一致】，否则结果不可预测
	//	如果pDetailInfo非空，还会检查路径的真实存在性
	//	如果存在：是目录的话，sFilePart返回NULL，不是目录的话，将返回最短文件名
	//	如果不存在：bExist返回false; sFilePart返回不存在的部分
	bool GetOsPathName(char* sAfcFullPath, CPathDetailInfo* pDetailInfo=NULL);

	//OsPath to AFC Path
	char* GetAfcPathName(char* sOsFullPath);

	bool CreateDirectory(const char* sDirectory);

	//删除文件或目录，当递归时将删除目录下所有子文件及目录
	bool RemovePath(const char* sPathName, bool bRecursive=false);

	//修改文件名或目录名。
	//	如果旧路径不存在则返回失败。
	//	如果新路径不存在，则直接修改。
	//	如果新路径存在，新旧路径要么都是目录，要么都是文件。
	//		如果路径为文件，则覆盖。
	//		如果路径为目录，则必须要求新目录为空。
	bool RenamePath(const char* sOldPathName, const char* sNewPathName);

	//复制文件或目录。
	//	如果旧路径不存在则返回失败。
	bool CopyPath(const char* sOldPathName, const char* sNewPathName);

	bool AccessPath(const char* sPathName, uint32 nMode, bool &bDirectory);

	//搜索文件sFileName【不包含路径】，并把文件名存储在pBuf中。
	//如果文件名中不包含前缀或后缀名，可以自动加上在搜索。
	bool SearchFile(const char* sEnvVariable, const char* sFileName, char* pBuf, const char* sPrefixName=NULL, const char* sPostfixName=NULL);

	bool SearchFileEx(const char* sSearchPathList, const char* sFileName, char* pBuf, char cSeparator, const char* sPrefixName=NULL, const char* sPostfixName=NULL);

	void* OpenDirectory(const char* sDirectory);
	void CloseDirectory(void* pDirectory);
	const char* ReadDirectory(void* pDirectory, bool &bIsDirectory);
	void RewindDirectory(void* pDirectory);

private:
	bool GetPathSection(const char* &sAfcPath, const char* sEnd, CString &oName, uint32 &nType);
	bool RecurseRemoveDirectory(char* sPath, char* sFile);
	bool CopyFile(const char* sOldFile, const char* sNewFile);
	bool CopyDirectory(char* sOldDirectory, char* sNewDirectory, char* sBuf1, char* sBuf2, bool bExist, bool bForce=false);
	bool MoveDirectory(char* sOldDirectory, char* sNewDirectory, bool bExist);
	bool GetOsPathNameEx(void* pDir, char* sDirectory, char* sFileName, CPathDetailInfo* pDetailInfo);
};

class AFS_API CFilePathInfo
{
private:
	char m_sPath[FOCP_MAX_PATH];
	char m_sName[FOCP_MAX_PATH];
	char m_sHome[FOCP_MAX_PATH];
	char m_sDir[FOCP_MAX_PATH];

public:
	CFilePathInfo(const char* sPath=NULL);

	static CFilePathInfo* GetInstance();

	const char* GetPath();//file full name
	const char* GetName();//file name without the suffix
	const char* GetDir();//file's directory
	const char* GetHome();//the file directory's parent directory, maybe same with GetDir();
};

FOCP_END();

#endif //_Afs_FileSystem_Hpp_
