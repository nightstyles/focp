
#ifndef _jc_keyword_h_
#define _jc_keyword_h_

#include "jc_type.h"

typedef struct CJcKeyword
{
    jc_char *id;
    jc_int  token;
}CJcKeyword;

jc_int IsKeyWord(jc_char* id, CJcKeyword* pKeywords, jc_int n);

#endif
