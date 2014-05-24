
#include "AfsDef.hpp"

#ifndef _Afs_FileSystem_Hpp_
#define _Afs_FileSystem_Hpp_

FOCP_BEGIN();

//ͳһ����Windows��Unix������������ϵͳ�е��ļ�ϵͳ��
//	��1����·���ָ�������б�ߣ����Ƿ�б�ߡ����ݷ�б�ߡ���ȥ��Windows�е�������ð�š�	=> Unix���
//				 ���磬C:\windows\system32\explore.exe => /C/windows\system32\explore.exe
//	��2����·���������ִ�Сд����������Сд����ʾ���	=> Windows���
//	��3����֧��·������Я���հס�ͷβ�հ��Զ��������=> Windows���
//	��4����·�����в��ܰ�����б�ߡ���б�ߡ�ð�š��Ǻš��ʺš����ߡ����ںš�С�ںš�˫���š��ֺ�
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

	//���sAfcPath��ȫ·����ע����Ҫ��֤pBuf�㹻����
	//	�����ɹ����ͷ����棬���򷵻ؼ�
	//���pDetailInfoΪ�գ�������sAfcPath����Ĵʷ��Ϸ��ԡ�
	//���pDetailInfo�ǿգ������GetOsPathName���·������ʵ�����ԡ�
	//	������ڣ���Ŀ¼�Ļ���sFilePart����NULL������Ŀ¼�Ļ�������������ļ�����
	//	��������ڣ�bExist����false; sFilePart���ز����ڵĲ���.
	////////////////////////////////////////
	//ȫ·����ʽ:
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

	//����ϵͳ�趨����ʱĿ¼
	void GetTmpPath(char* sAfcPath);

	//��sAfcFullPath��ΪOs·�������سɹ���ʧ��
	//sAfcFullPath�����Ǹ�ʽ��ȷ��AFC·����������GetFullPathName���ص�һ�¡�������������Ԥ��
	//	���pDetailInfo�ǿգ�������·������ʵ������
	//	������ڣ���Ŀ¼�Ļ���sFilePart����NULL������Ŀ¼�Ļ�������������ļ���
	//	��������ڣ�bExist����false; sFilePart���ز����ڵĲ���
	bool GetOsPathName(char* sAfcFullPath, CPathDetailInfo* pDetailInfo=NULL);

	//OsPath to AFC Path
	char* GetAfcPathName(char* sOsFullPath);

	bool CreateDirectory(const char* sDirectory);

	//ɾ���ļ���Ŀ¼�����ݹ�ʱ��ɾ��Ŀ¼���������ļ���Ŀ¼
	bool RemovePath(const char* sPathName, bool bRecursive=false);

	//�޸��ļ�����Ŀ¼����
	//	�����·���������򷵻�ʧ�ܡ�
	//	�����·�������ڣ���ֱ���޸ġ�
	//	�����·�����ڣ��¾�·��Ҫô����Ŀ¼��Ҫô�����ļ���
	//		���·��Ϊ�ļ����򸲸ǡ�
	//		���·��ΪĿ¼�������Ҫ����Ŀ¼Ϊ�ա�
	bool RenamePath(const char* sOldPathName, const char* sNewPathName);

	//�����ļ���Ŀ¼��
	//	�����·���������򷵻�ʧ�ܡ�
	bool CopyPath(const char* sOldPathName, const char* sNewPathName);

	bool AccessPath(const char* sPathName, uint32 nMode, bool &bDirectory);

	//�����ļ�sFileName��������·�����������ļ����洢��pBuf�С�
	//����ļ����в�����ǰ׺���׺���������Զ�������������
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
