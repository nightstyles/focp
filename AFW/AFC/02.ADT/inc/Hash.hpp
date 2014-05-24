
#include "Arithmetic.hpp"
#include "List.hpp"

#ifndef _AFT_HASH_HPP_
#define _AFT_HASH_HPP_

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

template<typename TData> class CHashList: public CDoubleList<TData>
{
public:
	inline void DetachNode(void* pNode)
	{
		RemoveNode((CDoubleListDataNode<TData>*)pNode);
	}

	inline void AttachNode(void* pNode)
	{
		InsertNode(NULL, (CDoubleListDataNode<TData>*)pNode);
	}

	inline void FreeNode(void* pNode)
	{
		delete (CDoubleListDataNode<TData>*)pNode;
	}
};

FOCP_DETAIL_END();

template
<
	typename TKey, 
	typename TData=TKey,
	typename TGetKey=CGetKey<TKey, TData>,
	typename TCompareKey=CCompareKey<TKey>,
	typename THashArithmetic=CHashArithmetic<TKey>
> class CHash
{
private:
	uint32 m_nBucketSize;
	FOCP_DETAIL_NAME::CHashList<TData>* m_pBuckets;
	uint32 m_nSize;
	bool m_bUnique;

public:
	inline ~CHash()
	{
		Clear();
		delete[] m_pBuckets;
		m_nBucketSize = 0;
		m_pBuckets = NULL;
	}

	inline CHash(uint32 nBucketSize=1024, bool bUnique=true)
	{
		if(nBucketSize == 0)
			nBucketSize = 1024;
		m_nBucketSize = nBucketSize;
		m_bUnique = bUnique;
		m_nSize = 0;
		m_pBuckets = new FOCP_DETAIL_NAME::CHashList<TData>[m_nBucketSize];
	}

	inline CHash(const CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> &oSrc)
	{
		m_nSize = 0;
		m_nBucketSize = oSrc.m_nBucketSize;
		m_bUnique = oSrc.m_bUnique;
		m_pBuckets = new FOCP_DETAIL_NAME::CHashList<TData>[m_nBucketSize];
		Insert(oSrc);
	}

	inline CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic>& operator=(const CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> &oSrc)
	{
		if(this != &oSrc)
		{
			CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> o(oSrc);
			Swap(o);
		}
		return *this;
	}
	
	inline void Clear()
	{
		uint32 i;
		for(i=0; i<m_nBucketSize; i++)
			m_pBuckets[i].Clear();
		m_nSize = 0;
	}

	inline uint32 GetSize() const
	{
		return m_nSize;
	}

	inline bool Unique() const
	{
		return m_bUnique;
	}

	inline bool IteratorEqual(const CHashIterator& oLeft, const CHashIterator &oRight) const
	{
		return ((oLeft.pNode == oRight.pNode) && (oLeft.nBucket == oRight.nBucket));
	}

	inline CHashIterator First() const
	{
		CHashIterator oIt;
		oIt.pNode = NULL;
		oIt.nBucket = m_nBucketSize;
		if(m_nSize)
		{
			oIt.nBucket=0;
			while(true)
			{
				if(m_pBuckets[oIt.nBucket].GetSize())
				{
					oIt.pNode = m_pBuckets[oIt.nBucket].First();
					break;
				}
				oIt.nBucket++;
			}
		}
		return oIt;
	}

	inline CHashIterator Last() const
	{
		CHashIterator oIt;
		oIt.pNode = NULL;
		oIt.nBucket = m_nBucketSize;
		if(m_nSize)
		{
			--oIt.nBucket;
			while(true)
			{
				if(m_pBuckets[oIt.nBucket].GetSize())
				{
					oIt.pNode = m_pBuckets[oIt.nBucket].Last();
					break;
				}
				--oIt.nBucket;
			}
		}
		return oIt;
	}

	inline CHashIterator End() const
	{
		CHashIterator oIt;
		oIt.pNode = NULL;
		oIt.nBucket = m_nBucketSize;
		return oIt;
	}

	inline const TData* At(const CHashIterator& oIt) const
	{
		if(oIt.pNode && oIt.nBucket < m_nBucketSize)
			return m_pBuckets[oIt.nBucket].At(oIt.pNode);
		return NULL;
	}

	inline const TData& GetItem(const CHashIterator& oIt) const
	{
		return *At(oIt);
	}

	inline CHashIterator GetNext(const CHashIterator& oSrc) const
	{
		CHashIterator oIt = oSrc;
		if(oIt.pNode && oIt.nBucket < m_nBucketSize)
		{
			oIt.pNode = m_pBuckets[oIt.nBucket].GetNext(oIt.pNode);
			if(oIt.pNode == NULL)
			{
				for(++oIt.nBucket; oIt.nBucket<m_nBucketSize; oIt.nBucket++)
				{
					if(m_pBuckets[oIt.nBucket].GetSize())
					{
						oIt.pNode = m_pBuckets[oIt.nBucket].First();
						break;
					}
				}
			}
		}
		else
		{
			oIt.nBucket = m_nBucketSize;
			oIt.pNode = NULL;
		}
		return oIt;
	}

	inline CHashIterator GetPrev(const CHashIterator& oSrc) const
	{
		if(oSrc.pNode == NULL || oSrc.nBucket >= m_nBucketSize)
			return Last();
		CHashIterator oIt = oSrc;
		oIt.pNode = m_pBuckets[oIt.nBucket].GetPrev(oIt.pNode);
		if(oIt.pNode == NULL)
		{
			while(oIt.nBucket)
			{
				--oIt.nBucket;
				if(m_pBuckets[oIt.nBucket].GetSize())
				{
					oIt.pNode = m_pBuckets[oIt.nBucket].Last();
					break;
				}
			}
			if(oIt.nBucket == 0)
				oIt.nBucket = m_nBucketSize;
		}
		return oIt;
	}

	inline CHashIterator Find(const TKey& oKey) const
	{
		CHashIterator oIt;
		uint32 nHashValue = THashArithmetic::GetHashValue(&oKey);
		nHashValue %= m_nBucketSize;
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[nHashValue];
		oIt.pNode = oList.First();
		oIt.nBucket = m_nBucketSize;
		while(oIt.pNode)
		{
			const TKey* pKey = TGetKey::GetKey(oList.GetItem(oIt.pNode));
			if(TCompareKey::Compare(&oKey, pKey) == 0)
			{
				oIt.nBucket = nHashValue;
				break;
			}
			oIt.pNode = oList.GetNext(oIt.pNode);
		}
		return oIt;
	}

	inline CHashIterator FindNext(const CHashIterator& oPrev) const
	{
		CHashIterator oIt;
		void* pNode = oPrev.pNode;
		if(oPrev.nBucket >= m_nBucketSize || !pNode || m_bUnique)
			return End();
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[oPrev.nBucket];
		const TKey* pKey1 = TGetKey::GetKey(oList.GetItem(pNode));
		oIt.pNode = oList.GetNext(pNode);
		oIt.nBucket = m_nBucketSize;
		while(oIt.pNode)
		{
			const TKey* pKey2 = TGetKey::GetKey(oList.GetItem(oIt.pNode));
			if(TCompareKey::Compare(pKey1, pKey2) == 0)
			{
				oIt.nBucket = oPrev.nBucket;
				break;
			}
			oIt.pNode = oList.GetNext(oIt.pNode);
		}
		return oIt;
	}

	inline CHashIterator Insert(const TData &oSrc)
	{
		const TKey* pKey = TGetKey::GetKey(oSrc);
		CHashIterator oIt = Find(*pKey), oEnd=End();
		if(!IteratorEqual(oIt, oEnd) && m_bUnique)
			return oEnd;
		uint32 nHashValue = THashArithmetic::GetHashValue(pKey);
		nHashValue %= m_nBucketSize;
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[nHashValue];
		oIt.pNode = oList.Push(oSrc);
		oIt.nBucket = nHashValue;
		++m_nSize;
		return oIt;
	}

	inline void Update(const TData &oSrc, bool bUpdateAll=true)
	{
		const TKey* pKey1 = TGetKey::GetKey(oSrc);
		uint32 nHashValue = THashArithmetic::GetHashValue(pKey1);
		nHashValue %= m_nBucketSize;
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[nHashValue];
		void* pNode = oList.First();
		while(pNode)
		{
			const TKey* pKey2 = TGetKey::GetKey(oList.GetItem(pNode));
			if(TCompareKey::Compare(pKey1, pKey2) == 0)
			{
				oList.GetItem(pNode) = oSrc;
				if(m_bUnique || !bUpdateAll)
					break;
			}
			pNode = oList.GetNext(pNode);
		}
	}

	inline void MoveFrom(CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> &oSrc, bool bDiscardConflictItem=true)
	{
		if(this != &oSrc)
		{
			uint32 i, nSrcBucketSize = oSrc.m_nBucketSize;
			for(i=0; i<nSrcBucketSize; ++i)
			{
				FOCP_DETAIL_NAME::CHashList<TData>& oList = oSrc.m_pBuckets[i];
				void* pNode = oList.First();
				while(pNode)
				{
					void* pNext = oList.GetNext(pNode);
					TData& oData = oList.GetItem(pNode);
					const TKey* pKey = TGetKey::GetKey(oData);
					if(!m_bUnique || Find(*pKey).pNode==NULL)
					{
						uint32 nHashValue = THashArithmetic::GetHashValue(pKey);
						nHashValue %= m_nBucketSize;
						FOCP_DETAIL_NAME::CHashList<TData>&oDstList = m_pBuckets[nHashValue];
						oList.DetachNode(pNode);
						oDstList.AttachNode(pNode);
					}
					else if(bDiscardConflictItem)
					{
						oList.DetachNode(pNode);
						oList.FreeNode(pNode);
					}
					pNode = pNext;
				}
			}
		}
	}

	//拷贝oSrc中的元素进来。
	inline void Insert(const CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> &oSrc)
	{
		if(this == &oSrc)
		{
			if(!m_bUnique)
			{
				uint32 i;
				CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> o(oSrc);
				for(i=0; i<m_nBucketSize; ++i)
					m_pBuckets[i].Append(NULL, o.m_pBuckets[i]);
			}
		}
		else
		{
			CHashIterator oIt=oSrc.First(), oEnd=oSrc.End();
			for(; !oSrc.IteratorEqual(oIt, oEnd);  oIt=oSrc.GetNext(oIt))
				Insert(oSrc.GetItem(oIt));
		}
	}

	inline CHashIterator Remove(const CHashIterator& oSrc)
	{
		if(oSrc.nBucket >= m_nBucketSize || !oSrc.pNode)
			return End();
		CHashIterator oIt = GetNext(oSrc);
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[oSrc.nBucket];
		oList.Remove(oSrc.pNode);
		--m_nSize;
		return oIt;
	}

	inline void Remove(const TKey& oKey, bool bRemoveAll=true)
	{
		uint32 nHashValue = THashArithmetic::GetHashValue(&oKey);
		nHashValue %= m_nBucketSize;
		FOCP_DETAIL_NAME::CHashList<TData> &oList = m_pBuckets[nHashValue];
		void* pNode = oList.First();
		while(pNode)
		{
			void* pNext = oList.GetNext(pNode);
			const TKey* pKey = TGetKey::GetKey(oList.GetItem(pNode));
			if(TCompareKey::Compare(&oKey, pKey) == 0)
			{
				oList.Remove(pNode);
				--m_nSize;
				if(m_bUnique || !bRemoveAll)
					break;
			}
			pNode = pNext;
		}
	}

	inline void Swap(CHash<TKey, TData, TGetKey, TCompareKey, THashArithmetic> &oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_nBucketSize, oSrc.m_nBucketSize);
			::FOCP_NAME::Swap(m_pBuckets, oSrc.m_pBuckets);
			::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
			::FOCP_NAME::Swap(m_bUnique, oSrc.m_bUnique);
		}
	}
};

template
<
	typename TKey, 
	typename TData, 
	typename TCompareKey=CCompareKey<TKey>,
	typename THashArithmetic=CHashArithmetic<TKey>
>class CHashMap
{
public:
	typedef CHash<TKey, CMapNode<TKey, TData>, CGetMapKey<TKey, TData>, TCompareKey, THashArithmetic> THash;

private:
	THash m_oHash;

public:
	inline ~CHashMap()
	{
	}

	inline CHashMap(uint32 nBucketSize=1024, bool bUnique=true)
		:m_oHash(nBucketSize, bUnique)
	{
	}

	inline CHashMap(const CHashMap<TKey, TData, TCompareKey, THashArithmetic> &oSrc)
		:m_oHash(oSrc.m_oHash)
	{
	}

	inline CHashMap<TKey, TData, TCompareKey, THashArithmetic>& operator=(const CHashMap<TKey, TData, TCompareKey, THashArithmetic> &oSrc)
	{
		m_oHash = oSrc.m_oHash;
		return *this;
	}

	inline void Clear()
	{
		m_oHash.Clear();
	}

	inline bool Unique() const
	{
		return m_oHash.Unique();
	}

	inline uint32 GetSize() const
	{
		return m_oHash.GetSize();
	}

	inline bool IteratorEqual(const CHashIterator& oLeft, const CHashIterator &oRight) const
	{
		return m_oHash.IteratorEqual(oLeft, oRight);
	}

	inline CHashIterator First() const
	{
		return m_oHash.First();
	}

	inline CHashIterator Last() const
	{
		return m_oHash.Last();
	}

	inline CHashIterator End() const
	{
		return m_oHash.End();
	}

	inline CHashIterator GetNext(const CHashIterator& oSrc) const
	{
		return m_oHash.GetNext(oSrc);
	}

	inline CHashIterator GetPrev(const CHashIterator& oSrc) const
	{
		return m_oHash.GetPrev(oSrc);
	}

	inline CHashIterator Find(const TKey& oKey) const
	{
		return m_oHash.Find(oKey);
	}

	inline CHashIterator FindNext(const CHashIterator& oPrev) const
	{
		return m_oHash.FindNext(oPrev);
	}

	inline const TData* At(const CHashIterator& oIt) const
	{
		const CMapNode<TKey, TData>* pNode = m_oHash.At(oIt);
		if(pNode == NULL)
			return NULL;
		return &pNode->oData;
	}

	inline const TData& GetItem(const CHashIterator& oIt) const
	{
		return m_oHash.At(oIt)->oData;
	}

	inline TData* At(CHashIterator& oIt)
	{
		CMapNode<TKey, TData>* pNode = (CMapNode<TKey, TData>*)m_oHash.At(oIt);
		if(pNode == NULL)
			return NULL;
		return &pNode->oData;
	}

	inline TData& GetItem(CHashIterator& oIt)
	{
		return (TData&)(m_oHash.At(oIt)->oData);
	}

	inline const TKey& GetKey(const CHashIterator& oIt) const
	{
		return m_oHash.At(oIt)->oKey;
	}

	inline CHashIterator Insert(const TKey& oKey, const TData &oSrc)
	{
		CMapNode<TKey, TData> oNode(oKey, oSrc);
		return m_oHash.Insert(oNode);
	}

	inline void Update(const TKey& oKey, const TData &oSrc, bool bUpdateAll=true)
	{
		CMapNode<TKey, TData> oNode(oKey, oSrc);
		m_oHash.Update(oNode, bUpdateAll);
	}

	inline void Insert(const CHashMap<TKey, TData, TCompareKey, THashArithmetic> &oSrc)
	{
		m_oHash.Insert(oSrc.m_oHash);
	}

	inline void MoveFrom(CHashMap<TKey, TData, TCompareKey, THashArithmetic> &oSrc, bool bDiscardConflictItem=true)
	{
		m_oHash.MoveFrom(oSrc.m_oHash, bDiscardConflictItem);
	}

	inline CHashIterator Remove(const CHashIterator& oSrc)
	{
		return m_oHash.Remove(oSrc);
	}

	inline void Remove(const TKey& oKey, bool bRemoveAll=true)
	{
		m_oHash.Remove(oKey, bRemoveAll);
	}

	inline TData& operator[](const TKey& oKey)
	{
		CHashIterator oIt = Find(oKey);
		CMapNode<TKey, TData>* pNode = m_oHash.At(oIt);
		if(pNode == NULL)
		{
			CMapNode<TKey, TData> oNode(oKey);
			oIt = m_oHash.Insert(oNode);
			pNode = m_oHash.At(oIt);
		}
		return pNode->oData;
	}

	inline const TData& operator[](const TKey& oKey) const
	{
		CHashIterator oIt = Find(oKey);
		return *At(oIt);
	}

	inline void Swap(CHashMap<TKey, TData, TCompareKey, THashArithmetic> &oSrc)
	{
		m_oHash.Swap(oSrc.m_oHash);
	}
};

template<typename TObject> class CFreeHash
{
public:
	struct CHashNode
	{
		CHashNode* pNextNode;
		uint32 nHashValue;
		TObject oObject;
	};
	struct CHashBranch
	{
		CHashBranch* pBranch[256];
		uint32 nSize;
	};
	typedef bool (*FIsEqual0)(void* pCond, const TObject& oObject);
	typedef bool (*FIsEqual1)(const TObject& a, const TObject& b, void* pContext);

private:
	CHashBranch* m_pTop;
	uint32 m_nUnique;
	uint32 m_nSize;
	uint32 m_nHashSize;
	uint32 m_nHashLevel;
	bool m_nBigEndian, m_bNeedExtend;
	double m_nHashRate;

public:
	CFreeHash(uint32 nUnique, double nHashRate=1.0)
	{
		if(nHashRate<=0.0)
			nHashRate = 0.3;
		if(nHashRate > 1.0)
			nHashRate = 1.0;
		m_pTop = NULL;
		m_nUnique = nUnique;
		m_nSize = 0;
		m_nHashRate = nHashRate;
		if((m_bNeedExtend = (m_nHashRate < 1.0)))
		{
			m_nHashSize = GetPrimeHashSize(0);
			m_nHashLevel = GetHashLevel(m_nHashSize);
		}
		else
		{
			m_nHashSize = 0;
			m_nHashLevel = 4;
		}
		m_nBigEndian = IsSmallEndian();
	}

	~CFreeHash()
	{
		Clear();
	}

	bool IsUnique()
	{
		return m_nUnique!=0;
	}

	CHashNode* GetNode(uint32 nIdx)
	{
		uint32 nMod;
		uint8* pIdx;
		if(m_bNeedExtend)
		{
			nMod = nIdx % m_nHashSize;
			pIdx = (uint8*)&nMod;
			if(m_nBigEndian)
				pIdx += 4 - m_nHashLevel;
		}
		else
			pIdx = (uint8*)&nIdx;
		CHashBranch* pBranch = m_pTop;
		for(uint32 i=0; pBranch && i<m_nHashLevel; ++i)
			pBranch = pBranch->pBranch[pIdx[i]];
		CHashNode* pNode = (CHashNode*)pBranch;
		while(pNode)
		{
			if(pNode->nHashValue == nIdx)
				break;
			pNode = pNode->pNextNode;
		}
		return pNode;
	}

	CHashNode* GetNode(uint32 nIdx, FIsEqual0 IsEqual, void* pCond)
	{
		uint32 nMod;
		uint8* pIdx;
		if(m_bNeedExtend)
		{
			nMod = nIdx % m_nHashSize;
			pIdx = (uint8*)&nMod;
			if(m_nBigEndian)
				pIdx += 4 - m_nHashLevel;
		}
		else
			pIdx = (uint8*)&nIdx;
		CHashBranch* pBranch = m_pTop;
		for(uint32 i=0; pBranch && i<m_nHashLevel; ++i)
			pBranch = pBranch->pBranch[pIdx[i]];
		CHashNode* pNode = (CHashNode*)pBranch;
		while(pNode)
		{
			if(pNode->nHashValue == nIdx && IsEqual(pCond, pNode->oObject))
				break;
			pNode = pNode->pNextNode;
		}
		return pNode;
	}

	CHashNode* GetNextNode(CHashNode* pCur, FIsEqual0 IsEqual, void* pCond)
	{
		if(m_nUnique)
			return NULL;
		uint32 nIdx = pCur->nHashValue;
		CHashNode* pNode = pCur->pNextNode;
		while(pNode)
		{
			if(pNode->nHashValue == nIdx && IsEqual(pCond, pNode->oObject))
				break;
			pNode = pNode->pNextNode;
		}
		return pNode;
	}

	uint32 RemoveNode(uint32 nIdx, FIsEqual0 IsEqual, void* pCond)
	{
		uint32 nMod;
		uint8* pIdx;
		if(m_bNeedExtend)
		{
			nMod = nIdx % m_nHashSize;
			pIdx = (uint8*)&nMod;
			if(m_nBigEndian)
				pIdx += 4 - m_nHashLevel;
		}
		else
			pIdx = (uint8*)&nIdx;
		uint32 nRet = RemoveNode(m_pTop, pIdx, 0, nIdx, IsEqual, pCond);
		if(nRet)
		{
			m_nSize -= nRet;
			if(!m_nSize && m_nHashRate < 1.0)
			{
				m_nHashSize = GetPrimeHashSize((uint32)(m_nSize*m_nHashRate));
				m_nHashLevel = GetHashLevel(m_nHashSize);
			}
		}
		return nRet;
	}

	CHashNode* InsertNode(uint32 nIdx, const TObject& oObject, FIsEqual1 IsEqual, void* pContext, bool& bConflict)
	{
		bConflict = false;
		uint32 nMod;
		uint8* pIdx;
		if(m_bNeedExtend)
		{
			nMod = nIdx % m_nHashSize;
			pIdx = (uint8*)&nMod;
			if(m_nBigEndian)
				pIdx += 4 - m_nHashLevel;
		}
		else
			pIdx = (uint8*)&nIdx;
		CHashNode* pNode = InsertNode(m_pTop, pIdx, 0, nIdx, IsEqual, oObject, pContext, bConflict);
		if(pNode)
		{
			++m_nSize;
			if(m_bNeedExtend)
			{
				uint32 nNewHashSize = GetPrimeHashSize((uint32)(m_nSize * m_nHashRate));
				if(nNewHashSize > m_nHashSize)
					ExpandTable(nNewHashSize);
			}
		}
		return pNode;
	}

	void Clear()
	{
		Clear(m_pTop, 0);
		m_nSize = 0;
		if(m_bNeedExtend)
		{
			m_nHashSize = GetPrimeHashSize(0);
			m_nHashLevel = GetHashLevel(m_nHashSize);
		}
	}

	uint32 GetSize()
	{
		return m_nSize;
	}

private:
	uint32 GetPrimeHashSize(uint32 nHashSize)
	{
		static uint32 nStart[] =
		{
			193,		1543,		12289,		49157,
			196613,		786433,		1572869,	3145739,
			6291469,	12582917,	25165843,	50331653
		};

		register uint32 __half, __len = (sizeof(nStart)>>2);
		register uint32* __middle, *__first = nStart, *__last = __first + __len;

		while(__len>0)
		{
			__half = __len >> 1;
			__middle = __first + __half;
			if(*__middle < nHashSize)
			{
				__first = __middle + 1;
				__len -= __half + 1;
			}
			else
				__len = __half;
		}

		return __first == __last ? *(__last - 1) : *__first;
	}

	uint32 GetHashLevel(uint32 nHashSize)
	{
		uint32 nLevel = 4;
		uint32 nMod = 0xFF000000;
		while(nLevel && !(nMod & nHashSize))
		{
			--nLevel;
			nMod >>= 8;
		}
		return nLevel;
	}

	void Clear(CHashBranch* &pBranch, uint32 nLevel)
	{
		if(pBranch)
		{
			if(nLevel < m_nHashLevel-1)
			{
				++nLevel;
				for(uint32 i=0; i<256; ++i)
					Clear(pBranch->pBranch[i], nLevel);
				delete pBranch;
			}
			else
			{
				CHashNode* pNode = (CHashNode*)pBranch;
				while(pNode)
				{
					CHashNode* pNext = pNode->pNextNode;
					delete pNode;
					pNode = pNext;
				}
			}
			pBranch = NULL;
		}
	}

	uint32 RemoveNode(CHashBranch* &pBranch, uint8* pIdx, uint32 nLevel, uint32 nHash, FIsEqual0 IsEqual, void* pCond)
	{
		uint32 nRet = 0;
		if(pBranch)
		{
			CHashBranch*& pBranch2 = pBranch->pBranch[pIdx[nLevel]], *pBranch3 = pBranch2;
			if(nLevel < m_nHashLevel-1)
				nRet = RemoveNode(pBranch3, pIdx, nLevel+1, nHash, IsEqual, pCond);
			else
			{
				CHashNode* pPrev = NULL;
				CHashNode* pNode = (CHashNode*)pBranch3;
				while(pNode)
				{
					CHashNode* pNext = pNode->pNextNode;
					if(pNode->nHashValue == nHash && IsEqual(pCond, pNode->oObject))
					{
						++nRet;
						delete pNode;
						if(pPrev)
							pPrev->pNextNode = pNext;
						else
							pBranch3 = (CHashBranch*)pNext;
						if(m_nUnique)
							break;
					}
					else
						pPrev = pNode;
					pNode = pNext;
				}
			}
			if(pBranch2 != pBranch3)
			{
				pBranch2 = pBranch3;
				if(!pBranch3)
				{
					--pBranch->nSize;
					if(!pBranch->nSize)
					{
						delete pBranch;
						pBranch = NULL;
					}
				}
			}
		}
		return nRet;
	}

	CHashNode* InsertNode(CHashBranch* &pBranch, uint8* pIdx, uint32 nLevel, uint32 nHash, FIsEqual1 IsEqual, const TObject& oObject, void* pContext, bool& bConflict)
	{
		CHashNode* pRet = NULL;
		if(pBranch)
		{
			CHashBranch*& pBranch2 = pBranch->pBranch[pIdx[nLevel]], *pBranch3 = pBranch2;
			if(nLevel < m_nHashLevel-1)
				pRet = InsertNode(pBranch3, pIdx, nLevel+1, nHash, IsEqual, oObject, pContext, bConflict);
			else
			{
				CHashNode* pNode = NULL;
				if(m_nUnique)
				{
					pNode = (CHashNode*)pBranch3;
					while(pNode)
					{
						if(pNode->nHashValue == nHash && IsEqual(oObject, pNode->oObject, pContext))
						{
							bConflict = true;
							return NULL;
						}
						pNode = pNode->pNextNode;
					}
				}
				pRet = new CHashNode;
				pRet->nHashValue = nHash;
				pRet->oObject = oObject;
				pRet->pNextNode = (CHashNode*)pBranch3;
				pBranch3 = (CHashBranch*)pRet;
			}
			if(pBranch2 != pBranch3)
			{
				if(!pBranch2)
					++pBranch->nSize;
				pBranch2 = pBranch3;
			}
		}
		else
		{
			pBranch = new CHashBranch;
			for(uint32 i=0; i<256; ++i)
				pBranch->pBranch[i] = NULL;
			pBranch->nSize = 0;
			CHashBranch*& pBranch2 = pBranch->pBranch[pIdx[nLevel]];
			if(nLevel < m_nHashLevel-1)
				pRet = InsertNode(pBranch2, pIdx, nLevel+1, nHash, IsEqual, oObject, pContext, bConflict);
			else
			{
				pRet = new CHashNode;
				pRet->nHashValue = nHash;
				pRet->oObject = oObject;
				pRet->pNextNode = (CHashNode*)pBranch2;
				pBranch2 = (CHashBranch*)pRet;
			}
			++pBranch->nSize;
		}
		return pRet;
	}

	void ExpandTable(uint32 nNewHashSize)
	{
		CHashBranch* pBranchList = NULL;
		CFreeHash<TObject> oNewHashTable(m_nUnique, m_nHashRate);
		oNewHashTable.m_nHashSize = nNewHashSize;
		oNewHashTable.m_nHashLevel = GetHashLevel(nNewHashSize);
		if(m_pTop)
			oNewHashTable.MoveFrom(m_nHashLevel, m_pTop, 0, pBranchList);
		m_pTop = oNewHashTable.m_pTop;
		m_nHashSize = nNewHashSize;
		m_nHashLevel = oNewHashTable.m_nHashLevel;
		oNewHashTable.m_pTop = 0;
		/*
				while(pBranchList)
				{
					CHashBranch* pNext = pBranchList->pBranch[0];
					delete pBranchList;
					pBranchList = pNext;
				}
		*/
	}

	void MoveFrom(uint32 nHashLevel, CHashBranch* pBranch, uint32 nLevel, CHashBranch* &pBranchList)
	{
		if(nLevel < nHashLevel-1)
		{
			++nLevel;
			CHashBranch oBranch = *pBranch;
			pBranch->pBranch[0] = pBranchList;
			pBranchList = pBranch;
			for(uint32 i=0; i<256; ++i)
				if(oBranch.pBranch[i])
					MoveFrom(nHashLevel, oBranch.pBranch[i], nLevel, pBranchList);
		}
		else
		{
			CHashNode* pNode = (CHashNode*)pBranch;
			while(pNode)
			{
				CHashNode* pNext = pNode->pNextNode;
				uint32 nHash = pNode->nHashValue;
				uint32 nMod = nHash % m_nHashSize;
				uint8* pIdx = (uint8*)&nMod;
				if(m_nBigEndian)
					pIdx += 4 - m_nHashLevel;
				MoveNode(m_pTop, pIdx, 0, pNode, pBranchList);
				pNode = pNext;
			}
		}
	}

	void MoveNode(CHashBranch* &pBranch, uint8* pIdx, uint32 nLevel, CHashNode* pNode, CHashBranch* &pBranchList)
	{
		if(pBranch)
		{
			CHashBranch*& pBranch2 = pBranch->pBranch[pIdx[nLevel]], *pBranch3 = pBranch2;
			if(nLevel < m_nHashLevel-1)
				MoveNode(pBranch3, pIdx, nLevel+1, pNode, pBranchList);
			else
			{
				pNode->pNextNode = (CHashNode*)pBranch3;
				pBranch3 = (CHashBranch*)pNode;
			}
			if(pBranch2 != pBranch3)
			{
				if(!pBranch2)
					++pBranch->nSize;
				pBranch2 = pBranch3;
			}
		}
		else
		{
			if(pBranchList)
			{
				pBranch = pBranchList;
				pBranchList = pBranchList->pBranch[0];
			}
			else
				pBranch = new CHashBranch;
			pBranch->nSize = 0;
			for(uint32 i=0; i<256; ++i)
				pBranch->pBranch[i] = NULL;
			CHashBranch*& pBranch2 = pBranch->pBranch[pIdx[nLevel]];
			if(nLevel < m_nHashLevel-1)
				MoveNode(pBranch2, pIdx, nLevel+1, pNode, pBranchList);
			else
			{
				pNode->pNextNode = (CHashNode*)pBranch2;
				pBranch2 = (CHashBranch*)pNode;
			}
			++pBranch->nSize;
		}
	}
};

FOCP_END();

#endif //_AFT_HASH_HPP_
