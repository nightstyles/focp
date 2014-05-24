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
#include "RdbApi.hpp"
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
     TSTRING = 269,
     CSTRING = 270,
     VSTRING = 271,
     VCSTRING = 272,
     RAW = 273,
     VRAW = 274,
     TSIZE = 275,
     RECSIZE = 276,
     DEFAULT = 277,
     BACKUP = 278,
     NOT = 279,
     TNULL = 280,
     TNUMBER = 281,
     XSTRING = 282,
     UNIQUE = 283,
     FOREIGN = 284,
     RBTREE = 285,
     NTREE = 286,
     HASH = 287,
     REPEAT = 288,
     CREATE = 289,
     DEVICE = 290,
     DATABASE = 291,
     TABLE = 292,
     MODIFY = 293,
     DROP = 294,
     INDEX = 295,
     ON = 296,
     BY = 297,
     INSERT = 298,
     INTO = 299,
     VALUES = 300,
     COMMENT = 301,
     USE = 302,
     TDELETE = 303,
     FROM = 304,
     WHERE = 305,
     AND = 306,
     UPDATE = 307,
     SET = 308,
     VCOUNT = 309,
     TRUNCATE = 310,
     SELECT = 311,
     DESC = 312,
     QUIT = 313,
     HELP = 314,
     BATCH = 315,
     GE = 316,
     LE = 317,
     EQ = 318,
     MT = 319,
     LT = 320,
     TSQLERROR = 321
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
#define TSTRING 269
#define CSTRING 270
#define VSTRING 271
#define VCSTRING 272
#define RAW 273
#define VRAW 274
#define TSIZE 275
#define RECSIZE 276
#define DEFAULT 277
#define BACKUP 278
#define NOT 279
#define TNULL 280
#define TNUMBER 281
#define XSTRING 282
#define UNIQUE 283
#define FOREIGN 284
#define RBTREE 285
#define NTREE 286
#define HASH 287
#define REPEAT 288
#define CREATE 289
#define DEVICE 290
#define DATABASE 291
#define TABLE 292
#define MODIFY 293
#define DROP 294
#define INDEX 295
#define ON 296
#define BY 297
#define INSERT 298
#define INTO 299
#define VALUES 300
#define COMMENT 301
#define USE 302
#define TDELETE 303
#define FROM 304
#define WHERE 305
#define AND 306
#define UPDATE 307
#define SET 308
#define VCOUNT 309
#define TRUNCATE 310
#define SELECT 311
#define DESC 312
#define QUIT 313
#define HELP 314
#define BATCH 315
#define GE 316
#define LE 317
#define EQ 318
#define MT 319
#define LT 320
#define TSQLERROR 321




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)

typedef union YYSTYPE {
	bool 	b;
	int 	i;
	char* 	sv;
	char	nam[FOCP_NAME::RDB_NAME_MAXLEN+1];
	double 	d;
} YYSTYPE;
/* Line 1447 of yacc.c.  */

# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





