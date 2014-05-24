
#include "jc_type.h"

jc_void MemorySet(jc_void* ptr, jc_uint val, jc_uint size)
{
	jc_uint i;
	jc_uchar nVal = (jc_uchar)(val);
	jc_uchar* pMem = (jc_uchar*)(ptr);
	for(i=0; i<size; ++i)
		pMem[i] = nVal;
}

jc_void MemoryCopy(jc_void* dst, const jc_void* src, jc_uint size)
{
	jc_uint i;
	jc_char* x = (jc_char*)(dst);
	jc_char* y = (jc_char*)(src);
	for(i=0; i<(size); ++i)
		x[i] = y[i];
}

jc_uint StringLength(const jc_char* s)
{
	jc_uint r = 0;
	while(*s)
	{
		++r;
		++s;
	}
	return r;
}

jc_char* StringFindChar(jc_char* s, jc_char c)
{
	while(*s)
	{
		if((*s) == c)
			return s;
		++s;
	}
	return NULL;
}

jc_void StringCopy(jc_char* dst, const jc_char* src)
{
	while(1)
	{
		*dst = *src;
		if(*src == 0)
			break;
		++dst;
		++src;
	}
}

jc_void StringCopyN(jc_char* dst, const jc_char* src, jc_int nSize)
{
	jc_int i;
	for(i=0; i<nSize; ++i)
	{
		*dst = *src;
		if(*src == 0)
			break;
		++dst;
		++src;
	}
}

jc_void StringAppend(jc_char* dst, const jc_char* src)
{
	StringCopy(dst+StringLength(dst), src);
}

jc_int StringCompare(const jc_char* s1, const jc_char* s2)
{
	jc_int nRet = 0;
	while(1)
	{
		if((*s1) < (*s2))
		{
			nRet = -1;
			break;
		}
		if((*s1) > (*s2))
		{
			nRet = 1;
			break;
		}
		if(!(*s1) || !(*s2))
			break;
		++s1;
		++s2;
	}
	return nRet;
}

jc_int StringCaseCompare(const jc_char* s1, const jc_char* s2)
{
	jc_int nRet = 0;
	while(1)
	{
		jc_char c1 = *s1, c2=*s2;
		if(c1 > 'a')
			c1 -= 'a' - 'A';
		if(c2 > 'a')
			c2 -= 'a' - 'A';
		if(c1 < c2)
		{
			nRet = -1;
			break;
		}
		if(c1 > c2)
		{
			nRet = 1;
			break;
		}
		if(!c1 || !c2)
			break;
		++s1;
		++s2;
	}
	return nRet;
}

jc_int StringCompareN(const jc_char* s1, const jc_char* s2, jc_int nSize)
{
	jc_int i, nRet = 0;
	for(i=0; i<nSize; ++i)
	{
		if((*s1) < (*s2))
		{
			nRet = -1;
			break;
		}
		if((*s1) > (*s2))
		{
			nRet = 1;
			break;
		}
		if(!(*s1) || !(*s2))
			break;
		++s1;
		++s2;
	}
	return nRet;
}

jc_int MemoryCompare(jc_void* dst, jc_void* src, jc_int nSize)
{
	jc_int i, nRet = 0;
	jc_uchar* s1 = (jc_uchar*)dst;
	jc_uchar* s2 = (jc_uchar*)src;
	for(i=0; i<nSize; ++i)
	{
		if((*s1) < (*s2))
		{
			nRet = -1;
			break;
		}
		if((*s1) > (*s2))
		{
			nRet = 1;
			break;
		}
		++s1;
		++s2;
	}
	return nRet;
}

jc_char* StringDuplicate(const jc_char* s)
{
	jc_char* s2 = (jc_char*)g_oInterface.Malloc(StringLength(s)+1);
	StringCopy(s2, s);
	return s2;
}
