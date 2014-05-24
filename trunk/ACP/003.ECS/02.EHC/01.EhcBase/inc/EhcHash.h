
#include "EhcBase.h"

#ifndef _EHC_HASH_H_
#define _EHC_HASH_H_

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

/*****************************************************************************/
typedef ehc_uint (*FHashFunction)(ehc_void *pKey);
typedef ehc_int (*FHashKeyEqual)(ehc_void *pKey1, ehc_void *pKey2);
typedef ehc_void* (*FGetHashKey)(ehc_void* pVal);
typedef ehc_void (*FFreeHashKey)(ehc_void* pKey);
typedef ehc_void (*FFreeHashVal)(ehc_void* pVal);

/*****************************************************************************/
typedef struct CEhcHashEntry
{
    ehc_void *pKey, *pVal;
    ehc_uint nHashValue;
    struct CEhcHashEntry *pNext;
}CEhcHashEntry;

typedef struct CEhcHashTable
{
    ehc_uint nTableLength;
    CEhcHashEntry **pBuckets;
    ehc_uint BucketSize;
    ehc_uint nLoadLimit;
    ehc_uint nPrimeIndex;
	FHashFunction HashFunc;
	FHashKeyEqual HashKeyEqual;
	FFreeHashKey FreeKey;
	FFreeHashVal FreeValue;
	FGetHashKey GetKey;
}CEhcHashTable;

/*****************************************************************************/
EBS_API ehc_uint GetHashValue(CEhcHashTable *pHashTable, ehc_void *pKey);
EBS_API ehc_uint GetCrc32Hash(ehc_void* pKey, ehc_uint nKeyLen);

/*****************************************************************************
 * create or destroy hashtable
*****************************************************************************/
EBS_API CEhcHashTable * CreateHashTable(FGetHashKey GetKey,
										FHashFunction HashFunc,
										FHashKeyEqual HashKeyEqual,
										FFreeHashKey FreeKey,
										FFreeHashVal FreeValue);

EBS_API ehc_void DestroyHashTable(CEhcHashTable *pHashTable);

/*****************************************************************************
 * InsertIntoHashTable:返回非0成功
*****************************************************************************/
EBS_API ehc_void InsertIntoHashTable(CEhcHashTable *pHashTable, ehc_void *pKey, ehc_void *pVal);

/*****************************************************************************
 * SearchInHashTable
*****************************************************************************/
EBS_API ehc_void * SearchInHashTable(CEhcHashTable *pHashTable, ehc_void *pKey);

/*****************************************************************************
 * RemoveFromHashTable
*****************************************************************************/
EBS_API ehc_void * RemoveFromHashTable(CEhcHashTable *pHashTable, ehc_void *pKey);

/*****************************************************************************
 * CountOfHashTable
*****************************************************************************/
EBS_API ehc_uint CountOfHashTable(CEhcHashTable *pHashTable);

/*****************************************************************************
* BeginOfHashTable & NextOfHashTable
*****************************************************************************/
EBS_API CEhcHashEntry* BeginOfHashTable(CEhcHashTable *pHashTable);
EBS_API CEhcHashEntry* NextOfHashTable(CEhcHashTable *pHashTable, CEhcHashEntry* it);

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
