
#include "Trait.hpp"
#include "Exception.hpp"

#ifndef _ADT_ANY_HPP_
#define _ADT_ANY_HPP_

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

class ADT_API CAnyPlaceHolder
{
public:
	uint32 m_nType;

public: // structors
	CAnyPlaceHolder();
	virtual ~CAnyPlaceHolder();

public: // queries
	virtual CAnyPlaceHolder * Clone() const;
};

template<typename TValueType> class CAnyHolder : public CAnyPlaceHolder
{
public: // structors
	inline CAnyHolder()
	{
	}

	inline CAnyHolder(const TValueType & oValue)
		:m_oHeld(oValue)
	{
	}

	inline CAnyHolder(const CAnyHolder<TValueType>& oSrc)
		:m_oHeld(oSrc.m_oHeld)
	{
		m_nType=oSrc.m_nType;
	}

	inline virtual CAnyPlaceHolder * Clone() const
	{
		return new CAnyHolder<TValueType>(*this);
	}

	TValueType m_oHeld;
};

FOCP_DETAIL_END();

template<typename T> struct CNull
{
	inline CNull()
	{
	}

	inline CNull(const CNull<T> &)
	{
	}

	inline operator T*() const
	{
		return NULL;
	}

	inline operator const T*() const
	{
		return NULL;
	}
};

//含有模版成员函数的类不可以输出，即不能携带ADT_API修饰符
//如果该类在以动态库的方式提供，需要以内联方式实现所有成员函数
class CAny
{
private:
	FOCP_DETAIL_NAME::CAnyPlaceHolder* m_pOntent;

public:
	inline CAny():m_pOntent(NULL)
	{
	}

	template<typename TValueType> inline CAny(const CNull<TValueType>&)
		:m_pOntent(new FOCP_DETAIL_NAME::CAnyHolder<TValueType>())
	{
	}

	template<typename TValueType> inline CAny(const TValueType & oVal)
		:m_pOntent(new FOCP_DETAIL_NAME::CAnyHolder<TValueType>(oVal))
	{
	}

	inline CAny(const CAny & oSrc)
		: m_pOntent(oSrc.m_pOntent? oSrc.m_pOntent->Clone() : NULL)
	{
	}

	inline virtual ~CAny()
	{
		Clear();
	}

	template<typename TValueType> inline CAny & operator=(const TValueType & oVal)
	{
		CAny(oVal).Swap(*this);
		return *this;
	}

	inline CAny & operator=(const CAny &oSrc)
	{
		if(m_pOntent != oSrc.m_pOntent)
		{
			if(m_pOntent)
				delete m_pOntent;
			m_pOntent = oSrc.m_pOntent? oSrc.m_pOntent->Clone() : NULL;
		}
		return *this;
	}

	inline bool Empty() const
	{
		return !m_pOntent;
	}

	inline void Clear()
	{
		if(m_pOntent)
		{
			delete m_pOntent;
			m_pOntent = NULL;
		}
	}

	inline void SetType(uint32 nType)
	{
		if(nType)
		{
			if(!m_pOntent)
				FocpThrow(CAny::SetType);
			m_pOntent->m_nType = nType;
		}
		else if(m_pOntent)
			FocpThrow(CAny::SetType);
	}

	inline uint32 GetType() const
	{
		if(m_pOntent)
			return m_pOntent->m_nType;
		else
			return 0;
	}

	inline CAny & Swap(CAny & oSrc)
	{
		::FOCP_NAME::Swap(m_pOntent, oSrc.m_pOntent);
		return *this;
	}

	template<typename TValueType> inline TValueType* GetValue(TValueType*p)
	{
		if(m_pOntent == NULL)
			return NULL;
		TValueType &oVal = ((FOCP_DETAIL_NAME::CAnyHolder<TValueType>*)m_pOntent)->m_oHeld;
		return &oVal;
	}
};

template<typename TValueType> TValueType& any_cast(CAny &oAny)
{
	TValueType* pVal = oAny.GetValue((TValueType*)NULL);
	return *pVal;
}

template<typename TValueType> const TValueType& any_cast(const CAny &oAny)
{
	const TValueType* pVal = oAny.GetValue((TValueType*)NULL);
	return *pVal;
}

template<typename TValueType> TValueType* any_cast(CAny *pAny)
{
	if(pAny == NULL)
		return NULL;
	return &any_cast<TValueType>(*pAny);
}

template<typename TValueType> const TValueType* any_cast(const CAny* pAny)
{
	if(pAny == NULL)
		return NULL;
	return &any_cast<TValueType>(*pAny);
}

FOCP_END();

#endif
