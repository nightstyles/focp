
#include "Allocator.hpp"

FOCP_BEGIN();

CBaseClass::CBaseClass()
{
}

CBaseClass::~CBaseClass()
{
}

uint32 CBaseClass::GetInstanceSize() const
{
	return 0;
}

void CBaseClass::ResetObject(void* pObject) const
{
}

void CBaseClass::ConstructInstance(void* pBuf) const
{
}

void CBaseClass::DestructInstance(void* pObject) const
{
}

CPyramidClass::CPyramidClass()
{
}

CPyramidClass::~CPyramidClass()
{
}

uint32 CPyramidClass::GetObjectId(void* pObject) const
{
	return 0;
}

void CPyramidClass::SetObjectId(void* pBuf, uint32 nId) const
{
}

CAllocator::CAllocator(CBaseClass* pClass, bool bThread)
	:m_oMutex(bThread)
{
	m_nSize = 0;
	m_oAllocatePolicy.nInitializeObjectNum=0;
	m_oAllocatePolicy.nMaxObjectNum = 0;
	m_oAllocatePolicy.nRecycleSize = 0;
	m_pArray = NULL;
	m_pClass = pClass;
	m_pReclaimer = NULL;
}

CAllocator::~CAllocator()
{
	Clear();
}

//针对数组管理器而言，才有该返回值。

bool CAllocator::IsArrayContainer() const
{
	return m_pArray != NULL;
}

uint32 CAllocator::GetObjectIndex(void* pData) const
{
	uint8* pObject = (uint8*)pData;
	if(sizeof(ulong) >= (ulong)pObject)
		return (uint32)(-1);
	if(m_pArray == NULL)
		return (uint32)(-1);
	pObject -= sizeof(ulong);
	uint32 nUnitSize = m_pClass->GetInstanceSize()+sizeof(ulong);
	uint8* pEndObj = m_pArray + nUnitSize*m_oAllocatePolicy.nInitializeObjectNum;
	if(pObject < m_pArray || pObject >= pEndObj)
		return (uint32)(-1);
	return (pObject - m_pArray)/nUnitSize;
}

const void* CAllocator::QueryObject(uint32 nIdx) const
{
	if(m_pArray == NULL || nIdx >= m_oAllocatePolicy.nInitializeObjectNum)
		return NULL;
	return m_pArray + (m_pClass->GetInstanceSize()+sizeof(ulong))*nIdx + sizeof(ulong);
}

void* CAllocator::CreateObject(bool bLock)
{
	uint8* pRet = NULL;
	if(bLock)
		m_oMutex.Enter();
	if(m_pReclaimer == NULL || (!(pRet=m_pReclaimer->Pop()) && m_nSize < m_oAllocatePolicy.nMaxObjectNum))
	{
		pRet = (uint8*)CMalloc::Malloc(m_pClass->GetInstanceSize()+sizeof(ulong));
		if(pRet)
		{
			++m_nSize;
			m_pClass->ConstructInstance(pRet+sizeof(ulong));
		}
	}
	if(bLock)
		m_oMutex.Leave();
	if(pRet)
		return pRet+sizeof(ulong);
	return NULL;
}

void CAllocator::DestroyObject(void* pObject, bool bLock)
{
	if((ulong)pObject < sizeof(ulong))
		return;
	m_pClass->ResetObject(pObject);
	if(bLock)
		m_oMutex.Enter();
	if(m_pReclaimer == NULL || m_pReclaimer->GetSize() >= m_oAllocatePolicy.nRecycleSize)
	{
		--m_nSize;
		m_pClass->DestructInstance(pObject);
		CMalloc::Free((uint8*)pObject-sizeof(ulong));
	}
	else
		m_pReclaimer->Push((uint8*)pObject-sizeof(ulong));
	if(bLock)
		m_oMutex.Leave();
}

uint32 CAllocator::GetCapacity() const
{
	return m_oAllocatePolicy.nMaxObjectNum;
}

uint32 CAllocator::GetTotalSize() const
{
	return m_nSize;
}

uint32 CAllocator::GetActiveSize() const
{
	((CMutex&)m_oMutex).Enter();
	uint32 nRet = m_nSize - m_pReclaimer->GetSize();
	((CMutex&)m_oMutex).Leave();
	return nRet;
}

bool CAllocator::SetAllocatePolicy(const CAllocatePolicy &oPolicy)
{
	bool bForbidCustomPolicy;
	CAllocatePolicy oDefault;
	if(m_nSize || m_pArray || m_pReclaimer)
		return false;
	if(GetDefaultPolicy(oDefault, bForbidCustomPolicy) && bForbidCustomPolicy)
		return true;
	m_oAllocatePolicy = oPolicy;
	if(m_oAllocatePolicy.nMaxObjectNum < m_oAllocatePolicy.nInitializeObjectNum)
		m_oAllocatePolicy.nMaxObjectNum = m_oAllocatePolicy.nInitializeObjectNum;
	if(m_oAllocatePolicy.nRecycleSize < m_oAllocatePolicy.nInitializeObjectNum)
		m_oAllocatePolicy.nRecycleSize = m_oAllocatePolicy.nInitializeObjectNum;
	if(m_oAllocatePolicy.nRecycleSize > m_oAllocatePolicy.nMaxObjectNum)
		m_oAllocatePolicy.nRecycleSize = m_oAllocatePolicy.nMaxObjectNum;
	if(m_oAllocatePolicy.nRecycleSize)
		m_pReclaimer = new CBaseSingleList<uint8>(0);
	if(m_oAllocatePolicy.nInitializeObjectNum)
	{
		uint8* pShift;
		uint32 i, nUnitSize = m_pClass->GetInstanceSize()+sizeof(ulong);
		if(m_oAllocatePolicy.nMaxObjectNum == m_oAllocatePolicy.nInitializeObjectNum &&
				m_oAllocatePolicy.nMaxObjectNum == m_oAllocatePolicy.nRecycleSize)
		{
			uint32 nSize = nUnitSize * m_oAllocatePolicy.nRecycleSize;
			pShift = (uint8*)CMalloc::Malloc(nSize);
			m_pArray = pShift;
			for(i=0; i<m_oAllocatePolicy.nRecycleSize; ++i)
			{
				m_pClass->ConstructInstance(pShift+sizeof(ulong));
				m_pReclaimer->Push(pShift);
				pShift += nUnitSize;
			}
		}
		else
		{
			for(i=0; i<m_oAllocatePolicy.nInitializeObjectNum; ++i)
			{
				pShift = (uint8*)CMalloc::Malloc(nUnitSize);
				m_pClass->ConstructInstance(pShift+sizeof(ulong));
				m_pReclaimer->Push(pShift);
			}
		}
		m_nSize = m_oAllocatePolicy.nInitializeObjectNum;
	}
	return true;
}

bool CAllocator::GetDefaultPolicy(CAllocatePolicy &oPolicy, bool &bForbidCustomPolicy)
{
	return false;
}

void CAllocator::Clear()
{
	if(m_pReclaimer)
	{
		uint8* pData;
		while((pData=m_pReclaimer->Pop()))
		{
			m_pClass->DestructInstance(pData+sizeof(ulong));
			if(m_pArray == NULL)
				CMalloc::Free(pData);
		}
		delete m_pReclaimer;
		m_pReclaimer = NULL;
	}
	if(m_pArray)
	{
		CMalloc::Free(m_pArray);
		m_pArray = NULL;
	}
	m_oAllocatePolicy.nInitializeObjectNum=0;
	m_oAllocatePolicy.nMaxObjectNum = 0;
	m_oAllocatePolicy.nRecycleSize = 0;
}

CPyramidAllocator::CPyramidAllocator(CPyramidClass* pClass, bool bThread)
	:m_oMutex(bThread)
{
	m_pClass = pClass;
	m_nLevel = 0;
	m_nMaxId = 0;
	m_nSize = 0;
	m_nInstanceSize = m_pClass->GetInstanceSize();
	m_pNode = CreateNode(m_nLevel);
}

CPyramidAllocator::~CPyramidAllocator()
{
	DestroyNode(m_pNode, m_nLevel);
	m_pNode = NULL;
}

uint32 CPyramidAllocator::GetCapacity()
{
	return m_nMaxId;
}

uint32 CPyramidAllocator::GetSize()
{
	return m_nSize;
}

void* CPyramidAllocator::CreateObject()
{
	bool bFull;
	uint8 nBit;
	uint32 pIndex[7];
	void* pNode[7];
	uint32 nIdx, nMod;

	m_oMutex.Enter();

	if(!ApplyNode(pIndex, pNode))
	{
		m_oMutex.Leave();
		return NULL;
	}

	CLeafNode* pLeaf = (CLeafNode*)pNode[6];

	nIdx = pIndex[6]/8;
	nMod = pIndex[6]%8;
	nBit = 1;
	if(nMod)
		nBit <<= nMod;
	pLeaf->nMask.pFlag[nIdx] |= nBit;
	bFull = (pLeaf->nMask.nFlag == 0xFFFFFFFF);
	void* pObject = pLeaf->pObjects + pIndex[6] * m_nInstanceSize;
	for(uint32 j=0; bFull && j<6; ++j)
	{
		uint32 k = 5 - j;
		CBranchNode* pBranch = (CBranchNode*)pNode[k];
		if(!pBranch)
			break;
		nIdx = pIndex[k]/8;
		nMod = pIndex[k]%8;
		nBit = 1;
		if(nMod)
			nBit <<= nMod;
		pBranch->nMask.pFlag[nIdx] |= nBit;
		bFull = (pBranch->nMask.nFlag == 0xFFFFFFFF);
	}

	++m_nSize;
	m_oMutex.Leave();

	return pObject;
}

void* CPyramidAllocator::QueryObject(uint32 nObjectId)
{
	uint8 nBit;
	uint32 nIdx, nMod;
	uint32 pIndex[7];
	void* pNode[7];
	CLeafNode* pLeaf;

	ParseObjectId(nObjectId, pIndex, pNode);
	pLeaf = (CLeafNode*)pNode[6];
	nIdx = pIndex[6]/8;
	nMod = pIndex[6]%8;
	nBit = 1;
	if(nMod)
		nBit <<= nMod;
	if(!(pLeaf->nMask.pFlag[nIdx] & nBit))
		FocpAbort(("CPyramidAllocator::QueryObject(%u), the argument is invalid", nObjectId));
	return pLeaf->pObjects + pIndex[6] * m_nInstanceSize;
}

void CPyramidAllocator::DestroyObject(uint32 nObjectId)
{
	bool bFull;
	uint8 nBit;
	uint32 nIdx, nMod;
	uint32 pIndex[7];
	void* pNode[7];
	CLeafNode* pLeaf;
	void* pObject;

	m_oMutex.Enter();

	if(!m_nSize)
		FocpAbort(("CPyramidAllocator::DestroyObject(%u), the allocator is empty", nObjectId));

	ParseObjectId(nObjectId, pIndex, pNode);

	pLeaf = (CLeafNode*)pNode[6];

	nIdx = pIndex[6]/8;
	nMod = pIndex[6]%8;
	nBit = 1;
	if(nMod)
		nBit <<= nMod;
	if(!(pLeaf->nMask.pFlag[nIdx] & nBit))
		FocpAbort(("CPyramidAllocator::DestroyObject(%u), the argument is invalid", nObjectId));

	pObject = pLeaf->pObjects + pIndex[6] * m_nInstanceSize;

	m_pClass->ResetObject(pObject);
	bFull = (pLeaf->nMask.nFlag == 0xFFFFFFFF);
	pLeaf->nMask.pFlag[nIdx] &= ~nBit;
	for(uint32 j=0; bFull && j<6; ++j)
	{
		uint32 k = 5 - j;
		CBranchNode* pBranch = (CBranchNode*)pNode[k];
		if(!pBranch)
			break;
		nIdx = pIndex[k]/8;
		nMod = pIndex[k]%8;
		nBit = 1;
		if(nMod)
			nBit <<= nMod;
		bFull = (pBranch->nMask.nFlag == 0xFFFFFFFF);
		pBranch->nMask.pFlag[nIdx] &= ~nBit;
	}
	--m_nSize;

	m_oMutex.Leave();
}

void CPyramidAllocator::DestroyObject(void* pObject)
{
	bool bFull;
	uint8 nBit;
	uint32 nObjId, nIdx, nMod;
	uint32 pIndex[7];
	void* pNode[7];
	CLeafNode* pLeaf;
	uint8* pBuf;

	m_oMutex.Enter();

	if(!m_nSize)
		FocpAbort(("CPyramidAllocator::DestroyObject(%X), the object is invalid", pObject));

	nObjId = m_pClass->GetObjectId(pObject);
	ParseObjectId(nObjId, pIndex, pNode);

	pLeaf = (CLeafNode*)pNode[6];
	pBuf = pLeaf->pObjects + pIndex[6] * m_nInstanceSize;
	if(pBuf != (uint8*)pObject)
		FocpAbort(("CPyramidAllocator::DestroyObject(%X), the object is invalid", pObject));

	nIdx = pIndex[6]/8;
	nMod = pIndex[6]%8;
	nBit = 1;
	if(nMod)
		nBit <<= nMod;
	if(!(pLeaf->nMask.pFlag[nIdx] & nBit))
		FocpAbort(("CPyramidAllocator::DestroyObject(%X), the object is invalid", pObject));

	m_pClass->ResetObject(pObject);
	bFull = (pLeaf->nMask.nFlag == 0xFFFFFFFF);
	pLeaf->nMask.pFlag[nIdx] &= ~nBit;
	for(uint32 j=0; bFull && j<6; ++j)
	{
		uint32 k = 5 - j;
		CBranchNode* pBranch = (CBranchNode*)pNode[k];
		if(!pBranch)
			break;
		nIdx = pIndex[k]/8;
		nMod = pIndex[k]%8;
		nBit = 1;
		if(nMod)
			nBit <<= nMod;
		bFull = (pBranch->nMask.nFlag == 0xFFFFFFFF);
		pBranch->nMask.pFlag[nIdx] &= ~nBit;
	}
	--m_nSize;

	m_oMutex.Leave();
}

void* CPyramidAllocator::CreateNode(uint32 nLevel)
{
	if(nLevel == 0)
	{
		CLeafNode* pNode = (CLeafNode*)CMalloc::Malloc(sizeof(CLeafNode));
		if(pNode == NULL)
			return NULL;
		pNode->nMask.nFlag = 0;
		pNode->pObjects = (uint8*)CMalloc::Malloc(32*m_nInstanceSize);
		if(pNode->pObjects == NULL)
		{
			CMalloc::Free(pNode);
			return NULL;
		}
		uint8* pBuf = pNode->pObjects;
		for(uint32 i=0; i<32; ++i)
		{
			m_pClass->ConstructInstance(pBuf);
			m_pClass->SetObjectId(pBuf, m_nMaxId);
			pBuf += m_nInstanceSize;
			++m_nMaxId;
		}
		return pNode;
	}
	CBranchNode* pNode = (CBranchNode*)CMalloc::Malloc(sizeof(CBranchNode));
	if(pNode == NULL)
		return NULL;
	pNode->nMask.nFlag = 0;
	for(uint32 i=0; i<32; ++i)
		pNode->pNodes[i] = NULL;
	if(nLevel == 6)
	{
		pNode->nMask.nFlag = 0xFFFFFFFF;
		pNode->nMask.pFlag[0] = 0xFC;
	}
	return pNode;
}

void CPyramidAllocator::DestroyNode(void* pNode, uint32 nLevel)
{
	if(nLevel == 0)
	{
		CLeafNode* pLeaf = (CLeafNode*)pNode;
		uint8* pBuf = pLeaf->pObjects;
		for(uint32 i=0; i<32; ++i)
		{
			m_pClass->DestructInstance(pBuf);
			pBuf += m_nInstanceSize;
		}
		CMalloc::Free(pLeaf->pObjects);
		CMalloc::Free(pLeaf);
	}
	else
	{
		CBranchNode* pBranch = (CBranchNode*)pNode;
		for(uint32 i=0; i<32; ++i)
		{
			if(pBranch->pNodes[i])
				DestroyNode(pBranch->pNodes[i], nLevel - 1);
		}
		CMalloc::Free(pBranch);
	}
}

void CPyramidAllocator::ParseObjectId(uint32 nObjId, uint32 pIndex[7], void* pNode[7])
{
	uint32 nOldObjId = nObjId;
	uint32* pIdx = pIndex + 6, i;
	for(i=0; i<=m_nLevel; ++i)
	{
		*pIdx = nObjId % 32;
		nObjId /= 32;
		--pIdx;
	}
	if(nObjId)
		FocpAbort(("CPyramidAllocator::ParseObjectId(%u), the object is invalid", nOldObjId));
	if(m_nLevel == 6 && pIndex[0] > 1)
		FocpAbort(("CPyramidAllocator::ParseObjectId(%u), the object is invalid", nOldObjId));
	uint32 nIdx = 6 - m_nLevel;
	for(i=0; i<nIdx; ++i)
		pNode[i] = NULL;
	void* pTmp = m_pNode;
	for(++i; i<7; ++i)
	{
		CBranchNode* pBranch = (CBranchNode*)pTmp;
		pTmp = pBranch->pNodes[pIndex[i]];
		pNode[i] = pTmp;
	}
}

bool CPyramidAllocator::FindFreeChild(CMask &nMask, uint32 &nIndex)
{
	uint32 i, j;

	if(nMask.nFlag == 0xFFFFFFFF)
		return false;

	uint8* pFlag = nMask.pFlag;
	for(i=0; i<4; ++i)
	{
		uint8 &nBits = pFlag[i];
		if(nBits != 0xFF)
		{
			uint8 nBit = 1;
			for(j=0; j<8; ++j)
			{
				if(!(nBits & nBit))
					break;
				nBit <<= 1;
			}
			break;
		}
	}

	nIndex = i * 8 + j;

	return true;
}

bool CPyramidAllocator::ApplyNode(uint32 pIndex[7], void* pNode[7])
{
	uint32 i, nStart;
	nStart = 6 - m_nLevel;
	for(i=0; i<nStart; ++i)
	{
		pNode[i] = NULL;
		pIndex[i] = 0xFFFFFFFF;
	}
	void* pTmp = m_pNode;
	for(; i<7; ++i)
	{
		uint32 nIndex;
		pNode[i] = pTmp;
		if(FindFreeChild(*(CMask*)pTmp, nIndex))
		{
			CBranchNode* pBranch = (CBranchNode*)pTmp;
			if(i < 6)
			{
				pTmp = pBranch->pNodes[nIndex];
				if(pTmp == NULL)
				{
					pTmp = pBranch->pNodes[nIndex] = CreateNode(6 - i);
					if(pTmp == NULL)
						return false;
				}
			}
			pIndex[i] = nIndex;
		}
		else
		{
			if(m_nLevel == 6 || i>nStart)
				FocpAbort(("CPyramidAllocator::ApplyNode() failure"));
			void* pNew = CreateNode(6 - i);
			if(pNew == NULL)
				return false;
			CBranchNode* pBranch = (CBranchNode*)CreateNode(7 - i);
			if(pBranch == NULL)
			{
				DestroyNode(pNew, 6-i);
				return false;
			}
			pBranch->pNodes[0] = m_pNode;
			pBranch->pNodes[1] = pNew;
			pBranch->nMask.pFlag[0] |= 1;
			m_pNode = pBranch;
			++m_nLevel;
			pTmp = m_pNode;
			i = 6 - m_nLevel - 1;
		}
	}
	return true;
}

FOCP_DETAIL_BEGIN();

CBufferClass::CBufferClass()
{
	m_nSize = 0;
}

CBufferClass::CBufferClass(const CBufferClass& oSrc)
{
	m_nSize = oSrc.m_nSize;
}

CBufferClass& CBufferClass::operator=(const CBufferClass& oSrc)
{
	if(this != &oSrc)
		m_nSize = oSrc.m_nSize;
	return *this;
}

uint32 CBufferClass::GetInstanceSize() const
{
	return m_nSize;
}

CBufferAllocator::~CBufferAllocator()
{
}

CBufferAllocator::CBufferAllocator(bool bThread)
	:m_oAllocator(&m_oClass, bThread), m_bThread(bThread)
{
	CAllocatePolicy oPolicy = {0, 0xFFFFFFFF, 0xFFFFFFFF};
	m_oAllocator.SetAllocatePolicy(oPolicy);
}

CBufferAllocator::CBufferAllocator(const CBufferAllocator& oSrc)
	:m_oClass(oSrc.m_oClass), m_oAllocator(&m_oClass, oSrc.m_bThread), m_bThread(oSrc.m_bThread)
{
	CAllocatePolicy oPolicy = {0, 0xFFFFFFFF, 0xFFFFFFFF};
	m_oAllocator.SetAllocatePolicy(oPolicy);
}

CBufferAllocator& CBufferAllocator::operator=(const CBufferAllocator& oSrc)
{
	if(this != &oSrc)
	{
		CAllocatePolicy oPolicy = {0, 0xFFFFFFFF, 0xFFFFFFFF};
		m_oClass = oSrc.m_oClass;
		m_oAllocator.~CAllocator();
		new(&m_oAllocator) CAllocator(&m_oClass, oSrc.m_bThread);
		m_bThread = oSrc.m_bThread;
		m_oAllocator.SetAllocatePolicy(oPolicy);
	}
	return *this;
}

void* CBufferAllocator::AllocateBuffer(bool bLock)
{
	return m_oAllocator.CreateObject(bLock);
}

void CBufferAllocator::DeAllocateBuffer(void* pBuffer, bool bLock)
{
	m_oAllocator.DestroyObject(pBuffer, bLock);
}

void CBufferAllocator::SetSize(uint32 nSize)
{
	m_oClass.m_nSize = nSize;
}

const uint32* CGetBufferAllocatorKey::GetKey(const CBufferAllocator& oData)
{
	return &oData.m_oClass.m_nSize;
}

CMemStreamNode* CApuStreamAllocator::Allocate()
{
	return (CMemStreamNode*)CBufferManager::GetInstance()->AllocateBuffer(FOCP_SIZE_OF(CMemStreamNode));
}

void CApuStreamAllocator::DeAllocate(CMemStreamNode* pNode)
{
	CBufferManager::GetInstance()->DeAllocateBuffer(pNode);
}

static CApuStreamAllocator g_oStreamAllocator;
static bool g_bInitializeMemoryStreamAllocator = CMemoryStreamAllocator::SetAllocator(&g_oStreamAllocator);

FOCP_DETAIL_END();

CBufferManager::CBufferManager(bool bThread, uint32 nUnit)
	:m_oMutex(bThread), m_bThread(bThread)
{
	m_nUnit = nUnit;
}

CBufferManager::~CBufferManager()
{
}

void* CBufferManager::AllocateBuffer(uint32 nSize, bool bLock)
{
	if(nSize == 0)
		return NULL;
	uint32 nMod = nSize % sizeof(ulong);
	if(nMod)
		nSize += sizeof(ulong) - nMod;
	nSize += sizeof(ulong);
	nMod = nSize % m_nUnit;
	if(nMod)
		nSize += m_nUnit - nMod;
	if(bLock)
		m_oMutex.Enter();
	CRbTreeNode* pIt = m_oTree.Find(nSize);
	if(pIt == m_oTree.End())
	{
		FOCP_DETAIL_NAME::CBufferAllocator oAllocator(m_bThread);
		oAllocator.SetSize(nSize);
		pIt = m_oTree.Insert(oAllocator);
	}
	if(bLock)
		m_oMutex.Leave();
	FOCP_DETAIL_NAME::CBufferAllocator& oAllocator = (FOCP_DETAIL_NAME::CBufferAllocator&)m_oTree.GetItem(pIt);
	void* pRet = oAllocator.AllocateBuffer();
	if(pRet)
	{
		*(ulong*)pRet = nSize;
		return (uint8*)pRet + sizeof(ulong);
	}
	return NULL;
}

void CBufferManager::DeAllocateBuffer(void* pBuffer, bool bLock)
{
	uint8* pBuf = (uint8*)pBuffer;
	if(pBuf > (uint8*)sizeof(ulong))
	{
		pBuf -= sizeof(ulong);
		uint32 nSize = *(ulong*)pBuf;
		if(bLock)
			m_oMutex.Enter();
		CRbTreeNode* pIt = m_oTree.Find(nSize);
		if(bLock)
			m_oMutex.Leave();
		if(pIt != m_oTree.End())
		{
			FOCP_DETAIL_NAME::CBufferAllocator& oAllocator = (FOCP_DETAIL_NAME::CBufferAllocator&)m_oTree.GetItem(pIt);
			oAllocator.DeAllocateBuffer(pBuf);
		}
	}
}

CBufferManager* CBufferManager::GetInstance()
{
	return CSingleInstance<CBufferManager>::GetInstance();
}

FOCP_END();
