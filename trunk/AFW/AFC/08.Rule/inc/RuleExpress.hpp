
#include "RuleType.hpp"

#ifndef _ARF_EXP_HPP_
#define _ARF_EXP_HPP_

FOCP_BEGIN();
class CRuleExpress;
class CRuleConstExpress;
class CRuleDataExpress;
class CRuleVariableExpress;
class CRuleCallExpress;
class CRuleUnaryExpress;
class CRuleBinaryExpress;
class CRuleVectorExpress;
class CRuleVectorInsertExpress;
class CRuleVectorRemoveExpress;
class CRuleVectorSetSizeExpress;
class CRuleFunc;

// ---------------------------------------------------
// CRuleExpress
// ---------------------------------------------------
class RULE_API CRuleExpress: public CRuleFileInfo
{
public:
	CRuleExpress();
	virtual ~CRuleExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack) const;
	virtual CRuleExpress* Clone() const;
	virtual bool IsVariable() const;
	virtual bool IsLiteral() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
	virtual void Dump(CString &oDump) const;
	virtual bool Same(const CRuleExpress &oSrc) const;
	static bool GetCond(uint32 nCondType, void* pCond);
};

// ---------------------------------------------------
// CRuleConstExpress
// ---------------------------------------------------
class RULE_API CRuleConstExpress: public CRuleExpress
{
private:
	uint32 m_nType;
	CRuleVal m_oVal;
	CRuleType* m_pType;

public:
	CRuleConstExpress(uint32 nType, const CRuleVal& oVal);
	CRuleConstExpress(bool b);
	CRuleConstExpress(int8 i8, bool bChar=false);
	CRuleConstExpress(uint8 u8);
	CRuleConstExpress(int16 i16);
	CRuleConstExpress(uint16 u16);
	CRuleConstExpress(int32 i32);
	CRuleConstExpress(uint32 u32);
	CRuleConstExpress(int64 i64);
	CRuleConstExpress(uint64 u64);
	CRuleConstExpress(float f);
	CRuleConstExpress(double d);
	CRuleConstExpress(const char* s, uint32 nSize=0);

	virtual ~CRuleConstExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack) const;
	virtual bool IsLiteral() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
	virtual void Dump(CString &oDump) const;
	virtual bool Same(const CRuleExpress &oSrc) const;

	const CRuleVal* GetValue() const;
};

// ---------------------------------------------------
// CRuleDataExpress
// ---------------------------------------------------
class RULE_API CRuleDataExpress: public CRuleExpress
{
private:
	uint8* m_pData;
	CRuleType* m_pType;
	bool m_bOwned;

public:
	CRuleDataExpress(CRuleType* pType, void* pData);
	virtual ~CRuleDataExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack) const;
	virtual CRuleExpress* Clone() const;
	virtual bool IsVariable() const;
};

// ---------------------------------------------------
// CRuleVariableExpress
// ---------------------------------------------------
class RULE_API CRuleVariableExpress: public CRuleExpress
{
	friend class CRuleUnaryExpress;
	friend class CRuleBinaryExpress;
private:
	CString m_oName;
	CRuleType* m_pType;
	bool m_bVariable;

public:
	CRuleVariableExpress(const char* sName);
	virtual ~CRuleVariableExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack) const;
	virtual CRuleExpress* Clone() const;
	virtual bool IsVariable() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
	virtual void Dump(CString &oDump) const;
	virtual bool Same(const CRuleExpress &oSrc) const;

private:
	void* FindVarData(CRuleStack &oStack, const char* sName) const;
	void* FindVarData(CRuleStack &oStack, CRuleStruct* pStruct, const char* sName, void* pVarv, CRuleType** pType) const;
	CRuleType* FindVarType(CRuleSystem &oSystem, CRuleStruct* pVariables, CRuleStruct* pParameters, CRuleStruct* pGlobalVariables, CRuleChecker* pChecker, const char* sName);
};

// ---------------------------------------------------
// CRuleCallExpress
// ---------------------------------------------------
class RULE_API CRuleCallExpress: public CRuleExpress
{
private:
	CString m_oName;
	CVector< CAutoPointer<CRuleExpress> > m_oArgv;
	CRuleFunc* m_pFunction;
	CRuleExpress* m_pRetExp;

public:
	//变参部分，CRuleExpress*，以NULL结束.
	CRuleCallExpress(const char* sName, ...);
	CRuleCallExpress(const char* sName, const CVector< CAutoPointer<CRuleExpress> > &oArgv);
	virtual ~CRuleCallExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack) const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
	virtual void Dump(CString &oDump) const;
	virtual bool Same(const CRuleExpress &oSrc) const;

	void AddExpress(CRuleExpress* pExp);

	static void BuildArgv(CRuleStack *pStack, void* pVarv, CRuleStruct *pType, const CVector< CAutoPointer<CRuleExpress> >& oArgv);
};

// ---------------------------------------------------
// CRuleUnaryExpress
// ---------------------------------------------------
class RULE_API CRuleUnaryExpress: public CRuleExpress
{
private:
	uint32 m_nOperator;
	CRuleExpress* m_pExp;
	CRuleVariableExpress* m_pRet;

public:
	CRuleUnaryExpress(uint32 nOpr, const CRuleExpress& oExp);
	virtual ~CRuleUnaryExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

// ---------------------------------------------------
// CRuleBinaryExpress
// ---------------------------------------------------
class RULE_API CRuleBinaryExpress: public CRuleExpress
{
private:
	uint32 m_nOperator;
	CRuleExpress *m_pLeft, *m_pRight;
	CRuleVariableExpress* m_pRet;

public:
	CRuleBinaryExpress(const CRuleExpress& oLeft, uint32 nOpr, const CRuleExpress& oRight);
	virtual ~CRuleBinaryExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

// ---------------------------------------------------
// CRuleVectorExpress
// ---------------------------------------------------
class RULE_API CRuleVectorExpress: public CRuleExpress
{
private:
	CRuleExpress *m_pVec, *m_pIdx;

public:
	CRuleVectorExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp);
	virtual ~CRuleVectorExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

// ---------------------------------------------------
// CRuleVectorInsertExpress
// ---------------------------------------------------
class RULE_API CRuleVectorInsertExpress: public CRuleExpress
{
private:
	CRuleExpress *m_pVec, *m_pIdx, *m_pVal;

public:
	CRuleVectorInsertExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp, const CRuleExpress& oValExp);
	virtual ~CRuleVectorInsertExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

// ---------------------------------------------------
// CRuleVectorRemoveExpress
// ---------------------------------------------------
class RULE_API CRuleVectorRemoveExpress: public CRuleExpress
{
private:
	CRuleExpress *m_pVec, *m_pIdx;

public:
	CRuleVectorRemoveExpress(const CRuleExpress& oVecExp, const CRuleExpress& oIdxExp);
	virtual ~CRuleVectorRemoveExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

// ---------------------------------------------------
// CRuleVectorSetSizeExpress
// ---------------------------------------------------
class RULE_API CRuleVectorSetSizeExpress: public CRuleExpress
{
private:
	CRuleExpress *m_pVec, *m_pSize;

public:
	CRuleVectorSetSizeExpress(const CRuleExpress& oVecExp, const CRuleExpress& oSizeExp);
	virtual ~CRuleVectorSetSizeExpress();

	virtual CRuleType* GetExpressType(const CRuleSystem* pSystem) const;
	virtual void* GetExpressValue(CRuleStack* pStack)const ;
	virtual bool IsVariable() const;
	virtual CRuleExpress* Clone() const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker);
};

FOCP_END();

#endif
