
#include "jc_type.h"

#ifdef TYJC

extern host jc_void* JC_CALL JcMalloc(jc_uint nSize);
extern host jc_void JC_CALL JcFree(jc_void* pMem);
extern host jc_void* JC_CALL JcRealloc(jc_void* pOld, jc_uint nNewSize);
extern host jc_void* JC_CALL JcCalloc(jc_uint nUnitNum, jc_uint nUnitSize);
extern host jc_double JC_CALL JcCeil(jc_double x);
extern host jc_double JC_CALL JcStringToDouble(const jc_char *nptr, jc_char **endptr, jc_bool *pError);
extern host jc_char* JC_CALL JcGetCurrentPath(jc_char* pPath, jc_int nPathLen);
extern host jc_void* JC_CALL JcOpenFile(const jc_char* sFileName, const jc_char* sMode);
extern host jc_void JC_CALL JcCloseFile(jc_void* fp);
extern host jc_uint JC_CALL JcGetFileSize(jc_void* fp);
extern host jc_char* JC_CALL JcGetLine(jc_char* sLine, jc_int nLineLength, jc_void* fp);
extern host jc_int JC_CALL JcWriteFile(jc_void* fp, jc_char* pBuf, jc_int nBufLen);
extern host jc_bool JC_CALL JcIsSpace(jc_int c);
extern host jc_bool JC_CALL JcIsAlpha(jc_int c);
extern host jc_bool JC_CALL JcIsAlnum(jc_int c);
extern host jc_bool JC_CALL JcIsDigit(jc_int c);
extern host jc_bool JC_CALL JcIsPunct(jc_int c);
extern host jc_bool JC_CALL JcIsPrint(jc_int c);
extern host jc_void JC_CALL JcGetTime(jc_char* sTime);
extern host jc_void JC_CALL JcGetDate(jc_char* sTime);
extern host jc_void JC_CALL JcFormatPrintV(jc_char* sBuf, const jc_char* sFormat, JcArgList oArgList);
extern host jc_void JC_CALL JcPrintErrorV(jc_bool bWarning, const jc_char* sFormat, JcArgList oArgList);
extern host jc_void JC_CALL JcAbort();
extern host jc_void* JC_CALL JcLoadLibrary(const jc_char* pLibName);
extern host jc_void JC_CALL JcFreeLibrary(jc_void* pLibrary);
extern host jc_void* JC_CALL JcFindSymbol(jc_void* pLibrary, const jc_char* pSymbolName);
#else

#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <direct.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>

static jc_void* JC_CALL JcMalloc(jc_uint nSize) {return malloc(nSize);}
static jc_void JC_CALL JcFree(jc_void* pMem){free(pMem);}
static jc_void* JC_CALL JcRealloc(jc_void* pOld, jc_uint nNewSize){return realloc(pOld, nNewSize);}
static jc_void* JC_CALL JcCalloc(jc_uint nUnitNum, jc_uint nUnitSize){return calloc(nUnitNum, nUnitSize);}
static jc_double JC_CALL JcCeil(jc_double x){return ceil(x);}
static jc_double JC_CALL JcStringToDouble(const jc_char *nptr, jc_char **endptr, jc_bool *pError)
{
	jc_double d;
	errno = 0;
	d = strtod(nptr, endptr);
	pError[0] = errno?1:0;
	return d;
}
static jc_char* JC_CALL JcGetCurrentPath(jc_char* pPath, jc_int nPathLen){return getcwd(pPath, nPathLen);}
static jc_void* JC_CALL JcOpenFile(const jc_char* sFileName, const jc_char* sMode){return fopen(sFileName, sMode);}
static jc_void JC_CALL JcCloseFile(jc_void* fp){fclose((FILE*)fp);}
static jc_uint JC_CALL JcGetFileSize(jc_void* fp)
{
	jc_uint nRet;
	fseek((FILE*)fp, 0, SEEK_END);
	nRet=ftell((FILE*)fp);
	fseek((FILE*)fp, 0, SEEK_SET);
	return nRet;
}
static jc_char* JC_CALL JcGetLine(jc_char* sLine, jc_int nLineLength, jc_void* fp){return fgets(sLine, nLineLength, (FILE*)fp);}
static jc_int JC_CALL JcWriteFile(jc_void* fp, jc_char* pBuf, jc_int nBufLen){return fwrite((const void*)pBuf, nBufLen, 1, (FILE*)fp);}
static jc_bool JC_CALL JcIsSpace(jc_int c){return isspace(c)?1:0;}
static jc_bool JC_CALL JcIsAlpha(jc_int c){return isalpha(c)?1:0;}
static jc_bool JC_CALL JcIsAlnum(jc_int c){return isalnum(c)?1:0;}
static jc_bool JC_CALL JcIsDigit(jc_int c){return isdigit(c)?1:0;}
static jc_bool JC_CALL JcIsPunct(jc_int c){return ispunct(c)?1:0;}
static jc_bool JC_CALL JcIsPrint(jc_int c){return isprint(c)?1:0;}
static jc_void JC_CALL JcGetTime(jc_char* sTime)
{
	time_t t = time(NULL);
	jc_char * ct = ctime(&t);
	StringCopyN(sTime, ct+4, 7);
	StringCopyN(sTime+7, ct+20, 4);
	sTime[11] = 0;
}
static jc_void JC_CALL JcGetDate(jc_char* sTime)
{
	time_t t = time(NULL);
	jc_char * ct = ctime(&t);
	StringCopyN(sTime, ct+11, 8);
	sTime[8] = 0;
}
static jc_void JC_CALL JcPrintErrorV(jc_bool bWarning, const jc_char* sFormat, JcArgList oArgList)
{
	FILE* fp = bWarning?stdout:stderr;
	fprintf(fp, bWarning?"Warning: ":"Error: ");
	vfprintf(fp, sFormat, oArgList);
	fprintf(fp, "\n");
}
static jc_void JC_CALL JcFormatPrintV(jc_char* sBuf, const jc_char* sFormat, JcArgList oArgList){vsprintf(sBuf, sFormat, oArgList);}
static jc_void JC_CALL JcAbort(){abort();};
static jc_void* JC_CALL JcLoadLibrary(const jc_char* pLibName)
{
#ifdef WINDOWS
	return LoadLibrary(pLibName);
#else
	return dlopen(pLibName, RTLD_NOW|RTLD_GLOBAL);
#endif
}

static jc_void JC_CALL JcFreeLibrary(jc_void* pLibrary)
{
#ifdef WINDOWS
	FreeLibrary((HMODULE)pLibrary);
#else
	dlclose(pLibrary);
#endif
}

static jc_void* JC_CALL JcFindSymbol(jc_void* pLibrary, const jc_char* pSymbolName)
{
#ifdef WINDOWS
	return GetProcAddress((HMODULE)pLibrary, pSymbolName);
#else
	return dlsym(pLibrary, pSymbolName);
#endif	
}

#endif

static jc_void JC_CALL JcPrintError(jc_bool bWarning, const jc_char *sFormat, ...)
{
	JcArgList oArgList;
	JcArgStart(oArgList, sFormat);
	g_oInterface.PrintErrorV(bWarning, sFormat, oArgList);
	JcArgEnd(oArgList);
}
static jc_void JC_CALL JcFormatPrint(jc_char* sBuf, const jc_char* sFormat, ...)
{
	JcArgList oArgList;
	JcArgStart(oArgList, sFormat);
	g_oInterface.FormatPrintV(sBuf, sFormat, oArgList);
	JcArgEnd(oArgList);
}

CJcInterface g_oInterface = 
{
	//内存管理接口
	JcMalloc,
	JcFree,
	JcRealloc,
	JcCalloc,

	//转换函数
	JcCeil,
	JcStringToDouble,

	//文件操作接口
	JcOpenFile,
	JcCloseFile,
	JcGetLine,
	JcGetFileSize,
	JcGetCurrentPath,
	JcWriteFile,

	//错误函数
	JcPrintError,
	JcPrintErrorV,
	JcFormatPrint,
	JcFormatPrintV,
	JcAbort,

	//字符属性接口
	JcIsSpace,
	JcIsAlpha,
	JcIsAlnum,
	JcIsDigit,
	JcIsPunct,
	JcIsPrint,

	//时间函数
	JcGetTime,
	JcGetDate,

	//动态库函数
	JcLoadLibrary,
	JcFreeLibrary,
	JcFindSymbol
};

jc_int main(jc_int argc, jc_char* argv[])
{
	return JcCompile(argc, argv);
}

