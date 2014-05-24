
#include "Object.hpp"

FOCP_BEGIN();

CObject::~CObject()
{
}

CObject::CObject()
{
}

CObject::CObject(const CObject &oSrc)
{
}

CObject& CObject::operator=(const CObject &oSrc)
{
	return *this;
}

void CObject::Reset()
{
}

uint32 CObject::GetClassId() const
{
	return 0;
}

CClass::CClass(CFactory* pFactory, uint32 nClassId)
{
	m_pAllocator = new CAllocator(this);
	m_nClassId = nClassId;
	m_pFactory = pFactory;
	m_nStart = 0xFFFFFFFF;
	if(m_pFactory && m_nClassId)
		m_pFactory->RegisterClass(this);
}

CClass::~CClass()
{
	if(m_pFactory && m_nClassId)
		m_pFactory->DeRegisterClass(this);
	delete m_pAllocator;
}

void CClass::Clear()
{
	m_pAllocator->Clear();
}

uint32 CClass::GetClassId() const
{
	return m_nClassId;
}

bool CClass::IsArrayContainer() const
{
	return m_pAllocator->IsArrayContainer();
}

void CClass::SetObjectStart(uint32 nIdx)
{
	if(m_pFactory && m_nClassId && m_nStart != nIdx)
	{
		if(m_nStart != 0xFFFFFFFF)
		{
			if(nIdx != 0xFFFFFFFF)
				FocpAbort(("CClass::SetObjectStart(%u), nIdx is invalid "));
			m_pFactory->DeRegisterObjectStart(this);
		}
		m_nStart = nIdx;
		if(m_nStart != 0xFFFFFFFF)
			m_pFactory->RegisterObjectStart(this);
	}
}

uint32 CClass::GetObjectIndex(CObject* pObject) const
{
	return m_pAllocator->GetObjectIndex(pObject);
}

CObject* CClass::QueryObject(uint32 nIdx) const
{
	return (CObject*)m_pAllocator->QueryObject(nIdx);
}

CObject* CClass::CreateObject()
{
	return (CObject*)m_pAllocator->CreateObject();
}

void CClass::DestroyObject(CObject* pObject)
{
	m_pAllocator->DestroyObject(pObject);
}

uint32 CClass::GetCapacity() const
{
	return m_pAllocator->GetCapacity();
}

uint32 CClass::GetTotalSize() const
{
	return m_pAllocator->GetTotalSize();
}

uint32 CClass::GetActiveSize() const
{
	return m_pAllocator->GetActiveSize();
}

uint32 CClass::GetInstanceSize() const
{
	return FOCP_SIZE_OF(CObject);
}

void CClass::ConstructInstance(void* pBuf) const
{
	new(pBuf) CObject;
}

void CClass::ResetObject(void* pObject) const
{
	((CObject*)pObject)->Reset();
}

void CClass::DestructInstance(void* pObject) const
{
	((CObject*)pObject)->~CObject();
}

bool CClass::SetAllocatePolicy(const CAllocatePolicy &oPolicy)
{
	return m_pAllocator->SetAllocatePolicy(oPolicy);
}

bool CClass::GetDefaultPolicy(CAllocatePolicy &oPolicy, bool &bForbidCustomPolicy)
{
	return m_pAllocator->GetDefaultPolicy(oPolicy, bForbidCustomPolicy);
}

CFactory::CFactory(bool bThread)
	:m_oMutex(bThread), m_bThread(bThread)
{
}

CFactory::~CFactory()
{
}

CObject* CFactory::CreateObject(uint32 nHandle)
{
	CObject * pObject = NULL;
	m_oMutex.EnterRead();
	CRbTreeNode* pIt = m_oWorkShop.Find(nHandle);
	if(pIt != m_oWorkShop.End())
		pObject = m_oWorkShop.GetItem(pIt)->CreateObject();
	m_oMutex.LeaveRead();
	return pObject;
}

void CFactory::DestroyObject(CObject* pObject)
{
	CClass* pClass = NULL;
	uint32 nClassId = pObject->GetClassId();
	if(nClassId)
	{
		m_oMutex.EnterRead();
		CRbTreeNode*pIt = m_oWorkShop.Find(nClassId);
		if(pIt != m_oWorkShop.End())
			pClass = m_oWorkShop.GetItem(pIt);
		m_oMutex.LeaveRead();
	}
	if(pClass)
		pClass->DestroyObject(pObject);
	else
		pObject->Reset();
}

bool CFactory::SetAllocatePolicy(uint32 nHandle, const CAllocatePolicy &oPolicy)
{
	bool bRet = false;
	CRbTreeNode* pIt = m_oWorkShop.Find(nHandle);
	if(pIt != m_oWorkShop.End())
	{
		CClass* pClass = m_oWorkShop.GetItem(pIt);
		bRet = pClass->CClass::SetAllocatePolicy(oPolicy);
	}
	return bRet;
}

void CFactory::SetDefaultPolicy()
{
	CAllocatePolicy oPolicy;
	bool bForbidCustomPolicy;

	CRbTreeNode*pIt = m_oWorkShop.First();
	CRbTreeNode*pEnd = m_oWorkShop.End();
	while(pIt != pEnd)
	{
		CClass* pClass = m_oWorkShop.GetItem(pIt);
		if(pClass->GetDefaultPolicy(oPolicy, bForbidCustomPolicy))
			pClass->CClass::SetAllocatePolicy(oPolicy);
		pIt = m_oWorkShop.GetNext(pIt);
	}
}

bool CFactory::GetCapacity(uint32 &nCapacity) const
{
	nCapacity = 0;
	CRbTreeNode*pEnd = m_oWorkShop.End();
	CRbTreeNode*pNode = m_oWorkShop.First();
	while(pNode != pEnd)
	{
		const CClass* pClass = m_oWorkShop.GetItem(pNode);
		uint32 nCapacity1 = pClass->GetCapacity();
		if(nCapacity + nCapacity1 < nCapacity)
			return false;
		nCapacity += nCapacity1;
		pNode = m_oWorkShop.GetNext(pNode);
	}

	return true;
}

void CFactory::Clear()
{
	CRbTreeNode*pEnd = m_oWorkShop.End();
	CRbTreeNode*pNode = m_oWorkShop.First();
	while(pNode != pEnd)
	{
		CClass* pClass = m_oWorkShop.GetItem(pNode);
		pClass->Clear();
		pNode = m_oWorkShop.GetNext(pNode);
	}
}

void CFactory::RegisterClass(CClass* pClass)
{
	m_oMutex.EnterWrite();
	CRbTreeNode*pIt = m_oWorkShop.Find(pClass->m_nClassId);
	if(pIt != m_oWorkShop.End())
		FocpAbort(("CFactory::RegisterClass(%u) is registered", pClass->m_nClassId));
	else
		m_oWorkShop[pClass->m_nClassId] = pClass;
	m_oMutex.LeaveWrite();
}

void CFactory::DeRegisterClass(CClass* pClass)
{
	m_oMutex.EnterWrite();
	m_oWorkShop.Remove(pClass->m_nClassId);
	m_oMutex.LeaveWrite();
}

void CFactory::SetObjectStart()
{
	uint32 nStart = 0;
	CRbTreeNode*pEnd = m_oWorkShop.End();
	CRbTreeNode*pNode = m_oWorkShop.First();
	while(pNode != pEnd)
	{
		CClass* pClass = m_oWorkShop.GetItem(pNode);
		if(pClass->IsArrayContainer())
		{
			pClass->SetObjectStart(nStart);
			nStart += pClass->GetCapacity();
		}
		pNode = m_oWorkShop.GetNext(pNode);
	}
}

void CFactory::RegisterObjectStart(CClass* pClass)
{
	m_oMutex.EnterWrite();
	CRbTreeNode*pIt = m_oIndexTable.Find(pClass->m_nStart);
	if(pIt != m_oIndexTable.End())
		FocpAbort(("CFactory::RegisterObjectStart(%u) is registered", pClass->m_nStart));
	else
		m_oIndexTable[pClass->m_nStart] = pClass;
	m_oMutex.LeaveWrite();
}

void CFactory::DeRegisterObjectStart(CClass* pClass)
{
	m_oMutex.EnterWrite();
	m_oIndexTable.Remove(pClass->m_nStart);
	m_oMutex.LeaveWrite();
}

uint32 CFactory::GetObjectIndex(CObject* pObject) const
{
	uint32 nRet = 0xFFFFFFFF;
	((CRwMutex&)m_oMutex).EnterRead();
	CRbTreeNode* pIt = m_oWorkShop.Find(pObject->GetClassId());
	if(pIt != m_oWorkShop.End())
	{
		CClass* pClass = m_oWorkShop.GetItem(pIt);
		nRet = pClass->GetObjectIndex(pObject);
		if(nRet != 0xFFFFFFFF)
			nRet += pClass->m_nStart;
	}
	((CRwMutex&)m_oMutex).LeaveRead();
	return nRet;
}

CObject* CFactory::QueryObject(uint32 nIdx) const
{
	CObject* pObject = NULL;
	((CRwMutex&)m_oMutex).EnterRead();
	CRbTreeNode* pIt = m_oIndexTable.UpperBound(nIdx);
	CRbTreeNode* pEnd = m_oIndexTable.End();
	if(pIt == pEnd)
		pIt = m_oIndexTable.Last();
	pIt = m_oIndexTable.GetPrev(pIt);
	if(pIt != pEnd)
	{
		CClass* pClass = m_oIndexTable.GetItem(pIt);
		pObject = pClass->QueryObject(nIdx - pClass->m_nStart);
	}
	((CRwMutex&)m_oMutex).LeaveRead();
	return pObject;
}

void* CFactory::GetClassNode()
{
	return m_oWorkShop.First();
}

void* CFactory::GetNextClassNode(void* pIt)
{
	return m_oWorkShop.GetNext((CRbTreeNode*)pIt);
}

CClass* CFactory::GetClass(void* pIt)
{
	if(m_oWorkShop.End() == (CRbTreeNode*)pIt)
		return NULL;
	return m_oWorkShop.GetItem((CRbTreeNode*)pIt);
}

FOCP_END();
