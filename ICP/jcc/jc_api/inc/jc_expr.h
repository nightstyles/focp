
#ifndef _jc_expr_h_
#define _jc_expr_h_

#include "jc_typeinfo.h"

typedef struct CJcSymbolStack CJcSymbolStack;
typedef union CJcVal CJcVal;
typedef struct CJcSymbol CJcSymbol;

typedef struct CJcExpress
{
	CJcTypeInfo* pType;
	jc_int nVarId;
	jc_uint nOpt, nArg;
	jc_uint nBits;
	jc_uint nStart;
	jc_uint nQuote;
	jc_uint nLeftValue;
	CJcSymbolStack* pStack;
}CJcExpress;

typedef struct CJcCastNode
{
	jc_uint nClass, nType, nPrevArith, nNextArith, nUpArith, nDownArith;
}CJcCastNode;

typedef struct CJcCastTable
{
	jc_uint nCount;
	CJcCastNode * pNode;
}CJcCastTable;

typedef enum JC_UNARY_OPERATOR
{
	JC_UNARY_GETADDR='&', JC_UNARY_GETVALUE='*', 
	JC_UNARY_POSITIVE='+', JC_UNARY_NEGATIVE='-', 
	JC_UNARY_BIT_NOT='~', JC_UNARY_LOGIC_NOT='!'
}JC_UNARY_OPERATOR;

typedef enum JC_BINARY_OPERATOR
{
	JC_BINARY_ADD=1, JC_BINARY_SUB, JC_BINARY_MUL, JC_BINARY_DIV, JC_BINARY_MOD,
	JC_BINARY_LSHIFT, JC_BINARY_RSHIFT, 
	JC_BINARY_BIT_AND, JC_BINARY_BIT_OR, JC_BINARY_BIT_XOR, 

	JC_BINARY_LT, JC_BINARY_LE, JC_BINARY_MT, JC_BINARY_ME, JC_BINARY_EQ, JC_BINARY_NE,
	JC_BINARY_LOGIC_AND, JC_BINARY_LOGIC_OR
}JC_BINARY_OPERATOR;

typedef enum JC_ASSIGN_OPERATOR
{
	JC_ASSIGN_EQUAL = 0xFF000000, 
	JC_ASSIGN_ADD_EQ = JC_BINARY_ADD | JC_ASSIGN_EQUAL,
	JC_ASSIGN_SUB_EQ = JC_BINARY_SUB | JC_ASSIGN_EQUAL,
	JC_ASSIGN_MUL_EQ = JC_BINARY_MUL | JC_ASSIGN_EQUAL,
	JC_ASSIGN_DIV_EQ = JC_BINARY_DIV | JC_ASSIGN_EQUAL,
	JC_ASSIGN_MOD_EQ = JC_BINARY_MOD | JC_ASSIGN_EQUAL,  
	JC_ASSIGN_LSH_EQ = JC_BINARY_LSHIFT | JC_ASSIGN_EQUAL,
	JC_ASSIGN_RSH_EQ = JC_BINARY_RSHIFT | JC_ASSIGN_EQUAL,
	JC_ASSIGN_AND_EQ = JC_BINARY_BIT_AND | JC_ASSIGN_EQUAL,
	JC_ASSIGN_OR_EQ = JC_BINARY_BIT_OR | JC_ASSIGN_EQUAL,
	JC_ASSIGN_XOR_EQ = JC_BINARY_BIT_XOR | JC_ASSIGN_EQUAL
}JC_ASSIGN_OPERATOR;

jc_void InitializeExpress(CJcSymbolStack* pStack, CJcExpress* pExp);
jc_void DestroyExpress(CJcExpress* pExp);

jc_void AllocTempVar(CJcExpress *pExp);
jc_void FreeTempVar(CJcExpress *pExp);

jc_void UpdateExpressType(CJcExpress *pExp, CJcTypeInfo* pType);

jc_bool CheckCondExpress(CJcExpress *pExp);
jc_void CheckSwitchExpress(CJcExpress *pExp);
jc_void CheckCaseExpress(CJcExpress *pExp);
jc_uint GetConstValue(CJcExpress *pExp, CJcVal* pVal, jc_uint nDstType, jc_uint bAllowFloat);
jc_uint GetEnumValue(CJcExpress *pExp, jc_int* pEnumValue);

CJcExpress BuildCharConstantExpress(CJcSymbolStack* pStack, jc_char* pVal);
CJcExpress BuildStringConstantExpress(CJcSymbolStack* pStack, jc_char* pVal);
CJcExpress BuildIntegerConstantExpress(CJcSymbolStack* pStack, jc_char* pVal);
CJcExpress BuildIntegerConstantExpressA(CJcSymbolStack* pStack, jc_uint nVal, jc_bool bSigned);
CJcExpress BuildLongConstantExpressA(CJcSymbolStack* pStack, jc_ulong nVal, jc_bool bSigned);
CJcExpress BuildFloatConstantExpress(CJcSymbolStack* pStack, jc_char* pVal);
CJcExpress BuildSymbolExpress(CJcSymbolStack* pStack, CJcSymbol* pSymbol);
CJcExpress BuildSizeExpress(CJcSymbolStack* pStack, CJcTypeInfo* pType, jc_bool bSizeOf);

CJcExpress LoadExpress(CJcExpress* pExp);
jc_void SaveExpress(CJcExpress* pLeftExp, CJcExpress *pRightExp);
CJcExpress CopyExpress(CJcExpress* pExp);
jc_void ZeroBitExpress(CJcExpress *pExp);
jc_void LoadBitExpress(CJcExpress *pExp);
jc_void BuildBitExpress(CJcExpress *pExp, jc_uint nBits, jc_int nStart);

jc_void ReturnExpress(CJcExpress *pExp);
jc_void EmitCondJmp(CJcExpress *pExp, jc_bool bTrue, jc_uint nJmpAddr, jc_uint nInsAddr);
jc_void EmitSwitchJmp(CJcExpress *pExp, jc_uint nJmpAddr, jc_uint nInsAddr);

jc_void BuildFieldExpress(CJcExpress* pStruct, jc_char* sField, jc_bool bIsPointer);
jc_void BuildArrayExpress(CJcExpress* pArray, CJcExpress* pDim);
jc_void BuildArgumentExpress(CJcExpress* pFunction, CJcExpress* pArg, jc_uint nArgIdx, jc_uint* npArgSize);
jc_void BuildCallExpress(CJcExpress* pFunction, jc_uint nArgSize, jc_uint nArgInsAddr);
jc_void BuildIncreaseExpress(CJcExpress *pExp, jc_bool bReturnNewVal, jc_bool bIncrearse);
jc_void BuildConditionalExpress(CJcExpress *pCondExp, CJcExpress *pIfExp, CJcExpress *pElseExp);

jc_void BuildUnaryExpress(CJcExpress *pExp, jc_uint nUnaryOperator);
jc_void BuildCastExpress(CJcExpress *pExp, CJcTypeInfo* pTargetType);
jc_void BuildBinaryExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nBinaryOperator);
jc_void BuildAssignmentExpress(CJcExpress *pLeftExp, CJcExpress *pRightExp, jc_uint nAssignOperator, jc_uint nCheck);

#endif
