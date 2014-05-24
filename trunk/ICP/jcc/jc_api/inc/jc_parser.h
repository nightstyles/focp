
#if !defined(_jc_parser_h_)
#define _jc_parser_h_
#include "jc_expr.h"
#include "jc_symbol.h"
#include "jc_goto.h"
#include "jc_break.h"
#include "jc_continue.h"
#include "jc_switch.h"
#include "jc_struct.h"
#include "jc_enum.h"
#include "jc_array.h"

#include "jc_scanner.h"
#include "jc_error.h"

typedef struct CJcParser{
	int _EOF;
	int _Identifier;
	int _CharConst;
	int _StringConst;
	int _IntegerConst;
	int _FloatConst;
	int _auto;
	int _break;
	int _case;
	int _char;
	int _const;
	int _continue;
	int _default;
	int _do;
	int _double;
	int _else;
	int _enum;
	int _extern;
	int _float;
	int _for;
	int _goto;
	int _host;
	int _if;
	int _int;
	int _long;
	int _register;
	int _return;
	int _share;
	int _short;
	int _signed;
	int _sizeof;
	int _static;
	int _struct;
	int _switch;
	int _typedef;
	int _union;
	int _unsigned;
	int _void;
	int _volatile;
	int _while;
	int _ellipsis;
	int _rshifteq;
	int _lshifteq;
	int _noteq;
	int _modeq;
	int _xoreq;
	int _andeq;
	int _muleq;
	int _subeq;
	int _addeq;
	int _oreq;
	int _diveq;
	int _mteq;
	int _lteq;
	int _equal;
	int _lshift;
	int _rshift;
	int _land;
	int _lor;
	int _pointer;
	int _inc;
	int _dec;
	int _tilde;
	int _not;
	int _mod;
	int _xor;
	int _and;
	int _mul;
	int _lparentheses;
	int _rparentheses;
	int _sub;
	int _add;
	int _assign;
	int _lbrace;
	int _rbrace;
	int _lbrack;
	int _rbrack;
	int _or;
	int _colon;
	int _scolon;
	int _lt;
	int _mt;
	int _comma;
	int _dot;
	int _quest;
	int _div;
	int maxT;

	int errDist;
	int minErrDist;
	CJcScanner *scanner;
	CJcErrorSystem* pErrorSystem;
	CJcToken *t;			/* last recognized token */
	CJcToken *la;			/* lookahead token */
CJcSymbolStack oSymbolStack;
	CJcGotoStack oGotoStack;
	CJcBreakStack oBreakStack;
	CJcContinueStack oContinueStack;
	CJcSwitchStack oSwitchStack;
	CJcFunctionInfo* pFunction;
	jc_bool bSupportHostSymbol;
	jc_bool bSupportShareSymbol;


}CJcParser;

void InitializeParser(CJcParser* parser, CJcScanner *scanner, CJcErrorSystem *pErrorSystem, int argc, char* argv[]);
void DestroyParser(CJcParser* parser);
void SetError(CJcParser* parser, int bWarning, const char* msg, ...);
void SetErrorEx(CJcParser* parser, int bWarning, char* fname, int line, int col, const char *msg, ...);
void Parse(CJcParser* parser);
CJcToken* Get(CJcParser* parser);

#endif

