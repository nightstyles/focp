
#include "Bits.hpp"
#include "Binary.hpp"

FOCP_BEGIN();

CBits::CBits(uint32 nSize, bool bDefault)
{
	m_nUnit = (nSize+7)>>3;
	m_nSize = nSize;
	if(m_nUnit == 0)
		m_pData = NULL;
	else
	{
		m_pData = new uint8[ m_nUnit ];
		if (bDefault)
			CBinary::MemorySet(m_pData, 0xFF, m_nUnit);
		else
			CBinary::MemorySet(m_pData, 0x00, m_nUnit);
	}
}

CBits::CBits(const CBits &oSrc)
{
	m_nUnit = oSrc.m_nUnit;
	m_nSize = oSrc.m_nSize;
	if(m_nUnit == 0)
		m_pData = NULL;
	else
	{
		m_pData = new uint8[ m_nUnit ];
		CBinary::MemoryCopy(m_pData, oSrc.m_pData, m_nUnit);
	}
}

CBits::~CBits()
{
	if(m_pData)
	{
		delete[] m_pData;
		m_pData = NULL;
	}
}

const CBits& CBits::operator=(const CBits &oSrc)
{
	if(&oSrc != this)
	{
		if(m_pData)
			delete[] m_pData;
		m_nSize  = oSrc.m_nSize;
		m_nSize = oSrc.m_nSize;
		if(m_nUnit == 0)
			m_pData = NULL;
		else
		{
			m_pData = new uint8[ m_nUnit ];
			CBinary::MemoryCopy(m_pData, oSrc.m_pData, m_nUnit);
		}
	}
	return *this;
}

CBits* CBits::Clone() const
{
	return new CBits(*this);
}

uint32 CBits::GetSize() const
{
	return m_nSize;
}

uint32 CBits::Elements() const
{
	uint32 n = 0;
	for(uint32 i=0; i<m_nSize; ++i)
	{
		if(Get(i))
			++n;
	}
	return n;
}

uint32 CBits::First() const
{
	for(uint32 i=0; i<m_nSize; ++i)
		if (Get(i)) return i;
	return (uint32)-1;
}

bool CBits::Get(uint32 nIdx) const
{
	uint32 nFlg = nIdx>>3;
	if(nFlg >= m_nUnit)
		return false;
	return (m_pData[nFlg] & (1<<(nIdx&7))) != 0;
}

void CBits::Set(uint32 nIdx, bool bValue)
{
	uint32 nFlg = nIdx>>3;
	if(nFlg >= m_nUnit)
		return;
	if(bValue)
		m_pData[nFlg] |= (1 << (nIdx&7));
	else
	{
		uint8 mask = 0xFF;
		mask ^= (1 << (nIdx&7));
		m_pData[nFlg] &= mask;
	}
}

void CBits::SetAll(bool bValue)
{
	if(m_nUnit)
	{
		if (bValue)
			CBinary::MemorySet(m_pData, 0xFF, (m_nSize+7)>>3);
		else
			CBinary::MemorySet(m_pData, 0x00, (m_nSize+7)>>3);
	}
}

bool CBits::Equal(const CBits &oSrc) const
{
	if(&oSrc == this)
		return true;
	if (m_nSize != oSrc.m_nSize)
		return false;
	for(uint32 nIdx = 0; nIdx < m_nSize; nIdx++)
	{
		if (Get(nIdx) != oSrc.Get(nIdx))
			return false;
	}
	return true;
}

bool CBits::Include(const CBits& oSrc) const
{
	for(uint32 nIdx = 0; nIdx < m_nSize; nIdx++)
	{
		if(oSrc.Get(nIdx) && !Get(nIdx))
			return false;
	}
	return true;
}

bool CBits::Intersect(const CBits& oSrc) const
{
	for(uint32 nIdx = 0; nIdx < m_nSize; nIdx++)
	{
		if(Get(nIdx) && oSrc.Get(nIdx))
			return true;
	}
	return false;
}

bool CBits::operator[](uint32 nIdx)
{
	return Get(nIdx);
}

CBits& CBits::Not()
{
	for (uint32 i=0; i<m_nUnit; i++)
		m_pData[i] ^= 0xFF;
	return *this;
}

CBits& CBits::And(const CBits &oSrc)
{
	if(this != &oSrc)
	{
		for (uint32 i=0; (i<m_nUnit) && (i<oSrc.m_nUnit); i++)
			m_pData[i] &= oSrc.m_pData[i];
	}
	return *this;
}

CBits& CBits::Or(const CBits &oSrc)
{
	if(this != &oSrc)
	{
		for (uint32 i=0; (i<m_nUnit) && (i<oSrc.m_nUnit); i++)
			m_pData[i] |= oSrc.m_pData[i];
	}
	return *this;
}

CBits& CBits::Xor(const CBits &oSrc)
{
	if(this != &oSrc)
	{
		for (uint32 i=0; (i<m_nUnit) && (i<oSrc.m_nUnit); i++)
			m_pData[i] ^= oSrc.m_pData[i];
	}
	return *this;
}

CBits& CBits::Subtract(const CBits& oSrc)
{
	return And(CBits(oSrc).Not());
}

FOCP_END();
