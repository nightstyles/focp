
#include "RuleDef.hpp"

#ifndef _ARF_TYPE_HPP_
#define _ARF_TYPE_HPP_

FOCP_BEGIN();

class CRuleType;
class CRuleVariable;
class CRuleTmpVar;
class CRuleParameter;
class CRuleStruct;
class CRuleVector;

// ---------------------------------------------------
// CRuleType
// ---------------------------------------------------
class RULE_API CRuleType: public CRuleFileInfo
{
protected:
	CString m_oName;

public:
	CRuleType(CRuleSystem *pRuleSystem, const char* sName);
	virtual ~CRuleType();

	const char* GetName() const;
	virtual uint32 GetSize() const;
	virtual uint32 TypeCode() const;//缺省返回ARF_OBJECT
	virtual void InitObject(void* pData) const;
	virtual void ClearObject(void* pData) const;
	virtual void AssignObject(void* pLeft, void* pRight) const;
	virtual bool Check(CRuleChecker* pChecker);
	virtual void Dump(CString & oDump, uint32 nLevel) const;
	static void DumpLevel(CString & oDump, uint32 nLevel);
};

RULE_API CRuleType*& GetRuleArgType(CRuleArgv& oArgv);

template<typename TData, uint32 nTypeCode> class CCommonRuleType: public CRuleType
{
public:
	inline CCommonRuleType(CRuleSystem *pRuleSystem, const char* sName):
		CRuleType(pRuleSystem, sName)
	{
	}

	inline virtual uint32 GetSize() const
	{
		return sizeof(TData);
	}

	inline virtual uint32 TypeCode() const
	{
		return nTypeCode;
	}

	inline virtual void InitObject(void* pData) const
	{
		new(pData) TData();
	}

	inline virtual void ClearObject(void* pData) const
	{
		((TData*)pData)->~TData();
	}

	inline virtual void AssignObject(void* pLeft, void* pRight) const
	{
		*(TData*)pLeft = *(TData*)pRight;
	}
};

// ---------------------------------------------------
// CRuleVariable
// ---------------------------------------------------
class RULE_API CRuleVariable: public CRuleFileInfo
{
	friend class CRuleStruct;
protected:
	CString m_oName;
	CRuleType* m_pType;
	uint32 m_nOffset;

public:
	CRuleVariable(CRuleType* pType, const char* sVarName);
	virtual ~CRuleVariable();

	CRuleType* GetType() const;
	uint32 GetOffset() const;

	virtual uint32 GetSize() const;
	virtual uint32 GetAlign() const;

	const char* GetName() const;

	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleVariable
// ---------------------------------------------------
class RULE_API CRuleTmpVar: public CRuleVariable
{
	friend class CRuleStruct;
private:
	CRuleTmpVar(CRuleType* pType, const char* sVarName);

public:
	CRuleTmpVar(CRuleType* pType, uint32 nId);
	virtual ~CRuleTmpVar();
};

// ---------------------------------------------------
// CRuleParameter
// ---------------------------------------------------
class RULE_API CRuleParameter: public CRuleVariable
{
private:
	bool m_bOut;

public:
	CRuleParameter(CRuleType* pType, const char* sVarName, bool bOut);
	virtual ~CRuleParameter();

	bool IsOut() const;
	virtual uint32 GetSize() const;
	virtual uint32 GetAlign() const;

	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleStruct
//	结构成员中，对象和串字段用指针表示，是因为不知道外部对象的字节对齐模式。
//	结构成员中，结构字段用指针表示，是因为本规则语言不提供指针，为方便数据结构的构造而设置为指针模式。
//	结构成员中，引用字段用指针表示，是为了传递返回值。
// ---------------------------------------------------
class RULE_API CRuleStruct: public CRuleType
{
private:
	CVector<CRuleVariable*> m_oFields;
	uint32 m_nSize;
	bool m_bImplemented, m_bUnion;

public:
	CRuleStruct(CRuleSystem *pRuleSystem, const char* sName, bool bUnion=false);
	virtual ~CRuleStruct();

	bool IsUnion() const;
	uint32 GetFieldCount() const;
	uint32 GetFieldNo(const char* sName) const;
	CRuleVariable* GetField(uint32 nIdx) const;
	CRuleVariable* FindField(const char* sName) const;

	bool AddField(CRuleVariable* pField);
	bool Implemented() const;
	void FinishDefine();

	virtual uint32 GetSize() const;
	virtual uint32 TypeCode() const;
	virtual void InitObject(void* pData) const;
	virtual void ClearObject(void* pData) const;
	virtual void AssignObject(void* pLeft, void* pRight) const;
	virtual bool Check(CRuleChecker* pChecker);
	virtual void Dump(CString & oDump, uint32 nLevel) const;
	void Dump(CString & oDump, bool bForRule=true) const;

	CRuleTmpVar* AllocVar(CRuleType* pType);
};

// ---------------------------------------------------
// CRuleVector
//	向量中，除结构类型外，直接用数据表示。
//	向量中，结构构类用指针表示，是因为本规则语言不提供指针，为方便数据结构的构造而设置为指针模式。
// ---------------------------------------------------
struct CRuleVectorData
{
	uint8* pData;
	uint32 nCount;
};
class RULE_API CRuleVector: public CRuleType
{
private:
	CRuleType* m_pBaseType;

public:
	CRuleVector(CRuleSystem *pRuleSystem, const char* sName);
	virtual ~CRuleVector();

	CRuleType* GetBaseType() const;

	virtual uint32 GetSize() const;
	virtual uint32 TypeCode() const;//缺省返回ARF_OBJECT
	virtual void InitObject(void* pData) const;
	virtual void ClearObject(void* pData) const;
	virtual void AssignObject(void* pLeft, void* pRight) const;

	uint32 GetVectorSize(void* pData) const;
	void SetVectorSize(void* pData, uint32 nNewSize) const;
};

FOCP_END();

#endif
