
#include "jcvm_mm.h"

//global variable define
CJcFile* g_pJcFile = NULL;
CJcHostFile* g_pHostFile = NULL;
CJcProgram* g_pJcProgram = NULL;
jc_uint g_nError = 0;
extern CJcVmInterface g_oInterface;

//static other function define
static jc_char* FindFile(const jc_char* sFileName, jc_int bJcFile);

//static JcFile function define
static CJcFile* LoadJcVmFile(CJcFile* pRoot, const jc_char* sFileName);
static void UnLoadJcVmFile(CJcFile* pFile);
static CJcFile* QueryJcVmFile(CJcFile* pRoot, const jc_char* sFileName, jc_uint bLoadLink);
static jc_uint LoadJcVmChildFile(CJcFile* pRoot, CJcFile* pParent);
static jc_void UnLoadJcVmChildFile(CJcFile* pRoot, CJcFileItem* pEndFileItem);
static jc_uint AddChildFileItem(CJcFile* pRoot, CJcFile* pChild);
static CJcFile* LoadOneJcVmFile(const jc_char* sFileName);
static jc_uint DynamicJcVmFile(CJcFile* pFile);

//static host function define
static jc_void UnLoadJcHostFile(CJcHostFile* pHostFile);
static jc_void UnLoadAllJcHostFile(CJcFile* pFile);

//static JcModule function define
static CJcModule* QueryJcVmModule(const jc_char* sFileName, CJcProgram* pProgram);
static CJcModule* CreateJcVmModule(CJcProgram* pProgram, CJcFile* pFile);
static jc_uint LoadJcVmChildModule(CJcModule* pRoot, CJcModule* pParent);
static jc_void UnLoadJcVmChildModule(CJcModule* pRoot, CJcModuleItem* pEndModuleItem);

/////////////////////////////////////
//FindFile
/////////////////////////////////////
static jc_char* FindFile(const jc_char* sFileName, jc_int bJcFile)
{
	jc_int nMaxPath;
	jc_char* sFullPath;
	nMaxPath = g_oInterface.GetMaxPath();
	sFullPath = (jc_char*)g_oInterface.Malloc(nMaxPath);
	if(sFullPath == NULL)
	{
		g_nError = JCVM_MME_LACK_MEMORY;
		return NULL;
	}
	if(g_oInterface.FindFile(sFullPath, sFileName, 1))
	{
		g_nError = JCVM_MME_FIND_FAILURE;
		g_oInterface.Free(sFullPath);
		return NULL;
	}
	return sFullPath;
}

/////////////////////////////////////
//LoadJcVmProgram & UnLoadJcVmProgram
/////////////////////////////////////
CJcProgram* LoadJcVmProgram(const jc_char* sFileName)
{
	CJcProgram* pProgram = (CJcProgram*)g_oInterface.Malloc(sizeof(CJcProgram));
	if(!pProgram)
	{
		g_nError = JCVM_MME_LACK_MEMORY;
		return NULL;
	}

	pProgram->argc = 0;
	pProgram->argv = NULL;
	pProgram->pMainModule = NULL;
	pProgram->pPrev = NULL;
	pProgram->pNext = NULL;

	LoadJcVmModule(sFileName, pProgram);
	if(pProgram->pMainModule == NULL || pProgram->pMainModule->pEntry == NULL)
	{
		UnLoadJcVmProgram(pProgram);
		pProgram = NULL;
	}

	pProgram->pNext = g_pJcProgram;
	g_pJcProgram = pProgram;

	return pProgram;
}

void UnLoadJcVmProgram(CJcProgram* pProgram)
{
	CJcProgram* pPrev = pProgram->pPrev;
	CJcProgram* pNext = pProgram->pNext;
	if(pPrev)
		pPrev->pNext = pNext;
	else if(g_pJcProgram == pProgram)
		g_pJcProgram = pNext;
	if(pNext)
		pNext->pPrev = pPrev;
	if(pProgram->pMainModule)
		UnLoadJcVmModule(pProgram->pMainModule);
	g_oInterface.Free(pProgram);
}

/////////////////////////////////////
//LoadJcVmModule & UnLoadJcVmModule
/////////////////////////////////////
CJcModule* LoadJcVmModule(const jc_char* sFileName, CJcProgram* pProgram)
{
	jc_uint dsize;
	CJcModule * pModule;
	CJcFile* pFile;
	jc_char* sFullPath;

	sFullPath = FindFile(sFileName, 1);
	if(sFullPath == NULL)
		return NULL;

	pModule = QueryJcVmModule(sFullPath, pProgram);
	if(pModule || g_nError==JCVM_MME_LACK_MEMORY)
	{
		g_oInterface.Free(sFullPath);
		return pModule;
	}

	pFile = LoadJcVmFile(NULL, sFullPath);
	g_oInterface.Free(sFullPath);

	if(pFile == NULL)
		return NULL;

	pModule = CreateJcVmModule(pProgram, pFile);
	if(pModule == NULL)
	{
		UnLoadJcVmFile(pFile);
		return NULL;
	}

	if(LoadJcVmChildModule(pModule, pModule))
	{
		UnLoadJcVmModule(pModule);
		return NULL;
	}

	return pModule;
}

void UnLoadJcVmModule(CJcModule* pModule)
{
	CJcProgram* pProgram;
	CJcModule *pPrev, *pNext;

	--pModule->nCounter;
	if(pModule->nCounter)
		return;

	pProgram = pModule->pProgram;
	pPrev = pModule->pPrev;
	pNext = pModule->pNext;
	if(pPrev)
		pPrev->pNext = pNext;
	else
		pProgram->pMainModule = pNext;
	if(pNext)
		pNext->pPrev = pPrev;
	UnLoadJcVmChildModule(pModule, NULL);
	if(pModule->pDataSegment)
		g_oInterface.Free(pModule->pDataSegment);
	UnLoadJcVmFile(pModule->pFile);
	g_oInterface.Free(pModule);
}

/////////////////////////////////////
//static JcModule function define
/////////////////////////////////////
static CJcModule* QueryJcVmModule(const jc_char* sFileName, CJcProgram* pProgram)
{
	CJcModule* pModule = pProgram->pMainModule;
	while(pModule)
	{
		if(!g_oInterface.CompareFile(pModule->pFile->sFileName, sFileName))
		{
			if(LoadJcVmChildModule(pModule, pModule))
			{
				*g_nError = JCVM_MME_LACK_MEMORY;
				return NULL;
			}
			++pModule->nCounter;
			return pModule;
		}
		pModule = pModule->pNext;
	}
	g_nError = JCVM_MME_FIND_FAILURE;
	return NULL;
}

static jc_uint LoadJcVmChildModule(CJcModule* pRoot, CJcModule* pParent)
{
	CJcModuleItem* pNewModuleItem = NULL;
	CJcFileItem* pFileItem = pParent->pFile->pChildren;
	while(pFileItem)
	{
		CJcFileItem* pNextFileItem = pFileItem->pNext;
		CJcFile* pFile = pFileItem->pFile;
		CJcModuleItem* pModuleItem = pRoot->pChildren;
		while(pModuleItem)
		{
			CJcModuleItem* pNextModuleItem = pModuleItem->pNext;
			if(pNextModuleItem->pModule->pFile == pFile)
				break;
			pModuleItem = pNextModuleItem;
		}
		if(pModuleItem == NULL)
		{
			CJcProgram* pProgram = pParent->pProgram;
			CJcModule* pModule = pProgram->pMainModule;
			pModuleItem = (CJcModuleItem*)g_oInterface.Malloc(sizeof(CJcModuleItem));
			if(pModuleItem == NULL)
			{
				if(pNewModuleItem)
					UnLoadJcVmChildModule(pRoot, pNewModuleItem);
				g_nError = JCVM_MME_LACK_MEMORY;
				return 1;
			}
			while(pModule)
			{
				if(pModule->pFile == pFile)
				{
					++pModule->nCounter;
					break;
				}
				pModule = pModule->pNext;
			}
			if(pModule == NULL)
			{
				pModule = CreateJcVmModule(pProgram, pFile);
				if(pModule == NULL)
				{
					if(pNewModuleItem)
						UnLoadJcVmChildModule(pRoot, pNewModuleItem);
					g_oInterface.Free(pModuleItem);
					return 1;
				}
			}
			pModuleItem->pModule = pModule;
			pModuleItem->pNext = pRoot->pChildren;
			pRoot->pChildren = pModuleItem;
			if(pNewModuleItem == NULL)
				pNewModuleItem = pModuleItem;
		}
		pFileItem = pNextFileItem;
	}
	return 0;
}

static CJcModule* CreateJcVmModule(CJcProgram* pProgram, CJcFile* pFile)
{
	jc_uint dsize;
	CJcModule* pModule;	
	
	pModule = (CJcModule*)g_oInterface.Malloc(sizeof(CJcModule));
	if(pModule == NULL)
	{
		g_nError = JCVM_MME_LACK_MEMORY;
		return NULL;
	}

	dsize = pFile->pDataSegment->dsize;
	if(dsize)
	{
		pModule->pDataSegment = (jc_char*)g_oInterface.Malloc(dsize);
		if(pModule->pDataSegment == NULL)
		{
			g_oInterface.Free(pModule);
			g_nError = JCVM_MME_LACK_MEMORY;
			return NULL;
		}
		else
			memcpy(pModule->pDataSegment, pFile->pDataSegment, dsize);
	}
	else
		pModule->pDataSegment = NULL;

	pModule->nCounter = 1;
	pModule->pFile = pFile;
	pModule->pProgram = pProgram;
	pModule->pPrev = pProgram->pMainModule;
	if(pProgram->pMainModule)
	{
		pModule->pNext = pProgram->pMainModule->pNext;
		if(pModule->pNext)
			pModule->pNext->pPrev = pModule;
	}
	else
	{
		pModule->pNext = NULL;
		pProgram->pMainModule = pModule;
	}
	pModule->pChildren = NULL;

	return pModule;
}

static jc_void UnLoadJcVmChildModule(CJcModule* pRoot, CJcModuleItem* pEndModuleItem)
{
	CJcModuleItem* pChild = pRoot->pChildren;
	if(pEndModuleItem)
		pRoot->pChildren = pEndModuleItem->pNext;
	else
		pRoot->pChildren = NULL;
	while(pChild)
	{
		CJcModuleItem* pNext = pChild->pNext;
		UnLoadJcVmModule(pChild->pModule);
		g_oInterface.Free(pChild);
		if(pChild == pEndModuleItem)
			break;
		pChild = pNext;
	}
}

/////////////////////////////////////
//static JcFile function define
/////////////////////////////////////
static CJcFile* LoadJcVmFile(CJcFile* pRoot, const jc_char* sFileName)
{
	CJcFile* pFile = QueryJcVmFile(pRoot, sFileName, 1);
	if(pFile || g_nError==JCVM_MME_LACK_MEMORY)
		return pFile;
	pFile = LoadOneJcVmFile(sFileName);
	if(pFile == NULL)
		return NULL;
	if(pRoot && AddChildFileItem(pRoot, pFile))
	{
		UnLoadJcVmFile(pFile);
		return NULL;
	}
	if(LoadJcVmChildFile(pRoot?pRoot:pFile, pFile) || DynamicJcVmFile(pFile))
	{
		if(pRoot == NULL)
			UnLoadJcVmFile(pFile);
		return NULL;
	}
	return pFile;
}

static CJcFile* QueryJcVmFile(CJcFile* pRoot, const char* sFileName, jc_uint bLoadLink)
{
	CJcFile* pFile = g_pJcFile;
	while(pFile)
	{
		if(!g_oInterface.CompareFile(pFile->sFileName, sFileName))
		{
			if(bLoadLink)
			{
				if(pFile->pChildren == NULL && pFile->pLibraryTable->len)
				{
					if(LoadJcVmChildFile(pRoot?pRoot:pFile, pFile))
					{
						UnLoadJcVmChildFile(pFile, NULL);
						return NULL;
					}
				}
				++pFile->nCounter;
			}
			return pFile;
		}
		pFile = pFile->pNext;
	}
	g_nError = JCVM_MME_FIND_FAILURE;
	return NULL;
}

static jc_uint LoadJcVmChildFile(CJcFile* pRoot, CJcFile* pParent)
{
	CJcFileItem *pFileItem, *pNewItem;
	if(pRoot == NULL)
		pRoot = pParent;
	if(pParent->pChildren)
	{
		pFileItem = pParent->pChildren;
		while(pFileItem)
		{
			if(AddChildFileItem(pRoot, pFileItem->pFile))
				return 1;
			pFileItem = pFileItem->pNext;
		}
	}
	else
	{
		jc_uint i, nCount = pParent->pLibraryTable->len / sizeof(jc_uint);
		for(i=0; i<nCount; ++i)
		{
			CJcFile* pFile;
			jc_char* sFileName = pParent->pConstSegment->data + pParent->pLibraryTable->pLibTable[i];
			jc_char* sFullPath = FindFile(sFileName, 1);
			if(sFullPath == NULL)
				return 1;
			pFile = QueryJcVmFile(sFullPath, 0);
			if(pFile)
			{
				g_oInterface.Free(sFullPath);
				if(AddChildFileItem(pRoot, pFile) || LoadJcVmChildFile(pRoot, pFile))
					return 1;
			}
			else
			{
				pFile = LoadJcVmFile(pRoot, sFullPath);
				g_oInterface.Free(sFullPath);
				if(pFile == NULL)
					return 1;
			}
		}
	}
	return 0;
}

static jc_uint AddChildFileItem(CJcFile* pRoot, CJcFile* pFile)
{
	CJcFileItem* pFileItem;
	if(pFile == pRoot)
		return 0;
	pFileItem = pRoot->pChildren;
	while(pNewItem)
	{
		if(pFileItem->pFile == pFile)
			break;
		pFileItem = pFileItem->pNext;
	}
	if(pFileItem)
		return 0;
	pFileItem = (CJcFileItem*)g_oInterface.Malloc(sizeof(CJcFileItem));
	if(pFileItem == NULL)
	{
		g_nError = JCVM_MME_LACK_MEMORY;
		return 1;
	}
	pFileItem->pFile = pFile;
	++pFile->nCounter;
	pFileItem->pNext = pRoot->pChildren;
	pRoot->pChildren = pFileItem;
	return 0;
}

static jc_void UnLoadJcVmChildFile(CJcFile* pRoot, CJcFileItem* pEndFileItem)
{
	CJcFileItem* pFileItem = pRoot->pChildren;
	if(pEndFileItem)
		pRoot->pChildren = pEndFileItem->pNext;
	else
		pRoot->pChildren = NULL;
	while(pFileItem)
	{
		CJcFileItem* pNext = pFileItem->pNext;
		UnLoadJcVmFile(pFileItem->pFile);
		g_oInterface.Free(pFileItem);
		if(pFileItem == pEndFileItem)
			break;
		pFileItem = pNext;
	}
}

static void UnLoadJcVmFile(CJcFile* pFile)
{
	--pFile->nCounter;
	if(pFile->nCounter)
		return;
	if(pFile->sFileName)
		g_oInterface.Free(pFile->sFileName);
	if(pFile->sFileData)
		g_oInterface.Free(pFile->sFileData);


	UnLoadAllJcHostFile();
	UnLoadJcVmChildFile(pFile, NULL);

}
struct CJcFile
{
	jc_uint nCounter;
	jc_uint nFileType;
	jc_char* sFileName;
	jc_char* sFileData;
	jc_uint nDataLen;
	CJcCodeSegument* pCodeSegment;
	CJcDataSegument* pDataSegment;
	CJcConstSegument* pConstSegment;
	CJcSymbolTable* pSymbolTable;
	CJcLibTable* pLibraryTable;
	CJcLibTable* pHostTable;
	CJcEntry* pEntry;

	jc_char** pSymbolLinkTable;

	CJcHostFile* pHostFile;
	CJcFileItem* pChildren;

	CJcFile *pNext, *pPrev;
};