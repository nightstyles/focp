
#include "AFC.hpp"

#ifndef _ICP_COCODEF_HPP_
#define _ICP_COCODEF_HPP_

#if defined(COCOR_EXPORTS)
#define COCOR_API FOCP_EXPORT
#else
#define COCOR_API FOCP_IMPORT
#endif

FOCP_BEGIN();

struct CCocoCharClass;
struct CCocoComment;
struct CCocoPosition;
struct CCocoState;
struct CCocoAction;
struct CCocoTarget;

typedef CRangeSet<wchar_t> CCocoCharSet;

struct CCocoCharClass
{
	int32 nClassId;
	wchar_t* sName;
	CCocoCharSet* pSet;
};

struct CCocoPosition 
{
	int32 nBegin;    // start relative to the beginning of the file
	int32 nEnd;      // end of stretch
	int32 nCol;      // column number of start position
	int32 nLine;     // line number of beginnnig of source code stretch
};

struct CCocoState
{
	int32 nNr;
	CCocoAction *pFirstAction;
	CCocoSymbol *pEndOf;
	bool bCtx;
	CCocoState* pNext;
};

struct CCocoAction  		// action of finite automaton
{
	int32 nType;			// type of action symbol: clas, chr
	int32 nSymbol;			// action symbol
	int32 nTransitionCode;	// transition code: normalTrans, contextTrans
	CCocoTarget *pTarget;	// states reached from this action
	CCocoAction *pNext;
};

struct CCocoTarget		// set of states that are reached by an action  				
{
	CCocoState* pState;	// target state
	CCocoTarget *pNext;
};

struct CCocoComment  					// info about comment syntax
{
	wchar_t* sSart;
	wchar_t* sStop;
	bool bNested;
	CCocoComment *pNext;
};

struct CCocoGraph
{
	CCocoNode *l;	// left end of graph = head
	CCocoNode *r;	// right end of graph = list of nodes to be linked to successor graph
};

struct CCocoMelted			// info about melted states
{
	CBits *pSet;			// set of old states
	CCocoState *pState;		// new state
	CCocoMelted *pNext;
};

struct CCocoNode
{
	int32 nNo;		// node number
	int32 nType;		// t, nt, wt, chr, clas, any, eps, sem, sync, alt, iter, opt, rslv
	CCocoNode *pNext;	// to successor node
	CCocoNode *pDown;	// alt: to next alternative
	CCocoNode *pSub;	// alt, iter, opt: to first node of substructure
	bool bUp;			// true: "next" leads to successor in enclosing structure
	CCocoSymbol *pSym;	// nt, t, wt: symbol represented by this node
	int32 nVal;			// chr:  ordinal character value
						// clas: index of character class
	int32 nCode;		// chr, clas: transition code
	CBits *pSet;		// any, sync: the set represented by this node
	CCocoPosition *pPos;// nt, t, wt: pos of actual attributes
						// sem: pos of semantic action in source text
						// rslv: pos of resolver in source text
	int32 nLine;		// source text line number of item in this node
	CCocoState *pState;	// DFA state corresponding to this node
						// (only used in DFA.ConvertToStates)
}; 

struct CCocoSymbol
{
	int32 nNo;				// symbol number
	int32 nType;			// t, nt, pr, unknown, rslv /* ML 29_11_2002 slv added */ /* AW slv --> rslv */
	wchar_t *sName;			// symbol name
	CCocoNode *pGraph;		// nt: to first node of syntax graph
	int32 nTokenKind;		// t:  token kind (fixedToken, classToken, ...)
	bool bDeletable;		// nt: true if nonterminal is deletable
	bool bFirstReady;		// nt: true if terminal start symbols have already been computed
	CBits *pFirst;			// nt: terminal start symbols
	CBits *pFollow;			// nt: terminal followers
	CBits *pNonTerminals;	// nt: nonterminals whose followers have to be added to this sym
	int32 nLine;			// source text line number of item in this node
	CCocoPosition *pAttrPos;	// nt: position of attributes in source text (or null)
	CCocoPosition *pSemPos;		// pr: pos of semantic action in source text (or null)
								// nt: pos of local declarations in source text (or null)
};

struct CCocoSymbolTable
{
	CCocoPosition *pSemDeclPos;	// position of global semantic declarations
	CCocoCharSet *pIgnored;	// characters ignored by the scanner
	bool bDebug[10];		// debug and test switches
	CCocoSymbol *pGramSym;	// root nonterminal; filled by ATG
	CCocoSymbol *pEofSym;	// end of file symbol
	CCocoSymbol *pNoSym;	// used in case of an error
	CBits *pAllSyncSets;	// union of all synchronisation sets
	HashTable *literals;	// symbols that are used as literals

	wchar_t* srcName;            // name of the atg file (including path)
	wchar_t* srcDir;             // directory path of the atg file
	wchar_t* nsName;             // namespace for generated files
	wchar_t* frameDir;           // directory containing the frame files
	wchar_t* outDir;             // directory for generated files
	bool checkEOF;               // should coco generate a check for EOF at
	                             // the end of Parser.Parse():
	bool emitLines;              // emit line directives in generated parser

	BitArray *visited;                // mark list for graph traversals
	Symbol *curSy;                     // current symbol in computation of sets

	Parser *parser;                    // other Coco objects
	FILE* trace;

	Errors *errors;

	ArrayList *terminals;
	ArrayList *pragmas;
	ArrayList *nonterminals;


	ArrayList *nodes;
	static const char* nTyp[];
	Node *dummyNode;

	ArrayList *classes;
	int dummyName;
};

FOCP_END();

#endif
