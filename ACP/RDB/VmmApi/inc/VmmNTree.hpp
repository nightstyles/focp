
#include "VmmVao.hpp"

#ifndef _VMM_NTREE_HPP_
#define _VMM_NTREE_HPP_

FOCP_BEGIN();

template<typename TVirtualObject> struct CVmmNTreeNode
{
	uint64 nNextNode;
	uint32 m_nDirty, m_nCounter;
	TVirtualObject oObject;
};

struct CVmmNTreeBranch
{
	uint64 nBranch[256];
	uint64 nNode[256];
	uint32 m_nCounter, m_nDirty;
	uint32 nSize;
};

template<typename TVirtualObject> class CVmmNTree
{
public:
	typedef CVmmNTreeNode<TVirtualObject> TNTreeNode;
	typedef bool (*FIsEqual0)(void* pCond, const TVirtualObject& oObject);

private:
	uint64 m_nThis;
	uint32 m_nUnique;
	CVirtualAllocator* m_pAllocator;
	CPersistObjectPool<CVmmNTreeBranch> m_oBranchPool;
	CPersistObjectPool<TNTreeNode> m_oNodePool;
	int32 m_bMemory;

public:
	CVmmNTree(CVirtualAllocator* pAllocator, uint32 nUnique)
	{
		m_nThis = 0;
		m_pAllocator = pAllocator;
		m_nUnique = nUnique;
		m_bMemory = 0;
	}

	~CVmmNTree()
	{
	}

	void SetThis(uint64 nThis)
	{
		m_nThis = nThis;
		m_bMemory = IS_MEMORY_ADDR(nThis);
	}

	uint64 GetThis()
	{
		return m_nThis;
	}

	TNTreeNode* GetNode(const uint8* pStr, uint32 nStrLen, uint64 &nList)
	{
		if(!m_nThis || !pStr || !pStr[0])
			return NULL;

		nList = Get(m_nThis, pStr, nStrLen);

		if(!nList)
			return NULL;

		if(m_bMemory)
			return GET_MEMORY_OBJECT(TNTreeNode, nList);
		return m_oNodePool.QueryObject(nList);
	}

	TNTreeNode* GetNextNode(TNTreeNode* pNode, uint64 &nList)
	{
		nList = pNode->nNextNode;
		if(nList)
		{
			if(m_bMemory)
				return GET_MEMORY_OBJECT(TNTreeNode, nList);
			return m_oNodePool.QueryObject(nList);
		}
		return NULL;
	}

	void ReleaseNode(uint64 nNode, TNTreeNode* pNode)
	{
		if(!m_bMemory)
			m_oNodePool.ReleaseObject(nNode, pNode);
	}

	uint32 RemoveNode(const uint8* pStr, uint32 nStrLen, FIsEqual0 IsEqual, void* pCond)
	{
		bool bFound = false;
		if(!m_nThis || !pStr || !pStr[0])
			return 0;
		return RemoveNode(m_nThis, pStr, nStrLen, IsEqual, pCond, bFound);
	}

	TNTreeNode* InsertNode(const uint8* pStr, uint32 nStrLen, const TVirtualObject& oObject, int32 bInMemory, uint64& nNode, bool& bConflict)
	{
		bConflict = false;
		if(!pStr || !pStr[0])
			return NULL;
		m_bMemory = bInMemory;
		return InsertNode(m_nThis, pStr, nStrLen, oObject, bInMemory, nNode, bConflict);
	}

private:
	TNTreeNode* InsertNode(uint64& nBranch, const uint8* pStr, uint32 nStrLen, const TVirtualObject& oObject, int32 bInMemory, uint64& nNode, bool& bConflict)
	{
		CVmmNTreeBranch* pBranch;
		TNTreeNode* pRet = NULL;
		if(nBranch)
		{
			if(m_bMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmNTreeBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch);
		}
		else
		{
			nBranch = m_pAllocator->Allocate(sizeof(CVmmNTreeBranch), bInMemory);
			if(!nBranch)
				return NULL;
			if(m_bMemory)
				pBranch = GET_MEMORY_OBJECT(CVmmNTreeBranch, nBranch);
			else
				pBranch = m_oBranchPool.QueryObject(nBranch, true);
			for(uint32 i=0; i<256; ++i)
				pBranch->nBranch[i] = pBranch->nNode[i] = 0;
			pBranch->nSize = 0;
		}
		if(pStr[1] && nStrLen)
		{
			--nStrLen;
			uint64 nBranch2 = pBranch->nBranch[pStr[0]];
			pRet = InsertNode(nBranch2, pStr+1, nStrLen, oObject, bInMemory, nNode, bConflict);
			if(nBranch2 != pBranch->nBranch[pStr[0]])
			{
				if(nBranch2)
					++pBranch->nSize;
				pBranch->nBranch[pStr[0]] = nBranch2;
				pBranch->m_nDirty = 1;
			}
		}
		else
		{
			uint64 nHead = pBranch->nBranch[pStr[0]];
			if(nHead && m_nUnique)
				bConflict = true;
			else
			{
				nNode = m_pAllocator->Allocate(sizeof(TNTreeNode), bInMemory);
				if(nNode)
				{
					if(!nHead)
						++pBranch->nSize;
					if(m_bMemory)
						pRet = GET_MEMORY_OBJECT(TNTreeNode, nNode);
					else
						pRet = m_oNodePool.QueryObject(nNode, true);
					pRet->oObject = oObject;
					pRet->nNextNode = nHead;
					pBranch->nBranch[pStr[0]] = nNode;
					pBranch->m_nDirty = 1;
				}
			}
		}
		bool bDelete = false;
		if(!pBranch->nSize)
		{
			bDelete = true;
			pBranch->m_nDirty = 0;
		}
		if(!m_bMemory)
			m_oBranchPool.ReleaseObject(nBranch, pBranch);
		if(bDelete)
		{
			m_pAllocator->DeAllocate(nBranch);
			nBranch = 0;
		}
		return pRet;
	}

	uint32 RemoveNode(uint64& nBranch, const uint8* pStr, uint32 nStrLen, FIsEqual0 IsEqual, void* pCond, bool& bFound)
	{
		uint32 nRet = 0;
		if(!nBranch)
			return nRet;
		bool bDelete = false;
		CVmmNTreeBranch* pBranch;
		if(m_bMemory)
			pBranch = GET_MEMORY_OBJECT(CVmmNTreeBranch, nBranch);
		else
			pBranch = m_oBranchPool.QueryObject(nBranch);
		if(pStr[1] && nStrLen)
		{
			uint64 nBranch2 = pBranch->nBranch[pStr[0]];
			if(nBranch2)
			{
				--nStrLen;
				nRet = RemoveNode(nBranch2, pStr+1, nStrLen, IsEqual, pCond, bFound);
				if(nBranch2 != pBranch->nBranch[pStr[0]])
				{
					pBranch->nBranch[pStr[0]] = nBranch2;
					pBranch->m_nDirty = 1;
					if(!nBranch2 && !pBranch->nNode[pStr[0]])
						bDelete = true;
				}
			}
		}
		if(!bFound)
		{
			uint64 nNode = pBranch->nNode[pStr[0]];
			if(nNode)
			{
				uint64 nPrev = 0;
				bFound = true;
				do
				{
					TNTreeNode* pNode;
					if(m_bMemory)
						pNode = GET_MEMORY_OBJECT(TNTreeNode, nNode);
					else
						pNode = m_oNodePool.QueryObject(nNode);
					uint64 nNext = pNode->nNextNode;
					if(IsEqual(pCond, pNode->oObject))
					{
						++nRet;
						pNode->m_nDirty = 0;
						if(!m_bMemory)
							m_oNodePool.ReleaseObject(nNode, pNode);
						m_pAllocator->DeAllocate(nNode);
						if(nPrev)
						{
							if(m_bMemory)
								pNode = GET_MEMORY_OBJECT(TNTreeNode, nPrev);
							else
								pNode = m_oNodePool.QueryObject(nPrev);
							pNode->nNextNode = nNext;
							pNode->m_nDirty = 1;
							if(!m_bMemory)
								m_oNodePool.ReleaseObject(nPrev, pNode);
						}
						else
						{
							pBranch->nNode[pStr[0]] = nNext;
							pBranch->m_nDirty = 1;
							if(!nNext && !pBranch->nBranch[pStr[0]])
								bDelete = true;
						}
						break;
					}
					else
					{
						nPrev = nNode;
						if(!m_bMemory)
							m_oNodePool.ReleaseObject(nNode, pNode);
					}
					nNode = nNext;
				}while(nNode);
			}
		}
		if(bDelete)
		{
			--pBranch->nSize;
			if(pBranch->nSize)
				bDelete = false;
		}
		if(bDelete)
			pBranch->m_nDirty = 0;
		if(!m_bMemory)
			m_oBranchPool.ReleaseObject(nBranch, pBranch);
		if(bDelete)
		{
			m_pAllocator->DeAllocate(nBranch);
			nBranch = 0;
		}
		return nRet;
	}

	uint64 Get(uint64 nBranch, const uint8* pStr, uint32 nStrLen)
	{
		uint64 nList = 0;
		CVmmNTreeBranch* pBranch;
		if(m_bMemory)
			pBranch = GET_MEMORY_OBJECT(CVmmNTreeBranch, nBranch);
		else
			pBranch = m_oBranchPool.QueryObject(nBranch);
		if(pStr[1] && nStrLen)
		{
			--nStrLen;
			uint64 nBranch2 = pBranch->nBranch[pStr[0]];
			if(nBranch2)
				nList = Get(nBranch2, pStr+1, nStrLen);
		}
		if(!nList)
			nList = pBranch->nNode[pStr[0]];
		if(!m_bMemory)
			m_oBranchPool.ReleaseObject(nBranch, pBranch);
		return nList;
	}
};

FOCP_END();

#endif
