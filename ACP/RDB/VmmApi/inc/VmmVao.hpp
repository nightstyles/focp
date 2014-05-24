
#include "VmmApi.hpp"

#ifndef _VMM_VAO_HPP_
#define _VMM_VAO_HPP_

FOCP_BEGIN();

template<typename TPersistObject> class CPersistObjectPool
{
private:
	CMutex m_oMutex;

public:
	CPersistObjectPool()
	{
	}

	~CPersistObjectPool()
	{
	}

	TPersistObject* QueryObject(uint64 nHandle, bool bFirst=false)
	{
		TPersistObject* pObject = (TPersistObject*)vmap(nHandle);
		if(bFirst)
		{
			pObject->m_nCounter = 1;
			pObject->m_nDirty = 1;
		}
		else
		{
			m_oMutex.Enter();
			if(pObject->m_nCounter)
				unvmap(pObject);
			++pObject->m_nCounter;
			m_oMutex.Leave();
		}
		return pObject;
	}

	void ReleaseObject(uint64 nHandle, TPersistObject* pObject)
	{
		m_oMutex.Enter();
		if(pObject->m_nCounter)
		{
			--pObject->m_nCounter;
			if(!pObject->m_nCounter)
			{
				if(pObject->m_nDirty)
				{
					pObject->m_nDirty = 0;
					vflush(pObject);
				}
				unvmap(pObject);
			}
		}
		else
			Abort();
		m_oMutex.Leave();
	}

	void AddRef(TPersistObject* pObject)
	{
		m_oMutex.Enter();
		++pObject->m_nCounter;
		m_oMutex.Leave();
	}
};

template<typename TVirtualObject, typename TObjectFactory> class CVirtualObjectPool
{
	struct CObjectNode
	{
		TVirtualObject* pObject;
		uint32 nCounter;
	};
private:
	CMutex m_oMutex;
	CRbMap<uint64, CObjectNode> m_oObjectPool;

public:
	CVirtualObjectPool()
	{
	}

	~CVirtualObjectPool()
	{
		CRbTreeNode* idx = m_oObjectPool.First();
		CRbTreeNode* end = m_oObjectPool.End();
		for(; idx!=end; idx=m_oObjectPool.GetNext(idx))
		{
			CObjectNode& oNode = m_oObjectPool.GetItem(idx);
			delete oNode.pObject;
		}
	}

	TVirtualObject* QueryObject(uint64 nThis, TObjectFactory* pFactory)
	{
		TVirtualObject* pObject;
		m_oMutex.Enter();
		CRbTreeNode* idx = m_oObjectPool.Find(nThis);
		if(idx != m_oObjectPool.End())
		{
			CObjectNode& oNode = m_oObjectPool.GetItem(idx);
			pObject = oNode.pObject;
			++oNode.nCounter;
		}
		else
		{
			CObjectNode& oNode = m_oObjectPool[nThis];
			pObject = oNode.pObject = pFactory->QueryObject(nThis);
			oNode.nCounter = 1;
		}
		m_oMutex.Leave();
		return pObject;
	}

	void ReleaseObject(uint64 nThis)
	{
		m_oMutex.Enter();
		CRbTreeNode* idx = m_oObjectPool.Find(nThis);
		if(idx != m_oObjectPool.End())
		{
			CObjectNode& oNode = m_oObjectPool.GetItem(idx);
			if(!(--oNode.nCounter))
			{
				delete oNode.pObject;
				m_oObjectPool.Remove(idx);
			}
		}
		m_oMutex.Leave();
	}
};

struct CVirtualAccess
{
	virtual void Clear(void*)
	{
	}
};

struct CVirtualAllocator
{
private:
	void* m_pLogicSpace;

public:
	CVirtualAllocator()
	{
		m_pLogicSpace = NULL;
	}

	virtual ~CVirtualAllocator()
	{
		if(m_pLogicSpace)
			DeleteVirtualSpace(m_pLogicSpace);
	}

	bool CreateLogicSpace(int32 bInMemory)
	{
		if(m_pLogicSpace)
			return false;
		m_pLogicSpace = NewVirtualSpace();
		if(!m_pLogicSpace)
			return false;
		bool bRet = CreateVirtualSpace(m_pLogicSpace, bInMemory);
		if(!bRet)
		{
			DeleteVirtualSpace(m_pLogicSpace);
			m_pLogicSpace = NULL;
		}
		return bRet;
	}

	void DestroyLogicSpace()
	{
		if(m_pLogicSpace)
		{
			DestroyVirtualSpace(m_pLogicSpace);
			DeleteVirtualSpace(m_pLogicSpace);
			m_pLogicSpace = NULL;
		}
	}

	virtual uint64 Allocate(uint32 nSaveSize, int32 nInMemory)
	{
		if(m_pLogicSpace)
		{
			if(nInMemory)
				nInMemory = 1;
			if(VirtualSpaceInMemory(m_pLogicSpace) != nInMemory)
				return 0;
			return AllocateFromVirtualSpace(m_pLogicSpace, nSaveSize);
		}
		return vmalloc(nSaveSize, nInMemory);
	}

	virtual void DeAllocate(uint64 nAddr)
	{
		if(m_pLogicSpace)
			DeAllocateIntoVirtualSpace(m_pLogicSpace, nAddr);
		vfree(nAddr);
	}

	virtual void DeAllocateAll()
	{
		if(m_pLogicSpace)
			ClearVirtualSpace(m_pLogicSpace);
	}
};

template<typename TVirtualObject, typename TVirtualKey=TVirtualObject> struct CVirtualGetKey
{
	CVirtualGetKey()
	{
	}

	virtual ~CVirtualGetKey()
	{
	}

	virtual TVirtualKey GetKey(TVirtualObject& oSrc)
	{
		return *(TVirtualKey*)&oSrc;
	}
};

template<typename TVirtualKey> struct CVirtualCompare
{
	CVirtualCompare()
	{
	}

	virtual ~CVirtualCompare()
	{
	}

	virtual int32 Compare(TVirtualKey&l, TVirtualKey& r)
	{
		return CBinary::MemoryCompare(&l, &r, sizeof(TVirtualKey));
	}
};

template<typename TVirtualKey> struct CNumberCompare: public CVirtualCompare<TVirtualKey>
{
	CNumberCompare()
	{
	}

	virtual ~CNumberCompare()
	{
	}

	virtual int32 Compare(TVirtualKey&l, TVirtualKey& r)
	{
		if(l > r)
			return 1;
		if(l < r)
			return -1;
		return 0;
	}
};

FOCP_END();

#endif
