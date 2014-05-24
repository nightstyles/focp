
#include "AdtDef.hpp"

#ifndef _ADT_LIST_HPP_
#define _ADT_LIST_HPP_

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

template<typename TData> class CSingleListDataNode
{
public:
	CSingleListDataNode<TData> *pNext;
	TData oData;

	inline CSingleListDataNode()
	{
	}

	inline CSingleListDataNode(const TData &o)
		:oData(o)
	{
	}
};

template<typename TData> class CDoubleListDataNode
{
public:
	CDoubleListDataNode<TData> *pNext, *pPrev;
	TData oData;

	inline CDoubleListDataNode()
	{
	}

	inline CDoubleListDataNode(const TData &o)
		:oData(o)
	{
	}
};

FOCP_DETAIL_END();

#define FOCP_GET_NEXT(pNode) (*(TNode**)(((uint8*)pNode)+m_nNextOffset))
#define FOCP_GET_PREV(pNode) (*(TNode**)(((uint8*)pNode)+m_nPrevOffset))
#define FOCP_SET_NEXT(pNode, pNextNode) (FOCP_GET_NEXT(pNode) = pNextNode)
#define FOCP_SET_PREV(pNode, pPrevNode) (FOCP_GET_PREV(pNode) = pPrevNode)

template<typename TNode> class CBaseSingleList
{
private:
	TNode *m_pHead, *m_pTail;
	uint32 m_nCount, m_nNextOffset;

	CBaseSingleList(const CBaseSingleList<TNode> &oSrc);
	CBaseSingleList<TNode>& operator=(const CBaseSingleList<TNode> &oSrc);

public:
	inline virtual ~CBaseSingleList()
	{
		Clear();
	}

	inline CBaseSingleList(uint32 nNextOffset)
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		m_nNextOffset = nNextOffset;
	}

	inline void Clear()
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nCount;
	}

	inline TNode* First() const
	{
		return (TNode*)m_pHead;
	}

	inline TNode* Last() const
	{
		return (TNode*)m_pTail;
	}

	inline TNode* GetNext(const TNode* pNode) const
	{
		if(pNode == NULL)
			return NULL;
		return (TNode*)(FOCP_GET_NEXT(pNode));
	}

	inline void SetNext(TNode* pNode, TNode* pNextNode)
	{
		if(pNode)
			FOCP_SET_NEXT(pNode, pNextNode);
	}

	inline void Push(TNode* pNode, bool bAppend=true)
	{
		Append(bAppend?m_pTail:NULL, pNode);
	}

	inline TNode* Pop()
	{
		return RemoveNext(NULL);
	}

	inline void Append(TNode* pNode, TNode* pNewNode)
	{
		if(pNode)
		{
			FOCP_SET_NEXT(pNewNode, FOCP_GET_NEXT(pNode));
			FOCP_SET_NEXT(pNode, pNewNode);
			if(pNode == m_pTail)
				m_pTail = pNewNode;
		}
		else
		{
			FOCP_SET_NEXT(pNewNode, m_pHead);
			if(m_pTail == NULL)
				m_pTail = pNewNode;
			m_pHead = pNewNode;
		}
		++m_nCount;
	}

	inline void AppendList(TNode* pHead, TNode* pTail, bool bAppend=true)//[pHead, pTail]
	{
		if(pHead && pTail)
		{
			if(bAppend)
			{
				if(m_pTail)
					FOCP_SET_NEXT(m_pTail, pHead);
				else
					m_pHead = pHead;
				m_pTail = pTail;
			}
			else
			{
				FOCP_SET_NEXT(pTail, m_pHead);
				m_pHead = pHead;
				if(!m_pTail)
					m_pTail = pTail;
			}
			while(pHead != pTail)
			{
				++m_nCount;
				pHead = FOCP_GET_NEXT(pHead);
			}
			++m_nCount;
		}
	}

	inline TNode* RemoveNext(TNode* pNode)
	{
		if(pNode)
		{
			TNode* pNext = FOCP_GET_NEXT(pNode);
			if(pNext)
			{
				FOCP_SET_NEXT(pNode, FOCP_GET_NEXT(pNext));
				FOCP_SET_NEXT(pNext, NULL);
				if(pNext == m_pTail)
					m_pTail = pNode;
				--m_nCount;
				return pNext;
			}
		}
		else
		{
			pNode = m_pHead;
			if(pNode)
			{
				m_pHead = FOCP_GET_NEXT(pNode);
				FOCP_SET_NEXT(pNode, NULL);
				if(m_pHead == NULL)
					m_pTail = NULL;
				--m_nCount;
				return pNode;
			}
		}
		return NULL;
	}

	//从当前容器中提取(pFromNode, pEndNode]到指定容器oTo中，当前容器会因元素的提取而缩小。
	//pEndNode为空，表示提取pFromNode之后的所有元素。pFromNode为空表示从头开始
	//如果oTo是自己，实际上就是将自己减容。
	inline void Cut(CBaseSingleList<TNode>& oTo, TNode* pFromNode, TNode* pEndNode)//(pFromNode, pEndNode]
	{
		//获取新列表的头尾
		TNode *pHead = pFromNode?FOCP_GET_NEXT(pFromNode):m_pHead;
		TNode *pTail = pEndNode?pEndNode:m_pTail;

		if(pHead == NULL || pTail == NULL)
			oTo.Clear();
		else
		{
			//剪除子列表。
			TNode* pNext = FOCP_GET_NEXT(pTail);
			if(pFromNode)
				FOCP_SET_NEXT(pFromNode, pNext);
			else
				m_pHead = pNext;
			if(pTail == m_pTail)
				m_pTail = pFromNode;

			//构造新的子列表对象
			FOCP_SET_NEXT(pTail, NULL);
			CBaseSingleList<TNode> o(m_nNextOffset);
			o.AppendList(pHead, pTail);

			//更新当前列表的成员个数
			m_nCount -= o.m_nCount;

			//返回子列表对象。
			o.Swap(oTo);
		}
	}

	inline void Swap(CBaseSingleList<TNode>& oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_pHead, oSrc.m_pHead);
			::FOCP_NAME::Swap(m_pTail, oSrc.m_pTail);
			::FOCP_NAME::Swap(m_nCount, oSrc.m_nCount);
			::FOCP_NAME::Swap(m_nNextOffset, oSrc.m_nNextOffset);
		}
	}
};

template<typename TNode> class CBaseDoubleList
{
private:
	TNode *m_pHead, *m_pTail;
	uint32 m_nCount, m_nPrevOffset, m_nNextOffset;

	CBaseDoubleList(const CBaseDoubleList& oSrc);
	CBaseDoubleList<TNode>& operator=(const CBaseDoubleList& oSrc);

public:
	inline virtual ~CBaseDoubleList()
	{
		Clear();
	}

	inline CBaseDoubleList(uint32 nPrevOffset, uint32 nNextOffset)
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		m_nPrevOffset = nPrevOffset;
		m_nNextOffset = nNextOffset;
	}

	inline void Clear()
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nCount;
	}

	inline TNode* First() const
	{
		return (TNode*)m_pHead;
	}

	inline TNode* Last() const
	{
		return (TNode*)m_pTail;
	}

	inline TNode* GetNext(const TNode* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return (TNode*)(FOCP_GET_NEXT(pIt));
	}

	inline TNode* GetPrev(const TNode* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return (TNode*)(FOCP_GET_PREV(pIt));
	}

	inline void SetNext(TNode* pIt, TNode* pNode)
	{
		if(pIt)
			FOCP_SET_NEXT(pIt, pNode);
	}

	inline void SetPrev(TNode* pIt, TNode* pNode)
	{
		if(pIt)
			FOCP_SET_PREV(pIt, pNode);
	}

	inline void Push(TNode* pNode, bool bAppend=true)
	{
		Append(bAppend?m_pTail:NULL, pNode);
	}

	inline TNode* Pop(bool bFromHead=true)
	{
		TNode* pNode = bFromHead?m_pHead:m_pTail;
		Remove(pNode);
		return pNode;
	}

	inline void Insert(TNode* pNode, TNode* pNewNode)
	{
		if(pNode)
		{
			TNode* pPrev = FOCP_GET_PREV(pNode);
			FOCP_SET_PREV(pNewNode, pPrev);
			FOCP_SET_NEXT(pNewNode, pNode);
			FOCP_SET_PREV(pNode, pNewNode);
			if(pPrev)
				FOCP_SET_NEXT(pPrev, pNewNode);
			else
				m_pHead = pNewNode;
		}
		else
		{
			FOCP_SET_NEXT(pNewNode, NULL);
			FOCP_SET_PREV(pNewNode, m_pTail);
			if(m_pTail)
				FOCP_SET_NEXT(m_pTail, pNewNode);
			else
				m_pHead = pNewNode;
			m_pTail = pNewNode;
		}
		++m_nCount;
	}

	inline void Append(TNode* pNode, TNode* pNewNode)
	{
		if(pNode)
		{
			TNode* pNext = FOCP_GET_NEXT(pNode);
			FOCP_SET_NEXT(pNewNode, pNext);
			FOCP_SET_PREV(pNewNode, pNode);
			FOCP_SET_NEXT(pNode, pNewNode);
			if(pNext)
				FOCP_SET_PREV(pNext, pNewNode);
			else
				m_pTail = pNewNode;
		}
		else
		{
			FOCP_SET_NEXT(pNewNode, m_pHead);
			FOCP_SET_PREV(pNewNode, NULL);
			if(m_pHead)
				FOCP_SET_PREV(m_pHead, pNewNode);
			else
				m_pTail = pNewNode;
			m_pHead = pNewNode;
		}
		++m_nCount;
	}

	inline void AppendList(TNode* pHead, TNode* pTail)//[pHead, pTail]
	{
		if(pHead && pTail)
		{
			TNode* pPrev = NULL;
			if(m_pTail)
				FOCP_SET_NEXT(m_pTail, pHead);
			else
				m_pHead = pHead;
			FOCP_SET_PREV(pHead, m_pTail);
			m_pTail = pTail;
			while(pHead != pTail)
			{
				++m_nCount;
				pHead = FOCP_GET_NEXT(pHead);
			}
			++m_nCount;
		}
	}

	inline void Remove(TNode* pNode)
	{
		if(pNode)
		{
			TNode* pPrev = FOCP_GET_PREV(pNode);
			TNode* pNext = FOCP_GET_NEXT(pNode);
			if(pPrev)
				FOCP_SET_NEXT(pPrev, pNext);
			else
				m_pHead = pNext;
			if(pNext)
				FOCP_SET_PREV(pNext, pPrev);
			else
				m_pTail = pPrev;
			--m_nCount;
		}
	}

	//从当前容器中提取[pFromNode, pEndNode)到指定容器oTo中，当前容器会因元素的提取而缩小。
	//pEndNode为空，表示提取pFromNode及之后的所有元素。pFromNode为空表示从头开始
	//如果oTo是自己，实际上就是将自己减容。
	inline void Cut(CBaseDoubleList<TNode>& oTo, TNode* pFromNode, TNode* pEndNode)
	{
		//获取新列表的头尾
		TNode *pHead = pFromNode?pFromNode:m_pHead;
		TNode *pTail = pEndNode?FOCP_GET_PREV(pEndNode):m_pTail;

		if(pHead == NULL || pTail == NULL)
			oTo.Clear();
		else
		{
			//剪除子列表。
			TNode* pPrev = FOCP_GET_PREV(pHead);
			TNode* pNext = FOCP_GET_NEXT(pTail);
			if(pPrev)
				FOCP_SET_NEXT(pPrev, pNext);
			else
				m_pHead = pNext;
			if(pNext)
				FOCP_SET_PREV(pNext, pPrev);
			else
				m_pTail = pPrev;

			//构造新的子列表对象
			FOCP_SET_PREV(pHead, NULL);
			FOCP_SET_NEXT(pTail, NULL);
			CBaseDoubleList<TNode> o(m_nPrevOffset, m_nNextOffset);
			o.AppendList(pHead, pTail);

			//更新当前列表的成员个数
			m_nCount -= o.m_nCount;

			//返回子列表对象。
			o.Swap(oTo);
		}
	}

	inline void Swap(CBaseDoubleList<TNode>& oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_pHead, oSrc.m_pHead);
			::FOCP_NAME::Swap(m_pTail, oSrc.m_pTail);
			::FOCP_NAME::Swap(m_nCount, oSrc.m_nCount);
			::FOCP_NAME::Swap(m_nPrevOffset, oSrc.m_nPrevOffset);
			::FOCP_NAME::Swap(m_nNextOffset, oSrc.m_nNextOffset);
		}
	}
};

#undef FOCP_GET_NEXT
#undef FOCP_SET_NEXT
#undef FOCP_GET_PREV
#undef FOCP_SET_PREV

template<typename TData> class CSingleList
{
	typedef FOCP_DETAIL_NAME::CSingleListDataNode<TData> CNode;
private:
	CNode *m_pHead, *m_pTail;
	uint32 m_nCount;

	inline void InsideConstruct(CNode *pHead, CNode *pTail)
	{
		m_pHead = pHead;
		m_pTail = pTail;
		m_nCount = 0;
		while(pHead)
		{
			pHead = pHead->pNext;
			++m_nCount;
		}
	}

public:
	inline virtual ~CSingleList()
	{
		Clear();
	}

	//拷贝部分元素[pFrom, pEnd)以构造新列表。
	inline CSingleList(const void* pFrom=NULL, const void* pEnd=NULL)
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		CNode* pNode = (CNode*)pFrom;
		if(pNode)while(pNode != pEnd)
		{
			CNode* pNext = pNode->pNext;
			Push(pNode->oData);
			pNode = pNext;
		}
	}

	inline CSingleList(const CSingleList<TData> &oSrc)
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		CSingleList<TData> o((const void*)oSrc.m_pHead, (const void*)NULL);
		Swap(o);
	}

	inline CSingleList<TData>& operator=(const CSingleList<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			CSingleList<TData> o(oSrc);
			Swap(o);
		}
		return *this;
	}

	inline void Clear()
	{
		while(m_pHead)
		{
			m_pTail = m_pHead->pNext;
			delete m_pHead;
			m_pHead = m_pTail;
		}
		m_nCount = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nCount;
	}

	inline TData* At(void* pIt)
	{
		if(pIt == NULL)
			return NULL;
		return &((CNode*)pIt)->oData;
	}

	inline TData& GetItem(void* pIt)
	{
		return *At(pIt);
	}

	inline const TData* At(const void* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return &((CNode*)pIt)->oData;
	}

	inline const TData& GetItem(const void* pIt) const
	{
		return *At(pIt);
	}

	inline void* First() const
	{
		return (void*)m_pHead;
	}

	inline void* Last() const
	{
		return (void*)m_pTail;
	}

	inline void* GetNext(const void* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return ((CNode*)pIt)->pNext;
	}

	//可指定从尾部还是头部加对象。
	inline void* Push(const TData& oData, bool bAppend=true)
	{
		CNode* pNode = new CNode(oData);
		AppendNode(bAppend?m_pTail:NULL, pNode);
		return pNode;
	}

	//从头取对象
	inline bool Pop(TData& oData)
	{
		CNode* pNode = PopNextNode(NULL);
		if(pNode == NULL)
			return false;
		oData = pNode->oData;
		delete pNode;
		return true;
	}

	//插入pIt之后，如果pIt为空，插到头部
	inline void* Append(void* pIt, const TData& oData)
	{
		CNode* pNode = new CNode(oData);
		AppendNode(pIt, pNode);
		return pNode;
	}

	//将oSrc中所有元素挪到pIt之后，如果pIt为空，则插到头部。
	inline void Append(void* pIt, CSingleList<TData>& oSrc)
	{
		if(&oSrc == this)
		{
			CSingleList<TData> o(oSrc);
			Append(pIt, o);
		}
		else if(oSrc.m_nCount)
		{
			CNode* pNode = (CNode*)pIt;
			if(pNode)
			{
				oSrc.m_pTail->pNext = pNode->pNext;
				pNode->pNext = oSrc.m_pHead;
				if(pNode == m_pTail)
					m_pTail = oSrc.m_pTail;
			}
			else
			{
				oSrc.m_pTail->pNext = m_pHead;
				if(m_pTail == NULL)
					m_pTail = oSrc.m_pTail;
				m_pHead = oSrc.m_pHead;
			}
			m_nCount += oSrc.m_nCount;
			oSrc.m_pHead = NULL;
			oSrc.m_pTail = NULL;
			oSrc.m_nCount = 0;
		}
	}

	//删除pIt的下一个元素，如果pIt为空，则删除头部第一个元素。
	inline void RemoveNext(void* pIt)
	{
		CNode* pNode = PopNextNode((CNode*)pIt);
		if(pNode)
			delete pNode;
	}

	//从当前容器中提取(pFrom, pEnd]到指定容器oTo中，当前容器会因元素的提取而缩小。
	//pEnd为空，表示提取pFrom之后的所有元素。pFrom为空表示从头开始
	//如果oTo是自己，实际上就是将自己减容。
	inline void Cut(CSingleList<TData>& oTo, void* pFrom, void* pEnd)
	{
		CNode* pFromNode = (CNode*)pFrom;
		CNode* pEndNode = (CNode*)pEnd;

		//获取新列表的头尾
		CNode *pHead = pFromNode?pFromNode->pNext:m_pHead;
		CNode *pTail = pEndNode?pEndNode:m_pTail;

		if(pHead == NULL || pTail == NULL)
			oTo.Clear();
		else
		{
			//剪除子列表。
			if(pFromNode)
				pFromNode->pNext = pTail->pNext;
			else
				m_pHead = pTail->pNext;
			if(pTail == m_pTail)
				m_pTail = pFromNode;

			//构造新的子列表对象
			pTail->pNext = NULL;
			CSingleList<TData> o;
			o.InsideConstruct(pHead, pTail);

			//更新当前列表的成员个数
			m_nCount -= o.m_nCount;

			//返回子列表对象。
			o.Swap(oTo);
		}
	}

	inline void Swap(CSingleList<TData>& oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_pHead, oSrc.m_pHead);
			::FOCP_NAME::Swap(m_pTail, oSrc.m_pTail);
			::FOCP_NAME::Swap(m_nCount, oSrc.m_nCount);
		}
	}

protected:
	inline void AppendNode(void* pIt, CNode* pNewNode)
	{
		CNode* pNode = (CNode*)pIt;
		if(pNode)
		{
			pNewNode->pNext = pNode->pNext;
			pNode->pNext = pNewNode;
			if(pNode == m_pTail)
				m_pTail = pNewNode;
		}
		else
		{
			pNewNode->pNext = m_pHead;
			if(m_pTail == NULL)
				m_pTail = pNewNode;
			m_pHead = pNewNode;
		}
		++m_nCount;
	}

	inline CNode* PopNextNode(CNode* pNode)
	{
		if(pNode)
		{
			CNode* pNext = pNode->pNext;
			if(pNext)
			{
				pNode->pNext = pNext->pNext;
				if(pNext == m_pTail)
					m_pTail = pNode;
				--m_nCount;
				return pNext;
			}
		}
		else
		{
			pNode = m_pHead;
			if(pNode)
			{
				m_pHead = pNode->pNext;
				if(m_pHead == NULL)
					m_pTail = NULL;
				--m_nCount;
				return pNode;
			}
		}
		return NULL;
	}
};

template<typename TData> class CLoopList
{
private:
	CSingleList<TData> m_oList;
	void *m_pHead, *m_pTail;
	uint32 m_nSize;

public:
	inline virtual ~CLoopList()
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nSize = 0;
	}

	inline CLoopList()
	{
		TData oData;
		m_nSize = 0;
		m_oList.Push(oData);
		m_pHead = m_oList.First();
		m_pTail = m_pHead;
	}

	inline CLoopList(const CLoopList<TData> &oSrc)
		:m_oList(oSrc.m_oList),m_nSize(oSrc.m_nSize)
	{
		m_pHead = m_oList.First();
		m_pTail = m_pHead;
		uint32 nCount = 0;
		void* p	= oSrc.m_oList.First();
		bool bFill[2] = {false,false};
		while(p && nCount < 2)
		{
			if(p == oSrc.m_pHead)
			{
				++nCount;
				bFill[0] = true;
			}
			else if(p == oSrc.m_pTail)
			{
				++nCount;
				bFill[1] = true;
			}
			p = oSrc.m_oList.GetNext(p);
			if(bFill[0] == false)
				m_pHead = m_oList.GetNext(m_pHead);
			if(bFill[1] == false)
				m_pTail = m_oList.GetNext(m_pTail);
		}
	}

	inline CLoopList<TData>& operator=(const CLoopList<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			CLoopList<TData> oRet(oSrc);
			Swap(oRet);
		}
		return *this;
	}

	inline void Clear()
	{
		TData oData;
		m_oList.Clear();
		m_nSize = 0;
		m_oList.Push(oData);
		m_pHead = m_oList.First();
		m_pTail = m_pHead;
	}

	inline uint32 GetSize() const
	{
		return m_nSize;
	}

	inline void Push(const TData& oData)
	{
		TData& oData2 = m_oList.GetItem(m_pTail);
		oData2 = oData;
		void* pNext = GetNext(m_pTail);
		if(pNext == m_pHead)
		{
			TData xData;
			pNext = m_oList.Append(m_pTail, xData);
		}
		m_pTail = pNext;
		++m_nSize;
	}

	inline bool Pop(TData& oData)
	{
		if(m_nSize == 0)
			return false;
		oData = m_oList.GetItem(m_pHead);
		m_pHead = GetNext(m_pHead);
		--m_nSize;
		return true;
	}

	inline TData* Head()
	{
		if(m_nSize == 0)
			return NULL;
		TData& oData = m_oList.GetItem(m_pHead);
		return &oData;
	}

	inline void Pop()
	{
		if(m_nSize)
		{
			m_pHead = GetNext(m_pHead);
			--m_nSize;
		}
	}

	inline TData* Tail()
	{
		TData& oData = m_oList.GetItem(m_pTail);
		return &oData;
	}

	inline void Push()
	{
		void* pNext = GetNext(m_pTail);
		if(pNext == m_pHead)
		{
			TData xData;
			pNext = m_oList.Append(m_pTail, xData);
		}
		m_pTail = pNext;
		++m_nSize;
	}

	inline void Swap(CLoopList<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			m_oList.Swap(oSrc.m_oList);
			::FOCP_NAME::Swap(m_pHead, oSrc.m_pHead);
			::FOCP_NAME::Swap(m_pTail, oSrc.m_pTail);
			::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
		}
	}

private:
	inline void* GetNext(void* pIt)
	{
		if(pIt)
		{
			pIt = m_oList.GetNext(pIt);
			if(pIt == NULL)
				pIt = m_oList.First();
		}
		return pIt;
	}
};

template<typename TData> class CDoubleList
{
	typedef FOCP_DETAIL_NAME::CDoubleListDataNode<TData> CNode;
private:
	CNode *m_pHead, *m_pTail;
	uint32 m_nCount;

	inline void InsideConstruct(CNode* pHead, CNode* pTail)
	{
		m_pHead = pHead;
		m_pTail = pTail;
		m_nCount = 0;
		while(pHead)
		{
			pHead = pHead->pNext;
			++m_nCount;
		}
	}

public:
	inline virtual ~CDoubleList()
	{
		Clear();
	}

	inline CDoubleList(const void* pFrom=NULL, const void* pEnd=NULL)
	{
		const CNode* pNode = (const CNode*)pFrom;
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		if(pNode)while(pNode != pEnd)
		{
			const CNode* pNext = pNode->pNext;
			Push(pNode->oData);
			pNode = pNext;
		}
	}

	inline CDoubleList(const CDoubleList<TData> &oSrc)
	{
		m_pHead = NULL;
		m_pTail = NULL;
		m_nCount = 0;
		CDoubleList o((const void*)oSrc.m_pHead, (const void*)NULL);
		Swap(o);
	}

	inline CDoubleList<TData>& operator=(const CDoubleList<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			CDoubleList<TData> o(oSrc);
			Swap(o);
		}
		return *this;
	}

	inline void Clear()
	{
		while(m_pHead)
		{
			m_pTail = m_pHead->pNext;
			delete m_pHead;
			m_pHead = m_pTail;
		}
		m_nCount = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nCount;
	}

	inline TData* At(void* pIt)
	{
		if(pIt == NULL)
			return NULL;
		return &((CNode*)pIt)->oData;
	}

	inline TData& GetItem(void* pIt)
	{
		return *At(pIt);
	}

	inline const TData* At(const void* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return &((CNode*)pIt)->oData;
	}

	inline const TData& GetItem(const void* pIt) const
	{
		return *At(pIt);
	}

	inline void* First() const
	{
		return (void*)m_pHead;
	}

	inline void* Last() const
	{
		return (void*)m_pTail;
	}

	inline void* GetNext(const void* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return ((CNode*)pIt)->pNext;
	}

	inline void* GetPrev(const void* pIt) const
	{
		if(pIt == NULL)
			return NULL;
		return ((CNode*)pIt)->pPrev;
	}

	//可指定从尾部加还是头加。
	inline void* Push(const TData& oData, bool bAppend=true)
	{
		CNode* pNode = new CNode(oData);
		AppendNode(bAppend?m_pTail:NULL, pNode);
		return pNode;
	}

	//可指定从尾部还是头部取对象。
	inline bool Pop(TData& oData, bool bFromHead=true)
	{
		CNode* pNode = bFromHead?m_pHead:m_pTail;
		if(pNode)
		{
			oData = pNode->oData;
			Remove(pNode);
			return true;
		}
		return false;
	}

	//插入pIt之后，如果pIt为空，插到头部
	inline void* Append(void* pIt, const TData& oData)
	{
		CNode* pNode = new CNode(oData);
		AppendNode(pIt, pNode);
		return pNode;
	}

	//将oSrc中所有元素挪到pIt之后，如果pIt为空，插到头部
	inline void Append(void* pIt, CDoubleList<TData>& oSrc)
	{
		if(this == &oSrc)
		{
			CDoubleList<TData> o(oSrc);
			Append(pIt, o);
		}
		else if(oSrc.m_nCount)
		{
			CNode* pNode = (CNode*)pIt;
			if(pNode)
			{
				CNode* pNext = pNode->pNext;
				oSrc.m_pTail->pNext = pNext;
				oSrc.m_pHead->pPrev = pNode;
				if(pNext)
					pNext->pPrev = oSrc.m_pTail;
				else
					m_pTail = oSrc.m_pTail;
				pNode->pNext = oSrc.m_pHead;
			}
			else
			{
				oSrc.m_pTail->pNext = m_pHead;
				if(m_pHead)
					m_pHead->pPrev = oSrc.m_pTail;
				else
					m_pTail = oSrc.m_pTail;
				m_pHead = oSrc.m_pHead;
			}
			m_nCount += oSrc.m_nCount;
			oSrc.m_pHead = NULL;
			oSrc.m_pTail = NULL;
			oSrc.m_nCount = 0;
		}
	}

	//将对象插入pIt之前，如果pIt为空，插到尾部
	inline void* Insert(void* pIt, const TData& oData)
	{
		CNode* pNode = new CNode(oData);
		InsertNode(pIt, pNode);
		return pNode;
	}

	//将oSrc中所有元素挪到pIt之前，如果pIt为空，插到尾部
	inline void Insert(void* pIt, CDoubleList<TData>& oSrc)
	{
		if(this != &oSrc)
		{
			CDoubleList<TData> o(oSrc);
			Insert(pIt, o);
		}
		else if(oSrc.m_nCount)
		{
			CNode* pNode = (CNode*)pIt;
			if(pNode)
			{
				CNode* pPrev = pNode->pPrev;
				oSrc.m_pTail->pNext = pNode;
				oSrc.m_pHead->pPrev = pPrev;
				pNode->pPrev = oSrc.m_pTail;
				if(pPrev)
					pPrev->pNext = oSrc.m_pHead;
				else
					m_pHead = oSrc.m_pHead;
			}
			else
			{
				oSrc.m_pHead->pPrev = m_pTail;
				if(m_pTail)
					m_pTail->pNext = oSrc.m_pHead;
				else
					m_pHead = oSrc.m_pHead;
				m_pTail = oSrc.m_pTail;
			}
			m_nCount += oSrc.m_nCount;
			oSrc.m_pHead = NULL;
			oSrc.m_pTail = NULL;
			oSrc.m_nCount = 0;
		}
	}

	//删除指定元素
	inline void Remove(void* pIt)
	{
		CNode* pNode = RemoveNode((CNode*)pIt);
		if(pNode)
			delete pNode;
	}

	//从当前容器中提取[pFrom, pEnd)到指定容器oTo中，当前容器会因元素的提取而缩小。
	//pEnd为空，表示提取pFrom及之后的所有元素。pFrom为空表示从头开始
	//如果oTo是自己，实际上就是将自己减容。
	inline void Cut(CDoubleList<TData>& oTo, void* pFrom, void* pEnd)
	{
		CNode* pFromNode = (CNode*)pFrom;
		CNode* pEndNode = (CNode*)pEnd;

		//获取新列表的头尾
		CNode *pHead = pFromNode?pFromNode:m_pHead;
		CNode *pTail = pEndNode?pEndNode->pPrev:m_pTail;

		if(pHead == NULL || pTail == NULL)
			oTo.Clear();
		else
		{
			//剪除子列表。
			CNode* pPrev = pHead->pPrev;
			CNode* pNext = pTail->pNext;
			if(pPrev)
				pPrev->pNext = pNext;
			else
				m_pHead = pNext;
			if(pNext)
				pNext->pPrev = pPrev;
			else
				m_pTail = pPrev;

			//构造新的子列表对象
			pHead->pPrev = NULL;
			pTail->pNext = NULL;
			CDoubleList<TData> o;
			o.InsideConstruct(pHead, pTail);

			//更新当前列表的成员个数
			m_nCount -= o.m_nCount;

			//返回子列表对象。
			o.Swap(oTo);
		}
	}

	inline void Swap(CDoubleList<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_pHead, oSrc.m_pHead);
			::FOCP_NAME::Swap(m_pTail, oSrc.m_pTail);
			::FOCP_NAME::Swap(m_nCount, oSrc.m_nCount);
		}
	}

protected:
	inline void AppendNode(void* pIt, CNode* pNewNode)
	{
		CNode* pNode = (CNode*)pIt;
		if(pNode)
		{
			CNode* pNext = pNode->pNext;
			pNewNode->pNext = pNext;
			pNewNode->pPrev = pNode;
			pNode->pNext = pNewNode;
			if(pNext)
				pNext->pPrev = pNewNode;
			else
				m_pTail = pNewNode;
		}
		else
		{
			pNewNode->pNext = m_pHead;
			pNewNode->pPrev = NULL;
			if(m_pHead)
				m_pHead->pPrev = pNewNode;
			else
				m_pTail = pNewNode;
			m_pHead = pNewNode;
		}
		++m_nCount;
	}

	inline void InsertNode(void* pIt, CNode* pNewNode)
	{
		CNode* pNode = (CNode*)pIt;
		if(pNode)
		{
			CNode* pPrev = pNode->pPrev;
			pNewNode->pPrev = pPrev;
			pNewNode->pNext = pNode;
			pNode->pPrev = pNewNode;
			if(pPrev)
				pPrev->pNext = pNewNode;
			else
				m_pHead = pNewNode;
		}
		else
		{
			pNewNode->pNext = NULL;
			pNewNode->pPrev = m_pTail;
			if(m_pTail)
				m_pTail->pNext = pNewNode;
			else
				m_pHead = pNewNode;
			m_pTail = pNewNode;
		}
		++m_nCount;
	}

	inline CNode* RemoveNode(CNode* pNode)
	{
		if(pNode)
		{
			CNode* pPrev = pNode->pPrev;
			CNode* pNext = pNode->pNext;
			if(pPrev)
				pPrev->pNext = pNext;
			else
				m_pHead = pNext;
			if(pNext)
				pNext->pPrev = pPrev;
			else
				m_pTail = pPrev;
			--m_nCount;
		}
		return pNode;
	}
};

FOCP_END();

#endif //_ADT_LIST_HPP_
