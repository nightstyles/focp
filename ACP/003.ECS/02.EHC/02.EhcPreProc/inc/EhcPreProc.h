
#include "EhcStab.h"
#include "EhcFtab.h"
#include "EhcVec.h"

#ifndef _EHC_PREPROC_H_
#define _EHC_PREPROC_H_

#ifdef EPP_EXPORTS
#define EPP_API EHC_EXPORT
#else
#define EPP_API EHC_IMPORT
#endif

#define TopMax 256

#ifdef __cplusplus
EHC_C_BEGIN();
#endif

typedef struct EhcMacro
{
	ehc_char *sMacroName;  /* token name */
	ehc_char *sTokenSeq;   /* token replacement sequence */
	ehc_int nParaNum;      /* number of parameters */
	ehc_char *sProtect;    /* if one then no expansion before replacement for a given parameter*/
	ehc_ulong nHashCode;	  /* `id' string hash code */
	ehc_char *sFileName;   /* file name pointer */
}EhcMacro;

typedef struct CEhcCodeLine
{
	ehc_char * sLine;
	ehc_char * sFileName;
	ehc_int nFileLineNo;
}CEhcCodeLine;

typedef struct CEhcPreProc
{
	CEhcFileTable oFileTable;
	CEhcCodeLine oCodeLine;
	ehc_char* sLine;
	ehc_char * sPreLineCur, *sPreLine; ehc_uint nPreLineLen;
	ehc_char * sProLineCur, *sProLine; ehc_uint nProLineLen;
	ehc_char* sFileName;
	ehc_uint nFileLineNo;
	ehc_int nIfLevel;
	ehc_int nSkip;
	ehc_int nCMode;
	EhcMacro oDefMacro;
	CEhcVector oMacros;/*<EhcMacro*>*/
	ehc_int nForbid[TopMax];
	CEhcStringTable oStringTab;
	ehc_uint nSZ;
	ehc_char *pStr;
	ehc_char *pS;
}CEhcPreProc;

EPP_API ehc_int InitializePreProcessor(CEhcPreProc* pPreProc, ehc_char* sFileName, ehc_int nArgc, ehc_char* sArgv[]);
EPP_API ehc_void ClearPreProcessor(CEhcPreProc* pPreProc);
EPP_API CEhcCodeLine* GetProcLine(CEhcPreProc* pPreProc);

#ifdef __cplusplus
EHC_C_END();
#endif

#endif
