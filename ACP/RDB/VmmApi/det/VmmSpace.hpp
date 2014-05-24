
#include "VmmRbTree.hpp"
#include "VmmStorage.hpp"

#ifndef _VMM_SPACE_HPP_
#define _VMM_SPACE_HPP_

FOCP_BEGIN();

class CVirtualSpace
{
public:
	CVirtualSpace();
	virtual ~CVirtualSpace();

	virtual bool CreateObject(int32 bInMemory)=0;
	virtual void DestroyObject()=0;

	virtual uint64 GetThis()=0;
	virtual CVirtualSpace& SetThis(uint64 nThis)=0;

	virtual int32 InMemory()=0;

	virtual uint64 Allocate(uint32 nSize)=0;
	virtual void DeAllocate(uint64 nAddr)=0;
	virtual void DeAllocateAll()=0;
};

struct CSmallSpaceNode
{
	uint64 nPageAddr;
	uint32 nBlockSize;
};

struct CGetSmallPageKey: public CVirtualGetKey<CSmallSpaceNode, uint32>
{
	virtual uint32 GetKey(CSmallSpaceNode& oSrc);
};

struct CSmallSpaceAccess: public CVirtualAccess
{
	virtual void Clear(void*);
};

struct CGlobalSpaceAllocator: public CVirtualAllocator
{
	virtual uint64 Allocate(uint32 nSaveSize, int32 nInMemory);
	virtual void DeAllocate(uint64 nAddr);
};

struct CSmallSpaceHead
{
	CVmmBaseRbTreeInfo oTree;
	CVmmBasicRbTreeNode<CSmallSpaceNode> oRoot;
};

class CSmallSpace: public CVirtualSpace
{
private:
	int32 m_bInMemory;
	CMutex m_oMutex;
	uint32 m_nBlockSize, m_nTotalBlock;

	CVmmRbTree<CSmallSpaceNode, uint32> m_oSpace;

public:
	CSmallSpace();
	virtual ~CSmallSpace();

	virtual bool CreateObject(int32 bInMemory);
	virtual void DestroyObject();

	virtual uint64 GetThis();
	virtual CVirtualSpace& SetThis(uint64 nThis);

	virtual int32 InMemory();

	virtual uint64 Allocate(uint32 nSize);
	virtual void DeAllocate(uint64 nAddr);
	virtual void DeAllocateAll();

private:
	uint64 AllocateNewPage(uint32 nTotalBlock, uint32 nBlockSize);
	uint32 InsertPage(uint64 nPageAddr, uint32 nBlockSize);
	CVmmRbTree<CSmallSpaceNode, uint32>::CIterator FindBlock(uint64 nPageAddr, uint32 nBlockSize);
	uint32 AllocatePagePair();
	uint64 TryAllocate(uint32 nBlockSize);
	bool CreateGlobalSpace();
};

struct CBigSpaceNode
{
	uint16 nBlockSize;
	uint16 nPageSize;
	uint16 nPageCapacity;
	uint16 nCurPageNum;
	uint16 nCurPage;
	uint16 nNullPage;
	uint32 nFreeRecNum;
};

struct CBigPageHead
{
	uint64 nMainPage;
	uint32 nIdx;
};

struct CBigSpacePage
{
	uint64 nPageAddr;
	uint16 nFreeNum;
	uint16 nAllocator;
	uint16 nPrevPage;
	uint16 nNextPage;
};

struct CGetBigPageKey: public CVirtualGetKey<CBigSpaceNode, uint32>
{
	virtual uint32 GetKey(CBigSpaceNode& oSrc);
};

struct CBigSpaceAllocator: public CVirtualAllocator
{
	virtual uint64 Allocate(uint32 nSaveSize, int32 nInMemory);
	virtual void DeAllocate(uint64 nAddr);
};

struct CBigSpaceAccess: public CVirtualAccess
{
	virtual void Clear(void*);
};

class CBigSpace: public CVirtualSpace
{
private:
	int32 m_bInMemory;
	CMutex m_oMutex;
	CVmmRbTree<CBigSpaceNode, uint32> m_oSpace;

public:
	CBigSpace();
	virtual ~CBigSpace();

	virtual bool CreateObject(int32 bInMemory);
	virtual void DestroyObject();

	virtual uint64 GetThis();
	virtual CVirtualSpace& SetThis(uint64 nThis);

	virtual int32 InMemory();

	virtual uint64 Allocate(uint32 nSize);
	virtual void DeAllocate(uint64 nAddr);
	virtual void DeAllocateAll();

private:
	uint64 TryAllocate(uint32 nBlockSize, uint64 &nPageAddr);
	uint64 AllocateMainPage(uint32 nBlockSize);
	uint64 Allocate(CBigSpaceNode& oNode);
	uint32 AppendPage(CBigSpaceNode& oNode, uint64 nMainPage);
	uint64 GetMainPage(uint64 nPageAddr, uint32& nIdx);
	void DeAllocate(uint64 nMainPage, uint32 nIdx, uint32 nOffset);
	void MovePage(CBigSpacePage* pPages, uint16 nIdx, uint16& nFrom, uint16& nTo);
};

FOCP_END();

#endif
