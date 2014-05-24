
#ifndef _jc_stack_h_
#define _jc_stack_h_

#include "jc_type.h"

typedef struct CJcVariableInStack
{
	jc_uint nUsed;
	jc_uint nDataLen;
	jc_uint nAlign;
	jc_uint nPosition;
	struct CJcVariableInStack* pNext;
}CJcVariableInStack;

typedef struct CJcVariableTable
{
	struct CJcVariableTable* pPrev;
	jc_uint nStartPosition;
	CJcVariableInStack *pHead, *pTail;
}CJcVariableTable;

typedef struct CJcLocalStack
{
	jc_uint nPos;
	jc_uint nSize;
	CJcVariableTable * pVariableTable;
}CJcLocalStack;

jc_void InitializeLocalStack(CJcLocalStack* pStack);
jc_void ClearLocalStack(CJcLocalStack* pStack);

jc_uint GetSizeOfStack(CJcLocalStack* pStack);
jc_uint GetPosOfStack(CJcLocalStack* pStack);
jc_void SetPosOfStack(CJcLocalStack* pStack, jc_uint nPos);

jc_void NewVariableTable(CJcLocalStack* pStack);
jc_void PopVariableTable(CJcLocalStack* pStack);

jc_uint AllocVarInStack(CJcLocalStack* pStack, jc_uint nDataLen, 
				jc_uint nAlign, jc_uint* pVarId);
jc_void FreeVarFromStack(CJcLocalStack* pStack, jc_uint nVarId);

#endif
