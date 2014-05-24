
#include "Malloc.hpp"
#include "Trait.hpp"

#ifndef _ADT_VECTOR_HPP_
#define _ADT_VECTOR_HPP_

FOCP_BEGIN();

template<typename TData> class CVector
{
private:
	uint32 m_nSize;
	uint32 m_nCapacity;
	TData* m_pItems;

public:
	inline ~CVector()
	{
		Clear();
	}

	inline CVector()
	{
		m_nSize = 0;
		m_nCapacity = 0;
		m_pItems = NULL;
	}

	inline CVector(const CVector<TData>& oSrc)
	{
		m_nSize = oSrc.m_nSize;
		m_nCapacity = m_nSize;
		m_pItems = NULL;
		if(m_nSize)
		{
			uint32 i;
			m_pItems = (TData*)CMalloc::Malloc(m_nSize*FOCP_SIZE_OF(TData));
			for(i=0; i<m_nSize; ++i)
				new(m_pItems+i) TData(oSrc.m_pItems[i]);
		}
	}

	inline CVector<TData>& operator=(const CVector<TData>& oSrc)
	{
		if(this != &oSrc)
		{
			CVector oVec(oSrc);
			Swap(oVec);
		}
		return *this;
	}

	inline void Clear()
	{
		if(m_nSize)
		{
			uint32 i;
			for(i=0; i<m_nSize; ++i)
				m_pItems[i].~TData();
			m_nSize = 0;
		}
		if(m_pItems)
		{
			CMalloc::Free(m_pItems);
			m_pItems = NULL;
			m_nCapacity = 0;
		}
	}

	inline uint32 GetCapacity() const
	{
		return m_nCapacity;
	}

	inline bool SetCapacity(uint32 nCapacity)
	{
		uint32 i, nSize;
		if(nCapacity==m_nCapacity)
			return true;
		if(nCapacity == 0)
		{
			Clear();
			return true;
		}
		TData* pItems = (TData*)CMalloc::Malloc(nCapacity*FOCP_SIZE_OF(TData));
		if(pItems == NULL)
			return false;
		nSize = m_nSize;
		if(nSize > nCapacity)
			nSize = nCapacity;
		for(i=0; i<nSize; ++i)
			new(pItems+i) TData(m_pItems[i]);
		if(m_pItems)
		{
			for(i=0; i<m_nSize; ++i)
				m_pItems[i].~TData();
			CMalloc::Free(m_pItems);
		}
		m_pItems = pItems;
		m_nCapacity = nCapacity;
		m_nSize = nSize;
		return true;
	}

	inline uint32 GetSize() const
	{
		return m_nSize;
	}

	inline void SetSize(uint32 nNewSize, const TData &oSrc=TData())
	{
		if(nNewSize > m_nSize)
		{
			uint32 nDelta = nNewSize - m_nSize;
			if(Grow(nDelta))
			{
				uint32 i;
				for(i=0; i<nDelta; ++i)
					new(m_pItems+m_nSize+i) TData(oSrc);
				m_nSize = nNewSize;
			}
		}
		else if(nNewSize == 0)
			Clear();
		else if(nNewSize < m_nSize)
		{
			uint32 i=nNewSize;
			for(; i<m_nSize; ++i)
				m_pItems[i].~TData();
			m_nSize = nNewSize;
			UnGrow();
		}
	}

	inline TData* At(uint32 nIdx)
	{
		if(nIdx >= m_nSize)
			return NULL;
		return m_pItems + nIdx;
	}

	inline TData& operator[](uint32 nIdx)
	{
		return *At(nIdx);
	}

	inline const TData* At(uint32 nIdx) const
	{
		if(nIdx >= m_nSize)
			return NULL;
		return m_pItems + nIdx;
	}

	inline const TData& operator[](uint32 nIdx) const
	{
		return *At(nIdx);
	}

	inline uint32 Insert(uint32 nIdx, const TData* pDatas, uint32 nCount)
	{
		uint32 i, j;
		if(nIdx >= m_nSize)
			nIdx = m_nSize;
		if(nCount == 0)
			return (uint32)(-1);
		if(m_nSize + nCount > m_nCapacity && Grow(nCount) == false)
			return (uint32)(-1);
		for(i=0; i<nCount; ++i)
			new(m_pItems+m_nSize+i) TData();
		j = m_nSize + nCount -1;
		for(i = m_nSize; i>nIdx; --j)
			m_pItems[j] = m_pItems[--i];
		for(i=0; i<nCount; ++i)
			m_pItems[nIdx+i] = pDatas[i];
		m_nSize += nCount;
		return nIdx;
	}

	inline uint32 Insert(uint32 nIdx, const TData& oSrc=TData(), uint32 nCount = 1)
	{
		uint32 i, nRet = 0;
		if(nCount == 0)
			return 0;
		for(i=0; i<nCount; ++i)
		{
			uint32 x = Insert(nIdx, &oSrc, 1);
			if(x == 1)
				++nRet;
			else
				break;
		}
		if(i == 0)
			return uint32(-1);
		return nRet;
	}

	inline uint32 Remove(uint32 nIdx)
	{
		if(nIdx == (uint32)(-1))
			return (uint32)(-1);
		return Remove(nIdx, nIdx+1);
	}

	inline uint32 Remove(uint32 nFromIdx, uint32 nEndIdx)//[nFromIdx,nEndIdx)
	{
		uint32 nDistance;
		if(nEndIdx < nFromIdx)
			FocpAbort(("end Index is less than the from Index"));
		nDistance = nEndIdx - nFromIdx;
		if(nDistance && nFromIdx < m_nSize)
		{
			uint32 i=nFromIdx, j=nEndIdx;
			for(; j<m_nSize; ++i, ++j)
				m_pItems[i] = m_pItems[j];
			for(; i<m_nSize; ++i)
				m_pItems[i].~TData();
			m_nSize -= nDistance;
			UnGrow();
		}
		if(nFromIdx >= m_nSize)
			return (uint32)(-1);
		return nFromIdx;
	}

	inline void Swap(CVector<TData>& oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_nSize,oSrc.m_nSize);
			::FOCP_NAME::Swap(m_nCapacity,oSrc.m_nCapacity);
			::FOCP_NAME::Swap(m_pItems,oSrc.m_pItems);
		}
	}

private:
	inline bool Grow(uint32 nSize=1)
	{
		bool bRet;
		uint32 nCapacity = m_nSize + GetDelta(m_nSize);
		if(nCapacity < m_nSize + nSize)
			nCapacity = m_nSize + nSize;
		bRet = SetCapacity(nCapacity);
		if(bRet == false)
			bRet = SetCapacity(m_nSize + nSize);
		return bRet;
	}

	inline void UnGrow()
	{
		uint32 nCapacity = m_nSize + GetDelta(m_nSize);
		if(nCapacity < m_nCapacity)
			SetCapacity(nCapacity);
	}

	inline uint32 GetDelta(uint32 nCapacity)
	{
		uint32 nDelta;
		if(nCapacity > 64)
			nDelta = (nCapacity>>2);
		else if(nCapacity > 8)
			nDelta = 16;
		else
			nDelta = 4;
		return nDelta;
	}
};

FOCP_END();

#endif //_ADT_VECTOR_HPP_
