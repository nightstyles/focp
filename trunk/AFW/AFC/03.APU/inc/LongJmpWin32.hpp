
#include "ApuDef.hpp"

#if !defined(MSVC)
#include <setjmp.h>
#endif

#ifndef _APU_LONGJMPWIN32_HPP_
#define _APU_LONGJMPWIN32_HPP_

FOCP_BEGIN();

#if defined(MSVC)
typedef struct xJmpBuf
{
	uint32 ebp;
	uint32 ebx;
	uint32 edi;
	uint32 esi;
	uint32 esp;
	uint32 eip;
	uint32 fs0;//for VC6
} CJmpBuf[1];
APU_API int32 FocpSetJmp(CJmpBuf pBuf);
APU_API void FocpLongJmp(CJmpBuf pBuf, int32 nRet);
#endif

struct CBreak
{
#if defined(MSVC)
	CJmpBuf oBreak;
#else
	jmp_buf oBreak;
#endif
};

FOCP_END();

#endif
