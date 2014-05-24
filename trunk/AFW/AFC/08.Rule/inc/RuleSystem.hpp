
#include "RuleSentence.hpp"

#ifndef _ARF_RULE_HPP_
#define _ARF_RULE_HPP_

FOCP_BEGIN();

class CRule;
class CRuleProc;
class CCallRule;
class CRuleModule;
class CRuleSystem;
class CSequenceRule;
class COrRule;
class CLoopRule;
class CBreakRule;
class CSemanticRule;

// ---------------------------------------------------
// CRule
// ---------------------------------------------------
class RULE_API CRule: public CRuleFileInfo
{
	friend class CUnionRule;
	friend class CRuleProc;
public:
	CRule();
	CRule(const CRule& oRule);
	virtual ~CRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	uint32 Call(CRuleStack &oStack, bool bMust) const;
	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Merge(const CRule *pRule);
	virtual bool Same(const CRule *pRule);
	virtual CRule* Optimize();//���ܷ���������¶���

	virtual void Dump(CString & oDump, uint32 nLevel);

//�����Ԫ����
	//a | b		: ƥ��a������ƥ��b������1��������
	COrRule operator|(const CRule& oRule);

	//a >> b 	: �����ƥ��a��b������2��������
	CSequenceRule operator>>(const CRule& oRule);

//ѭ������
	CLoopRule operator*();//ƥ��0����������a������ƥ�������������
	CLoopRule operator-();//ƥ��0����1��a������ƥ�������������

private:
	CRule& operator=(const CRule &oRule);
};

// ---------------------------------------------------
// CRuleProc
//	ֻ����new�������ɶ����Ҳ���Ҫ�ϲ��ͷ�
// ---------------------------------------------------
class RULE_API CRuleProc: public CRuleFileInfo
{
	friend class CRuleSystem;
private:
	CRuleSystem* m_pRuleSystem;
	CString m_oName;
	CRule* m_pProcBody;
	FRuleHost m_fFunc;
	//���������ֲ���������
	bool m_bMain;
	CRuleStruct m_oParameters;
	CRuleStruct m_oVariables;

public:
	//��β���˵����
	// {const char* sTypeName, const char* sParaName, uint32 bOut}�ɶԳ��֣���ʾ�����������
	// {NULL},��ʾ�����������
	// {const char* sTypeName, const char* sVarName}��ʾ�ֲ���������
	// {NULL}��ʾ�ֲ������������
	CRuleProc(CRuleSystem *pRuleSystem, const char* sName, CRule *pRule, bool bMain, ...);

	virtual ~CRuleProc();

	CRuleStruct* GetParameters();
	CRuleStruct* GetVariables();
	void SetRule(CRule* pRule);

	const char* GetName() const;
	bool IsMain() const;

	bool Check(CRuleChecker* pChecker);
	virtual void Optimize();

	uint32 Call(CRuleStack &oStack, bool bMust) const;
	virtual void Dump(CString & oDump, uint32 nLevel);

protected:
	virtual void Before(CRuleModule* pModule)const;
	virtual uint32 After(CRuleModule* pModule)const;
	virtual void OnSuccess(CRuleModule* pModule)const;
	virtual void OnFailure(CRuleModule* pModule)const;
	void InitializeParametersAndVariables(CVaList& args);
	void InitializeParameters(CVaList& args);
	void InitializeVariables(CVaList& args);

private:
	uint32 DoCall(CRuleStack &oStack, bool bMust) const;
};

// ---------------------------------------------------
// CCallRule
// ---------------------------------------------------
class RULE_API CCallRule: public CRule
{
private:
	CString m_oName;
	CVector< CAutoPointer<CRuleExpress> > m_oArgv;
	CRuleProc* m_pProc;//callee

public:
	//��β��֣�CRuleExpress*����NULL����.
	CCallRule(const char* sName, ...);
	CCallRule(const CCallRule& oRule);
	virtual ~CCallRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	void AddExpress(CRuleExpress* pExp);

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual void Dump(CString & oDump, uint32 nLevel);
};

// ---------------------------------------------------
// CRuleModule
// ---------------------------------------------------
class RULE_API CRuleModule
{
	friend class CRuleSystem;

protected:
	char* m_pData;

public:
	CRuleModule();
	virtual ~CRuleModule();

	virtual bool IsBroken();
	virtual void Push();

	//��nRetΪ0����bCheckΪtrueʱ����Ҫ����λ���Ƿ�仯
	virtual bool Pop(uint32 nRet, bool bCheck=false);
	virtual void OnUnknown();
	virtual void OnParseError();

	void InitData(CRuleSystem* pRuleSystem);
	void ClearData(CRuleSystem* pRuleSystem);
};

// ---------------------------------------------------
// CRuleSystem
// ---------------------------------------------------
class RULE_API CRuleSystem
{
	friend class CCallRule;
	friend class CRuleProc;
	friend class CRuleFunc;
	friend class CRuleType;
	friend class CSemanticRule;
	friend class CRuleVariableExpress;
private:
	CRuleProc* m_pCheckProc;//������Check
	CRuleFunc* m_pCheckFunc;//������Check
	CVector< CAutoPointer<CRuleProc> > m_oSystemRules;
	CVector< CAutoPointer<CRuleType> > m_oSystemTypes;
	CVector< CAutoPointer<CRuleFunc> > m_oSystemFunctions;
	CRuleStruct m_oVariables;

public:
	CRuleSystem();
	virtual ~CRuleSystem();

	bool Check(CRuleChecker* pChecker);
	virtual void Dump(CString & oDump);

	CRuleType* GetType(const char* sName) const;
	CRuleFunc* GetFunc(const char* sName) const;
	CRuleProc* GetRule(const char* sName) const;

	void Parse(bool bLoop, CRuleModule &oModule) const;

	CRuleStruct* GetVariables();
	CRuleStruct* GetCheckVariables();
	CRuleStruct* GetCheckParameters();

protected:
	virtual const char* GetRuleSection();
	virtual FRuleHost GetHost(const char* sName) const;
	virtual void Reset();

private:
	void RegRule(CRuleProc* pProc);
	void RegType(CRuleType* pType);
	void RegFunc(CRuleFunc* pFunc);
};

// ---------------------------------------------------
// CSequenceRule
// ---------------------------------------------------
class RULE_API CSequenceRule: public CRule
{
private:
	uint32 m_nType;//0=Sequence,1=Test,2=Optimism

protected:
	CVector< CAutoPointer< CRule > > m_oRuleTable;

public:
	CSequenceRule(uint32 nType=0);
	CSequenceRule(const CSequenceRule& oRule);
	virtual ~CSequenceRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Merge(const CRule *pRule);
	virtual bool Same(const CRule *pRule);
	virtual CRule* Optimize();
	virtual void Dump(CString & oDump, uint32 nLevel);

	void InsertRule(const CRule& oRule, bool bAppend=true);
	uint32 GetSize() const;
	CRule* GetRule(uint32 nIdx) const;
	void Remove(uint32 nIdx);
};

// ---------------------------------------------------
// COrRule
// ---------------------------------------------------
class RULE_API COrRule: public CSequenceRule
{
public:
	COrRule();
	COrRule(const COrRule& oRule);
	virtual ~COrRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual bool Merge(const CRule *pRule);
	virtual CRule* Optimize();
	virtual void Dump(CString & oDump, uint32 nLevel);
};

// ---------------------------------------------------
// CLoopRule
// ---------------------------------------------------
class RULE_API CLoopRule: public CRule
{
private:
	bool m_bAnyTimes;
	CRule* m_pRule;

public:
	CLoopRule(const CRule& oRule, bool bAnyTimes);
	CLoopRule(const CLoopRule& oRule);
	virtual ~CLoopRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;

	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual bool Merge(const CRule *pRule);
	virtual CRule* Optimize();
	virtual void Dump(CString & oDump, uint32 nLevel);
};

// ---------------------------------------------------
// CBreakRule
// ---------------------------------------------------
class RULE_API CBreakRule: public CRule
{
public:
	CBreakRule();
	CBreakRule(const CBreakRule& oRule);
	virtual ~CBreakRule();

	virtual uint32 GetRuleType() const;
	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual void Dump(CString & oDump, uint32 nLevel);
};

// ---------------------------------------------------
// CIfRule
// ---------------------------------------------------
class RULE_API CIfRule: public CRule
{
private:
	CRuleExpress* m_pExp;

public:
	CIfRule(CRuleExpress* pExp);
	CIfRule(const CIfRule& oRule);
	virtual ~CIfRule();

	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual void Dump(CString & oDump, uint32 nLevel);
};

// ---------------------------------------------------
// CSemanticRule
// ---------------------------------------------------
class RULE_API CSemanticRule: public CRule
{
private:
	CRuleSentence* m_pSentence;

public:
	CSemanticRule(CRuleSentence* pSentence);
	CSemanticRule(const CSemanticRule& oRule);
	virtual ~CSemanticRule();

	virtual CRule* Clone() const;

	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);
	virtual bool Same(const CRule *pRule);
	virtual void Dump(CString & oDump, uint32 nLevel);
};

FOCP_END();

#endif
