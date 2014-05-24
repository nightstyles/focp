
#include "RuleCompiler.hpp"

#ifndef _ARF_META_COMPILER_HPP_
#define _ARF_META_COMPILER_HPP_

FOCP_BEGIN();

///////////////////////////////////////////////////////////////////////////////
//元编译器文法定义
// 1.词法规范：
//	标识符：C语言标示符，不区分大小写
//	字符串: C语言字符串
//	关键字: 不区分大小写
//		WHITESPACE，用于描述空白字符序列
//		PUNCTS，用于定义标记
//		TOKENS，用于定义单词
//		TYPES，用于定义类型
//		VARIABLES，用于定义全局变量
//		RULES，用于定义语法规则
//		MAIN, 用于说明规则是主规则
//		BREAK，用于构建中断规则或Break语句
//		CONST,	用于修饰词法规则是常量串还是正则串，
//		INSENSITIVE，用于修饰常量单词为大小写敏感。
//		SKIP，用于修饰单词是否被忽略，将不计入单词库。
//		IF，用于定义条件规则或条件语句
//		STRUCT，用于定义结构类型
//		UNION，用于定义联合类型
//		VECTOR，用于定义向量字段
//		SWITCH, WHILE, DO, FOR, CONTINUE, ELSE, RETURN, DEFAULT，CASE用于定义语句。
//	因向量引发的规则变化：
//		类型
//		变量定义, VariableRule
//		参数定义, ParameterRule
//		单个函数, FunctionRule
// 2.语法规范：
//	语法定义 -> [空白定义] [标记定义] 单词定义 [类型定义] [全局变量] 规则定义
//	空白定义 -> WHITESPACE '=' 字符串 '.'
//	标记定义 -> PUNCTS '=' 字符串 '.'
//	单词定义 -> TOKENS ':' 单个单词 {单个单词}
//	单个单词 -> [CONST [INSENSITIVE] ] 单词名 '=' [SKIP] 正则串 '.'
//	类型定义 -> TYPES ':' 单个类型 {单个类型}
//	单个类型 -> (STRUCT|UNION) 标识符 [结构体] ';'
//	结构体	 -> '{' { 字段定义 ';'} '}'
//	类型	 -> 类型名 | VECTOR '<' 类型名 '>'
//	字段定义 -> 类型 字段名
//	全局变量 -> VARIABLES ':' 变量定义 ';' {变量定义 ';'}
//	变量定义 -> 类型 变量名
//	规则定义 -> RULES ':' (单个规则 | 单个函数) {单个规则 | 单个函数}
//	单个规则 -> (MAIN 规则名 | 规则名 [ '<' 参数定义 { ',' 参数定义} '>' ] ) '=' [局部变量] 规则项 '.'
//	参数定义 -> 类型 [ '&' ] 参数名
//		字符'&'表示可以输出。
//	局部变量 -> '<' 变量定义 { ',' 变量定义} '>'
//	规则项   -> 中断规则 | 条件规则 | 调用规则 | 顺序规则 | 测试规则 | 乐观规则 | 选择规则 | 任意次规则 | 可选规则 | 语义规则
//	中断规则 -> 'Break'
//	调用规则 -> ['$' | '#'] 规则名 [ '<!' 赋值表达式 {',' 赋值表达式} '!>' ]
//			'$'表示行首单词，列号可不为1
//			'#'表示行首单词，列号必须为1
//  条件规则 -> IF '<!' 表达式 '!>'
//		因为IF是一个关键字，无法匹配到调用规则上
//	顺序规则 -> '(' 规则项 { 规则项 } ')'
//	测试规则 -> '[?' 规则项 { 规则项 } '?]'
//	乐观规则 -> '[' 规则项 { 规则项 } ']'
//	选择规则 -> '(?' 规则项 { 规则项 } '?)'
//	任意次规则 -> '{' 规则项 { 规则项 } '}'
//	可选规则 -> '{?' 规则项 { 规则项 } '?}'
//	语义规则 -> '(.' {语句定义} '.)'
//	单个函数 -> 类型 函数名 '(' [ 参数定义 { ',' 参数定义} ] ')' (函数体 | ';')
//		没有函数体的函数为内部函数，并非向前申明（也没有必要）。
//	函数体	 -> '{' {变量定义 ';'} {语句定义} '}'
//	语句定义 -> 表达式语句 | RETURN语句 | IF语句 | WHILE语句 | DO语句 | FOR语句 | SWITCH语句 | ELSE语句 | BREAK语句 | CONTINUE语句 | CASE语句 | DEFAULT语句 | 复合语句
//	表达式语句 -> [表达式] ';'
//	RETURN语句 -> RETURN ';'
//	IF语句	 -> IF '(' 表达式 ')' 语句定义 [ ELSE语句 ]
//	WHILE语句 -> WHILE '(' 表达式 ')' 语句定义
//	DO语句 -> DO 复合语句 WHILE '(' 表达式 ')' ';'
//	FOR语句 -> FOR '(' [表达式] ';' [表达式] ';' [表达式] ')' 语句定义
//	SWITCH语句 -> SWITCH '(' 表达式 ')' 复合语句
//	ELSE语句 -> ELSE 语句定义
//	BREAK语句 -> BREAK ';'
//	CONTINUE语句 -> CONTINUE ';'
//	CASE语句 -> CASE (整型常量 | 字符常量) ':'
//	DEFAULT语句 -> DEFAULT ':'
//	复合语句 -> '{' {语句定义} '}'
//	表达式	 -> 赋值表达式 {',' 赋值表达式}
//	赋值表达式 -> 逻辑或表达式 [ ('=' | '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '&=' | '^=' | '|=') 赋值表达式]
//	逻辑或表达式 -> 逻辑与表达式 {'||' 逻辑与表达式}
//	逻辑与表达式 -> 位或表达式 {'&&' 位或表达式}
//	位或表达式 -> 位异或表达式 {'|' 位异或表达式}
//	位异或表达式 -> 位与表达式 {'^' 位与表达式}
//	位与表达式 -> 相等表达式 {'&' 相等表达式}
//	相等表达式 -> 比较表达式 { ('==' | '!=') 比较表达式}
//	比较表达式 -> 移位表达式 { ('<' | '>' | '<=' | '>=') 移位表达式}
//	移位表达式 -> 加法表达式 { ('<<' | '>>') 加法表达式}
//	加法表达式 -> 乘法表达式 { ('+' | '-') 乘法表达式}
//	乘法表达式 -> 转换表达式 { ('*' | '/' | '%') 转换表达式}
//	转换表达式 -> '(' 类型名 ')' 转换表达式 | 一元表达式
//	一元表达式 = {'++' | '--'} ( 基本表达式 {'++' | '--'} | ( '&' | '*' | '+' | '-' | '~' | '!') 转换表达式)
//	基本表达式 = ['$']标识符( '(' [赋值表达式 {','赋值表达式}] ')' | {'.'['$']字段名} ) ['('向量参数表达式 [',' 向量参数表达式]')' | '['向量下标表达式']'] | 整型常量 | 浮点常量 | 字符常量 | 字符串常量 | '(' 表达式 ')'
//	正则串   -> 字符串
//	单词名   -> 标识符
//	类型名	 -> 标识符
//	字段名	 -> 标识符
//	变量名	 -> 标识符
//	规则名   -> 标识符
//	参数名   -> 标识符
//	注意，没有采用随机规则，后续可加强
// 3.系统规则定义：
//       基本类型: bool, char, short, int, long, uchar, ushort, uint, ulong, float, double, string, token
//	 扩展类型: struct, union
//	 系统函数:
//		bool IsAlnum(char c);
//		bool IsAlpha(char c);
//		bool IsControl(char c);
//		bool IsDigit(char c);
//		bool IsGraph(char c);
//		bool IsLower(char c);
//		bool IsUpper(char c);
//		bool IsPrint(char c);
//		bool IsPunct(char c);
//		bool IsSpace(char c);
//		bool IsXdigit(char c);
//		char ToLower(char c);
//		char ToUpper(char c);
//		int Atoi(string s);
//		long Atoi64(string s);
//		double Atof(string s);
//		int StrToInt(string s, string& end);
//		long StrToInt64(string s, string& end);
//		double StrToDouble(string s, string& end);
//		uint StrLen(string s);
//		uint StrCat(string& s1, string s2);
//		char GetChar(string s, uint nIdx);
//		char SetChar(string& s, uint nIdx, char c);
//		uint ClearStr(string& s);
//		uint TrimLeft(string& s);
//		uint TrimRight(string& s);
//		uint Trim(string& s);
//		uint InsertChar(string& s, uint nIdx, char c, uint nCount);
//		uint InsertString(string& s, uint nIdx, string SubStr, uint nCount);
//		uint ReplaceChar(string& s, uint nIdx, char c, uint nCount);
//		uint ReplaceString(string& s, uint nIdx, string SubStr, uint nCount);
//		uint RemoveString(string& s, uint nIdx, uint nCount);
//		uint AppendChar(string& s, char v);
//		uint AppendInt8(string& s, char v, char flag, uint width);
//		uint AppendInt16(string& s, short v, char flag, uint width);
//		uint AppendInt32(string& s, int v, char flag, uint width);
//		uint AppendInt64(string& s, long v, char flag, uint width);
//		uint AppendUInt8(string& s, uchar v, char flag, uint width);
//		uint AppendUInt16(string& s, ushort v, char flag, uint width);
//		uint AppendUInt32(string& s, uint v, char flag, uint width);
//		uint AppendUInt64(string& s, ulong v, char flag, uint width);
//		uint AppendFloat(string& s, double v, char flag, uint width, uint prec, char type);//type=efgEG
//		int StrCmp(string s1, string s2, bool bSensitive, uint nMaxCount);
//		uint FindChar(string s, uint nFrom, char c, bool bSensitive);
//		uint FindString(string s, uint nFrom, string substr, uint nCount, bool bSensitive);
//		string StrTok(string s, uint& nIdx, string delimiters);
//		token PrevToken();
//		token NextToken();
//		token PeekToken();
//		string GetTokenString(token t);
//		string GetTokenKind(token t);
//		uint Error(bool bWaring, token pos, string info);
///////////////////////////////////////////////////////////////////////////////
RULE_API uint32 MetaCompile(CFile &oErrFile, CFile& oSyntaxFile, CCompileSystem* pSystem, uint32 &nWarning);

FOCP_END();

#endif
