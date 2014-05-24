
#ifndef _jc_symbol_h_
#define _jc_symbol_h_

#include "jc_string.h"
#include "jc_function.h"
#include "jc_typeinfo.h"
#include "jc_variable.h"
#include "jc_hash.h"
#include "jc_dataseg.h"
#include "jc_stack.h"

typedef enum JC_SYMBOL
{
	JC_TYPE_SYMBOL,
	JC_VARIABLE_SYMBOL,
	JC_FUNCTION_SYMBOL,
	JC_ENUM_SYMBOL,
}JC_SYMBOL;

typedef struct CJcSymbol
{
	jc_uint nStorage;
	jc_uint nSymbol;
	union sym_info_t
	{
		CJcTypeInfo * pType;
		CJcFunctionInfo * pFunction;
		CJcVariableInfo* pVariable;
	}info;
}CJcSymbol;

typedef struct CJcSymbolTable
{
	struct CJcSymbolTable* pPrev;
	CJcFunctionInfo* pFunction;
	CJcHashTable* pSymbolTable; /*<oName, CJcSymbol>*/
}CJcSymbolTable;

typedef struct CJcProtoType
{
	jc_uint nUsed;
	jc_uint nNewIdx;
	CJcString oName;
	CJcSymbol oSymbol;
}CJcProtoType;

typedef struct CJcProtoTypeTable
{
	jc_uint nCount;
	CJcProtoType* pProtoTypeTable;
}CJcProtoTypeTable;

typedef struct CJcSymbolStack CJcSymbolStack;
typedef struct CJcParser CJcParser;
typedef struct CJcSymbolStack
{
	jc_uint nLevel;
	CJcParser* pParser;
	CJcSymbolTable* pSymbolTable;
	CJcFunctionInfo* pInFunction;
	CJcProtoTypeTable oProtoTypeTable;
	CJcSegment oCodeSegment;
	CJcSegment oConstSegment;
	CJcConstTable oConstTable;
	CJcLocalStack oDataSegment;
	CJcLocalStack oGlobalStack;
	CJcLocalStack oLocalStack;
}CJcSymbolStack;

jc_void InitializeProtoTypeTable(CJcProtoTypeTable* pTable);
jc_void ClearProtoTypeTable(CJcProtoTypeTable* pTable);

jc_void InitializeSymbolStack(CJcSymbolStack*pStack);
jc_void ClearSymbolStack(CJcSymbolStack*pStack);

CJcSymbolStack* CreateSymbolStack();

jc_void DestroySymbolStack(CJcSymbolStack* pStack);

jc_void NewSymbolTable(CJcSymbolStack* pStack);
jc_void PopSymbolTable(CJcSymbolStack* pStack);
jc_uint GetSymbolTableLevel(CJcSymbolStack* pStack);

jc_void SetFunction(CJcSymbolStack* pStack, 
					CJcFunctionInfo* pFunction);
CJcFunctionInfo* GetFunction(CJcSymbolStack* pStack);

jc_uint FindSymbol(CJcSymbolStack* pStack,
				   jc_char* sSymbolName,
				   jc_uint bFindAll,
				   CJcSymbol * pSymbol);

CJcSymbol* CreateEnumSymbol(CJcSymbolStack* pStack, jc_char* sEnumName, CJcTypeInfo *pType, jc_int nVal);

CJcSymbol* CreateTypeSymbol(CJcSymbolStack* pStack,
							jc_char* sTypeName,
							CJcTypeInfo *pType);

CJcSymbol* CreateFunctionSymbol(CJcSymbolStack* pStack,
								jc_uint nStorage,
								jc_char* sFunctionName,
								CJcFunctionInfo* pFunction);

CJcSymbol* CreateVariableSymbol(CJcSymbolStack* pStack,
								jc_uint nStorage,
								jc_char* sVariableName,
								CJcTypeInfo *pType);
jc_void ReAllocateVariableSpace(CJcSymbolStack* pStack, CJcSymbol* pSymbol, jc_uint nSpace);

CJcSymbol* CreateSymbol(CJcSymbolStack* pStack,
						jc_uint nStorage,
						jc_char* sName,
						CJcTypeInfo *pType);
jc_void ClearSymbol(CJcSymbol* pSymbol);
jc_void DestroySymbol(CJcSymbol* pSymbol);

jc_void FunctionDefine(CJcSymbolStack* pStack,
					   CJcSymbol* pSymbol,
					   jc_uint nFuncAddr);

jc_uint GetProtoTypeCount(CJcSymbolStack* pStack);

CJcProtoType* GetProtoType(CJcSymbolStack* pStack,
						   jc_uint nIdx);

CJcProtoType* FindProtoType(CJcSymbolStack* pStack,
						   jc_char* sSymbolName,
						   jc_uint* pIdx);

jc_bool IsExpressSymbol(CJcSymbolStack* pStack, jc_bool IsId, jc_char* pVal, CJcSymbol* pSymbol);
jc_bool IsTypeSpecifier(CJcSymbolStack* pStack, jc_int kind, jc_char* v);
jc_bool NeedReturnValue(CJcSymbolStack* pStack);

#endif
