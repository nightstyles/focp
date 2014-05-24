
#include "Binary.hpp"
#include "Malloc.hpp"

FOCP_BEGIN();

CBinary::~CBinary()
{
	Clear();
}

CBinary::CBinary()
{
	m_nMode = 0;
	m_nCapacity = 0;
	m_nSize = 0;
	m_pData = NULL;
}

CBinary::CBinary(uint32 nBufSize)
{
	m_nMode = 1;
	m_nCapacity = nBufSize;
	m_nSize = nBufSize;
	m_pData = (uint8*)(m_nSize?CMalloc::Malloc(m_nSize):NULL);
}

CBinary::CBinary(uint8* pData, uint32 nDataLen)
{
	m_nMode = 2;
	m_nCapacity = nDataLen;
	m_nSize = m_nCapacity;
	m_pData = pData;
}

CBinary::CBinary(const CBinary& oSrc)
{
	m_nMode = oSrc.m_nMode;
	m_nCapacity = oSrc.m_nCapacity;
	m_nSize = oSrc.m_nSize;
	m_pData = oSrc.m_pData;
	if(m_nMode != 2 && m_nSize)
	{
		m_pData = (uint8*)CMalloc::Malloc(m_nSize);
		MemoryCopy(m_pData, oSrc.m_pData, m_nSize);
	}
}

CBinary::CBinary(const CBinary& oSrc, uint32 nOff, uint32 nSize)
{
	m_nMode = 0;
	m_nCapacity = 0;
	m_nSize = 0;
	m_pData = NULL;

	if(nOff >= oSrc.m_nSize)
		nOff = oSrc.m_nSize;
	uint32 nRest = oSrc.m_nSize - nOff;
	if(nSize > nRest)
		nSize = nRest;
	if(nSize)
	{
		m_pData = (uint8*)CMalloc::Malloc(nSize);
		MemoryCopy(m_pData, oSrc.m_pData+nOff, nSize);
		m_nCapacity = nSize;
		m_nSize = nSize;
	}
}

CBinary& CBinary::operator=(const CBinary& oStr)
{
	if(this != &oStr)
	{
		CBinary o(oStr);
		Swap(o);
	}
	return *this;
}

void* CBinary::CharOfMemory(const void* pBuf, uint8 nVal, uint32 nSize)
{
	uint8* pShift = (uint8*)pBuf;
	for(uint32 i=0; i<nSize; ++i, ++pShift)
		if(nVal == (*pShift))
			return (void*)pShift;
	return NULL;
}

int32 CBinary::MemoryCompare(const void* pBuf1, const void* pBuf2, uint32 nSize, bool bSensitive)
{
	uint8* pShift1 = (uint8*)pBuf1;
	uint8* pShift2 = (uint8*)pBuf2;
	if(bSensitive)
	{
		for(uint8 i=0; i<nSize; ++i, ++pShift1, ++pShift2)
		{
			if((*pShift1) > (*pShift2))
				return 1;
			if((*pShift1) < (*pShift2))
				return -1;
		}
	}
	else
	{
		for(uint8 i=0; i<nSize; ++i, ++pShift1, ++pShift2)
		{
			uint8 c1 = *pShift1;
			uint8 c2 = *pShift2;
			if(c1 >= 'a' && c1 <= 'z')
				c1 -= 'a' - 'A';
			if(c2 >= 'a' && c2 <= 'z')
				c2 -= 'a' - 'A';
			if(c1 > c2)
				return 1;
			if(c1 < c2)
				return -1;
		}
	}
	return 0;
}

void* CBinary::MemoryCopy(void* pDst, const void* pSrc, uint32 nSize)
{
	return ArrayCopy((uint8*)pDst, (uint8*)pSrc, nSize);
}

void* CBinary::MemorySet(void* pDst, uint8 nVal, uint32 nSize)
{
	return ArraySet((uint8*)pDst, nVal, nSize);
}

void CBinary::Clear()
{
	if(m_nMode < 2 && m_pData)
		CMalloc::Free(m_pData);
	m_nMode = 0;
	m_nCapacity = 0;
	m_nSize = 0;
	m_pData = NULL;
}

uint32 CBinary::GetSize() const
{
	return m_nSize;
}

void CBinary::SetSize(uint32 nSize)
{
	if(m_nMode || nSize == m_nSize)
		return;
	if(nSize > m_nSize)
	{
		Grow(nSize - m_nSize);
		m_nSize = nSize;
	}
	else if(nSize == 0)
		Clear();
	else
	{
		m_nSize = nSize;
		UnGrow();
	}
}

uint32 CBinary::GetCapacity() const
{
	return m_nCapacity;
}

void CBinary::SetCapacity(uint32 nCapacity)
{
	if(m_nMode || nCapacity == m_nCapacity)
		return;
	if(nCapacity == 0)
		Clear();
	else
	{
		uint8 * pData = (uint8*)CMalloc::Malloc(nCapacity);
		uint32 nCopySize = nCapacity;
		if(nCopySize > m_nSize)
			nCopySize = m_nSize;
		if(nCopySize)
			MemoryCopy(pData, m_pData, nCopySize);
		if(m_pData)
			CMalloc::Free(m_pData);
		m_pData = pData;
		m_nCapacity = nCapacity;
		if(m_nSize > m_nCapacity)
			m_nSize = m_nCapacity;
	}
}

uint8* CBinary::GetData() const
{
	return m_pData;
}

CBinary& CBinary::Bind(uint8* pData, uint32 nDataLen)
{
	if(m_pData && pData >= (uint8*)m_pData && pData < (uint8*)m_pData + m_nCapacity)
		FocpAbort(("bind to the current object"));
	CBinary o(pData, nDataLen);
	Swap(o);
	return *this;
}

uint8* CBinary::Detach(uint32 &nSize, uint32 &nCapacity)
{
	uint8* pData = (uint8*)m_pData;
	nSize = m_nSize;
	nCapacity = m_nCapacity;
	m_nMode = 0;
	m_nCapacity = 0;
	m_nSize = 0;
	m_pData = NULL;
	return pData;
}

CBinary& CBinary::Write(uint32 nOff, const CBinary& oSrc)
{
	return Write(nOff, oSrc.m_pData, oSrc.m_nSize);
}

CBinary& CBinary::Write(uint32 nOff, const uint8* pData, uint32 nDataLen)
{
	uint32 nOff2 = (uint32)-1;
	if(m_pData && pData >= m_pData && pData < m_pData + m_nSize)
	{
		nOff2 = (pData - m_pData);
		if(m_nSize - nOff2 < nDataLen)
			FocpAbort(( "read memory beyond the border" ));
	}
	if(m_pData && pData < m_pData && pData + nDataLen > m_pData)
	{
		if(m_nMode != 2)
			FocpAbort(( "read memory beyond the border" ));
	}
	if(nOff >= m_nSize)
		nOff = m_nSize;
	uint32 nRest = m_nSize - nOff;
	if(nDataLen > nRest)
	{
		if(m_nMode)
			FocpAbort(( "fixed buffer cann't extend data" ));
		else
		{
			nRest =  nDataLen - nRest;
			Grow(nRest);
			m_nSize += nRest;
			if(nOff2 != (uint32)(-1))
				pData = m_pData + nOff2;
		}
	}
	if(nDataLen)
		MemoryCopy(m_pData+nOff, pData, nDataLen);
	return *this;
}

CBinary& CBinary::WriteByte(uint32 nOff, uint8 nData)
{
	return Write(nOff, &nData, 1);
}

CBinary& CBinary::WriteWord(uint32 nOff, uint16 nData)
{
	nData = U16Code(nData);
	return Write(nOff, (const uint8*)&nData, sizeof(nData));
}

CBinary& CBinary::WriteDWord(uint32 nOff, uint32 nData)
{
	nData = U32Code(nData);
	return Write(nOff, (const uint8*)&nData, sizeof(nData));
}

CBinary& CBinary::WriteQWord(uint32 nOff, uint64 nData)
{
	nData = U64Code(nData);
	return Write(nOff, (const uint8*)&nData, sizeof(nData));
}

CBinary& CBinary::WriteFloat(uint32 nOff, float nData)
{
	return WriteDWord(nOff, *(uint32*)&nData);
}

CBinary& CBinary::WriteDouble(uint32 nOff, double nData)
{
	return WriteQWord(nOff, *(uint64*)&nData);
}

uint32 CBinary::Read(uint32 nOff, uint8* pData, uint32 nDataLen)const
{
	if(nOff >= m_nSize)
		nOff = m_nSize;
	uint32 nRest = m_nSize - nOff;
	if(nDataLen > nRest)
		nDataLen = nRest;
	if(nDataLen)
	{
		if(m_pData && pData >= m_pData && pData < m_pData + m_nSize)
		{
			uint32 nOff2 = (pData - m_pData);
			if(m_nSize - nOff2 < nDataLen)
				FocpAbort(( "write memory beyond the border" ));
		}
		if(m_pData && pData < m_pData && pData + nDataLen > m_pData)
		{
			if(m_nMode != 2)
				FocpAbort(( "write memory beyond the border" ));
		}
		MemoryCopy(pData, m_pData+nOff, nDataLen);
	}
	return nDataLen;
}

bool CBinary::ReadByte(uint32 nOff, uint8 &nData)const
{
	uint32 nLen = Read(nOff, &nData, sizeof(nData));
	return nLen == sizeof(nData);
}

bool CBinary::ReadWord(uint32 nOff, uint16 &nData)const
{
	uint32 nLen = Read(nOff, (uint8*)&nData, sizeof(nData));
	if(nLen != sizeof(nData))
		return false;
	nData = U16Code(nData);
	return true;
}

bool CBinary::ReadDWord(uint32 nOff, uint32 &nData)const
{
	uint32 nLen = Read(nOff, (uint8*)&nData, sizeof(nData));
	if(nLen != sizeof(nData))
		return false;
	nData = U32Code(nData);
	return true;
}

bool CBinary::ReadQWord(uint32 nOff, uint64 &nData)const
{
	uint32 nLen = Read(nOff, (uint8*)&nData, sizeof(nData));
	if(nLen != sizeof(nData))
		return false;
	nData = U64Code(nData);
	return true;
}

bool CBinary::ReadFloat(uint32 nOff, float &nData)const
{
	uint32 nData2;
	bool bRet = ReadDWord(nOff, nData2);
	if(bRet)
		nData = *(float*)&nData2;
	return bRet;
}

bool CBinary::ReadDouble(uint32 nOff, double &nData)const
{
	uint64 nData2;
	bool bRet = ReadQWord(nOff, nData2);
	if(bRet)
		nData = *(double*)&nData2;
	return bRet;
}

CBinary& CBinary::Remove(uint32 nOff, uint32 nSize)
{
	if(m_nMode)
		FocpAbort(( "fixed buffer cann't erase data" ));
	if(nOff >= m_nSize)
		nOff = m_nSize;
	uint32 nRest = m_nSize - nOff;
	if(nSize > nRest)
		nSize = nRest;
	if(nSize)
	{
		uint32 nMoveSize = nRest-nSize;
		if(nMoveSize)
			MemoryCopy(m_pData+nOff, m_pData+nOff+nSize, nMoveSize);
		m_nSize = m_nSize - nSize;
		UnGrow();
	}
	return *this;
}

bool CBinary::Equal(const CBinary& oSrc) const
{
	if(this == &oSrc)
		return true;
	return Compare(oSrc.m_pData, oSrc.m_nSize)==0;
}

bool CBinary::Equal(const uint8* pData, uint32 nDataLen) const
{
	return Compare(pData, nDataLen)==0;
}

int32 CBinary::Compare(const CBinary& oSrc) const
{
	if(this == &oSrc)
		return 0;
	return Compare(oSrc.m_pData, oSrc.m_nSize);
}

int32 CBinary::Compare(const uint8* pData, uint32 nDataLen) const
{
	int32 nRet=0;
	uint32 nCmpSize = nDataLen;
	if(nCmpSize > m_nSize)
		nCmpSize = m_nSize;
	if(nCmpSize)
		nRet = MemoryCompare(m_pData, pData, nCmpSize);
	if(nRet == 0)
	{
		if(nDataLen > m_nSize)
			nRet = -1;
		else if(nDataLen < m_nSize)
			nRet = 1;
	}
	return nRet;
}

uint8* CBinary::FindTag(uint16 nTag, uint16 &nTagLen, uint32 nOff) const
{
	uint16 nTag2;

	while(true)
	{
		if(ReadWord(nOff, nTag2) == false)
			return NULL;
		nOff += sizeof(nTag);
		if(ReadWord(nOff, nTagLen) == false)
			return NULL;
		nOff += sizeof(nTagLen);
		if(nTag2 == nTag)
			return (uint8*)m_pData + nOff;
		nOff += nTagLen;
	}

	return NULL;
}

uint8* CBinary::WalkTag(uint16 &nTag, uint16 &nTagLen, uint32 &nOff) const
{
	uint32 nIdx = nOff;
	if(ReadWord(nIdx, nTag) == false)
		return NULL;
	nIdx += sizeof(nTag);
	if(ReadWord(nIdx, nTagLen) == false)
		return NULL;
	nIdx += sizeof(nTagLen);
	uint32 nRest = m_nSize - nIdx;
	if(nRest < nTagLen)
		return NULL;
	nOff = nIdx + nTagLen;
	return (uint8*)m_pData + nIdx;
}

bool CBinary::GetByteTag(uint8* pTag, uint16 nTagLen, uint8 &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	CBinary::MemoryCopy(&nData, pTag, sizeof(nData));
	return true;
}

bool CBinary::GetWordTag(uint8* pTag, uint16 nTagLen, uint16 &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	MemoryCopy(&nData, pTag, sizeof(nData));
	nData = U16Code(nData);
	return true;
}

bool CBinary::GetDWordTag(uint8* pTag, uint16 nTagLen, uint32 &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	MemoryCopy(&nData, pTag, sizeof(nData));
	nData = U32Code(nData);
	return true;
}

bool CBinary::GetQWordTag(uint8* pTag, uint16 nTagLen, uint64 &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	MemoryCopy(&nData, pTag, sizeof(nData));
	nData = U64Code(nData);
	return true;
}

bool CBinary::GetFloatTag(uint8* pTag, uint16 nTagLen, float &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	MemoryCopy(&nData, pTag, sizeof(nData));
	nData = FloatCode(nData);
	return true;
}

bool CBinary::GetDoubleTag(uint8* pTag, uint16 nTagLen, double &nData)//pTag from FindTag or WalkTag
{
	if((pTag == NULL) || (nTagLen != sizeof(nData)))
		return false;
	MemoryCopy(&nData, pTag, sizeof(nData));
	nData = DoubleCode(nData);
	return true;
}

bool CBinary::FindByteTag(uint16 nTag, uint8 &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetByteTag(pTag, nTagLen, nData);
}

bool CBinary::FindWordTag(uint16 nTag, uint16 &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetWordTag(pTag, nTagLen, nData);
}

bool CBinary::FindDWordTag(uint16 nTag, uint32 &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetDWordTag(pTag, nTagLen, nData);
}

bool CBinary::FindQWordTag(uint16 nTag, uint64 &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetQWordTag(pTag, nTagLen, nData);
}

bool CBinary::FindFloatTag(uint16 nTag, float &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetFloatTag(pTag, nTagLen, nData);
}

bool CBinary::FindDoubleTag(uint16 nTag, double &nData, uint32 nOff)
{
	uint16 nTagLen;
	uint8* pTag = FindTag(nTag, nTagLen, nOff);
	return GetDoubleTag(pTag, nTagLen, nData);
}

uint32 CBinary::AddTag(uint16 nTag, uint8* pData, uint16 nTagLen, uint32 nOff)
{
	WriteWord(nOff, nTag);
	nOff+=sizeof(nTag);
	WriteWord(nOff, nTagLen);
	nOff+=sizeof(nTagLen);
	Write(nOff, pData, nTagLen);
	nOff += nTagLen;
	return nOff;
}

uint32 CBinary::AddByteTag(uint16 nTag, uint8 nData, uint32 nOff)
{
	return AddTag(nTag, &nData, sizeof(nData), nOff);
}

uint32 CBinary::AddWordTag(uint16 nTag, uint16 nData, uint32 nOff)
{
	nData = U16Code(nData);
	return AddTag(nTag, (uint8*)&nData, sizeof(nData), nOff);
}

uint32 CBinary::AddDWordTag(uint16 nTag, uint32 nData, uint32 nOff)
{
	nData = U32Code(nData);
	return AddTag(nTag, (uint8*)&nData, sizeof(nData), nOff);
}

uint32 CBinary::AddQWordTag(uint16 nTag, uint64 nData, uint32 nOff)
{
	nData = U64Code(nData);
	return AddTag(nTag, (uint8*)&nData, sizeof(nData), nOff);
}

uint32 CBinary::AddFloatTag(uint16 nTag, float nData, uint32 nOff)
{
	nData = FloatCode(nData);
	return AddTag(nTag, (uint8*)&nData, sizeof(nData), nOff);
}

uint32 CBinary::AddDoubleTag(uint16 nTag, double nData, uint32 nOff)
{
	nData = DoubleCode(nData);
	return AddTag(nTag, (uint8*)&nData, sizeof(nData), nOff);
}

void CBinary::Swap(CBinary& oSrc)
{
	if(this != &oSrc)
	{
		::FOCP_NAME::Swap(m_nMode, oSrc.m_nMode);
		::FOCP_NAME::Swap(m_nCapacity, oSrc.m_nCapacity);
		::FOCP_NAME::Swap(m_nSize, oSrc.m_nSize);
		::FOCP_NAME::Swap(m_pData, oSrc.m_pData);
	}
}

uint16 CBinary::U16Code(uint16 nData)
{
	uint8* sData = (uint8*)&nData;
	return ((((uint16)sData[0])<<8)&0xFF00) |
		   ((((uint16)sData[1]))&0xFF);
}

uint32 CBinary::U32Code(uint32 nData)
{
	uint8* sData = (uint8*)&nData;
	return (((uint32)sData[0])<<24)|
		   (((uint32)sData[1])<<16)|
		   (((uint32)sData[2])<<8)|
		   (((uint32)sData[3]));
}

uint64 CBinary::U64Code(uint64 nData)
{
	uint8* sData = (uint8*)&nData;
	return (((uint64)sData[0])<<56)|
		   (((uint64)sData[1])<<48) |
		   (((uint64)sData[2])<<40) |
		   (((uint64)sData[3])<<32) |
		   (((uint64)sData[4])<<24)|
		   (((uint64)sData[5])<<16) |
		   (((uint64)sData[6])<<8) |
		   (((uint64)sData[7]));
}

float CBinary::FloatCode(float nData)
{
	uint32 fData = U32Code(*(uint32*)&nData);
	return *(float*)&fData;
}

double CBinary::DoubleCode(double nData)
{
	uint64 fData = U64Code(*(uint64*)&nData);
	return *(double*)&fData;
}

uint32 CBinary::GetDelta(uint32 nCapacity)
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

void CBinary::Grow(uint32 nSize)
{
	uint32 nCapacity = m_nSize + GetDelta(m_nSize);
	uint32 nMod = nCapacity % sizeof(ulong);
	if(nMod)
		nCapacity += sizeof(ulong) - nMod;
	if(nCapacity < m_nSize + nSize)
		nCapacity = m_nSize + nSize;
	SetCapacity(nCapacity);
}

void CBinary::UnGrow()
{
	uint32 nCapacity = m_nSize + GetDelta(m_nSize);
	if(nCapacity < m_nCapacity)
		SetCapacity(nCapacity);
}

CMemoryStreamAllocator::CMemoryStreamAllocator()
{
}

CMemoryStreamAllocator::~CMemoryStreamAllocator()
{
}

CMemStreamNode* CMemoryStreamAllocator::Allocate()
{
	return new CMemStreamNode;
}

void CMemoryStreamAllocator::DeAllocate(CMemStreamNode* pNode)
{
	delete pNode;
}

static CMemoryStreamAllocator* g_pAllocator = NULL;

bool CMemoryStreamAllocator::SetAllocator(CMemoryStreamAllocator* pAllocator)
{
	if(g_pAllocator)
		return false;
	g_pAllocator = pAllocator;
	return true;
}

CMemoryStream::~CMemoryStream()
{
	SetPosition(0);
	Truncate();
}

CMemoryStream::CMemoryStream()
{
	m_pos.size = 0;
	m_pos.pos = 0;
	m_pos.node = NULL;
	m_pos.offset = 0;
	m_sBuf = NULL;
	m_bLocalCode = false;
}

CMemoryStream::CMemoryStream(char* sBuf, uint32 nSize)
{
	m_pos.pos = 0;
	m_pos.node = NULL;
	m_pos.offset = 0;
	m_sBuf = sBuf;
	m_pos.size = nSize;
	m_bLocalCode = false;
}

CMemoryStream::CMemoryStream(const CMemoryStream& oSrc)
{
	m_pos.size = 0;
	m_pos.pos = 0;
	m_pos.node = NULL;
	m_pos.offset = 0;
	m_sBuf = NULL;
	m_bLocalCode = oSrc.m_bLocalCode;

	CMemoryStream& oSrc2 = (CMemoryStream&)oSrc;
	uint32 nPos = oSrc2.GetPosition();
	oSrc2.SetPosition(0);
	CopyFrom(oSrc2);
	oSrc2.SetPosition(nPos);
	SetPosition(nPos);
}

CMemoryStream& CMemoryStream::operator=(const CMemoryStream& oSrc)
{
	if(this != &oSrc)
	{
		SetPosition(0);
		Truncate();
		m_bLocalCode = oSrc.m_bLocalCode;
		CMemoryStream& oSrc2 = (CMemoryStream&)oSrc;
		uint32 nPos = oSrc2.GetPosition();
		oSrc2.SetPosition(0);
		CopyFrom(oSrc2);
		oSrc2.SetPosition(nPos);
		SetPosition(nPos);
	}
	return *this;
}

void CMemoryStream::Swap(CMemoryStream& oSrc)
{
	if(this != &oSrc)
	{
		::FOCP_NAME::Swap(m_sBuf, oSrc.m_sBuf);
		::FOCP_NAME::Swap(m_bLocalCode, oSrc.m_bLocalCode);
		::FOCP_NAME::Swap(m_pos.size, oSrc.m_pos.size);
		::FOCP_NAME::Swap(m_pos.pos, oSrc.m_pos.pos);
		::FOCP_NAME::Swap(m_pos.node, oSrc.m_pos.node);
		::FOCP_NAME::Swap(m_pos.offset, oSrc.m_pos.offset);
	}
}

bool CMemoryStream::SetPosition(uint32 pos)
{
	if(pos > m_pos.size)
		return false;
	Seek(pos - m_pos.pos);
	return true;
}

uint32 CMemoryStream::GetPosition()
{
	return m_pos.pos;
}

char* CMemoryStream::GetBuf(uint32 &nCopySize)
{
	if(m_sBuf)
	{
		nCopySize = m_pos.size - m_pos.pos;
		return m_sBuf+m_pos.pos;
	}
	CMemStreamNode* node = m_pos.node;
	if(node)
	{
		if(node->next)
			nCopySize = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		else
			nCopySize = m_pos.size - m_pos.pos;
		return node->node+m_pos.offset;
	}
	return NULL;
}

uint32 CMemoryStream::GetSize()
{
	return m_pos.size;
}

bool CMemoryStream::Align(uint32 nSize, bool bWrite, uint32 nBasePos)
{
	uint32 nMod = (nBasePos + m_pos.pos) % nSize;
	if(nMod)
	{
		nMod = nSize - nMod;
		if(bWrite)
		{
			if(nMod != Fill('\0', nMod))
				return false;
		}
		else
		{
			if(m_pos.size < nMod)
				return false;
			Seek(nMod);
		}
	}
	return true;
}

bool CMemoryStream::Seek(int32 offset)
{
	if(!offset)
		return true;
	int32 pos2 = offset + m_pos.pos;
	if(pos2 < 0 || pos2 > (int32)m_pos.size)
		return false;
	if(m_sBuf)
	{
		m_pos.pos = pos2;
		return true;
	}
	CMemStreamNode* node = m_pos.node;
	if(!node)
		return false;
	int32 off2 = offset + m_pos.offset;
	while(off2 < 0)
	{
		if(!node->prev)
			return false;
		node = node->prev;
		off2 += AFC_STREAM_BLOCKSIZE;
	}
	while(off2 >= AFC_STREAM_BLOCKSIZE)
	{
		if(!node->next)
		{
			if(off2 > AFC_STREAM_BLOCKSIZE)
				return false;
			break;
		}
		else
		{
			node = node->next;
			off2 -= AFC_STREAM_BLOCKSIZE;
		}
	}
	m_pos.node = node;
	m_pos.offset = off2;
	m_pos.pos = pos2;
	return true;
}

void CMemoryStream::Truncate()
{
	if(m_sBuf)
	{
		m_pos.size = m_pos.pos;
		if(!m_pos.pos)
			m_sBuf = NULL;
		return;
	}
	CMemStreamNode * node = m_pos.node, *tmp;
	if(!node)
		return;
	node = node->next;
	while(node)
	{
		tmp = node->next;
		if(g_pAllocator)
			g_pAllocator->DeAllocate(node);
		else
			delete node;
		node = tmp;
	}
	m_pos.node->next = NULL;
	m_pos.size = m_pos.pos;
	if(!m_pos.size && m_pos.node)
	{
		if(g_pAllocator)
			g_pAllocator->DeAllocate(m_pos.node);
		else
			delete m_pos.node;
		m_pos.node = NULL;
	}
}

CMemStreamNode* CMemoryStream::ExtendNode()
{
	CMemStreamNode* node;
	if(g_pAllocator)
		node = g_pAllocator->Allocate();
	else
		node = new CMemStreamNode;
	if(node)
	{
		node->next = NULL;
		node->prev = m_pos.node;
		if(m_pos.node)
			m_pos.node->next = node;
		m_pos.node = node;
		m_pos.offset = 0;
	}
	return node;
}

uint32 CMemoryStream::ExtendSize(uint32 nSize)
{
	if(nSize == 0 || m_sBuf)
		return 0;
	uint32 nlen = nSize;
	SetPosition(m_pos.size);
	uint32 nPos = m_pos.pos;
	CMemStreamNode* node = m_pos.node;
	uint32 rest;
	while(nlen)
	{
		if(!node)
		{
			node = ExtendNode();
			if(!node)
				break;
		}
		rest = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		if(rest > nlen)
			rest = nlen;
		nlen -= rest;
		m_pos.pos += rest;
		m_pos.offset += rest;
		if(m_pos.offset == AFC_STREAM_BLOCKSIZE)
		{
			node = node->next;
			if(node)
			{
				m_pos.node = node;
				m_pos.offset = 0;
			}
		}
	}
	nlen = nSize - nlen;
	m_pos.size += nlen;

	SetPosition(nPos);

	return nlen;
}

uint32 CMemoryStream::Fill(char c, uint32 size)
{
	uint32 nlen = size;
	if(m_sBuf)
	{
		uint32 rest = m_pos.size - m_pos.pos;
		if(rest > nlen)
			rest = nlen;
		CBinary::MemorySet(m_sBuf+m_pos.pos, (uint8)c, rest);
		m_pos.pos += rest;
		return rest;
	}
	CMemStreamNode* node = m_pos.node;
	uint32 rest, nOldPos = m_pos.pos;
	while(nlen)
	{
		if(!node)
		{
			node = ExtendNode();
			if(!node)
				break;
		}
		rest = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		if(rest > nlen)
			rest = nlen;
		CBinary::MemorySet(node->node+m_pos.offset, (uint8)c, rest);
		nlen -= rest;
		m_pos.pos += rest;
		m_pos.offset += rest;
		if(m_pos.offset == AFC_STREAM_BLOCKSIZE)
		{
			node = node->next;
			if(node)
			{
				m_pos.node = node;
				m_pos.offset = 0;
			}
		}
	}
	nlen = size - nlen;
	rest = m_pos.size - nOldPos;
	if(rest < nlen)
		m_pos.size += nlen - rest;
	return nlen;
}

uint32 CMemoryStream::WriteString(const char* s, bool bMask, uint8 c)
{
	uint8 x;
	uint32 rest, nlen = 0;
	if(m_sBuf)
	{
		rest = m_pos.size - m_pos.pos;
		for(; nlen<rest; ++nlen)
		{
			x = *(uint8*)s;
			if(bMask && x)
				x ^= c;
			m_sBuf[m_pos.pos+nlen] = x;
			if(!x)
			{
				++nlen;
				break;
			}
			++s;
		}
		m_pos.pos += nlen;
		return nlen;
	}
	CMemStreamNode* node = m_pos.node;
	uint32 i, nOldPos = m_pos.pos;
	while(true)
	{
		if(!node)
		{
			node = ExtendNode();
			if(!node)
				break;
		}
		rest = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		for(i=0; i<rest; ++i)
		{
			x = *(uint8*)s;
			if(bMask && x)
				x ^= c;
			node->node[m_pos.offset+i] = x;
			if(!x)
			{
				++i;
				break;//写完了
			}
			++s;
		}
		nlen += i;
		m_pos.pos += i;
		m_pos.offset += i;
		if(m_pos.offset == AFC_STREAM_BLOCKSIZE)
		{
			node = node->next;
			if(node)
			{
				m_pos.node = node;
				m_pos.offset = 0;
			}
		}
		if(!x)
			break;//写完了
	}
	rest = m_pos.size - nOldPos;
	if(rest < nlen)
		m_pos.size += nlen - rest;
	return nlen;
}

uint32 CMemoryStream::Write(void* buf, uint32 nbuflen)
{
	uint8* pbuf = (uint8*)buf;
	uint32 nlen = nbuflen;
	if(m_sBuf)
	{
		uint32 rest = m_pos.size - m_pos.pos;
		if(rest > nlen)
			rest = nlen;
		CBinary::MemoryCopy(m_sBuf+m_pos.pos, buf, rest);
		m_pos.pos += rest;
		return rest;
	}
	CMemStreamNode* node = m_pos.node;
	uint32 rest, nOldPos = m_pos.pos;
	while(nlen)
	{
		if(!node)
		{
			node = ExtendNode();
			if(!node)
				break;
		}
		rest = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		if(rest > nlen)
			rest = nlen;
		CBinary::MemoryCopy(node->node+m_pos.offset, pbuf, rest);
		pbuf += rest;
		nlen -= rest;
		m_pos.pos += rest;
		m_pos.offset += rest;
		if(m_pos.offset == AFC_STREAM_BLOCKSIZE)
		{
			node = node->next;
			if(node)
			{
				m_pos.node = node;
				m_pos.offset = 0;
			}
		}
	}
	nlen = nbuflen - nlen;
	rest = m_pos.size - nOldPos;
	if(rest < nlen)
		m_pos.size += nlen - rest;
	return nlen;
}

uint32 CMemoryStream::Read(void* buf, uint32 nbuflen)
{
	uint8* pbuf = (uint8*)buf;
	uint32 nlen = nbuflen;
	if(m_sBuf)
	{
		uint32 rest = m_pos.size - m_pos.pos;
		if(rest > nlen)
			rest = nlen;
		CBinary::MemoryCopy(buf, m_sBuf+m_pos.pos, rest);
		m_pos.pos += rest;
		return rest;
	}
	CMemStreamNode* node = m_pos.node;
	while(nlen && node)
	{
		uint32 rest;
		if(node->next)
			rest = AFC_STREAM_BLOCKSIZE - m_pos.offset;
		else
			rest = m_pos.size - m_pos.pos;
		if(rest > nlen)
			rest = nlen;
		if(rest == 0)
			break;
		CBinary::MemoryCopy(pbuf, node->node+m_pos.offset, rest);
		pbuf += rest;
		nlen -= rest;
		m_pos.pos += rest;
		m_pos.offset += rest;
		if(m_pos.offset == AFC_STREAM_BLOCKSIZE && node->next)
		{
			node = node->next;
			m_pos.node = node;
			m_pos.offset = 0;
		}
	}
	return nbuflen - nlen;
}

void CMemoryStream::LeftTrim()
{
	CMemStreamNode* node = m_pos.node;
	if(node)
	{
		CMemStreamNode* prev = node->prev;
		node->prev = NULL;
		while(prev)
		{
			node = prev;
			prev = node->prev;
			if(g_pAllocator)
				g_pAllocator->DeAllocate(node);
			else
				delete node;
			m_pos.pos -= AFC_STREAM_BLOCKSIZE;
			m_pos.size -= AFC_STREAM_BLOCKSIZE;
		}
	}
}

uint32 CMemoryStream::CopyFrom(CMemoryStream& oSrc, uint32 nCopySize)
{
	if(nCopySize == 0)
		return 0;
	uint32 nRest = oSrc.m_pos.size - oSrc.m_pos.pos;
	if(nCopySize > nRest)
		nCopySize = nRest;
	uint32 nOldCopySize = nCopySize;
	while(nCopySize)
	{
		uint32 nBlockSize;
		char* pBuf = oSrc.GetBuf(nBlockSize);
		if(nBlockSize > nCopySize)
			nBlockSize = nCopySize;
		uint32 nRet = Write(pBuf, nBlockSize);
		if(nRet)
		{
			nCopySize -= nRet;
			oSrc.Seek((int32)nRet);
		}
		if(nRet < nBlockSize)
			break;
	}
	return nOldCopySize - nCopySize;
}

void CMemoryStream::SetLocalCode(bool bLocalCode)
{
	m_bLocalCode = bLocalCode;
}

bool CMemoryStream::IsLocalCode()
{
	return m_bLocalCode;
}

#define ChangeEndian(v) \
do{\
	union tag_endian2\
	{\
		uint32 i;\
		uint8 c;\
	};\
	static tag_endian2 a;\
	static int bUnInit = 1;\
	if(bUnInit)\
	{\
		a.i = 1;\
		bUnInit = 0;\
	}\
	if(a.c)\
	{\
		uint8* s = (uint8*)&(v);\
		uint8* e = s + sizeof(v) - 1;\
		while(s < e)\
		{\
			uint8 t = *s;\
			*s = *e; *e = t;\
			++s; --e;\
		}\
	}\
}while(0)

#define Encode(v, bLocalCode)\
	do{\
		if(!bLocalCode)ChangeEndian(v);\
		uint32 nRet = Write(&v, sizeof(v));\
		if(nRet != sizeof(v))\
		{\
			Seek(-(int32)nRet);\
			return false;\
		}\
		return true;\
	}while(0)

#define Decode(v, bLocalCode)\
	do{\
		uint32 nRet = Read(&v, sizeof(v));\
		if(nRet != sizeof(v))\
		{\
			Seek(-(int32)nRet);\
			return false;\
		}\
		if(!bLocalCode)ChangeEndian(v);\
		return true;\
	}while(0)

bool CMemoryStream::Write(int8 v)
{
	return Write(&v, 1)==1;
}

bool CMemoryStream::Write(int16 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(int32 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(int64 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(uint8 v)
{
	return Write(&v, 1)==1;
}

bool CMemoryStream::Write(uint16 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(uint32 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(uint64 v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(float v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Write(double v)
{
	Encode(v, m_bLocalCode);
}

bool CMemoryStream::Read(int8 &v)
{
	return Read(&v, 1)==1;
}

bool CMemoryStream::Read(int16 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(int32 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(int64 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(uint8 &v)
{
	return Read(&v, 1)==1;
}

bool CMemoryStream::Read(uint16 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(uint32 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(uint64 &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(float &v)
{
	Decode(v, m_bLocalCode);
}

bool CMemoryStream::Read(double &v)
{
	Decode(v, m_bLocalCode);
}

FOCP_END();
