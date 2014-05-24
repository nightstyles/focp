
#include "jc_scanner.h"

static void InitializeBuffer(CJcBuffer* pBuffer, void* pSource, FGetLineFromSource GetLine);
static void DestroyBuffer(CJcBuffer* pBuffer);
static jc_int Read(CJcBuffer* pBuffer);
static jc_int Peek(CJcBuffer* pBuffer);
static jc_char* GetString(CJcBuffer* pBuffer, jc_int beg, jc_int end);
static jc_int GetPos(CJcBuffer* pBuffer);
static void SetPos(CJcBuffer* pBuffer, jc_int nPos);
static jc_int GetLine(CJcBuffer* pBuffer);
static jc_int GetCol(CJcBuffer* pBuffer);
static jc_char* GetFileName(CJcBuffer* pBuffer);

static void InitializeStateElem(CJcStateElem* elem, jc_int nKey, jc_int nVal);
static void InitializeStartStates(CJcStartStates* states);
static void DestroyStartStates(CJcStartStates* states);
static void SetStartState(CJcStartStates* states, jc_int nKey, jc_int nVal);
static jc_int GetStartState(CJcStartStates* states, jc_int nKey);

static void InitializeKeywordElem(CJcKeywordElem* elem, jc_char* sKey, jc_int nVal);
static void DestroyKeywordElem(CJcKeywordElem* elem);
static void InitializeKeywordMap(CJcKeywordMap* map);
static void DestroyKeywordMap(CJcKeywordMap* map);
static void SetKeyword(CJcKeywordMap* map, const jc_char* sKey, jc_int nVal);
static jc_int GetKeyword(CJcKeywordMap* map, const jc_char* sKey, jc_int defaultVal);

static void CreateHeapBlock(CJcScanner* pScanner);
static CJcToken* CreateToken(CJcScanner* pScanner);
static void AppendVal(CJcScanner* pScanner, CJcToken *pToken);
static void Init(CJcScanner* pScanner);
static void NextCh(CJcScanner* pScanner);
static void AddCh(CJcScanner* pScanner);

static CJcToken* NextToken(CJcScanner* pScanner);

static jc_int jcc_string_hash(const jc_char *data)
{
	jc_int h = 0;
	if (!data)
		return 0;
	while(*data != 0)
	{
		h = (h * 7) ^ *data;
		++data;
	}
	if (h < 0)
		h = -h;
	return h;
}

static jc_char* jcc_string_create_lower_ax(const jc_char* data, jc_int startIndex, jc_int dataLen)
{
	jc_int i;
	jc_char* newData;
	if (!data)
		return NULL;
	newData = (jc_char*)g_oInterface.Malloc(dataLen + 1);
	for(i = 0; i <= dataLen; i++)
	{
		jc_char ch = data[startIndex + i];
		if (('A' <= ch) && (ch <= 'Z'))
			newData[i] = ch - ('A' - 'a');
		else
			newData[i] = ch;
	}
	newData[dataLen] = '\0';
	return newData;
}

static jc_char* jcc_string_create_lower(const jc_char* data)
{
	if (!data)
		return NULL;
	return jcc_string_create_lower_ax(data, 0, StringLength(data));
}

static jc_char* jcc_string_create(const jc_char *value , jc_int startIndex, jc_int length)
{
	jc_int len = 0;
	jc_char* data;

	if(value)
		len = length;
	data = (jc_char*)g_oInterface.Malloc(len + 1);
	StringCopyN(data, &(value[startIndex]), len);
	data[len] = 0;

	return data;
}

void InitializeToken(CJcToken* pToken){
	pToken->kind = 0;
	pToken->pos  = 0;
	pToken->col  = 0;
	pToken->line = 0;
	pToken->fcol  = 0;
	pToken->fline = 0;
	pToken->val  = NULL;
	pToken->fname = NULL;
	pToken->next = NULL;
}

void DestroyToken(CJcToken* pToken){
	if(pToken->val){
		g_oInterface.Free(pToken->val);
		pToken->val = NULL;
	}
}

static void InitializeBuffer(CJcBuffer* pBuffer, void* pSource, FGetLineFromSource GetLine)
{
	pBuffer->pSource = pSource;
	pBuffer->GetLine = GetLine;
	pBuffer->pFileNameList = NULL;
	pBuffer->pHead = pBuffer->pTail = NULL;
	pBuffer->nPos = 0;
	pBuffer->pCurLine = NULL;
	pBuffer->nOff = 0;
}

static void DestroyBuffer(CJcBuffer* pBuffer)
{
	while(pBuffer->pFileNameList)
	{
		CJcFileNameList* pNext = pBuffer->pFileNameList->pNext;
		g_oInterface.Free(pBuffer->pFileNameList);
		pBuffer->pFileNameList = pNext;
	}
	while(pBuffer->pHead)
	{
		pBuffer->pTail = pBuffer->pHead->pNext;
		if(pBuffer->pHead->pLine)
		{
			if(pBuffer->pHead->pLine->sLine)
				g_oInterface.Free(pBuffer->pHead->pLine->sLine);
			g_oInterface.Free(pBuffer->pHead->pLine);
		}
		g_oInterface.Free(pBuffer->pHead);
		pBuffer->pHead = pBuffer->pTail;
	}
	pBuffer->nPos = 0;
	pBuffer->pCurLine = NULL;
	pBuffer->nOff = 0;
}

static jc_int Read(CJcBuffer* pBuffer)
{
	jc_char c;
	if(!pBuffer->pCurLine)
	{
		const jc_char* sLine;
		pBuffer->pCurLine = (CJcSourceLineList*)g_oInterface.Malloc(sizeof(CJcSourceLineList));
		pBuffer->pCurLine->pLine = (CJcSourceLine*)g_oInterface.Malloc(sizeof(CJcSourceLine));
		pBuffer->pCurLine->pNext = NULL;
		sLine = pBuffer->GetLine(pBuffer->pSource, &pBuffer->pCurLine->pLine->nLine, &pBuffer->pCurLine->pLine->sFileName);
		if(!sLine)
			sLine = "";
		pBuffer->pCurLine->pLine->nPos = pBuffer->nPos;
		pBuffer->pCurLine->pLine->nSize = StringLength(sLine);
		pBuffer->pCurLine->pLine->sLine = StringDuplicate(sLine);
		pBuffer->pCurLine->pPrev = pBuffer->pTail;
		if(pBuffer->pTail)
			pBuffer->pTail->pNext = pBuffer->pCurLine;
		else
			pBuffer->pHead = pBuffer->pCurLine;
		pBuffer->pTail = pBuffer->pCurLine;
		pBuffer->nOff = 0;
	}
	c = pBuffer->pCurLine->pLine->sLine[pBuffer->nOff];
	++pBuffer->nOff;
	if(pBuffer->nOff >= pBuffer->pCurLine->pLine->nSize)
	{
		if(!c || c == -1)
		{
			--pBuffer->nOff;
			return EoF;
		}
		pBuffer->pCurLine = pBuffer->pCurLine->pNext;
		pBuffer->nOff = 0;
	}
	++pBuffer->nPos;
	return (jc_int)(jc_uchar)c;
}

static jc_int Peek(CJcBuffer* pBuffer)
{
	jc_int curPos = GetPos(pBuffer);
	jc_int ch = Read(pBuffer);
	SetPos(pBuffer, curPos);
	return ch;
}

static jc_char* GetString(CJcBuffer* pBuffer, jc_int beg, jc_int end)
{
	jc_int i, len = end - beg;
	jc_char *buf = (jc_char*)g_oInterface.Malloc(len);
	jc_int oldPos = GetPos(pBuffer);
	SetPos(pBuffer, beg);
	for (i = 0; i < len; ++i)
		buf[i] = (jc_char)Read(pBuffer);
	SetPos(pBuffer, oldPos);
	return buf;
}

static jc_int GetPos(CJcBuffer* pBuffer)
{
	return pBuffer->nPos;
}

static void SetPos(CJcBuffer* pBuffer, jc_int nPos)
{
	if (nPos < 0)
		nPos = 0;
	if(pBuffer->nPos == nPos)
		return;
	if(!pBuffer->pCurLine)
		pBuffer->pCurLine = pBuffer->pTail;
	while(1)
	{
		if(!pBuffer->pCurLine)
			Peek(pBuffer);
		if(!pBuffer->pCurLine->pLine->sLine[0])
			break;
		if(pBuffer->pCurLine->pLine->nPos > nPos)
			pBuffer->pCurLine = pBuffer->pCurLine->pPrev;
		else if(pBuffer->pCurLine->pLine->nPos + pBuffer->pCurLine->pLine->nSize > nPos)
		{
			pBuffer->nOff = nPos - pBuffer->pCurLine->pLine->nPos;
			pBuffer->nPos = nPos;
		}
		else
		{
			pBuffer->nPos = pBuffer->pCurLine->pLine->nPos + pBuffer->pCurLine->pLine->nSize;
			pBuffer->pCurLine = pBuffer->pCurLine->pNext;
			pBuffer->nOff = 0;
		}
	}
}

static jc_int GetLine(CJcBuffer* pBuffer)
{
	if(!pBuffer->pCurLine)
		Peek(pBuffer);
	return pBuffer->pCurLine->pLine->nLine;
}

static jc_int GetCol(CJcBuffer* pBuffer)
{
	return pBuffer->nOff;
}

static jc_char* GetFileName(CJcBuffer* pBuffer)
{
	if(!pBuffer->pCurLine)
		Peek(pBuffer);
	return pBuffer->pCurLine->pLine->sFileName;
}

static void InitializeStateElem(CJcStateElem* elem, jc_int nKey, jc_int nVal)
{
	elem->nKey = nKey;
	elem->nVal = nVal;
	elem->pNext = NULL;
}

static void InitializeStartStates(CJcStartStates* states)
{
	states->pStateTable = (CJcStateElem**)g_oInterface.Malloc(128*sizeof(void*));
	MemorySet(states->pStateTable, 0, 128 * sizeof(void*));
}

static void DestroyStartStates(CJcStartStates* states)
{
	jc_int i;
	for (i = 0; i < 128; ++i)
	{
		CJcStateElem *e = states->pStateTable[i];
		while (e != NULL)
		{
			CJcStateElem *pNext = e->pNext;
			g_oInterface.Free(e);
			e = pNext;
		}
	}
	g_oInterface.Free(states->pStateTable);
}

static void SetStartState(CJcStartStates* states, jc_int nKey, jc_int nVal)
{
	jc_int k = nKey % 128;
	CJcStateElem *e = (CJcStateElem*)g_oInterface.Malloc(sizeof(CJcStateElem));
	InitializeStateElem(e, nKey, nVal);
	e->pNext = states->pStateTable[k];
	states->pStateTable[k] = e;
}

static jc_int GetStartState(CJcStartStates* states, jc_int nKey)
{
	CJcStateElem *e = states->pStateTable[nKey % 128];
	while (e != NULL && e->nKey != nKey)
		e = e->pNext;
	return e == NULL ? 0 : e->nVal;
}

static void InitializeKeywordElem(CJcKeywordElem* elem, jc_char* sKey, jc_int nVal)
{
	elem->sKey = StringDuplicate(sKey);
	elem->nVal = nVal;
	elem->pNext = NULL;
}

static void DestroyKeywordElem(CJcKeywordElem* elem)
{
	g_oInterface.Free(elem->sKey);
}

static void InitializeKeywordMap(CJcKeywordMap* map)
{
	map->pKeywordTable = (CJcKeywordElem**)g_oInterface.Malloc(128*sizeof(void*));
	MemorySet(map->pKeywordTable, 0, 128 * sizeof(void*));
}

static void DestroyKeywordMap(CJcKeywordMap* map)
{
	jc_int i;
	for (i = 0; i < 128; ++i)
	{
		CJcKeywordElem *e = map->pKeywordTable[i];
		while (e != NULL)
		{
			CJcKeywordElem *pNext = e->pNext;
			DestroyKeywordElem(e);
			g_oInterface.Free(e);
			e = pNext;
		}
	}
	g_oInterface.Free(map->pKeywordTable);
}

static void SetKeyword(CJcKeywordMap* map, const jc_char* sKey, jc_int nVal)
{
	jc_int k = jcc_string_hash(sKey) % 128;
	CJcKeywordElem *e = (CJcKeywordElem*)g_oInterface.Malloc(sizeof(CJcStateElem));
	InitializeKeywordElem(e, (jc_char*)sKey, nVal);
	e->pNext = map->pKeywordTable[k];
	map->pKeywordTable[k] = e;
}

static jc_int GetKeyword(CJcKeywordMap* map, const jc_char* sKey, jc_int defaultVal)
{
	CJcKeywordElem *e = map->pKeywordTable[jcc_string_hash(sKey) % 128];
	while (e != NULL && StringCompare(e->sKey, sKey))
		e = e->pNext;
	return e == NULL ? defaultVal : e->nVal;
}

static void CreateHeapBlock(CJcScanner* pScanner)
{
	void* newHeap;
	jc_char* pHeap = (jc_char*) pScanner->pFirstHeap;

	while(((jc_char*) pScanner->tokens < pHeap) || ((jc_char*) pScanner->tokens > (pHeap + HEAP_BLOCK_SIZE)))
	{
		pHeap = *((jc_char**) (pHeap + HEAP_BLOCK_SIZE));
		g_oInterface.Free(pScanner->pFirstHeap);
		pScanner->pFirstHeap = pHeap;
	}

	/* HEAP_BLOCK_SIZE byte heap + pointer to next heap block */
	newHeap = g_oInterface.Malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	*pScanner->pHeapEnd = newHeap;
	pScanner->pHeapEnd = (void**) (((jc_char*) newHeap) + HEAP_BLOCK_SIZE);
	*pScanner->pHeapEnd = 0;
	pScanner->pHeap = newHeap;
	pScanner->pHeapTop = pScanner->pHeap;
}

static CJcToken* CreateToken(CJcScanner* pScanner)
{
	CJcToken *pToken;
	if (((jc_char*) pScanner->pHeapTop + (jc_int) sizeof(CJcToken)) >= (jc_char*) pScanner->pHeapEnd)
		CreateHeapBlock(pScanner);
	pToken = (CJcToken*) pScanner->pHeapTop;
	pScanner->pHeapTop = (void*) ((jc_char*) pScanner->pHeapTop + sizeof(CJcToken));
	pToken->val = NULL;
	pToken->next = NULL;
	return pToken;
}

static void AppendVal(CJcScanner* pScanner, CJcToken *pToken)
{
	jc_int reqMem = (pScanner->tlen + 1) * sizeof(jc_char);
	if (((jc_char*) pScanner->pHeapTop + reqMem) >= (jc_char*) pScanner->pHeapEnd)
	{
		if(reqMem > HEAP_BLOCK_SIZE)
		{
			g_oInterface.PrintError(0, "%s\n", "--- Too long token value");
			g_oInterface.Abort();
		}
		CreateHeapBlock(pScanner);
	}
	pToken->val = (jc_char*) pScanner->pHeapTop;
	pScanner->pHeapTop = (void*) ((jc_char*) pScanner->pHeapTop + reqMem);
	StringCopyN(pToken->val, pScanner->tval, pScanner->tlen);
	pToken->val[pScanner->tlen] = '\0';
}

static void Init(CJcScanner* scanner){
	jc_int i;
	scanner->EOL = '\n';
	scanner->eofSym = 0;
	scanner->maxT = 87;
	scanner->noSym = 87;
	for (i = 65; i <= 90; ++i) SetStartState(&scanner->start, i, 1);
	for (i = 95; i <= 95; ++i) SetStartState(&scanner->start, i, 1);
	for (i = 97; i <= 122; ++i) SetStartState(&scanner->start, i, 1);
	for (i = 49; i <= 57; ++i) SetStartState(&scanner->start, i, 71);
	SetStartState(&scanner->start, 48, 72);
	SetStartState(&scanner->start, 39, 2);
	SetStartState(&scanner->start, 34, 9);
	SetStartState(&scanner->start, 46, 73);
	SetStartState(&scanner->start, 62, 74);
	SetStartState(&scanner->start, 60, 75);
	SetStartState(&scanner->start, 33, 76);
	SetStartState(&scanner->start, 37, 77);
	SetStartState(&scanner->start, 94, 78);
	SetStartState(&scanner->start, 38, 79);
	SetStartState(&scanner->start, 42, 80);
	SetStartState(&scanner->start, 45, 81);
	SetStartState(&scanner->start, 43, 82);
	SetStartState(&scanner->start, 124, 83);
	SetStartState(&scanner->start, 47, 84);
	SetStartState(&scanner->start, 61, 85);
	SetStartState(&scanner->start, 126, 61);
	SetStartState(&scanner->start, 40, 62);
	SetStartState(&scanner->start, 41, 63);
	SetStartState(&scanner->start, 123, 64);
	SetStartState(&scanner->start, 125, 65);
	SetStartState(&scanner->start, 91, 66);
	SetStartState(&scanner->start, 93, 67);
	SetStartState(&scanner->start, 58, 86);
	SetStartState(&scanner->start, 59, 68);
	SetStartState(&scanner->start, 44, 69);
	SetStartState(&scanner->start, 63, 70);
	SetStartState(&scanner->start, EoF, -1);
	SetKeyword(&scanner->keywords, "auto", 6);
	SetKeyword(&scanner->keywords, "break", 7);
	SetKeyword(&scanner->keywords, "case", 8);
	SetKeyword(&scanner->keywords, "char", 9);
	SetKeyword(&scanner->keywords, "const", 10);
	SetKeyword(&scanner->keywords, "continue", 11);
	SetKeyword(&scanner->keywords, "default", 12);
	SetKeyword(&scanner->keywords, "do", 13);
	SetKeyword(&scanner->keywords, "double", 14);
	SetKeyword(&scanner->keywords, "else", 15);
	SetKeyword(&scanner->keywords, "enum", 16);
	SetKeyword(&scanner->keywords, "extern", 17);
	SetKeyword(&scanner->keywords, "float", 18);
	SetKeyword(&scanner->keywords, "for", 19);
	SetKeyword(&scanner->keywords, "goto", 20);
	SetKeyword(&scanner->keywords, "host", 21);
	SetKeyword(&scanner->keywords, "if", 22);
	SetKeyword(&scanner->keywords, "int", 23);
	SetKeyword(&scanner->keywords, "long", 24);
	SetKeyword(&scanner->keywords, "register", 25);
	SetKeyword(&scanner->keywords, "return", 26);
	SetKeyword(&scanner->keywords, "share", 27);
	SetKeyword(&scanner->keywords, "short", 28);
	SetKeyword(&scanner->keywords, "signed", 29);
	SetKeyword(&scanner->keywords, "sizeof", 30);
	SetKeyword(&scanner->keywords, "static", 31);
	SetKeyword(&scanner->keywords, "struct", 32);
	SetKeyword(&scanner->keywords, "switch", 33);
	SetKeyword(&scanner->keywords, "typedef", 34);
	SetKeyword(&scanner->keywords, "union", 35);
	SetKeyword(&scanner->keywords, "unsigned", 36);
	SetKeyword(&scanner->keywords, "void", 37);
	SetKeyword(&scanner->keywords, "volatile", 38);
	SetKeyword(&scanner->keywords, "while", 39);
	SetKeyword(&scanner->keywords, "alignof", 86);

	scanner->tvalLength = 128;
	scanner->tval = (char*)g_oInterface.Malloc(scanner->tvalLength); /* text of current token */

	/* HEAP_BLOCK_SIZE byte heap + pointer to next heap block */
	scanner->pHeap = g_oInterface.Malloc(HEAP_BLOCK_SIZE + sizeof(void*));
	scanner->pFirstHeap = scanner->pHeap;
	scanner->pHeapEnd = (void**) (((char*) scanner->pHeap) + HEAP_BLOCK_SIZE);
	*scanner->pHeapEnd = 0;
	scanner->pHeapTop = scanner->pHeap;
	if(sizeof(CJcToken) > HEAP_BLOCK_SIZE){
		g_oInterface.PrintError(0, "--- Too small HEAP_BLOCK_SIZE\n");
		g_oInterface.Abort();
	}

	scanner->pos = -1;
	scanner->line = 1;
	scanner->col = 0;
	scanner->oldEols = 0;
}

void BeginScan(CJcScanner* scanner){
	NextCh(scanner);

	scanner->pt = scanner->tokens = CreateToken(scanner); /* first token is a dummy */
}

static void NextCh(CJcScanner* scanner){
	if (scanner->oldEols > 0){
		scanner->ch = scanner->EOL;
		scanner->oldEols--;
	}
	else{
		scanner->pos = GetPos(scanner->buffer);
		scanner->ch = Read(scanner->buffer);
		scanner->col++;
		/* replace isolated '\r' by '\n' in order to make
		   eol handling uniform across Windows, Unix and Mac */
		if (scanner->ch == '\r' && Peek(scanner->buffer) != '\n')
			scanner->ch = scanner->EOL;
		if (scanner->ch == scanner->EOL){
			scanner->line++;
			scanner->col = 0;
		}
	}

}

static void AddCh(CJcScanner* scanner){
	char *newBuf;
	if (scanner->tlen >= scanner->tvalLength){
		scanner->tvalLength *= 2;
		newBuf = (char*)g_oInterface.Malloc(scanner->tvalLength);
		MemoryCopy(newBuf, scanner->tval, scanner->tlen*sizeof(char));
		g_oInterface.Free(scanner->tval);
		scanner->tval = newBuf;
	}
	scanner->tval[scanner->tlen++] = scanner->ch;
	NextCh(scanner);
}



#define free g_oInterface.Free
static CJcToken* NextToken(CJcScanner* scanner){
	jc_int state;
	while (scanner->ch == ' ' ||
			scanner->ch >= 9 && scanner->ch <= 10 || scanner->ch == 13
	) NextCh(scanner);

	scanner->t = CreateToken(scanner);
	scanner->t->fcol = GetCol(scanner->buffer);
	scanner->t->fline = GetLine(scanner->buffer);
	scanner->t->fname = GetFileName(scanner->buffer);
	scanner->t->pos = scanner->pos;
	scanner->t->col = scanner->col;
	scanner->t->line = scanner->line;
	state = GetStartState(&scanner->start, scanner->ch);
	scanner->tlen = 0;
	AddCh(scanner);

	switch (state){
	case -1:
		scanner->t->kind = scanner->eofSym;
		break;
	case 0:
		scanner->t->kind = scanner->noSym;
		break;
		case 1:
			case_1:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'Z' || scanner->ch == '_' || scanner->ch >= 'a' && scanner->ch <= 'z') {AddCh(scanner); goto case_1;}
			else {scanner->t->kind = 1; {char *literal = jcc_string_create(scanner->tval, 0, scanner->tlen); scanner->t->kind = GetKeyword(&scanner->keywords, literal, scanner->t->kind); free(literal); break;}}
		case 2:
			if (scanner->ch <= 9 || scanner->ch >= 11 && scanner->ch <= 12 || scanner->ch >= 14 && scanner->ch <= '&' || scanner->ch >= '(' && scanner->ch <= '[' || scanner->ch >= ']' && scanner->ch <= 65535) {AddCh(scanner); goto case_4;}
			else if (scanner->ch == 92) {AddCh(scanner); goto case_3;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 3:
			case_3:
			if (scanner->ch == '"' || scanner->ch == 39 || scanner->ch == '?' || scanner->ch == 92 || scanner->ch >= 'a' && scanner->ch <= 'b' || scanner->ch == 'f' || scanner->ch == 'n' || scanner->ch == 'r' || scanner->ch == 't' || scanner->ch == 'v') {AddCh(scanner); goto case_4;}
			else if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_5;}
			else if (scanner->ch == 'X' || scanner->ch == 'x') {AddCh(scanner); goto case_6;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 4:
			case_4:
			if (scanner->ch == 39) {AddCh(scanner); goto case_8;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 5:
			case_5:
			if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_87;}
			else if (scanner->ch == 39) {AddCh(scanner); goto case_8;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 6:
			case_6:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'F' || scanner->ch >= 'a' && scanner->ch <= 'f') {AddCh(scanner); goto case_7;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 7:
			case_7:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'F' || scanner->ch >= 'a' && scanner->ch <= 'f') {AddCh(scanner); goto case_4;}
			else if (scanner->ch == 39) {AddCh(scanner); goto case_8;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 8:
			case_8:
			{scanner->t->kind = 2; break;}
		case 9:
			case_9:
			if (scanner->ch <= 9 || scanner->ch >= 11 && scanner->ch <= 12 || scanner->ch >= 14 && scanner->ch <= '!' || scanner->ch >= '#' && scanner->ch <= '[' || scanner->ch >= ']' && scanner->ch <= 65535) {AddCh(scanner); goto case_9;}
			else if (scanner->ch == '"') {AddCh(scanner); goto case_14;}
			else if (scanner->ch == 92) {AddCh(scanner); goto case_10;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 10:
			case_10:
			if (scanner->ch == '"' || scanner->ch == 39 || scanner->ch == '?' || scanner->ch == 92 || scanner->ch >= 'a' && scanner->ch <= 'b' || scanner->ch == 'f' || scanner->ch == 'n' || scanner->ch == 'r' || scanner->ch == 't' || scanner->ch == 'v') {AddCh(scanner); goto case_9;}
			else if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_11;}
			else if (scanner->ch == 'X' || scanner->ch == 'x') {AddCh(scanner); goto case_12;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 11:
			case_11:
			if (scanner->ch <= 9 || scanner->ch >= 11 && scanner->ch <= 12 || scanner->ch >= 14 && scanner->ch <= '!' || scanner->ch >= '#' && scanner->ch <= '/' || scanner->ch >= '8' && scanner->ch <= '[' || scanner->ch >= ']' && scanner->ch <= 65535) {AddCh(scanner); goto case_9;}
			else if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_88;}
			else if (scanner->ch == '"') {AddCh(scanner); goto case_14;}
			else if (scanner->ch == 92) {AddCh(scanner); goto case_10;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 12:
			case_12:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'F' || scanner->ch >= 'a' && scanner->ch <= 'f') {AddCh(scanner); goto case_13;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 13:
			case_13:
			if (scanner->ch <= 9 || scanner->ch >= 11 && scanner->ch <= 12 || scanner->ch >= 14 && scanner->ch <= '!' || scanner->ch >= '#' && scanner->ch <= '[' || scanner->ch >= ']' && scanner->ch <= 65535) {AddCh(scanner); goto case_9;}
			else if (scanner->ch == '"') {AddCh(scanner); goto case_14;}
			else if (scanner->ch == 92) {AddCh(scanner); goto case_10;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 14:
			case_14:
			{scanner->t->kind = 3; break;}
		case 15:
			case_15:
			if (scanner->ch == '8' || scanner->ch == 'L' || scanner->ch == 'l') {AddCh(scanner); goto case_25;}
			else if (scanner->ch == '1') {AddCh(scanner); goto case_16;}
			else if (scanner->ch == '3') {AddCh(scanner); goto case_17;}
			else if (scanner->ch == '6') {AddCh(scanner); goto case_18;}
			else {scanner->t->kind = 4; break;}
		case 16:
			case_16:
			if (scanner->ch == '6') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 17:
			case_17:
			if (scanner->ch == '2') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 18:
			case_18:
			if (scanner->ch == '4') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 19:
			case_19:
			if (scanner->ch == '8') {AddCh(scanner); goto case_25;}
			else if (scanner->ch == '1') {AddCh(scanner); goto case_20;}
			else if (scanner->ch == '3') {AddCh(scanner); goto case_21;}
			else if (scanner->ch == '6') {AddCh(scanner); goto case_22;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 20:
			case_20:
			if (scanner->ch == '6') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 21:
			case_21:
			if (scanner->ch == '2') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 22:
			case_22:
			if (scanner->ch == '4') {AddCh(scanner); goto case_25;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 23:
			case_23:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'F' || scanner->ch >= 'a' && scanner->ch <= 'f') {AddCh(scanner); goto case_24;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 24:
			case_24:
			if (scanner->ch >= '0' && scanner->ch <= '9' || scanner->ch >= 'A' && scanner->ch <= 'F' || scanner->ch >= 'a' && scanner->ch <= 'f') {AddCh(scanner); goto case_24;}
			else if (scanner->ch == 'U' || scanner->ch == 'u') {AddCh(scanner); goto case_15;}
			else if (scanner->ch == 'I' || scanner->ch == 'i') {AddCh(scanner); goto case_19;}
			else {scanner->t->kind = 4; break;}
		case 25:
			case_25:
			{scanner->t->kind = 4; break;}
		case 26:
			case_26:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_26;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_27;}
			else {scanner->t->kind = 5; break;}
		case 27:
			case_27:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_29;}
			else if (scanner->ch == '+' || scanner->ch == '-') {AddCh(scanner); goto case_28;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 28:
			case_28:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_29;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 29:
			case_29:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_29;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else {scanner->t->kind = 5; break;}
		case 30:
			case_30:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_30;}
			else if (scanner->ch == '.') {AddCh(scanner); goto case_31;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_36;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 31:
			case_31:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_35;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_32;}
			else {scanner->t->kind = 5; break;}
		case 32:
			case_32:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_34;}
			else if (scanner->ch == '+' || scanner->ch == '-') {AddCh(scanner); goto case_33;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 33:
			case_33:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_34;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 34:
			case_34:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_34;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else {scanner->t->kind = 5; break;}
		case 35:
			case_35:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_35;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_32;}
			else {scanner->t->kind = 5; break;}
		case 36:
			case_36:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_38;}
			else if (scanner->ch == '+' || scanner->ch == '-') {AddCh(scanner); goto case_37;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 37:
			case_37:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_38;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 38:
			case_38:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_38;}
			else if (scanner->ch == 'F' || scanner->ch == 'f') {AddCh(scanner); goto case_39;}
			else {scanner->t->kind = 5; break;}
		case 39:
			case_39:
			{scanner->t->kind = 5; break;}
		case 40:
			case_40:
			if (scanner->ch == '.') {AddCh(scanner); goto case_41;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 41:
			case_41:
			{scanner->t->kind = 40; break;}
		case 42:
			case_42:
			{scanner->t->kind = 41; break;}
		case 43:
			case_43:
			{scanner->t->kind = 42; break;}
		case 44:
			case_44:
			{scanner->t->kind = 43; break;}
		case 45:
			case_45:
			{scanner->t->kind = 44; break;}
		case 46:
			case_46:
			{scanner->t->kind = 45; break;}
		case 47:
			case_47:
			{scanner->t->kind = 46; break;}
		case 48:
			case_48:
			{scanner->t->kind = 47; break;}
		case 49:
			case_49:
			{scanner->t->kind = 48; break;}
		case 50:
			case_50:
			{scanner->t->kind = 49; break;}
		case 51:
			case_51:
			{scanner->t->kind = 50; break;}
		case 52:
			case_52:
			{scanner->t->kind = 51; break;}
		case 53:
			case_53:
			{scanner->t->kind = 52; break;}
		case 54:
			case_54:
			{scanner->t->kind = 53; break;}
		case 55:
			case_55:
			{scanner->t->kind = 54; break;}
		case 56:
			case_56:
			{scanner->t->kind = 57; break;}
		case 57:
			case_57:
			{scanner->t->kind = 58; break;}
		case 58:
			case_58:
			{scanner->t->kind = 59; break;}
		case 59:
			case_59:
			{scanner->t->kind = 60; break;}
		case 60:
			case_60:
			{scanner->t->kind = 61; break;}
		case 61:
			{scanner->t->kind = 62; break;}
		case 62:
			{scanner->t->kind = 68; break;}
		case 63:
			{scanner->t->kind = 69; break;}
		case 64:
			case_64:
			{scanner->t->kind = 73; break;}
		case 65:
			case_65:
			{scanner->t->kind = 74; break;}
		case 66:
			case_66:
			{scanner->t->kind = 75; break;}
		case 67:
			case_67:
			{scanner->t->kind = 76; break;}
		case 68:
			{scanner->t->kind = 79; break;}
		case 69:
			{scanner->t->kind = 82; break;}
		case 70:
			{scanner->t->kind = 84; break;}
		case 71:
			case_71:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_71;}
			else if (scanner->ch == 'U' || scanner->ch == 'u') {AddCh(scanner); goto case_15;}
			else if (scanner->ch == 'I' || scanner->ch == 'i') {AddCh(scanner); goto case_19;}
			else if (scanner->ch == '.') {AddCh(scanner); goto case_31;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_36;}
			else {scanner->t->kind = 4; break;}
		case 72:
			case_72:
			if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_72;}
			else if (scanner->ch >= '8' && scanner->ch <= '9') {AddCh(scanner); goto case_30;}
			else if (scanner->ch == 'X' || scanner->ch == 'x') {AddCh(scanner); goto case_23;}
			else if (scanner->ch == 'U' || scanner->ch == 'u') {AddCh(scanner); goto case_15;}
			else if (scanner->ch == 'I' || scanner->ch == 'i') {AddCh(scanner); goto case_19;}
			else if (scanner->ch == '.') {AddCh(scanner); goto case_31;}
			else if (scanner->ch == 'E' || scanner->ch == 'e') {AddCh(scanner); goto case_36;}
			else {scanner->t->kind = 4; break;}
		case 73:
			if (scanner->ch >= '0' && scanner->ch <= '9') {AddCh(scanner); goto case_26;}
			else if (scanner->ch == '.') {AddCh(scanner); goto case_40;}
			else {scanner->t->kind = 83; break;}
		case 74:
			if (scanner->ch == '>') {AddCh(scanner); goto case_89;}
			else if (scanner->ch == '=') {AddCh(scanner); goto case_53;}
			else {scanner->t->kind = 81; break;}
		case 75:
			if (scanner->ch == '<') {AddCh(scanner); goto case_90;}
			else if (scanner->ch == '=') {AddCh(scanner); goto case_54;}
			else if (scanner->ch == '%') {AddCh(scanner); goto case_64;}
			else if (scanner->ch == ':') {AddCh(scanner); goto case_66;}
			else {scanner->t->kind = 80; break;}
		case 76:
			if (scanner->ch == '=') {AddCh(scanner); goto case_44;}
			else {scanner->t->kind = 63; break;}
		case 77:
			if (scanner->ch == '=') {AddCh(scanner); goto case_45;}
			else if (scanner->ch == '>') {AddCh(scanner); goto case_65;}
			else {scanner->t->kind = 64; break;}
		case 78:
			if (scanner->ch == '=') {AddCh(scanner); goto case_46;}
			else {scanner->t->kind = 65; break;}
		case 79:
			if (scanner->ch == '=') {AddCh(scanner); goto case_47;}
			else if (scanner->ch == '&') {AddCh(scanner); goto case_56;}
			else {scanner->t->kind = 66; break;}
		case 80:
			if (scanner->ch == '=') {AddCh(scanner); goto case_48;}
			else {scanner->t->kind = 67; break;}
		case 81:
			if (scanner->ch == '=') {AddCh(scanner); goto case_49;}
			else if (scanner->ch == '>') {AddCh(scanner); goto case_58;}
			else if (scanner->ch == '-') {AddCh(scanner); goto case_60;}
			else {scanner->t->kind = 70; break;}
		case 82:
			if (scanner->ch == '=') {AddCh(scanner); goto case_50;}
			else if (scanner->ch == '+') {AddCh(scanner); goto case_59;}
			else {scanner->t->kind = 71; break;}
		case 83:
			if (scanner->ch == '=') {AddCh(scanner); goto case_51;}
			else if (scanner->ch == '|') {AddCh(scanner); goto case_57;}
			else {scanner->t->kind = 77; break;}
		case 84:
			if (scanner->ch == '=') {AddCh(scanner); goto case_52;}
			else {scanner->t->kind = 85; break;}
		case 85:
			if (scanner->ch == '=') {AddCh(scanner); goto case_55;}
			else {scanner->t->kind = 72; break;}
		case 86:
			if (scanner->ch == '>') {AddCh(scanner); goto case_67;}
			else {scanner->t->kind = 78; break;}
		case 87:
			case_87:
			if (scanner->ch >= '0' && scanner->ch <= '7') {AddCh(scanner); goto case_4;}
			else if (scanner->ch == 39) {AddCh(scanner); goto case_8;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 88:
			case_88:
			if (scanner->ch <= 9 || scanner->ch >= 11 && scanner->ch <= 12 || scanner->ch >= 14 && scanner->ch <= '!' || scanner->ch >= '#' && scanner->ch <= '[' || scanner->ch >= ']' && scanner->ch <= 65535) {AddCh(scanner); goto case_9;}
			else if (scanner->ch == '"') {AddCh(scanner); goto case_14;}
			else if (scanner->ch == 92) {AddCh(scanner); goto case_10;}
			else {scanner->t->kind = scanner->noSym; break;}
		case 89:
			case_89:
			if (scanner->ch == '=') {AddCh(scanner); goto case_42;}
			else {scanner->t->kind = 56; break;}
		case 90:
			case_90:
			if (scanner->ch == '=') {AddCh(scanner); goto case_43;}
			else {scanner->t->kind = 55; break;}

	}
	AppendVal(scanner, scanner->t);
	return scanner->t;
}

void InitializeScanner(CJcScanner* scanner, void* source, FGetLineFromSource getline){
	scanner->buffer = (CJcBuffer*)g_oInterface.Malloc(sizeof(CJcBuffer));
	InitializeBuffer(scanner->buffer, source, getline);
	InitializeStartStates(&scanner->start);
	InitializeKeywordMap(&scanner->keywords);
	Init(scanner);
}

void DestroyScanner(CJcScanner* scanner){
	char* cur = (char*) scanner->pFirstHeap;
	while(cur != NULL){
		cur = *(char**) (cur + HEAP_BLOCK_SIZE);
		g_oInterface.Free(scanner->pFirstHeap);
		scanner->pFirstHeap = cur;
	}
	g_oInterface.Free(scanner->tval);
	DestroyBuffer(scanner->buffer);
	g_oInterface.Free(scanner->buffer);
	DestroyKeywordMap(&scanner->keywords);
	DestroyStartStates(&scanner->start);
}

/* get the next token (possibly a token already seen during peeking) */
CJcToken* ScanToken(CJcScanner* scanner){
	if (scanner->tokens->next == NULL)
		return scanner->pt = scanner->tokens = NextToken(scanner);
	return scanner->pt = scanner->tokens = scanner->tokens->next;
}

/* peek for the next token, ignore pragmas */
CJcToken* PeekToken(CJcScanner* scanner){
	if (scanner->pt->next == NULL)	{
		do{
			scanner->pt = scanner->pt->next = NextToken(scanner);
		} while (scanner->pt->kind > scanner->maxT); /* skip pragmas */
	}
	else{
		do{
			scanner->pt = scanner->pt->next; 
		} while (scanner->pt->kind > scanner->maxT);
	}
	return scanner->pt;
}

/* make sure that peeking starts at the current scan position */
void ResetPeek(CJcScanner* scanner){
	scanner->pt = scanner->tokens;
}

