
#ifndef _jc_pp_h_
#define _jc_pp_h_

#include "jc_stab.h"
#include "jc_ftab.h"
#include "jc_vec.h"
#include "jc_error.h"

#define TopMax 256

typedef struct JcMacro
{
	jc_char *sMacroName;  /* token name */
	jc_char *sTokenSeq;   /* token replacement sequence */
	jc_int nParaNum;      /* number of parameters */
	jc_char *sProtect;    /* if one then no expansion before replacement for a given parameter*/
	jc_ulong nHashCode;	  /* `id' string hash code */
	jc_char *sFileName;   /* file name pointer */
}JcMacro;

typedef struct CJcCodeLine
{
	jc_char * sLine;
	jc_char * sFileName;
	jc_int nFileLineNo;
}CJcCodeLine;

typedef struct CJcPreProc
{
	CJcFileTable oFileTable;
	CJcCodeLine oCodeLine;
	jc_char* sLine;
	jc_char * sPreLineCur, *sPreLine; jc_uint nPreLineLen;
	jc_char * sProLineCur, *sProLine; jc_uint nProLineLen;
	jc_char* sFileName;
	jc_uint nFileLineNo;
	jc_int nIfLevel;
	jc_int nSkip;
	jc_int nCMode;
	JcMacro oDefMacro;
	CJcVector oMacros;/*<JcMacro*>*/
	jc_int nForbid[TopMax];
	CJcStringTable oStringTab;
	jc_uint nSZ;
	jc_char *pStr;
	jc_char *pS;
	CJcErrorSystem* pErrorSystem;
	jc_bool bSupportOutput;
}CJcPreProc;

jc_int InitializePreProcessor(CJcPreProc* pPreProc, CJcErrorSystem* pErrorSystem, jc_char* sFileName);
jc_void ClearPreProcessor(CJcPreProc* pPreProc);
jc_void InitializePreProcessorCmdLine(CJcPreProc* pPreProc, jc_int nArgc, jc_char* sArgv[]);
CJcCodeLine* GetProcLine(CJcPreProc* pPreProc);

#endif
