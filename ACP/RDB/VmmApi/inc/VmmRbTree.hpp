
#include "VmmVao.hpp"

#ifndef _VMM_RBTREE_HPP_
#define _VMM_RBTREE_HPP_

FOCP_BEGIN();

struct CVmmBaseRbTreeInfo
{
	// can't change these fields' sequence
	uint32 m_bUnique;
	uint32 m_nCount;
	uint64 m_nHeader;
};

template<typename TVirtualObject> struct CVmmBasicRbTreeNode
{
	// can't change these fields' sequence
	TVirtualObject m_oValue;
	uint64 m_nParent;
	uint64 m_nLeft;
	uint64 m_nRight;
	uint32 m_nRed:16;
	uint32 m_nDirty:16;
	uint32 m_nCounter;
};

#define nMyLeft m_pNode->m_nLeft
#define nMyRight m_pNode->m_nRight
#define nMyParent m_pNode->m_nParent
#define nMyRed m_pNode->m_nRed
#define oMyValue m_pNode->m_oValue
#define nMyDirty m_pNode->m_nDirty
#define nMyCounter m_pNode->m_nCounter

template<typename TVirtualObject, typename TVirtualKey> class CVmmRbTree
{
public:
	typedef CVmmBasicRbTreeNode<TVirtualObject> TBaseNode;
	typedef CVmmRbTree<TVirtualObject, TVirtualKey> TRbTree;
	typedef CPersistObjectPool<TBaseNode> TNodePool;

	enum {VMM_OBJECT_SIZE = sizeof(CVmmBaseRbTreeInfo), VMM_NODE_SIZE=sizeof(TBaseNode)};

	class CVmmRbTreeNode
	{
	public:
		TRbTree* m_pRbTree;
		TBaseNode* m_pNode;
		uint64 m_nThis;
		uint32 m_bMemory;

		void Clear()
		{
			if(m_pNode)
			{
				if(!m_bMemory)
					m_pRbTree->DeAllocateNode(m_nThis, m_pNode);
				m_pNode = NULL;
			}
		}

		~CVmmRbTreeNode()
		{
			Clear();
		}

		CVmmRbTreeNode(TRbTree* pRbTree=NULL)
		{
			m_pRbTree = pRbTree;
			m_pNode = NULL;
			m_nThis = 0;
			m_bMemory = 0;
		}

		CVmmRbTreeNode(const CVmmRbTreeNode& oSrc)
		{
			m_pRbTree = oSrc.m_pRbTree;
			m_pNode = oSrc.m_pNode;
			m_nThis = oSrc.m_nThis;
			m_bMemory = oSrc.m_bMemory;
			if(m_pNode && !m_bMemory)
				m_pRbTree->AddRef(m_pNode);
		}

		CVmmRbTreeNode& operator=(const CVmmRbTreeNode& oSrc)
		{
			if(this != &oSrc)
			{
				if(m_pNode && !m_bMemory)
					m_pRbTree->DeAllocateNode(m_nThis, m_pNode);
				m_pRbTree = oSrc.m_pRbTree;
				m_pNode = oSrc.m_pNode;
				m_nThis = oSrc.m_nThis;
				m_bMemory = oSrc.m_bMemory;
				if(m_pNode && !m_bMemory)
					m_pRbTree->AddRef(m_pNode);
			}
			return *this;
		}

		uint64 GetThis()
		{
			return m_nThis;
		}

		CVmmRbTreeNode& SetThis(uint64 nThis, bool bFirst=false)
		{
			if(m_nThis != nThis)
			{
				if(m_pNode)
				{
					if(!m_bMemory)
						m_pRbTree->DeAllocateNode(m_nThis, m_pNode);
					m_pNode = NULL;
				}
				m_nThis = nThis;
				m_bMemory = IS_MEMORY_ADDR(nThis);
				if(nThis)
				{
					if(m_bMemory)
						m_pNode = GET_MEMORY_OBJECT(TBaseNode, nThis);
					else
						m_pNode = m_pRbTree->AllocateNode(nThis, bFirst);
				}
			}
			return *this;
		}

		void ReplaceNode(CVmmRbTreeNode& oSrc)
		{
			uint64 nNewNode = oSrc.GetThis();
			if(nMyLeft)
			{
				CVmmRbTreeNode oNode(m_pRbTree);
				oNode.SetThis(nMyLeft);
				oNode.SetParent(nNewNode);
				oSrc.SetLeft(nMyLeft);
			}
			if(nMyRight)
			{
				CVmmRbTreeNode oNode(m_pRbTree);
				oNode.SetThis(nMyRight);
				oNode.SetParent(nNewNode);
				oSrc.SetRight(nMyRight);
			}
			if(nMyParent)
			{
				CVmmRbTreeNode oNode(m_pRbTree);
				oNode.SetThis(nMyParent);
				if(oNode.nMyLeft == m_nThis)
					oNode.SetLeft(nNewNode);
				else
					oNode.SetRight(nNewNode);
				oSrc.SetParent(nMyParent);
			}
			oSrc.SetRed(nMyRed);
		}

		uint64 MinNode()
		{
			CVmmRbTreeNode x(*this);
			while(x.nMyLeft)
				x.SetThis(x.nMyLeft);
			return x.m_nThis;
		}

		uint64 MaxNode()
		{
			CVmmRbTreeNode x(*this);
			while(x.nMyRight)
				x.SetThis(x.nMyRight);
			return x.m_nThis;
		}

		uint64 NextNode()
		{
			CVmmRbTreeNode x(*this);
			if(x.nMyRight)
			{
				x.SetThis(x.nMyRight);
				while(x.nMyLeft)
					x.SetThis(x.nMyLeft);
			}
			else
			{
				CVmmRbTreeNode y(m_pRbTree);
				y.SetThis(x.nMyParent);
				while(x.m_nThis == y.nMyRight)
				{
					x = y;
					y.SetThis(y.nMyParent);
				}
				if(x.nMyRight != y.m_nThis)
					x = y;
			}
			return x.m_nThis;
		}

		uint64 PrevNode()
		{
			CVmmRbTreeNode x(*this), y(m_pRbTree);
			if (x.nMyRed && y.SetThis(x.nMyParent).nMyParent == x.m_nThis)
				x.SetThis(x.nMyRight);
			else if(x.nMyLeft)
			{
				y.SetThis(x.nMyLeft);
				while (y.nMyRight)
					y.SetThis(y.nMyRight);
				x = y;
			}
			else
			{
				y.SetThis(x.nMyParent);
				while (x.m_nThis == y.nMyLeft)
				{
					x = y;
					y.SetThis(y.nMyParent);
				}
				x = y;
			}
			return x.m_nThis;
		}

		TVirtualObject& GetValue()
		{
			return oMyValue;
		}

		void ClearDirty()
		{
			nMyDirty = 0;
		}

		void SetValue(TVirtualObject & oObject)
		{
			TVirtualObject* pDst = &oMyValue;
			if(pDst != &oObject)
				oMyValue = oObject;
			nMyDirty = 1;
		}

		void SetRed(uint32 nRed)
		{
			if(nMyRed != nRed)
			{
				nMyRed = nRed;
				nMyDirty = 1;
			}
		}

		uint64 GetParent()
		{
			return nMyParent;
		}

		uint64 GetLeft()
		{
			return nMyLeft;
		}

		uint64 GetRight()
		{
			return nMyRight;
		}

		uint32 GetRed()
		{
			return nMyRed;
		}

		void SetParent(uint64 nParent)
		{
			if(nMyParent != nParent)
			{
				nMyParent = nParent;
				nMyDirty = 1;
			}
		}
		
		void SetLeft(uint64 nLeft)
		{
			if(nMyLeft != nLeft)
			{
				nMyLeft = nLeft;
				nMyDirty = 1;
			}
		}

		void SetRight(uint64 nRight)
		{
			if(nMyRight != nRight)
			{
				nMyRight = nRight;
				nMyDirty = 1;
			}
		}

		void Insert(uint64 x, uint64 y, uint64 h, 
			CVirtualGetKey<TVirtualObject, TVirtualKey>* pGetKey, 
			CVirtualCompare<TVirtualKey>* pCompare)
		{
			CVmmRbTreeNode yobj(m_pRbTree);
			CVmmRbTreeNode hobj(m_pRbTree);
			TVirtualKey lKey, rKey;

			yobj.SetThis(y);
			hobj.SetThis(h);

			if (y == h || x || 
				pCompare->Compare(lKey=pGetKey->GetKey(GetValue()), 
								  rKey=pGetKey->GetKey(yobj.GetValue())) < 0)
			{
				yobj.SetLeft(m_nThis);
				if (y == h)
				{
					hobj.SetParent(m_nThis);
					hobj.SetRight(m_nThis);
				}
				else if (y == hobj.nMyLeft)
					hobj.SetLeft(m_nThis);
			}
			else
			{
				yobj.SetRight(m_nThis);
				if (y == hobj.nMyRight)
					hobj.SetRight(m_nThis);
			}
			SetParent(y);
			SetLeft(0);
			SetRight(0);
			InsertRebalance(hobj);
		}

		void TakeOut(CVmmRbTreeNode& hobj)
		{
			CVmmRbTreeNode y(*this);
			CVmmRbTreeNode x(m_pRbTree);
			CVmmRbTreeNode xp(m_pRbTree);

			if(!y.nMyLeft)
				x.SetThis(y.nMyRight);
			else if(!y.nMyRight)
				x.SetThis(y.nMyLeft);
			else
			{
				y.SetThis(y.nMyRight);
				while (y.nMyLeft)
					y.SetThis(y.nMyLeft);
				x.SetThis(y.nMyRight);
			}

			if(y.m_nThis != m_nThis)
			{
				CVmmRbTreeNode p(m_pRbTree), l(m_pRbTree);
				l.SetThis(nMyLeft).SetParent(y.m_nThis);
				y.SetLeft(nMyLeft);
				if(y.m_nThis != nMyRight)
				{
					CVmmRbTreeNode r(m_pRbTree);
					xp.SetThis(y.nMyParent);
					if(x.m_nThis)
						x.SetParent(y.nMyParent);
					xp.SetLeft(x.m_nThis);
					y.SetRight(nMyRight);
					r.SetThis(nMyRight).SetParent(y.m_nThis);
				}
				else
					xp = y;
				if (hobj.nMyParent == m_nThis)
					hobj.SetParent(y.m_nThis);
				else if(p.SetThis(nMyParent).nMyLeft == m_nThis)
					p.SetLeft(y.m_nThis);
				else
					p.SetRight(y.m_nThis);
				y.SetParent(nMyParent);
				uint32 nRed = nMyRed;
				SetRed(y.nMyRed);
				y.SetRed(nRed);
				y = *this;
			}
			else
			{
				xp.SetThis(y.nMyParent);
				if(x.m_nThis)
					x.SetParent(y.nMyParent);
				if (hobj.nMyParent == m_nThis)
					hobj.SetParent(x.m_nThis);
				else if(xp.nMyLeft == m_nThis)
					xp.SetLeft(x.m_nThis);
				else
					xp.SetRight(x.m_nThis);
				if (hobj.nMyLeft == m_nThis)
				{
					if (!nMyRight)
						hobj.SetLeft(nMyParent);
					else
						hobj.SetLeft(x.MinNode());
				}
				if (hobj.nMyRight == m_nThis)
				{
					if (!nMyLeft)
						hobj.SetRight(nMyParent);
					else
						hobj.SetRight(x.MaxNode());
				}
			}

			if(!y.nMyRed)
			{
				CVmmRbTreeNode w(m_pRbTree), wl(m_pRbTree), wr(m_pRbTree);
				while(x.m_nThis != hobj.nMyParent && (!x.m_nThis || !x.nMyRed))
				{
					if(x.m_nThis == xp.nMyLeft)
					{
						w.SetThis(xp.nMyRight);
						if(w.nMyRed)
						{
							w.SetRed(0);
							xp.SetRed(1);
							xp.LeftRotate(hobj);
							w.SetThis(xp.nMyRight);
						}
						if ((!w.nMyLeft || !wl.SetThis(w.nMyLeft).nMyRed) &&
							(!w.nMyRight || !wr.SetThis(w.nMyRight).nMyRed)) 
						{
							w.SetRed(1);
							x = xp;
							xp.SetThis(xp.nMyParent);
						} 
						else
						{
							if (!w.nMyRight || !wr.SetThis(w.nMyRight).nMyRed) 
							{
								if(w.nMyLeft)
									wl.SetThis(w.nMyLeft).SetRed(0);
								w.SetRed(1);
								w.RightRotate(hobj);
								w.SetThis(xp.nMyRight);
							}
							w.SetRed(xp.nMyRed);
							xp.SetRed(0);
							if(w.nMyRight)
								wr.SetThis(w.nMyRight).SetRed(0);
							xp.LeftRotate(hobj);
							break;
						}
					}
					else
					{
						w.SetThis(xp.nMyLeft);
						if(w.nMyRed)
						{
							w.SetRed(0);
							xp.SetRed(1);
							xp.RightRotate(hobj);
							w.SetThis(xp.nMyLeft);
						}
						if ((!w.nMyRight || !wr.SetThis(w.nMyRight).nMyRed) && 
							(!w.nMyLeft || !wr.SetThis(w.nMyLeft).nMyRed)) 
						{
							w.SetRed(1);
							x = xp;
							xp.SetThis(xp.nMyParent);
						} 
						else
						{
							if(!w.nMyLeft || !wl.SetThis(w.nMyLeft).nMyRed)
							{
								if(w.nMyRight)
									wr.SetThis(w.nMyRight).SetRed(0);
								w.SetRed(1);
								w.LeftRotate(hobj);
								w.SetThis(xp.nMyLeft);
							}
							w.SetRed(xp.nMyRed);
							xp.SetRed(0);
							if(w.nMyLeft)
								wl.SetThis(w.nMyLeft).SetRed(0);
							xp.RightRotate(hobj);
							break;
						}
					}
				}
				if(x.m_nThis)
					x.SetRed(0);
			}
		}

	private:
		void LeftRotate(CVmmRbTreeNode &hobj)
		{
			CVmmRbTreeNode y(m_pRbTree);
			CVmmRbTreeNode yl(m_pRbTree);
			CVmmRbTreeNode xp(m_pRbTree);

			y.SetThis(nMyRight);
			SetRight(y.nMyLeft);
			if(y.nMyLeft)
				yl.SetThis(y.nMyLeft).SetParent(m_nThis);
			y.SetParent(nMyParent);
			if(m_nThis == hobj.nMyParent)
				hobj.SetParent(y.m_nThis);
			else if(m_nThis == xp.SetThis(nMyParent).nMyLeft)
				xp.SetLeft(y.m_nThis);
			else
				xp.SetRight(y.m_nThis);
			y.SetLeft(m_nThis);
			SetParent(y.m_nThis);
		}

		void RightRotate(CVmmRbTreeNode &hobj)
		{
			CVmmRbTreeNode y(m_pRbTree);
			CVmmRbTreeNode yr(m_pRbTree);
			CVmmRbTreeNode xp(m_pRbTree);

			y.SetThis(nMyLeft);
			SetLeft(y.nMyRight);
			if(y.nMyRight)
				yr.SetThis(y.nMyRight).SetParent(m_nThis);
			y.SetParent(nMyParent);
			
			if(m_nThis == hobj.nMyParent)
				hobj.SetParent(y.m_nThis);
			else if(m_nThis == xp.SetThis(nMyParent).nMyRight)
				xp.SetRight(y.m_nThis);
			else
				xp.SetLeft(y.m_nThis);
			y.SetRight(m_nThis);
			SetParent(y.m_nThis);
		}

		void InsertRebalance(CVmmRbTreeNode &hobj)
		{
			CVmmRbTreeNode xp(m_pRbTree);
			CVmmRbTreeNode xpp(m_pRbTree);
			CVmmRbTreeNode y(m_pRbTree);

			uint64 nThis = m_nThis;

			SetRed(1);
			while (m_nThis != hobj.nMyParent && 
				xp.SetThis(nMyParent).nMyRed)
			{
				if(nMyParent == xpp.SetThis(xp.nMyParent).nMyLeft)
				{
					y.SetThis(xpp.nMyRight);
					if(y.m_nThis && y.nMyRed)
					{
						xp.SetRed(0);
						y.SetRed(0);
						xpp.SetRed(1);
						*this = xpp;
					}
					else
					{
						if(m_nThis == xp.nMyRight)
						{
							*this = xp;
							LeftRotate(hobj);
							xp.SetThis(nMyParent);
							xpp.SetThis(xp.nMyParent);
						}
						xp.SetRed(0);
						xpp.SetRed(1);
						xpp.RightRotate(hobj);
					}
				}
				else
				{
					y.SetThis(xpp.nMyLeft);
					if (y.m_nThis && y.nMyRed) 
					{
						xp.SetRed(0);
						y.SetRed(0);
						xpp.SetRed(1);
						*this = xpp;
					}
					else 
					{
						if (m_nThis == xp.nMyLeft)
						{
							*this = xp;
							RightRotate(hobj);
							xp.SetThis(nMyParent);
							xpp.SetThis(xp.nMyParent);
						}
						xp.SetRed(0);
						xpp.SetRed(1);
						xpp.LeftRotate(hobj);
					}
				}
			}
			CVmmRbTreeNode(m_pRbTree).SetThis(hobj.nMyParent).SetRed(0);
			SetThis(nThis);
		}
	};

	class CIterator
	{
	private:
		uint64 m_nHeader;
		CVmmRbTreeNode m_oNode;

	public:
		~CIterator()
		{
		}

		CIterator()
		{
			m_nHeader = 0;
		}

		CIterator(CVmmRbTreeNode& n, uint64 nHeader)
			:m_oNode(n)
		{
			m_nHeader = nHeader;
			if(!m_oNode.GetThis() && nHeader)
				m_oNode.SetThis(nHeader);
		}

		CIterator(const CIterator & oSrc)
		{
			if(this != &oSrc)
			{
				m_oNode = oSrc.m_oNode;
				m_nHeader = oSrc.m_nHeader;
			}
		}

		CIterator& operator++ ()
		{
			if(m_oNode.GetThis() == m_nHeader)
				m_oNode.SetThis(m_oNode.nMyLeft);
			else
				m_oNode.SetThis(m_oNode.NextNode());
			return *this;
		}

		CIterator operator++ (int)
		{
			CIterator it(*this);
			operator++();
			return it;
		}

		CIterator& operator-- ()
		{
			CVmmRbTreeNode oEndNode(m_oNode);
			oEndNode.SetThis(m_nHeader);
			if(oEndNode.nMyLeft == m_oNode.GetThis())
				m_oNode = oEndNode;
			else
				m_oNode.SetThis(m_oNode.PrevNode());
			return *this;
		}

		CIterator operator-- (int)
		{
			CIterator it(*this);
			operator--();
			return it;
		}

		bool operator == (const CIterator &oSrc)
		{
			return GetThis() == ((CIterator&)oSrc).GetThis();
		}

		bool operator != (const CIterator &oSrc)
		{
			return GetThis() != ((CIterator&)oSrc).GetThis();
		}

		uint64 GetThis()
		{
			uint64 nRet = m_oNode.GetThis();
			if(nRet == m_nHeader)
				nRet = 0;
			return nRet;
		}
		
		CIterator& SetThis(uint64 nThis)
		{
			m_oNode.SetThis(nThis);
			return *this;
		}

		TVirtualObject& GetValue()
		{
			return m_oNode.GetValue();
		}

		void SetValue(TVirtualObject & oObject)
		{
			m_oNode.SetValue(oObject);
		}

		CVmmRbTreeNode& GetNode()
		{
			return m_oNode;
		}

		uint64 GetHead()
		{
			return m_nHeader;
		}
	};

private:
	uint64 m_nThis;
	uint32 m_bMemory;
	CVmmBaseRbTreeInfo * m_pObjectInfo;
	CVirtualAccess* m_pAccess;
	CVirtualAllocator* m_pAllocator;
	CVirtualAllocator* m_pTopAllocator;
	TNodePool m_oNodePool;

	CVirtualGetKey<TVirtualObject, TVirtualKey>* m_pGetKey;
	CVirtualCompare<TVirtualKey>* m_pCompare;

	CVmmRbTree(TRbTree& oSrc);
	TRbTree& operator=(TRbTree & oSrc);

private:
	void InitRoot()
	{
		CVmmRbTreeNode oRoot(this);
		oRoot.SetThis(m_pObjectInfo->m_nHeader, true);
		oRoot.SetParent(0);
		oRoot.SetLeft(m_pObjectInfo->m_nHeader);
		oRoot.SetRight(m_pObjectInfo->m_nHeader);
		oRoot.SetRed(1);
	}

public:
	void* GetObjectInfo()
	{
		return m_pObjectInfo;
	}

	virtual ~CVmmRbTree()
	{
		if(m_pObjectInfo)
		{
			if(!m_bMemory)
				unvmap(m_pObjectInfo);
			m_pObjectInfo = NULL;
		}
	}

	CVmmRbTree(CVirtualAccess* pAccess,
		CVirtualGetKey<TVirtualObject, TVirtualKey>* pGetKey,
		CVirtualCompare<TVirtualKey>* pCompare,
		CVirtualAllocator* pAllocator, 
		CVirtualAllocator* pTopAllocator=NULL)
	{
		m_bMemory = 0;
		m_nThis = 0;
		m_pGetKey = pGetKey;
		m_pCompare = pCompare;
		m_pAccess = pAccess;
		m_pAllocator = pAllocator;
		m_pTopAllocator = pTopAllocator;
		m_pObjectInfo = NULL;
	}

	TBaseNode* AllocateNode(uint64 nThis, bool bFirst=false)
	{
		return m_oNodePool.QueryObject(nThis, bFirst);
	}

	void DeAllocateNode(uint64 nThis, TBaseNode* pNode)
	{
		m_oNodePool.ReleaseObject(nThis, pNode);
	}

	void AddRef(TBaseNode* pNode)
	{
		m_oNodePool.AddRef(pNode);
	}

	uint64 GetThis()
	{
		return m_nThis;
	}

	TRbTree& SetThis(uint64 nThis)
	{
		if(nThis != m_nThis)
		{
			if(m_pObjectInfo)
			{
				if(!m_bMemory)
					unvmap(m_pObjectInfo);
				m_pObjectInfo = NULL;
			}
			m_nThis = nThis;
			if(m_nThis)
			{
				m_bMemory = ((nThis&VMM_MEMORY_FLAG)?1:0);
				if(m_bMemory)
					m_pObjectInfo = GET_MEMORY_OBJECT(CVmmBaseRbTreeInfo, nThis);
				else
					m_pObjectInfo = (CVmmBaseRbTreeInfo*)vmap(nThis);
			}
		}
		return *this;
	}

	bool CreateObject(uint32 bUnique, int32 bInMemory)
	{
		DestroyObject();
		m_bMemory = bInMemory;
		if(m_pTopAllocator)
			m_nThis = m_pTopAllocator->Allocate((uint32)VMM_OBJECT_SIZE + VMM_NODE_SIZE, bInMemory);
		else
			m_nThis = vmalloc((uint32)VMM_OBJECT_SIZE + VMM_NODE_SIZE, bInMemory);
		if(!m_nThis)
			return false;
		if(m_bMemory)
			m_pObjectInfo = GET_MEMORY_OBJECT(CVmmBaseRbTreeInfo, m_nThis);
		else
			m_pObjectInfo = (CVmmBaseRbTreeInfo*)vmap(m_nThis);
		m_pObjectInfo->m_bUnique = bUnique;
		m_pObjectInfo->m_nHeader = m_nThis + VMM_OBJECT_SIZE;
		m_pObjectInfo->m_nCount = 0;
		if(!m_bMemory)
			vflush(m_pObjectInfo);
		InitRoot();
		return true;
	}

	void DestroyObject()
	{
		if(m_nThis)
		{
			Clear();
			if(!m_bMemory)
				unvmap(m_pObjectInfo);
			m_pObjectInfo = NULL;
			if(m_pTopAllocator)
				m_pTopAllocator->DeAllocate(m_nThis);
			else
				vfree(m_nThis);
			m_nThis = 0;
		}
	}

	void Truncate()
	{
		if(m_nThis && m_pObjectInfo->m_nCount)
		{
			InitRoot();
			m_pObjectInfo->m_nCount = 0;
			if(!m_bMemory)
				vflush(m_pObjectInfo);
		}
	}

	void Clear()
	{
		if(m_nThis && m_pObjectInfo->m_nCount)
		{
			CVmmRbTreeNode oRoot(this);
			oRoot.SetThis(m_pObjectInfo->m_nHeader, true);
			ClearSubTree(oRoot.m_pNode->m_nParent);
			oRoot.SetParent(0);
			oRoot.SetLeft(m_pObjectInfo->m_nHeader);
			oRoot.SetRight(m_pObjectInfo->m_nHeader);
			oRoot.SetRed(1);
		}
	}

	uint32 IsUnique()
	{
		return m_pObjectInfo->m_bUnique;
	}

	uint32 GetSize()
	{
		return m_pObjectInfo->m_nCount;
	}

	CIterator Begin()
	{
		return ++End();
	}

	CIterator End()
	{
		uint64 nHeader = m_pObjectInfo->m_nHeader;
		return CIterator(CVmmRbTreeNode(this).SetThis(nHeader), nHeader);
	}

	CIterator At(uint64 nNode, bool bFirst=false)
	{
		return CIterator(CVmmRbTreeNode(this).SetThis(nNode, bFirst), m_pObjectInfo->m_nHeader);
	}

	CVmmRbTreeNode NodeAt(uint64 nNode, bool bFirst=false)
	{
		return CVmmRbTreeNode(this).SetThis(nNode, bFirst);
	}

	CIterator Find(TVirtualKey& oKey)
	{
		CIterator it = LowerBound(oKey);
		CIterator end = End();
		if(it != end)
		{
			TVirtualKey lKey = m_pGetKey->GetKey(it.GetValue());
			if(!m_pCompare->Compare(lKey, oKey))
				return it;
		}
		return end;
	}

	CIterator LowerBound(TVirtualKey& oKey)
	{
		CVmmRbTreeNode y(this);
		CVmmRbTreeNode x(this);
		y.SetThis(m_pObjectInfo->m_nHeader);
		x.SetThis(y.nMyParent);
		while (x.GetThis())
		{
			TVirtualKey lKey = m_pGetKey->GetKey(x.GetValue());
			int32 nCmp = m_pCompare->Compare(lKey, oKey);
			if(m_pObjectInfo->m_bUnique && !nCmp)
				return CIterator(x, m_pObjectInfo->m_nHeader);
			if(nCmp >= 0)
			{
				y = x;
				x.SetThis(x.nMyLeft);
			}
			else
				x.SetThis(x.nMyRight);
		}
		return CIterator(y, m_pObjectInfo->m_nHeader);
	}

	CIterator UpperBound(TVirtualKey& oKey)
	{
		CVmmRbTreeNode y(this);
		CVmmRbTreeNode x(this);
		y.SetThis(m_pObjectInfo->m_nHeader);
		x.SetThis(y.nMyParent);
		while(x.GetThis())
		{
			TVirtualKey lKey = m_pGetKey->GetKey(x.GetValue());
			if(m_pCompare->Compare(oKey, lKey) < 0)
			{
				y = x;
				x.SetThis(x.nMyLeft);
			}
			else
				x.SetThis(x.nMyRight);
		}
		return CIterator(y, m_pObjectInfo->m_nHeader);
	}

	CIterator& Erase(CIterator& it)
	{
		CIterator end = End();
		if(it != end)
		{
			CIterator cur(it++);
			CVmmRbTreeNode& oNode = cur.GetNode();
			oNode.TakeOut(end.GetNode());
			uint64 nNode = oNode.GetThis();
			m_pAccess->Clear(&oNode.GetValue());
			oNode.Clear();
			m_pAllocator->DeAllocate(nNode);
			DecCount();
		}
		return it;
	}

	CIterator EraseKey(TVirtualKey& oKey)
	{
		CIterator it = LowerBound(oKey);
		CIterator &it2 = it;
		while(it2 != End())
		{
			TVirtualKey lKey = m_pGetKey->GetKey(it2.GetValue());
			if(m_pCompare->Compare(lKey, oKey))
				break;
			it2 = Erase(it2);
			if(m_pObjectInfo->m_bUnique)
				break;
		}
		return it2;
	}

	CIterator InsertNode(CVmmRbTreeNode& it, TVirtualKey &oKey, bool bReplace=false)
	{
		uint64 nNode;
		CVmmRbTreeNode y(this);
		CVmmRbTreeNode x(this);

		y.SetThis(m_pObjectInfo->m_nHeader);
		x.SetThis(y.nMyParent);

		TVirtualKey lKey;
		CIterator j;
		bool lessthan = true;
		while(x.GetThis())
		{
			y = x;
			TVirtualKey rKey = m_pGetKey->GetKey(x.GetValue());
			lessthan = (m_pCompare->Compare(oKey, rKey) < 0);
			x.SetThis(lessthan?x.nMyLeft:x.nMyRight);
		}
		if(!m_pObjectInfo->m_bUnique && !bReplace)
			goto ins;
		j = CIterator(y, m_pObjectInfo->m_nHeader);
		if(lessthan)
		{
			if(j == Begin())
				goto ins;
			else --j;
		}
		lKey = m_pGetKey->GetKey(j.GetValue());
		if(m_pCompare->Compare(lKey, oKey) < 0)
			goto ins;
		if(bReplace)
		{
			m_pAccess->Clear(&j.GetValue());
			j.SetValue(it.GetValue());
		}
		else
			m_pAccess->Clear(&it.GetValue());
		nNode = it.GetThis();
		it.Clear();
		m_pAllocator->DeAllocate(nNode);
		return bReplace?j:End();
ins:
		it.Insert(x.GetThis(), y.GetThis(), m_pObjectInfo->m_nHeader, m_pGetKey, m_pCompare);
		IncCount();
		return CIterator(it, m_pObjectInfo->m_nHeader);
	}

	CIterator Insert(TVirtualObject& oSrc, bool bReplace=false, bool* pLack=NULL)
	{
		if(pLack)
			pLack[0] = false;
		CIterator end = End();
		uint64 nNode = m_pAllocator->Allocate(VMM_NODE_SIZE, m_bMemory);
		if(!nNode)
		{
			if(pLack)
				pLack[0] = true;
			return end;
		}
		CVmmRbTreeNode it(this);
		it.SetThis(nNode, true);
		it.SetValue(oSrc);
		TVirtualKey oKey = m_pGetKey->GetKey(oSrc);
		return InsertNode(it, oKey, bReplace);
	}

	bool Verify()
	{
		CVmmRbTreeNode oRoot(this);
		oRoot.SetThis(m_pObjectInfo->m_nHeader);
		if(m_pObjectInfo->m_nCount == 0 || Begin() == End())
			return m_pObjectInfo->m_nCount == 0 && Begin() == End() &&
				oRoot.GetLeft() == m_pObjectInfo->m_nHeader && oRoot.GetRight() == m_pObjectInfo->m_nHeader;
		
		CVmmRbTreeNode oParent(this);
		oParent.SetThis(oRoot.GetParent());

		uint32 nLen = BlackCount(oRoot.nMyLeft, oRoot.nMyParent);

		for (CIterator it = Begin(); it != End(); ++it)
		{
			CVmmRbTreeNode x = it.GetNode();
			CVmmRbTreeNode l = End().GetNode().SetThis(x.GetLeft());
			CVmmRbTreeNode r = End().GetNode().SetThis(x.GetRight());
			TVirtualKey lKey, rKey;
		
			if (x.GetRed())
				if ((l.GetThis() && x.GetRed()) || (r.GetThis() && r.GetRed()))
					return false;
			
			if (l.GetThis())
			{
				lKey = m_pGetKey->GetKey(x.GetValue());
				rKey = m_pGetKey->GetKey(l.GetValue());
				if(m_pCompare->Compare(lKey, rKey) < 0)
					return false;
			}

			if (r.GetThis())
			{
				lKey = m_pGetKey->GetKey(r.GetValue());
				rKey = m_pGetKey->GetKey(x.GetValue());
				if(m_pCompare->Compare(lKey, rKey) < 0)
					return false;
			}
				
			if (!l.GetThis() && !r.GetThis() && BlackCount(x.GetThis(), oRoot.GetParent()) != nLen)
				return false;
		}
		
		if (oRoot.GetLeft() != oParent.MinNode())
			return false;

		if (oRoot.GetRight() != oParent.MaxNode())
			return false;
		
		return true;
	}


private:
	CIterator& Erase(CIterator& oFirst, CIterator &oEnd)
	{
		while(oFirst != oEnd)
			Erase(oFirst);
		return oEnd;
	}

	void IncCount()
	{
		++m_pObjectInfo->m_nCount;
		if(!m_bMemory)
			vflush(m_pObjectInfo);
	}

	void DecCount()
	{
		--m_pObjectInfo->m_nCount;
		if(!m_bMemory)
			vflush(m_pObjectInfo);
	}

	uint32 BlackCount(uint32 nBegin, uint32 nRoot)
	{
		if(!nBegin)
			return 0;
		CVmmRbTreeNode oNode(this);
		oNode.SetThis(nBegin);

		uint32 bc = oNode.GetRed()?0:1;
		if(nBegin == nRoot)
			return bc;
		return bc + BlackCount(oNode.GetParent(), nRoot);
	}

	void ClearSubTree(uint64 nTree)
	{
		while(nTree)
		{
			CVmmRbTreeNode oRoot(this);
			oRoot.SetThis(nTree, true);
			uint64 nLeft = oRoot.GetLeft();
			uint64 nRight = oRoot.GetRight();
			m_pAccess->Clear(&oRoot.GetValue());
			oRoot.Clear();
			m_pAllocator->DeAllocate(nTree);
			ClearSubTree(nRight);
			nTree = nLeft;
		}
	}
};

#undef nMyLeft
#undef nMyRight 
#undef nMyParent
#undef nMyRed
#undef oMyValue

FOCP_END();

#endif
