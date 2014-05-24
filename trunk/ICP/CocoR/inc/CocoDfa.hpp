
#include "CocoDef.hpp"

FOCP_BEGIN();

class COCOR_API CCocoParser;
class COCOR_API CCocoSymbolTable;

class COCOR_API CCocoDfa
{
private:
	int32 m_nMaxStates;
	int32 m_nLastStateNr;		// highest state number
	CCocoState *m_pFirstState;
	CCocoState *m_pLastState;	// last allocated state
	int32 m_nLastSimState;		// last non melted state
	CFile m_oFram;				// scanner frame input
	CFile m_oGen;				// generated scanner file
	CCocoSymbol *m_pCurSy;		// current token to be recognized (in FindTrans)
	CCocoNode *m_pCurGraph;		// start of graph for current token (in FindTrans)
	bool m_bIgnoreCase;			// true if input should be treated case-insensitively
	bool m_bDirtyDFA;			// DFA may become nondeterministic in MatchLiteral
	bool m_HasCtxMoves;			// DFA has context transitions
	bool *m_pExistLabel;		// checking the Labels (in order to avoid the warning messages)

	CCocoParser *m_pParser;		// other Coco objects
	CCocoSymbolTable *m_pSymTab;
	CCocoErrors *m_pErrors;
	CFile m_oTrace;

	CCocoMelted *m_pFirstMelted;	// head of melted state list
	CCocoComment *m_pFirstComment;	// list of comments

public:
	CCocoDfa(CCocoParser *pParser);

	//---------- Output primitives
	wchar_t* Ch(wchar_t ch);
	wchar_t* ChCond(wchar_t ch);
	void  PutRange(CCocoCharSet *s);

	//---------- State handling
	CCocoState* NewState();
	void NewTransition(CCocoState *pFrom, CCocoState *pTo, int32 nType, int nSymbol, int nTransitionCode);
	void CombineShifts();
	void FindUsedStates(CCocoState *pState, CBits *pUsed);
	void DeleteRedundantStates();
	CCocoState* TheState(CCocoNode *pNode);
	void Step(CCocoState *pFrom, CCocoNode *pNode, CBits *pStepped);
	void NumberNodes(CCocoNode *pNode, CCocoState *pState, bool renumIter);
	void FindTrans (CCocoNode *pNode, bool bStart, CBits *pMarked);
	void ConvertToStates(CCocoNode *pNode, CCocoSymbol *pSym);
	// match string against current automaton; store it either as a fixedToken or as a litToken
	void MatchLiteral(wchar_t* s, CCocoSymbol *pSym);
	void SplitActions(CCocoState *pState, CCocoAction *pActA, CCocoAction *pActB);
	bool Overlap(CCocoAction *pActA, CCocoAction *pActB);
	bool MakeUnique(CCocoState *pState); // return true if actions were split
	void MeltStates(CCocoState *pState);
	void FindCtxStates();
	void MakeDeterministic();
	void PrintStates();
	void CheckLabels();

	//---------------------------- actions --------------------------------
	CCocoAction* FindAction(CCocoState *pState, wchar_t ch);
	void GetTargetStates(CCocoAction *a, CBits* &pTargets, CCocoSymbol* &pEndOf, bool &bCtx);

	//------------------------- melted states ------------------------------
	CCocoMelted* NewMelted(CBits *pSet, CCocoState *pState);
	CBits* MeltedSet(int32 nr);
	CCocoMelted* StateWithSet(CBits *pSet);

	//------------------------ comments --------------------------------
	wchar_t* CommentStr(CCocoNode *pNode);
	void NewComment(CCocoNode *pFrom, CCocoNode *pTo, bool bNested);

	//------------------------ scanner generation ----------------------
	void GenComBody(CCocoComment *pComment);
	void GenCommentHeader(CCocoComment *pComment, int32 i);
	void GenComment(CCocoComment *pComment, int32 i);
	void CopyFramePart(const wchar_t* pStop);
	wchar_t* SymName(Symbol *sym); // real name value is stored in Tab.literals
	void GenLiterals ();
	int GenNamespaceOpen(const wchar_t* sName);
	void GenNamespaceClose(int nNrOfNs);
	void WriteState(CCocoState *pState);
	void WriteStartTab();
	void OpenGen(const wchar_t* sGenName, bool bBackUp); /* pdt */
	void WriteScanner();
};

FOCP_END();
