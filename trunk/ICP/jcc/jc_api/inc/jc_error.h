
#ifndef _jc_error_h_
#define _jc_error_h_

#include "jc_type.h"

typedef struct CJcErrorSystem
{
	jc_int nErrorCount;
	jc_int nWarningCount;
}CJcErrorSystem;

void CompileError(CJcErrorSystem* pErrors, jc_bool bWarning, const jc_char *sFormat, ...);
void AllError(CJcErrorSystem* pErrors);

#endif
