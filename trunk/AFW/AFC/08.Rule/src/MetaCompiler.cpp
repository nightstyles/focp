
#include "MetaCompiler.hpp"
#include <stdlib.h>
#include <errno.h>

FOCP_BEGIN();

static void DefineLexRule(CLexicalSystem* pLexSystem);
static void DefineSystemType(CSyntaxSystem* pSynSystem);
static void DefineMetaRules(CSyntaxSystem* pSynSystem);
static void PreDefineSyntaxSystem(CSyntaxSystem* pSynSystem);

#ifndef MSVC
FOCP_PRIVATE_BEGIN();
#endif

class CMetaCompileSystem: public CCompileSystem
{
public:
	CMetaCompileSystem()
	{
	}

	~CMetaCompileSystem()
	{
	}

	static CMetaCompileSystem* GetInstance()
	{
		return CSingleInstanceEx<CMetaCompileSystem>::GetInstance();
	}

	void InitializeInstance()
	{
		DefineLexRule(GetLexicalSystem());
		DefineSystemType(GetSyntaxSystem());
		DefineMetaRules(GetSyntaxSystem());
	}

	virtual void Reset()
	{
		CCompileSystem::Reset();
		InitializeInstance();
	}
};

class CMetaCompileModule: public CCompileModule
{
private:
	CCompileSystem* m_pSystem;
	CRuleProc* m_pCreateProc;
	CRuleFunc* m_pCreateFunc;

public:
	CMetaCompileModule(CFile &oErrorFile):
		CCompileModule(oErrorFile, *CMetaCompileSystem::GetInstance())
	{
		m_pSystem = NULL;
		m_pCreateProc = NULL;
		m_pCreateFunc = NULL;
	}

	virtual ~CMetaCompileModule()
	{
	}

	//在调用Compile之前需要调用该函数
	void SetTarget(CCompileSystem* pSystem)
	{
		m_pSystem = pSystem;
		PreDefineSyntaxSystem(pSystem->GetSyntaxSystem());
	}

//内部函数使用
	CLexicalSystem* GetTargetLexicalSystem()
	{
		return m_pSystem->GetLexicalSystem();
	}

	CSyntaxSystem* GetTargetSyntaxSystem()
	{
		return m_pSystem->GetSyntaxSystem();
	}

	//创建的过程，作上下文传递作用。
	void SetProc(CRuleProc* pProc)
	{
		m_pCreateProc = pProc;
		m_pCreateFunc = NULL;
	}

	CRuleProc* GetProc()
	{
		return m_pCreateProc;
	}

	void SetFunc(CRuleFunc* pFunc)
	{
		m_pCreateProc = NULL;
		m_pCreateFunc = pFunc;
	}

	CRuleFunc* GetFunc()
	{
		return m_pCreateFunc;
	}
};

#ifndef MSVC
FOCP_PRIVATE_END();
#endif

RULE_API uint32 MetaCompile(CFile &oErrFile, CFile& oSyntaxFile, CCompileSystem* pSystem, uint32 &nWarning)
{
	CMetaCompileSystem* pCompileSystem = CMetaCompileSystem::GetInstance();
	CRuleSystem* pSyntaxSystem = pCompileSystem->GetSyntaxSystem();
	CMetaCompileModule oMetaModule(oErrFile);
	pCompileSystem->Check(oMetaModule);
	nWarning = oMetaModule.GetWarningCount();
	uint32 nRet = oMetaModule.GetErrorCount();
	if(nRet == 0)
	{
		oMetaModule.GetSyntaxModule()->InitData(pSyntaxSystem);
		oMetaModule.SetTarget(pSystem);
		oMetaModule.Compile(oSyntaxFile);
		pSystem->Check(oMetaModule);
		nWarning = oMetaModule.GetWarningCount();
		nRet = oMetaModule.GetErrorCount();
		oMetaModule.GetSyntaxModule()->ClearData(pSyntaxSystem);
		if(nRet)
			pSystem->Reset();
	}
	return nRet;
}

#define IdentifierRegStr ("[%AZ%%az%_][%AZ%%az%_%09%]*")
#define CommentsRegStr ("//[(//^\n*\n?)(/*[</*//!>@]*/*//)]")
#define CharRegStr ("\'[(\\@)<[\'\n]!>@]*\'")
#define StringRegStr ("\"[(\\@)<[\"\n]!>@]*\"")
#define IntegerRegStr ("[(%19%%09%*)(0[([xX][%09%%AF%%af%][%09%%AF%%af%]*)(%07%*)])][([uU][lL8(16)(32)(64)]?)([iI][8(16)(32)(64)])]?")
#define FloatRegStr ("[(.%09%*([eE][/+/-]%09%%09%*)?)(%09%%09%*[(.%09%*([eE][/+/-]%09%%09%*)?)([eE][/+/-]%09%%09%*)])][fF]?")

static void DefineLexRule(CLexicalSystem* pLexSystem)
{
	//需要忽略的单词放在最前面
	new CLexRule(pLexSystem, CommentsRegStr, "Comment", false, true, true);
	//关键字定义
	new CLexRule(pLexSystem, "WHITESPACE", "WHITESPACE", true, false);
	new CLexRule(pLexSystem, "PUNCTS", "PUNCTS", true, false);
	new CLexRule(pLexSystem, "TOKENS", "TOKENS", true, false);
	new CLexRule(pLexSystem, "TYPES", "TYPES", true, false);
	new CLexRule(pLexSystem, "STRUCT", "STRUCT", true, false);
	new CLexRule(pLexSystem, "UNION", "UNION", true, false);
	new CLexRule(pLexSystem, "VARIABLES", "VARIABLES", true, false);
	new CLexRule(pLexSystem, "RULES", "RULES", true, false);
	new CLexRule(pLexSystem, "MAIN", "MAIN", true, false);
	new CLexRule(pLexSystem, "BREAK", "BREAK", true, false);
	new CLexRule(pLexSystem, "CONST", "CONST", true, false);
	new CLexRule(pLexSystem, "INSENSITIVE", "INSENSITIVE", true, false);
	new CLexRule(pLexSystem, "SKIP", "SKIP", true, false);
	new CLexRule(pLexSystem, "IF", "IF", true, false);
	new CLexRule(pLexSystem, "SWITCH", "SWITCH", true, false);
	new CLexRule(pLexSystem, "WHILE", "WHILE", true, false);
	new CLexRule(pLexSystem, "DO", "DO", true, false);
	new CLexRule(pLexSystem, "FOR", "FOR", true, false);
	new CLexRule(pLexSystem, "CONTINUE", "CONTINUE", true, false);
	new CLexRule(pLexSystem, "ELSE", "ELSE", true, false);
	new CLexRule(pLexSystem, "RETURN", "RETURN", true, false);
	new CLexRule(pLexSystem, "DEFAULT", "DEFAULT", true, false);
	new CLexRule(pLexSystem, "CASE", "CASE", true, false);
	new CLexRule(pLexSystem, "TRUE", "TRUE", true, false);
	new CLexRule(pLexSystem, "FALSE", "FALSE", true, false);
	new CLexRule(pLexSystem, "VECTOR", "VECTOR", true, false);
	//定义标点符号
	new CLexRule(pLexSystem, "<<=", "LeftShiftEqual", true);
	new CLexRule(pLexSystem, ">>=", "RightShiftEqual", true);
	new CLexRule(pLexSystem, "(?", "LeftParenQuestion", true);
	new CLexRule(pLexSystem, "?)", "RightParenQuestion", true);
	new CLexRule(pLexSystem, "[?", "LeftBracketQuestion", true);
	new CLexRule(pLexSystem, "?]", "RightBracketQuestion", true);
	new CLexRule(pLexSystem, "{?", "LeftBraceQuestion", true);
	new CLexRule(pLexSystem, "?}", "RightBraceQuestion", true);
	new CLexRule(pLexSystem, "{+", "LeftBracePlus", true);
	new CLexRule(pLexSystem, "+}", "RightBracePlus", true);
	new CLexRule(pLexSystem, "(.", "LeftParenPeriod", true);
	new CLexRule(pLexSystem, ".)", "RightParenPeriod", true);
	new CLexRule(pLexSystem, "<!", "LessExclam", true);
	new CLexRule(pLexSystem, "!>", "GreaterExclam", true);
	new CLexRule(pLexSystem, "*=", "MulEqual", true);
	new CLexRule(pLexSystem, "/=", "DivEqual", true);
	new CLexRule(pLexSystem, "%=", "ModEqual", true);
	new CLexRule(pLexSystem, "+=", "AddEqual", true);
	new CLexRule(pLexSystem, "-=", "SubEqual", true);
	new CLexRule(pLexSystem, "&=", "AndEqual", true);
	new CLexRule(pLexSystem, "|=", "OrEqual", true);
	new CLexRule(pLexSystem, "^=", "XorEqual", true);
	new CLexRule(pLexSystem, "||", "LogicOr", true);
	new CLexRule(pLexSystem, "&&", "LogicAnd", true);
	new CLexRule(pLexSystem, "==", "EqualEqual", true);
	new CLexRule(pLexSystem, "!=", "NotEqual", true);
	new CLexRule(pLexSystem, ">=", "GreaterEqual", true);
	new CLexRule(pLexSystem, "<=", "LessEqual", true);
	new CLexRule(pLexSystem, "<<", "LeftShift", true);
	new CLexRule(pLexSystem, ">>", "RightShift", true);
	new CLexRule(pLexSystem, "++", "PlusPlus", true);
	new CLexRule(pLexSystem, "--", "MinusMinus", true);
	new CLexRule(pLexSystem, "~", "Tilde", true, false);
//	new CLexRule(pLexSystem, "`", "BbackQuote", true, false);
	new CLexRule(pLexSystem, "!", "Exclam", true, false);
//	new CLexRule(pLexSystem, "@", "At", true, false);
	new CLexRule(pLexSystem, "#", "Sharp", true, false);
	new CLexRule(pLexSystem, "$", "Dollar", true, false);
	new CLexRule(pLexSystem, "%", "Percent", true, false);
	new CLexRule(pLexSystem, "^", "Caret", true, false);
	new CLexRule(pLexSystem, "&", "Ampersand", true, false);
	new CLexRule(pLexSystem, "*", "Star", true, false);
	new CLexRule(pLexSystem, "(", "LeftParen", true, false);
	new CLexRule(pLexSystem, ")", "RightParen", true, false);
//	new CLexRule(pLexSystem, "_", "UnderScore", true, false);//会与Identifier冲突
	new CLexRule(pLexSystem, "-", "Minus", true, false);
	new CLexRule(pLexSystem, "+", "Plus", true, false);
	new CLexRule(pLexSystem, "=", "Equal", true, false);
	new CLexRule(pLexSystem, "{", "LeftBrace", true, false);
	new CLexRule(pLexSystem, "}", "RightBrace", true, false);
	new CLexRule(pLexSystem, "[", "LeftBracket", true, false);
	new CLexRule(pLexSystem, "]", "RightBracket", true, false);
	new CLexRule(pLexSystem, "|", "Bar", true, false);
	new CLexRule(pLexSystem, "/", "Slash", true, false);
//	new CLexRule(pLexSystem, "\\", "BackSlash", true, false);
	new CLexRule(pLexSystem, ":", "Colon", true, false);
	new CLexRule(pLexSystem, ";", "Semicolon", true, false);
//	new CLexRule(pLexSystem, "\"", "DoubleQuote", true, false);//会与StringKey冲突
//	new CLexRule(pLexSystem, "\'", "Quote", true, false);//会与CharKey冲突
	new CLexRule(pLexSystem, "<", "Less", true, false);
	new CLexRule(pLexSystem, ">", "Greater", true, false);
	new CLexRule(pLexSystem, ",", "Comma", true, false);
	new CLexRule(pLexSystem, ".", "Period", true, false);
//	new CLexRule(pLexSystem, "?", "Question", true, false);//不支持问号表达式
	//定义正则单词
	new CLexRule(pLexSystem, IdentifierRegStr, "Identifier");
	new CLexRule(pLexSystem, CharRegStr, "CharKey");
	new CLexRule(pLexSystem, StringRegStr, "StringKey");
	new CLexRule(pLexSystem, IntegerRegStr, "IntegerKey");
	new CLexRule(pLexSystem, FloatRegStr, "FloatKey");
}

static void DefineSystemType(CSyntaxSystem* pSynSystem)
{
	new CCommonRuleType<bool, ARF_BOOL>(pSynSystem, "bool");
	new CCommonRuleType<int8, ARF_INT8>(pSynSystem, "char");
	new CCommonRuleType<int16, ARF_INT16>(pSynSystem, "short");
	new CCommonRuleType<int32, ARF_INT32>(pSynSystem, "int");
	new CCommonRuleType<int64, ARF_INT64>(pSynSystem, "long");
	new CCommonRuleType<uint8, ARF_UINT8>(pSynSystem, "uchar");
	new CCommonRuleType<uint16, ARF_UINT16>(pSynSystem, "ushort");
	new CCommonRuleType<uint32, ARF_UINT32>(pSynSystem, "uint");
	new CCommonRuleType<uint64, ARF_UINT64>(pSynSystem, "ulong");
	new CCommonRuleType<float, ARF_FLOAT>(pSynSystem, "float");
	new CCommonRuleType<double, ARF_DOUBLE>(pSynSystem, "double");
	new CCommonRuleType<CString, ARF_STRING>(pSynSystem, "string");
	if(sizeof(void*) == sizeof(uint32))
	{
		new CCommonRuleType<void*, ARF_UINT32>(pSynSystem, "token");
		new CCommonRuleType<void*, ARF_UINT32>(pSynSystem, "type");
	}
	else
	{
		new CCommonRuleType<void*, ARF_UINT64>(pSynSystem, "token");
		new CCommonRuleType<void*, ARF_UINT64>(pSynSystem, "type");
	}
	new CCommonRuleType<CMasterPointer<CRuleExpress>, ARF_OBJECT>(pSynSystem, "express");
	new CCommonRuleType<CMasterPointer<CRuleSentence>, ARF_OBJECT>(pSynSystem, "sentence");
	new CCommonRuleType<CMasterPointer<CRule>, ARF_OBJECT>(pSynSystem, "rule");
}

static void DefineMainRule(CSyntaxSystem* pSynSystem);
static void DefineWhiteSpaceRule(CSyntaxSystem* pSynSystem);
static void DefinePunctsRule(CSyntaxSystem* pSynSystem);
static void DefineTokensRule(CSyntaxSystem* pSynSystem);
static void DefineTokenRule(CSyntaxSystem* pSynSystem);
static void DefineTypeRule(CSyntaxSystem* pSynSystem);
static void DefineTypesRule(CSyntaxSystem* pSynSystem);
static void DefineStructRule(CSyntaxSystem* pSynSystem);
static void DefineStructBodyRule(CSyntaxSystem* pSynSystem);
static void DefineGbloalVariablesRule(CSyntaxSystem* pSynSystem);
static void DefineParametersRule(CSyntaxSystem* pSynSystem);
static void DefineLocalVariablesRule(CSyntaxSystem* pSynSystem);
static void DefineVariableRule(CSyntaxSystem* pSynSystem);
static void DefineParameterRule(CSyntaxSystem* pSynSystem);
static void DefineSyntaxsRule(CSyntaxSystem* pSynSystem);
static void DefineSyntaxRule(CSyntaxSystem* pSynSystem);
static void DefineRuleItem(CSyntaxSystem* pSynSystem);
static void DefineBreakRule(CSyntaxSystem* pSynSystem);
static void DefineCallRule(CSyntaxSystem* pSynSystem);
static void DefineIfRule(CSyntaxSystem* pSynSystem);
static void DefineSequenceRule(CSyntaxSystem* pSynSystem);
static void DefineTestRule(CSyntaxSystem* pSynSystem);
static void DefineOptimismRule(CSyntaxSystem* pSynSystem);
static void DefineChoiceRule(CSyntaxSystem* pSynSystem);
static void DefineLoopRule(CSyntaxSystem* pSynSystem);
static void DefineOptionRule(CSyntaxSystem* pSynSystem);
static void DefineMustRule(CSyntaxSystem* pSynSystem);
static void DefineSemanticRule(CSyntaxSystem* pSynSystem);
static void DefineFunctionRule(CSyntaxSystem* pSynSystem);
static void DefineFuncBodyRule(CSyntaxSystem* pSynSystem);
static void DefineSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineExpressSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineReturnSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineIfSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineWhileSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineDoSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineForSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineSwitchSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineElseSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineBreakSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineContinueSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineCaseSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineDefaultSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineComplexSentenceRule(CSyntaxSystem* pSynSystem);
static void DefineExpressRule(CSyntaxSystem* pSynSystem);
static void DefineAssignExpressRule(CSyntaxSystem* pSynSystem);
static void DefineLogicOrExpressRule(CSyntaxSystem* pSynSystem);
static void DefineLogicAndExpressRule(CSyntaxSystem* pSynSystem);
static void DefineBitOrExpressRule(CSyntaxSystem* pSynSystem);
static void DefineBitXorExpressRule(CSyntaxSystem* pSynSystem);
static void DefineBitAndExpressRule(CSyntaxSystem* pSynSystem);
static void DefineEqualExpressRule(CSyntaxSystem* pSynSystem);
static void DefineRelationExpressRule(CSyntaxSystem* pSynSystem);
static void DefineShiftExpressRule(CSyntaxSystem* pSynSystem);
static void DefineAddExpressRule(CSyntaxSystem* pSynSystem);
static void DefineMulExpressRule(CSyntaxSystem* pSynSystem);
static void DefineCastExpressRule(CSyntaxSystem* pSynSystem);
static void DefineUnaryExpressRule(CSyntaxSystem* pSynSystem);
static void DefinePrimaryExpressRule(CSyntaxSystem* pSynSystem);

static void DefineMetaRules(CSyntaxSystem* pSynSystem)
{
	DefineMainRule(pSynSystem);
	DefineWhiteSpaceRule(pSynSystem);
	DefinePunctsRule(pSynSystem);
	DefineTokensRule(pSynSystem);
	DefineTokenRule(pSynSystem);
	DefineTypeRule(pSynSystem);
	DefineTypesRule(pSynSystem);
	DefineStructRule(pSynSystem);
	DefineStructBodyRule(pSynSystem);
	DefineGbloalVariablesRule(pSynSystem);
	DefineParametersRule(pSynSystem);
	DefineLocalVariablesRule(pSynSystem);
	DefineVariableRule(pSynSystem);
	DefineParameterRule(pSynSystem);
	DefineSyntaxsRule(pSynSystem);
	DefineSyntaxRule(pSynSystem);
	DefineRuleItem(pSynSystem);
	DefineBreakRule(pSynSystem);
	DefineCallRule(pSynSystem);
	DefineIfRule(pSynSystem);
	DefineSequenceRule(pSynSystem);
	DefineTestRule(pSynSystem);
	DefineOptimismRule(pSynSystem);
	DefineChoiceRule(pSynSystem);
	DefineLoopRule(pSynSystem);
	DefineOptionRule(pSynSystem);
	DefineMustRule(pSynSystem);
	DefineSemanticRule(pSynSystem);
	DefineFunctionRule(pSynSystem);
	DefineFuncBodyRule(pSynSystem);
	DefineSentenceRule(pSynSystem);
	DefineExpressSentenceRule(pSynSystem);
	DefineReturnSentenceRule(pSynSystem);
	DefineIfSentenceRule(pSynSystem);
	DefineWhileSentenceRule(pSynSystem);
	DefineDoSentenceRule(pSynSystem);
	DefineForSentenceRule(pSynSystem);
	DefineSwitchSentenceRule(pSynSystem);
	DefineElseSentenceRule(pSynSystem);
	DefineBreakSentenceRule(pSynSystem);
	DefineContinueSentenceRule(pSynSystem);
	DefineCaseSentenceRule(pSynSystem);
	DefineDefaultSentenceRule(pSynSystem);
	DefineComplexSentenceRule(pSynSystem);
	DefineExpressRule(pSynSystem);
	DefineAssignExpressRule(pSynSystem);
	DefineLogicOrExpressRule(pSynSystem);
	DefineLogicAndExpressRule(pSynSystem);
	DefineBitOrExpressRule(pSynSystem);
	DefineBitXorExpressRule(pSynSystem);
	DefineBitAndExpressRule(pSynSystem);
	DefineEqualExpressRule(pSynSystem);
	DefineRelationExpressRule(pSynSystem);
	DefineShiftExpressRule(pSynSystem);
	DefineAddExpressRule(pSynSystem);
	DefineMulExpressRule(pSynSystem);
	DefineCastExpressRule(pSynSystem);
	DefineUnaryExpressRule(pSynSystem);
	DefinePrimaryExpressRule(pSynSystem);
}

#define V(X) (new CRuleVariableExpress(X))
#define T(X) CTokenRule(CMetaCompileSystem::GetInstance()->GetLexicalSystem(), X)//定义普通单词
#define D(X, Name) CTokenRule(CMetaCompileSystem::GetInstance()->GetLexicalSystem(), X).SetOutput(V(Name))//定义数据单词
#define C(X) CCallRule X
#define IF(X) CIfRule(new CRuleVariableExpress(X))

static void DefineMainRule(CSyntaxSystem* pSynSystem)
{//	语法定义 -> [空白定义] [标记定义] 单词定义 [类型定义] [全局变量] 规则定义
	CRule* pRule = (
					   -C(("WhiteSpaceRule", NULL)) >>
					   -C(("PunctsRule", NULL)) >>
					   C(("TokensRule", NULL)) >>
					   -C(("TypesRule", NULL)) >>
					   -C(("VariablesRule", NULL)) >>
					   C(("SyntaxsRule", NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "MainRule", pRule, true, NULL, NULL);
}

static void SetWhiteSpace(CRuleStack &oStack)
{//"express", "Exp", 0
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	CRuleConstExpress* pConstExp = dynamic_cast<CRuleConstExpress*>((CRuleExpress*)oExp);
	if(pConstExp == NULL)
	{
		pLexModule->OnError(*oExp, "must be const string for WHITESPACE declare");
		return;
	}
	pConstExp->Check(*oStack.pSystem, pLexModule);
	if(ARF_STRING != pConstExp->GetExpressType(oStack.pSystem)->TypeCode())
	{
		pLexModule->OnError(*oExp, "must be const string for WHITESPACE declare");
		return;
	}
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	pLexSystem->SetWhiteSpace((const char*)pConstExp->GetExpressValue(&oStack));
}

static void DefineWhiteSpaceRule(CSyntaxSystem* pSynSystem)
{//	空白定义 -> WHITESPACE '=' 字符串 '.'
	CRule* pRule = (
					   T("WHITESPACE") >>
					   T("Equal") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("Period") >>
					   C(("SetWhiteSpace", V("Exp"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "WhiteSpaceRule", pRule, false, NULL, "express Exp", NULL);
	new CSyntaxRule(pSynSystem, "SetWhiteSpace", NULL, false, "express Exp", NULL, NULL);
	pSynSystem->RegHost("SetWhiteSpace", SetWhiteSpace);
}

static void SetPuncts(CRuleStack &oStack)
{//"express", "sPuncts", 0
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	CRuleConstExpress* pConstExp = dynamic_cast<CRuleConstExpress*>((CRuleExpress*)oExp);
	if(pConstExp == NULL)
	{
		pLexModule->OnError(*oExp, "must be const string for PUNCTS declare");
		return;
	}
	pConstExp->Check(*oStack.pSystem, pLexModule);
	if(ARF_STRING != pConstExp->GetExpressType(oStack.pSystem)->TypeCode())
	{
		pLexModule->OnError(*oExp, "must be const string for PUNCTS declare");
		return;
	}
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	pLexSystem->SetPuncts((const char*)pConstExp->GetExpressValue(&oStack));
}

static void DefinePunctsRule(CSyntaxSystem* pSynSystem)
{//	标记定义 -> PUNCTS '=' 字符串 '.'
	CRule* pRule = (
					   T("PUNCTS") >>
					   T("Equal") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("Period") >>
					   C(("SetPuncts", V("Exp"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "PunctsRule", pRule, false, NULL, "express Exp", NULL);
	new CSyntaxRule(pSynSystem, "SetPuncts", NULL, false, "express Exp", NULL, NULL);
	pSynSystem->RegHost("SetPuncts", SetPuncts);
}

static void DefineTokensRule(CSyntaxSystem* pSynSystem)
{//	单词定义 -> TOKENS ':' 单个单词 {单个单词}
	CRule* pRule = (
					   T("TOKENS") >>
					   T("Colon") >>
					   C(("TokenRule", NULL)) >>
					   *C(("TokenRule", NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "TokensRule", pRule, false, NULL, NULL);
}

static void CreateToken(CRuleStack &oStack)
{//"token", "pTokenName", 0, "express", "pTokenStr", 0, "token", "pConst", 0, "token", "pInsensitive", 0, "token", "pSkip", 0
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pTokenName = GetRuleArgToken(oArgv);
	const char* sTokenName = pTokenName->GetToken();
	CMasterPointer<CRuleExpress>& oTokenStrExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleConstExpress* pTokenStr = dynamic_cast<CRuleConstExpress*>((CRuleExpress*)oTokenStrExp);
	bool bConst = GetRuleArgToken(oArgv)?true:false;
	bool bInsensitive = GetRuleArgToken(oArgv)?true:false;
	bool bSkip = GetRuleArgToken(oArgv)?true:false;

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	if(pTokenStr == NULL)
	{
		pLexModule->OnError(*oTokenStrExp, "must be const string for token string declare");
		return;
	}
	pTokenStr->Check(*oStack.pSystem, pLexModule);
	if(ARF_STRING != pTokenStr->GetExpressType(oStack.pSystem)->TypeCode())
	{
		pLexModule->OnError(*oTokenStrExp, "must be const string for token string declare");
		return;
	}
	const char* sTokenStr = (const char*)pTokenStr->GetExpressValue(&oStack);

	if(pLexSystem->GetRule(sTokenName))
	{
		pLexModule->OnError(*pTokenName, "Redefine token '%s'", sTokenName);
		return;
	}

	if(pSynSystem->GetType(sTokenName))
	{
		pLexModule->OnError(*pTokenName, "Redefine type '%s' into token", sTokenName);
		return;
	}

	if(pSynSystem->GetVariables()->FindField(sTokenName))
	{
		pLexModule->OnError(*pTokenName, "Redefine global variable '%s' into token", sTokenName);
		return;
	}

	if(pSynSystem->GetRule(sTokenName))
	{
		pLexModule->OnError(*pTokenName, "Redefine rule '%s' into token", sTokenName);
		return;
	}

	if(pSynSystem->GetFunc(sTokenName))
	{
		pLexModule->OnError(*pTokenName, "Redefine function '%s' into token", sTokenName);
		return;
	}

	CLexRule* pRule = new CLexRule(pLexSystem, sTokenStr, sTokenName, bConst, !bInsensitive, bSkip);
	pRule->SetFile(*pTokenName);
}

static void DefineTokenRule(CSyntaxSystem* pSynSystem)
{//	单个单词 -> [CONST [INSENSITIVE] ] 单词名 '=' [SKIP] 正则串 '.'
	CRule* pRule = (
					   -(
						   D("CONST", "pConst") >>
						   -D("INSENSITIVE", "pInsensitive")
					   ) >>
					   D("Identifier", "pTokenName") >>
					   T("Equal") >>
					   -D("SKIP", "pSkip") >>
					   C(("ExpressRule", V("pTokenStr"), NULL)) >>
					   T("Period") >>
					   C(("CreateToken", V("pTokenName"), V("pTokenStr"), V("pConst"), V("pInsensitive"), V("pSkip"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "TokenRule", pRule, false, NULL,
					"token pConst",
					"token pInsensitive",
					"token pSkip",
					"token pTokenName",
					"express pTokenStr", NULL);
	new CSyntaxRule(pSynSystem, "CreateToken", NULL, false,
					"token pTokenName",
					"express pTokenStr",
					"token pConst",
					"token pInsensitive",
					"token pSkip", NULL, NULL);
	pSynSystem->RegHost("CreateToken", CreateToken);
}

static void CreateType(CRuleStack &oStack)
{//type& Type, token TypeName, Vector
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CRuleType*& pType = GetRuleArgObject<CRuleType*>(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	CToken* pVector = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();

	pType = pSynSystem->GetType(sTypeName);
	if(pType == NULL)
	{
		pLexModule->OnError(*pTypeName, "Undefined type name '%s'", sTypeName);
		return;
	}

	if(pVector)
		pType = new CRuleVector(pSynSystem, sTypeName);
}

static void DefineTypeRule(CSyntaxSystem* pSynSystem)
{//类型名 | VECTOR '<' 类型名 '>'
	CRule* pRule = (
						(
							D("Identifier", "TypeName") |
							(
								D("VECTOR", "Vector") >>
								T("Less") >>
								D("Identifier", "TypeName") >>
								T("Greater")
							)
						) >>
						C(("CreateType", V("Type"), V("TypeName"), V("Vector"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "TypeRule", pRule, false,
					"type& Type", NULL, 
					"token TypeName",
					"token Vector", NULL);
	new CSyntaxRule(pSynSystem, "CreateType", NULL, false,
					"type& Type",
					"token TypeName",
					"token Vector", NULL, NULL);
	pSynSystem->RegHost("CreateType", CreateType);
}

static void DefineTypesRule(CSyntaxSystem* pSynSystem)
{//	类型定义 -> TYPES ':' 单个类型 {单个类型}
	CRule* pRule = (
					   T("TYPES") >>
					   T("Colon") >>
					   C(("StructRule", NULL)) >>
					   *C(("StructRule", NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "TypesRule", pRule, false, NULL, NULL);
}

static void CreateStruct(CRuleStack &oStack)
{//token Struct, "token", "pTypeName", "type", "pType"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pStruct = GetRuleArgToken(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);
	const char* sTypeName = pTypeName->GetToken();
	CRuleType*& pType = GetRuleArgObject<CRuleType*>(oArgv);

	bool bUnion = !CString::StringCompare(pStruct->GetKind(), "UNION", false);

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	pType = pSynSystem->GetType(sTypeName);
	if(pType == NULL)
	{
		if(pLexSystem->GetRule(sTypeName))
			pLexModule->OnError(*pTypeName, "Redefine token '%s' into %s", sTypeName, bUnion?"union":"struct");
		else if(pSynSystem->GetVariables()->FindField(sTypeName))
			pLexModule->OnError(*pTypeName, "Redefine global variable '%s' into %s", sTypeName, bUnion?"union":"struct");
		else if(pSynSystem->GetRule(sTypeName))
			pLexModule->OnError(*pTypeName, "Redefine rule '%s' into %s", sTypeName, bUnion?"union":"struct");
		else if(pSynSystem->GetFunc(sTypeName))
			pLexModule->OnError(*pTypeName, "Redefine function '%s' into %s", sTypeName, bUnion?"union":"struct");
		else
		{
			pType = new CRuleStruct(pSynSystem, sTypeName, bUnion);
			pType->SetFile(*pTypeName);
		}
	}
	else if(pType->TypeCode()!= ARF_STRUCT)
	{
		pLexModule->OnError(*pTypeName, "Redefine type '%s' into %s", sTypeName, bUnion?"union":"struct");
		pType = NULL;
	}
	else
	{
		CRuleStruct* pStruct = (CRuleStruct*)pType;
		if(pStruct->IsUnion() != bUnion)
		{
			pLexModule->OnError(*pTypeName, "Redefine type '%s %s' into %s", pStruct->IsUnion()?"union":"struct", sTypeName, bUnion?"union":"struct");
			pType = NULL;
		}
	}
}

static void DefineStructRule(CSyntaxSystem* pSynSystem)
{//	单个类型 -> (STRUCT|UNION) 标识符 [结构体] ';'
	CRule* pRule = (
					   ( D("STRUCT", "Struct") | D("UNION", "Struct")) >>
					   D("Identifier", "pTypeName") >>
					   C(("CreateStruct", V("Struct"), V("pTypeName"), V("pType"), NULL)) >>
					   -C(("StructBodyRule", V("pType"), V("pTypeName"), NULL)) >>
					   T("Semicolon")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "StructRule", pRule, false, NULL,
					"token pTypeName",
					"type pType", 
					"token Struct", NULL);
	new CSyntaxRule(pSynSystem, "CreateStruct", NULL, false,
					"token Struct", 
					"token pTypeName",
					"type & pType", NULL, NULL);
	pSynSystem->RegHost("CreateStruct", CreateStruct);
}

static void CheckStruct(CRuleStack &oStack)
{//"type", "pStruct", token pTypeName
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleStruct*& pType = GetRuleArgObject<CRuleStruct*>(oArgv);
	CToken* pTypeName = GetRuleArgToken(oArgv);

	if(pType && pType->Implemented())
	{
		CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
		CLexicalModule* pLexModule = pModule->GetLexicalModule();
		pLexModule->OnError(*pTypeName, "Redefine type '%s'", pType->GetName());
		pType = NULL;
	}
}

static void CloseStruct(CRuleStack &oStack)
{//"type", "pStruct"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleStruct*& pType = (CRuleStruct*&)GetRuleArgType(oArgv);
	if(pType)
		pType->FinishDefine();
}

static void DefineStructBodyRule(CSyntaxSystem* pSynSystem)
{//	结构体	 -> '{' { 字段定义 ';'} '}'
	CRule* pRule = (
					   T("LeftBrace") >>
					   C(("CheckStruct", V("pStruct"), V("pTypeName"), NULL)) >>
					   *(
						   C(("VariableRule", V("pStruct"), NULL)) >>
						   T("Semicolon")
					   ) >>
					   T("RightBrace") >>
					   C(("CloseStruct", V("pStruct"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "StructBodyRule", pRule, false,
					"type pStruct",
					"token pTypeName", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CheckStruct", NULL, false,
					"type & pStruct",
					"token pTypeName", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CloseStruct", NULL, false,
					"type pStruct", NULL, NULL);
	pSynSystem->RegHost("CheckStruct", CheckStruct);
	pSynSystem->RegHost("CloseStruct", CloseStruct);
}

static void GetGlobalVariables(CRuleStack &oStack)
{//"type", "pStruct"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleStruct*& pType = GetRuleArgObject<CRuleStruct*>(oArgv);

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	pType = pSynSystem->GetVariables();
}

static void DefineGbloalVariablesRule(CSyntaxSystem* pSynSystem)
{//VARIABLES ':' 变量定义 ';' {变量定义 ';'}
	CRule* pRule = (
					   T("VARIABLES") >>
					   T("Colon") >>
					   C(("GetGlobalVariables", V("pStruct"), NULL)) >>
					   C(("VariableRule", V("pStruct"), NULL)) >>
					   T("Semicolon") >>
					   *(
						   C(("VariableRule", V("pStruct"), NULL)) >>
						   T("Semicolon")
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "VariablesRule", pRule, false, NULL,
					"type pStruct", NULL);
	new CSyntaxRule(pSynSystem, "GetGlobalVariables", NULL, false,
					"type& pStruct", NULL, NULL);
	pSynSystem->RegHost("GetGlobalVariables", GetGlobalVariables);
}

static void GetLocalVariables(CRuleStack &oStack)
{//"type", "pStruct"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleStruct*& pType = GetRuleArgObject<CRuleStruct*>(oArgv);

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CRuleProc* pProc = pModule->GetProc();
	if(pProc)
		pType = pProc->GetVariables();
	else
	{
		CRuleFunc * pFunc = pModule->GetFunc();
		if(pFunc)
			pType = pFunc->GetVariables();
		else
			pType = NULL;
	}
}

static void DefineLocalVariablesRule(CSyntaxSystem* pSynSystem)
{//'<' 变量定义 { ',' 变量定义} '>'
	CRule* pRule = (
					   T("Less") >>
					   C(("GetLocalVariables", V("pStruct"), NULL)) >>
					   C(("VariableRule", V("pStruct"), NULL)) >>
					   *(
						   T("Comma") >>
						   C(("VariableRule", V("pStruct"), NULL))
					   ) >>
					   T("Greater")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "LocalVariablesRule", pRule, false, NULL,
					"type pStruct", NULL);
	new CSyntaxRule(pSynSystem, "GetLocalVariables", NULL, false,
					"type& pStruct", NULL, NULL);
	pSynSystem->RegHost("GetLocalVariables", GetLocalVariables);
}

static void CreateVariable(CRuleStack &oStack)
{//"type", "pStruct", "type", "pType", "token", "VarName"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleStruct* pStruct = (CRuleStruct*)GetRuleArgType(oArgv);
	CRuleType*& pType = GetRuleArgType(oArgv);
	CToken* pVarToken = GetRuleArgToken(oArgv);
	const char* sVarName = pVarToken->GetToken();

	if(pType == NULL)
		return;

	bool bVector = (pType->TypeCode() == ARF_VECTOR);
	if(pLexSystem->GetRule(sVarName))
	{
		pLexModule->OnError(*pVarToken, "Redefine token '%s' into variable or field", sVarName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetType(sVarName))
	{
		pLexModule->OnError(*pVarToken, "Redefine type '%s' into variable or field", sVarName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetRule(sVarName))
	{
		pLexModule->OnError(*pVarToken, "Redefine rule '%s' into variable or field", sVarName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetFunc(sVarName))
	{
		pLexModule->OnError(*pVarToken, "Redefine function '%s' into variable or field", sVarName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pStruct)
	{
		if(pStruct->FindField(sVarName))
		{
			pLexModule->OnError(*pVarToken, "Redefine variable of field '%s'", sVarName);
			if(bVector)
			{
				delete pType;
				pType = NULL;
			}
			return;
		}

		CRuleVariable* pVar = new CRuleVariable(pType, sVarName);
		pStruct->AddField(pVar);
		pVar->SetFile(*pVarToken);
	}
}

static void DefineVariableRule(CSyntaxSystem* pSynSystem)
{//类型 变量名
	CRule* pRule = (
					   C(("TypeRule", V("pType"), NULL)) >>
					   D("Identifier", "VarName") >>
					   C(("CreateVariable", V("pStruct"), V("pType"), V("VarName"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "VariableRule", pRule, false,
					"type pStruct", NULL,
					"type pType",
					"token VarName", NULL);
	new CSyntaxRule(pSynSystem, "CreateVariable", NULL, false,
					"type pStruct", 
					"type pType",
					"token VarName", NULL, NULL);
	pSynSystem->RegHost("CreateVariable", CreateVariable);
}

static void GetParameters(CRuleStack &oStack)
{//"type", "pStruct"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleStruct*& pStruct = GetRuleArgObject<CRuleStruct*>(oArgv);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CRuleProc* pProc = pModule->GetProc();
	if(pProc)
		pStruct = pProc->GetParameters();
	else
	{
		CRuleFunc* pFunc = pModule->GetFunc();
		if(pFunc)
			pStruct = pFunc->GetParameters();
		else
			pStruct = NULL;
	}
}

static void DefineParametersRule(CSyntaxSystem* pSynSystem)
{//'<' 参数定义 { ',' 参数定义} '>'
	CRule* pRule = (
					   T("Less") >>
					   C(("GetParameters", V("pStruct"), NULL)) >>
					   C(("ParameterRule", V("pStruct"), NULL)) >>
					   *(
						   T("Comma") >>
						   C(("ParameterRule", V("pStruct"), NULL))
					   ) >>
					   T("Greater")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ParametersRule", pRule, false, NULL,
					"type pStruct", NULL);
	new CSyntaxRule(pSynSystem, "GetParameters", NULL, false,
					"type& pStruct", NULL, NULL);
	pSynSystem->RegHost("GetParameters", GetParameters);
}

static void CreateParameter(CRuleStack &oStack)
{//"type", "pStruct", "token", "type pType", "token", "Out", "token", "ParaName"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);

	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleStruct* pStruct = (CRuleStruct*)GetRuleArgType(oArgv);
	CRuleType*& pType = GetRuleArgType(oArgv);
	bool bOut = GetRuleArgToken(oArgv)?true:false;
	CToken* pParaToken = GetRuleArgToken(oArgv);
	const char* sParaName = pParaToken->GetToken();

	if(pType == NULL)
		return;
	
	bool bVector = (pType->TypeCode() == ARF_VECTOR);
	if(pLexSystem->GetRule(sParaName))
	{
		pLexModule->OnError(*pParaToken, "Redefine token '%s' into parameter", sParaName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetType(sParaName))
	{
		pLexModule->OnError(*pParaToken, "Redefine type '%s' into parameter", sParaName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetRule(sParaName))
	{
		pLexModule->OnError(*pParaToken, "Redefine rule '%s' into parameter", sParaName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pSynSystem->GetFunc(sParaName))
	{
		pLexModule->OnError(*pParaToken, "Redefine function '%s' into parameter", sParaName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}

	if(pStruct)
	{//创建规则过程失败时，该参数为NULL
		if(pStruct->FindField(sParaName))
		{
			pLexModule->OnError(*pParaToken, "Redefine parameter '%s'", sParaName);
			if(bVector)
			{
				delete pType;
				pType = NULL;
			}
			return;
		}
		CRuleParameter* pPara = new CRuleParameter(pType, sParaName, bOut);
		pStruct->AddField(pPara);
		pPara->SetFile(*pParaToken);
	}
}

static void DefineParameterRule(CSyntaxSystem* pSynSystem)
{//类型 [ '&' ] 参数名
	CRule* pRule = (
					   C(("TypeRule", V("pType"), NULL)) >>
					   -D("Ampersand", "Out") >>
					   D("Identifier", "ParaName") >>
					   C(("CreateParameter", V("pStruct"), V("pType"), V("Out"), V("ParaName"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ParameterRule", pRule, false,
					"type pStruct", NULL,
					"type pType",
					"token Out",
					"token ParaName", NULL);
	new CSyntaxRule(pSynSystem, "CreateParameter", NULL, false,
					"type pStruct",
					"type pType",
					"token Out",
					"token ParaName", NULL, NULL);
	pSynSystem->RegHost("CreateParameter", CreateParameter);
}

static void CloseGlobalVariables(CRuleStack &oStack)
{
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	pSynSystem->GetVariables()->FinishDefine();
}

static void DefineSyntaxsRule(CSyntaxSystem* pSynSystem)
{//RULES ':' (单个规则|单个函数) {(单个规则|单个函数)}
	CRule* pRule = (
					   T("RULES") >>
					   T("Colon") >>
					   C(("CloseGlobalVariables", NULL)) >>
					   C(("SyntaxRule", NULL)) >>
					   *C(("SyntaxRule", NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "SyntaxsRule", pRule, false, NULL, NULL);
	new CSyntaxRule(pSynSystem, "CloseGlobalVariables", NULL, false, NULL, NULL);
	pSynSystem->RegHost("CloseGlobalVariables", CloseGlobalVariables);
}

static void CreateRuleProc(CRuleStack &oStack)
{//"token", "bMain", "token", "ProcName"
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool bMain = GetRuleArgToken(oArgv)?true:false;
	CToken* pProcName = GetRuleArgToken(oArgv);
	const char* sProcName = pProcName->GetToken();

	pModule->SetProc(NULL);
	if(pLexSystem->GetRule(sProcName))
	{
		pLexModule->OnError(*pProcName, "Redefine token '%s' into rule", sProcName);
		return;
	}
	if(pSynSystem->GetType(sProcName))
	{
		pLexModule->OnError(*pProcName, "Redefine type '%s' into rule", sProcName);
		return;
	}
	if(pSynSystem->GetVariables()->FindField(sProcName))
	{
		pLexModule->OnError(*pProcName, "Redefine global variable '%s' into rule", sProcName);
		return;
	}
	if(pSynSystem->GetFunc(sProcName))
	{
		pLexModule->OnError(*pProcName, "Redefine function '%s' into rule", sProcName);
		return;
	}
	if(pSynSystem->GetRule(sProcName))
	{
		pLexModule->OnError(*pProcName, "Redefine rule '%s'", sProcName);
		return;
	}
	CSyntaxRule* pProc = new CSyntaxRule(pSynSystem, sProcName, NULL, bMain, NULL, NULL);
	pModule->SetProc(pProc);
	pProc->SetFile(*pProcName);
}

static void SetRuleProcBody(CRuleStack &oStack)
{//"rule", "ProcBody"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CRuleProc* pProc = pModule->GetProc();
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	if(pProc)
	{
		CRule* pRule = oRule.Detach();
		if(pRule)
			pProc->SetRule(pRule);
	}
}

static void IsFunctionRule(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool &bRet = GetRuleArgObject<bool>(oArgv);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxModule* pSyntaxModule = pModule->GetSyntaxModule();
	bRet = false;
	CToken* pNextToken = pSyntaxModule->PullToken(false);
	if(pNextToken && !CString::StringCompare(pNextToken->GetKind(), "Identifier", false))
	{
		pNextToken = pSyntaxModule->PeekToken();
		if(pNextToken && !CString::StringCompare(pNextToken->GetKind(), "Identifier", false))
			bRet = true;
	}
	else if(pNextToken && !CString::StringCompare(pNextToken->GetKind(), "Vector", false))
		bRet = true;
}

static void DefineSyntaxRule(CSyntaxSystem* pSynSystem)
{//	(MAIN 规则名 | 规则名 [ 规则参数 ] ) [ '=' [局部变量] 规则项 ] '.'
	CRule* pRule =	(
						(
							C(("IsFunctionRule", V("IsFun"), NULL)) >>
							IF("IsFun") >> C(("FunctionRule", NULL))
						) |
						(
							(
								(
									D("MAIN", "bMain") >>
									D("Identifier", "ProcName") >>
									C(("CreateRuleProc", V("bMain"), V("ProcName"), NULL))
								) |
								(
									D("Identifier", "ProcName") >>
									C(("CreateRuleProc", V("bMain"), V("ProcName"), NULL)) >>
									-C(("ParametersRule", NULL))
								)
							) >>
							T("Equal") >>
							-C(("LocalVariablesRule", NULL)) >>
							C(("RuleItem", V("ProcBody"), NULL)) >>
							C(("SetRuleProcBody", V("ProcBody"), NULL)) >>
							T("Period")
						)
					).Clone();
	new CSyntaxRule(pSynSystem, "SyntaxRule", pRule, false, NULL,
					"bool IsFun",
					"token bMain",
					"token ProcName",
					"rule ProcBody", NULL);
	new CSyntaxRule(pSynSystem, "IsFunctionRule", NULL, false,
					"bool& IsFun", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateRuleProc", NULL, false,
					"token bMain",
					"token ProcName", NULL, NULL);
	new CSyntaxRule(pSynSystem, "SetRuleProcBody", NULL, false,
					"rule ProcBody", NULL, NULL);
	pSynSystem->RegHost("IsFunctionRule", IsFunctionRule);
	pSynSystem->RegHost("CreateRuleProc", CreateRuleProc);
	pSynSystem->RegHost("SetRuleProcBody", SetRuleProcBody);
}

static void CreateFunction(CRuleStack &oStack)
{//"token", "TypeName", "token", "FuncName"
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CRuleType*& pType = GetRuleArgType(oArgv);
	CToken* pFuncName = GetRuleArgToken(oArgv);
	const char* sFuncName = pFuncName->GetToken();

	pModule->SetFunc(NULL);
	if(pType == NULL)
		return;

	bool bVector = (pType->TypeCode() == ARF_VECTOR);
	if(pSynSystem->GetFunc(sFuncName))
	{
		pLexModule->OnError(*pFuncName, "redefined function '%s", sFuncName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}
	if(pLexSystem->GetRule(sFuncName))
	{
		pLexModule->OnError(*pFuncName, "Redefine token '%s' into function", sFuncName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}
	if(pSynSystem->GetType(sFuncName))
	{
		pLexModule->OnError(*pFuncName, "Redefine type '%s' into function", sFuncName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}
	if(pSynSystem->GetVariables()->FindField(sFuncName))
	{
		pLexModule->OnError(*pFuncName, "Redefine global variable '%s' into function", sFuncName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}
	if(pSynSystem->GetRule(sFuncName))
	{
		pLexModule->OnError(*pFuncName, "Redefine rule '%s' into function", sFuncName);
		if(bVector)
		{
			delete pType;
			pType = NULL;
		}
		return;
	}
	CRuleFunc* pFunc = new CRuleFunc(pSynSystem, sFuncName, pType, NULL, NULL);
	pFunc->SetFile(*pFuncName);
	pModule->SetFunc(pFunc);
}

static void DefineFunctionRule(CSyntaxSystem* pSynSystem)
{//类型 函数名 '(' [ 参数定义 { ',' 参数定义} ] ')' (函数体 | ';')
	CRule* pRule =	(
						C(("TypeRule", V("pType"), NULL)) >>
						D("Identifier", "FuncName") >>
						C(("CreateFunction", V("pType"), V("FuncName"), NULL)) >>
						T("LeftParen") >>
						C(("GetParameters", V("pStruct"), NULL)) >>
						-(
							C(("ParameterRule", V("pStruct"), NULL)) >>
							*(
								T("Comma") >>
								C(("ParameterRule", V("pStruct"), NULL))
							)
						) >>
						T("RightParen") >>
						(
							T("Semicolon") |
							C(("FuncBodyRule", NULL))
						)
					).Clone();
	new CSyntaxRule(pSynSystem, "FunctionRule", pRule, false, NULL,
					"type pType",
					"token FuncName",
					"type pStruct", NULL);
	new CSyntaxRule(pSynSystem, "CreateFunction", NULL, false,
					"type pType",
					"token FuncName", NULL, NULL);
	pSynSystem->RegHost("CreateFunction", CreateFunction);
}

static void SetFuncBody(CRuleStack &oStack)
{//sentence oSentence
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CRuleFunc* pFunc = pModule->GetFunc();
	if(pFunc)
		pFunc->SetSentence((CRuleComplexSentence*)oSentence.Detach());
}

static void DefineFuncBodyRule(CSyntaxSystem* pSynSystem)
{//	'{' {变量定义 ';'} {语句定义} '}'
	CRule* pRule =	(
						D("LeftBrace", "Pos") >>
						C(("GetLocalVariables", V("pStruct"), NULL)) >>
						*(
							C(("VariableRule", V("pStruct"), NULL)) >>
							T("Semicolon")
						) >>
						C(("CreateComplexSentence", V("oSentence"), (new CRuleConstExpress(false)), V("Pos"), NULL)) >>
						*(
							C(("SentenceRule", V("SubSentence"), NULL)) >>
							C(("AddSentence", V("oSentence"), V("SubSentence"), (new CRuleConstExpress(false)), NULL))
						) >>
						T("RightBrace") >>
						C(("SetFuncBody", V("oSentence"), NULL))
					).Clone();
	new CSyntaxRule(pSynSystem, "FuncBodyRule", pRule, false, NULL,
					"token Pos",
					"type pStruct",
					"sentence oSentence",
					"sentence SubSentence", NULL);
	new CSyntaxRule(pSynSystem, "SetFuncBody", NULL, false,
					"sentence oSentence", NULL, NULL);
	pSynSystem->RegHost("SetFuncBody", SetFuncBody);
}

static void DefineRuleItem(CSyntaxSystem* pSynSystem)
{//	中断规则 | 条件规则 | 调用规则 | 顺序规则 | 测试规则 | 乐观规则 | 选择规则 | 任意次规则 | 至少1次规则 | 可选规则 | 语义规则
	CRule* pRule =	(
						C(("BreakRule", V("Item"), NULL)) |
						C(("CallRule", V("Item"), NULL)) |
						C(("IfRule", V("Item"), NULL)) |
						C(("SequenceRule", V("Item"), NULL)) |
						C(("TestRule", V("Item"), NULL)) |
						C(("OptimismRule", V("Item"), NULL)) |
						C(("ChoiceRule", V("Item"), NULL)) |
						C(("LoopRule", V("Item"), NULL)) |
						C(("OptionRule", V("Item"), NULL)) |
						C(("MustRule", V("Item"), NULL)) |
						C(("SemanticRule", V("Item"), NULL))
					).Clone();
	new CSyntaxRule(pSynSystem, "RuleItem", pRule, false,
					"rule& Item", NULL, NULL);
}

static void CreateBreakRule(CRuleStack &oStack)
{//"rule", "Item", token nBreak
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CToken* pToken = GetRuleArgToken(oArgv);
	CBreakRule* pRule = new CBreakRule();
	pRule->SetFile(*pToken);
	oRule = pRule;
}

static void DefineBreakRule(CSyntaxSystem* pSynSystem)
{//中断规则 -> 'Break'
	CRule* pRule =	(
						D("BREAK", "nBreak") >>
						C(("CreateBreakRule", V("Item"),  V("nBreak"), NULL))
					).Clone();
	new CSyntaxRule(pSynSystem, "BreakRule", pRule, false,
					"rule& Item", NULL,
					"token nBreak", NULL);
	new CSyntaxRule(pSynSystem, "CreateBreakRule", NULL, false,
					"rule& Item",
					"token nBreak", NULL, NULL);
	pSynSystem->RegHost("CreateBreakRule", CreateBreakRule);
}

static void CreateCallRule(CRuleStack &oStack)
{//"token", "Flag", "token", "RuleName", "rule", "Item"
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalSystem* pLexSystem = pModule->GetTargetLexicalSystem();
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pFlag = GetRuleArgToken(oArgv);
	CToken* pRuleName = GetRuleArgToken(oArgv);
	const char* sRuleName = pRuleName->GetToken();
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);

	CRule* pRule = NULL;
	if(pLexSystem->GetRule(sRuleName))
	{
		CTokenRule* pTokenRule = new CTokenRule(pLexSystem, sRuleName);
		if(pFlag)
		{
			if(!CString::StringCompare(pFlag->GetKind(), "Sharp", false))
				pTokenRule->SetFirst();
			else
				pTokenRule->SetHome();
		}
		pRule = pTokenRule;
	}
	else
	{
		pRule = new CCallRule(sRuleName, NULL);
		if(pFlag)
			pLexModule->OnError(*pRuleName, "undefined token '%s'", sRuleName);
	}
	pRule->SetFile(*pRuleName);
	oRule = pRule;
}

static void AddRuleExp(CRuleStack &oStack)
{//"rule", "Item", "express", "Exp"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRule* pRule = oRule;
	CRuleExpress* pExp;
	CTokenRule* pTokenRule = dynamic_cast<CTokenRule*>(pRule);
	if(pTokenRule)
	{
		if(pTokenRule->GetOutExp())
		{
			pLexModule->OnError(*pRule, "too many argument for token rule");
			return;
		}
		pExp = oExp.Detach();
		if(pExp)
			pTokenRule->SetOutput(pExp);
		return;
	}
	CCallRule* pCallRule = dynamic_cast<CCallRule*>(pRule);
	if(pCallRule)
	{
		pExp = oExp.Detach();
		if(pExp)
			pCallRule->AddExpress(pExp);
	}
}

static void DefineCallRule(CSyntaxSystem* pSynSystem)
{//['$' | '#'] 规则名 [ '<!' 赋值表达式 {',' 赋值表达式} '!>' ]
 // '$'表示行首单词，列号可不为1
 // '#'表示行首单词，列号必须为1
	CRule* pRule = (
					   -(
						   D("Sharp", "Flag") |
						   D("Dollar", "Flag")
					   ) >>
					   D("Identifier", "RuleName") >>
					   C(("CreateCallRule", V("Flag"), V("RuleName"), V("Item"), NULL)) >>
					   -(
						   T("LessExclam") >>
						   C(("AssignExpressRule", V("Exp"), NULL)) >>
						   C(("AddRuleExp", V("Item"), V("Exp"), NULL)) >>
						   *(
							   T("Comma") >>
							   C(("AssignExpressRule", V("Exp"), NULL)) >>
							   C(("AddRuleExp", V("Item"), V("Exp"), NULL))
						   ) >>
						   T("GreaterExclam")
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "CallRule", pRule, false,
					"rule& Item", NULL,
					"express Exp",
					"token RuleName",
					"token Flag", NULL);
	new CSyntaxRule(pSynSystem, "CreateCallRule", NULL, false,
					"token Flag",
					"token RuleName",
					"rule& Item", NULL, NULL);
	new CSyntaxRule(pSynSystem, "AddRuleExp", NULL, false,
					"rule Item",
					"express Exp", NULL, NULL);
	pSynSystem->RegHost("CreateCallRule", CreateCallRule);
	pSynSystem->RegHost("AddRuleExp", AddRuleExp);
}

static void CreateIfRule(CRuleStack &oStack)
{//"rule", "Item", "express", "Exp", "token", "Pos",
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CToken* pPos = GetRuleArgToken(oArgv);
	CRuleExpress* pExp = oExp.Detach();
	CRule* pRule = new CIfRule(pExp);
	pRule->SetFile(*pPos);
	oRule = pRule;
}

static void DefineIfRule(CSyntaxSystem* pSynSystem)
{//IF '<!' 表达式 '!>'
	CRule* pRule = (
					   D("IF", "Pos") >>
					   T("LessExclam") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   C(("CreateIfRule", V("Item"), V("Exp"), V("Pos"), NULL)) >>
					   T("GreaterExclam")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "IfRule", pRule, false,
					"rule& Item", NULL,
					"express Exp",
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateIfRule", NULL, false,
					"rule& Item",
					"express Exp",
					"token Pos", NULL, NULL);
	pSynSystem->RegHost("CreateIfRule", CreateIfRule);
}

static void CreateSequenceRule(CRuleStack &oStack)
{//"rule", "Item", "uint", "nMode", "token", "Pos"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	uint32 nMode = GetRuleArgUInt(oArgv);
	CToken* pPos = GetRuleArgToken(oArgv);
	CRule* pRule = new CSequenceRule(nMode);
	pRule->SetFile(*pPos);
	oRule = pRule;
}

static void AddRule(CRuleStack &oStack)
{//"rule", "Item", "rule", "SubRule"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CSequenceRule>& oRule = GetRuleArgObject< CMasterPointer<CSequenceRule> >(oArgv);
	CMasterPointer<CRule>& oSubRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	oRule->InsertRule(*oSubRule);
}

static void DefineSequenceRule(CSyntaxSystem* pSynSystem)
{//'(' 规则项 { 规则项 } ')'
	CRule* pRule = (
					   D("LeftParen", "Pos") >>
					   C(("CreateSequenceRule", V("Item"), (new CRuleConstExpress((uint32)0)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   T("RightParen")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "SequenceRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateSequenceRule", NULL, false,
					"rule& Item",
					"uint nMode",
					"token Pos", NULL, NULL);
	new CSyntaxRule(pSynSystem, "AddRule", NULL, false,
					"rule Item",
					"rule SubRule", NULL, NULL);
	pSynSystem->RegHost("CreateSequenceRule", CreateSequenceRule);
	pSynSystem->RegHost("AddRule", AddRule);
}

static void DefineTestRule(CSyntaxSystem* pSynSystem)
{//'[?' 规则项 { 规则项 } '?]'
	CRule* pRule = (
					   D("LeftBracketQuestion", "Pos") >>
					   C(("CreateSequenceRule", V("Item"), (new CRuleConstExpress((uint32)1)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   T("RightBracketQuestion")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "TestRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"token Pos", NULL);
}

static void DefineOptimismRule(CSyntaxSystem* pSynSystem)
{//'[' 规则项 { 规则项 } ']'
	CRule* pRule = (
					   D("LeftBracket", "Pos") >>
					   C(("CreateSequenceRule", V("Item"), (new CRuleConstExpress((uint32)2)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   T("RightBracket")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "OptimismRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"token Pos", NULL);
}

static void CreateOrRule(CRuleStack &oStack)
{//"rule", "Item", "token", "Pos"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CToken* pPos = GetRuleArgToken(oArgv);
	CRule* pRule = new COrRule;
	pRule->SetFile(*pPos);
	oRule = pRule;
}

static void DefineChoiceRule(CSyntaxSystem* pSynSystem)
{//'(?' 规则项 { 规则项 } '?)'
	CRule* pRule = (
					   D("LeftParenQuestion", "Pos") >>
					   C(("CreateOrRule", V("Item"), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   T("RightParenQuestion")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ChoiceRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateOrRule", NULL, false,
					"rule& Item",
					"token Pos", NULL, NULL);
	pSynSystem->RegHost("CreateOrRule", CreateOrRule);
}

static void CreateLoopRule(CRuleStack &oStack)
{//"rule", "Item", "rule", "SeqRule", "bool", "AnyTimes"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CMasterPointer<CRule>& oSeqRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	bool bAnyTimes = GetRuleArgBool(oArgv);
	CRule* pRule = new CLoopRule(*oSeqRule, bAnyTimes);
	pRule->SetFile(*oSeqRule);
	oRule = pRule;
}

static void DefineLoopRule(CSyntaxSystem* pSynSystem)
{//'{' 规则项 { 规则项 } '}'
	CRule* pRule = (
					   D("LeftBrace", "Pos") >>
					   C(("CreateSequenceRule", V("SeqRule"), (new CRuleConstExpress((uint32)0)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("SeqRule"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("SeqRule"), V("SubRule"), NULL))
					   ) >>
					   T("RightBrace") >>
					   C(("CreateLoopRule", V("Item"), V("SeqRule"), (new CRuleConstExpress(true)), V("Pos"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "LoopRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"rule SeqRule",
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateLoopRule", NULL, false,
					"rule& Item",
					"rule SeqRule",
					"bool bAny",
					"token Pos", NULL, NULL);
	pSynSystem->RegHost("CreateLoopRule", CreateLoopRule);
}

static void DefineOptionRule(CSyntaxSystem* pSynSystem)
{//'{?' 规则项 { 规则项 } '?}'
	CRule* pRule = (
					   D("LeftBraceQuestion", "Pos") >>
					   C(("CreateSequenceRule", V("SeqRule"), (new CRuleConstExpress((uint32)0)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("SeqRule"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("SeqRule"), V("SubRule"), NULL))
					   ) >>
					   T("RightBraceQuestion") >>
					   C(("CreateLoopRule", V("Item"), V("SeqRule"), (new CRuleConstExpress(false)), V("Pos"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "OptionRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"rule SeqRule",
					"token Pos", NULL);
}

static void DefineMustRule(CSyntaxSystem* pSynSystem)
{//'{+' 规则项 { 规则项 } '+}'
	CRule* pRule = (
					   D("LeftBracePlus", "Pos") >>
					   C(("CreateSequenceRule", V("Item"), (new CRuleConstExpress((uint32)0)), V("Pos"), NULL)) >>
					   (
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   *(
						   C(("RuleItem", V("SubRule"), NULL)) >>
						   C(("AddRule", V("Item"), V("SubRule"), NULL))
					   ) >>
					   T("RightBracePlus") >>
					   C(("CreateLoopRule", V("LoopRule"), V("Item"), (new CRuleConstExpress(true)), V("Pos"), NULL)) >>
					   C(("AddRule", V("Item"), V("LoopRule"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "MustRule", pRule, false,
					"rule& Item", NULL,
					"rule SubRule",
					"rule LoopRule",
					"token Pos", NULL);
}

static void CreateSemanticRule(CRuleStack &oStack)
{//"rule", "Item", "sentence", "SeqSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRule>& oRule = GetRuleArgObject< CMasterPointer<CRule> >(oArgv);
	CMasterPointer<CRuleSentence>& oSeqSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CRuleSentence* pSentence = oSeqSentence.Detach();
	CRule* pRule = new CSemanticRule(pSentence);
	pRule->SetFile(*pSentence);
	oRule = pRule;
}

static void DefineSemanticRule(CSyntaxSystem* pSynSystem)
{//'(.' {语句定义} '.)'
	CRule* pRule = (
					   D("LeftParenPeriod", "Pos") >>
					   C(("CreateComplexSentence", V("oSeqSentence"), (new CRuleConstExpress(true)), V("Pos"), NULL)) >>
					   *(
						   C(("SentenceRule", V("oSentence"), NULL)) >>
						   C(("AddSentence", V("oSeqSentence"), V("oSentence"), (new CRuleConstExpress(false)), NULL))
					   ) >>
					   T("RightParenPeriod") >>
					   C(("CreateSemanticRule", V("Item"), V("oSeqSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "SemanticRule", pRule, false,
					"rule& Item", NULL,
					"sentence oSeqSentence",
					"sentence oSentence",
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateSemanticRule", NULL, false,
					"rule& Item",
					"sentence oSeqSentence", NULL, NULL);
	pSynSystem->RegHost("CreateSemanticRule", CreateSemanticRule);
}

static void DefineSentenceRule(CSyntaxSystem* pSynSystem)
{//	表达式语句 | RETURN语句 | IF语句 | WHILE语句 | DO语句 | FOR语句 | SWITCH语句 |
 // BREAK语句 | CONTINUE语句 | ELSE语句 | CASE语句 | DEFAULT语句 | 复合语句
	CRule* pRule = (
					   C(("ExpressSentenceRule", V("oSentence"), NULL)) |
					   C(("ReturnSentenceRule", V("oSentence"), NULL)) |
					   C(("IfSentenceRule", V("oSentence"), NULL)) |
					   C(("WhileSentenceRule", V("oSentence"), NULL)) |
					   C(("DoSentenceRule", V("oSentence"), NULL)) |
					   C(("ForSentenceRule", V("oSentence"), NULL)) |
					   C(("SwitchSentenceRule", V("oSentence"), NULL)) |
					   C(("BreakSentenceRule", V("oSentence"), NULL)) |
					   C(("ContinueSentenceRule", V("oSentence"), NULL)) |
					   C(("ElseSentenceRule", V("oSentence"), NULL)) |
					   C(("CaseSentenceRule", V("oSentence"), NULL)) |
					   C(("DefaultSentenceRule", V("oSentence"), NULL)) |
					   C(("ComplexSentenceRule", V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "SentenceRule", pRule, false,
					"sentence& oSentence", NULL, NULL);
}

static void CreateExpressSentence(CRuleStack &oStack)
{//"express", "Exp", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CRuleExpress* pExp = oExp.Detach();
	if(pExp)//可能是空语句
	{
		CRuleSentence* pSentence = new CRuleExpSentence(*pExp);
		pSentence->SetFile(*pExp);
		oSentence = pSentence;
		delete pExp;
	}
}

static void DefineExpressSentenceRule(CSyntaxSystem* pSynSystem)
{
	CRule* pRule = (
					   -C(("ExpressRule", V("Exp"), NULL)) >>
					   T("Semicolon") >>
					   C(("CreateExpressSentence", V("Exp"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ExpressSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"express Exp", NULL);
	new CSyntaxRule(pSynSystem, "CreateExpressSentence", NULL, false,
					"express Exp",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateExpressSentence", CreateExpressSentence);
}

static void CreateReturnSentence(CRuleStack &oStack)
{//"token", "Pos", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CRuleSentence* pSentence = new CRuleReturnSentence;
	pSentence->SetFile(*pPos);
	oSentence = pSentence;
}

static void DefineReturnSentenceRule(CSyntaxSystem* pSynSystem)
{
	CRule* pRule = (
					   D("RETURN", "Pos") >>
					   T("Semicolon") >>
					   C(("CreateReturnSentence", V("Pos"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ReturnSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateReturnSentence", NULL, false,
					"token Pos",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateReturnSentence", CreateReturnSentence);
}

static void CheckSentenceDepend(CRuleSentence* &pSentence, CRuleStack &oStack)
{
	uint32 nDepend = pSentence->GetDepend();
	if(nDepend)
	{
		CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
		CLexicalModule* pLexModule = pModule->GetLexicalModule();
		switch(nDepend)//1=Else,2=case,3=default
		{
		case 1:
			pLexModule->OnError(*pSentence, "Invalid 'else' sentence");
			break;
		case 2:
			pLexModule->OnError(*pSentence, "Invalid 'case' sentence");
			break;
		case 3:
			pLexModule->OnError(*pSentence, "Invalid 'default' sentence");
			break;
		}
		delete pSentence;
		pSentence = NULL;
	}
}

static void CreateIfSentence(CRuleStack &oStack)
{//token Pos,express Exp,sentence Then,sentence Else,sentence& Ret
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oThenSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMasterPointer<CRuleSentence>& oElseSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pThen = oThenSentence.Detach();
	CRuleSentence* pElse = oElseSentence.Detach();

	if(pThen)
		CheckSentenceDepend(pThen, oStack);

	CRuleSentence* pRet = new CRuleCondSentence(*oExp, pThen, pElse);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineIfSentenceRule(CSyntaxSystem* pSynSystem)
{//IF '(' 表达式 ')' 语句定义 [ ELSE语句 ]
	CRule* pRule = (
					   D("IF", "Pos") >>
					   T("LeftParen") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("RightParen") >>
					   C(("SentenceRule", V("ThenSentence"), NULL)) >>
					   -C(("ElseSentenceRule", V("ElseSentence"), NULL)) >>
					   C(("CreateIfSentence", V("Pos"), V("Exp"), V("ThenSentence"), V("ElseSentence"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "IfSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express Exp",
					"sentence ThenSentence",
					"sentence ElseSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateIfSentence", NULL, false,
					"token Pos",
					"express Exp",
					"sentence ThenSentence",
					"sentence ElseSentence",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateIfSentence", CreateIfSentence);
}

static void CreateWhileSentence(CRuleStack &oStack)
{//token Pos,express Exp,sentence While, bool DoWhile, sentence& Ret
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oWhileSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	bool bDoWhile = GetRuleArgBool(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pWhile = oWhileSentence.Detach();

	if(pWhile)
		CheckSentenceDepend(pWhile, oStack);

	CRuleSentence* pRet = new CRuleWhileSentence(*oExp, pWhile, bDoWhile);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineWhileSentenceRule(CSyntaxSystem* pSynSystem)
{//WHILE '(' 表达式 ')' 语句定义
	CRule* pRule = (
					   D("WHILE", "Pos") >>
					   T("LeftParen") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("RightParen") >>
					   C(("SentenceRule", V("WhileSentence"), NULL)) >>
					   C(("CreateWhileSentence", V("Pos"), V("Exp"), V("WhileSentence"), (new CRuleConstExpress(false)), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "WhileSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express Exp",
					"sentence WhileSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateWhileSentence", NULL, false,
					"token Pos",
					"express Exp",
					"sentence WhileSentence",
					"bool DoWhile",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateWhileSentence", CreateWhileSentence);
}

static void DefineDoSentenceRule(CSyntaxSystem* pSynSystem)
{//DO 复合语句 WHILE '(' 表达式 ')' ';'
	CRule* pRule = (
					   D("DO", "Pos") >>
					   C(("ComplexSentenceRule", V("WhileSentence"), NULL)) >>
					   T("While") >>
					   T("LeftParen") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("RightParen") >>
					   T("Semicolon") >>
					   C(("CreateWhileSentence", V("Pos"), V("Exp"), V("WhileSentence"), (new CRuleConstExpress(true)), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "DoSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express Exp",
					"sentence WhileSentence", NULL);
}

static void CreateForSentence(CRuleStack &oStack)
{//token Pos;
 //express InitExp, CondExp, LoopExp;
 //sentence ForSentence, &oSentence;
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oInitExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oCondExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oLoopExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oForSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleExpress* pInitExp = oInitExp.Detach();
	CRuleExpress* pCondExp = oCondExp.Detach();
	CRuleExpress* pLoopExp = oLoopExp.Detach();
	CRuleSentence* pForSentence = oForSentence.Detach();

	if(pForSentence)
		CheckSentenceDepend(pForSentence, oStack);

	CRuleSentence* pRet = new CRuleForSentence(pInitExp, pCondExp, pLoopExp, pForSentence);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineForSentenceRule(CSyntaxSystem* pSynSystem)
{//FOR '(' [表达式] ';' [表达式] ';' [表达式] ')' 语句定义
	CRule* pRule = (
					   D("FOR", "Pos") >>
					   T("LeftParen") >>
					   -C(("ExpressRule", V("InitExp"), NULL)) >>
					   T("Semicolon") >>
					   -C(("ExpressRule", V("CondExp"), NULL)) >>
					   T("Semicolon") >>
					   -C(("ExpressRule", V("LoopExp"), NULL)) >>
					   T("RightParen") >>
					   C(("SentenceRule", V("ForSentence"), NULL)) >>
					   C(("CreateForSentence", V("Pos"), V("InitExp"), V("CondExp"), V("LoopExp"), V("ForSentence"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ForSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express InitExp",
					"express CondExp",
					"express LoopExp",
					"sentence ForSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateForSentence", NULL, false,
					"token Pos",
					"express InitExp",
					"express CondExp",
					"express LoopExp",
					"sentence ForSentence",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateForSentence", CreateForSentence);
}

static void CreateSwitchSentence(CRuleStack &oStack)
{//"token", "Pos", "express", "SwitchExp", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oSwitchExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CRuleSentence* pRet = new CRuleSwitchSentence(*oSwitchExp);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineSwitchSentenceRule(CSyntaxSystem* pSynSystem)
{//SWITCH '(' 表达式 ')' 复合语句
	CRule* pRule = (
					   D("SWITCH", "Pos") >>
					   T("LeftParen") >>
					   C(("ExpressRule", V("SwitchExp"), NULL)) >>
					   T("RightParen") >>
					   T("LeftBrace") >>
					   C(("CreateSwitchSentence", V("Pos"), V("SwitchExp"), V("oSentence"), NULL)) >>
					   (
						   C(("DefaultSentenceRule", V("CaseSentence"), NULL)) |
						   C(("CaseSentenceRule", V("CaseSentence"), NULL))
					   ) >>
					   C(("AddSentence", V("oSentence"), V("CaseSentence"), (new CRuleConstExpress(true)), NULL)) >>
					   *(
						   C(("SentenceRule", V("CaseSentence"), NULL)) >>
						   C(("AddSentence", V("oSentence"), V("CaseSentence"), (new CRuleConstExpress(true)), NULL))
					   ) >>
					   T("RightBrace")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "SwitchSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express SwitchExp",
					"sentence CaseSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateSwitchSentence", NULL, false,
					"token Pos",
					"express SwitchExp",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateSwitchSentence", CreateSwitchSentence);
}

static void CreateElseSentence(CRuleStack &oStack)
{//"token", "Pos", "sentence", "ElseSentence", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleSentence>& oElseSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pElse = oElseSentence.Detach();
	if(pElse)
		CheckSentenceDepend(pElse, oStack);

	CRuleSentence* pRet = new CRuleElseSentence(pElse);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineElseSentenceRule(CSyntaxSystem* pSynSystem)
{//ELSE 语句定义
	CRule* pRule = (
					   D("ELSE", "Pos") >>
					   C(("SentenceRule", V("ElseSentence"), NULL)) >>
					   C(("CreateElseSentence", V("Pos"), V("ElseSentence"), V("oSentence"), NULL))
				   ).Clone();

	new CSyntaxRule(pSynSystem, "ElseSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"sentence ElseSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateElseSentence", NULL, false,
					"token Pos",
					"sentence ElseSentence",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateElseSentence", CreateElseSentence);
}

static void CreateBreakSentence(CRuleStack &oStack)
{//"token", "Pos", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pRet = new CRuleBreakSentence;
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineBreakSentenceRule(CSyntaxSystem* pSynSystem)
{//BREAK ';'
	CRule* pRule = (
					   D("BREAK", "Pos") >> T("Semicolon") >>
					   C(("CreateBreakSentence", V("Pos"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "BreakSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateBreakSentence", NULL, false,
					"token Pos",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateBreakSentence", CreateBreakSentence);
}

static void CreateContinueSentence(CRuleStack &oStack)
{//"token", "Pos", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pRet = new CRuleContinueSentence;
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineContinueSentenceRule(CSyntaxSystem* pSynSystem)
{//CONTINUE ';'
	CRule* pRule = (
					   D("CONTINUE", "Pos") >>
					   T("Semicolon") >>
					   C(("CreateContinueSentence", V("Pos"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ContinueSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateContinueSentence", NULL, false,
					"token Pos",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateContinueSentence", CreateContinueSentence);
}

static void CreateCaseSentence(CRuleStack &oStack)
{//"token", "Pos", "express", "Exp", "sentence", "oSentence",
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	bool bError = false;
	uint64 nVal = 0;
	CRuleConstExpress* pConstExp = dynamic_cast<CRuleConstExpress*>((CRuleExpress*)oExp);
	if(pConstExp == NULL)
	{
		bError = true;
		pLexModule->OnError(*oExp, "must be integer constant");
	}
	else
	{
		pConstExp->Check(*oStack.pSystem, pLexModule);
		void* pVal = pConstExp->GetExpressValue(&oStack);
		uint32 nType = pConstExp->GetExpressType(oStack.pSystem)->TypeCode();
		if(nType < ARF_CHAR || nType > ARF_UINT64)
		{
			bError = true;
			pLexModule->OnError(*oExp, "only can return integer constant");
		}
		else switch(nType)
			{
			case ARF_CHAR:
				nVal = *(char*)pVal;
				break;
			case ARF_INT8:
				nVal = *(int8*)pVal;
				break;
			case ARF_UINT8:
				nVal = *(uint8*)pVal;
				break;
			case ARF_INT16:
				nVal = *(int16*)pVal;
				break;
			case ARF_UINT16:
				nVal = *(uint16*)pVal;
				break;
			case ARF_INT32:
			case ARF_UINT32:
				nVal = *(uint32*)pVal;
				break;
			case ARF_INT64:
			case ARF_UINT64:
				nVal = *(uint64*)pVal;
				break;
			}
	}
	CRuleCaseSentence* pRet = new CRuleCaseSentence(nVal, bError);
	pRet->SetFile(*pPos);
	oSentence = pRet;
}

static void DefineCaseSentenceRule(CSyntaxSystem* pSynSystem)
{//CASE (整型常量 | 字符常量) ':'
	CRule* pRule = (
					   D("CASE", "Pos") >>
					   C(("ExpressRule", V("Exp"), NULL)) >>
					   T("Colon") >>
					   C(("CreateCaseSentence", V("Pos"), V("Exp"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "CaseSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"express Exp", NULL);
	new CSyntaxRule(pSynSystem, "CreateCaseSentence", NULL, false,
					"token Pos",
					"express Exp",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateCaseSentence", CreateCaseSentence);
}

static void CreateDefaultSentence(CRuleStack &oStack)
{//"token", "Pos", "sentence", "oSentence"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);

	CRuleSentence* pRet = new CRuleDefaultSentence();
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void DefineDefaultSentenceRule(CSyntaxSystem* pSynSystem)
{//DEFAULT ':'
	CRule* pRule = (
					   D("DEFAULT", "Pos") >>
					   T("Colon") >>
					   C(("CreateDefaultSentence", V("Pos"), V("oSentence"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "DefaultSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos", NULL);
	new CSyntaxRule(pSynSystem, "CreateDefaultSentence", NULL, false,
					"token Pos",
					"sentence& oSentence", NULL, NULL);
	pSynSystem->RegHost("CreateDefaultSentence", CreateDefaultSentence);
}

static void CreateComplexSentence(CRuleStack &oStack)
{//"sentence", "oSentence", "bool", "bTop", "token", "Pos"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	bool bTop = GetRuleArgBool(oArgv);
	CToken* pPos = GetRuleArgToken(oArgv);
	CRuleSentence* pRet = new CRuleComplexSentence(bTop);
	oSentence = pRet;
	pRet->SetFile(*pPos);
}

static void AddSentence(CRuleStack &oStack)
{//"sentence", "oSentence", "sentence", "SubSentence", "bool", "bSwitch"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleSentence>& oSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	CMasterPointer<CRuleSentence>& oSubSentence = GetRuleArgObject< CMasterPointer<CRuleSentence> >(oArgv);
	bool bSwitch = GetRuleArgBool(oArgv);
	CRuleSentence* pSub = oSubSentence.Detach();
	if(pSub)
	{
		uint64 nSwitchVal;
		CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
		CLexicalModule* pLexModule = pModule->GetLexicalModule();
		uint32 nDepend = pSub->GetDepend();
		if(nDepend)
		{
			switch(nDepend)
			{
			case 1:
				pLexModule->OnError(*pSub, "'else' sentence must be used in the if sentence");
				break;
			case 2:
				if(!bSwitch)
					pLexModule->OnError(*pSub, "'case' sentence must be used in the switch sentence");
				else if(!((CRuleCaseSentence*)pSub)->GetSwitchValue(nSwitchVal))
				{
					CRuleSwitchSentence* pRet = (CRuleSwitchSentence*)(CRuleSentence*)oSentence;
					if(!pRet->SetCaseLable(nSwitchVal))
						pLexModule->OnError(*pSub, "repeat case sentence");
				}
				break;
			case 3:
				if(!bSwitch)
					pLexModule->OnError(*pSub, "'default' sentence must be used in the switch sentence");
				else
				{
					CRuleSwitchSentence* pRet = (CRuleSwitchSentence*)(CRuleSentence*)oSentence;
					if(!pRet->SetDefaultLable())
						pLexModule->OnError(*pSub, "repeat default sentence");
				}
				break;
			}
			delete pSub;
		}
		else
		{
			CRuleComplexSentence* pSeqSentence = (CRuleComplexSentence*)(CRuleSentence*)oSentence;
			pSeqSentence->AddSentence(pSub);
		}
	}
}

static void DefineComplexSentenceRule(CSyntaxSystem* pSynSystem)
{//'{' {语句定义} '}'
	CRule* pRule = (
					   D("LeftBrace", "Pos") >>
					   C(("CreateComplexSentence", V("oSentence"), (new CRuleConstExpress(false)), V("Pos"), NULL)) >>
					   *(
						   C(("SentenceRule", V("SubSentence"), NULL)) >>
						   C(("AddSentence", V("oSentence"), V("SubSentence"), (new CRuleConstExpress(false)), NULL))
					   ) >>
					   T("RightBrace")
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ComplexSentenceRule", pRule, false,
					"sentence& oSentence", NULL,
					"token Pos",
					"sentence SubSentence", NULL);
	new CSyntaxRule(pSynSystem, "CreateComplexSentence", NULL, false,
					"sentence& oSentence",
					"bool bTop",
					"token Pos", NULL, NULL);
	new CSyntaxRule(pSynSystem, "AddSentence", NULL, false,
					"sentence oSentence",
					"sentence SubSentence",
					"bool bSwitch", NULL, NULL);
	pSynSystem->RegHost("CreateComplexSentence", CreateComplexSentence);
	pSynSystem->RegHost("AddSentence", AddSentence);
}

static void CreateBinaryExpress(CRuleStack &oStack)
{//"express", "Exp", "token", "Op", "express", "SubExp"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CToken* pOp = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oSubExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	uint32 nOp;
	const char* sOp = pOp->GetKind();
	if(!CString::StringCompare(sOp, "Equal", false))
		nOp = ARF_ASIGN_OP;
	else if(!CString::StringCompare(sOp, "MulEqual", false))
		nOp = ARF_MULEQ_OP;
	else if(!CString::StringCompare(sOp, "DivEqual", false))
		nOp = ARF_DIVEQ_OP;
	else if(!CString::StringCompare(sOp, "ModEqual", false))
		nOp = ARF_MODEQ_OP;
	else if(!CString::StringCompare(sOp, "AddEqual", false))
		nOp = ARF_ADDEQ_OP;
	else if(!CString::StringCompare(sOp, "SubEqual", false))
		nOp = ARF_SUBEQ_OP;
	else if(!CString::StringCompare(sOp, "LeftShiftEqual", false))
		nOp = ARF_LSHEQ_OP;
	else if(!CString::StringCompare(sOp, "RightShiftEqual", false))
		nOp = ARF_RSHEQ_OP;
	else if(!CString::StringCompare(sOp, "AndEqual", false))
		nOp = ARF_ANDEQ_OP;
	else if(!CString::StringCompare(sOp, "OrEqual", false))
		nOp = ARF_OREQ_OP;
	else if(!CString::StringCompare(sOp, "XorEqual", false))
		nOp = ARF_XOREQ_OP;
	else if(!CString::StringCompare(sOp, "Comma", false))
		nOp = ARF_COMMA_OP;
	else if(!CString::StringCompare(sOp, "LogicOr", false))
		nOp = ARF_LOR_OP;
	else if(!CString::StringCompare(sOp, "LogicAnd", false))
		nOp = ARF_LAND_OP;
	else if(!CString::StringCompare(sOp, "Bar", false))
		nOp = ARF_OR_OP;
	else if(!CString::StringCompare(sOp, "Caret", false))
		nOp = ARF_XOR_OP;
	else if(!CString::StringCompare(sOp, "Ampersand", false))
		nOp = ARF_AND_OP;
	else if(!CString::StringCompare(sOp, "EqualEqual", false))
		nOp = ARF_EQ_OP;
	else if(!CString::StringCompare(sOp, "NotEqual", false))
		nOp = ARF_NE_OP;
	else if(!CString::StringCompare(sOp, "Less", false))
		nOp = ARF_LT_OP;
	else if(!CString::StringCompare(sOp, "Greater", false))
		nOp = ARF_GT_OP;
	else if(!CString::StringCompare(sOp, "GreaterEqual", false))
		nOp = ARF_GE_OP;
	else if(!CString::StringCompare(sOp, "LessEqual", false))
		nOp = ARF_LE_OP;
	else if(!CString::StringCompare(sOp, "LeftShift", false))
		nOp = ARF_LSH_OP;
	else if(!CString::StringCompare(sOp, "RightShift", false))
		nOp = ARF_RSH_OP;
	else if(!CString::StringCompare(sOp, "Plus", false))
		nOp = ARF_ADD_OP;
	else if(!CString::StringCompare(sOp, "Minus", false))
		nOp = ARF_SUB_OP;
	else if(!CString::StringCompare(sOp, "Star", false))
		nOp = ARF_MUL_OP;
	else if(!CString::StringCompare(sOp, "Slash", false))
		nOp = ARF_DIV_OP;
	else if(!CString::StringCompare(sOp, "Percent", false))
		nOp = ARF_MOD_OP;
	CRuleExpress* pExp = new CRuleBinaryExpress(*oExp, nOp, *oSubExp);
	pExp->SetFile(*oExp);
	oExp = pExp;
}

static void DefineExpressRule(CSyntaxSystem* pSynSystem)
{//赋值表达式 {',' 赋值表达式}
	CRule* pRule = (
					   C(("AssignExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("Comma", "Op") >>
						   C(("AssignExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
	new CSyntaxRule(pSynSystem, "CreateBinaryExpress", NULL, false,
					"express& Exp",
					"token nOp",
					"express SubExp", NULL, NULL);
	pSynSystem->RegHost("CreateBinaryExpress", CreateBinaryExpress);
}

static void DefineAssignExpressRule(CSyntaxSystem* pSynSystem)
{//逻辑或表达式 [ ('=' | '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '&=' | '^=' | '|=') 赋值表达式]
	CRule* pRule = (
					   C(("LogicOrExpressRule", V("Exp"), NULL)) >>
					   -(
						   (
							   D("Equal", "Op") |
							   D("MulEqual", "Op") |
							   D("DivEqual", "Op") |
							   D("ModEqual", "Op") |
							   D("AddEqual", "Op") |
							   D("SubEqual", "Op") |
							   D("LeftShiftEqual", "Op") |
							   D("RightShiftEqual", "Op") |
							   D("AndEqual", "Op") |
							   D("OrEqual", "Op") |
							   D("XorEqual", "Op")
						   ) >>
						   C(("AssignExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "AssignExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineLogicOrExpressRule(CSyntaxSystem* pSynSystem)
{//逻辑与表达式 {'||' 逻辑与表达式}
	CRule* pRule = (
					   C(("LogicAndExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("LogicOr", "Op") >>
						   C(("LogicAndExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "LogicOrExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineLogicAndExpressRule(CSyntaxSystem* pSynSystem)
{//位或表达式 {'&&' 位或表达式}
	CRule* pRule = (
					   C(("BitOrExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("LogicAnd", "Op") >>
						   C(("BitOrExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "LogicAndExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineBitOrExpressRule(CSyntaxSystem* pSynSystem)
{//位异或表达式 {'|' 位异或表达式}
	CRule* pRule = (
					   C(("BitXorExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("Bar", "Op") >>
						   C(("BitXorExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "BitOrExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineBitXorExpressRule(CSyntaxSystem* pSynSystem)
{//位与表达式 {'^' 位与表达式}
	CRule* pRule = (
					   C(("BitAndExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("Caret", "Op") >>
						   C(("BitAndExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "BitXorExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineBitAndExpressRule(CSyntaxSystem* pSynSystem)
{//相等表达式 {'&' 相等表达式}
	CRule* pRule = (
					   C(("EqualExpressRule", V("Exp"), NULL)) >>
					   *(
						   D("Ampersand", "Op") >>
						   C(("EqualExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "BitAndExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineEqualExpressRule(CSyntaxSystem* pSynSystem)
{//比较表达式 { ('==' | '!=') 比较表达式}
	CRule* pRule = (
					   C(("RelationExpressRule", V("Exp"), NULL)) >>
					   *(
						   (
							   D("EqualEqual", "Op") |
							   D("NotEqual", "Op")
						   ) >>
						   C(("RelationExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "EqualExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineRelationExpressRule(CSyntaxSystem* pSynSystem)
{//移位表达式 { ('<' | '>' | '<=' | '>=') 移位表达式}
	CRule* pRule = (
					   C(("ShiftExpressRule", V("Exp"), NULL)) >>
					   *(
						   (
							   D("Less", "Op") |
							   D("Greater", "Op") |
							   D("GreaterEqual", "Op") |
							   D("LessEqual", "Op")
						   ) >>
						   C(("ShiftExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "RelationExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineShiftExpressRule(CSyntaxSystem* pSynSystem)
{//加法表达式 { ('<<' | '>>') 加法表达式}
	CRule* pRule = (
					   C(("AddExpressRule", V("Exp"), NULL)) >>
					   *(
						   (
							   D("LeftShift", "Op") |
							   D("RightShift", "Op")
						   ) >>
						   C(("AddExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "ShiftExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineAddExpressRule(CSyntaxSystem* pSynSystem)
{//乘法表达式 { ('+' | '-') 乘法表达式}
	CRule* pRule = (
					   C(("MulExpressRule", V("Exp"), NULL)) >>
					   *(
						   (
							   D("Plus", "Op") |
							   D("Minus", "Op")
						   ) >>
						   C(("MulExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "AddExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void DefineMulExpressRule(CSyntaxSystem* pSynSystem)
{//转换表达式 { ('*' | '/' | '%') 转换表达式}
	CRule* pRule = (
					   C(("CastExpressRule", V("Exp"), NULL)) >>
					   *(
						   (
							   D("Star", "Op") |
							   D("Slash", "Op") |
							   D("Percent", "Op")
						   ) >>
						   C(("CastExpressRule", V("SubExp"), NULL)) >>
						   C(("CreateBinaryExpress", V("Exp"), V("Op"), V("SubExp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "MulExpressRule", pRule, false,
					"express& Exp", NULL,
					"express SubExp",
					"token Op", NULL);
}

static void GetType1(CRuleStack &oStack)
{//"type", "TypeInfo"
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxSystem* pSynSystem = pModule->GetTargetSyntaxSystem();
	CSyntaxModule* pSyntaxModule = pModule->GetSyntaxModule();
	InitRuleArgv(oArgv, oStack);
	CRuleType* &pType = GetRuleArgObject<CRuleType*>(oArgv);
	pType = NULL;
	CToken* pNextToken = pSyntaxModule->PullToken(false);
	if(pNextToken && !CString::StringCompare(pNextToken->GetKind(), "("))
	{
		pNextToken = pSyntaxModule->PeekToken();
		if(pNextToken && !CString::StringCompare(pNextToken->GetKind(), "Identifier", false))
			pType = pSynSystem->GetType(pNextToken->GetToken());
	}
}

static void CreateCastExpress(CRuleStack &oStack)
{//"token", "Pos", "type", "TypeInfo", "express", "Exp"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CRuleType* pLeftType = GetRuleArgType(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleExpress* pExp = oExp;
	CRuleType* pRightType = pExp->GetExpressType(oStack.pSystem);
	if(pRightType != pLeftType)
	{
		CRuleTmpVar* pVar = oStack.pProc->GetVariables()->AllocVar(pLeftType);
		CRuleVariableExpress oVarExp(pVar->GetName());
		pExp = new CRuleBinaryExpress(oVarExp, ARF_ASIGN_OP, *pExp);
		pExp->SetFile(*pPos);
		oExp = pExp;
	}
}

static void DefineCastExpressRule(CSyntaxSystem* pSynSystem)
{//IF(GetType1()) '(' 类型名 ')' 转换表达式 | 一元表达式
	CRule* pRule = (
					   C(("GetType1", V("TypeInfo"), NULL)) >>
					   (
						   (
							   IF("TypeInfo") >>
							   T("LeftParen") >>
							   D("Identifier", "Pos") >>
							   T("RightParen") >>
							   C(("CastExpressRule", V("Exp"), NULL)) >>
							   C(("CreateCastExpress", V("Pos"), V("TypeInfo"), V("Exp"), NULL))
						   ) |
						   C(("UnaryExpressRule", V("Exp"), NULL))
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "CastExpressRule", pRule, false,
					"express& Exp", NULL,
					"token Pos",
					"type TypeInfo", NULL);
	new CSyntaxRule(pSynSystem, "CreateCastExpress", NULL, false,
					"token Pos",
					"type TypeInfo",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "GetType1", NULL, false,
					"type& TypeInfo", NULL, NULL);
	pSynSystem->RegHost("CreateCastExpress", CreateCastExpress);
	pSynSystem->RegHost("GetType1", GetType1);
}

static void CreateUnaryExpress(CRuleStack &oStack)
{//"token Op", "express Exp", "int nCount"
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pOp = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	int32 nCount = GetRuleArgInt(oArgv);

	uint32 nOp;
	if(nCount)
	{
		if(nCount > 0)
			nOp = ARF_SINC_OP;
		else
		{
			nOp = ARF_SDEC_OP;
			nCount = -nCount;
		}
		for(int32 i=0; i<nCount; ++i)
		{
			CRuleExpress * pExp = new CRuleUnaryExpress(nOp, *oExp);
			pExp->SetFile(*oExp);
			oExp = pExp;
		}
		return;
	}
	if(pOp == NULL)
		return;
	const char* sOp = pOp->GetKind();
	if(!CString::StringCompare(sOp, "PlusPlus", false))
		nOp = ARF_INC_OP;
	else if(!CString::StringCompare(sOp, "MinusMinus", false))
		nOp = ARF_DEC_OP;
	else if(!CString::StringCompare(sOp, "Plus", false))
		nOp = ARF_POST_OP;
	else if(!CString::StringCompare(sOp, "Minus", false))
		nOp = ARF_NEG_OP;
	else if(!CString::StringCompare(sOp, "Tilde", false))
		nOp = ARF_NON_OP;
	else if(!CString::StringCompare(sOp, "Exclam", false))
		nOp = ARF_LNON_OP;
	CRuleExpress * pExp = new CRuleUnaryExpress(nOp, *oExp);
	if(nOp == ARF_INC_OP || nOp == ARF_DEC_OP)
		pExp->SetFile(*oExp);
	else
		pExp->SetFile(*pOp);
	oExp = pExp;
}

static void IncDec(CRuleStack &oStack)
{//int&, bool
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgInt(oArgv);
	bool bMode = GetRuleArgBool(oArgv);
	if(bMode)
		++nRet;
	else
		--nRet;
}

static void DefineUnaryExpressRule(CSyntaxSystem* pSynSystem)
{//{'++' | '--'} ( 基本表达式 {'++' | '--'} | ( '+' | '-' | '~' | '!') 转换表达式)
	CRule* pRule = (
					   *(
						   ( T("PlusPlus") >> C(("IncDec", V("nCount"), (new CRuleConstExpress(true)), NULL)) ) |
						   ( T("MinusMinus") >> C(("IncDec", V("nCount"), (new CRuleConstExpress(false)), NULL)) )
					   ) >>
					   (
						   (
							   C(("PrimaryExpressRule", V("Exp"), NULL)) >>
							   *(
								   (
									   D("PlusPlus", "Op1") |
									   D("MinusMinus", "Op1")
								   ) >>
								   C(("CreateUnaryExpress", V("Op1"), V("Exp"), (new CRuleConstExpress((int32)0)), NULL))
							   )
						   ) |
						   (
							   (
								   D("Plus", "Op2") |
								   D("Minus", "Op2") |
								   D("Tilde", "Op2") |
								   D("Exclam", "Op2")
							   ) >>
							   C(("CastExpressRule", V("Exp"), NULL)) >>
							   C(("CreateUnaryExpress", V("Op2"), V("Exp"), (new CRuleConstExpress((int32)0)), NULL))
						   )
					   ) >>
					   C(("CreateUnaryExpress", V("Op0"), V("Exp"), V("nCount"), NULL))
				   ).Clone();
	new CSyntaxRule(pSynSystem, "UnaryExpressRule", pRule, false,
					"express& Exp", NULL,
					"token Op0", "token Op1", "token Op2", "int nCount", NULL);
	new CSyntaxRule(pSynSystem, "CreateUnaryExpress", NULL, false,
					"token Op",
					"express& Exp",
					"int nCount", NULL, NULL);
	new CSyntaxRule(pSynSystem, "IncDec", NULL, false,
					"int& nCount",
					"bool bInc", NULL, NULL);
	pSynSystem->RegHost("CreateUnaryExpress", CreateUnaryExpress);
	pSynSystem->RegHost("IncDec", IncDec);
}

static void CreateIntConst(CRuleStack &oStack)
{//token Const; express Exp;
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pConst = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	const char* sConst = pConst->GetToken();

	enum
	{
		ARF_SUFFIX_NON = 0,
		ARF_SUFFIX_U = 1,
		ARF_SUFFIX_L = 2,
		ARF_SUFFIX_I = 4
	};
	enum
	{
		ARF_INT_CONST_0 = 0,
		ARF_INT_CONST_8 = 8,
		ARF_INT_CONST_16 = 16,
		ARF_INT_CONST_32 = 32,
		ARF_INT_CONST_64 = 64
	};
	enum
	{
		ARF_INT_CONST_SIGNED = 1,
		ARF_INT_CONST_UNSIGNED = 2
	};

	int32 nSys = 10;
	uint64 ul = 0;
	char* val = (char*)sConst;
	uchar* s = (uchar*)val;
	uint32 nSuffixType = ARF_SUFFIX_NON;
	uint32 nConstBits = ARF_INT_CONST_0;
	uint32 nSigned = ARF_INT_CONST_0;
	uint64 c, d;
	uchar a1;
	uint16 a2;
	uint32 a3;
	if(s[0] == '0')
	{
		++s;
		nSys = 8;
		if(s[0] == 'x' || s[0] == 'X')
		{
			nSys = 16;
			++s;
		}
	}
	while(s[0])
	{
		if(s[0] == 'u' || s[0] =='U')
		{
			nSuffixType = ARF_SUFFIX_U;
			nSigned = ARF_INT_CONST_UNSIGNED;
			++s;
			break;
		}
		if(s[0] == 'l' || s[0] =='L')
		{
			nSuffixType = ARF_SUFFIX_L;
			nSigned = ARF_INT_CONST_SIGNED;
			++s;
			break;
		}
		if(s[0] == 'i' || s[0] =='I')
		{
			nSuffixType = ARF_SUFFIX_I;
			nSigned = ARF_INT_CONST_SIGNED;
			++s;
			break;
		}
		d = ul * nSys;
		if(d < ul)
			pLexModule->OnError(*pConst, "integer constant overflow");
		ul = d;
		c = s[0];
		if(c >= '0' && c <= '9')
			c -= '0';
		else if(c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if(c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		d += c;
		if(d < ul)
			pLexModule->OnError(*pConst, "integer constant overflow");
		ul = d;
		++s;
	}
	if(nSuffixType == ARF_SUFFIX_U)
	{
		nConstBits = ARF_INT_CONST_32;
		if(s[0] == 'l' || s[0] == 'L')
			nConstBits = ARF_INT_CONST_64;
		else
		{
			if(!CString::StringCompare((const char*)s, "8"))
				nConstBits = ARF_INT_CONST_8;
			else if(!CString::StringCompare((const char*)s, "16"))
				nConstBits = ARF_INT_CONST_16;
			else if(!CString::StringCompare((const char*)s, "32"))
				nConstBits = ARF_INT_CONST_32;
			else if(!CString::StringCompare((const char*)s, "64"))
				nConstBits = ARF_INT_CONST_64;
		}
	}
	else if(nSuffixType == ARF_SUFFIX_L)
	{
		nConstBits = ARF_INT_CONST_64;
		if(s[0] == 'u' || s[0] == 'U')
			nSigned = ARF_INT_CONST_UNSIGNED;
	}
	else if(nSuffixType == ARF_SUFFIX_I)
	{
		if(!CString::StringCompare((const char*)s, "8"))
			nConstBits = ARF_INT_CONST_8;
		else if(!CString::StringCompare((const char*)s, "16"))
			nConstBits = ARF_INT_CONST_16;
		else if(!CString::StringCompare((const char*)s, "32"))
			nConstBits = ARF_INT_CONST_32;
		else if(!CString::StringCompare((const char*)s, "64"))
			nConstBits = ARF_INT_CONST_64;
	}
	else
	{
		nSigned = ARF_INT_CONST_SIGNED;
		if(ul >> 32)
		{
			nConstBits = ARF_INT_CONST_64;
			if(0 > (int64)ul)
				nSigned = ARF_INT_CONST_UNSIGNED;
		}
		else
		{
			nConstBits = ARF_INT_CONST_32;
			if(0 > (int32)ul)
				nSigned = ARF_INT_CONST_UNSIGNED;
		}
	}
	CRuleConstExpress* pExp = NULL;
	switch(nConstBits)
	{
	case ARF_INT_CONST_8:
		if( ((nSigned == ARF_INT_CONST_SIGNED) && (ul > 0x7F)) || (ul > 0xFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		a1 = (uchar)ul;
		if(nSigned == ARF_INT_CONST_SIGNED)
			pExp = new CRuleConstExpress((int8)a1);
		else
			pExp = new CRuleConstExpress((uint8)a1);
		break;
	case ARF_INT_CONST_16:
		if( ((nSigned == ARF_INT_CONST_SIGNED) && (ul > 0x7FFF)) || (ul > 0xFFFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		a2 = (uint16)ul;
		if(nSigned == ARF_INT_CONST_SIGNED)
			pExp = new CRuleConstExpress((int16)a2);
		else
			pExp = new CRuleConstExpress((uint16)a2);
		break;
	case ARF_INT_CONST_32:
		if( ((nSigned == ARF_INT_CONST_SIGNED) && (ul > 0x7FFFFFFF)) || (ul > 0xFFFFFFFF))
			pLexModule->OnError(*pConst, "integer constant overflow");
		a3 = (uint32)ul;
		if(nSigned == ARF_INT_CONST_SIGNED)
			pExp = new CRuleConstExpress((int32)a3);
		else
			pExp = new CRuleConstExpress((uint32)a3);
		break;
	case ARF_INT_CONST_64:
		if( (nSigned == ARF_INT_CONST_SIGNED) && (ul > FOCP_UINT64_CONST(0x7FFFFFFFFFFFFFFF)))
			pLexModule->OnError(*pConst, "integer constant overflow");
		if(nSigned == ARF_INT_CONST_SIGNED)
			pExp = new CRuleConstExpress((int64)ul);
		else
			pExp = new CRuleConstExpress((uint64)ul);
		break;
	}
	oExp = pExp;
}

static void CreateStringConst(CRuleStack &oStack)
{//token Const; express Exp;
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pConst = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	const char* sConst = pConst->GetToken();

	uchar c, d;
	CVector<char> oDst;
	char* val = (char*)sConst, *sSrc;

	sSrc = val+1;
	while(sSrc[0])
	{
		if(sSrc[0] == '\"')
			break;
		if(sSrc[0] != '\\')
			oDst.Insert((uint32)(-1), sSrc[0]);
		else
		{
			++sSrc;
			switch(sSrc[0])
			{
			case 'a':
				oDst.Insert((uint32)(-1), '\a');
				break;
			case 'b':
				oDst.Insert((uint32)(-1), '\b');
				break;
			case 'f':
				oDst.Insert((uint32)(-1), '\f');
				break;
			case 'n':
				oDst.Insert((uint32)(-1), '\n');
				break;
			case 'r':
				oDst.Insert((uint32)(-1), '\r');
				break;
			case 't':
				oDst.Insert((uint32)(-1), '\t');
				break;
			case 'v':
				oDst.Insert((uint32)(-1), '\v');
				break;
			case '\'':
				oDst.Insert((uint32)(-1), '\'');
				break;
			case '\"':
				oDst.Insert((uint32)(-1), '\"');
				break;
			case '\\':
				oDst.Insert((uint32)(-1), '\\');
				break;
			case '?':
				oDst.Insert((uint32)(-1), '?');
				break;
			default:
				if(sSrc[0] == 'x' || sSrc[0] == 'X')
				{
					++sSrc;
					c = sSrc[0];
					if(c >= '0' && c <= '9')
						c -= '0';
					else if(c >= 'A' && c <= 'F')
						c = c - 'A' + 10;
					else
						c = c - 'a' + 10;
					sSrc++;
					if(sSrc[0] != '\"')
					{
						c <<= 4;
						d = sSrc[0];
						if(d >= '0' && d <= '9')
							d -= '0';
						else if(d >= 'A' && d <= 'F')
							d = d - 'A' + 10;
						else
							d = d - 'a' + 10;
						c |= d;
					}
				}
				else
				{
					c = sSrc[0] - '0';
					++sSrc;
					if(sSrc[0] != '\"')
					{
						c <<= 3;
						d = sSrc[0] - '0';
						c |= d;
						++sSrc;
						if(sSrc[0] != '\"')
						{
							d = sSrc[0] - '0';
							if(c>(d|(c<<3)))
								pLexModule->OnError(*pConst, "octal char:%u overflow", (((uint32)c)<<3)|d);
							c <<= 3;
							c |= d;
						}
					}
				}
				oDst.Insert((uint32)(-1), (char)c);
			}
		}
		++sSrc;
	}
	oDst.Insert((uint32)(-1), (char)0);

	CRuleConstExpress* pExp = new CRuleConstExpress(oDst.At(0), oDst.GetSize());
	pExp->SetFile(*pConst);
	oExp = pExp;
}

static void CreateCharConst(CRuleStack &oStack)
{//token Const; express Exp;
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pConst = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	const char* sConst = pConst->GetToken();

	uchar c, d;
	char* val = (char*)sConst;
	char* s = val + 1;
	if(s[0] != '\\')
		c = s[0];
	else
	{
		++s;
		switch(s[0])
		{
		case 'a':
			c = '\a';
			break;
		case 'b':
			c = '\b';
			break;
		case 'f':
			c = '\f';
			break;
		case 'n':
			c = '\n';
			break;
		case 'r':
			c = '\r';
			break;
		case 't':
			c = '\t';
			break;
		case 'v':
			c = '\v';
			break;
		case '\'':
			c = '\'';
			break;
		case '\"':
			c = '\"';
			break;
		case '\\':
			c = '\\';
			break;
		case '?':
			c = '?';
			break;
		default:
			if(s[0] == 'x' || s[0] == 'X')
			{
				++s;
				c = s[0];
				if(c >= '0' && c <= '9')
					c -= '0';
				else if(c >= 'A' && c <= 'F')
					c = c - 'A' + 10;
				else
					c = c - 'a' + 10;
				s++;
				if(s[0] != '\'')
				{
					c <<= 4;
					d = s[0];
					if(d >= '0' && d <= '9')
						d -= '0';
					else if(d >= 'A' && d <= 'F')
						d = d - 'A' + 10;
					else
						d = d - 'a' + 10;
					c |= d;
				}
			}
			else
			{
				c = s[0] - '0';
				++s;
				if(s[0] != '\'')
				{
					c <<= 3;
					d = s[0] - '0';
					c |= d;
					++s;
					if(s[0] != '\'')
					{
						d = s[0] - '0';
						if(c>(d|(c<<3)))
							pLexModule->OnError(*pConst, "octal char:%u overflow", (((uint32)c)<<3)|d);
						c <<= 3;
						c |= d;
					}
				}
			}
		}
	}
	CRuleExpress* pExp = new CRuleConstExpress((int8)c, true);
	pExp->SetFile(*pConst);
	oExp = pExp;
}

static void CreateFloatConst(CRuleStack &oStack)
{//token Const; express Exp;
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pConst = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	const char* sConst = pConst->GetToken();

	char *end, *val;
	float f;
	double d;
	bool bDouble = true;

	val = (char*)sConst;

	errno = 0;
	d = strtod(val, &end);
	int32 nError = errno;
	if(nError)
	{
		if(!d)
			pLexModule->OnError(*pConst, "float constant downflow");
		else
			pLexModule->OnError(*pConst, "float constant overflow");
	}
	if(end && (end[0] == 'F' || end[0] == 'f'))
	{
		bDouble = false;
		f = (float)d;
		if(!nError)
		{
			if(*(uint32*)&f == 0x7F800000)
				pLexModule->OnError(*pConst, "float constant overflow");
			if(!f && d)
				pLexModule->OnError(*pConst, "float constant downflow");
		}
	}
	CRuleExpress* pExp;
	if(bDouble)
		pExp = new CRuleConstExpress(d);
	else
		pExp = new CRuleConstExpress(f);
	pExp->SetFile(*pConst);
	oExp = pExp;
}

static void CreateBoolConst(CRuleStack &oStack)
{//token Const; express Exp;
	CRuleArgv oArgv;
	CRuleExpress* pExp;
	InitRuleArgv(oArgv, oStack);
	CToken* pConst = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	if(!CString::StringCompare(pConst->GetKind(), "True", false))
		pExp = new CRuleConstExpress(true);
	else
		pExp = new CRuleConstExpress(false);
	pExp->SetFile(*pConst);
	oExp = pExp;
}

static void LinkStringConst(CRuleStack &oStack)
{//express Exp, ArgExp;
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oArgExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);

	CRuleConstExpress* pConstExp = (CRuleConstExpress*)(CRuleExpress*)oExp;
	CRuleConstExpress* pConstArgExp = (CRuleConstExpress*)(CRuleExpress*)oArgExp;

	CVector<char> oDst;
	const CRuleStr* pStr1 = &pConstExp->GetValue()->s;
	const CRuleStr* pStr2 = &pConstArgExp->GetValue()->s;
	if(pStr2->len - 1)
	{
		if(pStr1->len - 1)
			oDst.Insert((uint32)(-1), pStr1->s, pStr1->len-1);
		oDst.Insert((uint32)(-1), pStr2->s, pStr2->len);

		CRuleConstExpress* pExp = new CRuleConstExpress(oDst.At(0), oDst.GetSize());
		pExp->SetFile(*pConstExp);
		oExp = pExp;
	}
}

static void CreateCallExp(CRuleStack &oStack)
{//token Name; express Exp;
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pName = GetRuleArgToken(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleExpress* pExp;

	pExp = new CRuleCallExpress(pName->GetToken(), NULL);
	pExp->SetFile(*pName);
	oExp = pExp;
}

static void AddArgExp(CRuleStack &oStack)
{//express Exp, ArgExp;
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oArgExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleCallExpress* pExp = (CRuleCallExpress*)(CRuleExpress*)oExp;
	pExp->AddExpress(oArgExp->Clone());
}

static void AddFieldName(CRuleStack &oStack)
{//string FieldName; token Name
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CString& oFieldName = GetRuleArgString(oArgv);
	CToken* pName = GetRuleArgToken(oArgv);
	if(!oFieldName.Empty())
		oFieldName += ".";
	oFieldName+=pName->GetToken();
}

static void CreateVariableExp(CRuleStack &oStack)
{//"token", "Pos", string FieldName; express Exp
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CString& oFieldName = GetRuleArgString(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleExpress* pExp = new CRuleVariableExpress(oFieldName.GetStr());
	pExp->SetFile(*pPos);
	oExp = pExp;
}

static void CreateVectorExpress(CRuleStack &oStack)
{//express& Exp, IndexExp
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oIndexExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CRuleVectorExpress* pExp = new CRuleVectorExpress(*oExp, *oIndexExp);
	pExp->SetFile(*pExp);
	oExp = pExp;
}

static void CreateVectorMethod(CRuleStack &oStack)
{//string FieldName, express& Exp, ArgExp
	CRuleArgv oArgv;
	CMetaCompileModule* pModule = (CMetaCompileModule*)CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();

	InitRuleArgv(oArgv, oStack);
	CToken* pPos = GetRuleArgToken(oArgv);
	CString& oFieldName = GetRuleArgString(oArgv);
	CMasterPointer<CRuleExpress>& oExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	CMasterPointer<CRuleExpress>& oArgExp = GetRuleArgObject< CMasterPointer<CRuleExpress> >(oArgv);
	const char* sName = oFieldName.GetStr();
	char* sMethod = CString::CharOfString(sName, '.', true);
	*sMethod = '\0';
	++sMethod;
	CRuleVariableExpress oVarExp(sName);
	oVarExp.SetFile(*pPos);
	if(!CString::StringCompare(sMethod, "size", false))
	{
		if(oArgExp)
			pLexModule->OnError(*oArgExp, "size method only need one para");
		else
		{
			CRuleVectorSetSizeExpress* pSetSize = new CRuleVectorSetSizeExpress(oVarExp, *oExp);
			pSetSize->SetFile(*pPos);
			oExp = pSetSize;
		}
	}
	else if(!CString::StringCompare(sMethod, "remove", false))
	{
		if(oArgExp)
			pLexModule->OnError(*oArgExp, "remove method only need one para");
		else
		{
			CRuleVectorRemoveExpress* pRemove = new CRuleVectorRemoveExpress(oVarExp, *oExp);
			pRemove->SetFile(*pPos);
			oExp = pRemove;
		}
	}
	else if(!CString::StringCompare(sMethod, "insert", false))
	{
		if(!oArgExp)
			pLexModule->OnError(*pPos, "remove method need two parameters");
		else
		{
			CRuleVectorInsertExpress* pInsert = new CRuleVectorInsertExpress(oVarExp, *oExp, *oArgExp);
			pInsert->SetFile(*pPos);
			oExp = pInsert;
		}
	}
	else
		pLexModule->OnError(*pPos, "invalid vector method");
}

static void DefinePrimaryExpressRule(CSyntaxSystem* pSynSystem)
{//标识符( '(' [赋值表达式 {','赋值表达式}] ')' | {'.'字段名} ) ['('向量参数表达式 [',' 向量参数表达式]')' | '['向量下标表达式']'] | 整型常量 | 浮点常量 | 字符常量 | 字符串常量 | '(' 表达式 ')'
	CRule* pRule = (
					   (
						   D("IntegerKey", "Const") >>
						   C(("CreateIntConst", V("Const"), V("Exp"), NULL))
					   ) |
					   (
						   D("StringKey", "Const") >>
						   C(("CreateStringConst", V("Const"), V("Exp"), NULL)) >>
						   *(
							   D("StringKey", "Const") >>
							   C(("CreateStringConst", V("Const"), V("ArgExp"), NULL)) >>
							   C(("LinkStringConst", V("Exp"), V("ArgExp"), NULL))
						   )
					   ) |
					   (
						   D("CharKey", "Const") >>
						   C(("CreateCharConst", V("Const"), V("Exp"), NULL))
					   ) |
					   (
						   D("FloatKey", "Const") >>
						   C(("CreateFloatConst", V("Const"), V("Exp"), NULL))
					   ) |
					   (
						   (
							   D("TRUE", "Const") |
							   D("FALSE", "Const")
						   ) >>
						   C(("CreateBoolConst", V("Const"), V("Exp"), NULL))
					   ) |
					   (
						   T("LeftParen") >>
						   C(("ExpressRule", V("Exp"), NULL)) >>
						   T("RightParen")
					   ) |
					   (
						   D("Identifier", "Name") >>
						   (
							   (
								   T("LeftParen") >>
								   C(("CreateCallExp", V("Name"), V("Exp"), NULL)) >>
								   -(
									   C(("AssignExpressRule", V("ArgExp"), NULL)) >>
									   C(("AddArgExp", V("Exp"), V("ArgExp"), NULL)) >>
									   *(
										   T("Comma") >>
										   C(("AssignExpressRule", V("ArgExp"), NULL)) >>
										   C(("AddArgExp", V("Exp"), V("ArgExp"), NULL))
									   )
								   ) >>
								   T("RightParen")
							   ) |
							   (
								   C(("AddFieldName", V("FieldName"), V("Name"), NULL)) >>
								   *(
									   T("Period") >>
									   D("Identifier", "Name2") >>
									   C(("AddFieldName", V("FieldName"), V("Name2"), NULL))
								   ) >>
								   (
										(
											T("LeftParen")>>
											C(("AssignExpressRule", V("Exp"), NULL)) >>
										   -(
												T("Comma") >>
												C(("AssignExpressRule", V("ArgExp"), NULL))
											) >>
											T("RightParen") >>
											C(("CreateVectorMethod", V("Name"), V("FieldName"), V("Exp"), V("ArgExp"), NULL))
									    ) |
										C(("CreateVariableExp", V("Name"), V("FieldName"), V("Exp"), NULL))
								   )
							   )
						   ) >>
						  -(
								T("LeftBracket") >> 
								C(("ExpressRule", V("IndexExp"), NULL)) >>
								T("RightBracket") >> 
								C(("CreateVectorExpress", V("Exp"), V("IndexExp"), NULL))
						   )
					   )
				   ).Clone();
	new CSyntaxRule(pSynSystem, "PrimaryExpressRule", pRule, false,
					"express& Exp", NULL,
					"token Const",
					"token Name",
					"token Name2",
					"token Sharp", 
					"string FieldName",
					"express ArgExp", 
					"express IndexExp", NULL);
	new CSyntaxRule(pSynSystem, "CreateIntConst", NULL, false,
					"token Const",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateStringConst", NULL, false,
					"token Const",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "LinkStringConst", NULL, false,
					"express& Exp",
					"express ArgExp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateCharConst", NULL, false,
					"token Const",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateFloatConst", NULL, false,
					"token Const",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateBoolConst", NULL, false,
					"token Const",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateCallExp", NULL, false,
					"token Name",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "AddArgExp", NULL, false,
					"express Exp",
					"express ArgExp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "AddFieldName", NULL, false,
					"string& FieldName",
					"token Name", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateVariableExp", NULL, false,
					"token Pos",
					"string FieldName",
					"express& Exp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateVectorExpress", NULL, false,
					"express& Exp", 
					"express IndexExp", NULL, NULL);
	new CSyntaxRule(pSynSystem, "CreateVectorMethod", NULL, false,
					"token Pos",
					"string FieldName",
					"express& Exp", 
					"express ArgExp", NULL, NULL);
	pSynSystem->RegHost("CreateIntConst", CreateIntConst);
	pSynSystem->RegHost("CreateStringConst", CreateStringConst);
	pSynSystem->RegHost("LinkStringConst", LinkStringConst);
	pSynSystem->RegHost("CreateCharConst", CreateCharConst);
	pSynSystem->RegHost("CreateFloatConst", CreateFloatConst);
	pSynSystem->RegHost("CreateBoolConst", CreateBoolConst);
	pSynSystem->RegHost("CreateCallExp", CreateCallExp);
	pSynSystem->RegHost("AddArgExp", AddArgExp);
	pSynSystem->RegHost("AddFieldName", AddFieldName);
	pSynSystem->RegHost("CreateVariableExp", CreateVariableExp);
	pSynSystem->RegHost("CreateVectorExpress", CreateVectorExpress);
	pSynSystem->RegHost("CreateVectorMethod", CreateVectorMethod);
}

//----------------------------------------------------------------------
// 内部函数预定义
//----------------------------------------------------------------------

//字符(串)操作函数
static void IsAlnum(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsAlnum(c);
}

static void IsAlpha(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsAlpha(c);
}

static void IsControl(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsControl(c);
}

static void IsDigit(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsDigit(c);
}

static void IsGraph(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsGraph(c);
}

static void IsLower(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsLower(c);
}

static void IsUpper(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsUpper(c);
}

static void IsPrint(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsPrint(c);
}

static void IsPunct(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsPunct(c);
}

static void IsSpace(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsSpace(c);
}

static void IsXdigit(CRuleStack &oStack)//bool&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	bool& bRet = GetRuleArgObject<bool>(oArgv);
	char c = GetRuleArgChar(oArgv);
	bRet = CString::IsXdigit(c);
}

static void ToLower(CRuleStack &oStack)//char&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	char& cRet = GetRuleArgObject<char>(oArgv);
	char c = GetRuleArgChar(oArgv);
	cRet = CString::ToLower(c);
}

static void ToUpper(CRuleStack &oStack)//char&, char
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	char& cRet = GetRuleArgObject<char>(oArgv);
	char c = GetRuleArgChar(oArgv);
	cRet = CString::ToUpper(c);
}

static void Atoi(CRuleStack &oStack)//int32& ret, string str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	nRet = CString::Atoi(oStr.GetStr(), NULL);
}

static void Atoi64(CRuleStack &oStack)//int64 &ret, string str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int64 &nRet = GetRuleArgObject<int64>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	nRet = CString::Atoi64(oStr.GetStr(), NULL);
}

static void Atof(CRuleStack &oStack)//double &ret, string str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	double &nRet = GetRuleArgObject<double>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	nRet = CString::Atof(oStr.GetStr(), NULL);
}

static void StrToInt(CRuleStack &oStack)//int32& ret, string str, string &end
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	CString& oEnd = GetRuleArgString(oArgv);
	const char* sEnd = NULL;
	nRet = CString::Atoi(oStr.GetStr(), &sEnd);
	oEnd = sEnd;
}

static void StrToInt64(CRuleStack &oStack)//int32& ret, string str, string &end
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int64& nRet = GetRuleArgObject<int64>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	CString& oEnd = GetRuleArgString(oArgv);
	const char* sEnd = NULL;
	nRet = CString::Atoi64(oStr.GetStr(), &sEnd);
	oEnd = sEnd;
}

static void StrToDouble(CRuleStack &oStack)//double& ret, string str, string &end
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	double &nRet = GetRuleArgObject<double>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	CString& oEnd = GetRuleArgString(oArgv);
	const char* sEnd = NULL;
	nRet = CString::Atof(oStr.GetStr(), &sEnd);
	oEnd = sEnd;
}

static void StrLen(CRuleStack &oStack)//uint32 &ret, string str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32 &nRet = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	nRet = oStr.GetSize();
}

static void StrCat(CRuleStack &oStack)//uint32 &nLen, string& dst, string src
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32 &nRet = GetRuleArgObject<uint32>(oArgv);
	CString& oRet = GetRuleArgString(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	oRet += oStr;
	nRet = oRet.GetSize();
}

static void GetChar(CRuleStack &oStack)//char&, string str, uint32 nIdx
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	char& cRet = GetRuleArgObject<char>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	if(nIdx >= oStr.GetSize())
		cRet = '\0';
	else
		cRet = oStr[nIdx];
}

static void SetChar(CRuleStack &oStack)//char& c, string& str, uint32 nidx, char c,c ?????0???0???????κ?Ч??
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	char &r = GetRuleArgObject<char>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	char c = GetRuleArgChar(oArgv);
	if(c && nIdx < oStr.GetSize())
	{
		oStr[nIdx] = c;
		r = c;
	}
	else
		r = '\0';
}

static void ClearStr(CRuleStack &oStack)//uint32& len, string& str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	oStr.Clear();
	nLen = 0;
}

static void TrimLeft(CRuleStack &oStack)//uint32& len, string& str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	oStr.TrimLeft();
	nLen = oStr.GetSize();
}

static void TrimRight(CRuleStack &oStack)//uint32& len, string& str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	oStr.TrimRight();
	nLen = oStr.GetSize();
}

static void Trim(CRuleStack &oStack)//uint32& len, string& str
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	oStr.Trim();
	nLen = oStr.GetSize();
}

static void InsertChar(CRuleStack &oStack)//uint32& len, string& str, uint32 nIdx, char c, uint32 nCount
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	char c = GetRuleArgChar(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	oStr.Insert(nIdx, c, nCount);
	nLen = oStr.GetSize();
}

static void InsertString(CRuleStack &oStack)//uint32& len, string& str, uint32 nIdx, string s, uint32 nCount
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	CString& oSubStr = GetRuleArgString(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	oStr.Insert(nIdx, oSubStr.GetStr(), nCount);
	nLen = oStr.GetSize();
}

static void ReplaceChar(CRuleStack &oStack)//uint32& len, string& str, uint32 nIdx, char c, uint32 nCount
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	char c = GetRuleArgChar(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	oStr.Replace(nIdx, c, nCount);
	nLen = oStr.GetSize();
}

static void ReplaceString(CRuleStack &oStack)//uint32& len, string& str, uint32 nIdx, string s, uint32 nCount
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	CString& oSubStr = GetRuleArgString(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	oStr.Replace(nIdx, oSubStr.GetStr(), nCount);
	nLen = oStr.GetSize();
}

static void RemoveString(CRuleStack &oStack)//uint32& len, string& str, uint32 nIdx, uint32 nCount
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 nIdx = GetRuleArgUInt(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	oStr.Remove(nIdx, nCount);
	nLen = oStr.GetSize();
}

static void StrCmp(CRuleStack &oStack)//int32 &ret, string s1, string s2, bool bSensitive, uint32 count
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	int32& nRet = GetRuleArgObject<int32>(oArgv);
	CString& oStr1 = GetRuleArgString(oArgv);
	CString& oStr2 = GetRuleArgString(oArgv);
	bool bSensitive = GetRuleArgBool(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	nRet = oStr1.Compare(oStr2.GetStr(), bSensitive, nCount);
}

static void FindChar(CRuleStack &oStack)//uint32&ret, string s, uint32 nFrom, char c, bool bSensitive
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nRet = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	uint32 nFrom = GetRuleArgUInt(oArgv);
	char c = GetRuleArgChar(oArgv);
	bool bSensitive = GetRuleArgBool(oArgv);
	nRet = oStr.Find(c, nFrom, bSensitive);
}

static void FindString(CRuleStack &oStack)//uint32&ret, string s, uint32 nFrom, string find, uint32 count, bool bSensitive
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nRet = GetRuleArgObject<uint32>(oArgv);
	CString& oStr1 = GetRuleArgString(oArgv);
	uint32 nFrom = GetRuleArgUInt(oArgv);
	CString& oStr2 = GetRuleArgString(oArgv);
	uint32 nCount = GetRuleArgUInt(oArgv);
	bool bSensitive = GetRuleArgBool(oArgv);
	nRet = oStr1.Find(oStr2.GetStr(), nCount, nFrom, bSensitive);
}

static void StrTok(CRuleStack &oStack)//string&, string s, uint32&nIdx, string oDelimiters);
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CString& oRet = GetRuleArgObject<CString>(oArgv);
	CString& oStr = GetRuleArgString(oArgv);
	uint32& nIdx = GetRuleArgUInt(oArgv);
	CString& oDelimiters = GetRuleArgString(oArgv);
	nIdx = oStr.GetToken(oRet, nIdx, oDelimiters);
}

static void AppendChar(CRuleStack &oStack)//uint32&, string&ret, char c;
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	char v = GetRuleArgChar(oArgv);
	if(v)
		oStr += v;
	nLen = oStr.GetSize();
}

static bool IsPrintFlag(char c)
{
	bool bRet = false;
	switch(c)
	{
	case '-':
	case '0':
	case '+':
	case ' ':
	case '#':
		bRet = true;
	}
	return bRet;
}

static void AppendInt8(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	int8 v = (int8)GetRuleArgChar(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("d8");
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendInt16(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	int16 v = GetRuleArgShort(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("d16");
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendInt32(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	int32 v = GetRuleArgInt(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("d32");
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendInt64(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	int64 v = GetRuleArgLong(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("d64");
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static bool IsPrintSystem(char s)
{//u,o,x,X
	bool bRet = false;
	switch(s)
	{
	case 'u':
	case 'o':
	case 'x':
	case 'X':
		bRet = true;
	}
	return bRet;
}

static void AppendUInt8(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint8 v = (uint8)GetRuleArgChar(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	char s = GetRuleArgChar(oArgv);
	if(!IsPrintSystem(s))
		s = 'u';
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("%c8", s);
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendUInt16(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint16 v = GetRuleArgUShort(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	char s = GetRuleArgChar(oArgv);
	if(!IsPrintSystem(s))
		s = 'u';
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("%c16", s);
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendUInt32(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint32 v = GetRuleArgUInt(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	char s = GetRuleArgChar(oArgv);
	if(!IsPrintSystem(s))
		s = 'u';
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("%c32", s);
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static void AppendUInt64(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	uint64 v = GetRuleArgULong(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	char s = GetRuleArgChar(oArgv);
	if(!IsPrintSystem(s))
		s = 'u';
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	oFmt.Print("%c64", s);
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

static bool IsPrintFloatType(char t)
{
	bool bRet = false;
	switch(t)
	{
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
		bRet = true;
	}
	return bRet;
}

static void AppendFloat(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nLen = GetRuleArgObject<uint32>(oArgv);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	double v = GetRuleArgDouble(oArgv);
	char f = GetRuleArgChar(oArgv);
	uint32 w = GetRuleArgUInt(oArgv);
	uint32 p = GetRuleArgUInt(oArgv);
	char t = GetRuleArgChar(oArgv);
	if(!IsPrintFloatType(t))
		t = 'g';
	CFormatString oFmt, oRet;
	oFmt.Print("%%");
	if(IsPrintFlag(f))//-0+空格#
		oFmt.Print("%c", f);
	if(w)
		oFmt.Print("%u", w);
	if(p)
		oFmt.Print(".%u", p);
	oFmt.Print("%c64", t);
	oRet.Print(oFmt.GetStr(), v);
	oStr += oRet;
	nLen = oStr.GetSize();
}

//单词操作函数
static void PrevToken(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* &pToken = GetRuleArgObject<CToken*>(oArgv);
	CCompileModule* pModule = CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxModule* pSyntaxModule = pModule->GetSyntaxModule();
	pToken = pSyntaxModule->GetCurToken();
}

static void NextToken(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* &pToken = GetRuleArgObject<CToken*>(oArgv);
	CCompileModule* pModule = CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxModule* pSyntaxModule = pModule->GetSyntaxModule();
	pToken = pSyntaxModule->PullToken(false);
}

static void PeekToken(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CToken* &pToken = GetRuleArgObject<CToken*>(oArgv);
	CCompileModule* pModule = CCompileModule::GetCompileModule(oStack.pModule);
	CSyntaxModule* pSyntaxModule = pModule->GetSyntaxModule();
	pToken = pSyntaxModule->PeekToken();
}

static void GetTokenString(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	CToken* pToken = GetRuleArgToken(oArgv);
	oStr.Clear();
	if(pToken)
		oStr = pToken->GetToken();
}

static void GetTokenKind(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	CString& oStr = GetRuleArgObject<CString>(oArgv);
	CToken* pToken = GetRuleArgToken(oArgv);
	oStr.Clear();
	if(pToken)
		oStr = pToken->GetKind();
}

//错误处理函数
static void Error(CRuleStack &oStack)
{
	CRuleArgv oArgv;
	InitRuleArgv(oArgv, oStack);
	uint32& nRet = GetRuleArgObject<uint32>(oArgv);
	bool bWarning = GetRuleArgBool(oArgv);
	CToken* pToken = GetRuleArgToken(oArgv);
	CString& oInfo = GetRuleArgString(oArgv);
	CCompileModule* pModule = CCompileModule::GetCompileModule(oStack.pModule);
	CLexicalModule* pLexModule = pModule->GetLexicalModule();
	if(pToken)
		pLexModule->WriteError(bWarning, pToken->m_sFileName, pToken->m_nLine, pToken->m_nCol, "%s", oInfo.GetStr());
	else
		pLexModule->WriteError(bWarning, "???", 0, 0, "%s", oInfo.GetStr());
	nRet = 0;
}

static void PreDefineSyntaxSystem(CSyntaxSystem* pSynSystem)
{
	//预定义类型
	new CCommonRuleType<bool, ARF_BOOL>(pSynSystem, "bool");
	new CCommonRuleType<int8, ARF_INT8>(pSynSystem, "char");
	new CCommonRuleType<int16, ARF_INT16>(pSynSystem, "short");
	new CCommonRuleType<int32, ARF_INT32>(pSynSystem, "int");
	new CCommonRuleType<int64, ARF_INT64>(pSynSystem, "long");
	new CCommonRuleType<uint8, ARF_UINT8>(pSynSystem, "uchar");
	new CCommonRuleType<uint16, ARF_UINT16>(pSynSystem, "ushort");
	new CCommonRuleType<uint32, ARF_UINT32>(pSynSystem, "uint");
	new CCommonRuleType<uint64, ARF_UINT64>(pSynSystem, "ulong");
	new CCommonRuleType<float, ARF_FLOAT>(pSynSystem, "float");
	new CCommonRuleType<double, ARF_DOUBLE>(pSynSystem, "double");
	new CCommonRuleType<CString, ARF_STRING>(pSynSystem, "string");
	if(sizeof(void*) == sizeof(uint32))
		new CCommonRuleType<void*, ARF_UINT32>(pSynSystem, "token");
	else
		new CCommonRuleType<void*, ARF_UINT64>(pSynSystem, "token");
	//函数注册
	pSynSystem->RegHost("IsAlnum",IsAlnum);
	pSynSystem->RegHost("IsAlpha",IsAlpha);
	pSynSystem->RegHost("IsControl", IsControl);
	pSynSystem->RegHost("IsDigit", IsDigit);
	pSynSystem->RegHost("IsGraph", IsGraph);
	pSynSystem->RegHost("IsLower", IsLower);
	pSynSystem->RegHost("IsUpper", IsUpper);
	pSynSystem->RegHost("IsPrint", IsPrint);
	pSynSystem->RegHost("IsPunct", IsPunct);
	pSynSystem->RegHost("IsSpace", IsSpace);
	pSynSystem->RegHost("IsXdigit", IsXdigit);
	pSynSystem->RegHost("ToLower", ToLower);
	pSynSystem->RegHost("ToUpper", ToUpper);
	pSynSystem->RegHost("Atoi", Atoi);
	pSynSystem->RegHost("Atoi64", Atoi64);
	pSynSystem->RegHost("Atof", Atof);
	pSynSystem->RegHost("StrToInt", StrToInt);
	pSynSystem->RegHost("StrToInt64", StrToInt64);
	pSynSystem->RegHost("StrToDouble", StrToDouble);
	pSynSystem->RegHost("StrLen", StrLen);
	pSynSystem->RegHost("StrCat", StrCat);
	pSynSystem->RegHost("GetChar", GetChar);
	pSynSystem->RegHost("SetChar", SetChar);
	pSynSystem->RegHost("ClearStr", ClearStr);
	pSynSystem->RegHost("TrimLeft", TrimLeft);
	pSynSystem->RegHost("TrimRight", TrimRight);
	pSynSystem->RegHost("Trim", Trim);
	pSynSystem->RegHost("InsertChar", InsertChar);
	pSynSystem->RegHost("InsertString", InsertString);
	pSynSystem->RegHost("ReplaceChar", ReplaceChar);
	pSynSystem->RegHost("ReplaceString", ReplaceString);
	pSynSystem->RegHost("RemoveString", RemoveString);
	pSynSystem->RegHost("StrCmp", StrCmp);
	pSynSystem->RegHost("FindChar", FindChar);
	pSynSystem->RegHost("FindString", FindString);
	pSynSystem->RegHost("StrTok", StrTok);
	pSynSystem->RegHost("AppendChar", AppendChar);
	pSynSystem->RegHost("AppendInt8", AppendInt8);
	pSynSystem->RegHost("AppendInt16", AppendInt16);
	pSynSystem->RegHost("AppendInt32", AppendInt32);
	pSynSystem->RegHost("AppendInt64", AppendInt64);
	pSynSystem->RegHost("AppendUInt8", AppendUInt8);
	pSynSystem->RegHost("AppendUInt16", AppendUInt16);
	pSynSystem->RegHost("AppendUInt32", AppendUInt32);
	pSynSystem->RegHost("AppendUInt64", AppendUInt64);
	pSynSystem->RegHost("AppendFloat", AppendFloat);
	pSynSystem->RegHost("PrevToken", PrevToken);
	pSynSystem->RegHost("NextToken", NextToken);
	pSynSystem->RegHost("PeekToken", PeekToken);
	pSynSystem->RegHost("GetTokenString", GetTokenString);
	pSynSystem->RegHost("GetTokenKind", GetTokenKind);
	pSynSystem->RegHost("Error", Error);
	//函数规则注册
	new CRuleFunc(pSynSystem, "IsAlnum", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsAlpha", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsControl", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsDigit", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsGraph", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsLower", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsUpper", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsPrint", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsPunct", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsSpace", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "IsXdigit", "bool",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "ToLower", "char",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "ToUpper", "char",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "Atoi", "int",
				  "string str", NULL, NULL);
	new CRuleFunc(pSynSystem, "Atoi64", "long",
				  "string str", NULL, NULL);
	new CRuleFunc(pSynSystem, "Atof", "double",
				  "string str", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrToInt", "int",
				  "string str",
				  "string& end", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrToInt64", "long",
				  "string str",
				  "string& end", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrToDouble", "double",
				  "string str",
				  "string& end", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrLen", "uint",
				  "string str", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrCat", "uint",
				  "string& s1",
				  "string s2", NULL, NULL);
	new CRuleFunc(pSynSystem, "GetChar", "char",
				  "string str",
				  "uint nIdx", NULL, NULL);
	new CRuleFunc(pSynSystem, "SetChar", "char",
				  "string& str",
				  "uint nIdx",
				  "char c", NULL, NULL);
	new CRuleFunc(pSynSystem, "ClearStr", "uint",
				  "string& str", NULL, NULL);
	new CRuleFunc(pSynSystem, "TrimLeft", "uint",
				  "string& str", NULL, NULL);
	new CRuleFunc(pSynSystem, "TrimRight", "uint",
				  "string& str", NULL, NULL);
	new CRuleFunc(pSynSystem, "Trim", "uint",
				  "string& str", NULL, NULL);
	new CRuleFunc(pSynSystem, "InsertChar", "uint",
				  "string& str",
				  "uint nIdx",
				  "char c",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "InsertString", "uint",
				  "string& str",
				  "uint nIdx",
				  "string SubStr",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "ReplaceChar", "uint",
				  "string& str",
				  "uint nIdx",
				  "char c",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "ReplaceString", "uint",
				  "string& str",
				  "uint nIdx",
				  "string SubStr",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "RemoveString", "uint",
				  "string& str",
				  "uint nIdx",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrCmp", "int",
				  "string s1",
				  "string s2",
				  "bool bSensitive",
				  "uint nCount", NULL, NULL);
	new CRuleFunc(pSynSystem, "FindChar", "uint",
				  "string s",
				  "uint nFrom",
				  "char c",
				  "bool bSensitive", NULL, NULL);
	new CRuleFunc(pSynSystem, "FindString", "uint",
				  "string s",
				  "uint nFrom",
				  "string substr",
				  "uint nCount",
				  "bool bSensitive", NULL, NULL);
	new CRuleFunc(pSynSystem, "StrTok", "string",
				  "string oStr",
				  "uint& nIdx",
				  "string oDelimiters", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendChar", "uint",
				  "string& ret",
				  "char v", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendInt8", "uint",
				  "string& ret",
				  "char v",
				  "char flag",
				  "uint width", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendInt16", "uint",
				  "string& ret",
				  "short v",
				  "char flag",
				  "uint width", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendInt32", "uint",
				  "string& ret",
				  "int v",
				  "char flag",
				  "uint width", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendInt64", "uint",
				  "string& ret",
				  "long v",
				  "char flag",
				  "uint width", NULL, NULL);
	new CRuleFunc(pSynSystem, "AppendUInt8", "uint",
				  "string& ret",
				  "uchar v",
				  "char flag",
				  "uint width",
				  "char system", NULL, NULL);//u,o,x,X
	new CRuleFunc(pSynSystem, "AppendUInt16", "uint",
				  "string& ret",
				  "ushort v",
				  "char flag",
				  "uint width",
				  "char system", NULL, NULL);//u,o,x,X
	new CRuleFunc(pSynSystem, "AppendUInt32", "uint",
				  "string& ret",
				  "uint v",
				  "char flag",
				  "uint width",
				  "char system", NULL, NULL);//u,o,x,X
	new CRuleFunc(pSynSystem, "AppendUInt64", "uint",
				  "string& ret",
				  "ulong v",
				  "char flag",
				  "uint width",
				  "char system", NULL, NULL);//u,o,x,X
	new CRuleFunc(pSynSystem, "AppendFloat", "uint",
				  "string& ret",
				  "double v",
				  "char flag",
				  "uint width",
				  "uint prec",
				  "char type", NULL, NULL);
	new CRuleFunc(pSynSystem, "PrevToken", "token", NULL, NULL);
	new CRuleFunc(pSynSystem, "NextToken", "token", NULL, NULL);
	new CRuleFunc(pSynSystem, "PeekToken", "token", NULL, NULL);
	new CRuleFunc(pSynSystem, "GetTokenString", "string",
				  "token tok", NULL, NULL);
	new CRuleFunc(pSynSystem, "GetTokenKind", "string",
				  "token tok", NULL, NULL);
	new CRuleFunc(pSynSystem, "Error", "uint",
				  "bool bWaring",
				  "token tok",
				  "string info", NULL, NULL);
}

FOCP_END();
