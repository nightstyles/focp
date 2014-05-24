
#include "Formatter.hpp"

#ifdef UNIX
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(WINDOWS)
#include <process.h>
#endif

#include <stdlib.h>

FOCP_BEGIN();

union tag_endian
{
	uint16 s;
	uint8 c;
	tag_endian()
	{
		s = 1;
	}
};

ADT_API bool IsSmallEndian()
{
	static tag_endian endian;
	return endian.c!=0;
}

ADT_API bool IsRecursive(void* pData)
{
	char s[sizeof(void*)+sizeof(uint32)];
	*(void**)s = pData;
	BaseCall(AFC_RECURSIVE_LOCK, s);
	return (*(uint32*)(s+sizeof(void*))) != 0;
}

ADT_API void UnRecursive(void* pData)
{
	BaseCall(AFC_RECURSIVE_UNLOCK, pData);
}

ADT_API void SystemLock()
{
	BaseCall(AFC_SYSTEM_LOCK, NULL);
}

ADT_API void SystemUnLock()
{
	BaseCall(AFC_SYSTEM_UNLOCK, NULL);
}

ADT_API void Abort()
{
	abort();
}

ADT_API int32 Random()
{
#ifdef WINDOWS
	return rand();
#else
	return random();
#endif
}

ADT_API void RandomSeed(uint32 nSeed)
{
#ifdef WINDOWS
	srand(nSeed);
#else
	srandom(nSeed);
#endif
}

static void StackAttriTest(CStackAttr* pAttr, uint8 *pHelp)
{
	uint8 nHelp2;
	uint8 nHelp3;

	pAttr->bParaUp = ((uint8*)&pAttr < (uint8*)&pHelp);
	pAttr->bCallDown = (pHelp>(&nHelp2));
	pAttr->bLocalDown = ((&nHelp2) > (&nHelp3));
}

ADT_API void GetStackAttr(CStackAttr& oStackAttr)
{
	uint8 nHelp = 0;
	StackAttriTest(&oStackAttr, &nHelp);
}

ADT_API uint32 GetPid()
{
	return getpid();
}

FOCP_END();
