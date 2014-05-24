/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     TINT8 = 259,
     TINT16 = 260,
     TINT32 = 261,
     TINT64 = 262,
     TUINT8 = 263,
     TUINT16 = 264,
     TUINT32 = 265,
     TUINT64 = 266,
     TFLOAT = 267,
     TDOUBLE = 268,
     TDATE = 269,
     TTIME = 270,
     TDATETIME = 271,
     TSTRING = 272,
     CSTRING = 273,
     VSTRING = 274,
     VCSTRING = 275,
     RAW = 276,
     VRAW = 277,
     TSIZE = 278,
     DEFAULT = 279,
     NOT = 280,
     TNULL = 281,
     TNUMBER = 282,
     XSTRING = 283,
     UNIQUE = 284,
     FOREIGN = 285,
     RBTREE = 286,
     HASH = 287,
     REPEAT = 288,
     CAPACITY = 289,
     CREATE = 290,
     DATABASE = 291,
     TABLE = 292,
     INDEX = 293,
     ON = 294,
     BY = 295,
     INSERT = 296,
     INTO = 297,
     VALUES = 298,
     COMMENT = 299,
     USE = 300,
     TDELETE = 301,
     FROM = 302,
     WHERE = 303,
     AND = 304,
     OR = 305,
     UPDATE = 306,
     SET = 307,
     TRUNCATE = 308,
     SELECT = 309,
     DESC = 310,
     QUIT = 311,
     HELP = 312,
     ORDERBY = 313,
     ASC = 314,
     DEC = 315,
     BATCH = 316,
     GE = 317,
     LE = 318,
     EQ = 319,
     MT = 320,
     LT = 321,
     NE = 322,
     PE = 323,
     SE = 324,
     AE = 325,
     OE = 326,
     XE = 327,
     BN = 328,
     ME = 329,
     DE = 330,
     MOD = 331,
     TSQLERROR = 332
   };
#endif
/* Tokens.  */
#define IDENTIFIER 258
#define TINT8 259
#define TINT16 260
#define TINT32 261
#define TINT64 262
#define TUINT8 263
#define TUINT16 264
#define TUINT32 265
#define TUINT64 266
#define TFLOAT 267
#define TDOUBLE 268
#define TDATE 269
#define TTIME 270
#define TDATETIME 271
#define TSTRING 272
#define CSTRING 273
#define VSTRING 274
#define VCSTRING 275
#define RAW 276
#define VRAW 277
#define TSIZE 278
#define DEFAULT 279
#define NOT 280
#define TNULL 281
#define TNUMBER 282
#define XSTRING 283
#define UNIQUE 284
#define FOREIGN 285
#define RBTREE 286
#define HASH 287
#define REPEAT 288
#define CAPACITY 289
#define CREATE 290
#define DATABASE 291
#define TABLE 292
#define INDEX 293
#define ON 294
#define BY 295
#define INSERT 296
#define INTO 297
#define VALUES 298
#define COMMENT 299
#define USE 300
#define TDELETE 301
#define FROM 302
#define WHERE 303
#define AND 304
#define OR 305
#define UPDATE 306
#define SET 307
#define TRUNCATE 308
#define SELECT 309
#define DESC 310
#define QUIT 311
#define HELP 312
#define ORDERBY 313
#define ASC 314
#define DEC 315
#define BATCH 316
#define GE 317
#define LE 318
#define EQ 319
#define MT 320
#define LT 321
#define NE 322
#define PE 323
#define SE 324
#define AE 325
#define OE 326
#define XE 327
#define BN 328
#define ME 329
#define DE 330
#define MOD 331
#define TSQLERROR 332




/* Copy the first part of user declarations.  */


#include "SqlEnv.hpp"

#include <stdlib.h>

void CopyName(char* d, const char* s);

FOCP_BEGIN();

#define YYPARSE_PARAM lexer

void yyerror(const char* msg);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)

typedef union YYSTYPE {
	bool 	b;
	int 	i;
	char* 	sv;
	char	nam[FOCP_NAME::MDB_NAME_MAXLEN+1];
	double 	d;
} YYSTYPE;
/* Line 196 of yacc.c.  */

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */


#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  98
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   204

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  83
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  62
/* YYNRULES -- Number of rules. */
#define YYNRULES  146
/* YYNRULES -- Number of states. */
#define YYNSTATES  239

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   332

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      79,    80,    81,     2,    78,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    82,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     6,     9,    13,    14,    18,    23,
      24,    26,    29,    33,    35,    37,    39,    41,    43,    45,
      47,    49,    51,    53,    55,    57,    59,    61,    64,    67,
      70,    73,    76,    79,    81,    84,    85,    89,    90,    99,
     101,   103,   105,   109,   110,   114,   116,   118,   119,   121,
     123,   125,   129,   130,   140,   142,   146,   148,   152,   156,
     160,   162,   166,   168,   169,   174,   176,   178,   180,   182,
     184,   186,   188,   189,   195,   197,   201,   205,   209,   213,
     217,   221,   225,   229,   233,   237,   241,   245,   247,   251,
     253,   257,   258,   264,   266,   270,   273,   278,   281,   285,
     289,   291,   293,   297,   299,   301,   303,   307,   311,   315,
     316,   329,   331,   332,   334,   335,   340,   341,   345,   346,
     349,   353,   355,   357,   361,   363,   365,   367,   370,   372,
     375,   378,   381,   384,   387,   390,   393,   396,   399,   402,
     405,   408,   411,   414,   417,   419,   421
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short int yyrhs[] =
{
     144,     0,    -1,     3,    -1,    -1,    25,    26,    -1,    23,
      64,    27,    -1,    -1,    24,    64,    28,    -1,    84,    91,
      85,    87,    -1,    -1,    88,    -1,    89,   128,    -1,    89,
      78,    88,    -1,     4,    -1,     5,    -1,     6,    -1,     7,
      -1,     8,    -1,     9,    -1,    10,    -1,    11,    -1,    12,
      -1,    13,    -1,    14,    -1,    15,    -1,    16,    -1,    90,
      -1,    17,    86,    -1,    18,    86,    -1,    21,    86,    -1,
      19,    86,    -1,    20,    86,    -1,    22,    86,    -1,     3,
      -1,    45,     3,    -1,    -1,    34,    64,    27,    -1,    -1,
      35,    37,     3,    95,    79,    89,    80,    93,    -1,    94,
      -1,     3,    -1,    97,    -1,    98,    78,    97,    -1,    -1,
      79,    98,    80,    -1,    27,    -1,    28,    -1,    -1,   100,
      -1,    26,    -1,   101,    -1,   102,    78,   101,    -1,    -1,
      41,    42,     3,   104,    99,    43,    79,   102,    80,    -1,
     103,    -1,    46,    47,     3,    -1,   106,    -1,   106,    48,
     110,    -1,     3,   112,   100,    -1,     3,   112,    26,    -1,
     108,    -1,   109,    49,   108,    -1,   109,    -1,    -1,   110,
      50,   111,   109,    -1,    62,    -1,    63,    -1,    66,    -1,
      65,    -1,    64,    -1,    67,    -1,   107,    -1,    -1,    51,
       3,   115,    52,   118,    -1,   114,    -1,   114,    48,   110,
      -1,     3,    64,    73,    -1,     3,    64,   100,    -1,     3,
      64,    26,    -1,     3,    68,   100,    -1,     3,    69,   100,
      -1,     3,    74,   100,    -1,     3,    75,   100,    -1,     3,
      76,   100,    -1,     3,    70,   100,    -1,     3,    71,   100,
      -1,     3,    72,   100,    -1,   117,    -1,   118,    78,   117,
      -1,   116,    -1,    53,    37,     3,    -1,    -1,    54,   122,
     127,    47,     3,    -1,   121,    -1,   121,    48,   110,    -1,
     121,   124,    -1,   121,    48,   110,   124,    -1,    58,     3,
      -1,    58,     3,    59,    -1,    58,     3,    60,    -1,   123,
      -1,     3,    -1,   126,    78,     3,    -1,    81,    -1,   126,
      -1,    44,    -1,    55,    37,     3,    -1,    55,    37,    81,
      -1,    55,    36,    81,    -1,    -1,    35,   133,   134,    38,
       3,    39,     3,   131,    79,   138,    80,   136,    -1,   130,
      -1,    -1,    29,    -1,    -1,    30,    79,     3,    80,    -1,
      -1,    79,    27,    80,    -1,    -1,    40,    31,    -1,    40,
      32,   135,    -1,     3,    -1,   137,    -1,   138,    78,   137,
      -1,    56,    -1,    57,    -1,    61,    -1,    33,    27,    -1,
      82,    -1,    92,    82,    -1,    96,    82,    -1,    88,    82,
      -1,   132,    82,    -1,   105,    82,    -1,   113,    82,    -1,
     119,    82,    -1,   120,    82,    -1,   125,    82,    -1,   129,
      82,    -1,   128,    82,    -1,   139,    82,    -1,   140,    82,
      -1,   141,    82,    -1,   142,    82,    -1,     1,    -1,   143,
      -1,   144,   143,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,    77,    77,    84,    85,    96,   108,   109,   121,   123,
     124,   125,   126,   129,   131,   133,   135,   137,   139,   141,
     143,   145,   147,   149,   151,   153,   157,   166,   175,   184,
     193,   202,   211,   220,   245,   254,   258,   267,   266,   276,
     284,   291,   292,   296,   297,   300,   302,   306,   307,   313,
     320,   321,   325,   324,   334,   342,   351,   352,   355,   363,
     369,   370,   373,   374,   374,   377,   378,   379,   380,   381,
     382,   385,   394,   393,   403,   404,   408,   413,   419,   424,
     430,   436,   442,   448,   454,   460,   466,   474,   475,   478,
     486,   497,   496,   510,   511,   512,   513,   516,   517,   518,
     521,   529,   534,   541,   541,   543,   546,   554,   562,   570,
     569,   583,   592,   596,   604,   608,   616,   620,   629,   633,
     638,   645,   652,   653,   656,   664,   671,   678,   689,   690,
     691,   692,   693,   694,   695,   696,   697,   698,   699,   700,
     701,   702,   703,   704,   705,   714,   714
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "IDENTIFIER", "TINT8", "TINT16",
  "TINT32", "TINT64", "TUINT8", "TUINT16", "TUINT32", "TUINT64", "TFLOAT",
  "TDOUBLE", "TDATE", "TTIME", "TDATETIME", "TSTRING", "CSTRING",
  "VSTRING", "VCSTRING", "RAW", "VRAW", "TSIZE", "DEFAULT", "NOT", "TNULL",
  "TNUMBER", "XSTRING", "UNIQUE", "FOREIGN", "RBTREE", "HASH", "REPEAT",
  "CAPACITY", "CREATE", "DATABASE", "TABLE", "INDEX", "ON", "BY", "INSERT",
  "INTO", "VALUES", "COMMENT", "USE", "TDELETE", "FROM", "WHERE", "AND",
  "OR", "UPDATE", "SET", "TRUNCATE", "SELECT", "DESC", "QUIT", "HELP",
  "ORDERBY", "ASC", "DEC", "BATCH", "GE", "LE", "EQ", "MT", "LT", "NE",
  "PE", "SE", "AE", "OE", "XE", "BN", "ME", "DE", "MOD", "TSQLERROR",
  "','", "'('", "')'", "'*'", "';'", "$accept", "def_field", "def_notnull",
  "def_size", "def_default", "field", "fields", "basetype", "def_datatype",
  "usedatabase", "table_capacity", "createtable0", "@1", "createtable",
  "insfld", "insflds", "insfldlist", "value", "insvalue0", "insvalue",
  "insert0", "@2", "insert", "delete0", "delete1", "wherecond_base",
  "wherecond_and", "wherecond", "@3", "operator", "delete", "update0",
  "@4", "update1", "setexpr0", "setexpr", "update", "truncate", "select0",
  "@5", "select1", "orderby_sentence", "select", "selspecfld", "selflds",
  "comment", "desc", "createindex0", "@6", "createindex", "indexattr",
  "foreignattr", "hashrate", "indexalgo", "indexfld0", "indexfld", "quit",
  "help", "batch", "repeat", "stmt", "stmtlist", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,    44,    40,
      41,    42,    59
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    83,    84,    85,    85,    86,    87,    87,    88,    89,
      89,    89,    89,    90,    90,    90,    90,    90,    90,    90,
      90,    90,    90,    90,    90,    90,    91,    91,    91,    91,
      91,    91,    91,    91,    92,    93,    93,    95,    94,    96,
      97,    98,    98,    99,    99,   100,   100,   101,   101,   101,
     102,   102,   104,   103,   105,   106,   107,   107,   108,   108,
     109,   109,   110,   111,   110,   112,   112,   112,   112,   112,
     112,   113,   115,   114,   116,   116,   117,   117,   117,   117,
     117,   117,   117,   117,   117,   117,   117,   118,   118,   119,
     120,   122,   121,   123,   123,   123,   123,   124,   124,   124,
     125,   126,   126,   127,   127,   128,   129,   129,   129,   131,
     130,   132,   133,   133,   134,   134,   135,   135,   136,   136,
     136,   137,   138,   138,   139,   140,   141,   142,   143,   143,
     143,   143,   143,   143,   143,   143,   143,   143,   143,   143,
     143,   143,   143,   143,   143,   144,   144
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     0,     2,     3,     0,     3,     4,     0,
       1,     2,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       2,     2,     2,     1,     2,     0,     3,     0,     8,     1,
       1,     1,     3,     0,     3,     1,     1,     0,     1,     1,
       1,     3,     0,     9,     1,     3,     1,     3,     3,     3,
       1,     3,     1,     0,     4,     1,     1,     1,     1,     1,
       1,     1,     0,     5,     1,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     1,
       3,     0,     5,     1,     3,     2,     4,     2,     3,     3,
       1,     1,     3,     1,     1,     1,     3,     3,     3,     0,
      12,     1,     0,     1,     0,     4,     0,     3,     0,     2,
       3,     1,     1,     3,     1,     1,     1,     2,     1,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     1,     2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,   144,     2,     0,   112,     0,   105,     0,     0,     0,
       0,    91,     0,   124,   125,   126,   128,     0,     0,     0,
      39,     0,    54,     0,    56,    71,     0,    74,    89,     0,
       0,    93,   100,     0,     0,     0,   111,     0,     0,     0,
       0,     0,   145,     0,   127,   113,     0,   114,     0,    34,
       0,    72,     0,     0,     0,     0,    33,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
       0,     0,     0,     0,     0,     0,    26,     3,   131,   129,
     130,   133,     0,   134,     0,   135,   136,     0,     0,    95,
     137,   139,   138,   132,   140,   141,   142,   143,     1,   146,
      37,     0,     0,    52,    55,     0,    90,   101,   103,   104,
       0,   108,   106,   107,     0,    27,    28,    30,    31,    29,
      32,     0,     6,     0,    60,    62,    57,    75,    94,    97,
       0,     0,     0,    43,     0,     0,     0,     0,     4,     0,
       8,    65,    66,    69,    68,    67,    70,     0,     0,    63,
      96,    98,    99,     9,     0,     0,     0,     0,     0,    87,
      73,   102,    92,     5,     0,    59,    45,    46,    58,    61,
       0,    10,     0,   115,     0,    40,    41,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
      64,     0,    35,    11,   109,     0,    44,    47,    78,    76,
      77,    79,    80,    84,    85,    86,    81,    82,    83,    88,
      12,     0,    38,     0,    42,    49,    48,    50,     0,     0,
       0,    47,    53,    36,   121,   122,     0,    51,     0,   118,
     123,     0,   110,   119,   116,     0,   120,     0,   117
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    17,   122,   115,   140,    18,   172,    76,    77,    19,
     212,    20,   130,    21,   176,   177,   157,   216,   217,   218,
      22,   133,    23,    24,    25,   124,   125,   126,   170,   147,
      26,    27,   105,    28,   159,   160,    29,    30,    31,    53,
      32,    89,    33,   109,   110,    34,    35,    36,   213,    37,
      47,   102,   236,   232,   225,   226,    38,    39,    40,    41,
      42,    43
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -150
static const short int yypact[] =
{
      33,  -150,  -150,    -3,   -20,     1,  -150,    49,    12,    61,
      48,  -150,   -29,  -150,  -150,  -150,  -150,   113,    28,    29,
    -150,    30,  -150,    31,    66,  -150,    54,    89,  -150,    56,
      57,   -42,  -150,    58,    59,    60,  -150,    62,    63,    64,
      67,    68,  -150,     0,  -150,  -150,    78,   117,   140,  -150,
     145,  -150,   148,    -1,    71,     2,  -150,  -150,  -150,  -150,
    -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,
     130,   130,   130,   130,   130,   130,  -150,   129,  -150,  -150,
    -150,  -150,   152,  -150,   152,  -150,  -150,   152,   153,  -150,
    -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,
    -150,    79,   119,  -150,  -150,   107,  -150,  -150,  -150,    82,
     114,  -150,  -150,  -150,    98,  -150,  -150,  -150,  -150,  -150,
    -150,   137,   141,    42,  -150,   115,   116,   116,   -40,   -21,
      88,   165,   166,    91,   168,   169,   170,   147,  -150,   111,
    -150,  -150,  -150,  -150,  -150,  -150,  -150,    43,   152,  -150,
    -150,  -150,  -150,   173,    97,   139,   176,   138,   -49,  -150,
     102,  -150,  -150,  -150,   154,  -150,  -150,  -150,  -150,  -150,
     152,  -150,   -30,  -150,   180,  -150,  -150,   -31,   105,   -15,
      45,    45,    45,    45,    45,    45,    45,    45,   168,  -150,
     115,   173,   151,  -150,  -150,   176,  -150,    65,  -150,  -150,
    -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,  -150,
    -150,   122,  -150,   108,  -150,  -150,  -150,  -150,   -18,   161,
     186,    65,  -150,  -150,  -150,  -150,   -13,  -150,   186,   150,
    -150,    44,  -150,  -150,   112,   167,  -150,   118,  -150
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -150,  -150,  -150,   -43,  -150,  -149,  -150,  -150,  -150,  -150,
    -150,  -150,  -150,  -150,    -2,  -150,  -150,   -84,   -26,  -150,
    -150,  -150,  -150,  -150,  -150,    51,    22,   -47,  -150,  -150,
    -150,  -150,  -150,  -150,     8,  -150,  -150,  -150,  -150,  -150,
    -150,    69,  -150,  -150,  -150,    32,  -150,  -150,  -150,  -150,
    -150,  -150,  -150,  -150,   -28,  -150,  -150,  -150,  -150,  -150,
     158,  -150
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      98,     1,   107,     2,   171,   112,    87,    54,    55,    45,
     149,   198,   166,   167,     6,   179,    88,    46,    88,   180,
     181,   182,   183,   184,    44,   185,   186,   187,   116,   117,
     118,   119,   120,     3,     1,     4,     2,   127,   151,   152,
     128,     5,   210,    48,     6,     7,     8,   195,   191,   196,
     192,     9,    49,    10,    11,    12,    13,    14,   199,    50,
     221,    15,   222,   168,    51,   228,     3,   229,     4,   165,
     166,   167,   166,   167,     5,   233,   234,     6,     7,     8,
     108,   100,    16,   113,     9,    52,    10,    11,    12,    13,
      14,   215,   166,   167,    15,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   141,   142,   143,   144,   145,   146,
      78,    79,    80,    81,    82,    16,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    83,    84,    85,    86,
      90,    91,    92,   103,    93,    94,    95,   101,   104,    96,
      97,   106,   111,   114,   121,   123,   129,   132,   131,   134,
     135,   136,   137,   138,   148,   139,   149,   153,   154,   155,
     156,   158,   161,   162,   163,   164,     2,   173,   174,   175,
     188,   178,   189,   194,   197,   211,   219,   220,   223,   224,
     231,   235,   190,   214,   237,   227,   209,   150,   238,   169,
     230,    99,     0,     0,   193
};

static const short int yycheck[] =
{
       0,     1,     3,     3,   153,     3,    48,    36,    37,    29,
      50,    26,    27,    28,    44,    64,    58,    37,    58,    68,
      69,    70,    71,    72,    27,    74,    75,    76,    71,    72,
      73,    74,    75,    33,     1,    35,     3,    84,    59,    60,
      87,    41,   191,    42,    44,    45,    46,    78,    78,    80,
      80,    51,     3,    53,    54,    55,    56,    57,    73,    47,
      78,    61,    80,   147,     3,    78,    33,    80,    35,    26,
      27,    28,    27,    28,    41,    31,    32,    44,    45,    46,
      81,     3,    82,    81,    51,    37,    53,    54,    55,    56,
      57,    26,    27,    28,    61,   179,   180,   181,   182,   183,
     184,   185,   186,   187,    62,    63,    64,    65,    66,    67,
      82,    82,    82,    82,    48,    82,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    82,    48,    82,    82,
      82,    82,    82,     3,    82,    82,    82,    30,     3,    82,
      82,     3,    81,    23,    25,     3,     3,    38,    79,    52,
      78,    47,    64,    26,    49,    24,    50,    79,     3,     3,
      79,     3,     3,     3,    27,    64,     3,    80,    39,     3,
      78,    43,    28,     3,    79,    34,    64,    79,    27,     3,
      40,    79,   170,   195,    27,   221,   188,   128,    80,   148,
     228,    43,    -1,    -1,   172
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     1,     3,    33,    35,    41,    44,    45,    46,    51,
      53,    54,    55,    56,    57,    61,    82,    84,    88,    92,
      94,    96,   103,   105,   106,   107,   113,   114,   116,   119,
     120,   121,   123,   125,   128,   129,   130,   132,   139,   140,
     141,   142,   143,   144,    27,    29,    37,   133,    42,     3,
      47,     3,    37,   122,    36,    37,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    90,    91,    82,    82,
      82,    82,    48,    82,    48,    82,    82,    48,    58,   124,
      82,    82,    82,    82,    82,    82,    82,    82,     0,   143,
       3,    30,   134,     3,     3,   115,     3,     3,    81,   126,
     127,    81,     3,    81,    23,    86,    86,    86,    86,    86,
      86,    25,    85,     3,   108,   109,   110,   110,   110,     3,
      95,    79,    38,   104,    52,    78,    47,    64,    26,    24,
      87,    62,    63,    64,    65,    66,    67,   112,    49,    50,
     124,    59,    60,    79,     3,     3,    79,    99,     3,   117,
     118,     3,     3,    27,    64,    26,    27,    28,   100,   108,
     111,    88,    89,    80,    39,     3,    97,    98,    43,    64,
      68,    69,    70,    71,    72,    74,    75,    76,    78,    28,
     109,    78,    80,   128,     3,    78,    80,    79,    26,    73,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   117,
      88,    34,    93,   131,    97,    26,   100,   101,   102,    64,
      79,    78,    80,    27,     3,   137,   138,   101,    78,    80,
     137,    40,   136,    31,    32,    79,   135,    27,    80
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
#define YYLEX ((yyFlexLexer*)YYPARSE_PARAM)->yylex(&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()
    ;
#endif
#endif
{
  /* The look-ahead symbol.  */
CSqlEnv* pSqlEnv = ((CSqlEnv*)YYPARSE_PARAM);
CSqlDataBase * pDb = pSqlEnv->GetDataBase();
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

    {
		if(pDb)
			pDb->m_pCurrentTable->AddField((yyvsp[0].nam));
	;}
    break;

  case 4:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->null = true;
		}
	;}
    break;

  case 5:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->len = (int)strtod((yyvsp[0].sv), NULL);
		}
		free((yyvsp[0].sv));
	;}
    break;

  case 7:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->defval = (yyvsp[0].sv);
		}
		free((yyvsp[0].sv));
	;}
    break;

  case 13:

    { (yyval.i) = MDB_INT8_FIELD; ;}
    break;

  case 14:

    { (yyval.i) = MDB_INT16_FIELD; ;}
    break;

  case 15:

    { (yyval.i) = MDB_INT32_FIELD; ;}
    break;

  case 16:

    { (yyval.i) = MDB_INT64_FIELD; ;}
    break;

  case 17:

    { (yyval.i) = MDB_UINT8_FIELD; ;}
    break;

  case 18:

    { (yyval.i) = MDB_UINT16_FIELD; ;}
    break;

  case 19:

    { (yyval.i) = MDB_UINT32_FIELD; ;}
    break;

  case 20:

    { (yyval.i) = MDB_UINT64_FIELD; ;}
    break;

  case 21:

    { (yyval.i) = MDB_FLOAT_FIELD; ;}
    break;

  case 22:

    { (yyval.i) = MDB_DOUBLE_FIELD; ;}
    break;

  case 23:

    { (yyval.i) = MDB_DATE_FIELD; ;}
    break;

  case 24:

    { (yyval.i) = MDB_TIME_FIELD; ;}
    break;

  case 25:

    { (yyval.i) = MDB_DATETIME_FIELD; ;}
    break;

  case 26:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = (yyvsp[0].i);
		}
	;}
    break;

  case 27:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_CHAR_FIELD;
		}
	;}
    break;

  case 28:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_LCHAR_FIELD;
		}
	;}
    break;

  case 29:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_RAW_FIELD;
		}
	;}
    break;

  case 30:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARCHAR_FIELD;
		}
	;}
    break;

  case 31:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARLCHAR_FIELD;
		}
	;}
    break;

  case 32:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
				pField->type = MDB_VARRAW_FIELD;
		}
	;}
    break;

  case 33:

    {
		if(pDb)
		{
			CSqlField* pField = pDb->m_pCurrentTable->GetCurrentField();
			if(pField)
			{
				CSqlField* pAliasType = pDb->m_oAliasTable.FindField((yyvsp[0].nam));
				if(pAliasType)
				{
					pField->type = pAliasType->type;
					pField->len = pAliasType->len;
					pField->null = pAliasType->null;
					pField->defval = pAliasType->defval;
				}
				else
				{
					FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("'%s' is invalid type", (yyvsp[0].nam)));
					pField->type = MDB_INT32_FIELD;
				}
			}
		}
	;}
    break;

  case 34:

    {
		pSqlEnv->SelectDataBase((yyvsp[0].nam));
		pSqlEnv->Flush();
		pDb = pSqlEnv->GetDataBase();
	;}
    break;

  case 35:

    {
		if(pDb)
			pDb->m_pCurrentTable->SetMaxRecordNum(0xFFFFFFFF);
	;}
    break;

  case 36:

    {
		if(pDb)
			pDb->m_pCurrentTable->SetMaxRecordNum(atoi((yyvsp[0].sv)));
		free((yyvsp[0].sv));
	;}
    break;

  case 37:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlTable((yyvsp[0].nam));
	;}
    break;

  case 39:

    {
		if(pDb)
			pDb->CreateTable();
		pSqlEnv->Flush();
	;}
    break;

  case 40:

    {
		if(pDb)
			pDb->m_oInsert.AddField((yyvsp[0].nam));
	;}
    break;

  case 43:

    { ;}
    break;

  case 45:

    { (yyval.sv) = (yyvsp[0].sv); ;}
    break;

  case 46:

    { (yyval.sv) = (yyvsp[0].sv); ;}
    break;

  case 48:

    {
		if(pDb)
			pDb->m_oInsert.AddValue((yyvsp[0].sv));
		free((yyvsp[0].sv));
	;}
    break;

  case 49:

    {
		if(pDb)
			pDb->m_oInsert.AddValue("");
	;}
    break;

  case 52:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlInsert((yyvsp[0].nam));
	;}
    break;

  case 54:

    {
		if(pDb)
			pDb->InsertRecord();
		pSqlEnv->Flush();
	;}
    break;

  case 55:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlDelete((yyvsp[0].nam));
	;}
    break;

  case 58:

    {
		if(pDb)
		{
			pDb->m_pWhere->AddCond((yyvsp[-2].nam), (yyvsp[-1].i), (yyvsp[0].sv));
			free((yyvsp[0].sv));
		}
	;}
    break;

  case 59:

    {
		if(pDb)	pDb->m_pWhere->AddCond((yyvsp[-2].nam), (yyvsp[-1].i), "");
	;}
    break;

  case 63:

    {pDb->m_pWhere->NewSet();;}
    break;

  case 65:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_MOREEQUAL; ;}
    break;

  case 66:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_LESSEQUAL; ;}
    break;

  case 67:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_LESS; ;}
    break;

  case 68:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_MORE; ;}
    break;

  case 69:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_EQUAL; ;}
    break;

  case 70:

    { (yyval.i) = MDB_SQLPARA_OPERATOR_NOTEQUAL; ;}
    break;

  case 71:

    {
		if(pDb)
			pDb->DeleteRecord();
		pSqlEnv->Flush();
	;}
    break;

  case 72:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->CreateSqlUpdate((yyvsp[0].nam));
	;}
    break;

  case 76:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), "", MDB_SQLPARA_OPERATOR_BITNOT);
 ;}
    break;

  case 77:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_EQUAL);
	free((yyvsp[0].sv));
 ;}
    break;

  case 78:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), "", MDB_SQLPARA_OPERATOR_EQUAL);
 ;}
    break;

  case 79:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_ADD);
	free((yyvsp[0].sv));
 ;}
    break;

  case 80:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_SUB);
	free((yyvsp[0].sv));
 ;}
    break;

  case 81:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_MUL);
	free((yyvsp[0].sv));
 ;}
    break;

  case 82:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_DIV);
	free((yyvsp[0].sv));
 ;}
    break;

  case 83:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_MOD);
	free((yyvsp[0].sv));
 ;}
    break;

  case 84:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_BITAND);
	free((yyvsp[0].sv));
 ;}
    break;

  case 85:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_BITOR);
	free((yyvsp[0].sv));
 ;}
    break;

  case 86:

    {
	if(pDb)
		pDb->m_oUpdate.AddSetPara((yyvsp[-2].nam), (yyvsp[0].sv), MDB_SQLPARA_OPERATOR_BITXOR);
	free((yyvsp[0].sv));
 ;}
    break;

  case 89:

    {
		if(pDb)
			pDb->UpdateRecord();
		pSqlEnv->Flush();
	;}
    break;

  case 90:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->TruncateTable((yyvsp[0].nam));
		pSqlEnv->Flush();
	;}
    break;

  case 91:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->PrepareSqlSelect();
	;}
    break;

  case 92:

    {
		if(pDb)
			pDb->CreateSqlSelect((yyvsp[0].nam));
	;}
    break;

  case 97:

    { pDb->OrderBy((yyvsp[0].nam)); ;}
    break;

  case 98:

    { pDb->OrderBy((yyvsp[-1].nam), true); ;}
    break;

  case 99:

    { pDb->OrderBy((yyvsp[-1].nam), false); ;}
    break;

  case 100:

    {
		if(pDb)
			pDb->QueryTable();
		pSqlEnv->Flush();
	;}
    break;

  case 101:

    {
		if(pDb)
			pDb->m_oSelect.AddField((yyvsp[0].nam));
	;}
    break;

  case 102:

    {
		if(pDb)
			pDb->m_oSelect.AddField((yyvsp[0].nam));
	;}
    break;

  case 106:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->DescTable((yyvsp[0].nam));
		pSqlEnv->Flush();
	;}
    break;

  case 107:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->DescTable();
		pSqlEnv->Flush();
	;}
    break;

  case 108:

    {
		pSqlEnv->DescAllDataBase();
		pSqlEnv->Flush();
	;}
    break;

  case 109:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
		{
			pDb->m_oIndex.oIndexName = (yyvsp[-2].nam);
			pDb->m_oIndex.oTableName = (yyvsp[0].nam);
			pDb->m_oIndex.oIndexFieldTable.Clear();
		}
	;}
    break;

  case 111:

    {
		if(pDb)
			pDb->CreateIndex();
		pSqlEnv->Flush();
	;}
    break;

  case 112:

    {
		if(pDb)
			pDb->m_oIndex.nQualifier = MDB_COMMON_INDEX;
	;}
    break;

  case 113:

    {
		if(pDb)
			pDb->m_oIndex.nQualifier = MDB_UNIQUE_INDEX;
	;}
    break;

  case 114:

    {
		if(pDb)
			pDb->m_oIndex.oPrimaryIndex = "";
	;}
    break;

  case 115:

    {
		if(pDb)
			pDb->m_oIndex.oPrimaryIndex = (yyvsp[-1].nam);
	;}
    break;

  case 116:

    {
		if(pDb)
			pDb->m_oIndex.nHashRate = 125;
	;}
    break;

  case 117:

    {
		if(pDb)
			pDb->m_oIndex.nHashRate = (uint32)strtod((yyvsp[-1].sv), NULL);
		free((yyvsp[-1].sv));
	;}
    break;

  case 118:

    {
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_RBTREE_INDEX;
	;}
    break;

  case 119:

    {
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_RBTREE_INDEX;
	;}
    break;

  case 120:

    {
		if(pDb)
			pDb->m_oIndex.nArithmetic = MDB_HASH_INDEX;
	;}
    break;

  case 121:

    {
		if(pDb)
			pDb->m_oIndex.oIndexFieldTable.Insert((uint32)(-1), (yyvsp[0].nam));
	;}
    break;

  case 124:

    {
		int x = 1;
		if(x)
			return 0;
	;}
    break;

  case 125:

    {
		pSqlEnv->Help();
		pSqlEnv->Flush();
	;}
    break;

  case 126:

    {
		pSqlEnv->ExecuteFile((yyvsp[0].sv));
		free((yyvsp[0].sv));
	;}
    break;

  case 127:

    {
		if(!pDb)
			FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("there isn't a database selected"));
		else
			pDb->SetRepeat(atoi((yyvsp[0].sv)));
		pSqlEnv->Flush();
		free((yyvsp[0].sv));
	;}
    break;

  case 144:

    {
		FocpCmdLogEx("SQL", FOCP_LOG_ERROR, ("invalid sql command"));
		pSqlEnv->Flush();
		yyclearin;
		yyerrok;
	;}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */


  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}




void yyerror(const char*)
{
}

FOCP_END();

void CopyName(char* d, const char* s)
{
	FOCP_NAME::CString::StringCopy(d, s, FOCP_NAME::MDB_NAME_MAXLEN);
	d[FOCP_NAME::MDB_NAME_MAXLEN] = '\0';
}

