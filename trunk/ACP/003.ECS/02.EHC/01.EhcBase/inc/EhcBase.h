
#ifndef _EHC_BASE_H_
#define _EHC_BASE_H_

#ifdef MSVC
#pragma warning(disable:4786)
#pragma warning(disable:4251)
#pragma warning(disable:4355)
#pragma warning(disable:4275)
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define EHC_CALL __stdcall
#else
#define EHC_CALL
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define EHC_EXPORT __declspec(dllexport)
#else
#define EHC_EXPORT
#endif

#if defined(WINDOWS) || defined(CYGWIN_NT)
#define EHC_IMPORT __declspec(dllimport)
#else
#define EHC_IMPORT
#endif

#ifndef EHC_STRING_DEFINE
#define EHC_STRING_DEFINE2(S) #S
#define EHC_STRING_DEFINE(S) EHC_STRING_DEFINE2(S)
#endif

#define EHC_C_BEGIN() extern "C"{
#define EHC_C_END() }

#ifndef NULL
#ifdef __cplusplus
#define NULL 0ul
#else
#define NULL ((void*)0)
#endif
#endif

#ifndef EHC_MAX_PATH
	#define EHC_MAX_PATH FOCP_MAX_PATH
#endif

#define EhcFieldOffset(TType, oFieldName) ((char*)&(((TType*)0x10)->oFieldName)-(char*)0x10)

#include <stdarg.h>
#include <limits.h>

#ifdef MSVC
	#define FOCP_INT64_CONST(a) a##i64
	#define FOCP_UINT64_CONST(a) a##ui64
#else
	#define FOCP_INT64_CONST(a) a##ll
	#define FOCP_UINT64_CONST(a) a##ull
#endif

#ifdef EBS_EXPORTS
#define EBS_API EHC_EXPORT
#else
#define EBS_API EHC_IMPORT
#endif

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

#if (CHAR_MIN==-128)
	typedef char ehc_char;
	typedef unsigned char ehc_uchar;
#elif (SCHAR_MIN == -128)
	typedef signed char ehc_char;
	typedef unsigned char ehc_uchar;
#else
	#error (sizeof(char) != 1)
#endif

#if (SHRT_MIN==-32768)
	typedef short ehc_short;
	typedef unsigned short ehc_ushort;
#else
	#error (sizeof(short) != 2)
#endif

#if (INT_MIN==-2147483648)
	typedef int ehc_int;
	typedef unsigned int ehc_uint;
#elif (LONG_MIN == -2147483648)
	typedef long ehc_int;
	typedef unsigned long ehc_uint;
#else
	#error (sizeof(int) != 4)
#endif

#define EHC_LONG 0

#if ((EHC_LONG == 0) && (LONG_MIN==FOCP_INT64_CONST(-0x8000000000000000)))
	typedef long ehc_long;
	typedef unsigned long ehc_ulong;
	#undef EHC_LONG
	#define EHC_LONG 1
#endif

#if ((EHC_LONG == 0) && defined(LLONG_MIN))
	#if (LLONG_MIN==FOCP_INT64_CONST(-0x8000000000000000))
	typedef long long ehc_long;
	typedef unsigned long long ehc_ulong;
	#undef EHC_LONG
	#define EHC_LONG 1		
	#endif
#endif

#if ((EHC_LONG == 0) && defined(_I64_MIN))
	#if (_I64_MIN==FOCP_INT64_CONST(-0x8000000000000000))
	typedef __int64 ehc_long;
	typedef unsigned __int64 ehc_ulong;
	#undef EHC_LONG
	#define EHC_LONG 1		
	#endif
#endif

#if (EHC_LONG == 0)
	#error (sizeof(long)!=8)
#else
	#undef EHC_LONG
#endif

typedef float ehc_float;
typedef double ehc_double;
typedef void ehc_void;
typedef ehc_uint ehc_bool;

enum {False, True};

#define MAX_EHC_UCHAR ((ehc_uchar)(ehc_char)(-1))
#define MAX_EHC_USHORT ((ehc_ushort)(ehc_short)(-1))
#define MAX_EHC_UINT ((ehc_uint)(ehc_int)(-1))
#define MAX_EHC_ULONG ((ehc_ulong)(ehc_long)(-1))

#define MAX_EHC_CHAR ((ehc_char)(MAX_EHC_UCHAR>>1))
#define MAX_EHC_SHORT ((ehc_short)(MAX_EHC_USHORT>>1))
#define MAX_EHC_INT ((ehc_int)(MAX_EHC_UINT>>1))
#define MAX_EHC_LONG ((ehc_long)(MAX_EHC_ULONG>>1))

#define MIN_EHC_CHAR ((ehc_char)(~(MAX_EHC_UCHAR>>1)))
#define MIN_EHC_SHORT ((ehc_short)(~(MAX_EHC_USHORT>>1)))
#define MIN_EHC_INT ((ehc_int)(~(MAX_EHC_UINT>>1)))
#define MIN_EHC_LONG ((ehc_long)(~(MAX_EHC_ULONG>>1)))

#define New(A) ((A*)GetEhcInterface()->Calloc(sizeof(A), 1))
#define New2(A,n) ((A*)GetEhcInterface()->Calloc(sizeof(A), n))

EBS_API ehc_void MemorySet(ehc_void* ptr, ehc_uint val, ehc_uint size);
EBS_API ehc_void MemoryCopy(ehc_void* dst, ehc_void* src, ehc_uint size);
EBS_API ehc_int MemoryCompare(ehc_void* dst, ehc_void* src, ehc_int nSize);

EBS_API ehc_uint StringLength(ehc_char* s);
EBS_API ehc_void StringCopy(ehc_char* dst, ehc_char* src);
EBS_API ehc_void StringCopyN(ehc_char* dst, ehc_char* src, ehc_int nSize);
EBS_API ehc_int StringCompare(ehc_char* s1, ehc_char* s2);
EBS_API ehc_int StringCaseCompare(ehc_char* s1, ehc_char* s2);
EBS_API ehc_int StringCompareN(ehc_char* s1, ehc_char* s2, ehc_int nSize);
EBS_API ehc_void StringAppend(ehc_char* dst, ehc_char* src);
EBS_API ehc_char* StringFindChar(ehc_char* s, ehc_char c);
EBS_API ehc_char* StringFindString(ehc_char* s1, ehc_char* s2);
EBS_API ehc_char* StringDuplicate(ehc_char* s);

#define MaxAlign() (sizeof(ehc_ulong))
#define EHC_MAX(a,b) ((a)>(b)?(a):(b))
#define EHC_MIN(a,b) ((a)>(b)?(b):(a))

//---------------------------------------
//	Interface define
//---------------------------------------
typedef ehc_void* (*FMalloc)(ehc_uint nSize);
typedef ehc_void (*FFree)(ehc_void* pMem);
typedef ehc_void* (*FRealloc)(ehc_void* pOld, ehc_uint nNewSize);
typedef ehc_void* (*FCalloc)(ehc_uint nUnitNum, ehc_uint nUnitSize);

typedef ehc_double (*FCeil)(ehc_double x);
typedef ehc_double (*FStringToDouble)(ehc_char *nptr, ehc_char **endptr, ehc_bool *pError);

typedef ehc_char* (*FGetCurrentPath)(ehc_char* pPath, ehc_int nPathLen);
typedef ehc_void* (*FOpenFile)(ehc_char* sFileName, ehc_char* sMode);
typedef ehc_void (*FCloseFile)(ehc_void* fp);
typedef ehc_uint (*FGetFileSize)(ehc_void* fp);
//需要支持五种换行风格：\r、\n、\v, \f, \r\n，并统一成\n风格（以'\0'结尾）
typedef ehc_char* (*FGetLine)(ehc_char* sLine, ehc_int nLineLength, ehc_void* fp);
typedef ehc_int (*FWriteFile)(ehc_void* fp, ehc_char* pBuf, ehc_int nBufLen);

typedef ehc_bool (*FIsSpace)(ehc_int c);
typedef ehc_bool (*FIsAlpha)(ehc_int c);
typedef ehc_bool (*FIsAlnum)(ehc_int c);
typedef ehc_bool (*FIsDigit)(ehc_int c);
typedef ehc_bool (*FIsPunct)(ehc_int c);
typedef ehc_bool (*FIsPrint)(ehc_int c);

typedef ehc_void (*FGetTime)(ehc_char* sTime);
typedef ehc_void (*FGetDate)(ehc_char* sDate);

typedef ehc_void (*FFormatPrint)(ehc_char* sBuf, ehc_char* sFormat, ...);
typedef ehc_void (*FFormatPrintV)(ehc_char* sBuf, ehc_char* sFormat, va_list oArgList);

typedef ehc_void (*FPrintErrorV)(ehc_void* pErrSys, ehc_bool bWarning, ehc_char* sFormat, va_list oArgList);

typedef ehc_void (*FAbort)();

typedef struct CEhcInterface
{
	ehc_bool bSupportDigraph;//支持文件双字符组替换
	ehc_bool bSupportTrigraph;//支持文件三字符组替换
	ehc_bool bSupportPreProcOut;//支持预处理输出
	ehc_void* pErrSys;//错误支持系统
	//内存管理接口
	FMalloc Malloc;
	FFree Free;
	FRealloc Realloc;
	FCalloc Calloc;

	//转换函数
	FCeil Ceil;
	FStringToDouble StringToDouble;

	//文件操作接口
	FOpenFile OpenFile;
	FCloseFile CloseFile;
	FGetLine GetLine;
	FGetFileSize GetFileSize;
	FGetCurrentPath GetCurrentPath;
	FWriteFile WriteFile;

	//错误函数
	FPrintErrorV PrintErrorV;
	FFormatPrint FormatPrint;
	FFormatPrintV FormatPrintV;
	FAbort Abort;

	//字符属性接口
	FIsSpace IsSpace;
	FIsAlpha IsAlpha;
	FIsAlnum IsAlnum;
	FIsDigit IsDigit;
	FIsPunct IsPunct;
	FIsPrint IsPrint;

	//时间函数
	FGetTime GetTime;
	FGetDate GetDate;

}CEhcInterface;

EBS_API CEhcInterface* GetEhcInterface();

EBS_API ehc_uint StringLength(ehc_char* s);
EBS_API ehc_void PrintErrorV(ehc_bool bWarning, ehc_char* sFormat, va_list oArgList);
EBS_API ehc_void PrintError(ehc_bool bWarning, ehc_char* sFormat, ...);

#ifdef __cplusplus
EHC_C_END();
#endif

#define EHC_VERSION0 1
#define EHC_VERSION1 0
#define EHC_VERSION2 0

#endif
