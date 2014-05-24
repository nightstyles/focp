
#include "EhcBase.h"

#ifndef _EHC_VEC_H_
#define _EHC_VEC_H_

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

typedef struct CEhcVector
{
	ehc_uint nSize;
	ehc_void** pTable;
}CEhcVector;

typedef ehc_void (*FOnDestroyVectorItem)(ehc_void* pItem);
EBS_API ehc_void InitializeVector(CEhcVector* pVector);
EBS_API ehc_void ClearVector(CEhcVector* pVector, FOnDestroyVectorItem fOnDestroy);
EBS_API ehc_void Push(CEhcVector* pVector, ehc_void* pItem);
EBS_API ehc_void* Pop(CEhcVector* pVector);
EBS_API ehc_uint Remove(CEhcVector* pVector, ehc_uint Idx, FOnDestroyVectorItem fOnDestroy);

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
