
#include "RuleCompiler.hpp"

#ifndef _ARF_META_COMPILER_HPP_
#define _ARF_META_COMPILER_HPP_

FOCP_BEGIN();

///////////////////////////////////////////////////////////////////////////////
//Ԫ�������ķ�����
// 1.�ʷ��淶��
//	��ʶ����C���Ա�ʾ���������ִ�Сд
//	�ַ���: C�����ַ���
//	�ؼ���: �����ִ�Сд
//		WHITESPACE�����������հ��ַ�����
//		PUNCTS�����ڶ�����
//		TOKENS�����ڶ��嵥��
//		TYPES�����ڶ�������
//		VARIABLES�����ڶ���ȫ�ֱ���
//		RULES�����ڶ����﷨����
//		MAIN, ����˵��������������
//		BREAK�����ڹ����жϹ����Break���
//		CONST,	�������δʷ������ǳ������������򴮣�
//		INSENSITIVE���������γ�������Ϊ��Сд���С�
//		SKIP���������ε����Ƿ񱻺��ԣ��������뵥�ʿ⡣
//		IF�����ڶ�������������������
//		STRUCT�����ڶ���ṹ����
//		UNION�����ڶ�����������
//		VECTOR�����ڶ��������ֶ�
//		SWITCH, WHILE, DO, FOR, CONTINUE, ELSE, RETURN, DEFAULT��CASE���ڶ�����䡣
//	�����������Ĺ���仯��
//		����
//		��������, VariableRule
//		��������, ParameterRule
//		��������, FunctionRule
// 2.�﷨�淶��
//	�﷨���� -> [�հ׶���] [��Ƕ���] ���ʶ��� [���Ͷ���] [ȫ�ֱ���] ������
//	�հ׶��� -> WHITESPACE '=' �ַ��� '.'
//	��Ƕ��� -> PUNCTS '=' �ַ��� '.'
//	���ʶ��� -> TOKENS ':' �������� {��������}
//	�������� -> [CONST [INSENSITIVE] ] ������ '=' [SKIP] ���� '.'
//	���Ͷ��� -> TYPES ':' �������� {��������}
//	�������� -> (STRUCT|UNION) ��ʶ�� [�ṹ��] ';'
//	�ṹ��	 -> '{' { �ֶζ��� ';'} '}'
//	����	 -> ������ | VECTOR '<' ������ '>'
//	�ֶζ��� -> ���� �ֶ���
//	ȫ�ֱ��� -> VARIABLES ':' �������� ';' {�������� ';'}
//	�������� -> ���� ������
//	������ -> RULES ':' (�������� | ��������) {�������� | ��������}
//	�������� -> (MAIN ������ | ������ [ '<' �������� { ',' ��������} '>' ] ) '=' [�ֲ�����] ������ '.'
//	�������� -> ���� [ '&' ] ������
//		�ַ�'&'��ʾ���������
//	�ֲ����� -> '<' �������� { ',' ��������} '>'
//	������   -> �жϹ��� | �������� | ���ù��� | ˳����� | ���Թ��� | �ֹ۹��� | ѡ����� | ����ι��� | ��ѡ���� | �������
//	�жϹ��� -> 'Break'
//	���ù��� -> ['$' | '#'] ������ [ '<!' ��ֵ���ʽ {',' ��ֵ���ʽ} '!>' ]
//			'$'��ʾ���׵��ʣ��кſɲ�Ϊ1
//			'#'��ʾ���׵��ʣ��кű���Ϊ1
//  �������� -> IF '<!' ���ʽ '!>'
//		��ΪIF��һ���ؼ��֣��޷�ƥ�䵽���ù�����
//	˳����� -> '(' ������ { ������ } ')'
//	���Թ��� -> '[?' ������ { ������ } '?]'
//	�ֹ۹��� -> '[' ������ { ������ } ']'
//	ѡ����� -> '(?' ������ { ������ } '?)'
//	����ι��� -> '{' ������ { ������ } '}'
//	��ѡ���� -> '{?' ������ { ������ } '?}'
//	������� -> '(.' {��䶨��} '.)'
//	�������� -> ���� ������ '(' [ �������� { ',' ��������} ] ')' (������ | ';')
//		û�к�����ĺ���Ϊ�ڲ�������������ǰ������Ҳû�б�Ҫ����
//	������	 -> '{' {�������� ';'} {��䶨��} '}'
//	��䶨�� -> ���ʽ��� | RETURN��� | IF��� | WHILE��� | DO��� | FOR��� | SWITCH��� | ELSE��� | BREAK��� | CONTINUE��� | CASE��� | DEFAULT��� | �������
//	���ʽ��� -> [���ʽ] ';'
//	RETURN��� -> RETURN ';'
//	IF���	 -> IF '(' ���ʽ ')' ��䶨�� [ ELSE��� ]
//	WHILE��� -> WHILE '(' ���ʽ ')' ��䶨��
//	DO��� -> DO ������� WHILE '(' ���ʽ ')' ';'
//	FOR��� -> FOR '(' [���ʽ] ';' [���ʽ] ';' [���ʽ] ')' ��䶨��
//	SWITCH��� -> SWITCH '(' ���ʽ ')' �������
//	ELSE��� -> ELSE ��䶨��
//	BREAK��� -> BREAK ';'
//	CONTINUE��� -> CONTINUE ';'
//	CASE��� -> CASE (���ͳ��� | �ַ�����) ':'
//	DEFAULT��� -> DEFAULT ':'
//	������� -> '{' {��䶨��} '}'
//	���ʽ	 -> ��ֵ���ʽ {',' ��ֵ���ʽ}
//	��ֵ���ʽ -> �߼�����ʽ [ ('=' | '*=' | '/=' | '%=' | '+=' | '-=' | '<<=' | '>>=' | '&=' | '^=' | '|=') ��ֵ���ʽ]
//	�߼�����ʽ -> �߼�����ʽ {'||' �߼�����ʽ}
//	�߼�����ʽ -> λ����ʽ {'&&' λ����ʽ}
//	λ����ʽ -> λ�����ʽ {'|' λ�����ʽ}
//	λ�����ʽ -> λ����ʽ {'^' λ����ʽ}
//	λ����ʽ -> ��ȱ��ʽ {'&' ��ȱ��ʽ}
//	��ȱ��ʽ -> �Ƚϱ��ʽ { ('==' | '!=') �Ƚϱ��ʽ}
//	�Ƚϱ��ʽ -> ��λ���ʽ { ('<' | '>' | '<=' | '>=') ��λ���ʽ}
//	��λ���ʽ -> �ӷ����ʽ { ('<<' | '>>') �ӷ����ʽ}
//	�ӷ����ʽ -> �˷����ʽ { ('+' | '-') �˷����ʽ}
//	�˷����ʽ -> ת�����ʽ { ('*' | '/' | '%') ת�����ʽ}
//	ת�����ʽ -> '(' ������ ')' ת�����ʽ | һԪ���ʽ
//	һԪ���ʽ = {'++' | '--'} ( �������ʽ {'++' | '--'} | ( '&' | '*' | '+' | '-' | '~' | '!') ת�����ʽ)
//	�������ʽ = ['$']��ʶ��( '(' [��ֵ���ʽ {','��ֵ���ʽ}] ')' | {'.'['$']�ֶ���} ) ['('�����������ʽ [',' �����������ʽ]')' | '['�����±���ʽ']'] | ���ͳ��� | ���㳣�� | �ַ����� | �ַ������� | '(' ���ʽ ')'
//	����   -> �ַ���
//	������   -> ��ʶ��
//	������	 -> ��ʶ��
//	�ֶ���	 -> ��ʶ��
//	������	 -> ��ʶ��
//	������   -> ��ʶ��
//	������   -> ��ʶ��
//	ע�⣬û�в���������򣬺����ɼ�ǿ
// 3.ϵͳ�����壺
//       ��������: bool, char, short, int, long, uchar, ushort, uint, ulong, float, double, string, token
//	 ��չ����: struct, union
//	 ϵͳ����:
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
