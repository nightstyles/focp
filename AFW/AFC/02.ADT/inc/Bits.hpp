
#include "List.hpp"

#ifndef _ADT_BITS_HPP_
#define _ADT_BITS_HPP_

FOCP_BEGIN();

class ADT_API CBits
{
private:
	uint32 m_nUnit, m_nSize;
	uint8* m_pData;

public:
	CBits(uint32 nSize, bool bDefault = false);
	CBits(const CBits &oSrc);
	virtual ~CBits();

	const CBits &operator=(const CBits &oSrc);
	CBits* Clone() const;

	uint32 GetSize() const;
	uint32 Elements() const;//返回真值元素个数
	uint32 First() const;//无效返回0xFFFFFFFF

	bool Get(uint32 nIdx) const;
	void Set(uint32 nIdx, bool bValue);
	void SetAll(bool bValue);
	bool operator[](uint32 nIdx);

	bool Equal(const CBits &oSrc) const;
	bool Include(const CBits& oSrc) const;
	bool Intersect(const CBits& oSrc) const;

	CBits& Not();
	CBits& And(const CBits &oSrc);
	CBits& Or(const CBits &oSrc);
	CBits& Xor(const CBits &oSrc);
	CBits& Subtract(const CBits& oSrc);
};

template<typename TInteger> class CIntegerSet
{
public:
	struct CIntegerRange
	{
		TInteger oFrom;
		TInteger oTo;
	};

private:
	TInteger m_nMin, m_nMax;
	CSingleList<CIntegerRange> m_oSet;

public:
	inline CIntegerSet(TInteger nMin, TInteger nMax)
		:m_nMin(nMin),m_nMax(nMax)
	{
	}

	inline virtual ~CIntegerSet()
	{
	}

	inline CIntegerSet(const CIntegerSet<TInteger> &oSrc)
		:m_nMin(oSrc.m_nMin),m_nMax(oSrc.m_nMax),m_oSet(oSrc.m_oSet)
	{
	}

	inline CIntegerSet<TInteger>& operator=(const CIntegerSet<TInteger> &oSrc)
	{
		if(this != &oSrc)
		{
			m_oSet = oSrc.m_oSet;
			m_nMin = oSrc.m_nMin;
			m_nMax = oSrc.m_nMax;
		}
		return *this;
	}

	inline void* FirstIt() const
	{
		return m_oSet.First();
	}

	inline void* LastIt() const
	{
		return m_oSet.Last();
	}

	inline const CIntegerRange& GetItem(void* pIt) const
	{
		return m_oSet.GetItem(pIt);
	}

	inline uint32 GetSize() const
	{
		return m_oSet.GetSize();
	}

	inline bool Get(TInteger i) const
	{
		for (void*p = m_oSet.First(); p; p = m_oSet.GetNext(p))
		{
			CIntegerRange &oRange = m_oSet.GetItem(p);
			if (i < oRange.oFrom)
				return false;
			else if (i <= oRange.oTo)
				return true;
		}
		return false;
	}

	inline void Set(TInteger i)
	{
		if(i < m_nMin || i > m_nMax)
			Abort();
		void* pCur = m_oSet.First(), *pPrev=NULL;
		while(pCur)
		{
			CIntegerRange &oRange = m_oSet.GetItem(pCur);
			if(i < oRange.oFrom - 1)
				break;
			if(i <= oRange.oTo + 1)
			{
				if(i == oRange.oFrom - 1)
					--oRange.oFrom;
				else if(i == oRange.oTo + 1)
				{
					++oRange.oTo;
					void* pNext = m_oSet.GetNext(pCur);
					if(pNext)
					{
						CIntegerRange &oNext = m_oSet.GetItem(pNext);
						if(oRange.oTo == oNext.oFrom -1)
						{
							oRange.oTo = oNext.oTo;
							m_oSet.RemoveNext(pCur);
						}
					}
				}
				return;
			}
			pPrev = pCur;
			pCur = m_oSet.GetNext(pCur);
		}
		CIntegerRange oRange = {i, i};
		m_oSet.Append(pPrev, oRange);
	}

	inline void Subtract(TInteger i)
	{
		CIntegerSet<TInteger> o(m_nMin, m_nMax);
		o.Set(i);
		Subtract(o);
	}

	inline CIntegerSet<TInteger>* Clone() const
	{
		return new CIntegerSet<TInteger>(*this);
	}

	inline bool Equal(const CIntegerSet<TInteger>& oSrc) const
	{
		if(this == &oSrc)
			return true;
		void* p1 = m_oSet.First(), *p2 = oSrc.m_oSet.First();
		while(p1 && p2)
		{
			const CIntegerRange& oRange1 = m_oSet.GetItem(p1);
			const CIntegerRange& oRange2 = oSrc.m_oSet.GetItem(p2);
			if(oRange1.oFrom != oRange2.oFrom || oRange1.oTo != oRange2.oTo)
				return false;
			p1 = m_oSet.GetNext(p1);
			p2 = oSrc.m_oSet.GetNext(p2);
		}
		return p1 == p2;
	}

	inline TInteger Elements() const
	{
		TInteger n = 0;
		for(void*p=m_oSet.First(); p; p = m_oSet.GetNext(p))
		{
			CIntegerRange& oRange = m_oSet.GetItem(p);
			n += oRange.oTo - oRange.oFrom + 1;
		}
		return n;
	}

	inline TInteger First() const
	{
		void* p = m_oSet.First();
		if (p == NULL)
			Abort();
		return m_oSet.GetItem(p).oFrom;
	}

	inline bool Alloc(TInteger &nRet)
	{
		nRet = m_nMin;
		void* pIt = m_oSet.First();
		for(; pIt; pIt = m_oSet.GetNext(pIt))
		{
			CIntegerRange& oFrom = m_oSet.GetItem(pIt);
			if(nRet < oFrom.oFrom)
			{
				nRet = oFrom.oFrom - 1;
				Set(nRet);
				return true;
			}
			nRet = oFrom.oTo + 1;
		}
		if(nRet <= m_nMax)
		{
			Set(nRet);
			return true;
		}
		return false;
	}

	inline bool Empty() const
	{
		return 0 == m_oSet.GetSize();
	}

	inline void Clear()
	{
		m_oSet.Clear();
	}

	inline CIntegerSet<TInteger>& Or(const CIntegerSet<TInteger>& oSrc)
	{
		if(this != &oSrc)
		{
			for(void* p = oSrc.m_oSet.First(); p; p = oSrc.m_oSet.GetNext(p))
			{
				CIntegerRange& oRange = oSrc.m_oSet.GetItem(p);
				for(TInteger i=oRange.oFrom; i<=oRange.oTo; ++i)
					Set(i);
			}
		}
		return *this;
	}

	inline CIntegerSet<TInteger>& And(const CIntegerSet<TInteger>& oSrc)
	{
		CIntegerSet<TInteger> x(m_nMin, m_nMax);
		if(this != &oSrc)
		{
			for(void* p = m_oSet.First(); p; p = m_oSet.GetNext(p))
			{
				CIntegerRange& oRange = m_oSet.GetItem(p);
				for(TInteger i=oRange.oFrom; i<=oRange.oTo; ++i)
				{
					if(oSrc.Get(i))
						x.Set(i);
				}
			}
		}
		m_oSet.Swap(x.m_oSet);
		return *this;
	}

	inline CIntegerSet<TInteger>& Subtract(const CIntegerSet<TInteger>& oSrc)
	{
		CIntegerSet<TInteger> x(m_nMin, m_nMax);
		if(this != &oSrc)
		{
			for(void* p = m_oSet.First(); p; p = m_oSet.GetNext(p))
			{
				CIntegerRange& oRange = m_oSet.GetItem(p);
				for(TInteger i=oRange.oFrom; i<=oRange.oTo; ++i)
				{
					if(!oSrc.Get(i))
						x.Set(i);
				}
			}
		}
		m_oSet.Swap(x.m_oSet);
		return *this;
	}

	inline bool Include(const CIntegerSet<TInteger>& oSrc) const
	{
		for(void* p = oSrc.m_oSet.First(); p; p = oSrc.m_oSet.GetNext(p))
		{
			const CIntegerRange& oRange = oSrc.m_oSet.GetItem(p);
			for (TInteger i = oRange.oFrom; i <= oRange.oTo; i++)
			{
				if(!Get(i))
					return false;
			}
		}
		return true;
	}

	inline bool Intersect(const CIntegerSet<TInteger>& oSrc) const
	{
		for(void* p = oSrc.m_oSet.First(); p; p = oSrc.m_oSet.GetNext(p))
		{
			const CIntegerRange& oRange = oSrc.m_oSet.GetItem(p);
			for (TInteger i = oRange.oFrom; i <= oRange.oTo; i++)
			{
				if(Get(i))
					return true;
			}
		}
		return false;
	}

	inline void Fill()
	{
		Clear();
		CIntegerRange oRange(m_nMin, m_nMax);
		m_oSet.Push(oRange);
	}
};

FOCP_END();

#endif
