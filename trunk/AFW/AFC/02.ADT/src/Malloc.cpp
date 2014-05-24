
#include "Malloc.hpp"

#include <stdlib.h>

FOCP_BEGIN();

void* CMalloc::Malloc(uint32 nSize)
{
	return malloc(nSize);
}

void* CMalloc::Calloc(uint32 nUnit, uint32 nSize)
{
	return calloc(nUnit, nSize);
}

void* CMalloc::Realloc(void* pBuf, uint32 nNewSize)
{
	return realloc(pBuf, nNewSize);
}

void CMalloc::Free(void* pBuf)
{
	free(pBuf);
}

FOCP_END();
