
#include "jc_error.h"

void CompileError(CJcErrorSystem* pErrors, jc_bool bWarning, const jc_char *sFormat, ...)
{
	JcArgList oArgList;
	if(bWarning)
		++pErrors->nWarningCount;
	else
		++pErrors->nErrorCount;
	JcArgStart(oArgList, sFormat);
	g_oInterface.PrintErrorV(bWarning, sFormat, oArgList);
	JcArgEnd(oArgList);
}

void AllError(CJcErrorSystem* pErrors)
{
	if(pErrors->nWarningCount || pErrors->nErrorCount)
	{
		jc_bool bWarning = pErrors->nErrorCount?True:False;
		if(pErrors->nWarningCount && pErrors->nErrorCount)
			g_oInterface.PrintError(bWarning, "Total %u warning and %u error", pErrors->nWarningCount, pErrors->nErrorCount);
		else if(pErrors->nWarningCount)
			g_oInterface.PrintError(bWarning, "Total %u warning", pErrors->nWarningCount);
		else
			g_oInterface.PrintError(bWarning, "Total %u error", pErrors->nErrorCount);
	}
}
