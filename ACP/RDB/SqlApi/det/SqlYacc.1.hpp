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
     TEMPORARY = 293,
     MODIFY = 294,
     DROP = 295,
     INDEX = 296,
     ON = 297,
     BY = 298,
     INSERT = 299,
     INTO = 300,
     VALUES = 301,
     COMMENT = 302,
     USE = 303,
     TDELETE = 304,
     FROM = 305,
     WHERE = 306,
     AND = 307,
     UPDATE = 308,
     SET = 309,
     VCOUNT = 310,
     TRUNCATE = 311,
     SELECT = 312,
     DESC = 313,
     QUIT = 314,
     HELP = 315,
     BATCH = 316,
     GE = 317,
     LE = 318,
     EQ = 319,
     MT = 320,
     LT = 321,
     TSQLERROR = 322
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
#define TEMPORARY 293
#define MODIFY 294
#define DROP 295
#define INDEX 296
#define ON 297
#define BY 298
#define INSERT 299
#define INTO 300
#define VALUES 301
#define COMMENT 302
#define USE 303
#define TDELETE 304
#define FROM 305
#define WHERE 306
#define AND 307
#define UPDATE 308
#define SET 309
#define VCOUNT 310
#define TRUNCATE 311
#define SELECT 312
#define DESC 313
#define QUIT 314
#define HELP 315
#define BATCH 316
#define GE 317
#define LE 318
#define EQ 319
#define MT 320
#define LT 321
#define TSQLERROR 322




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





