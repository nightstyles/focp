
#ifndef _jc_goto_h_
#define _jc_goto_h_

#include "jc_codeseg.h"
#include "jc_string.h"
#include "jc_hash.h"

typedef struct CJcGotoFillError
{
	jc_int nLine;
	jc_int nCol;
	jc_char* sFileName;
	jc_char* sLabel;
}CJcGotoFillError;

typedef struct CJcGoto
{
	CJcString oLabelName;
	jc_uint nInsAddr;
	jc_int nLine;
	jc_int nCol;
	jc_char* sFileName;
}CJcGoto;

typedef struct CJcGotoTable
{
	struct CJcGotoTable* pPrev;
	CJcHashTable* pLabelTable;/*jc_char*, jc_uint*/
	jc_uint nCount;
	CJcGoto* pGotoTable;
}CJcGotoTable;

typedef struct CJcGotoStack
{
	CJcGotoTable * pGotoTable;
}CJcGotoStack;

jc_void InitializeGotoStack(CJcGotoStack* pStack);
jc_void ClearGotoStack(CJcGotoStack* pStack);

CJcGotoStack* CreateGotoStack();
jc_void DestroyGotoStack(CJcGotoStack * pStack);

jc_void NewGotoTable(CJcGotoStack * pStack);
jc_void PopGotoTable(CJcGotoStack * pStack);

jc_uint BackFillGotoStack(CJcGotoStack * pStack, CJcSegment* pCodeSegment, CJcGotoFillError *pError);

jc_uint NewGotoLabel(CJcGotoStack * pStack, jc_char* sLabelName, jc_uint nOffset);
jc_void CreateGoto(CJcGotoStack * pStack, jc_uint nGoInsAddr, jc_char* sFileName, jc_char* sLabelName, jc_int nLine, jc_int nCol);

#endif
