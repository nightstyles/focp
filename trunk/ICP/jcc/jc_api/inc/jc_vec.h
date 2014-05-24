
#ifndef _jc_vec_h_
#define _jc_vec_h_

#include "jc_type.h"

typedef struct CJcVector
{
	jc_uint nSize;
	jc_void** pTable;
}CJcVector;

typedef jc_void (JC_CALL *FOnDestroyVectorItem)(jc_void* pItem);
jc_void InitializeVector(CJcVector* pVector);
jc_void ClearVector(CJcVector* pVector, FOnDestroyVectorItem fOnDestroy);
jc_void Push(CJcVector* pVector, jc_void* pItem);
jc_void* Pop(CJcVector* pVector);
jc_uint Remove(CJcVector* pVector, jc_uint Idx, FOnDestroyVectorItem fOnDestroy);

#endif
