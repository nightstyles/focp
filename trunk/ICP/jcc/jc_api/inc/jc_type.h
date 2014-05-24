
#ifndef _jc_type_h_
#define _jc_type_h_

#include "jc_api.h"

#ifdef MSVC
	#pragma warning(disable:4786)
	#pragma warning(disable:4355)
#endif

#ifndef NULL
	#ifndef __cplusplus
		#define NULL ((void const*)0)
	#else
		#define NULL (0)
	#endif
#endif

#ifndef JC_MAX_PATH
	#define JC_MAX_PATH 512
#endif

#ifndef EOF
#define EOF (-1)
#endif

#define JC_VERSION0 1
#define JC_VERSION1 0
#define JC_VERSION2 0

#define MAX_JC_UCHAR ((jc_uchar)(jc_char)(-1))
#define MAX_JC_USHORT ((jc_ushort)(jc_short)(-1))
#define MAX_JC_UINT ((jc_uint)(jc_int)(-1))
#define MAX_JC_ULONG ((jc_ulong)(jc_long)(-1))

#define MAX_JC_CHAR ((jc_char)(MAX_JC_UCHAR>>1))
#define MAX_JC_SHORT ((jc_short)(MAX_JC_USHORT>>1))
#define MAX_JC_INT ((jc_int)(MAX_JC_UINT>>1))
#define MAX_JC_LONG ((jc_long)(MAX_JC_ULONG>>1))

#define MIN_JC_CHAR ((jc_char)(~(MAX_JC_UCHAR>>1)))
#define MIN_JC_SHORT ((jc_short)(~(MAX_JC_USHORT>>1)))
#define MIN_JC_INT ((jc_int)(~(MAX_JC_UINT>>1)))
#define MIN_JC_LONG ((jc_long)(~(MAX_JC_ULONG>>1)))

#define IsBit32() (sizeof(jc_void*)==sizeof(jc_int))
#define IsBit64() (sizeof(jc_void*)==sizeof(jc_long))

#define New(A) (A*)g_oInterface.Calloc(sizeof(A), 1)
#define New2(A,n) (A*)g_oInterface.Calloc(sizeof(A), n)

jc_void MemorySet(jc_void* ptr, jc_uint val, jc_uint size);
jc_void MemoryCopy(jc_void* dst, jc_void* src, jc_uint size);
jc_int MemoryCompare(jc_void* dst, jc_void* src, jc_int nSize);

jc_uint StringLength(const jc_char* s);
jc_void StringCopy(jc_char* dst, const jc_char* src);
jc_void StringCopyN(jc_char* dst, const jc_char* src, jc_int nSize);
jc_int StringCompare(const jc_char* s1, const jc_char* s2);
jc_int StringCaseCompare(const jc_char* s1, const jc_char* s2);
jc_int StringCompareN(const jc_char* s1, const jc_char* s2, jc_int nSize);
jc_void StringAppend(jc_char* dst, const jc_char* src);
jc_char* StringFindChar(jc_char* s, jc_char c);
jc_char* StringDuplicate(const jc_char* s);

#define MaxAlign() (sizeof(jc_ulong))
#define JC_MAX(a,b) ((a)>(b)?(a):(b))
#define JC_MIN(a,b) ((a)>(b)?(b):(a))

extern CJcInterface g_oInterface;

#endif
