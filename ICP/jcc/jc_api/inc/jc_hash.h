
#ifndef _jc_hash_h_
#define _jc_hash_h_

#include "jc_type.h"

/*****************************************************************************/
typedef jc_uint (JC_CALL *FHashFunction)(jc_void *pKey);
typedef jc_int (JC_CALL *FHashKeyEqual)(jc_void *pKey1, jc_void *pKey2);
typedef jc_void* (JC_CALL *FGetHashKey)(jc_void* pVal);
typedef jc_void (JC_CALL *FFreeHashKey)(jc_void* pKey);
typedef jc_void (JC_CALL *FFreeHashVal)(jc_void* pVal);

/*****************************************************************************/
typedef struct CJcHashEntry
{
    jc_void *pKey, *pVal;
    jc_uint nHashValue;
    struct CJcHashEntry *pNext;
}CJcHashEntry;

typedef struct CJcHashTable
{
    jc_uint nTableLength;
    CJcHashEntry **pBuckets;
    jc_uint BucketSize;
    jc_uint nLoadLimit;
    jc_uint nPrimeIndex;
	FHashFunction HashFunc;
	FHashKeyEqual HashKeyEqual;
	FFreeHashKey FreeKey;
	FFreeHashVal FreeValue;
	FGetHashKey GetKey;
}CJcHashTable;

/*****************************************************************************/
jc_uint GetHashValue(CJcHashTable *pHashTable, jc_void *pKey);
jc_uint GetCrc32Hash(jc_void* pKey, jc_uint nKeyLen);

/*****************************************************************************
 * create or destroy hashtable
*****************************************************************************/
CJcHashTable * CreateHashTable(FGetHashKey GetKey,
							   FHashFunction HashFunc,
							   FHashKeyEqual HashKeyEqual,
							   FFreeHashKey FreeKey,
							   FFreeHashVal FreeValue);

jc_void DestroyHashTable(CJcHashTable *pHashTable);

/*****************************************************************************
 * InsertIntoHashTable:返回非0成功
*****************************************************************************/
jc_void InsertIntoHashTable(CJcHashTable *pHashTable, jc_void *pKey, jc_void *pVal);

/*****************************************************************************
 * SearchInHashTable
*****************************************************************************/
jc_void * SearchInHashTable(CJcHashTable *pHashTable, jc_void *pKey);

/*****************************************************************************
 * RemoveFromHashTable
*****************************************************************************/
jc_void * RemoveFromHashTable(CJcHashTable *pHashTable, jc_void *pKey);

/*****************************************************************************
 * CountOfHashTable
*****************************************************************************/
jc_uint CountOfHashTable(CJcHashTable *pHashTable);

/*****************************************************************************
* BeginOfHashTable & NextOfHashTable
*****************************************************************************/
CJcHashEntry* BeginOfHashTable(CJcHashTable *pHashTable);
CJcHashEntry* NextOfHashTable(CJcHashTable *pHashTable, CJcHashEntry* it);

#endif
