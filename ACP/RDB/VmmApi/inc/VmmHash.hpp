
#include "VmmVao.hpp"

#ifndef _VMM_HASH_HPP_
#define _VMM_HASH_HPP_

FOCP_BEGIN();

struct CVmmHashInfo
{
	uint32 i;
	uint32 n;
	uint32 r;
	uint32 d;
};

template<typename TVirtualObject, uint32 nBlockSize> struct CVmmHashBlock
{
	uint64 nNextBlock;
	uint32 m_nDirty, m_nCounter, nSize;
	struct CHashNode
	{
		TVirtualObject oObject;
		uint32 nHashValue;
	}oNode[nBlockSize];
};

struct CVmmHashBranch
{
	uint64 nBranch[256];
	uint32 m_nCounter, m_nDirty;
	uint32 nSize;
};

template<typename TVirtualObject, uint32 nBlockSize=8> class CVmmHash
{
public:
	typedef CVmmHashBlock<TVirtualObject, nBlockSize> THashBlock;
	typedef bool (*FIsEqual0)(void* pCond, const TVirtualObject& oObject);
	typedef bool (*FIsEqual1)(const TVirtualObject& a, const TVirtualObject& b, void* pContext);
	struct CHashIterator
	{
		uint64 nBlock;
		THashBlock* pBlock;
		uint32 nSub, m;
		CVmmHashInfo* pHashInfo;
	};

private:
	uint64 m_nThis;
	uint32 m_nUnique;
	uint32 m_nBigEndian;
	double m_nHashRate;
	CVmmHashInfo m_oHashInfo;
	int32 m_bInMemory;

	CVirtualAllocator* m_pAllocator;
	CPersistObjectPool<CVmmHashBranch> m_oBranchPool;
	CPersistObjectPool<THashBlock> m_oBlockPool;

public:
	CVmmHash(CVirtualAllocator* pAllocator, uint32 nUnique, double nHashRate=1.0)
	{
		if(nHashRate<=0.0)
			nHashRate = 0.125;
		if(nHashRate > 1.0)
			nHashRate = 1.0;
		m_nHashRate = nHashRate;
		m_nThis = 0;
		m_pAllocator = pAllocator;
		m_nUnique = nUnique;
		m_nBigEndian = IsSmallEndian()?0:1;
		m_oHashInfo.i=1;
		m_oHashInfo.n=2;
		m_oHashInfo.r=0;
		m_oHashInfo.d=1;
		m_bInMemory = 0;
	}

	~CVmmHash()
	{
	}

	uint32 IsUnique()
	{
		return m_nUnique;
	}

	void SetThis(uint64 nThis, CVmmHashInfo& oHashInfo)
	{
		m_nThis = nThis;
		m_oHashInfo = oHashInfo;
		m_bInMemory = IS_MEMORY_ADDR(nThis);
	}

	uint64 GetThis()
	{
		return m_nThis;
	}

	CVmmHashInfo& GetHashInfo()
	{
		return m_oHashInfo;
	}

	THashBlock* GetNode(uint32 nHash, uint64 &nBlock)
	{
		static uint32* pMask = GetMask();
		uint32 m = nHash & pMask[m_oHashInfo.i-1];
		if(m >= m_oHashInfo.n)
			m = (nHash & pMask[m_oHashInfo.i-2]);

		return GetBlock(m, nBlock);
	}

	THashBlock* GetNode(uint32 nHash, uint64 &nBlock, uint32 &m)
	{
		static uint32* pMask = GetMask();
		m = nHash & pMask[m_oHashInfo.i-1];
		if(m >= m_oHashInfo.n)
			m = (nHash & pMask[m_oHashInfo.i-2]);

		return GetBlock(m, nBlock);
	}

	THashBlock* GetBlock(uint32 m, uint64 &nBlock)
	{
		uint8* pIdx = (uint8*)&m;
		if(m_nBigEndian)
			pIdx += 4 - m_oHashInfo.d;

		nBlock = m_nThis;
		CVmmHashBranch* pBranch;
		for(uint32 i=0; nBlock && i<m_oHashInfo.d; ++i)
		{
			uint64 nHandle = nBlock;
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
			else
				pBranch = m_oBranchPool.QueryObject(nHandle);
			nBlock = pBranch->nBranch[pIdx[i]];
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nHandle, pBranch);
		}
	
		if(!nBlock)
			return NULL;

		if(m_bInMemory)
			return GET_MEMORY_OBJECT(THashBlock, nBlock);
		return m_oBlockPool.QueryObject(nBlock);
	}

	THashBlock* GetNextNode(THashBlock* pBlock, uint64 &nBlock)
	{
		nBlock = pBlock->nNextBlock;
		if(!nBlock)
			return NULL;
		if(m_bInMemory)
			return GET_MEMORY_OBJECT(THashBlock, nBlock);
		return m_oBlockPool.QueryObject(nBlock);
	}

	uint32 RemoveNode(uint32 nHash, FIsEqual0 IsEqual, void* pCond)
	{
		static uint32* pMask = GetMask();
		uint32 m = nHash & pMask[m_oHashInfo.i-1];
		if(m >= m_oHashInfo.n)
			m = (nHash & pMask[m_oHashInfo.i-2]);

		uint8* pIdx = (uint8*)&m;
		if(m_nBigEndian)
			pIdx += 4 - m_oHashInfo.d;

		uint32 nRet = RemoveNode(m_nThis, pIdx, 0, nHash, IsEqual, pCond);
		if(nRet)
		{
			m_oHashInfo.r -= nRet;
			if(!m_oHashInfo.r)
			{
				m_oHashInfo.i=1;
				m_oHashInfo.n=2;
				m_oHashInfo.r=0;
				m_oHashInfo.d=1;
			}
		}
		return nRet;
	}

	void RemoveNode(uint32 m, uint64 nBlock, uint32 nSub)
	{
		uint8* pIdx = (uint8*)&m;
		if(m_nBigEndian)
			pIdx += 4 - m_oHashInfo.d;
		if(RemoveNode(m_nThis, pIdx, 0, nBlock, nSub))
		{
			--m_oHashInfo.r;
			if(!m_oHashInfo.r)
			{
				m_oHashInfo.i=1;
				m_oHashInfo.n=2;
				m_oHashInfo.r=0;
				m_oHashInfo.d=1;
			}
		}
	}

	THashBlock* InsertNode(uint32 nHash, const TVirtualObject& oObject, FIsEqual1 IsEqual, 
		void* pContext, int32 bInMemory, uint64& nBlock, uint32 &nSub, bool& bConflict)
	{
		static uint32* pMask = GetMask();

		bConflict = false;
		uint32 m = nHash & pMask[m_oHashInfo.i-1];
		if(m >= m_oHashInfo.n)
				m = (nHash & pMask[m_oHashInfo.i-2]);

		uint8* pIdx = (uint8*)&m;
		if(m_nBigEndian)
			pIdx += 4 - m_oHashInfo.d;

		m_bInMemory = bInMemory;
		THashBlock* pBlock = InsertNode(m_nThis, pIdx, 0, nHash, IsEqual, oObject, pContext, bInMemory, nBlock, nSub, bConflict);
		if(pBlock)
		{
			++m_oHashInfo.r;
			double nRate = ((double)m_oHashInfo.n) / m_oHashInfo.r;
			if(nRate < m_nHashRate)
				ExpandHash(pBlock, nBlock, nSub, bInMemory);
		}

		return pBlock;
	}

	void ReleaseBlock(uint64 nBlock, THashBlock* pBlock)
	{
		if(!m_bInMemory)
			m_oBlockPool.ReleaseObject(nBlock, pBlock);
	}

private:
	uint32 GetHashLevel(uint32 nHashSize)
	{
		--nHashSize;
		uint32 nLevel = 4;
		uint32 nMod = 0xFF000000;
		while(nLevel && !(nMod & nHashSize))
		{
			--nLevel;
			nMod >>= 8;
		}
		return nLevel;
	}

	uint32* GetMask()
	{
		static uint32 x[] = 
		{
			0x1, 0x3, 0x7, 0xF,
			0x1F, 0x3F, 0x7F, 0xFF,
			0x1FF, 0x3FF, 0x7FF, 0xFFF,
			0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
			0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
			0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
			0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
			0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
		};
		return x;
	}

	bool RemoveNode(uint64& nHandle, uint8* pIdx, uint32 nLevel, uint64 nDstBlock, uint32 nSub)
	{
		if(!nHandle)
			return false;
		bool bFound = false;
		bool nRet = false;
		CVmmHashBranch* pBranch;
		if(m_bInMemory)
			pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
		else
			pBranch = m_oBranchPool.QueryObject(nHandle);
		uint64& nHandle2 = pBranch->nBranch[pIdx[nLevel]], nHandle3 = nHandle2;
		if(nLevel < m_oHashInfo.d-1)
			nRet = RemoveNode(nHandle3, pIdx, nLevel+1, nDstBlock, nSub);
		else
		{
			uint64 nPrev = 0;
			THashBlock* pPrev = NULL;
			uint64 nBlock = nHandle3;
			while(nBlock)
			{
				THashBlock* pBlock;
				if(m_bInMemory)
					pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
				else
					pBlock = m_oBlockPool.QueryObject(nBlock);
				uint64 nNext = pBlock->nNextBlock;
				if(nBlock == nDstBlock)
				{
					bFound = true;
					if(pBlock->oNode[nSub].oObject)
					{
						nRet = true;
						--pBlock->nSize;
						pBlock->oNode[nSub].oObject = 0;
					}
				}
				if(pBlock->nSize)
				{
					if(pPrev && !m_bInMemory)
						m_oBlockPool.ReleaseObject(nPrev, pPrev);
					nPrev = nBlock;
					pPrev = pBlock;
				}
				else
				{
					pBlock->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBlockPool.ReleaseObject(nBlock, pBlock);
					m_pAllocator->DeAllocate(nBlock);
					if(pPrev)
					{
						pPrev->nNextBlock = nNext;
						pPrev->m_nDirty = 1;
					}
					else
						nHandle3 = nNext;
				}
				if(bFound)
					break;
				nBlock = nNext;
			}
			if(pPrev && !m_bInMemory)
				m_oBlockPool.ReleaseObject(nPrev, pPrev);
		}
		if(nHandle2 != nHandle3)
		{
			nHandle2 = nHandle3;
			pBranch->m_nDirty = 1;
			if(!nHandle3)
			{
				--pBranch->nSize;
				if(!pBranch->nSize)
				{
					pBranch->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					m_pAllocator->DeAllocate(nHandle);
					nHandle = 0;
					return nRet;
				}
			}
		}
		if(!m_bInMemory)
			m_oBranchPool.ReleaseObject(nHandle, pBranch);
		return nRet;
	}

	uint32 RemoveNode(uint64& nHandle, uint8* pIdx, uint32 nLevel, uint32 nHash, FIsEqual0 IsEqual, void* pCond)
	{
		if(!nHandle)
			return 0;
		uint32 nRet = 0;
		CVmmHashBranch* pBranch;
		if(m_bInMemory)
			pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
		else
			pBranch = m_oBranchPool.QueryObject(nHandle);
		uint64& nHandle2 = pBranch->nBranch[pIdx[nLevel]], nHandle3 = nHandle2;
		if(nLevel < m_oHashInfo.d-1)
			nRet = RemoveNode(nHandle3, pIdx, nLevel+1, nHash, IsEqual, pCond);
		else
		{
			uint64 nPrev = 0;
			THashBlock* pPrev = NULL;
			uint64 nBlock = nHandle3;
			while(nBlock)
			{
				THashBlock* pBlock;
				if(m_bInMemory)
					pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
				else
					pBlock = m_oBlockPool.QueryObject(nBlock);
				uint64 nNext = pBlock->nNextBlock;
				for(uint32 i=0; i<nBlockSize; ++i)
				{
					if(pBlock->oNode[i].oObject &&
						pBlock->oNode[i].nHashValue == nHash &&
						IsEqual(pCond, pBlock->oNode[i].oObject))
					{
						++nRet;
						--pBlock->nSize;
						pBlock->oNode[i].oObject = 0;
						if(m_nUnique || !pBlock->nSize)
							break;
					}
				}
				if(pBlock->nSize)
				{
					if(pPrev && !m_bInMemory)
						m_oBlockPool.ReleaseObject(nPrev, pPrev);
					nPrev = nBlock;
					pPrev = pBlock;
				}
				else
				{
					pBlock->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBlockPool.ReleaseObject(nBlock, pBlock);
					m_pAllocator->DeAllocate(nBlock);
					if(pPrev)
					{
						pPrev->nNextBlock = nNext;
						pPrev->m_nDirty = 1;
					}
					else
						nHandle3 = nNext;
				}
				if(nRet && m_nUnique)
					break;
				nBlock = nNext;
			}
			if(pPrev && !m_bInMemory)
				m_oBlockPool.ReleaseObject(nPrev, pPrev);
		}
		if(nHandle2 != nHandle3)
		{
			nHandle2 = nHandle3;
			pBranch->m_nDirty = 1;
			if(!nHandle3)
			{
				--pBranch->nSize;
				if(!pBranch->nSize)
				{
					pBranch->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					m_pAllocator->DeAllocate(nHandle);
					nHandle = 0;
					return nRet;
				}
			}
		}
		if(!m_bInMemory)
			m_oBranchPool.ReleaseObject(nHandle, pBranch);
		return nRet;
	}

	THashBlock* InsertNode(uint64& nHandle, uint8* pIdx, uint32 nLevel, uint32 nHash, FIsEqual1 IsEqual, 
		const TVirtualObject& oObject, void* pContext, int32 bInMemory, uint64 &nRet, uint32& nSub, bool& bConflict)
	{
		uint32 i;
		CVmmHashBranch* pBranch = NULL;
		THashBlock* pRet = NULL;
		if(nHandle)
		{
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
			else
				pBranch = m_oBranchPool.QueryObject(nHandle);
			uint64& nHandle2 = pBranch->nBranch[pIdx[nLevel]], nHandle3 = nHandle2;
			if(nLevel < m_oHashInfo.d-1)
				pRet = InsertNode(nHandle3, pIdx, nLevel+1, nHash, IsEqual, oObject, pContext, bInMemory, nRet, nSub, bConflict);
			else
			{
				uint64 nBlock = 0;
				if(m_nUnique)
				{
					nBlock = nHandle3;
					while(nBlock)
					{
						THashBlock* pBlock;
						if(m_bInMemory)
							pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
						else
							pBlock = m_oBlockPool.QueryObject(nBlock);
						for(i=0; i<nBlockSize; ++i)
						{
							if(pBlock->oNode[i].oObject &&
								pBlock->oNode[i].nHashValue == nHash &&
								IsEqual(oObject, pBlock->oNode[i].oObject, pContext))
							{
								bConflict = true;
								if(!m_bInMemory)
								{
									m_oBlockPool.ReleaseObject(nBlock, pBlock);
									m_oBranchPool.ReleaseObject(nHandle, pBranch);
								}
								return NULL;
							}
						}
						uint64 nNext = pBlock->nNextBlock;
						if(!m_bInMemory)
							m_oBlockPool.ReleaseObject(nBlock, pBlock);
						nBlock = nNext;
					}
				}
				nBlock = nHandle3;
				while(nBlock)
				{
					THashBlock* pBlock;
					if(m_bInMemory)
						pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
					else
						pBlock = m_oBlockPool.QueryObject(nBlock);
					if(pBlock->nSize < nBlockSize)for(i=0; i<nBlockSize; ++i)
					{
						if(pBlock->oNode[i].oObject)
							continue;
						pBlock->oNode[i].oObject = oObject;
						pBlock->oNode[i].nHashValue = nHash;
						++pBlock->nSize;
						nRet = nBlock;
						nSub = i;
						if(!m_bInMemory)
							m_oBranchPool.ReleaseObject(nHandle, pBranch);
						return pBlock;
					}
					uint64 nNext = pBlock->nNextBlock;
					if(!m_bInMemory)
						m_oBlockPool.ReleaseObject(nBlock, pBlock);
					nBlock = nNext;
				}
				nRet = m_pAllocator->Allocate(sizeof(THashBlock), bInMemory);
				if(!nRet)
				{
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					return NULL;
				}
				if(m_bInMemory)
					pRet = GET_MEMORY_OBJECT(THashBlock, nRet);
				else
					pRet = m_oBlockPool.QueryObject(nRet, true);
				for(i=1; i<nBlockSize; ++i)
					pRet->oNode[i].oObject = 0;
				pRet->oNode[0].oObject = oObject;
				pRet->oNode[0].nHashValue = nHash;
				pRet->nNextBlock = nHandle3;
				pRet->nSize = 1;
				nHandle3 = nRet;
				nSub = 0;
			}
			if(nHandle2 != nHandle3)
			{
				if(!nHandle2)
					++pBranch->nSize;
				nHandle2 = nHandle3;
				pBranch->m_nDirty = 1;
			}
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nHandle, pBranch);
		}
		else
		{
			nHandle = m_pAllocator->Allocate(sizeof(CVmmHashBranch), bInMemory);
			if(!nHandle)
				return NULL;
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
			else
				pBranch = m_oBranchPool.QueryObject(nHandle, true);
			for(i=0; i<256; ++i)
				pBranch->nBranch[i] = 0;
			pBranch->nSize = 0;
			uint64& nHandle2 = pBranch->nBranch[pIdx[nLevel]];
			if(nLevel < m_oHashInfo.d-1)
			{
				pRet = InsertNode(nHandle2, pIdx, nLevel+1, nHash, IsEqual, oObject, pContext, bInMemory, nRet, nSub, bConflict);
				if(pRet)
				{
					++pBranch->nSize;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
				}
				else
				{
					pBranch->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					m_pAllocator->DeAllocate(nHandle);
					nHandle = 0;
				}
			}
			else
			{
				nRet = m_pAllocator->Allocate(sizeof(THashBlock), bInMemory);
				if(nRet)
				{
					if(m_bInMemory)
						pRet = GET_MEMORY_OBJECT(THashBlock, nRet);
					else
						pRet = m_oBlockPool.QueryObject(nRet, true);
					for(i=1; i<nBlockSize; ++i)
						pRet->oNode[i].oObject = 0;
					pRet->oNode[0].oObject = oObject;
					pRet->oNode[0].nHashValue = nHash;
					pRet->nNextBlock = nHandle2;
					pRet->nSize = 1;
					nHandle2 = nRet;
					nSub = 0;
					++pBranch->nSize;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
				}
				else
				{
					pBranch->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					m_pAllocator->DeAllocate(nHandle);
					nHandle = 0;
				}
			}
		}
		return pRet;
	}

	void ExpandHash(THashBlock* &pBlock, uint64& nBlock, uint32& nSub, int32 bInMemory)
	{
		static uint32* pMask = GetMask();

		uint32 d, n1, n2;
		uint64 nNewBlock=0, nNewBranch=0;
		CHashIterator oOldIt = {nBlock, pBlock, nSub, 0, NULL};
		CHashIterator oNewIt = {0, NULL, 0, 0, NULL};
		CVmmHashInfo oNewHashInfo = m_oHashInfo;
		n1 = oNewHashInfo.n;
		if(n1 > pMask[oNewHashInfo.i-1])
			++oNewHashInfo.i;
		n2 = n1 & pMask[oNewHashInfo.i-2];
		oNewHashInfo.n = n1 + 1;
		oNewHashInfo.d = GetHashLevel(oNewHashInfo.n);

		if(CopyBucket(n1, n2, bInMemory, nNewBlock, oNewHashInfo, oOldIt, oNewIt) ||
			AddBucket(n1, bInMemory, nNewBlock, nNewBranch, d, oNewHashInfo))
		{
			if(nNewBranch)
				RemoveBranch(nNewBranch, oNewHashInfo.d - d);
			if(nNewBlock)
			{
				if(oNewIt.nBlock && !m_bInMemory)
					m_oBlockPool.ReleaseObject(oNewIt.nBlock, oNewIt.pBlock);
				RemoveBucket(nNewBlock);
			}
		}
		else
		{
			m_oHashInfo = oNewHashInfo;
			if(nNewBranch)
				m_nThis = nNewBranch;
			if(oNewIt.nBlock)
			{
				if(!m_bInMemory)
					m_oBlockPool.ReleaseObject(nBlock, pBlock);
				pBlock = oNewIt.pBlock;
				nBlock = oNewIt.nBlock;
				nSub = oNewIt.nSub;
			}
			if(nNewBlock)
			{
				uint8* pIdx = (uint8*)&n2;
				if(m_nBigEndian)
					pIdx += 4 - m_oHashInfo.d;
				DetachBucket(m_nThis, pIdx, 0, n1);
			}
		}
	}

	uint32 CopyBucket(uint32 nDest, uint32 nSrc, int32 bInMemory, uint64 &nBlock, CVmmHashInfo& oNewHashInfo, 
		CHashIterator& oOldIt, CHashIterator& oNewIt)
	{
		static uint32* pMask = GetMask();

		uint64 nSrcBlock;

		nBlock = 0;
		THashBlock* pSrcBlock = GetBlock(nSrc, nSrcBlock);
		if(!pSrcBlock)
			return 0;
		
		uint32 i, j = nBlockSize;
		THashBlock* pBlock = NULL;
		while(pSrcBlock)
		{
			for(i=0; i<nBlockSize; ++i)
			{
				if(pSrcBlock->oNode[i].oObject)
				{
					uint32 nHash = pSrcBlock->oNode[i].nHashValue;
					if(nDest == (nHash & pMask[oNewHashInfo.i-1]))
					{
						if(j >= nBlockSize)
						{
							uint64 nNewBlock = m_pAllocator->Allocate(sizeof(THashBlock), bInMemory);
							if(!nNewBlock)
							{
								if(!m_bInMemory)
								{
									m_oBlockPool.ReleaseObject(nSrcBlock, pSrcBlock);
									if(nBlock)
										m_oBlockPool.ReleaseObject(nBlock, pBlock);
								}
								return 1;
							}
							THashBlock* pNewBlock;
							if(m_bInMemory)
								pNewBlock = GET_MEMORY_OBJECT(THashBlock, nNewBlock);
							else
								pNewBlock = m_oBlockPool.QueryObject(nNewBlock, true);
							for(j=0; j<nBlockSize; ++j)
								pNewBlock->oNode[j].oObject = 0;
							pNewBlock->nSize = 0;
							pNewBlock->nNextBlock = nBlock;
							if(nBlock && !m_bInMemory)
								m_oBlockPool.ReleaseObject(nBlock, pBlock);
							nBlock = nNewBlock;
							pBlock = pNewBlock;
							j = 0;
						}
						++pBlock->nSize;
						pBlock->oNode[j].oObject = pSrcBlock->oNode[i].oObject;
						pBlock->oNode[j].nHashValue = nHash;
						if(pSrcBlock == oOldIt.pBlock && i == oOldIt.nSub)
						{
							oNewIt.nSub = j;
							oNewIt.nBlock = nBlock;
							oNewIt.pBlock = pBlock;
							++pBlock->m_nCounter;
						}
						++j;
					}
				}
			}
			uint64 nNext = pSrcBlock->nNextBlock;
			if(!m_bInMemory)
				m_oBlockPool.ReleaseObject(nSrcBlock, pSrcBlock);
			pSrcBlock = NULL;
			if(nNext)
			{
				if(m_bInMemory)
					pSrcBlock = GET_MEMORY_OBJECT(THashBlock, nNext);
				else
					pSrcBlock = m_oBlockPool.QueryObject(nNext);
				nSrcBlock = nNext;
			}
		}
		if(nBlock && !m_bInMemory)
			m_oBlockPool.ReleaseObject(nBlock, pBlock);
		return 0;
	}

	uint32 AddBucket(uint32 nDest, int32 bInMemory, uint64 nBlock, uint64& nBranch, uint32& d, CVmmHashInfo& oNewHashInfo)
	{
		nBranch = 0;
		CVmmHashBranch* pBranch;
		uint32 i, n = oNewHashInfo.n;
		for(d = oNewHashInfo.d; (1 == (n & 0xFF)); --d)
		{
			nBranch = m_pAllocator->Allocate(sizeof(CVmmHashBranch), bInMemory);
			if(!nBranch)
				return 1;
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch, true);
			for(i=1; i<256; ++i)
				pBranch->nBranch[i] = 0;
			pBranch->nSize = 1;
			pBranch->nBranch[0] = nBlock;
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nBranch, pBranch);
			nBlock = nBranch;
			n >>= 8;
		}
		if(n)
		{
			uint32 m = oNewHashInfo.n - 1;
			uint8* pIdx = (uint8*)&m;
			if(m_nBigEndian)
				pIdx += 4 - oNewHashInfo.d;
			nBranch = m_nThis;
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch);
			for(i=0; i<d-1; ++i)
			{
				uint64 nHandle = nBranch;
				nBranch = pBranch->nBranch[pIdx[i]];
				if(m_bInMemory)
					pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nBranch);
				else
				{
					m_oBranchPool.ReleaseObject(nHandle, pBranch);
					pBranch = m_oBranchPool.QueryObject(nBranch);
				}
			}
			++pBranch->nSize;
			pBranch->m_nDirty = 1;
			pBranch->nBranch[pIdx[i]] = nBlock;
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nBranch, pBranch);
			nBranch = 0;
		}
		else
		{
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch);
			++pBranch->nSize;
			pBranch->m_nDirty = 1;
			uint64 nHandle = pBranch->nBranch[0];
			pBranch->nBranch[0] = m_nThis;
			pBranch->nBranch[1] = nHandle;
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nBranch, pBranch);
		}
		return 0;
	}

	void RemoveBranch(uint64 nBranch, uint32 d)
	{
		CVmmHashBranch* pBranch;
		for(uint32 i=0; i<d && nBranch; ++i)
		{
			if(m_bInMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch);
			uint64 nNext = pBranch->nBranch[0];
			pBranch->m_nDirty = 0;
			if(!m_bInMemory)
				m_oBranchPool.ReleaseObject(nBranch, pBranch);
			m_pAllocator->DeAllocate(nBranch);
			nBranch = nNext;
		}
	}

	void RemoveBucket(uint64 nBlock)
	{
		THashBlock* pBlock;
		while(nBlock)
		{
			if(m_bInMemory)
				pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
			else
				pBlock = m_oBlockPool.QueryObject(nBlock);
			uint64 nNext = pBlock->nNextBlock;
			pBlock->m_nDirty = 0;
			if(!m_bInMemory)
				m_oBlockPool.ReleaseObject(nBlock, pBlock);
			m_pAllocator->DeAllocate(nBlock);
			nBlock = nNext;
		}
	}

	void DetachBucket(uint64& nHandle, uint8* pIdx, uint32 nLevel, uint32 nDst)
	{
		if(!nHandle)
			return;
		static uint32* pMask = GetMask();
		CVmmHashBranch* pBranch;
		if(m_bInMemory)
			pBranch = GET_MEMORY_OBJECT(CVmmHashBranch, nHandle);
		else
			pBranch = m_oBranchPool.QueryObject(nHandle);
		uint64& nHandle2 = pBranch->nBranch[pIdx[nLevel]], nHandle3 = nHandle2;
		if(nLevel < m_oHashInfo.d-1)
			DetachBucket(nHandle3, pIdx, nLevel+1, nDst);
		else
		{
			uint64 nPrev = 0;
			THashBlock* pPrev = NULL;
			uint64 nBlock = nHandle3;
			while(nBlock)
			{
				THashBlock* pBlock;
				if(m_bInMemory)
					pBlock = GET_MEMORY_OBJECT(THashBlock, nBlock);
				else
					pBlock = m_oBlockPool.QueryObject(nBlock);
				uint64 nNext = pBlock->nNextBlock;
				for(uint32 i=0; i<nBlockSize; ++i)
				{
					if(pBlock->oNode[i].oObject &&
						(pBlock->oNode[i].nHashValue & pMask[m_oHashInfo.i-1]) == nDst)
					{
						--pBlock->nSize;
						pBlock->oNode[i].oObject = 0;
						if(!pBlock->nSize)
							break;
					}
				}
				if(pBlock->nSize)
				{
					if(pPrev && !m_bInMemory)
						m_oBlockPool.ReleaseObject(nPrev, pPrev);
					nPrev = nBlock;
					pPrev = pBlock;
				}
				else
				{
					pBlock->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBlockPool.ReleaseObject(nBlock, pBlock);
					m_pAllocator->DeAllocate(nBlock);
					if(pPrev)
					{
						pPrev->nNextBlock = nNext;
						pPrev->m_nDirty = 1;
					}
					else
						nHandle3 = nNext;
				}
				nBlock = nNext;
			}
			if(pPrev && !m_bInMemory)
				m_oBlockPool.ReleaseObject(nPrev, pPrev);
		}
		if(nHandle2 != nHandle3)
		{
			nHandle2 = nHandle3;
			pBranch->m_nDirty = 1;
			if(!nHandle3)
			{
				--pBranch->nSize;
				if(!pBranch->nSize)
				{
					pBranch->m_nDirty = 0;
					if(!m_bInMemory)
						m_oBranchPool.ReleaseObject(nHandle, pBranch);
					m_pAllocator->DeAllocate(nHandle);
					nHandle = 0;
					return;
				}
			}
		}
		if(!m_bInMemory)
			m_oBranchPool.ReleaseObject(nHandle, pBranch);
	}
};

FOCP_END();

#endif
