
#include "RuleExpress.hpp"

#ifndef _ARF_SENTENCE_HPP_
#define _ARF_SENTENCE_HPP_

FOCP_BEGIN();

class CRuleSentence;
class CRuleReturnSentence;
class CRuleExpSentence;
class CRuleCondSentence;
class CRuleComplexSentence;
class CRuleWhileSentence;
class CRuleForSentence;
class CRuleSwitchSentence;
class CRuleBreakSentence;
class CRuleContinueSenctence;
class CRuleElseSentence;
class CRuleDefaultSentence;
class CRuleCaseSentence;
// ---------------------------------------------------
// CRuleSentence
// ---------------------------------------------------
class RULE_API CRuleSentence: public CRuleFileInfo
{
public:
	CRuleSentence();
	virtual ~CRuleSentence();

	virtual uint32 GetSentenceType() const;

	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;//0=normal;1=break;2=continue;3=return;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;

	//无效语句可以重载该函数，决定其依赖语句类型
	virtual uint32 GetDepend() const;
};

// ---------------------------------------------------
// CRuleReturnSentence
// ---------------------------------------------------
class RULE_API CRuleReturnSentence: public CRuleSentence
{
public:
	CRuleReturnSentence();
	virtual ~CRuleReturnSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;//0=normal;1=break;2=continue;3=return;
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleExpSentence
// ---------------------------------------------------
class RULE_API CRuleExpSentence: public CRuleSentence
{
private:
	CRuleExpress* m_pExp;

public:
	CRuleExpSentence(const CRuleExpress &oExp);
	virtual ~CRuleExpSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleExpSentence
// ---------------------------------------------------
class RULE_API CRuleCondSentence: public CRuleSentence
{
private:
	CRuleExpress* m_pExp;
	CRuleSentence* m_pIfSentence;
	CRuleSentence* m_pElseSentence;

public:
	CRuleCondSentence(const CRuleExpress &oCond, CRuleSentence* pIfSentence, CRuleSentence* pElseSentence);
	virtual ~CRuleCondSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleComplexSentence
// ---------------------------------------------------
class RULE_API CRuleComplexSentence: public CRuleSentence
{
protected:
	bool m_bTop;
	CVector< CAutoPointer<CRuleSentence> > m_oSentences;

public:
	CRuleComplexSentence(bool bTop=false);
	virtual ~CRuleComplexSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;//0=normal;1=break;2=continue;3=return;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;

	void DumpFunc(CString & oDump, uint32 nLevel) const;

	CRuleComplexSentence& AddSentence(CRuleSentence *pSentence);
};

// ---------------------------------------------------
// CRuleWhileSentence
// ---------------------------------------------------
class RULE_API CRuleWhileSentence: public CRuleSentence
{
private:
	bool m_bDoWhile;
	CRuleExpress* m_pExp;
	CRuleSentence* m_pDoSentence;

public:
	CRuleWhileSentence(const CRuleExpress &oCond, CRuleSentence* pSentence, bool bDoWhile);
	virtual ~CRuleWhileSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleForSentence
// ---------------------------------------------------
class RULE_API CRuleForSentence: public CRuleSentence
{
private:
	CRuleExpress* m_pInitExp, *m_pCondExp, *m_pLoopExp;
	CRuleSentence* m_pDoSentence;

public:
	CRuleForSentence(CRuleExpress* pInitExp, CRuleExpress* pCondExp, CRuleExpress* pLoopExp, CRuleSentence* pSentence);
	virtual ~CRuleForSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleSwitchSentence
// ---------------------------------------------------
class RULE_API CRuleSwitchSentence: public CRuleComplexSentence
{
private:
	CRuleExpress* m_pSwitchExp;
	CRbMap<uint64, uint32> m_oCaseLabel;
	uint32 m_nDefaultLabel;

public:
	CRuleSwitchSentence(const CRuleExpress &oSwitchExp);
	virtual ~CRuleSwitchSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;

	bool SetCaseLable(uint64 nCase);
	bool SetDefaultLable();
};

// ---------------------------------------------------
// CRuleBreakSentence
// ---------------------------------------------------
class RULE_API CRuleBreakSentence: public CRuleSentence
{
public:
	CRuleBreakSentence();
	virtual ~CRuleBreakSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleContinueSentence
// ---------------------------------------------------
class RULE_API CRuleContinueSentence: public CRuleSentence
{
public:
	CRuleContinueSentence();
	virtual ~CRuleContinueSentence();

	virtual uint32 GetSentenceType() const;
	virtual CRuleSentence* Clone() const;
	virtual uint32 Execute(CRuleStack* pStack) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel, uint32 nSwitchLevel);
	virtual bool Same(const CRuleSentence *pSentence) const;
	virtual void Dump(CString & oDump, uint32 nLevel) const;
};

// ---------------------------------------------------
// CRuleElseSentence
// ---------------------------------------------------
class RULE_API CRuleElseSentence: public CRuleSentence
{
private:
	CRuleSentence* m_pSentence;

public:
	CRuleElseSentence(CRuleSentence* pSentence);
	virtual ~CRuleElseSentence();
	virtual CRuleSentence* Clone() const;
	virtual uint32 GetDepend() const;
	CRuleSentence* Detach();
};

// ---------------------------------------------------
// CRuleDefaultSentence
// ---------------------------------------------------
class RULE_API CRuleDefaultSentence: public CRuleSentence
{
public:
	CRuleDefaultSentence();
	virtual ~CRuleDefaultSentence();
	virtual CRuleSentence* Clone() const;
	virtual uint32 GetDepend() const;
};

// ---------------------------------------------------
// CRuleCaseSentence
// ---------------------------------------------------
class RULE_API CRuleCaseSentence: public CRuleSentence
{
private:
	uint64 m_nVal;
	bool m_bError;

public:
	CRuleCaseSentence(uint64 nVal, bool bError = false);
	virtual ~CRuleCaseSentence();
	virtual CRuleSentence* Clone() const;
	virtual uint32 GetDepend() const;
	bool GetSwitchValue(uint64 &nVal) const;
};

// ---------------------------------------------------
// CRuleFunc
// ---------------------------------------------------
class RULE_API CRuleFunc: public CRuleFileInfo
{
	friend class CRuleSystem;
private:
	CRuleSystem* m_pRuleSystem;
	CString m_oName;
	CRuleType* m_pRetType;
	CRuleComplexSentence* m_pFuncBody;
	FRuleHost m_fFunc;
	//规则参数与局部变量定义
	CRuleStruct m_oParameters;
	CRuleStruct m_oVariables;

public:
	//变参部分说明：
	// {const char* sParaDefine}表示参数定义情况, NULL表示参数定义结束;
	//		typename [&] paraname
	// {const char* sVarDefine)表示局部变量定义, NULL表示局部变量定义结束
	//		typename varname
	CRuleFunc(CRuleSystem *pRuleSystem, const char* sName, const char* sReturnType, ...);
	CRuleFunc(CRuleSystem *pRuleSystem, const char* sName, CRuleType* pRetType, ...);
	virtual ~CRuleFunc();

	CRuleStruct* GetParameters();
	CRuleStruct* GetVariables();

	void SetSentence(CRuleComplexSentence* pSentence);
	bool Implemented();

	const char* GetName() const;
	CRuleType* GetRetType() const;

	bool Check(CRuleChecker* pChecker);

	void Call(CRuleStack &oStack) const;
	virtual void Dump(CString & oDump, uint32 nLevel);

protected:
	void InitializeParametersAndVariables(CVaList& args);
	void InitializeParameters(CVaList& args);
	void InitializeVariables(CVaList& args);
};

FOCP_END();

#endif
