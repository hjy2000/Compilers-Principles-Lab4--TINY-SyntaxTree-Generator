#pragma once
#pragma once
/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 18

typedef enum
/* book-keeping tokens */
{
	ENDFILE, ERROR_INP,
	/* reserved words */
	IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE, WHILE, DO, ENDWHILE, FOR, TO, DOWNTO, ENDDO,
	AND, OR, NOT,
	/* multicharacter tokens */
	ID, NUM,
	/* special symbols */
	/* 注意：NGT 表示 <=（not greater than），NLT 表示 >=（not less than） */
	ASSIGN, EQ, LT, GT, PLUS, MINUS, TIMES, OVER, LPAREN, RPAREN, SEMI, REMAINDER, NGT, NLT, NEQ, POWER, SUBMINUS,
	/* 正则表达式专用符号 */
	REGASSIGN, RE_OR, RE_CONCAT, RE_CLOSURE
} TokenTypeInProjece;

extern FILE* source; /* source code text file */
extern FILE* listing; /* listing output text file */
extern FILE* code; /* code text file for TM simulator */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   A tree for parsing ************/
/**************************************************/

typedef enum { StmtK, ExpK } NodeKind;
/* SubAssignK 表示 -= 语句，RegexAssignK 表示 ID := 正则表达式 语句 */
typedef enum { IfK, RepeatK, AssignK, ReadK, WriteK, WhileK, DoWhileK, ForK, SubAssignK, RegexAssignK } StmtKind;
/* RegexK 表示正则表达式结点，其 attr.op 记录 | & # 之外的连接关系由树结构体现 */
typedef enum { OpK, ConstK, IdK } ExpKind;

/* ExpType is used for type checking */
typedef enum { Void, Integer, Boolean } ExpType;

#define MAXCHILDREN 3

typedef struct Node
{
	struct Node* child[MAXCHILDREN];
	struct Node* sibling;
	int lineno;
	NodeKind nodekind;
	union { StmtKind stmt; ExpKind exp; } kind;
	union {
		TokenTypeInProjece op;
		int val;
		char* name;
	} attr;
	ExpType type; /* for type checking of exps */
} Node;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the A tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;
#endif
