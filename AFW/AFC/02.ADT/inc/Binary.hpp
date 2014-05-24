
#include "AdtDef.hpp"

#ifndef _ADT_BINARY_HPP_
#define _ADT_BINARY_HPP_

FOCP_BEGIN();

class ADT_API CBinary
{
private:
	uint32 m_nMode;//0可变的，1固定的，2外部的
	uint32 m_nCapacity;
	uint32 m_nSize;
	uint8* m_pData;

public:
	virtual ~CBinary();
	CBinary();//可变的
	CBinary(const CBinary& oSrc);
	CBinary(uint32 nBufSize);//内部固定缓冲
	CBinary(uint8* pData, uint32 nDataLen);//外部固定缓冲
	CBinary(const CBinary& oSrc, uint32 nOff, uint32 nSize);//可变的

	CBinary& operator=(const CBinary& oStr);

	void Clear();

	uint32 GetSize() const;
	void SetSize(uint32 nSize);

	uint32 GetCapacity() const;
	void SetCapacity(uint32 nCapacity);

	uint8* GetData() const;

	CBinary& Bind(uint8* pData, uint32 nDataLen);
	uint8* Detach(uint32 &nSize, uint32 &nCapacity);

	CBinary& Write(uint32 nOff, const CBinary& oSrc);
	CBinary& Write(uint32 nOff, const uint8* pData, uint32 nDataLen);

	CBinary& WriteByte(uint32 nOff, uint8 nData);
	CBinary& WriteWord(uint32 nOff, uint16 nData);
	CBinary& WriteDWord(uint32 nOff, uint32 nData);
	CBinary& WriteQWord(uint32 nOff, uint64 nData);
	CBinary& WriteFloat(uint32 nOff, float nData);
	CBinary& WriteDouble(uint32 nOff, double nData);

	uint32 Read(uint32 nOff, uint8* pData, uint32 nDataLen)const;
	bool ReadByte(uint32 nOff, uint8 &nData)const;
	bool ReadWord(uint32 nOff, uint16 &nData)const;
	bool ReadDWord(uint32 nOff, uint32 &nData)const;
	bool ReadQWord(uint32 nOff, uint64 &nData)const;
	bool ReadFloat(uint32 nOff, float &nData)const;
	bool ReadDouble(uint32 nOff, double &nData)const;

	CBinary& Remove(uint32 nOff, uint32 nSize);

	bool Equal(const CBinary& oSrc) const;
	bool Equal(const uint8* pData, uint32 nDataLen) const;

	int32 Compare(const CBinary& oSrc) const;
	int32 Compare(const uint8* pData, uint32 nDataLen) const;

	//TLV
	uint32 AddTag(uint16 nTag, uint8* pData, uint16 nTagLen, uint32 nOff);
	uint32 AddByteTag(uint16 nTag, uint8 nData, uint32 nOff);
	uint32 AddWordTag(uint16 nTag, uint16 nData, uint32 nOff);
	uint32 AddDWordTag(uint16 nTag, uint32 nData, uint32 nOff);
	uint32 AddQWordTag(uint16 nTag, uint64 nData, uint32 nOff);
	uint32 AddFloatTag(uint16 nTag, float nData, uint32 nOff);
	uint32 AddDoubleTag(uint16 nTag, double nData, uint32 nOff);

	uint8* FindTag(uint16 nTag, uint16 &nTagLen, uint32 nOff=0) const;
	uint8* WalkTag(uint16 &nTag, uint16 &nTagLen, uint32 &nOff) const;

	bool GetByteTag(uint8* pTag, uint16 nTagLen, uint8 &nData);//pTag from FindTag or WalkTag
	bool GetWordTag(uint8* pTag, uint16 nTagLen, uint16 &nData);//pTag from FindTag or WalkTag
	bool GetDWordTag(uint8* pTag, uint16 nTagLen, uint32 &nData);//pTag from FindTag or WalkTag
	bool GetQWordTag(uint8* pTag, uint16 nTagLen, uint64 &nData);//pTag from FindTag or WalkTag
	bool GetFloatTag(uint8* pTag, uint16 nTagLen, float &nData);//pTag from FindTag or WalkTag
	bool GetDoubleTag(uint8* pTag, uint16 nTagLen, double &nData);//pTag from FindTag or WalkTag

	bool FindByteTag(uint16 nTag, uint8 &nData, uint32 nOff=0);
	bool FindWordTag(uint16 nTag, uint16 &nData, uint32 nOff=0);
	bool FindDWordTag(uint16 nTag, uint32 &nData, uint32 nOff=0);
	bool FindQWordTag(uint16 nTag, uint64 &nData, uint32 nOff=0);
	bool FindFloatTag(uint16 nTag, float &nData, uint32 nOff=0);
	bool FindDoubleTag(uint16 nTag, double &nData, uint32 nOff=0);

	void Swap(CBinary& oSrc);

	static uint16 U16Code(uint16 nData);
	static uint32 U32Code(uint32 nData);
	static uint64 U64Code(uint64 nData);
	static float FloatCode(float nData);
	static double DoubleCode(double nData);

	template<typename T> inline static T NetCode(T nData)
	{
		switch(sizeof(nData))
		{
		case 2:
			{
				uint16 u = U16Code(*(uint16*)&nData);
				nData = *(T*)&u;
			}
			break;
		case 4:
			{
				uint32 u = U32Code(*(uint32*)&nData);
				nData = *(T*)&u;
			}
			break;
		case 8:
			{
				uint64 u = U64Code(*(uint64*)&nData);
				nData = *(T*)&u;
			}
			break;
		}
		return nData;
	}

	static void* CharOfMemory(const void* pBuf, uint8 nVal, uint32 nSize);//memchr
	static int32 MemoryCompare(const void* pBuf1, const void* pBuf2, uint32 nSize, bool bSensitive=true);//memcmp
	static void* MemoryCopy(void* pDst, const void* pSrc, uint32 nSize);//memmove
	static void* MemorySet(void* pDst, uint8 nVal, uint32 nSize);//memset

private:
	void Grow(uint32 nSize=1);
	void UnGrow();
	uint32 GetDelta(uint32 nCapacity);
};

template<typename TItem> TItem* ItemOfArray(const TItem* pArray, const TItem &nVal, uint32 nSize)
{
	for(uint32 i=0; i<nSize; ++i)
	{
		if(pArray[i] == nVal)
			return pArray + i;
	}
	return NULL;
}

template<typename TItem> int32 ArrayCompare(const TItem* pArray1, const TItem* pArray2, uint32 nSize)
{
	for(uint32 i=0; i<nSize; ++i)
	{
		if(pArray1[i] > pArray2[i])
			return 1;
		if(pArray1[i] < pArray2[i])
			return -1;
	}
	return 0;
}

template<typename TItem> TItem* ArrayCopy(TItem* pDst, const TItem* pSrc, uint32 nSize)
{
	TItem* pRet = pDst;
	if(pDst < pSrc)
	{
		for(uint32 i=0; i<nSize; ++i, ++pDst, ++pSrc)
			*pDst = *pSrc;
	}
	else if(pDst > pSrc)
	{
		pDst += nSize - 1;
		pSrc += nSize - 1;
		for(uint32 i=0; i<nSize; ++i, --pDst, --pSrc)
			*pDst = *pSrc;
	}
	return pRet;
}

template<typename TItem> TItem* ArraySet(TItem* pDst, const TItem &nVal, uint32 nSize)
{
	TItem* pShift = (TItem*)pDst;
	for(uint32 i=0; i<nSize; ++i, ++pShift)
		(*pShift) = nVal;
	return pDst;
}

#define AFC_STREAM_BLOCKSIZE 256

struct CMemStreamNode
{
	CMemStreamNode *prev, *next;
	char node[AFC_STREAM_BLOCKSIZE];
};

struct ADT_API CMemoryStreamAllocator
{
	CMemoryStreamAllocator();
	virtual ~CMemoryStreamAllocator();
	virtual CMemStreamNode* Allocate();
	virtual void DeAllocate(CMemStreamNode* pNode);

	static bool SetAllocator(CMemoryStreamAllocator* pAllocator);
};

struct CMemStreamPos
{
	uint32 size;
	uint32 pos;
	CMemStreamNode * node;
	uint32 offset;
};

class ADT_API CMemoryStream
{
private:
	char * m_sBuf;
	bool m_bLocalCode;
	CMemStreamPos m_pos;

private:
	CMemStreamNode* ExtendNode();

public:
	CMemoryStream();
	CMemoryStream(char* sBuf, uint32 nSize);
	CMemoryStream(const CMemoryStream& oSrc);
	~CMemoryStream();

	CMemoryStream& operator=(const CMemoryStream& oSrc);

	void Swap(CMemoryStream& oSrc);

	bool SetPosition(uint32 pos);
	uint32 GetPosition();

	char* GetBuf(uint32 &nCopySize);

	uint32 GetSize();
	uint32 ExtendSize(uint32 nSize);

	bool Seek(int32 offset);

	bool Align(uint32 nSize, bool bWrite=true, uint32 nBasePos=0);

	void Truncate();
	void LeftTrim();

	uint32 Write(void* buf, uint32 nbuflen);
	uint32 Read(void* buf, uint32 nbuflen);

	uint32 Fill(char c, uint32 size);
	uint32 WriteString(const char* s, bool bMask=false, uint8 c='\0');

	uint32 CopyFrom(CMemoryStream& oSrc, uint32 nCopySize=0xFFFFFFFF);

	void SetLocalCode(bool bLocalCode);
	bool IsLocalCode();

	bool Write(int8 v);
	bool Write(int16 v);
	bool Write(int32 v);
	bool Write(int64 v);
	bool Write(uint8 v);
	bool Write(uint16 v);
	bool Write(uint32 v);
	bool Write(uint64 v);
	bool Write(float v);
	bool Write(double v);

	bool Read(int8 &v);
	bool Read(int16 &v);
	bool Read(int32 &v);
	bool Read(int64 &v);
	bool Read(uint8 &v);
	bool Read(uint16 &v);
	bool Read(uint32 &v);
	bool Read(uint64 &v);
	bool Read(float &v);
	bool Read(double &v);
};

FOCP_END();

#endif //_ADT_BINARY_HPP_
