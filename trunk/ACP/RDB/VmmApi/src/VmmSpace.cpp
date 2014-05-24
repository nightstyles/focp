
#include "VmmSpace.hpp"
#include "VmmControl.hpp"

#ifdef MSVC
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000i64)
#else
#define ComputePageAddr(nAddr) ((nAddr) & 0xFFFFFFFFFFFF0000ULL)
#endif

FOCP_BEGIN();

static CVirtualStorage g_oStorage;
static CVmmControl g_oVmmControler;
uint32 g_nAutoIncPageSize = 0;
const char* GetShellPath(char* sShellPath);

//-------------------------------------------------------------
// CVirtualSpace
//-------------------------------------------------------------
CVirtualSpace::CVirtualSpace()
{
}

CVirtualSpace::~CVirtualSpace()
{
}

//-------------------------------------------------------------
// CSmallSpace
//-------------------------------------------------------------
uint32 CGetSmallPageKey::GetKey(CSmallSpaceNode& oSrc)
{
	return oSrc.nBlockSize;
}

void CSmallSpaceAccess::Clear(void * p)
{
	CSmallSpaceNode* pNode = (CSmallSpaceNode*)p;
	if(pNode->nPageAddr)
		g_oStorage.DeAllocatePage(pNode->nPageAddr);
}

uint64 CGlobalSpaceAllocator::Allocate(uint32, int32)
{
	return 0;
}

void CGlobalSpaceAllocator::DeAllocate(uint64)
{
}

static CGetSmallPageKey g_oSpaceGetKey;
static CNumberCompare<uint32> g_oSpaceCompare;
static CVirtualAccess g_oGlobalSpaceAccess;
static CGlobalSpaceAllocator g_oGlobalSpaceAllocator;
static CSmallSpaceAccess g_oSpaceAccess;
static CVirtualAllocator g_oSpaceAllocator;

CSmallSpace::CSmallSpace():
m_oSpace(&g_oGlobalSpaceAccess, &g_oSpaceGetKey, &g_oSpaceCompare, &g_oGlobalSpaceAllocator, &g_oGlobalSpaceAllocator)
{
	m_bInMemory = 0;
	m_nBlockSize = CVmmRbTree<CSmallSpaceNode, uint32>::VMM_NODE_SIZE;
	m_nTotalBlock = CVirtualPage::GetTotalBlock(m_nBlockSize);
}

CSmallSpace::~CSmallSpace()
{
}

bool CSmallSpace::CreateObject(int32 bInMemory)
{
	m_bInMemory = (bInMemory?1:0);
	return CreateGlobalSpace();
}

void CSmallSpace::DestroyObject()
{
}

uint64 CSmallSpace::GetThis()
{
	return m_oSpace.GetThis();
}

CVirtualSpace& CSmallSpace::SetThis(uint64 nThis)
{
	if(nThis)
		m_bInMemory = ((nThis&VMM_MEMORY_FLAG)?1:0);
	m_oSpace.SetThis(nThis);
	return *this;
}

int32 CSmallSpace::InMemory()
{
	return m_bInMemory;
}

void CSmallSpace::DeAllocateAll()
{
}

uint64 CSmallSpace::Allocate(uint32 nSize)
{
	register uint32 nBlockSize = MakeAlign(nSize, 8);
	register uint32 nTotalBlock = CVirtualPage::GetTotalBlock(nBlockSize);
	if(!nTotalBlock)
		return 0;
	register uint64 nAddr = TryAllocate(nBlockSize);
	if(nAddr)
		return nAddr;
	register uint64 nPageAddr = AllocateNewPage(nTotalBlock, nBlockSize);
	if(nPageAddr == VMM_INVALID_ADDR64)
		return 0;
	m_oMutex.Enter();
	if(!InsertPage(nPageAddr, nBlockSize))
		nAddr = g_oStorage.AllocateBlock(nPageAddr);
	else
		g_oStorage.DeAllocatePage(nPageAddr);
	m_oMutex.Leave();
	return nAddr;
}

void CSmallSpace::DeAllocate(uint64 nAddr)
{
	register uint32 nBlockSize;
#if VMM_SAFE_ENABLED
	if(!nAddr)
		Abort();
#endif

	register uint64 nPageAddr;
#ifdef VMM_COMPLEX_MAP
	nPageAddr = ComputePageAddr(nAddr);
#else
	if(nAddr & VMM_MEMORY_FLAG)
	{
		register uint32 nHeadAddr = (uint32)g_oStorage.m_pDevices[nAddr>>32]->m_pPages;
		nPageAddr = ComputePageAddr(nAddr - nHeadAddr) + nHeadAddr;
	}
	else nPageAddr = ComputePageAddr(nAddr);
#endif

	m_oMutex.Enter();
	register bool bEmpty = g_oStorage.DeAllocateBlock(nAddr, nBlockSize);
	if(bEmpty)
	{
		CVmmRbTree<CSmallSpaceNode, uint32>::CIterator idx = FindBlock(nPageAddr, nBlockSize);
#if VMM_SAFE_ENABLED
		if(idx == m_oSpace.End())
			Abort();
#endif
		register uint64 nThis = idx.GetThis();
		m_oSpace.Erase(idx);
		m_oMutex.Leave();
		g_oStorage.DeAllocatePage(nPageAddr);
		DeAllocate(nThis);
		return;
	}
	m_oMutex.Leave();
}

uint64 CSmallSpace::AllocateNewPage(uint32 nTotalBlock, uint32 nBlockSize)
{
	register uint64 nPageAddr = g_oStorage.AllocatePage(m_bInMemory);
	if(nPageAddr == VMM_INVALID_ADDR64)
		return VMM_INVALID_ADDR64;
	if(m_bInMemory)
	{
		register CVirtualPageHead* pHead = GET_MEMORY_OBJECT(CVirtualPageHead, nPageAddr);
		CVirtualPage(nPageAddr, pHead, nBlockSize, nTotalBlock);
	}
	else
	{
		register CVirtualCachePage* pPage = g_oStorage.MapPage(nPageAddr);
		CVirtualPage(nPageAddr, pPage, nBlockSize, nTotalBlock);
		g_oStorage.UnMapPage(pPage);
	}
	return nPageAddr;
}

uint32 CSmallSpace::InsertPage(uint64 nPageAddr, uint32 nBlockSize)
{
	CSmallSpaceNode oNode = {nPageAddr, nBlockSize};
	register uint64 nNode = TryAllocate(m_nBlockSize);
	if(!nNode)
	{
		if(AllocatePagePair())
			return 1;
		nNode = TryAllocate(m_nBlockSize);
	}
	CVmmRbTree<CSmallSpaceNode, uint32>::CVmmRbTreeNode it = m_oSpace.NodeAt(nNode, true);
	it.SetValue(oNode);
	m_oSpace.InsertNode(it, nBlockSize);
	return 0;
}

CVmmRbTree<CSmallSpaceNode, uint32>::CIterator CSmallSpace::FindBlock(uint64 nPageAddr, uint32 nBlockSize)
{
	CVmmRbTree<CSmallSpaceNode, uint32>::CIterator idx = m_oSpace.Find(nBlockSize), end = m_oSpace.End();
	while(idx != end)
	{
		CSmallSpaceNode& oNode = idx.GetValue();
		if(oNode.nBlockSize != nBlockSize)
			break;
		if(oNode.nPageAddr == nPageAddr)
			return idx;
		++idx;
	}
	return end;
}

uint32 CSmallSpace::AllocatePagePair()
{
	register uint64 nPage1 = AllocateNewPage(m_nTotalBlock, m_nBlockSize);
	if(nPage1 == VMM_INVALID_ADDR64)
		return 1;
	register uint64 nPage2 = AllocateNewPage(m_nTotalBlock, m_nBlockSize);
	if(nPage2 == VMM_INVALID_ADDR64)
	{
		g_oStorage.DeAllocatePage(nPage1);
		return 1;
	}
	register uint64 nNode1 = g_oStorage.AllocateBlock(nPage1);
	register uint64 nNode2 = g_oStorage.AllocateBlock(nPage2);
	CSmallSpaceNode oNode1 = {nPage1, m_nBlockSize};
	CSmallSpaceNode oNode2 = {nPage2, m_nBlockSize};

	CVmmRbTree<CSmallSpaceNode, uint32>::CVmmRbTreeNode it1(m_oSpace.NodeAt(nNode1, true)), it2(m_oSpace.NodeAt(nNode2, true));
	it1.SetValue(oNode2);
	it2.SetValue(oNode1);
	m_oSpace.InsertNode(it1, m_nBlockSize);
	m_oSpace.InsertNode(it2, m_nBlockSize);
	return 0;
}

uint64 CSmallSpace::TryAllocate(uint32 nBlockSize)
{
	register uint64 nAddr = 0;
	m_oMutex.Enter();
	CVmmRbTree<CSmallSpaceNode, uint32>::CIterator idx = m_oSpace.UpperBound(nBlockSize), end = m_oSpace.End();
	for(--idx; idx!=end; --idx)
	{
		CSmallSpaceNode& oNode = idx.GetValue();
		if(oNode.nBlockSize != nBlockSize)
			break;
		nAddr = g_oStorage.AllocateBlock(oNode.nPageAddr);
		if(nAddr)
			break;
	}
	m_oMutex.Leave();
	return nAddr;
}

bool CSmallSpace::CreateGlobalSpace()
{
	register uint64 nPageAddr = AllocateNewPage(m_nTotalBlock, m_nBlockSize);
	if(nPageAddr == VMM_INVALID_ADDR64)
		return false;

	register uint64 nNode1 = g_oStorage.AllocateBlock(nPageAddr);
	g_oStorage.AllocateBlock(nPageAddr);
	register uint64 nHeader = nNode1 + sizeof(CVmmBaseRbTreeInfo);
	CSmallSpaceHead oHead = { {0, 0, nHeader}, { {0, 0}, 0, nHeader, nHeader, 1 } };
	vcommit(nNode1, (char*)&oHead, sizeof(oHead));

	m_oSpace.SetThis(nNode1);
	if(InsertPage(nPageAddr, m_nBlockSize))
	{
		m_oSpace.SetThis(0);
		g_oStorage.DeAllocatePage(nPageAddr);
		return false;
	}

	return true;
}

//-------------------------------------------------------------
// CBigSpace
//-------------------------------------------------------------
uint32 CGetBigPageKey::GetKey(CBigSpaceNode& oSrc)
{
	return oSrc.nBlockSize;
}

uint64 CBigSpaceAllocator::Allocate(uint32, int32)
{
	return 0; // not use the function
}

void CBigSpaceAllocator::DeAllocate(uint64 nAddr)
{
	g_oStorage.DeAllocatePage(nAddr);
}

void CBigSpaceAccess::Clear(void* p)
{
	CBigSpaceNode* pNode = (CBigSpaceNode*)p;
	CBigSpacePage* pPages = (CBigSpacePage*)((CVmmBasicRbTreeNode<CBigSpaceNode>*)p + 1);
	register uint16 nCurPage = pNode->nCurPage;
	if(nCurPage != 0xFFFF)
	{
		do
		{
			CBigSpacePage& oPage = pPages[nCurPage];
			g_oStorage.DeAllocatePage(oPage.nPageAddr);
			nCurPage = oPage.nNextPage;
		}while(nCurPage != pNode->nCurPage);
	}
}

static CGetBigPageKey g_oBigSpaceGetKey;
static CBigSpaceAccess g_oBigSpaceAccess;
static CBigSpaceAllocator g_oBigSpaceAllocator;

CBigSpace::CBigSpace():
m_oSpace(&g_oBigSpaceAccess, &g_oBigSpaceGetKey, &g_oSpaceCompare, &g_oBigSpaceAllocator)
{
	m_bInMemory = 0;
}

CBigSpace::~CBigSpace()
{
}

bool CBigSpace::CreateObject(int32 bInMemory)
{
	m_bInMemory = (bInMemory?1:0);
	return m_oSpace.CreateObject(0, m_bInMemory);
}

void CBigSpace::DestroyObject()
{
	m_oSpace.DestroyObject();
}

uint64 CBigSpace::GetThis()
{
	return m_oSpace.GetThis();
}

CVirtualSpace& CBigSpace::SetThis(uint64 nThis)
{
	if(nThis)
		m_bInMemory = ((nThis&VMM_MEMORY_FLAG)?1:0);
	m_oSpace.SetThis(nThis);
	return *this;
}

int32 CBigSpace::InMemory()
{
	return m_bInMemory;
}

void CBigSpace::DeAllocateAll()
{
	m_oSpace.Clear();
}

uint64 CBigSpace::TryAllocate(uint32 nBlockSize, uint64 &nPageAddr)
{
	register uint64 nAddr = 0;
	m_oMutex.Enter();
	CVmmRbTree<CBigSpaceNode, uint32>::CIterator idx = m_oSpace.UpperBound(nBlockSize), end = m_oSpace.End();
	for(--idx; idx!=end; --idx)
	{
		CBigSpaceNode& oNode = idx.GetValue();
		if(oNode.nBlockSize != nBlockSize)
			break;
		if(oNode.nFreeRecNum)
		{
			nAddr = Allocate(oNode);
			break;
		}
		else if(!nPageAddr && oNode.nCurPageNum < oNode.nPageCapacity)
			nPageAddr = idx.GetThis();
	}
	m_oMutex.Leave();
	return nAddr;
}

uint64 CBigSpace::Allocate(uint32 nSize)
{
	register uint32 nBlockSize = MakeAlign(nSize, 8);
	register uint64 nPageAddr = 0;
	register uint64 nAddr = TryAllocate(nBlockSize, nPageAddr);
	if(nAddr)
		return nAddr;

	CVmmRbTree<CBigSpaceNode, uint32>::CIterator idx;

	register bool bNewPage = false;
	if(nPageAddr)
	{
		m_oMutex.Enter();
		idx = m_oSpace.At(nPageAddr);
	}
	else
	{
		nPageAddr = AllocateMainPage(nBlockSize);
		if(!nPageAddr)
			return nAddr;
		bNewPage = true;
		m_oMutex.Enter();
		CVmmRbTree<CBigSpaceNode, uint32>::CVmmRbTreeNode oNode = m_oSpace.NodeAt(nPageAddr, true);
		idx = m_oSpace.InsertNode(oNode, nBlockSize);
		if(idx == m_oSpace.End())
		{
			m_oMutex.Leave();
			return nAddr;
		}
	}
	CBigSpaceNode& oNode = idx.GetValue();
	if(!AppendPage(oNode, nPageAddr))
		nAddr = Allocate(oNode);
	else if(bNewPage)
		m_oSpace.Erase(idx);
	m_oMutex.Leave();
	return nAddr;
}

void CBigSpace::DeAllocate(uint64 nAddr)
{
#if VMM_SAFE_ENABLED
	if(!nAddr)
		Abort();
#endif
	register uint32 nIdx;
	register uint64 nPageAddr;
#ifdef VMM_COMPLEX_MAP
	nPageAddr = ComputePageAddr(nAddr);
#else
	if(nAddr & VMM_MEMORY_FLAG)
	{
		register uint32 nHeadAddr = (uint32)g_oStorage.m_pDevices[nAddr>>32]->m_pPages;
		nPageAddr = ComputePageAddr(nAddr - nHeadAddr) + nHeadAddr;
	}
	else nPageAddr = ComputePageAddr(nAddr);
#endif
	register uint64 nMainPage = GetMainPage(nPageAddr, nIdx);
#if VMM_SAFE_ENABLED
	if(!nMainPage)
		Abort();
#endif
	m_oMutex.Enter();
	DeAllocate(nMainPage, nIdx, (uint32)(nAddr-nPageAddr-sizeof(CBigPageHead)));
	m_oMutex.Leave();
}

uint64 CBigSpace::AllocateMainPage(uint32 nBlockSize)
{
	register uint32 i, nPageSize = VMM_PAGE_SIZE - sizeof(CBigPageHead);
	nPageSize /= nBlockSize;
	if(!nPageSize)
		return 0;
	// 48 + nCapacity * (16 + ((nPageSize-1)/32+1)*4) == 65536
	register uint32 nCapacity = 523904/(159+nPageSize);
	register uint64 nPageAddr = g_oStorage.AllocatePage(m_bInMemory);
	if(nPageAddr == VMM_INVALID_ADDR64)
		return 0;
	CBigSpaceNode* pSpaceNode;
	CVirtualCachePage* pCachePage = NULL;
	if(m_bInMemory)
		pSpaceNode = GET_MEMORY_OBJECT(CBigSpaceNode, nPageAddr);
	else
	{
		pCachePage = g_oStorage.MapPage(nPageAddr);
		pSpaceNode = (CBigSpaceNode*)pCachePage->m_sPage;
	}
	pSpaceNode->nBlockSize = nBlockSize;
	pSpaceNode->nPageSize = nPageSize;
	pSpaceNode->nPageCapacity = nCapacity;
	pSpaceNode->nCurPageNum = 0;
	pSpaceNode->nCurPage = 0xFFFF;
	pSpaceNode->nNullPage = 0;
	pSpaceNode->nFreeRecNum = 0;
	register CBigSpacePage* pPages = (CBigSpacePage*)((CVmmBasicRbTreeNode<CBigSpaceNode>*)pSpaceNode + 1);
	register uint32 nUnitNum = ((nPageSize-1)>>5)+1;
	register uint32 * pFlag = (uint32*)(pPages + nCapacity);
	CBinary::MemorySet(pFlag, 0, nCapacity*nUnitNum*4);
	register uint32 nLast, nMod = nPageSize & 0x1F;
	if(nMod)
	{
		static uint32 x[] = {0x1, 0x3, 0x7, 0xF,
							 0x1F, 0x3F, 0x7F, 0xFF,
							 0x1FF, 0x3FF, 0x7FF, 0xFFF,
							 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
							 0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
							 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
							 0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
							 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF};
		nLast = ~x[nMod-1];
	}
	for(i=0; i<nCapacity; ++i)
	{
		pPages[i].nPageAddr = 0;
		pPages[i].nFreeNum = 0;
		pPages[i].nAllocator = 0;
		if(i)
			pPages[i].nPrevPage = i - 1;
		else
			pPages[i].nPrevPage = nCapacity-1;
		if(i == nCapacity-1)
			pPages[i].nNextPage = 0;
		else
			pPages[i].nNextPage = i+1;
		if(nMod)
		{
			pFlag[nUnitNum-1] = nLast;
			pFlag += nUnitNum;
		}
	}
	if(pCachePage)
	{
		pCachePage->Commit();
		g_oStorage.UnMapPage(pCachePage);
	}

	return nPageAddr;
}

uint64 CBigSpace::Allocate(CBigSpaceNode& oNode)
{
	struct CSpaceHelper
	{
		uint32 nMod;
		uint32 nBit;
		uint32 nInit;
	};
	static CSpaceHelper oMod[] =
	{
		{0x000000FF, 0x00000001, 0},
		{0x0000FF00, 0x00000100, 8},
		{0x00FF0000, 0x00010000, 16},
		{0xFF000000, 0x01000000, 24}
	};
	register uint64 nAddr = 0;
	register uint32 nCapacity = oNode.nPageCapacity;
	register uint32 nUnitNum = ((oNode.nPageSize-1)>>5)+1;
	register CBigSpacePage* pPages = (CBigSpacePage*)((CVmmBasicRbTreeNode<CBigSpaceNode>*)&oNode + 1);
	register uint32* pTables = (uint32*)(pPages + nCapacity);
	register uint32 nCurPage = oNode.nCurPage;

	do
	{
		CBigSpacePage& oPage = pPages[nCurPage];
		if(oPage.nFreeNum)
		{
			register uint32* pTable = pTables + nCurPage * nUnitNum;
			for(register uint32 i=0; i<nUnitNum; ++i)
			{
				register uint32 &nTable = pTable[oPage.nAllocator];
				if(nTable != 0xFFFFFFFF)
				{
					register CSpaceHelper* pMod = oMod;
					while(true)
					{
						if((pMod->nMod & nTable) != pMod->nMod)
						{
							register uint32 j, nBit = pMod->nBit;
							for(j=pMod->nInit; nTable&nBit; ++j)
								nBit<<=1;
							nTable |= nBit;
							nAddr = oPage.nPageAddr + sizeof(CBigPageHead) + ((oPage.nAllocator<<5) + j) * oNode.nBlockSize;
							break;
						}
						++pMod;
					}
					break;
				}
				++oPage.nAllocator;
				if(oPage.nAllocator == nUnitNum)
					oPage.nAllocator = 0;
			}
			--oPage.nFreeNum;
			break;
		}
		nCurPage = oPage.nNextPage;
	}while(nCurPage != oNode.nCurPage);

	oNode.nCurPage = nCurPage;
	--oNode.nFreeRecNum;
	if(!m_bInMemory)
		g_oStorage.CommitMap(&oNode);
	return nAddr;
}

void CBigSpace::MovePage(CBigSpacePage* pPages, uint16 nIdx, uint16& nFrom, uint16& nTo)
{
	CBigSpacePage& oPage = pPages[nIdx];
	if(nFrom == nIdx)
	{
		nFrom = oPage.nNextPage;
		if(oPage.nNextPage == nIdx)
			nFrom = 0xFFFF;
	}
	if(nFrom != 0xFFFF)
	{
		CBigSpacePage& oPrevPage = pPages[oPage.nPrevPage];
		CBigSpacePage& oNextPage = pPages[oPage.nNextPage];
		oPrevPage.nNextPage = oPage.nNextPage;
		oNextPage.nPrevPage = oPage.nPrevPage;
	}

	if(nTo == 0xFFFF)
	{
		oPage.nPrevPage = nIdx;
		oPage.nNextPage = nIdx;
	}
	else
	{
		CBigSpacePage& oNextPage = pPages[nTo];
		CBigSpacePage& oPrevPage = pPages[oNextPage.nPrevPage];
		oPage.nNextPage = nTo;
		oPage.nPrevPage = oNextPage.nPrevPage;
		oPrevPage.nNextPage = nIdx;
		oNextPage.nPrevPage = nIdx;
	}
	nTo = nIdx;
}

uint32 CBigSpace::AppendPage(CBigSpaceNode& oNode, uint64 nMainPage)
{
	register uint64 nPageAddr = 	g_oStorage.AllocatePage(m_bInMemory);
	if(nPageAddr == VMM_INVALID_ADDR64)
		return 1;
	register uint16 nIdx = oNode.nNullPage;
	CBigSpacePage* pPages = (CBigSpacePage*)((CVmmBasicRbTreeNode<CBigSpaceNode>*)&oNode + 1);
	MovePage(pPages, nIdx, oNode.nNullPage, oNode.nCurPage);
	CBigSpacePage& oPage = pPages[nIdx];
	oPage.nPageAddr = nPageAddr;
	oPage.nFreeNum = oNode.nPageSize;
	oPage.nAllocator = 0;
	oNode.nCurPageNum += 1;
	oNode.nFreeRecNum += oPage.nFreeNum;
	if(!m_bInMemory)
		g_oStorage.CommitMap(&oNode);
	CBigPageHead* pHead = (CBigPageHead*)(m_bInMemory?GET_MEMORY_ADDR(nPageAddr):g_oStorage.Map(nPageAddr));
	pHead->nMainPage = nMainPage;
	pHead->nIdx = nIdx;
	if(!m_bInMemory)
	{
		g_oStorage.CommitMap(pHead);
		g_oStorage.UnMap(pHead);
	}
	return 0;
}

uint64 CBigSpace::GetMainPage(uint64 nPageAddr, uint32& nIdx)
{
	register CBigPageHead* pHead = (CBigPageHead*)(m_bInMemory?GET_MEMORY_ADDR(nPageAddr):g_oStorage.Map(nPageAddr));
	nIdx = pHead->nIdx;
	nPageAddr = pHead->nMainPage;
	if(!m_bInMemory)
		g_oStorage.UnMap(pHead);
	return nPageAddr;
}

void CBigSpace::DeAllocate(uint64 nMainPage, uint32 nIdx, uint32 nOffset)
{
	CVmmRbTree<CBigSpaceNode, uint32>::CIterator idx = m_oSpace.At(nMainPage);
	CBigSpaceNode& oNode = idx.GetValue();
	register uint32 nUnitNum = ((oNode.nPageSize-1)>>5)+1;
	register CBigSpacePage* pPages = (CBigSpacePage*)((CVmmBasicRbTreeNode<CBigSpaceNode>*)&oNode + 1);
	CBigSpacePage& oPage = pPages[nIdx];
#if VMM_SAFE_ENABLED
	if(!oPage.nFreeNum)
		Abort();
	if(nOffset % oNode.nBlockSize)
		Abort();
#endif
	register uint32* pTable = (uint32*)(pPages + oNode.nPageCapacity) + nIdx*nUnitNum;
	register uint32 nSub = nOffset / oNode.nBlockSize;
	register uint32 nFlag = nSub>>5;
	register uint32 nBit = 1 << (nSub & 0x1F);
#if VMM_SAFE_ENABLED
	if(!(pTable[nFlag] & nBit))
		Abort();
#endif
	--oNode.nFreeRecNum;
	--oPage.nFreeNum;
	pTable[nFlag] &= ~nBit;
	if(!oPage.nFreeNum)
	{
		g_oStorage.DeAllocatePage(oPage.nPageAddr);
		oPage.nPageAddr = 0;
		oPage.nAllocator = 0;
		--oNode.nCurPageNum;
		if(oNode.nCurPageNum)
			MovePage(pPages, nIdx, oNode.nCurPage, oNode.nNullPage);
		else
		{
			m_oSpace.Erase(idx);
			return;
		}
	}
	if(!m_bInMemory)
		g_oStorage.CommitMap(&oNode);
}

void SaveVmmConfig()
{
	g_oStorage.SaveConfig();
}

VMM_API int32 InitializeVmmControler(const char* sControlFile, uint32 nStatusNum)
{
	return g_oVmmControler.Startup(sControlFile, nStatusNum);
}

VMM_API uint8 GetVmmControlStatus(uint32 nIdx)
{
	return g_oVmmControler.GetStatus(nIdx);
}

VMM_API void SetVmmControlStatus(uint32 nIdx, uint8 nStatus)
{
	g_oVmmControler.SetStatus(nIdx, nStatus);
}

VMM_API int32 StartupVmm(const char* sConfilePath, const char* sDevicePath, const char* sBackupPath, uint32 nSwapSize, uint32 nAutoIncPageSize)
{
	if(!nAutoIncPageSize || nAutoIncPageSize > 32766)
		g_nAutoIncPageSize = 10;
	else
		g_nAutoIncPageSize = nAutoIncPageSize;
	if(nSwapSize < 32)
		nSwapSize = 32;
	else if(nSwapSize > 2047)
		nSwapSize = 2047;
	if(g_oStorage.Startup(nSwapSize))
		return 1;
	if(g_oStorage.Load(sConfilePath, sDevicePath, sBackupPath))
		return 1;
	FocpInfo(("StartupVmm() success"));
	return 0;
}

VMM_API void CleanupVmm()
{
	g_oStorage.CleanStorage();
}

VMM_API uint32 BackupVmm(const char* sBackupPath)
{
	CFormatString oCommand;
	char* sShellPath = new char[FOCP_MAX_PATH];
	char* sBackupPath2 = new char[FOCP_MAX_PATH];
	GetShellPath(sShellPath);
	CString::StringCopy(sBackupPath2, sBackupPath);
	CDiskFileSystem::GetInstance()->GetOsPathName(sBackupPath2);

#ifdef WINDOWS
	oCommand.Print("%s/mkdir.exe -p %s", sShellPath, sBackupPath2);
#else
	oCommand.Print("mkdir -p %s", sBackupPath2);
#endif
	System(oCommand.GetStr());

	g_oStorage.FlushAll(true);

	uint32 nRet = g_oStorage.BackupTo(sBackupPath);
	if(nRet)
	{
#ifdef WINDOWS
		oCommand.Print("%s/rm.exe -rf %s", sShellPath, sBackupPath2);
#else
		oCommand.Print("rm -rf %s", sBackupPath2);
#endif
		System(oCommand.GetStr());
	}

	delete[] sShellPath;
	delete[] sBackupPath2;
	return nRet;
}

VMM_API bool GetSystemPage(uint64 * pPageAddr)
{
	return 	g_oStorage.GetSystemPage(*pPageAddr);
}

VMM_API uint32 CreateMemoryDevice(uint32 nSize)
{
	if(nSize > 32767)
		nSize = 32767;
	else if(!nSize)
		nSize = 1;
	nSize *= 65536;
	nSize += VMM_DEVICE_HEADSIZE;

	EnterSystemRead();
	uint32 nRet = g_oStorage.CreateMemoryDevice(nSize);
	LeaveSystemRead();

	if(nRet)
		FocpError(("CreateMemoryDevice failure"));
	else
		FocpInfo(("CreateMemoryDevice success"));
	return nRet;
}

VMM_API uint32 CreateFileDevice(uint32 nSize, const char* sFileName)
{
	if(nSize > 32767)
		nSize = 32767;
	else if(!nSize)
		nSize = 1;
	nSize *= 65536;
	nSize += VMM_DEVICE_HEADSIZE;

	EnterSystemRead();
	uint32 nRet = g_oStorage.CreateFileDevice(sFileName, nSize);
	LeaveSystemRead();

	if(nRet)
		FocpError(("CreateFileHeap(%s) failure", sFileName));
	else
		FocpInfo(("CreateFileHeap(%s) success", sFileName));
	return nRet;
}

VMM_API uint32 GetVirtualDeviceCount()
{
	return g_oStorage.GetVirtualDeviceCount();
}

VMM_API uint32 GetVirtualDeviceInfo(uint32 nDeviceNo, CVirtualDeviceInfo* pInfo)
{
	return g_oStorage.GetVirtualDeviceInfo(nDeviceNo, pInfo);
}

VMM_API int32 DirectReadVmm(uint64 nAddr, char* pMem, int32 nSize)
{
	return g_oStorage.Read(nAddr, pMem, nSize);
}

VMM_API int32 DirectWriteVmm(uint64 nAddr, char* pMem, int32 nSize)
{
	return g_oStorage.Write(nAddr, pMem, nSize);
}

VMM_API void* NewVirtualSpace()
{
	return new(std::nothrow) CBigSpace;
}

VMM_API void DeleteVirtualSpace(void* pSpace)
{
	static void* g_pDiskSpace=GetGlobalSpace(0), *g_pMemorySpace=GetGlobalSpace(1);
	if(pSpace != g_pDiskSpace && pSpace != g_pMemorySpace)
		delete (CVirtualSpace*)pSpace;
}

VMM_API bool CreateVirtualSpace(void* pSpace, int32 bInMemory)
{
	return ((CVirtualSpace*)pSpace)->CreateObject(bInMemory);
}

VMM_API void DestroyVirtualSpace(void* pSpace)
{
	((CVirtualSpace*)pSpace)->DestroyObject();
}

VMM_API uint64 GetVirtualSpaceAddress(void* pSpace)
{
	return ((CVirtualSpace*)pSpace)->GetThis();
}

VMM_API void SetVirtualSpaceAddress(void* pSpace, uint64 nAddr)
{
	((CVirtualSpace*)pSpace)->SetThis(nAddr);
}

VMM_API int32 VirtualSpaceInMemory(void* pSpace)
{
	return ((CVirtualSpace*)pSpace)->InMemory();
}

VMM_API uint64 AllocateFromVirtualSpace(void* pSpace, uint32 nSize)
{
	return ((CVirtualSpace*)pSpace)->Allocate(nSize);
}

VMM_API void DeAllocateIntoVirtualSpace(void* pSpace, uint64 nAddr)
{
	((CVirtualSpace*)pSpace)->DeAllocate(nAddr);
}

VMM_API void ClearVirtualSpace(void* pSpace)
{
	((CVirtualSpace*)pSpace)->DeAllocateAll();
}

FOCP_PRIVATE_BEGIN();
struct CGlobalSpace { CSmallSpace oSpace[2]; };
FOCP_PRIVATE_END();

VMM_API void* GetGlobalSpace(int32 bInMemory)
{
	CGlobalSpace* pSpace = CStaticInstance<CGlobalSpace>::GetInstance();
	if(bInMemory)
		return pSpace->oSpace;
	return pSpace->oSpace+1;
}

VMM_API int32 IsMemoryAddr(uint64 nAddr)
{
	return (nAddr & VMM_MEMORY_FLAG)?1:0;
}

VMM_API uint64 vmalloc(uint32 size, int32 bInMemory)
{
	void * pSpace = GetGlobalSpace(bInMemory);
	return AllocateFromVirtualSpace(pSpace, size);
}

VMM_API void vfree(uint64 nAddr)
{
	int32 bInMemory = ((nAddr&VMM_MEMORY_FLAG)?1:0);
	void * pSpace = GetGlobalSpace(bInMemory);
	DeAllocateIntoVirtualSpace(pSpace, nAddr);
}

VMM_API void* GetMemoryAddress(uint64 nAddr)
{
	if(nAddr & VMM_MEMORY_FLAG)
#ifdef VMM_COMPLEX_MAP
		return g_oStorage.m_pDevices[nAddr>>32]->m_pPages + (nAddr&0x7FFFFFFF);
#else
		return (void*)(uint32)nAddr;
#endif
	return NULL;
}

VMM_API void* vmap(uint64 nAddr)
{
	return g_oStorage.Map(nAddr);
}

VMM_API void unvmap(void* pMem)
{
	g_oStorage.UnMap(pMem);
}

VMM_API void vflush(void* pMem)
{
	g_oStorage.CommitMap(pMem);
}

VMM_API void vquery(uint64 nAddr, char* pMem, uint32 nSize)
{
	if(nAddr & VMM_MEMORY_FLAG)
		CBinary::MemoryCopy(pMem, GET_MEMORY_ADDR(nAddr), nSize);
	else
	{
		void* p = vmap(nAddr);
		CBinary::MemoryCopy(pMem, p, nSize);
		unvmap(p);
	}
}

VMM_API void vcommit(uint64 nAddr, char* pMem, uint32 nSize)
{
	if(nAddr & VMM_MEMORY_FLAG)
		CBinary::MemoryCopy(GET_MEMORY_ADDR(nAddr), pMem, nSize);
	else
	{
		void* p = vmap(nAddr);
		CBinary::MemoryCopy(p, pMem, nSize);
		vflush(p);
		unvmap(p);
	}
}

VMM_API void vmemset(uint64 nAddr, int32 c, uint32 nSize)
{
	if(nAddr & VMM_MEMORY_FLAG)
		CBinary::MemorySet(GET_MEMORY_ADDR(nAddr), c, nSize);
	else
	{
		void* p = vmap(nAddr);
		CBinary::MemorySet(p, c, nSize);
		vflush(p);
		unvmap(p);
	}
}

VMM_API void vmemcpy(uint64 nDst, uint64 nSrc, uint32 nSize)
{
	void *pSrc, *pDst;
	if(nSrc & VMM_MEMORY_FLAG)
		pSrc = GET_MEMORY_ADDR(nSrc);
	else
		pSrc = vmap(nSrc);
	if(nDst & VMM_MEMORY_FLAG)
		pDst = GET_MEMORY_ADDR(nDst);
	else
		pDst = vmap(nDst);
	CBinary::MemoryCopy(pDst, pSrc, nSize);
	if(nSrc & VMM_MEMORY_FLAG);
	else unvmap(pSrc);
	if(nDst & VMM_MEMORY_FLAG);
	else
	{
		vflush(pDst);
		unvmap(pDst);
	}
}

VMM_API void FlushVmm()
{
	g_oStorage.FlushAll();
}

FOCP_END();
