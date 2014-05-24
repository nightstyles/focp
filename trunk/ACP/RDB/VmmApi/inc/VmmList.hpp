
#include "VmmVao.h"

#ifndef _VMM_LIST_HPP_
#define _VMM_LIST_HPP_

FOCP_BEGIN();

struct CVmmListInfo
{
	uint64 m_nList;
	uint32 m_nSize;
	uint32 m_nReserved;
};

template<typename TVirtualObject> class CVmmList
{
public:
	struct CVmmListNode
	{
		uint64 nPrevNode;
		uint64 nNextNode;
		uint32 m_nDirty, m_nCounter;
		TVirtualObject oObject;
	};

	typedef CVmmList<TVirtualObject> TList;
	typedef CPersistObjectPool<CVmmListNode> TNodePool;

	enum {VMM_NODE_SIZE = sizeof(CVmmListNode)};
	enum {VMM_OBJECT_SIZE = sizeof(CVmmListInfo)};

	class CIterator
	{
	private:
		uint64 m_nThis;
		TList* m_pList;
		CVmmListNode* m_pNode;
		int32 m_bMemory;

	public:
		~CIterator()
		{
			if(m_pNode)
			{
				if(!m_bMemory)
					m_pList->DeAllocateNode(m_nThis, m_pNode);
				m_pNode = NULL;
			}
		}

		CIterator()
		{
			m_nThis = 0;
			m_pList = NULL;
			m_pNode = NULL;
			m_bMemory = 0;
		}

		CIterator(TList* pList)
		{
			m_nThis = 0;
			m_pList = pList;
			m_pNode = NULL;
			m_bMemory = 0;
		}

		CIterator(CIterator& oSrc)
		{
			m_nThis = oSrc.m_nThis;
			m_pList = oSrc.m_pList;
			m_pNode = oSrc.m_pNode;
			m_bMemory = oSrc.m_bMemory;
			if(m_pNode && !m_bMemory)
				m_pList->AddRef(m_pNode);
		}
		
		CIterator& operator=(CIterator& oSrc)
		{
			if(this != &oSrc)
			{
				if(m_pNode && !m_bMemory)
					m_pList->DeAllocateNode(m_nThis, m_pNode);
				m_nThis = oSrc.m_nThis;
				m_pList = oSrc.m_pList;
				m_pNode = oSrc.m_pNode;
				m_bMemory = oSrc.m_bMemory;
				if(m_pNode && !m_bMemory)
					m_pList->AddRef(m_pNode);
			}
			return *this;
		}

		TList* GetList()
		{
			return m_pList;
		}

		uint64 GetThis()
		{
			return m_nThis;
		}

		CIterator& SetThis(uint64 nThis, bool bFirst = false)
		{
			if(m_nThis != nThis)
			{
				if(m_pNode)
				{
					if(!m_bMemory)
						m_pList->DeAllocateNode(m_nThis, m_pNode);
					m_pNode = NULL;
				}
				m_nThis = nThis;
				if(nThis)
				{
					if(m_bMemory)
						m_pNode = GET_MEMORY_OBJECT(CVmmListNode, nThis);
					else
						m_pNode = m_pList->AllocateNode(nThis, bFirst);
				}
			}
			return *this;
		}

		CIterator& operator++ ()
		{
			if(m_nThis)
				SetThis(GetNextNode());
			return *this;
		}

		CIterator& operator-- ()
		{
			if(m_nThis)
				SetThis(GetPrevNode());
			return *this;
		}

		CIterator operator++ (int)
		{
			CIterator oRet(*this);
			operator++();
			return oRet;
		}

		CIterator operator-- (int)
		{
			CIterator oRet(*this);
			operator--();
			return oRet;
		}

		bool operator == (CIterator &oSrc)
		{
			return (m_nThis == oSrc.m_nThis);
		}

		bool operator != (CIterator &oSrc)
		{
			return (m_nThis != oSrc.m_nThis);
		}

		CVmmListNode& GetNode()
		{
			return *m_pNode;
		}

		uint64 GetPrevNode()
		{
			return m_pNode->nPrevNode;
		}

		uint64 GetNextNode()
		{
			return m_pNode->nNextNode;
		}

		TVirtualObject& GetValue()
		{
			return m_pNode->oObject;
		}

		void SetPrevNode(uint64 nPrev)
		{
			if(m_pNode->nPrevNode != nPrev)
			{
				m_pNode->nPrevNode = nPrev;
				m_pNode->m_nDirty = 1;
			}
		}

		void SetNextNode(uint64 nNext)
		{
			if(m_pNode->nNextNode != nNext)
			{
				m_pNode->nNextNode = nNext;
				m_pNode->m_nDirty = 1;
			}
		}

		void SetValue(TVirtualObject& oValue)
		{
			if(&m_pNode->oObject != &oValue)
				m_pNode->oObject = oValue;
			m_pNode->m_nDirty = 1;
		}

		void SetNode(CVmmListNode& oNode)
		{
			if(m_pNode != &oNode)
				*m_pNode = oNode;
			m_pNode->m_nDirty = 1;
		}

		bool IsEnd()
		{
			return (m_nThis == m_pList->GetListInfo()->m_nList);
		}

	};

private:
	uint64 m_nThis;
	uint32 m_bMemory;
	CVmmListInfo* m_pObjectInfo;
	CVirtualAccess* m_pAccess;
	CVirtualAllocator* m_pAllocator;	// for the node object;
	CVirtualAllocator* m_pTopAllocator; // for the list object
	TNodePool m_oNodePool;

	CVmmList(TList& oSrc);
	TList& operator=(TList& oSrc);

public:
	~CVmmList()
	{
		if(m_pObjectInfo)
		{
			if(!m_bMemory)
				unvmap(m_pObjectInfo);
			m_pObjectInfo = NULL;
		}
	}

	CVmmList(CVirtualAccess* pAccess, 
		CVirtualAllocator* pAllocator, 
		CVirtualAllocator* pTopAllocator=NULL)
	{
		m_bMemory = 0;
		m_nThis = 0;
		m_pObjectInfo = NULL;
		m_pAccess = pAccess;
		m_pAllocator = pAllocator;
		m_pTopAllocator = pTopAllocator;
	}

	CVmmListNode* AllocateNode(uint64 nThis, bool bFirst)
	{
		return m_oNodePool.QueryObject(nThis, bFirst);
	}

	void DeAllocateNode(uint64 nThis, CVmmListNode* pNode)
	{
		m_oNodePool.ReleaseObject(nThis, pNode);
	}

	void AddRef(CVmmListNode* pNode)
	{
		m_oNodePool.AddRef(pNode);
	}

	CVmmListInfo* GetListInfo()
	{
		return m_pObjectInfo;
	}

	uint64 GetThis()
	{
		return m_nThis;
	}

	TList& SetThis(uint64 nThis)
	{
		if(!m_nThis && nThis)
		{
			m_nThis = nThis;
			m_bMemory = ((nThis&VMM_MEMORY_FLAG)?1:0);
			if(m_bMemory)
				m_pObjectInfo = GET_MEMORY_OBJECT(CVmmListInfo, nThis);
			else
				m_pObjectInfo = (CVmmListInfo*)vmap(nThis);
		}
		return *this;
	}

	bool CreateObject(int32 bInMemory)
	{
		if(m_nThis)
			return false;
		m_bMemory = bInMemory;
		if(m_pTopAllocator)
			m_nThis = m_pTopAllocator->Allocate((uint32)VMM_OBJECT_SIZE + VMM_NODE_SIZE, bInMemory);
		else
			m_nThis = vmalloc((uint32)VMM_OBJECT_SIZE + VMM_NODE_SIZE, bInMemory);
		if(!m_nThis)
			return false;
		if(m_bMemory)
			m_pObjectInfo = GET_MEMORY_OBJECT(CVmmListInfo, m_nThis);
		else
			m_pObjectInfo = (CVmmListInfo*)vmap(m_nThis);
		m_pObjectInfo->m_nList = m_nThis + VMM_OBJECT_SIZE;
		m_pObjectInfo->m_nSize = 0;
		if(!m_bMemory)
			vflush(m_pObjectInfo);
		CIterator it = End();
		it.SetPrevNode(m_pObjectInfo->m_nList);
		it.SetNextNode(m_pObjectInfo->m_nList);
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

	void Clear()
	{
		if(m_nThis && m_pObjectInfo->m_nSize)
		{
			CIterator idx = Begin();
			while(idx != End())
				Erase(idx);
		}
	}

	void Truncate()//dangerous function
	{
		if(m_nThis && m_pObjectInfo->m_nSize)
		{
			{
				CIterator end = End();
				end.SetPrevNode(m_pObjectInfo->m_nList);
				end.SetNextNode(m_pObjectInfo->m_nList);
			}
			m_pObjectInfo->m_nSize = 0;
			if(!m_bMemory)
				vflush(m_pObjectInfo);
		}
	}

	uint32 GetSize()
	{
		if(!m_nThis)
			return 0;
		return m_pObjectInfo->m_nSize;
	}

	CIterator Begin()
	{
		return ++End();
	}

	CIterator End()
	{
		return CIterator(this).SetThis(m_pObjectInfo->m_nList);
	}

	CIterator GetIterator(uint64 nNode)
	{
		return CIterator(this).SetThis(nNode);
	}

	CIterator& Erase(CIterator& it, bool bDetach=false)
	{
		uint64 nNode;
		if(!m_nThis || it.GetList() != this)
			Abort();

		CIterator end(End());
		if(it == end)
			return it;

		CIterator oPrev(it), oNext(it);
		--oPrev; ++oNext;
		oPrev.SetNextNode(it.GetNextNode());
		oNext.SetPrevNode(it.GetPrevNode());
		if(!bDetach)
		{
			nNode = it.GetThis();
			m_pAccess->Clear(&it.GetValue());
		}
		it = oNext;
		if(!bDetach)
			m_pAllocator->DeAllocate(nNode);

		--m_pObjectInfo->m_nSize;
		if(!m_bMemory)
			vflush(m_pObjectInfo);

		return it;
	}

	CIterator Insert(CIterator &it, TVirtualObject& oValue)
	{
		if(!m_nThis || it.GetList() != this)
			Abort();
		uint64 nNode = m_pAllocator->Allocate(VMM_NODE_SIZE, m_bMemory);
		if(!nNode)
			return End();

		CIterator oPrev(it); --oPrev;
		CIterator oRet(this);
		oRet.SetThis(nNode, true);
		oRet.SetValue(oValue);
		oPrev.SetNextNode(nNode);
		it.SetPrevNode(nNode);
		oRet.SetPrevNode(oPrev.GetThis());
		oRet.SetNextNode(it.GetThis());

		++m_pObjectInfo->m_nSize;
		if(!m_bMemory)
			vflush(m_pObjectInfo);
		return oRet;
	}
};

FOCP_END();

#endif
