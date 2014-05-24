
#include "RuleSystem.hpp"

#ifndef _Arf_Compiler_Hpp_
#define _Arf_Compiler_Hpp_

FOCP_BEGIN();
struct CCodeLine;
class CSourceCode;
class CSourceFile;
struct CCodePosition;
class CToken;
class CCharRule;
class CStringRule;
class CLexRule;
class CLexicalSystem;
class CLexicalModule;
class CTokenRule;
class CSyntaxSystem;
class CSyntaxRule;
class CSyntaxModule;
class CCompileSystem;
class CCompileModule;

struct CCodeLine
{
	char* sLine;
	const char* sFileName;
	uint32 nLine;
	uint32 nCol;
	uint32 nSize;
	CCodeLine *pPrev, *pNext;
};

class RULE_API CSourceCode
{
	friend class CLexicalModule;
protected:
	CLexicalModule* m_pLexModule;

private:
	CBaseDoubleList<CCodeLine> m_oCodes;
	CCodeLine* m_pLine;
	uint32 m_nPos;

public:
	CSourceCode(CLexicalModule* pLexModule);
	virtual ~CSourceCode();

	uint32 GetPos() const;
	void SetPos(uint32 nPos);

	bool GetChar(char &c, bool bNextPos=true);
	CCodeLine* GetLine() const;

private:
	void GetLine(uint32 nPos, CCodeLine &oLine);

protected:
	//���ص����в��ܺ��лس��ַ�
	//�ļ�����ͨ���ַ�char(-1)ָʾ.
	//������֧�����⣬��Ҫ�ڸú�����ʵ��
	virtual char* GetCodeLine(const char* &sFileName, uint32 &nLine) = 0;
};

class RULE_API CSourceFile: public CSourceCode
{
private:
	CString m_oLine;
	uint32 m_nLine;
	bool m_bContinuation;
	bool m_bOwner;
	CFile* m_pFile;
	CFileFormatter * m_pFmt;
	CString m_oFileName;

public:
	virtual ~CSourceFile();
	CSourceFile(CLexicalModule* pLexModule, const char* sFileName);
	CSourceFile(CLexicalModule* pLexModule, const char* sCode, const char* sFileName);
	CSourceFile(CLexicalModule* pLexModule, CFile& oFile);

protected:
	virtual char* GetCodeLine(const char* &sFileName, uint32 &nLine);
};

struct CCodePosition
{
	const char* sFileName;
	uint32 nPos, nLine, nCol, nTokenLen;
};

class RULE_API CToken: public CRuleFileInfo
{
	friend class CLexicalModule;
	friend class CSyntaxModule;
	friend class CTokenRule;
private:
	CToken *m_pNext, *m_pPrev;
	const char* m_sKind;
	const char* m_sToken;
	bool m_bNewLine;

public:
	CToken(const char* sKind);
	virtual ~CToken();

	const char* GetToken() const;
	void SetFrom(CToken* pFrom);

	const char* GetKind() const;
	bool NewLine() const;

	virtual void Parse(CLexicalModule& oLexical, const char* sToken, uint32 nLine, uint32 nCol, const char* sFileName);
	void SetNewLine();
};

RULE_API CToken*& GetRuleArgToken(CRuleArgv& oArgv);

class RULE_API CCharRule: public CRule
{
private:
	bool m_bNot;
	uint8 m_nMin, m_nMax;

public:
	CCharRule(uint8 nMin, uint8 nMax, bool bNot=false);
	CCharRule(const CCharRule& oSrc);
	virtual ~CCharRule();

	virtual CRule* Clone() const;
	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Same(const CRule *pRule);
};

class RULE_API CStringRule: public CRule
{
private:
	bool m_bSenstive;
	CString m_oString;

public:
	CStringRule(const char* sStr, bool bSenstive=true);
	CStringRule(const CStringRule& oSrc);
	virtual ~CStringRule();

	virtual CRule* Clone() const;
	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Same(const CRule *pRule);
};

class RULE_API CLexRule: public CRuleProc
{
	friend class CLexicalModule;
private:
	CString m_oRegex;
	bool m_bConst;
	bool m_bSenstive;
	bool m_bSkip;

public:
	CLexRule(CLexicalSystem* pSystem, const char* sRegex, const char* sKind,
			 bool bConst=false, bool bSenstive=true, bool bSkip=false);

	virtual ~CLexRule();

	virtual void Dump(CString & oDump, uint32 nLevel);

protected:
	virtual void Before(CRuleModule* pModule)const;
	virtual uint32 After(CRuleModule* pModule)const;
	virtual void OnSuccess(CRuleModule* pModule)const;
	virtual void OnFailure(CRuleModule* pModule)const;

private:
//�ַ�����
//	��ͨ�ַ�	ֱ��ʶ��Ϊ�ַ�
//	/			ת���ַ�������һ�ַ���Ϊ��ʵƥ���ַ���
//	%c1c2% 		�ַ���Χƥ��
//	^r			ȡ��ʶ��,r���ַ�����
//	@			�����ַ�����ƥ��(0x01~0xFF)
//���й���
//	*  			ƥ�����������ǰһ����,�Ƕ�������
//	?	 		���ƥ��һ��ǰһ����,�Ƕ�������
//	[r1r2r3]	ѡ�����
//	(r1r2r3)	˳�����
//	#r1r2r3#	�ֹ۹���
//	<r1r2r3>	���Թ���
//�������
//	!			�жϹ���
	static CRule* BuildRegexRule(const char* sRegex, bool bConst, bool bSenstive);
	static void BuildRegexRule(CSequenceRule& oRule, const char* &sRegex, char cEnd);
	static CRule* GetLastRule(CSequenceRule& oRule);
	static void RemoveLastRule(CSequenceRule& oRule);
};

class RULE_API CLexicalSystem: public CRuleSystem
{
	friend class CCompiler;
	friend class CLexicalModule;
	friend class CSourceFile;
	friend class CCompileSystem;
private:
	CString m_oWhiteSpace;
	CString m_oPuncts;
	bool m_bContinuation;

public:
	CLexicalSystem();
	virtual ~CLexicalSystem();

	void SetWhiteSpace(const CString& oWhiteSpace);
	void SetPuncts(const CString& oPuncts);
	void SetContinuation(bool bContinuation);//�����Ƿ�֧����������

	virtual void Dump(CString & oDump);

protected:
	virtual const char* GetRuleSection();
	virtual void Reset();
};

//////////////////////////////////////////////////////////////
// CLexicalǱ����
//	(1)Token�����ļ�
//	(2)��ʶ��ǿհ׵Ŀ����ַ�
//	(3)�����س��ַ�.
//	(4)���ļ�ʱ�Զ�����һ�������ַ�
//////////////////////////////////////////////////////////////
class RULE_API CLexicalModule: public CRuleModule, public CRuleChecker
{
	friend class CCharRule;
	friend class CStringRule;
	friend class CSyntaxRule;
	friend class CTokenRule;
	friend class CSyntaxModule;
	friend class CCompiler;
	friend class CMetaCompiler;
	friend class CSourceCode;
	friend class CSourceFile;
	friend class CLexRule;

private:
	//Դ�������
	CSourceCode* m_pSourceCode;
	CSingleList<CCodePosition> m_oPositionStack;
	CCodePosition* m_pPos;
	char m_cChar;
	uint32 m_nContinuation;
	//���ű�
	CRbTree<CString, CString> m_oStringTable;
	//CToken��
	CBaseDoubleList<CToken> m_oTokenTable;
	CToken* m_pTokenIdx;//�Ѿ���ȡ��.
	//��ǰToken
	CVector<char> m_oToken;
	const char* m_sKind;
	//�������
	CFile* m_pErrorFile;
	uint32 m_nErrorCount, m_nWarningCount;
	CLexicalSystem* m_pLexicalSystem;
	const char* m_sLastFileName;
	uint32 m_nLastLine, m_nLastCol;

public:
	CLexicalModule(CLexicalSystem* pSystem, CFile &oErrorFile);
	virtual ~CLexicalModule();

	//Դ�������
	bool GetChar(char &c, bool bNextPos=true);

	//��������
	void Parse(CSourceCode& oSourceCode);//��ȫ��������
	CToken* PullToken(bool bJmpToNextToken=true);//��������
	CToken* PeekToken();//ԤȡToken
	CToken* GetToken();//��ȡ��ǰToken
	void SetToken(CToken* pToken);//�޸ĵ�ǰToken
	void SetToken(CToken* pOldToken, CToken* pNewToken);//�滻Token

	//Token����
	void AppendToken(char c);
	void AppendToken(const char* s);

	//�������
	void WriteErrorV(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, CVaList& pArgList);
	void WriteError(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, ...);
	uint32 GetErrorCount();
	uint32 GetWarningCount();

	//����CRuleModule
	virtual void Push();
	virtual bool Pop(uint32 nRet, bool bCheck=false);
	virtual bool IsBroken();
	virtual void OnParseError();
	virtual void OnUnknown();

	//����CRuleChecker
	virtual void OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...);

private:
	//Դ�������
	void ClearTokenTable();
	void Reset(CSourceCode& oSourceCode);//����
	//��������
	bool JumpSpace();
	void JumpToTokenEnd();
	uint32 CheckTokenEnd();
	bool IsSpace(char c);
	bool IsPunct(char c);
	void CreateToken(const char* sKind);
	virtual const char* GetSymbol(const CString& oSymbol);
};

class RULE_API CTokenRule: public CRule
{
private:
	const char* m_sToken;
	CRuleExpress* m_pVarExp;//�������ʹ��
	bool m_bHome;//����������Token
	bool m_bFirst;//�еĵ�һ������

public:
	CTokenRule(CLexicalSystem* pLexSystem, const char* sKind);
	CTokenRule(const CTokenRule& oRule);
	virtual ~CTokenRule();

	virtual CRule* Clone() const;
	virtual uint32 Match(CRuleStack &oStack, bool bMust) const;
	virtual bool Same(const CRule *pRule);
	virtual void Dump(CString & oDump, uint32 nLevel);
	virtual bool Check(CRuleSystem &oSystem, CRuleChecker* pChecker, uint32 nLoopLevel);

	CTokenRule& SetOutput(CRuleExpress* pVarExp);
	CTokenRule& SetHome();
	CTokenRule& SetFirst();

	CRuleExpress* GetOutExp();
};

class RULE_API CSyntaxSystem: public CRuleSystem
{
	friend class CCompileSystem;
private:
	CString m_oLibName;
	CDynamicLibrary m_oLib;
	CRbMap<CString, FRuleHost, CNameCompare> m_oFunctions;

public:
	CSyntaxSystem();
	virtual ~CSyntaxSystem();

	//�����֣�����·����ǰ��׺.
	bool Load(const char* sLibName);

	void RegHost(const char* sName, FRuleHost fFunc);

protected:
	virtual FRuleHost GetHost(const char* sName) const;
	virtual void Reset();
};

class RULE_API CSyntaxRule: public CRuleProc
{
public:
	CSyntaxRule(CSyntaxSystem *pSyntaxSystem, const char* sName, CRule *pRule, bool bMain, ...);

protected:
	virtual void OnFailure(CRuleModule* pModule)const;
};

class RULE_API CSyntaxModule: public CRuleModule, public CRuleChecker
{
	friend class CTokenRule;
	friend class CCompiler;
	friend class CMetaCompiler;
	friend class CSyntaxRule;
	struct CTokenPos
	{
		CToken* pIdx;
	};
private:
	CLexicalModule* m_pLexical;
	CSingleList<CTokenPos> m_oTokenStack;
	CToken* m_pToken;
	CSyntaxSystem* m_pSyntaxSystem;

public:
	CSyntaxModule(CLexicalModule &oLexical, CSyntaxSystem* pSyntaxSystem);
	virtual ~CSyntaxModule();

	void Parse(CSourceCode& oSourceCode, bool bLoop=false);

	CToken* GetCurToken();

	CToken* PullToken(bool bNextToken=true);

	CToken* PeekToken();

	CLexicalModule& GetLexical();

	//����CRuleModule
	virtual void Push();
	virtual bool Pop(uint32 nRet, bool bCheck=false);
	virtual bool IsBroken();
	virtual void OnUnknown();
	virtual void OnParseError();

	//����CRuleChecker
	virtual void OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...);

private:
	void Reset(CSourceCode& oSourceCode);
};

class RULE_API CCompileSystem
{
	friend class CCompileModule;
	friend class CMetaCompileModule;
protected:
	CMutex m_oMutex;
	bool m_bChecked;
	CLexicalSystem m_oLexicalSystem;
	CSyntaxSystem m_oSyntaxSystem;

public:
	CCompileSystem();
	virtual ~CCompileSystem();

	bool Load(const char* sLibName);
	void Dump(CString & oDump);

	CLexicalSystem* GetLexicalSystem();
	CSyntaxSystem* GetSyntaxSystem();

	bool Check(CCompileModule& oModule);
	virtual void Reset();
};

class RULE_API CCompileModule
{
protected:
	CLexicalModule m_oLexicalModule;
	CSyntaxModule m_oSyntaxModule;

public:
	CCompileModule(CFile &oErrorFile, CCompileSystem &oCompileSystem);
	virtual ~CCompileModule();

	bool Compile(CFile& oSrcFile);

	bool Check();

	uint32 GetErrorCount();
	uint32 GetWarningCount();

	static CCompileModule* GetCompileModule(CRuleModule* pModule);

	CLexicalModule* GetLexicalModule();
	CSyntaxModule* GetSyntaxModule();
};

FOCP_END();

#endif
