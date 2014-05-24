
#include "jc_keyword.h"

jc_int IsKeyWord(jc_char*id, CJcKeyword* pKeywords, jc_int n)
{
	jc_int i;
    for(i=0; i<n; i++)
	{
		if(!StringCompare(pKeywords[i].id, id))
		   return pKeywords[i].token;
	}
    return 0;
}
