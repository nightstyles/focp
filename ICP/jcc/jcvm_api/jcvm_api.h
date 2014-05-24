
#ifndef _jcvm_api_h_
#define _jcvm_api_h_

#ifdef MSVC
	#pragma warning(disable:4786)
	#pragma warning(disable:4355)
#endif

#ifdef WINDOWS
#define JCVM_API __declspec(dllexport)
#define JCVM_CALL __stdcall
#else
#define JCVM_API
#define JCVM_CALL
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL ((void*)0)
#else
#define NULL (0)
#endif
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*
	JCVM_INT8 ~ JCVM_INT64 需要由外部定义成宏
*/
typedef signed JCVM_INT8 jc_char;
typedef unsigned JCVM_INT8 jc_uchar;
typedef signed JCVM_INT16 jc_short;
typedef unsigned JCVM_INT16 jc_ushort;
typedef signed JCVM_INT32 jc_int;
typedef unsigned JCVM_INT32 jc_uint;
typedef signed JCVM_INT64 jc_long;
typedef unsigned JCVM_INT64 jc_ulong;

typedef void jc_void;

enum
{
	JCVM_INVALID_ADDRESS=1,
	JCVM_LACK_MEMORY,
	JCVM_DIVDE_ZERO,
	JCVM_MEMORY_BEYOND,
	JCVM_POWER_OFF,
	JCVM_OTHER_EXCEPT,
};

enum//Module Management Error
{
	JCVM_MME_LACK_MEMORY=1,
	JCVM_MME_OPEN_FAILURE,
	JCVM_MME_READ_FAILURE,
	JCVM_MME_LINK_FAILURE,
	JCVM_MME_FIND_FAILURE,
	JCVM_MME_BAD_FILE,
};

typedef struct CJcVmSegment
{
	jc_char* IS, *DS, *CS, *SS;
}CJcVmSegment;

typedef struct CJcVmRegister
{
	jc_char* CP;
	jc_char* IS;
	jc_char* DS;
	jc_char* CS;
	jc_char* LS;
	jc_char* AS;
	jc_char* TS;
	jc_char* SS;
	jc_uint* PC;
	struct CJcVmRegister* FP;
}CJcVmRegister;

typedef jc_void (JCVM_CALL *FOnHost)(jc_uint nCpuIdx, jc_char* pArg);
typedef jc_void (JCVM_CALL *FSignal)(jc_uint nCpuIdx, jc_uint nReasonCode);
typedef jv_void (JCVM_CALL *FThreadProc)(jv_void* pContext);

typedef jc_void* (JCVM_CALL* FMalloc)(jc_uint nSize);
typedef jc_void (JCVM_CALL* FFree)(jc_void* pPtr);
typedef jc_void (JCVM_CALL* FSleep)(jc_uint nTimeOut);

typedef jc_void* (JCVM_CALL *FCreateEvent)();//init false and manual reset
typedef jc_void (JCVM_CALL* FDestroyEvent)(jc_void* pEvent);
typedef jc_void (JCVM_CALL* FSetEvent)(jc_void* pEvent);
typedef jc_void (JCVM_CALL* FResetEvent)(jc_void* pEvent);
typedef jc_void (JCVM_CALL* FWaitEvent)(jc_void* pEvent, jc_uint nTimeOut);

typedef jc_uint (JCVM_CALL *FCreateThread)(jv_void* pContext, FThreadProc ThreadProc);

typedef jc_int (JCVM_CALL *FGetMaxPath)();
typedef jc_int (JCVM_CALL *FFindFile)(jc_char* sFullPath, const jc_char* sFileName, jc_int bJcFile);
typedef jc_int (JCVM_CALL *FCompareFile)(const jc_char* sFileName1, const jc_char* sFileName2);
typedef jc_void* (JCVM_CALL *FOpenFile)(const jc_char* sFileName);//for read
typedef jc_void (JCVM_CALL *FCloseFile)(jc_void* pFile);
typedef jc_int (JCVM_CALL *FGetFileSize)(jc_void* pFile);
typedef jc_int (JCVM_CALL *FReadFile)(jc_void* pFile, jc_char* pBuf, jc_int nSize);

typedef jc_void* (JCVM_CALL *FLoadLibrary(const jc_char* pLibName);
typedef jc_void (JCVM_CALL *FFreeLibrary(jc_void* pLibrary);
typedef jc_void* (JCVM_CALL *FFindSymbol(jc_void* pLibrary, const jc_char* pSymbolName);

typedef struct CJcVmInterface
{
	FMalloc Malloc;
	FFree Free;
	FSleep Sleep;

	FCreateEvent CreateEvent;
	FDestroyEvent DestroyEvent;
	FSetEvent SetEvent;
	FResetEvent ResetEvent;
	FWaitEvent WaitEvent;

	FCreateThread CreateThread;

	FGetMaxPath GetMaxPath;
	FFindFile FindFile;
	FCompareFile CompareFile;
	FOpenFile OpenFile;
	FCloseFile CloseFile;
	FGetFileSize GetFileSize;
	FReadFile ReadFile;

	FLoadLibrary LoadLibrary;
	FFreeLibrary FreeLibrary;
	FFindSymbol FindSymbol;

	FSignal Signal;
}CJcVmInterface;

/////////////////////////////////////
//接口函数定义
/////////////////////////////////////
JCVM_API jc_uint JCVM_CALL JcVmStartup(CJcVmInterface* pInterface, jc_uint nCpuNum, jc_char* pBootFile);
JCVM_API jc_void JCVM_CALL JcVmCleanup();

/////////////////////////////////////
//宿主函数定义
/////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif
