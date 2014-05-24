
#include "AdtDef.hpp"

#ifndef _ADT_MALLOC_HPP_
#define _ADT_MALLOC_HPP_

FOCP_BEGIN();

class ADT_API CMalloc
{
public:
	static void* Malloc(uint32 nSize);
	static void* Calloc(uint32 nUnit, uint32 nSize);
	static void* Realloc(void* pBuf, uint32 nNewSize);
	static void Free(void* pBuf);
};

FOCP_END();

#endif //_ADT_MALLOC_HPP_
