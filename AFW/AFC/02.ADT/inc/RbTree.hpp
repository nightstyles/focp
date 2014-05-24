
#include "Arithmetic.hpp"
#include "List.hpp"

#ifndef _ADT_RBTREE_HPP_
#define _ADT_RBTREE_HPP_

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

template<typename TData> struct TRbTreeNode
{
	CRbTreeNode oNode;
	TData oData;
};

template<typename TKey, typename TData, typename TGetKey> struct CQueryKey
{
	inline static const TKey* QueryKey(const CRbTreeNode* pNode)
	{
		return TGetKey::GetKey(((TRbTreeNode<TData>*)pNode)->oData);
	}
};

FOCP_DETAIL_END();

template
<
	typename TKey,
	typename TQueryKey,//根据节点获得TKey
	typename TCompareKey=CCompareKey<TKey>
> class CBaseRbTree
{
private:
	uint32 m_nSize;
	CRbTreeNode m_oHeader;
	bool m_bUnique;

	CBaseRbTree(const CBaseRbTree<TKey, TQueryKey, TCompareKey> &oSrc);
	CBaseRbTree<TKey, TQueryKey, TCompareKey>& operator=(const CBaseRbTree<TKey, TQueryKey, TCompareKey>& oSrc);

public:
	inline ~CBaseRbTree()
	{
		Clear();
	}

	inline CBaseRbTree(bool bUnique=true)
	{
		m_bUnique = bUnique;
		Clear();
	}

	inline void Clear()
	{
		m_oHeader.bIsBlack = false;
		m_oHeader.pParent = NULL;
		m_oHeader.pLeft = &m_oHeader;
		m_oHeader.pRight = &m_oHeader;
		m_nSize = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nSize;
	}

	inline bool Unique() const
	{
		return m_bUnique;
	}

	inline CRbTreeNode* First() const
	{
		return (CRbTreeNode*)m_oHeader.pLeft;
	}

	inline CRbTreeNode* Last() const
	{
		return (CRbTreeNode*)m_oHeader.pRight;
	}

	inline CRbTreeNode* End() const
	{
		return (CRbTreeNode*)&m_oHeader;
	}

	inline CRbTreeNode* GetNext(const CRbTreeNode* pIt) const
	{
		if (pIt->pRight)
		{
			pIt = pIt->pRight;
			while (pIt->pLeft)
				pIt = pIt->pLeft;
		}
		else
		{
			const CRbTreeNode* y = pIt->pParent;
			while (pIt == y->pRight)
			{
				pIt = y;
				y = y->pParent;
			}
			if (pIt->pRight != y)
				pIt = y;
		}
		return (CRbTreeNode*)pIt;
	}

	inline CRbTreeNode* GetPrev(const CRbTreeNode* pIt) const
	{
		if (pIt->bIsBlack == false && pIt->pParent->pParent == pIt)
			pIt = pIt->pRight;
		else if (pIt->pLeft)
		{
			const CRbTreeNode* y = pIt->pLeft;
			while (y->pRight)
				y = y->pRight;
			pIt = y;
		}
		else
		{
			const CRbTreeNode* y = pIt->pParent;
			while (pIt == y->pLeft)
			{
				pIt = y;
				y = y->pParent;
			}
			pIt = y;
		}
		return (CRbTreeNode*)pIt;
	}

	inline CRbTreeNode* UpperBound(const TKey &oKey) const
	{
		const CRbTreeNode* y = &m_oHeader;
		const CRbTreeNode* x = y->pParent;

		while (x)
		{
			if (Less(&oKey, TQueryKey::QueryKey(x)))
				y = x, x = x->pLeft;
			else
				x = x->pRight;
		}

		return (CRbTreeNode*)y;
	}

	inline CRbTreeNode* LowerBound(const TKey &oKey) const
	{
		const CRbTreeNode* y = &m_oHeader;
		const CRbTreeNode* x = y->pParent;

		while (x)
		{
			if (!Less(TQueryKey::QueryKey(x), &oKey))
				y = x, x = x->pLeft;
			else
				x = x->pRight;
		}

		return (CRbTreeNode*)y;
	}

	inline CRbTreeNode* Find(const TKey &oKey) const
	{
		const CRbTreeNode* y = LowerBound(oKey);
		const CRbTreeNode* pEnd = &m_oHeader;
		if( y != pEnd && Less(&oKey, TQueryKey::QueryKey(y)))
			y = pEnd;
		return (CRbTreeNode*)y;
	}

	inline CRbTreeNode* Insert(CRbTreeNode* pNode)
	{
		CRbTreeNode* y = &m_oHeader;
		CRbTreeNode* x = y->pParent;
		bool bLess = true;
		const TKey* pKey = TQueryKey::QueryKey(pNode);
		while (x)
		{
			y = x;
			bLess = Less(pKey, TQueryKey::QueryKey(x));
			x = bLess ? x->pLeft : x->pRight;
		}
		if(m_bUnique == false)
		{
			InsertNode(x, y, pNode);
			return pNode;
		}
		CRbTreeNode* j = y;
		if (bLess)
		{
			if (j == First())
			{
				InsertNode(x, y, pNode);
				return pNode;
			}
			else
				j = GetPrev(j);
		}
		if (Less(TQueryKey::QueryKey(j), pKey))
		{
			InsertNode(x, y, pNode);
			return pNode;
		}
		return &m_oHeader;
	}

	inline CRbTreeNode* Remove(CRbTreeNode* pNode)
	{
		CRbTreeNode* pNext = &m_oHeader;
		if(pNode != &m_oHeader)
		{
			pNext = GetNext(pNode);
			EraseReBalance(pNode, m_oHeader.pParent, m_oHeader.pLeft, m_oHeader.pRight);
			--m_nSize;
		}
		return pNext;
	}

	inline void MoveFrom(CBaseRbTree<TKey, TQueryKey, TCompareKey> &oSrc)
	{
		if(this != &oSrc)
		{
			CBaseRbTree<TKey, TQueryKey, TCompareKey> oTmp(m_bUnique);
			MoveSubTree(&oTmp, oSrc.m_oHeader.pParent);
			oSrc.Clear();
			oSrc.Swap(oTmp);
		}
	}

	inline void Swap(CBaseRbTree<TKey, TQueryKey, TCompareKey> &oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
			::FOCP_NAME::Swap(m_oHeader, oSrc.m_oHeader);
			::FOCP_NAME::Swap(m_bUnique, oSrc.m_bUnique);
			if(m_oHeader.pParent == &oSrc.m_oHeader)
				::FOCP_NAME::Swap(m_oHeader.pParent, oSrc.m_oHeader.pParent);
			if(m_oHeader.pLeft == &oSrc.m_oHeader)
				::FOCP_NAME::Swap(m_oHeader.pLeft, oSrc.m_oHeader.pLeft);
			if(m_oHeader.pRight == &oSrc.m_oHeader)
				::FOCP_NAME::Swap(m_oHeader.pRight, oSrc.m_oHeader.pLeft);
		}
	}

private:
	inline static bool Less(const TKey* pLeft, const TKey* pRight)
	{
		return TCompareKey::Compare(pLeft, pRight)<0;
	}

	inline static CRbTreeNode* GetMinRbNode(CRbTreeNode* x)
	{
		while (x->pLeft)
			x = x->pLeft;
		return x;
	}

	inline static CRbTreeNode* GetMaxRbNode(CRbTreeNode* x)
	{
		while (x->pRight)
			x = x->pRight;
		return x;
	}

	inline static void RightRotate(CRbTreeNode* x, CRbTreeNode*& pRoot)
	{
		CRbTreeNode* y = x->pLeft;
		x->pLeft = y->pRight;
		if (y->pRight)
			y->pRight->pParent = x;
		y->pParent = x->pParent;

		if (x == pRoot)
			pRoot = y;
		else if (x == x->pParent->pRight)
			x->pParent->pRight = y;
		else
			x->pParent->pLeft = y;
		y->pRight = x;
		x->pParent = y;
	}

	inline static void LeftRotate(CRbTreeNode* x, CRbTreeNode* &pRoot)
	{
		CRbTreeNode* y = x->pRight;
		x->pRight = y->pLeft;
		if (y->pLeft)
			y->pLeft->pParent = x;
		y->pParent = x->pParent;
		if (x == pRoot)
			pRoot = y;
		else if (x == x->pParent->pLeft)
			x->pParent->pLeft = y;
		else
			x->pParent->pRight = y;
		y->pLeft = x;
		x->pParent = y;
	}

	inline static void InsertReBalance(CRbTreeNode* x, CRbTreeNode*& pRoot)
	{
		x->bIsBlack = false;
		while (x != pRoot && x->pParent->bIsBlack == false)
		{
			if (x->pParent == x->pParent->pParent->pLeft)
			{
				CRbTreeNode* y = x->pParent->pParent->pRight;
				if (y && y->bIsBlack == false)
				{
					x->pParent->bIsBlack = true;
					y->bIsBlack = true;
					x->pParent->pParent->bIsBlack = false;
					x = x->pParent->pParent;
				}
				else
				{
					if (x == x->pParent->pRight)
					{
						x = x->pParent;
						LeftRotate(x, pRoot);
					}
					x->pParent->bIsBlack = true;
					x->pParent->pParent->bIsBlack = false;
					RightRotate(x->pParent->pParent, pRoot);
				}
			}
			else
			{
				CRbTreeNode* y = x->pParent->pParent->pLeft;
				if (y && y->bIsBlack == false)
				{
					x->pParent->bIsBlack = true;
					y->bIsBlack = true;
					x->pParent->pParent->bIsBlack = false;
					x = x->pParent->pParent;
				}
				else
				{
					if (x == x->pParent->pLeft)
					{
						x = x->pParent;
						RightRotate(x, pRoot);
					}
					x->pParent->bIsBlack = true;
					x->pParent->pParent->bIsBlack = false;
					LeftRotate(x->pParent->pParent, pRoot);
				}
			}
		}
		pRoot->bIsBlack = true;
	}

	inline static CRbTreeNode* EraseReBalance(CRbTreeNode* z, CRbTreeNode*& pRoot, CRbTreeNode*& pLeftMost, CRbTreeNode*& pRightMost)
	{
		CRbTreeNode* y = z;
		CRbTreeNode* x = 0;
		CRbTreeNode* x_pParent = 0;
		if (y->pLeft == 0)     // z has at most one non-null child. y == z.
			x = y->pRight;     // x might be null.
		else if (y->pRight == 0)  // z has exactly one non-null child. y == z.
			x = y->pLeft;    // x is not null.
		else
		{
			// z has two non-null children.  Set y to
			y = y->pRight;   //   z's successor.  x might be null.
			while (y->pLeft)
				y = y->pLeft;
			x = y->pRight;
		}
		if (y != z)
		{
			// relink y in place of z.  y is z's successor
			z->pLeft->pParent = y;
			y->pLeft = z->pLeft;
			if (y != z->pRight)
			{
				x_pParent = y->pParent;
				if (x)
					x->pParent = y->pParent;
				y->pParent->pLeft = x;      // y must be a child of pLeft
				y->pRight = z->pRight;
				z->pRight->pParent = y;
			}
			else
				x_pParent = y;
			if (pRoot == z)
				pRoot = y;
			else if (z->pParent->pLeft == z)
				z->pParent->pLeft = y;
			else
				z->pParent->pRight = y;
			y->pParent = z->pParent;
			::FOCP_NAME::Swap(y->bIsBlack, z->bIsBlack);
			y = z;
			// y now points to node to be actually deleted
		}
		else
		{
			// y == z
			x_pParent = y->pParent;
			if (x)
				x->pParent = y->pParent;
			if (pRoot == z)
				pRoot = x;
			else if (z->pParent->pLeft == z)
				z->pParent->pLeft = x;
			else
				z->pParent->pRight = x;
			if (pLeftMost == z)
			{
				if (z->pRight == 0)        // z->pLeft must be null also
					pLeftMost = z->pParent;
				// makes pLeftMost == &m_oHeader if z == pRoot
				else
					pLeftMost = GetMinRbNode(x);
			}
			if (pRightMost == z)
			{
				if (z->pLeft == 0)         // z->pRight must be null also
					pRightMost = z->pParent;
				// makes pRightMost == &m_oHeader if z == pRoot
				else                      // x == z->pLeft
					pRightMost = GetMaxRbNode(x);
			}
		}
		if (y->bIsBlack)
		{
			while (x != pRoot && (x == 0 || x->bIsBlack))
			{
				if (x == x_pParent->pLeft)
				{
					CRbTreeNode* w = x_pParent->pRight;
					if (w->bIsBlack == false)
					{
						w->bIsBlack = true;
						x_pParent->bIsBlack = false;
						LeftRotate(x_pParent, pRoot);
						w = x_pParent->pRight;
					}
					if ((w->pLeft == 0 || w->pLeft->bIsBlack) &&
							(w->pRight == 0 || w->pRight->bIsBlack))
					{
						w->bIsBlack = false;
						x = x_pParent;
						x_pParent = x_pParent->pParent;
					}
					else
					{
						if (w->pRight == 0 || w->pRight->bIsBlack)
						{
							if (w->pLeft)
								w->pLeft->bIsBlack = true;
							w->bIsBlack = false;
							RightRotate(w, pRoot);
							w = x_pParent->pRight;
						}
						w->bIsBlack = x_pParent->bIsBlack;
						x_pParent->bIsBlack = true;
						if (w->pRight)
							w->pRight->bIsBlack = true;
						LeftRotate(x_pParent, pRoot);
						break;
					}
				}
				else
				{
					// same as above, with pRight <-> pLeft.
					CRbTreeNode* w = x_pParent->pLeft;
					if (w->bIsBlack == false)
					{
						w->bIsBlack = true;
						x_pParent->bIsBlack = false;
						RightRotate(x_pParent, pRoot);
						w = x_pParent->pLeft;
					}
					if ((w->pRight == 0 || w->pRight->bIsBlack) &&
							(w->pLeft == 0 || w->pLeft->bIsBlack))
					{
						w->bIsBlack = false;
						x = x_pParent;
						x_pParent = x_pParent->pParent;
					}
					else
					{
						if (w->pLeft == 0 || w->pLeft->bIsBlack)
						{
							if (w->pRight)
								w->pRight->bIsBlack = true;
							w->bIsBlack = false;
							LeftRotate(w, pRoot);
							w = x_pParent->pLeft;
						}
						w->bIsBlack = x_pParent->bIsBlack;
						x_pParent->bIsBlack = true;
						if (w->pLeft)
							w->pLeft->bIsBlack = true;
						RightRotate(x_pParent, pRoot);
						break;
					}
				}
			}
			if (x)
				x->bIsBlack = true;
		}
		return y;
	}

	inline void MoveSubTree(CBaseRbTree<TKey, TQueryKey, TCompareKey> *pTo, CRbTreeNode* x)
	{
		while (x)
		{
			MoveSubTree(pTo, x->pRight);
			CRbTreeNode* y = x->pLeft;
			if(Insert(x) == &m_oHeader)
				pTo->Insert(x);
			x = y;
		}
	}

	inline void InsertNode(CRbTreeNode* x, CRbTreeNode* y, CRbTreeNode* z)
	{
		if (y == &m_oHeader || x || Less(TQueryKey::QueryKey(z), TQueryKey::QueryKey(y)))
		{
			y->pLeft = z;
			if (y == &m_oHeader)
			{
				m_oHeader.pParent = z;
				m_oHeader.pRight = z;
			}
			else if (y == m_oHeader.pLeft)
				m_oHeader.pLeft = z;
		}
		else
		{
			y->pRight = z;
			if (y == m_oHeader.pRight)
				m_oHeader.pRight = z;
		}
		z->pParent = y;
		z->pLeft = 0;
		z->pRight = 0;
		InsertReBalance(z, m_oHeader.pParent);
		++m_nSize;
	}

};

template
<
	typename TKey,
	typename TData=TKey,
	typename TGetKey=CGetKey<TKey, TData>,//从数据中获得TKey
	typename TCompareKey=CCompareKey<TKey>
> class CRbTree
{
private:
	CBaseRbTree<TKey, FOCP_DETAIL_NAME::CQueryKey<TKey, TData, TGetKey>, TCompareKey> m_oTree;

public:
	inline ~CRbTree()
	{
		Clear();
	}

	inline CRbTree(bool bUnique=true)
		:m_oTree(bUnique)
	{
	}

	inline CRbTree(const CRbTree<TKey, TData, TGetKey, TCompareKey> &oSrc)
		:m_oTree(oSrc.m_oTree.Unique())
	{
		Insert(oSrc);
	}

	inline CRbTree<TKey, TData, TGetKey, TCompareKey>& operator=(const CRbTree<TKey, TData, TGetKey, TCompareKey>& oSrc)
	{
		if(this != &oSrc)
		{
			CRbTree<TKey, TData, TGetKey, TCompareKey> o(oSrc);
			Swap(o);
		}
		return *this;
	}

	inline void Clear()
	{
		if(GetSize())
		{
			ClearSubTree(End()->pParent);
			m_oTree.Clear();
		}
	}

	inline uint32 GetSize() const
	{
		return m_oTree.GetSize();
	}

	inline bool Unique() const
	{
		return m_oTree.Unique();
	}

	inline CRbTreeNode* First() const
	{
		return m_oTree.First();
	}

	inline CRbTreeNode* Last() const
	{
		return m_oTree.Last();
	}

	inline CRbTreeNode* End() const
	{
		return m_oTree.End();
	}

	inline CRbTreeNode* GetNext(const CRbTreeNode* pIt) const
	{
		return m_oTree.GetNext(pIt);
	}

	inline CRbTreeNode* GetPrev(const CRbTreeNode* pIt) const
	{
		return m_oTree.GetPrev(pIt);
	}

	inline CRbTreeNode* UpperBound(const TKey &oKey) const
	{
		return m_oTree.UpperBound(oKey);
	}

	inline CRbTreeNode* LowerBound(const TKey &oKey) const
	{
		return m_oTree.LowerBound(oKey);
	}

	inline CRbTreeNode* Find(const TKey &oKey) const
	{
		return m_oTree.Find(oKey);
	}

	inline const TData* At(const CRbTreeNode* pIt) const
	{
		if(pIt == End())
			return NULL;
		return &((FOCP_DETAIL_NAME::TRbTreeNode<TData>*)pIt)->oData;
	}

	inline const TData& GetItem(const CRbTreeNode* pIt) const
	{
		return *At(pIt);
	}

	inline TData* At(CRbTreeNode* pIt)
	{
		if(pIt == End())
			return NULL;
		return &((FOCP_DETAIL_NAME::TRbTreeNode<TData>*)pIt)->oData;
	}

	inline TData& GetItem(CRbTreeNode* pIt)
	{
		return *At(pIt);
	}

	inline CRbTreeNode* Insert(const TData &oData)
	{
		FOCP_DETAIL_NAME::TRbTreeNode<TData>* pNode = new FOCP_DETAIL_NAME::TRbTreeNode<TData>;
		pNode->oData = oData;
		CRbTreeNode* pRet = m_oTree.Insert(&pNode->oNode);
		if(pRet == End())
			delete pNode;
		return pRet;
	}

	inline void Insert(const CRbTree<TKey, TData, TGetKey, TCompareKey> &oSrc)
	{
		if(this == &oSrc)
		{
			if(!Unique())
			{
				CRbTree<TKey, TData, TGetKey, TCompareKey> o(oSrc);
				MoveFrom(o);
			}
		}
		else
		{
			CRbTreeNode* pNode = oSrc.First();
			CRbTreeNode* pEnd = oSrc.End();
			for(; pNode != pEnd; pNode=oSrc.GetNext(pNode))
				Insert(((FOCP_DETAIL_NAME::TRbTreeNode<TData>*)pNode)->oData);
		}
	}

	//将oSrc的所有元素移入到当前容器。如果bDiscardConflictItem为true，那么将丢弃键值冲突的数据项。
	inline void MoveFrom(CRbTree<TKey, TData, TGetKey, TCompareKey> &oSrc, bool bDiscardConflictItem=true)
	{
		if(this != &oSrc)
		{
			if(bDiscardConflictItem)
			{
				MoveSubTree(NULL, oSrc.End()->pParent);
				oSrc.m_oTree.Clear();
			}
			else
			{
				CRbTree<TKey, TData, TGetKey, TCompareKey> oTmp(oSrc.Unique());
				MoveSubTree(&oTmp, oSrc.End()->pParent);
				oSrc.m_oTree.Clear();
				oSrc.Swap(oTmp);
			}
		}
	}

	inline CRbTreeNode* Remove(CRbTreeNode* pNode)
	{
		if(pNode != m_oTree.End())
		{
			CRbTreeNode* pNext = m_oTree.GetNext(pNode);
			m_oTree.Remove(pNode);
			delete (FOCP_DETAIL_NAME::TRbTreeNode<TData>*)pNode;
			pNode = pNext;
		}
		return pNode;
	}

	inline void Remove(const TKey& oKey, bool bRemoveAll=true)
	{
		CRbTreeNode* pIt = m_oTree.Find(oKey);
		CRbTreeNode* pEnd = m_oTree.End();
		bool bUnique = m_oTree.Unique();

		if(pIt != pEnd)while(true)
			{
				pIt = (CRbTreeNode*)Remove(pIt);
				if(bUnique || !bRemoveAll || pIt == pEnd || TCompareKey::Compare(&oKey, TGetKey::GetKey(((const FOCP_DETAIL_NAME::TRbTreeNode<TData>*)pIt)->oData)))
					break;
			}
	}

	inline void Swap(CRbTree<TKey, TData, TGetKey, TCompareKey> &oSrc)
	{
		m_oTree.Swap(oSrc.m_oTree);
	}

private:
	inline void ClearSubTree(CRbTreeNode* x)
	{
		while (x)
		{
			ClearSubTree(x->pRight);
			CRbTreeNode* y = x->pLeft;
			delete (FOCP_DETAIL_NAME::TRbTreeNode<TData>*)x;
			x = y;
		}
	}

	inline void MoveSubTree(CRbTree<TKey, TData, TGetKey, TCompareKey> *pTo, CRbTreeNode* x)
	{
		CRbTreeNode* pEnd = End();
		while (x)
		{
			MoveSubTree(pTo, x->pRight);
			CRbTreeNode* y = x->pLeft;
			if(m_oTree.Insert(x) == pEnd)
			{
				if(pTo)
					pTo->m_oTree.Insert(x);
				else
					delete (FOCP_DETAIL_NAME::TRbTreeNode<TData>*)x;
			}
			x = y;
		}
	}
};

template
<
	typename TKey,
	typename TData,
	typename TCompareKey=CCompareKey<TKey>
>class CRbMap
{
private:
	CRbTree<TKey, CMapNode<TKey, TData>, CGetMapKey<TKey, TData>, TCompareKey> m_oTree;

public:
	inline ~CRbMap()
	{
	}

	inline CRbMap(bool bUnique=true):m_oTree(bUnique)
	{
	}

	inline CRbMap(const CRbMap<TKey, TData, TCompareKey> &oSrc):m_oTree(oSrc.m_oTree)
	{
	}

	inline CRbMap<TKey, TData, TCompareKey>& operator=(const CRbMap<TKey, TData, TCompareKey> &oSrc)
	{
		m_oTree = oSrc.m_oTree;
		return *this;
	}

	inline void Clear()
	{
		m_oTree.Clear();
	}

	inline uint32 GetSize() const
	{
		return m_oTree.GetSize();
	}

	inline bool Unique() const
	{
		return m_oTree.Unique();
	}

	inline CRbTreeNode* First() const
	{
		return m_oTree.First();
	}

	inline CRbTreeNode* Last() const
	{
		return m_oTree.Last();
	}

	inline CRbTreeNode* End() const
	{
		return m_oTree.End();
	}

	inline const TKey& GetKey(const CRbTreeNode* pIt) const
	{
		return m_oTree.At(pIt)->oKey;
	}

	inline CRbTreeNode* GetNext(const CRbTreeNode* pIt) const
	{
		return m_oTree.GetNext(pIt);
	}

	inline CRbTreeNode* GetPrev(const CRbTreeNode* pIt) const
	{
		return m_oTree.GetPrev(pIt);
	}

	inline CRbTreeNode* UpperBound(const TKey &oKey) const
	{
		return m_oTree.UpperBound(oKey);
	}

	inline CRbTreeNode* LowerBound(const TKey &oKey) const
	{
		return m_oTree.LowerBound(oKey);
	}

	inline CRbTreeNode* Find(const TKey& oKey) const
	{
		return m_oTree.Find(oKey);
	}

	inline const TData* At(const CRbTreeNode* pIt) const
	{
		CMapNode<TKey, TData>* pData = (CMapNode<TKey, TData>*)m_oTree.At(pIt);
		if(pData == NULL)
			return NULL;
		return &pData->oData;
	}


	inline const TData& GetItem(const CRbTreeNode* pIt) const
	{
		return *At(pIt);
	}

	inline TData* At(CRbTreeNode* pIt)
	{
		CMapNode<TKey, TData>* pData = (CMapNode<TKey, TData>*)m_oTree.At(pIt);
		if(pData == NULL)
			return NULL;
		return &pData->oData;
	}

	inline TData& GetItem(CRbTreeNode* pIt)
	{
		return *At(pIt);
	}

	inline CRbTreeNode* Insert(const TKey& oKey, const TData &oData)
	{
		CMapNode<TKey, TData> oNode(oKey, oData);
		return m_oTree.Insert(oNode);
	}

	inline void Insert(const CRbMap<TKey, TData, TCompareKey> &oSrc)
	{
		m_oTree.Insert(oSrc.m_oTree);
	}

	inline void MoveFrom(CRbMap<TKey, TData, TCompareKey> &oSrc, bool bDiscardConflictItem=true)
	{
		m_oTree.MoveFrom(oSrc.m_oTree, bDiscardConflictItem);
	}

	inline CRbTreeNode* Remove(CRbTreeNode* pIt)
	{
		return m_oTree.Remove(pIt);
	}

	inline void Remove(const TKey& oKey, bool bRemoveAll=true)
	{
		m_oTree.Remove(oKey, bRemoveAll);
	}

	inline TData& operator[](const TKey& oKey)
	{
		CRbTreeNode* pIt = Find(oKey);
		TData* pData = At(pIt);
		if(pData == NULL)
		{
			CMapNode<TKey, TData> oNode(oKey);
			pIt = m_oTree.Insert(oNode);
			pData = At(pIt);
		}
		return *pData;
	}

	inline const TData& operator[](const TKey& oKey) const
	{
		CRbTreeNode* pIt = Find(oKey);
		return *At(pIt);
	}

	inline void Swap(CRbMap<TKey, TData, TCompareKey> &oSrc)
	{
		m_oTree.Swap(oSrc.m_oTree);
	}
};

template<typename TData> class CSmartPointerManager;

template<typename TData> class CSmartPointer
{
	friend class CSmartPointerManager<TData>;

private:
	struct CNode
	{
		CSmartPointer<TData> *pNode;//必须是第一个字段
		uint32 nCounter;
	};
	TData* m_pObject;
	CRbTree<CSmartPointer<TData>*, CNode> m_oChildren;
	bool m_bOk, m_bDoActive;

	void DeActive()
	{
		m_bOk = false;
		CRbTreeNode* pEnd = m_oChildren.End();
		CRbTreeNode* pIt = m_oChildren.First();
		for(; pIt != pEnd; pIt=m_oChildren.GetNext(pIt))
		{
			CNode& oNode = m_oChildren.GetItem(pIt);
			CSmartPointer<TData>* pNode = oNode.pNode;
			if(pNode->m_bOk)
				pNode->DeActive();
		}
	}

	void ReActive()
	{
		m_bOk = true;
		m_bDoActive = true;
		CRbTreeNode* pEnd = m_oChildren.End();
		CRbTreeNode* pIt = m_oChildren.First();
		for(; pIt != pEnd; pIt=m_oChildren.GetNext(pIt))
		{
			CNode& oNode = m_oChildren.GetItem(pIt);
			CSmartPointer<TData>* pNode = oNode.pNode;
			if(!pNode->m_bDoActive)
				pNode->ReActive();
		}
	}

public:
	inline CSmartPointer(TData* pObj)
	{
		m_bDoActive = false;
		m_bOk = true;
		m_pObject = pObj;
	}

	inline ~CSmartPointer()
	{
		delete m_pObject;
	}

	inline void InsertPointer(CSmartPointer<TData>* pPointer)
	{
		CRbTreeNode* pEnd = m_oChildren.End();
		CRbTreeNode* pIt = m_oChildren.Find(pPointer);
		if(pIt != pEnd)
		{
			CNode& oNode = m_oChildren.GetItem(pIt);
			++oNode.nCounter;
		}
		else
		{
			CNode oNode = {pPointer, 1};
			m_oChildren.Insert(oNode);
		}
	}

	inline bool RemovePointer(CSmartPointer<TData>* pPointer)
	{
		bool bRet = false;
		CRbTreeNode* pEnd = m_oChildren.End();
		CRbTreeNode* pIt = m_oChildren.Find(pPointer);
		if(pIt != pEnd)
		{
			CNode& oNode = m_oChildren.GetItem(pIt);
			--oNode.nCounter;
			if(!oNode.nCounter)
			{
				bRet = true;
				pPointer->DeActive();
			}
		}
		return bRet;
	}

	inline TData* GetPointer()
	{
		return m_pObject;
	}

	inline const TData* GetPointer() const
	{
		return m_pObject;
	}

	inline TData* operator->()
	{
		return m_pObject;
	}
	inline const TData* operator->() const
	{
		return m_pObject;
	}

	inline operator TData*()
	{
		return m_pObject;
	}
	inline operator const TData*() const
	{
		return m_pObject;
	}

	inline operator TData&()
	{
		return *m_pObject;
	}
	inline operator const TData&() const
	{
		return *m_pObject;
	}

	inline TData& operator*()
	{
		return *m_pObject;
	}
	inline const TData& operator*() const
	{
		return *m_pObject;
	}
};

template<typename TData> class CSmartPointerManager
{
private:
	CRbTree<TData*, CSmartPointer<TData>*> m_oPointers;
	CSmartPointer<TData>* m_pMaster;

public:
	inline CSmartPointerManager(TData* pA)
	{
		m_pMaster = new CSmartPointer<TData>(pA);
		m_oPointers.Insert(m_pMaster);
	}

	inline ~CSmartPointerManager()
	{
	}

	inline bool InsertPointer(TData* pA, TData* pB)
	{
		if(pA == NULL || pB == NULL)
			return false;
		CRbTreeNode* pEnd = m_oPointers.End();
		CRbTreeNode* pItA = m_oPointers.Find(pA);
		CRbTreeNode* pItB = m_oPointers.Find(pB);
		if(pItA == pEnd)
			return false;
		CSmartPointer<TData>* pPointerA = m_oPointers.GetItem(pItA);
		CSmartPointer<TData>* pPointerB;
		if(pItB != pEnd)
			pPointerB = m_oPointers.GetItem(pItB);
		else
		{
			pPointerB = new CSmartPointer<TData>(pB);
			pItB = m_oPointers.Insert(pPointerB);
		}
		pPointerA->InsertPointer(pPointerB);
		return true;
	}

	inline bool RemovePointer(TData* pA, TData* pB)
	{
		if(pA == NULL || pB == NULL)
			return false;
		CRbTreeNode* pEnd = m_oPointers.End();
		CRbTreeNode* pItA = m_oPointers.Find(pA);
		CRbTreeNode* pItB = m_oPointers.Find(pB);
		if(pItA == pEnd || pItB == pEnd)
			return false;
		CSmartPointer<TData>* pPointerA = m_oPointers.GetItem(pItA);
		CSmartPointer<TData>* pPointerB = m_oPointers.GetItem(pItB);
		if(pPointerA->RemovePointer(pPointerB))
		{
			m_pMaster->ReActive();
			pItA = m_oPointers.First();
			while(pItA!=pEnd)
			{
				pPointerA = m_oPointers.GetItem(pItA);
				if(pPointerA->m_bOk)
				{
					pItA = m_oPointers.GetNext(pItA);
					pPointerA->m_bDoActive = false;
				}
				else
				{
					pItA = m_oPointers.Remove(pItA);
					delete pPointerA;
				}
			}
		}
		return true;
	}
};

FOCP_END();

#endif //_ADT_RBTREE_HPP_
