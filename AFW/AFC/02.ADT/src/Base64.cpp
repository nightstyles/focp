
#include "AdtDef.hpp"
#include "Malloc.hpp"

FOCP_BEGIN();

static const char encodeTable[] =
{
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

#define BASE64TOINT(c) (((c) < 128) ? decodeTable [(c) - 40] : -1)

static const signed char decodeTable[] =
{
	/*
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, */
	-1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,
	7,   8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22,
	23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32,
	33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, -1, -1, -1, -1, -1
};

static int rtBase64EncodeData(uint8* pSrcData, int srcDataSize, uint8** ppDstData)
{
	int i;
	uint8* pDstData;
	int numFullGroups = srcDataSize/3;
	int numBytesInPartialGroup = srcDataSize - 3*numFullGroups;
	int resultLen = 4*((srcDataSize + 2)/3);

	pDstData = *ppDstData = (uint8*) CMalloc::Malloc(resultLen + 1);

	/* Translate all full groups from byte array elements to Base64 */
	for (i = 0; i < numFullGroups; i++)
	{
		int byte0 = *pSrcData++ & 0xff;
		int byte1 = *pSrcData++ & 0xff;
		int byte2 = *pSrcData++ & 0xff;
		*pDstData++ = encodeTable[byte0 >> 2];
		*pDstData++ = encodeTable[((byte0 << 4) & 0x3f) | (byte1 >> 4)];
		*pDstData++ = encodeTable[((byte1 << 2) & 0x3f) | (byte2 >> 6)];
		*pDstData++ = encodeTable[byte2 & 0x3f];
	}

	/* Translate partial group if present */
	if (numBytesInPartialGroup != 0)
	{
		int byte0 = *pSrcData++ & 0xff;
		*pDstData++ = encodeTable[byte0 >> 2];
		if (numBytesInPartialGroup == 1)
		{
			*pDstData++ = encodeTable[(byte0 << 4) & 0x3f];
			*pDstData++ = '=';
			*pDstData++ = '=';
		}
		else
		{
			/* assert numBytesInPartialGroup == 2; */
			int byte1 = *pSrcData++ & 0xff;
			*pDstData++ = encodeTable[((byte0 << 4) & 0x3f) | (byte1 >> 4)];
			*pDstData++ = encodeTable[(byte1 << 2) & 0x3f];
			*pDstData++ = '=';
		}
	}
	return resultLen;
}

static int rtBase64DecodeData (uint8* pSrcData, int srcDataSize, uint8** ppDstData)
{
	int i;
	int numGroups = srcDataSize/4;
	int missingBytesInLastGroup = 0;
	int numFullGroups = numGroups;
	int ch0, ch1, ch2, ch3;
	uint8* pvalue;

	if (4*numGroups != srcDataSize)
		return 0;

	if (srcDataSize != 0)
	{
		if (pSrcData[srcDataSize - 1] == '=')
		{
			missingBytesInLastGroup++;
			numFullGroups--;
		}
		if (pSrcData[srcDataSize - 2] == '=')
			missingBytesInLastGroup++;
	}

	pvalue = *ppDstData = (uint8*)CMalloc::Malloc(3*numGroups - missingBytesInLastGroup + 1);

	/* Translate all full groups from base64 to byte array elements */
	for (i = 0; i < numFullGroups; i++)
	{
		ch0 = BASE64TOINT (*pSrcData);
		pSrcData++;
		ch1 = BASE64TOINT (*pSrcData);
		pSrcData++;
		ch2 = BASE64TOINT (*pSrcData);
		pSrcData++;
		ch3 = BASE64TOINT (*pSrcData);
		pSrcData++;
		if ((ch0 | ch1 | ch2 | ch3) < 0)
		{
			CMalloc::Free(*ppDstData);
			return 0;
		}
		*pvalue++ = (uint8) ((ch0 << 2) | (ch1 >> 4));
		*pvalue++ = (uint8) ((ch1 << 4) | (ch2 >> 2));
		*pvalue++ = (uint8) ((ch2 << 6) | ch3);
	}

	/* Translate partial group, if present */
	if (missingBytesInLastGroup != 0)
	{
		ch0 = BASE64TOINT (*pSrcData);
		pSrcData++;
		ch1 = BASE64TOINT (*pSrcData);
		pSrcData++;
		if ((ch0 | ch1) < 0)
		{
			CMalloc::Free(*ppDstData);
			return 0;
		}
		*pvalue++ = (uint8) ((ch0 << 2) | (ch1 >> 4));

		if (missingBytesInLastGroup == 1)
		{
			ch2 = BASE64TOINT (*pSrcData);
			pSrcData++;
			if (ch2 < 0)
			{
				CMalloc::Free(*ppDstData);
				return 0;
			}
			*pvalue++ = (uint8) ((ch1 << 4) | (ch2 >> 2));
		}
	}
	return pvalue - *ppDstData;
}

ADT_API char* Base64Decode(const char *sBuf, uint32 nLen, uint32 *pNewLen)
{
	uint8* pRet = NULL;
	int nRet = rtBase64DecodeData((uint8*)sBuf, nLen, &pRet);
	if(pNewLen)
		*pNewLen = nRet;
	if(!nRet)
		pRet = NULL;
	else
		pRet[nRet] = 0;
	return (char*)pRet;
}

ADT_API char* Base64Encode(const char *sBuf, uint32 nLen, uint32 *pNewLen)
{
	uint8* pRet = NULL;
	int nRet = rtBase64EncodeData((uint8*)sBuf, nLen, &pRet);
	if(pNewLen)
		*pNewLen = nRet;
	if(pRet)
		pRet[nRet] = 0;
	return (char*)pRet;
}

FOCP_END();
