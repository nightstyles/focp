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

/* Tokens.  */
#include "MdbApi.hpp"
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




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)

typedef union YYSTYPE {
	bool 	b;
	int 	i;
	char* 	sv;
	char	nam[FOCP_NAME::MDB_NAME_MAXLEN+1];
	double 	d;
} YYSTYPE;
/* Line 1447 of yacc.c.  */

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





