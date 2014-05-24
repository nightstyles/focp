
#include "EvmFile.hpp"

#ifndef _EVM_MODULE_HPP_
#define _EVM_MODULE_HPP_

FOCP_BEGIN();

class EVM_API CEvmModuleHandle
{
	friend class CEvmModule;
private:
	CEvmModule* m_pOwner;
	CEvmModule* m_pModule;

	CEvmModuleHandle(CEvmModule* pOwner, CEvmModule* pModule);
	~CEvmModuleHandle();
public:

	static CEvmModuleHandle* QueryCurrentModule(ehc_void* reg);
};

struct CEvmSymbolAddr
{
	ehc_char* pAddr;
	CEvmModule* pModule;
};

class EVM_API CEvmModule
{
	friend class CEvmProcess;
	friend class CEvmProcessManager;
	friend class CEvmModuleHandle;
	friend ehc_char* GetEhcVmProcAddr(ehc_void* reg, ehc_uint nArg, CEhcVmSegment* pSegment);
private:
	CEvmProcess* m_pProcess;
	CEvmFile* m_pFile;
	CSingleList<CEvmModule*> m_oChildren;
	CEvmSymbolAddr* m_pSymbolAddrTable;

	void* operator new(size_t nSize, size_t nDataSize);
	void operator delete(void* p);

	CEvmModule(CEvmFile* pFile, CEvmProcess* pProcess);
	void Push(CEvmModule* pChild);

public:
	~CEvmModule();

	const CString& GetFileName() const;

	static CEvmModuleHandle* LoadModule(ehc_void* reg, const char* sFileName);
	static void FreeModule(CEvmModuleHandle* pHandle);
};

class EVM_API CEvmProcess
{
	friend class CEvmModule;
	friend class CEvmProcessManager;
	friend class CEvmModuleHandle;
private:
	struct CGetModuleName
	{
		static const CString* GetKey(const CEvmModule* pModule);
	};
	CArguments m_oArgs;
	CBufferManager m_oMemoryManager;
	CBaseDoubleList<CEvmWaitor> m_oWaitorList;
	CRbTree<CString, CEvmModule*, CGetModuleName> m_oModuleTable;
	CSmartPointerManager<CEvmModule>* m_pModules;
	CEvmModule* m_pMain;
	ehc_uint m_nProcId;

	CEvmProcess();
	~CEvmProcess();
public:
	static CEvmProcess* GetCurrentProcess(ehc_void* reg);

	int32 GetArgc();
	char* const* GetArgv();
	const char* GetCmdLine();

	ehc_void Terminate(ehc_int nExit);

	ehc_uint GetPid();

	void* Malloc(uint32 nSize);
	void Free(void* p);

	const CString& GetFileName() const;

private:
	bool Load(const char* sFileName);
	CEvmModule* LoadModule(CEvmModule* pParent, const char* sFileName, bool bLock);
	bool CreateLink(CEvmProcessManager* pPm, CEvmFile* pFile, CRbMap<CString, CEvmFile*>& oFiles, CRbMap<CString, CEvmFile*>& oFiles2, CEvmFile* pRoot);
	void QueryLink(CEvmProcessManager* pPm, CEvmFile* pFile, CRbMap<CString, CEvmFile*>& oFiles, CEvmFile* pRoot);
	void Execute(CEvmCpu* pCpu);
};

class EVM_API CEvmProcessManager
{
	friend class CEvmProcess;
	friend class CEvmModule;
private:
	CMutex m_oMutex;
	CEvmFileManager m_oFileTable;
	CRbMap<ehc_uint, CEvmProcess*> m_oProcessTable;
	ehc_uint m_nProcId;

	ehc_uint AllocProcessId();

public:
	CEvmProcessManager();
	~CEvmProcessManager();

	static CEvmProcessManager* GetInstance();

	ehc_uint Execute(const char* sCmdLine);
	ehc_uint Wait(ehc_uint nPid, ehc_int *pExit, ehc_uint nOption);
};

FOCP_END();

#endif
