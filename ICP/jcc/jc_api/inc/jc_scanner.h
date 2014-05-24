
#if !defined(_jc_scanner_h_)
#define _jc_scanner_h_

#include "jc_api.h"
#include "jc_type.h"

typedef const jc_char* (JC_CALL* FGetLineFromSource)(void* pSource, jc_int * pLine, jc_char** sFileName);

#define JCC_CHAR_MAX 255
#define MAX_BUFFER_LENGTH (64*1024)
#define HEAP_BLOCK_SIZE (64*1024)

typedef struct CJcToken{
	int kind;			/* token kind */
	int pos;			/* token position in the source text (starting at 0) */
	int col;			/* token column (starting at 1) */
	int line;			/* token line (starting at 1) */
	int fcol, fline;	/* fact col & line */
	char* val;			/* token value */
	char* fname;		/* file name */
	struct CJcToken* next;
}CJcToken;

/* assumpsit: 
	return NULL or "", is file end;
	EOL is '\n';
*/
static const jc_int EoF = JCC_CHAR_MAX+1;

typedef struct CJcFileNameList{
	jc_char sFileName[256];
	struct CJcFileNameList* pNext;
}CJcFileNameList;

typedef struct CJcSourceLine{
	jc_int nPos;
	jc_int nLine;
	jc_int nSize;
	jc_char* sLine;
	jc_char* sFileName;
}CJcSourceLine;

typedef struct CJcSourceLineList{
	CJcSourceLine* pLine;
	struct CJcSourceLineList *pNext, *pPrev;
}CJcSourceLineList;

typedef struct CJcBuffer{
	void* pSource;
	FGetLineFromSource GetLine;
	CJcFileNameList* pFileNameList;
	CJcSourceLineList *pHead, *pTail;
	jc_int nPos;
	CJcSourceLineList* pCurLine;
	jc_int nOff;
}CJcBuffer;

/*********************************************************************************************
CJcStartStates  -- maps charactes to start states of tokens
**********************************************************************************************/
typedef struct CJcStateElem
{
	jc_int nKey, nVal;
	struct CJcStateElem *pNext;
}CJcStateElem;
typedef struct CJcStartStates
{
	CJcStateElem ** pStateTable;
}CJcStartStates;

/*********************************************************************************************
CJcKeywordMap  -- maps strings to integers (identifiers to keyword kinds)
**********************************************************************************************/
typedef struct CJcKeywordElem
{
	jc_char* sKey;
	jc_int nVal;
	struct CJcKeywordElem *pNext;
}CJcKeywordElem;
typedef struct CJcKeywordMap
{
	CJcKeywordElem ** pKeywordTable;
}CJcKeywordMap;

typedef struct CJcScanner{
	void *pFirstHeap;
	void *pHeap;
	void *pHeapTop;
	void **pHeapEnd;

	char EOL;
	int eofSym;
	int noSym;
	int maxT;
	int charSetSize;
	CJcStartStates start;
	CJcKeywordMap keywords;

	CJcToken *t;		/* current token */
	char *tval;			/* text of current token */
	int tvalLength;		/* length of text of current token */
	int tlen;			/* length of current token */

	CJcToken *tokens;	/* list of tokens already peeked (first token is a dummy) */
	CJcToken *pt;		/* current peek token */

	int ch;				/* current input character */

	int pos;			/* byte position of current character */
	int line;			/* line number of current character */
	int col;			/* column number of current character */
	int oldEols;		/* EOLs that appeared in a comment; */
	
	CJcBuffer *buffer;	/* scanner buffer */
}CJcScanner;

void InitializeScanner(CJcScanner* scanner, void* source, FGetLineFromSource getline);
void DestroyScanner(CJcScanner* scanner);
void BeginScan(CJcScanner* scanner);

void InitializeToken(CJcToken* pToken);
void DestroyToken(CJcToken* pToken);
CJcToken* ScanToken(CJcScanner* scanner);
CJcToken* PeekToken(CJcScanner* scanner);
void ResetPeek(CJcScanner* scanner);

#endif /* !defined(JCC_SCANNER_H__) */

