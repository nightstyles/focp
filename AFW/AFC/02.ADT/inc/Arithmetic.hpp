
#include "String.hpp"
#include "Binary.hpp"

#ifndef _ADT_ARITHMETIC_HPP_
#define _ADT_ARITHMETIC_HPP_

FOCP_BEGIN();

template<typename TKey> struct CHashArithmetic
{
	inline static uint32 GetHashValue(const TKey* pKey)
	{
		return 0;
	}
};

#define FOCP_BASE_DIGIT_HASH(TKey) \
template<> struct CHashArithmetic<TKey> \
{\
	inline static uint32 GetHashValue(const TKey* pKey)\
	{ \
		CUnion<uint32, TKey> oConvert;\
		oConvert.oVal1 = 0; \
		oConvert.oVal2 = *pKey; \
		return oConvert.oVal1; \
	} \
}

FOCP_BASE_DIGIT_HASH(int8);
FOCP_BASE_DIGIT_HASH(int16);
FOCP_BASE_DIGIT_HASH(int32);
FOCP_BASE_DIGIT_HASH(float);
FOCP_BASE_DIGIT_HASH(uint8);
FOCP_BASE_DIGIT_HASH(uint16);
FOCP_BASE_DIGIT_HASH(uint32);
#undef FOCP_BASE_DIGIT_HASH

#define FOCP_BASE_DIGIT_CRC_HASH(TKey) \
template<> struct CHashArithmetic<TKey> \
{ \
	inline static uint32 GetHashValue(const TKey* pKey)\
	{ \
		return GetCrc32((const uint8*)pKey, sizeof(TKey), 1); \
	} \
}
FOCP_BASE_DIGIT_CRC_HASH(int64);
FOCP_BASE_DIGIT_CRC_HASH(uint64);
FOCP_BASE_DIGIT_CRC_HASH(double);
#undef FOCP_BASE_DIGIT_CRC_HASH

template<> struct CHashArithmetic<CString>
{
	inline static uint32 GetHashValue(const CString* pKey)
	{
		return GetCrc32((const uint8*)pKey->GetStr(), pKey->GetSize(), 1);
	}
};

template<> struct CHashArithmetic<CBinary>
{
	inline static uint32 GetHashValue(const CBinary* pKey)
	{
		return GetCrc32((const uint8*)pKey->GetData(), pKey->GetSize(), 1);
	}
};

struct CNameHashArithmetic
{
	inline static uint32 GetHashValue(const CString* pKey)
	{
		return GetCrc32((const uint8*)pKey->GetStr(), pKey->GetSize(), 0);
	}
};

struct CRawHashArithmetic
{
	inline static uint32 GetHashValue(const CBinary* pKey)
	{
		return GetCrc32((const uint8*)pKey->GetData(), pKey->GetSize(), 0);
	}
};

template<typename TKey> struct CCompareKey
{
	inline static int32 Compare(const TKey* pLeft, const TKey* pRight)
	{
		if( (*pLeft) > (*pRight) )
			return 1;
		if( (*pLeft) < (*pRight) )
			return -1;
		return 0;
	}
};

template<> struct CCompareKey<CString>
{
	inline static int32 Compare(const CString* pLeft, const CString* pRight)
	{
		return pLeft->Compare(*pRight);
	}
};

template<> struct CCompareKey<CBinary>
{
	inline static int32 Compare(const CBinary* pLeft, const CBinary* pRight)
	{
		return pLeft->Compare(*pRight);
	}
};

struct ADT_API CNameCompare
{
	inline static int32 Compare(const CString* pLeft, const CString* pRight)
	{
		return pLeft->Compare(*pRight, false);
	}
};

template<typename TKey, typename TData> struct CGetKey
{
	inline static const TKey* GetKey(const TData& oData)
	{
		return (const TKey*)(const void*)&oData;
	}
};

template<typename TKey, typename TData> struct CMapNode
{
	TKey oKey;
	TData oData;

	inline CMapNode()
	{
	}
	inline CMapNode(const TKey& oKey1, const TData &oData1=TData())
		:oKey(oKey1), oData(oData1)
	{
	}
};

template<typename TKey, typename TData> struct CGetMapKey: public CGetKey<TKey, CMapNode<TKey, TData> >
{
	inline static const TKey* GetKey(const CMapNode<TKey, TData>& oData)
	{
		return &oData.oKey;
	}
};

struct CRbTreeNode
{
	CRbTreeNode* pParent;
	CRbTreeNode* pLeft;
	CRbTreeNode* pRight;
	bool bIsBlack;
};

struct CHashIterator
{
	uint32 nBucket;
	void* pNode;
};

FOCP_END();

#endif //_ADT_ARITHMETIC_HPP_
