
#include "AdtDef.hpp"

#ifndef _ADT_POINTER_HPP_
#define _ADT_POINTER_HPP_

FOCP_BEGIN();

template<typename TData> class CAutoPointer;
template<typename TData> class CMasterPointer;
template<typename TData> class CSlavePointer;

/*********************************************************
	CAutoPointer
		是标准库的auto_ptr的替代品，注意不支持循引用
**********************************************************/
template<typename TData> class CAutoPointer
{
private:
	bool m_bOwned;
	TData* m_pPointer;

public:
	inline ~CAutoPointer()
	{
		if(m_bOwned)
		{
			delete m_pPointer;
			m_bOwned =false;
		}
		m_pPointer = NULL;
	}

	inline CAutoPointer(TData* pPointer=NULL)
	{
		m_bOwned = pPointer?true:false;
		m_pPointer = pPointer;
	}

	inline CAutoPointer(const CAutoPointer<TData>& oPointer)
	{
		m_bOwned = oPointer.m_bOwned;
		m_pPointer = (TData*)oPointer.m_pPointer;
		((CAutoPointer<TData>&)oPointer).m_bOwned = false;
	}

	inline CAutoPointer(const CMasterPointer<TData>& oPointer);

	inline CAutoPointer<TData>& operator=(const CAutoPointer<TData>& oPointer)
	{
		if(this != &oPointer)
		{
			if(m_pPointer != oPointer.m_pPointer)
			{
				if(m_bOwned)
					delete m_pPointer;
				m_bOwned = oPointer.m_bOwned;
			}
			else if(oPointer.m_bOwned)
				m_bOwned = true;
			m_pPointer = (TData*)oPointer.m_pPointer;
			((CAutoPointer<TData>&)oPointer).m_bOwned = false;
		}
		return (*this);
	}

	inline CAutoPointer<TData>& operator=(const CMasterPointer<TData>& oPointer);

	inline CAutoPointer<TData>& operator=(TData* pPointer)
	{
		if(pPointer != m_pPointer)
		{
			if(m_bOwned)
				delete m_pPointer;
			m_bOwned = pPointer?true:false;
			m_pPointer = pPointer;
		}
		return *this;
	}

	inline bool IsOwner()
	{
		return m_bOwned;
	}

	inline TData* GetPointer(bool bDetach=false)
	{
		TData* pRet = m_pPointer;
		if(bDetach && pRet)
		{
			m_bOwned = false;
			m_pPointer = NULL;
		}
		return pRet;
	}

	inline const TData* GetPointer() const
	{
		return m_pPointer;
	}

	inline TData* operator->()
	{
		return m_pPointer;
	}

	inline const TData* operator->()const
	{
		return m_pPointer;
	}

	inline TData& operator*()
	{
		return *m_pPointer;
	}

	inline const TData& operator*() const
	{
		return *m_pPointer;
	}

	inline operator TData*()
	{
		return m_pPointer;
	}

	inline operator const TData*() const
	{
		return m_pPointer;
	}

	inline operator TData&()
	{
		return *m_pPointer;
	}

	inline operator const TData&()const
	{
		return *m_pPointer;
	}

	inline CAutoPointer<TData>& Swap(CAutoPointer<TData> &oSrc)
	{
		if(this != &oSrc)
		{
			::FOCP_NAME::Swap(m_bOwned, oSrc.m_bOwned);
			::FOCP_NAME::Swap(m_pPointer, oSrc.m_pPointer);
		}
	}
};

/*********************************************************
	CMasterPointer,主指针,具有垃圾回收机制,注意不支持循引用
	CSlavePointer,从指针,配合CMasterPointer,可支持循环引用
**********************************************************/

FOCP_DETAIL_BEGIN();
template<typename TData> struct CMasterSlavePointer
{
	uint32 nCounter;
	uint32 nWeaker;
	TData* pPointer;
};
FOCP_DETAIL_END();

template<typename TData> class CMasterPointer
{
	friend class CSlavePointer<TData>;
private:
	FOCP_DETAIL_NAME::CMasterSlavePointer<TData>* m_pPointer;

public:
	inline ~CMasterPointer()
	{
		if(m_pPointer)
		{
			if(!(--m_pPointer->nCounter))
			{
				if(m_pPointer->pPointer)//可能被Detach
					delete m_pPointer->pPointer;
				if(m_pPointer->nWeaker)
					m_pPointer->pPointer = NULL;
				else
					delete m_pPointer;
			}
			m_pPointer = NULL;
		}
	}

	inline CMasterPointer(TData* pPointer=NULL)
	{
		if(pPointer == NULL)
			m_pPointer = NULL;
		else
		{
			m_pPointer = new FOCP_DETAIL_NAME::CMasterSlavePointer<TData>;
			m_pPointer->nCounter = 1;
			m_pPointer->nWeaker = 0;
			m_pPointer->pPointer = pPointer;
		}
	}

	inline CMasterPointer(const CMasterPointer<TData>& oPointer)
	{
		m_pPointer = ((CMasterPointer<TData>&)oPointer).m_pPointer;
		if(m_pPointer)
			++m_pPointer->nCounter;
	}

	inline CMasterPointer(const CAutoPointer<TData>& oPointer)
	{
		TData* pPointer = ((CAutoPointer<TData>&)oPointer).GetPointer(true);
		if(pPointer == NULL)
			m_pPointer = NULL;
		else
		{
			m_pPointer = new FOCP_DETAIL_NAME::CMasterSlavePointer<TData>;
			m_pPointer->nCounter = 1;
			m_pPointer->nWeaker = 0;
			m_pPointer->pPointer = pPointer;
		}
	}

	inline CMasterPointer(const CSlavePointer<TData>& oPointer);

	inline CMasterPointer<TData>& operator=(const CMasterPointer<TData>& oPointer)
	{
		if(this != &oPointer && m_pPointer != ((CMasterPointer<TData>&)oPointer).m_pPointer)
		{
			if(m_pPointer && (!(--m_pPointer->nCounter)))
			{
				if(m_pPointer->pPointer)//可能被Detach
					delete m_pPointer->pPointer;
				if(m_pPointer->nWeaker)
					m_pPointer->pPointer = NULL;
				else
					delete m_pPointer;
			}
			m_pPointer = ((CMasterPointer<TData>&)oPointer).m_pPointer;
			if(m_pPointer)
				++m_pPointer->nCounter;
		}
		return (*this);
	}

	inline CMasterPointer<TData>& operator=(const CAutoPointer<TData>& oPointer)
	{
		return operator=(((CAutoPointer<TData>&)oPointer).GetPointer(true));
	}

	inline CMasterPointer<TData>& operator=(const CSlavePointer<TData>& oPointer);

	inline CMasterPointer<TData>& operator=(TData* pPointer)
	{
		if(m_pPointer)
		{
			if(pPointer == m_pPointer->pPointer)
				return *this;
			if(!(--m_pPointer->nCounter))
			{
				if(m_pPointer->pPointer)//可能被Detach
					delete m_pPointer->pPointer;
				if(m_pPointer->nWeaker)
					m_pPointer->pPointer = NULL;
				else
					delete m_pPointer;
			}
		}
		if(pPointer == NULL)
			m_pPointer = NULL;
		else
		{
			m_pPointer = new FOCP_DETAIL_NAME::CMasterSlavePointer<TData>;
			m_pPointer->nCounter = 1;
			m_pPointer->nWeaker = 0;
			m_pPointer->pPointer = pPointer;
		}
		return *this;
	}

	inline TData* operator->()
	{
		if(m_pPointer == NULL)
			return NULL;
		return m_pPointer->pPointer;
	}

	inline const TData* operator->() const
	{
		if(m_pPointer == NULL)
			return NULL;
		return m_pPointer->pPointer;
	}

	inline TData& operator*()
	{
		return *m_pPointer->pPointer;
	}

	inline const TData& operator*() const
	{
		return *m_pPointer->pPointer;
	}

	inline operator TData*()
	{
		if(m_pPointer == NULL)
			return NULL;
		return m_pPointer->pPointer;
	}

	inline operator const TData*() const
	{
		if(m_pPointer == NULL)
			return NULL;
		return m_pPointer->pPointer;
	}

	inline operator TData&()
	{
		return *m_pPointer->pPointer;
	}

	inline operator const TData&() const
	{
		return *m_pPointer->pPointer;
	}

	inline TData* Detach()
	{
		if(m_pPointer == NULL)
			return NULL;
		TData* pRet = m_pPointer->pPointer;
		if(pRet)
		{
			m_pPointer->pPointer = NULL;
			if(!(--m_pPointer->nCounter) && !m_pPointer->nWeaker)
				delete m_pPointer;
			m_pPointer = NULL;
		}
		return pRet;
	}

	inline CMasterPointer<TData>& Swap(CMasterPointer<TData>& oPointer)
	{
		if(this != &oPointer && m_pPointer != oPointer.m_pPointer)
			::FOCP_NAME::Swap(m_pPointer, oPointer.m_pPointer);
	}
};

template<typename TData> class CSlavePointer
{
	friend class CMasterPointer<TData>;
private:
	FOCP_DETAIL_NAME::CMasterSlavePointer<TData>* m_pPointer;

public:
	inline CSlavePointer(const CSlavePointer<TData>& oPointer)
	{
		m_pPointer = ((CSlavePointer<TData>&)oPointer).m_pPointer;
		if(m_pPointer)
			++m_pPointer->nWeaker;
	}

	inline CSlavePointer(const CMasterPointer<TData>& oPointer)
	{
		m_pPointer = ((CMasterPointer<TData>&)oPointer).m_pPointer;
		if(m_pPointer)
			++m_pPointer->nWeaker;
	}

	inline ~CSlavePointer()
	{
		if(m_pPointer)
		{
			if(!(--m_pPointer->nWeaker) && !m_pPointer->nCounter)
				delete m_pPointer;
			m_pPointer = NULL;
		}
	}

	inline CSlavePointer<TData>& operator=(const CSlavePointer<TData>& oPointer)
	{
		if(this != &oPointer && m_pPointer != ((CSlavePointer<TData>&)oPointer).m_pPointer)
		{
			if(m_pPointer)
			{
				if(!(--m_pPointer->nWeaker) && !m_pPointer->nCounter)
					delete m_pPointer;
				m_pPointer = NULL;
			}
			m_pPointer = ((CSlavePointer<TData>&)oPointer).m_pPointer;
			if(m_pPointer)
				++m_pPointer->nWeaker;
		}
	}

	inline CSlavePointer<TData>& operator=(const CMasterPointer<TData>& oPointer)
	{
		if(this != &oPointer && m_pPointer != ((CMasterPointer<TData>&)oPointer).m_pPointer)
		{
			if(m_pPointer)
			{
				if(!(--m_pPointer->nWeaker) && !m_pPointer->nCounter)
					delete m_pPointer;
				m_pPointer = NULL;
			}
			m_pPointer = ((CMasterPointer<TData>&)oPointer).m_pPointer;
			if(m_pPointer)
				++m_pPointer->nWeaker;
		}
	}

	inline CSlavePointer<TData>& Swap(CSlavePointer<TData>& oPointer)
	{
		if(this != &oPointer && m_pPointer != oPointer.m_pPointer)
			::FOCP_NAME::Swap(m_pPointer, oPointer.m_pPointer);
	}

	inline bool Expired() const
	{
		return (m_pPointer==NULL || m_pPointer->pPointer==NULL);
	}
};

template<typename TData> inline CAutoPointer<TData>::CAutoPointer(const CMasterPointer<TData>& oPointer)
{
	TData* pPointer = ((CMasterPointer<TData>&)oPointer).Detach();
	m_bOwned = pPointer?true:false;
	m_pPointer = pPointer;
}

template<typename TData> inline CAutoPointer<TData>& CAutoPointer<TData>::operator=(const CMasterPointer<TData>& oPointer)
{
	TData* pPointer = ((CMasterPointer<TData>&)oPointer).Detach();
	if(m_bOwned)
	{
		delete m_pPointer;
		m_bOwned =false;
	}
	m_bOwned = pPointer?true:false;
	m_pPointer = pPointer;
	return *this;
}

template<typename TData> inline CMasterPointer<TData>::CMasterPointer(const CSlavePointer<TData>& oPointer)
{
	if(oPointer.m_pPointer && oPointer.m_pPointer->pPointer)
	{
		m_pPointer = ((CSlavePointer<TData>&)oPointer).m_pPointer;
		++m_pPointer->nCounter;
	}
	else
		m_pPointer = NULL;
}

template<typename TData> inline CMasterPointer<TData>& CMasterPointer<TData>::operator=(const CSlavePointer<TData>& oPointer)
{
	if(m_pPointer != ((CSlavePointer<TData>&)oPointer).m_pPointer)
	{
		if(m_pPointer && (!(--m_pPointer->nCounter)))
		{
			if(m_pPointer->pPointer)//可能被Detach
				delete m_pPointer->pPointer;
			if(m_pPointer->nWeaker)
				m_pPointer->pPointer = NULL;
			else
				delete m_pPointer;
		}
		if(oPointer.m_pPointer && oPointer.m_pPointer->pPointer)
		{
			m_pPointer = ((CSlavePointer<TData>&)oPointer).m_pPointer;
			++m_pPointer->nCounter;
		}
		else
			m_pPointer = NULL;
	}
	return *this;
}

FOCP_END();

#endif //_Afc_Pointer_Hpp_
