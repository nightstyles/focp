
#include "Allocator.hpp"

#ifndef _APU_OBJECT_HPP_
#define _APU_OBJECT_HPP_

FOCP_BEGIN();

class CClass;

class APU_API CObject
{
public:
	virtual ~CObject();
	CObject();
	CObject(const CObject &oSrc);
	CObject& operator=(const CObject &oSrc);

	virtual void Reset();

	virtual uint32 GetClassId() const;
};

class CFactory;

class APU_API CClass: public CBaseClass
{
	FOCP_FORBID_COPY(CClass);
	friend class CFactory;
private:
	uint32 m_nClassId;
	CAllocator* m_pAllocator;
	CFactory* m_pFactory;
	uint32 m_nStart;

public:
	CClass(CFactory* pFactory=NULL, uint32 nClassId=0);
	virtual ~CClass();

	void Clear();

	uint32 GetClassId() const;

	//针对数组管理器而言，才有该返回值。
	bool IsArrayContainer() const;
	void SetObjectStart(uint32 nIdx);
	uint32 GetObjectIndex(CObject* pObject) const;
	CObject* QueryObject(uint32 nIdx) const;

	CObject* CreateObject();
	void DestroyObject(CObject* pObject);

	uint32 GetCapacity() const;
	uint32 GetTotalSize() const;
	uint32 GetActiveSize() const;

	virtual uint32 GetInstanceSize() const;
	virtual void ConstructInstance(void* pBuf) const;
	virtual void DestructInstance(void* pObject) const;
	virtual void ResetObject(void* pObject) const;

	virtual bool SetAllocatePolicy(const CAllocatePolicy &oPolicy);
	virtual bool GetDefaultPolicy(CAllocatePolicy &oPolicy, bool &bForbidCustomPolicy);
};

template<typename TObject=CObject> class CCommonClass: public CClass
{
public:
	inline virtual ~CCommonClass()
	{
	}

	inline CCommonClass(CFactory* pFactory=NULL)
		:CClass(pFactory, TObject().GetClassId())
	{
	}

	inline virtual uint32 GetInstanceSize() const
	{
		return FOCP_SIZE_OF(TObject);
	}

	virtual void ConstructInstance(void* pBuf) const
	{
		new(pBuf) TObject();
	}

	virtual void DestructInstance(void* pObject) const
	{
		((TObject*)pObject)->~TObject();
	}
};

class APU_API CFactory
{
	FOCP_FORBID_COPY(CFactory);
	friend class CObject;
	friend class CClass;
	friend class CFiberManager;
private:
	CRbMap<uint32, CClass*> m_oWorkShop;
	CRbMap<uint32, CClass*> m_oIndexTable;
	CRwMutex m_oMutex;
	bool m_bThread;

public:
	CFactory(bool bThread=true);
	~CFactory();

	void Clear();

	CObject* CreateObject(uint32 nHandle);
	void DestroyObject(CObject* pObject);

	uint32 GetObjectIndex(CObject* pObject) const;
	CObject* QueryObject(uint32 nIdx) const;

	//分配策略由应用通过配置指定。
	bool SetAllocatePolicy(uint32 nHandle, const CAllocatePolicy &oPolicy);
	void SetDefaultPolicy();
	void SetObjectStart();

	//获得总容量
	bool GetCapacity(uint32 &nCapacity) const;

	//获得类
	void* GetClassNode();
	void* GetNextClassNode(void* pIt);
	CClass* GetClass(void* pIt);

private:
	void RegisterClass(CClass* pClass);
	void DeRegisterClass(CClass* pClass);
	void RegisterObjectStart(CClass* pClass);
	void DeRegisterObjectStart(CClass* pClass);
};

FOCP_END();

#endif //_APU_OBJECT_HPP_
