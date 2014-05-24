
#include "EvmModule.hpp"

FOCP_BEGIN();

CEvmModuleHandle::CEvmModuleHandle(CEvmModule* pOwner, CEvmModule* pModule)
{
	m_pOwner = pOwner;
	m_pModule = pModule;
}

CEvmModuleHandle::~CEvmModuleHandle()
{
	if(m_pOwner == NULL || m_pModule == NULL)
		return;
	CEvmProcessManager* pM = CEvmProcessManager::GetInstance();
	pM->m_oMutex.Enter();
	m_pOwner->m_pProcess->m_pModules->RemovePointer(m_pOwner, m_pModule);
	pM->m_oMutex.Leave();
}

CEvmModuleHandle* CEvmModuleHandle::QueryCurrentModule(ehc_void* reg)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmModule* pModule = (CEvmModule*)pReg->DS - 1;
	ehc_void* pRet = pModule->m_pProcess->Malloc(sizeof(CEvmModuleHandle));
	return new(pRet) CEvmModuleHandle(NULL, pModule);
}

ehc_char* GetEhcVmDataAddr(ehc_void* reg, ehc_uint nArg)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmModule* pModule = (CEvmModule*)pReg->DS - 1;
	return pModule->m_pSymbolAddrTable[nArg].pAddr;
}

ehc_char* GetEhcVmProcAddr(ehc_void* reg, ehc_uint nArg, CEhcVmSegment* pSegment)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmModule* pModule = (CEvmModule*)pReg->DS - 1;
	CEvmSymbolAddr* pSym = pModule->m_pSymbolAddrTable + nArg;
	ehc_char* pRet = pSym->pAddr;
	pModule = pSym->pModule;
	if(pModule == NULL)
		pSegment->CS = NULL;
	else
	{
		CEvmFile* pFile = pModule->m_pFile;
		pSegment->IS = pFile->m_pConstSegment;
		pSegment->DS = (ehc_char*)(pModule+1);
		pSegment->CS = pFile->m_pCodeSegment;
		pSegment->SS = (ehc_char*)pFile->m_pSymbolTable;
	}
}

void* CEvmModule::operator new(size_t nSize, size_t nDataSize)
{
	size_t nTotalSize = nSize + nDataSize;
	uint8* pRet = CMalloc::Malloc(nTotalSize);
	if(pRet == NULL)
		throw std::bad_alloc();
	return pRet;
}

void CEvmModule::operator delete(void* p)
{
	if(p)
		CMalloc::Free(p);
}

CEvmModule::CEvmModule(CEvmFile* pFile, CEvmProcess* pProcess)
{
	m_pFile = pFile;
	m_pProcess = pProcess;
	m_pSymbolAddrTable = NULL;
}

CEvmModule::~CEvmModule()
{
	CEvmModule* pChild;
	while(m_oChildren.Pop(pChild))
		m_pProcess->m_pModules->RemovePointer(this, pChild);
	CEvmProcessManager::GetInstance()->m_oFileTable.UnLoad(m_pFile);
	if(m_pSymbolAddrTable)
	{
		delete[] m_pSymbolAddrTable;
		m_pSymbolAddrTable = NULL;
	}
	m_pFile = NULL;
	m_pProcess = NULL;
}

void CEvmModule::Push(CEvmModule* pChild)
{
	m_oChildren.Push(pChild);
}

const CString& CEvmModule::GetFileName() const
{
	return m_pFile->GetFileName();
}

CEvmModuleHandle* CEvmModule::LoadModule(ehc_void* reg, const char* sFileName)
{
	char sFullName[FOCP_MAX_PATH];
	CEvmFileManager::GetFullName(sFileName, sFullName);
	if(sFullName[0] == '\0')
		return NULL;

	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmModule* pModule = (CEvmModule*)pReg->DS - 1;

	CEvmModule* pSubModule = pModule->m_pProcess->LoadModule(pModule, sFullName, true);
	if(pSubModule == NULL)
		return NULL;
	
	ehc_void* pRet = pModule->m_pProcess->Malloc(sizeof(CEvmModuleHandle));
	return new(pRet) CEvmModuleHandle(pModule, pSubModule);
}

void CEvmModule::FreeModule(CEvmModuleHandle* pHandle)
{
	if(pHandle)
	{
		if(pHandle->m_pModule)
		{
			CEvmProcess* pProcess = pHandle->m_pModule->m_pProcess;
			pHandle->~CEvmModuleHandle();
			pProcess->Free(pHandle);
		}
		else
			delete pHandle;
	}
}

const CString* CEvmProcess::CGetModuleName::GetKey(const CEvmModule* pModule)
{
	return &pModule->GetFileName();
}

CEvmProcess::CEvmProcess():
	m_oWaitorList(FocpFieldOffset(CEvmWaitor, m_pPrev), FocpFieldOffset(CEvmWaitor, m_pNext))
{
	m_pModules = NULL;
	m_pMain = NULL;
}

CEvmProcess::~CEvmProcess()
{
	if(m_pModules)
	{
		if(m_pMain)
		{
			delete m_pMain;
			m_pMain = NULL;
		}
		CEvmProcessManager::GetInstance()->m_oProcessTable.Remove(m_nProcId);
		delete m_pModules;
		m_pModules = NULL;
	}
}

const CString& CEvmProcess::GetFileName() const
{
	return m_pMain->GetFileName();
}

CEvmProcess* CEvmProcess::GetCurrentProcess(ehc_void* reg)
{
	CEvmRegister* pReg = (CEvmRegister*)reg;
	CEvmModule* pModule = (CEvmModule*)pReg->DS - 1;
	return pModule->m_pProcess;
}

int32 CEvmProcess::GetArgc()
{
	return m_oArgs.GetArgc();
}

char* const* CEvmProcess::GetArgv()
{
	return m_oArgs.GetArgv();
}

const char* CEvmProcess::GetCmdLine()
{
	return m_oArgs.GetCmdLine();
}

ehc_void CEvmProcess::Terminate(ehc_int nExit)
{
	CEvmWaitor* pWait;
	if(m_oWaitorList.GetSize())
	{
		CEvmProcessManager* pM = CEvmProcessManager::GetInstance();
		pM->m_oMutex.Enter();
		while((pWait=m_oWaitorList.Pop()))
			pWait->OnExit(nExit);
		CEvmProcess* pProc = this;
		delete pProc;
		pM->m_oMutex.Leave();
	}
}

bool CEvmProcess::Load(const char* sFileName)
{
	m_pMain = LoadModule(NULL, sFileName, false);
	return (m_pMain != NULL);
}

void CEvmProcess::Execute(CEvmCpu* pCpu)
{
	CEvmFile* pFile = m_pMain->m_pFile;
	CEvmRegister* pReg = (CEvmRegister*)CMalloc::Malloc(sizeof(CEvmRegister));
	pReg->CP = NULL;
	pReg->IS = pFile->m_pConstSegment;
	pReg->DS = (ehc_char*)(ehc_void*)(m_pMain+1);
	pReg->CS = pFile->m_pCodeSegment;
	pReg->LS = NULL;
	pReg->AS = NULL;
	pReg->TS = NULL;
	pReg->SS = (ehc_char*)(ehc_void*)pFile->m_pSymbolTable;
	pReg->PC = pFile->m_pEntry;
	pCpu->Resume(pReg);
}

ehc_uint CEvmProcess::GetPid()
{
	return m_nProcId;
}

void* CEvmProcess::Malloc(uint32 nSize)
{
	return m_oMemoryManager.AllocateBuffer(nSize);
}

void CEvmProcess::Free(void* p)
{
	if(p)
		m_oMemoryManager.DeAllocateBuffer(p);
}

CEvmModule* CEvmProcess::LoadModule(CEvmModule* pParent, const char* sFileName, bool bLock)
{
	CEvmProcessManager* pPm = CEvmProcessManager::GetInstance();
	if(bLock)
		pPm->m_oMutex.Enter();

	CRbTreeNode* pIt = m_oModuleTable.Find(sFileName);
	if(pIt != m_oModuleTable.End())
	{
		CEvmModule *pModule = m_oModuleTable.GetItem(pIt);
		if(pParent)
		{
			m_pModules->InsertPointer(pParent, pModule);
			pParent->Push(pModule);
		}
		else
			m_pModules = new CSmartPointerManager<CEvmModule>(pModule);
		if(bLock)
			pPm->m_oMutex.Leave();
		return pModule;
	}

	CEvmFile* pFile = pPm->m_oFileTable.Load(sFileName);
	if(pFile == NULL)
	{
		if(bLock)
			pPm->m_oMutex.Leave();
		return NULL;
	}

	CRbMap<CString, CEvmFile*> oFiles, oFiles2;
	if(pFile->m_bLinked)
		QueryLink(pPm, pFile, oFiles, pFile);
	else if(CreateLink(pPm, pFile, oFiles, oFiles2, pFile))
	{
		CRbTreeNode* pEnd2 = oFiles.End();
		CRbTreeNode* pEnd = oFiles2.End();
		CRbTreeNode* pIt = oFiles2.First();
		for(; pIt!= pEnd; pIt=oFiles2.GetNext(pIt))
		{
			CEvmFile* pFile2 = oFiles2.GetItem(pIt);
			if( (pFile2 == pFile) || (pEnd!=oFiles.Find(pFile2->GetFileName())))
				pPm->m_oFileTable.UnLoad(pFile2);
			else
			{
				oFiles[pFile2->GetFileName()] = pFile2;
				QueryLink(pPm, pFile2, oFiles, pFile);
			}
		}
	}
	else
	{
		CRbTreeNode* pEnd = oFiles.End();
		CRbTreeNode* pIt = oFiles.First();
		for(; pIt!= pEnd; pIt=oFiles.GetNext(pIt))
			pPm->m_oFileTable.UnLoad(oFiles.GetItem(pIt));
		pEnd = oFiles2.End();
		pIt = oFiles2.First();
		for(; pIt!= pEnd; pIt=oFiles2.GetNext(pIt))
			pPm->m_oFileTable.UnLoad(oFiles2.GetItem(pIt));
		pPm->m_oFileTable.UnLoad(pFile);
		if(bLock)
			pPm->m_oMutex.Leave();
		return NULL;
	}

	CEvmModule* pModule = new(pFile->m_nDataSize) CEvmModule(pFile, this);
	if(pParent)
	{
		m_pModules->InsertPointer(pParent, pModule);
		pParent->Push(pModule);
	}
	else
		m_pModules = new CSmartPointerManager<CEvmModule>(pModule);
	
	CRbTreeNode* pEnd = oFiles.End();
	CRbTreeNode* pIt = oFiles.First();
	for(; pIt!= pEnd; pIt=oFiles.GetNext(pIt))
	{
		CEvmFile* pFile2 = oFiles.GetItem(pIt);
		CEvmModule* pModule2 = new(pFile2->m_nDataSize) CEvmModule(pFile2, this);
		m_pModules->InsertPointer(pModule, pModule2);
		pModule->Push(pModule2);
	}

	if(bLock)
		pPm->m_oMutex.Leave();

	return pModule;
}

void CEvmProcess::QueryLink(CEvmProcessManager* pPm, CEvmFile* pFile, CRbMap<CString, CEvmFile*>& oFiles, CEvmFile* pRoot)
{
	if(pFile->m_nLibCount)
	{
		ehc_uint i;
		CEvmFile** pFiles = new CEvmFile*[pFile->m_nLibCount];
		for(i=0; i<pFile->m_nLibCount; ++i)
		{
			CEvmFile* pFile2 = pPm->m_oFileTable.Load(pFile->m_pLibTable[i]);
			if(oFiles.Find(pFile2->GetFileName()) != oFiles.End())
			{
				pPm->m_oFileTable.UnLoad(pFile2);
				pFiles[i] = NULL;
			}
			else
			{
				oFiles[pFile2->GetFileName()] = pFile2;
				pFiles[i] = pFile2;
			}
		}
		for(i=0; i<pFile->m_nLibCount; ++i)
		{
			CEvmFile* pFile2 = pFiles[i];
			if(pFile2 && pFile2!=pRoot)
				QueryLink(pPm, pFile2, oFiles);
		}
		delete[] pFiles;
	}
}

bool CEvmProcess::CreateLink(CEvmProcessManager* pPm, CEvmFile* pFile, CRbMap<CString, CEvmFile*>& oFiles, CRbMap<CString, CEvmFile*>& oFiles2, CEvmFile* pRoot)
{
	if(pFile->m_nLibCount)
	{
		ehc_uint i;
		CEvmFile** pFiles = new CEvmFile*[pFile->m_nLibCount];
		for(i=0; i<pFile->m_nLibCount; ++i)
			pFiles[i] = NULL;
		for(i=0; i<pFile->m_nLibCount; ++i)
		{
			char sFullName[FOCP_MAX_PATH];
			CEvmFileManager::GetFullName(pFile->m_pLibTable[i], sFullName);
			if(sFullName[0] == '\0')
				break;
			pFiles[i] = pPm->m_oFileTable.Load(sFullName);
			if(pFiles[i] == NULL)
				break;
			pFile->m_pLibTable[i] = (ehc_char*)pFiles[i]->GetFileName().GetStr();
		}
		if(i<pFile->m_nLibCount)
		{
			for(ehc_uint j=0; j<i; ++j)
				pPm->m_oFileTable.UnLoad(pFiles[j]);
			delete[] pFiles;
			return false;
		}
		for(i=0; i<pFile->m_nLibCount; ++i)
			pFile->Link(pFiles[i]);
		if(!pFile->CheckLink())
		{
			for(i=0; i<pFile->m_nLibCount; ++i)
				pPm->m_oFileTable.UnLoad(pFiles[i]);
			delete[] pFiles;
			return false;
		}
		CRbTreeNode* pEnd = oFiles2.End();
		for(i=0; i<pFile->m_nLibCount; ++i)
		{
			CEvmFile* pFile2 = pFiles[i];
			if(pFile2->m_bLinked)
			{
				if(pFile2 == pRoot || (pEnd != oFiles2.Find(pFile2->GetFileName())))
					pPm->m_oFileTable.UnLoad(pFile2);
				else
					oFiles2[pFile2->GetFileName()] = pFile2;
				pFiles[i] = NULL;
			}
			else if(!CreateLink(pPm, pFile2, oFiles, oFiles2, pRoot))
				break;
		}
		if(i<pFile->m_nLibCount)
		{
			for(ehc_uint j=0; j<i; ++j)
			{
				if(pFiles[j])
					pPm->m_oFileTable.UnLoad(pFiles[j]);
			}
			delete[] pFiles;
			return false;
		}
		pEnd = oFiles.End();
		for(i=0; i<pFile->m_nLibCount; ++i)
		{
			CEvmFile* pFile2 = pFiles[i];
			if(pFile2)
			{
				if(pEnd != oFiles.Find(pFile2->GetFileName()))
					pPm->m_oFileTable.UnLoad(pFile2);
				else
					oFiles[pFile2->GetFileName()] = pFile2;
			}
		}
		delete[] pFiles;
	}
	else if(!pFile->CheckLink())
		return false;
	return true;
}

CEvmProcessManager::CEvmProcessManager()
{
	m_nProcId = 0;
}

CEvmProcessManager::~CEvmProcessManager()
{
}

CEvmProcessManager* CEvmProcessManager::GetInstance()
{
	return CSingleInstance<CEvmProcessManager>::GetInstance();
}

ehc_uint CEvmProcessManager::AllocProcessId()
{
	CRbTreeNode* pEnd = m_oProcessTable.End();
	do{
		++m_nProcId;
		if(m_nProcId == 0)
			++m_nProcId;
	}while(m_oProcessTable.Find(m_nProcId)!=pEnd());
	return m_nProcId;
}

ehc_uint CEvmProcessManager::Execute(const char* sCmdLine)
{
	char sFullName[FOCP_MAX_PATH];
	CEvmProcess* pProcess = new CEvmProcess;
	pProcess->m_oArgs.SetCmdLine(sCmdLine);
	char* const* sArgv = pProcess->m_oArgs.GetArgv();
	CEvmFileManager::GetFullName(sArgv[0], sFullName);
	if(sFullName[0] == '\0')
	{
		delete pProcess;
		return 0;
	}
	m_oMutex.Enter();
	if(!pProcess->Load(sFullName))
	{
		m_oMutex.Leave();
		delete pProcess;
		return 0;
	}
	sArgv[0] = pProcess->GetFileName().GetStr();
	ehc_uint nProcId = AllocProcessId();
	pProcess->m_nProcId = nProcId
	m_oProcessTable[nProcId] = pProcess;
	m_oMutex.Leave();
	pProcess->Execute(CEvmCpu::GetInstance());
	return nProcId;
}

FOCP_END();
