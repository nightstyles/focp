
#ifndef _jc_api_h_
#define _jc_api_h_

//---------------------------------------
// 本系统内存管理公约：
//		认为内存一定够用，不考虑内存分配
// 失败的情况，即分配返回的指针不用判断是
// 否为空。
//---------------------------------------

//---------------------------------------
#ifdef MSVC
#define JC_INT8 char
#define JC_INT16 short
#define JC_INT32 int
#define JC_INT64 __int64
#endif

#ifdef TYJC
#define JC_INT8 char
#define JC_INT16 short
#define JC_INT32 int
#define JC_INT64 long
#endif

#ifdef FOCP_GCC
#define JC_INT8 char
#define JC_INT16 short
#define JC_INT32 int
#define JC_INT64 long long
#endif

//---------------------------------------

#if defined(WINDOWS) && !defined(TYJC)
	#define JC_API __declspec(dllexport)
	#define JC_CALL __stdcall
#else
#ifdef TYJC
	#define JC_API extern share
#else
	#define JC_API
#endif
	#define JC_CALL
#endif

#ifdef TYJC
typedef struct {
        char *a0;       /* pointer to first argument */
        unsigned int offset;    /* byte offset of next parameter */
		unsigned int rewind;	   /* old offset */
} va_list;
extern host void __builtin_va_start(va_list* pAp, void* pArg);
extern share void* __builtin_va_arg(va_list* pAp, unsigned int nTypeSize, unsigned int nAlignSize);
#define va_arg(ap, type) __builtin_va_arg(&ap, sizeof(type), alignof(type))
#define va_start(ap,v) do{__builtin_va_start(&ap, &v);va_arg(ap, v);ap.rewind=ap.offset;}while(0)
#define va_rewind(ap) do{ap.offset=ap.rewind;}while(0)
#define va_end(ap) do{ap.offset=0xFFFFFFFF;}while(0)
#else
#include <stdarg.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

//---------------------------------------
//	JC_INT8 ~ JC_INT64 需要由外部定义成宏
//---------------------------------------
typedef void jc_void;
typedef float jc_float;
typedef double jc_double;

typedef signed JC_INT8 jc_char;
typedef unsigned JC_INT8 jc_uchar;
typedef signed JC_INT16 jc_short;
typedef unsigned JC_INT16 jc_ushort;
typedef signed JC_INT32 jc_int;
typedef unsigned JC_INT32 jc_uint;
typedef signed JC_INT64 jc_long;
typedef unsigned JC_INT64 jc_ulong;

typedef int jc_bool;
enum {False=0, True=1};

typedef va_list JcArgList;
#ifdef UNIX
	#define JcArgStart(a, f) va_start(a)
#else
	#define JcArgStart(a, f) va_start(a,f)
#endif
#define JcGetArg(ap, type) va_arg(ap, type)
#define JcArgEnd(ap) va_end(ap)

//---------------------------------------
//	Interface define
//---------------------------------------
typedef jc_void* (JC_CALL* FMalloc)(jc_uint nSize);
typedef jc_void (JC_CALL* FFree)(jc_void* pMem);
typedef jc_void* (JC_CALL* FRealloc)(jc_void* pOld, jc_uint nNewSize);
typedef jc_void* (JC_CALL* FCalloc)(jc_uint nUnitNum, jc_uint nUnitSize);

typedef jc_double (JC_CALL* FCeil)(jc_double x);
typedef jc_double (JC_CALL* FStringToDouble)(const jc_char *nptr, jc_char **endptr, jc_bool *pError);

typedef jc_char* (JC_CALL* FGetCurrentPath)(jc_char* pPath, jc_int nPathLen);
typedef jc_void* (JC_CALL* FOpenFile)(const jc_char* sFileName, const jc_char* sMode);
typedef jc_void (JC_CALL* FCloseFile)(jc_void* fp);
typedef jc_uint (JC_CALL* FGetFileSize)(jc_void* fp);
typedef jc_char* (JC_CALL* FGetLine)(jc_char* sLine, jc_int nLineLength, jc_void* fp);
typedef jc_int (JC_CALL* FWriteFile)(jc_void* fp, jc_char* pBuf, jc_int nBufLen);

typedef jc_bool (JC_CALL* FIsSpace)(jc_int c);
typedef jc_bool (JC_CALL* FIsAlpha)(jc_int c);
typedef jc_bool (JC_CALL* FIsAlnum)(jc_int c);
typedef jc_bool (JC_CALL* FIsDigit)(jc_int c);
typedef jc_bool (JC_CALL* FIsPunct)(jc_int c);
typedef jc_bool (JC_CALL* FIsPrint)(jc_int c);

typedef jc_void (JC_CALL* FGetTime)(jc_char* sTime);
typedef jc_void (JC_CALL* FGetDate)(jc_char* sDate);

typedef jc_void (JC_CALL* FPrintErrorV)(jc_bool bWarning, const jc_char* sFormat, JcArgList oArgList);
typedef jc_void (JC_CALL* FPrintError)(jc_bool bWarning, const jc_char* sFormat, ...);
typedef jc_void (JC_CALL* FFormatPrint)(jc_char* sBuf, const jc_char* sFormat, ...);
typedef jc_void (JC_CALL* FFormatPrintV)(jc_char* sBuf, const jc_char* sFormat, JcArgList oArgList);
typedef jc_void (JC_CALL* FAbort)();

typedef jc_void* (JC_CALL *FLoadLibrary)(const jc_char* pLibName);
typedef jc_void (JC_CALL *FFreeLibrary)(jc_void* pLibrary);
typedef jc_void* (JC_CALL *FFindSymbol)(jc_void* pLibrary, const jc_char* pSymbolName);

typedef struct CJcInterface
{
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
	FPrintError PrintError;
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

	//动态库函数
	FLoadLibrary LoadLibrary;
	FFreeLibrary FreeLibrary;
	FFindSymbol FindSymbol;
}CJcInterface;

/*
	"-H" support host symbol
	"-S" bSupportShareSymbol
	"-cpara" C File Name
	"-opara" object/library/executable FileName and object file list
	"-epara" entry symbol name
	"-Ipara" include path
	"-Dpara" macro define
	"-Lpara" library path
	"-llib" library file
	"-hlib" host library file
*/

JC_API jc_int JC_CALL JcCompile(jc_int argc, jc_char* argv[]);

#ifdef __cplusplus
}
#endif

#endif
