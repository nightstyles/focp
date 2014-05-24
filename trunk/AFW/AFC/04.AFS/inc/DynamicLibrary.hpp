
#include "AfsDef.hpp"

#ifndef _Afs_DynamicLibrary_Hpp_
#define _Afs_DynamicLibrary_Hpp_

FOCP_BEGIN();

class AFS_API CDynamicLibrary
{
	FOCP_FORBID_COPY(CDynamicLibrary);
private:
	void* m_pLib;

public:
	CDynamicLibrary();
	CDynamicLibrary(const char* sLibName, bool bTry=false);
	~CDynamicLibrary();

	//系统支持空文件名情况【将返回主程序句柄】，
	//如果sLibName含有路径分隔符[AFC路径格式，请参考FileSystem.hpp]，则直接打开。
	//否则，使用环境变量FOCP_LIBRARY_PATH搜索文件【先搜当前目录】，此时可以不带文件的后缀(.dll/.so)、或前缀(lib)。
	//	LD_LIBRARY_PATH也需要按照AFC路径格式定义。
	bool Load(const char* sLibName, bool bTry=false);
	void UnLoad();

	void* FindSymbol(const char* sSymbolName);

	bool Valid();

	const char* GetFileName();
};

class AFS_API CMainModule
{
	friend struct CSingleInstanceEx<CMainModule>;
private:
	CDynamicLibrary m_oMainLib;
	CDynamicLibrary m_oAppLib;

	CMainModule();

public:
	~CMainModule();

	static CMainModule* GetInstance();

	void* FindSymbol(const char* sSymbolName);

	void UnLoad();

	void InitializeInstance();

	bool Valid();
};

class AFS_API CAutoLibrary
{
private:
	CVector< CDynamicLibrary* > m_oLibrary;

public:
	CAutoLibrary(const char* sLibDir);
	~CAutoLibrary();

	uint32 GetSize();
	CDynamicLibrary* GetLibrary(uint32 nIdx);
};

class AFS_API CInterfaceManager;

class AFS_API CInterface
{
	friend class CInterfaceManager;
private:
	CInterfaceManager* m_pManager;

public:
	CInterface(CInterfaceManager* pManager);
	virtual ~CInterface();

protected:
	virtual const char* GetInterfaceName();
};

class AFS_API CInterfaceManager
{
	friend class CInterface;
private:
	CMutex m_oMutex;
	CVector<CInterface*> m_oInterfaces;
	CAutoLibrary* m_pAutoLibrary;
	const char* m_sLibDir;

public:
	CInterfaceManager(const char* sLibDir);
	virtual ~CInterfaceManager();

	CInterface* QueryInterface(const char* sInterfaceName);

	uint32 GetSize();
	CInterface* GetInterface(uint32 nIdx);

	void Load();
	void UnLoad();

private:
	void RegisterInterface(CInterface* pInterface);
	void DeRegisterInterface(CInterface* pInterface);
};

FOCP_END();

#endif //_Afs_DynamicLibrary_Hpp_
