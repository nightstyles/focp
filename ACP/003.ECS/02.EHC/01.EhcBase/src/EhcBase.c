
#include "EhcBase.h"

CEhcInterface g_oInterface;

EBS_API CEhcInterface* GetEhcInterface()
{
	return &g_oInterface;
}

EBS_API ehc_void MemorySet(ehc_void* ptr, ehc_uint val, ehc_uint size)
{
	ehc_uint i;
	ehc_uchar nVal = (ehc_uchar)(val);
	ehc_uchar* pMem = (ehc_uchar*)(ptr);
	for(i=0; i<size; ++i)
		pMem[i] = nVal;
}

EBS_API ehc_void MemoryCopy(ehc_void* dst, ehc_void* src, ehc_uint size)
{
	ehc_uint i;
	ehc_char* x = (ehc_char*)(dst);
	ehc_char* y = (ehc_char*)(src);
	for(i=0; i<(size); ++i)
		x[i] = y[i];
}

EBS_API ehc_uint StringLength(ehc_char* s)
{
	ehc_uint r = 0;
	while(*s)
	{
		++r;
		++s;
	}
	return r;
}

EBS_API ehc_char* StringFindChar(ehc_char* s, ehc_char c)
{
	while(*s)
	{
		if((*s) == c)
			return s;
		++s;
	}
	return NULL;
}

EBS_API ehc_char* StringFindString(ehc_char* s1, ehc_char* s2)
{
	ehc_uint nLen = StringLength(s2);
	while(s1[0])
	{
		if(!MemoryCompare(s1, s2, nLen))
			break;
		s1++;
	}
	return s1[0]?s1:NULL;
}

EBS_API ehc_void StringCopy(ehc_char* dst, ehc_char* src)
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

EBS_API ehc_void StringCopyN(ehc_char* dst, ehc_char* src, ehc_int nSize)
{
	ehc_int i;
	for(i=0; i<nSize; ++i)
	{
		*dst = *src;
		if(*src == 0)
			break;
		++dst;
		++src;
	}
}

EBS_API ehc_void StringAppend(ehc_char* dst, ehc_char* src)
{
	StringCopy(dst+StringLength(dst), src);
}

EBS_API ehc_int StringCompare(ehc_char* s1, ehc_char* s2)
{
	ehc_int nRet = 0;
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

EBS_API ehc_int StringCaseCompare(ehc_char* s1, ehc_char* s2)
{
	ehc_int nRet = 0;
	while(1)
	{
		ehc_char c1 = *s1, c2=*s2;
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

EBS_API ehc_int StringCompareN(ehc_char* s1, ehc_char* s2, ehc_int nSize)
{
	ehc_int i, nRet = 0;
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

EBS_API ehc_int MemoryCompare(ehc_void* dst, ehc_void* src, ehc_int nSize)
{
	ehc_int i, nRet = 0;
	ehc_uchar* s1 = (ehc_uchar*)dst;
	ehc_uchar* s2 = (ehc_uchar*)src;
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

EBS_API ehc_char* StringDuplicate(ehc_char* s)
{
	ehc_char* s2 = (ehc_char*)g_oInterface.Malloc(StringLength(s)+1);
	StringCopy(s2, s);
	return s2;
}

EBS_API ehc_void PrintErrorV(ehc_bool bWarning, ehc_char* sFormat, va_list oArgList)
{
	g_oInterface.PrintErrorV(g_oInterface.pErrSys, bWarning, sFormat, oArgList);
}

EBS_API ehc_void PrintError(ehc_bool bWarning, ehc_char* sFormat, ...)
{
	va_list a;
	va_start(a, sFormat);
	g_oInterface.PrintErrorV(g_oInterface.pErrSys, bWarning, sFormat, a);
	va_end(a);
}
