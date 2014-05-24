
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
	//返回的行中不能含有回车字符
	//文件结束通过字符char(-1)指示.
	//续航的支持问题，需要在该函数中实现
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
//字符规则
//	普通字符	直接识别为字符
//	/			转义字符，将下一字符作为真实匹配字符。
//	%c1c2% 		字符范围匹配
//	^r			取反识别,r是字符规则
//	@			任意字符均可匹配(0x01~0xFF)
//序列规则
//	*  			匹配任意次数的前一规则,非独立规则
//	?	 		最多匹配一次前一规则,非独立规则
//	[r1r2r3]	选择规则
//	(r1r2r3)	顺序规则
//	#r1r2r3#	乐观规则
//	<r1r2r3>	测试规则
//特殊规则：
//	!			中断规则
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
	void SetContinuation(bool bContinuation);//设置是否支持续行能力

	virtual void Dump(CString & oDump);

protected:
	virtual const char* GetRuleSection();
	virtual void Reset();
};

//////////////////////////////////////////////////////////////
// CLexical潜规则：
//	(1)Token不跨文件
//	(2)不识别非空白的控制字符
//	(3)丢弃回车字符.
//	(4)换文件时自动插入一个换行字符
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
	//源代码管理
	CSourceCode* m_pSourceCode;
	CSingleList<CCodePosition> m_oPositionStack;
	CCodePosition* m_pPos;
	char m_cChar;
	uint32 m_nContinuation;
	//符号表
	CRbTree<CString, CString> m_oStringTable;
	//CToken表
	CBaseDoubleList<CToken> m_oTokenTable;
	CToken* m_pTokenIdx;//已经获取的.
	//当前Token
	CVector<char> m_oToken;
	const char* m_sKind;
	//错误管理
	CFile* m_pErrorFile;
	uint32 m_nErrorCount, m_nWarningCount;
	CLexicalSystem* m_pLexicalSystem;
	const char* m_sLastFileName;
	uint32 m_nLastLine, m_nLastCol;

public:
	CLexicalModule(CLexicalSystem* pSystem, CFile &oErrorFile);
	virtual ~CLexicalModule();

	//源代码管理
	bool GetChar(char &c, bool bNextPos=true);

	//解析函数
	void Parse(CSourceCode& oSourceCode);//完全解析函数
	CToken* PullToken(bool bJmpToNextToken=true);//拉动解析
	CToken* PeekToken();//预取Token
	CToken* GetToken();//获取当前Token
	void SetToken(CToken* pToken);//修改当前Token
	void SetToken(CToken* pOldToken, CToken* pNewToken);//替换Token

	//Token管理
	void AppendToken(char c);
	void AppendToken(const char* s);

	//错误管理
	void WriteErrorV(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, CVaList& pArgList);
	void WriteError(bool bWarning, const char* sFileName, uint32 nLine, uint32 nCol, const char* sFormat, ...);
	uint32 GetErrorCount();
	uint32 GetWarningCount();

	//重载CRuleModule
	virtual void Push();
	virtual bool Pop(uint32 nRet, bool bCheck=false);
	virtual bool IsBroken();
	virtual void OnParseError();
	virtual void OnUnknown();

	//重载CRuleChecker
	virtual void OnError(const CRuleFileInfo& oFileInfo, const char* sFormat, ...);

private:
	//源代码管理
	void ClearTokenTable();
	void Reset(CSourceCode& oSourceCode);//重置
	//解析函数
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
	CRuleExpress* m_pVarExp;//用于输出使用
	bool m_bHome;//必须是行首Token
	bool m_bFirst;//行的第一个单词

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

	//简单名字，不含路劲及前后缀.
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

	//重载CRuleModule
	virtual void Push();
	virtual bool Pop(uint32 nRet, bool bCheck=false);
	virtual bool IsBroken();
	virtual void OnUnknown();
	virtual void OnParseError();

	//重载CRuleChecker
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
