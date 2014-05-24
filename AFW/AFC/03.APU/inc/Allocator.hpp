
#include "SyncObj.hpp"

#ifndef _APU_ALLOCATOR_HPP_
#define _APU_ALLOCATOR_HPP_

FOCP_BEGIN();

struct CAllocatePolicy
{
	uint32 nInitializeObjectNum;//��ʼ������������Ϊ0��ʾ������ʼ��
	uint32 nMaxObjectNum;//������������Ϊ0xFFFFFFFF��ʾ��������
	uint32 nRecycleSize;//����ջ����������Ϊ0xFFFFFFFF��ʾȫ������
};

class APU_API CBaseClass
{
public:
	virtual ~CBaseClass();
	CBaseClass();

	virtual uint32 GetInstanceSize() const;
	virtual void ResetObject(void* pObject) const;
	virtual void ConstructInstance(void* pBuf) const;
	virtual void DestructInstance(void* pObject) const;
};

class APU_API CPyramidClass: public CBaseClass
{
public:
	virtual ~CPyramidClass();
	CPyramidClass();

	virtual uint32 GetObjectId(void* pObject) const;
	virtual void SetObjectId(void* pBuf, uint32 nId) const;
};

class APU_API CAllocator
{
	FOCP_FORBID_COPY(CAllocator);
private:
	uint32 m_nSize;
	CBaseClass* m_pClass;
	CAllocatePolicy m_oAllocatePolicy;
	uint8* m_pArray;//���������������ԡ�
	CBaseSingleList<uint8> * m_pReclaimer;
	CMutex m_oMutex;

public:
	CAllocator(CBaseClass* pClass, bool bThread=true);
	virtual ~CAllocator();
	//���������������ԣ����и÷���ֵ��
	bool IsArrayContainer() const;
	uint32 GetObjectIndex(void* pData) const;
	const void* QueryObject(uint32 nIdx) const;
	void* CreateObject(bool bLock=true);
	void DestroyObject(void* pObject, bool bLock=true);
	uint32 GetCapacity() const;
	uint32 GetTotalSize() const;
	uint32 GetActiveSize() const;
	bool SetAllocatePolicy(const CAllocatePolicy &oPolicy);
	virtual bool GetDefaultPolicy(CAllocatePolicy &oPolicy, bool &bForbidCustomPolicy);
	void Clear();
};

template<typename TObject> class CGeneralClass: public CBaseClass
{
public:
	inline virtual uint32 GetInstanceSize() const
	{
		return FOCP_SIZE_OF(TObject);
	}

	inline virtual void ConstructInstance(void* pBuf) const
	{
		new(pBuf) TObject();
	}

	inline virtual void DestructInstance(void* pObject) const
	{
		((TObject*)pObject)->~TObject();
	}
};

template<typename TObject> class CGeneralPyramidClass: public CPyramidClass
{
public:
	inline virtual uint32 GetInstanceSize() const
	{
		return FOCP_SIZE_OF(TObject);
	}

	inline virtual void ConstructInstance(void* pBuf) const
	{
		new(pBuf) TObject();
	}

	inline virtual void DestructInstance(void* pObject) const
	{
		((TObject*)pObject)->~TObject();
	}
};

class APU_API CPyramidAllocator
{
public:
	union CMask
	{
		uint32 nFlag;
		uint8 pFlag[4];
	};
	struct CLeafNode
	{
		CMask nMask;
		uint8* pObjects;
	};
	struct CBranchNode
	{
		CMask nMask;
		void* pNodes[32];
	};

private:
	CMutex m_oMutex;
	uint32 m_nLevel, m_nMaxId, m_nInstanceSize, m_nSize;
	void* m_pNode;
	CPyramidClass* m_pClass;

public:
	CPyramidAllocator(CPyramidClass* pClass, bool bThread=true);
	~CPyramidAllocator();

	uint32 GetCapacity();
	uint32 GetSize();

	void* CreateObject();
	void DestroyObject(void* pObject);

	void* QueryObject(uint32 nObjectId);
	void DestroyObject(uint32 nObjectId);

private:
	void* CreateNode(uint32 nLevel);
	void DestroyNode(void* pNode, uint32 nLevel);
	void ParseObjectId(uint32 nObjId, uint32 pIndex[7], void* pNode[7]);
	bool FindFreeChild(CMask &nMask, uint32 &nIndex);
	bool ApplyNode(uint32 pIndex[7], void* pNode[7]);
};

FOCP_DETAIL_BEGIN();

struct CGetBufferAllocatorKey;

class APU_API CBufferClass: public CBaseClass
{
	friend class CBufferAllocator;
	friend struct CGetBufferAllocatorKey;
private:
	uint32 m_nSize;

public:
	CBufferClass();
	CBufferClass(const CBufferClass& oSrc);
	CBufferClass& operator=(const CBufferClass& oSrc);

	virtual uint32 GetInstanceSize() const;
};

class APU_API CBufferAllocator
{
	friend struct CGetBufferAllocatorKey;
private:
	CBufferClass m_oClass;
	CAllocator m_oAllocator;
	bool m_bThread;

public:
	~CBufferAllocator();
	CBufferAllocator(bool bThread=true);
	CBufferAllocator(const CBufferAllocator& oSrc);
	CBufferAllocator& operator=(const CBufferAllocator& oSrc);

	void* AllocateBuffer(bool bLock=true);
	void DeAllocateBuffer(void* pBuffer, bool bLock=true);

	void SetSize(uint32 nSize);
};

struct APU_API CGetBufferAllocatorKey
{
	static const uint32* GetKey(const CBufferAllocator& oData);
};

struct APU_API CApuStreamAllocator: public CMemoryStreamAllocator
{
	virtual CMemStreamNode* Allocate();
	virtual void DeAllocate(CMemStreamNode* pNode);
};

FOCP_DETAIL_END();

class APU_API CBufferManager
{
	friend struct CSingleInstance<CBufferManager>;
private:
	CMutex m_oMutex;
	CRbTree<uint32, FOCP_DETAIL_NAME::CBufferAllocator, FOCP_DETAIL_NAME::CGetBufferAllocatorKey> m_oTree;
	uint32 m_nUnit;
	bool m_bThread;

public:
	CBufferManager(bool bThread=true, uint32 nUnit=sizeof(ulong));
	~CBufferManager();

	void* AllocateBuffer(uint32 nSize, bool bLock=true);
	void DeAllocateBuffer(void* pBuffer, bool bLock=true);

	static CBufferManager* GetInstance();
};

FOCP_END();

#endif // _APU_ALLOCATOR_HPP_
