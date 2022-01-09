/****************************************************/
/* File: parse.c                                    */
/* The parser implementation for the TINY compiler  */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
#include "stdafx.h"
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

static TokenTypeInProjece token; /* holds current token */

/* function prototypes for recursive calls */
static Node* stmt_sequence(void);
static Node* statement(void);
static Node* if_stmt(void);
static Node* repeat_stmt(void);
static Node* assign_stmt(void);
static Node* read_stmt(void);
static Node* write_stmt(void);
static Node* exp(void);
static Node* simple_exp(void);
static Node* term(void);
static Node* factor(void);
static Node* While_stmt(void);
static Node* Dowhile_stmt(void);
static Node* for_stmt(void);

static void AError(char* message)
{
	fprintf(listing, "\n>>> ");
	fprintf(listing, "A error at line %d: %s", lineno, message);
	Error = TRUE;
}

static void match(TokenTypeInProjece expected)
{
	if (token == expected) token = getToken();
	else {
		AError("unexpected token -> ");
		printToken(token, tokenString);
		fprintf(listing, "      ");
	}
}

Node* stmt_sequence(void)
{
	Node* t = statement();
	Node* p = t;
	while ((token != ENDFILE) && (token != END) &&
		(token != ELSE) && (token != UNTIL) &&
		(token != WHILE) &&
		(token != ENDWHILE) && (token != ENDDO))
	{
		Node* q;
		match(SEMI);
		q = statement();
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else /* now p cannot be NULL either */
			{
				p->sibling = q;
				p = q;
			}
		}
	}
	return t;
}

Node* statement(void)
{
	Node* t = NULL;
	switch (token) {
	case IF: t = if_stmt(); break;
	case REPEAT: t = repeat_stmt(); break;
	case ID: t = assign_stmt(); break;
	case READ: t = read_stmt(); break;
	case WRITE: t = write_stmt(); break;
	case WHILE: t = While_stmt(); break;
	case DO: t = Dowhile_stmt(); break;
	case FOR: t = for_stmt(); break;
	default: AError("unexpected token -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	} /* end case */
	return t;
}

Node* if_stmt(void)
{
	
	Node * t = newStmtNode(IfK);
	match(IF);
	if (t != NULL) t->child[0] = exp();
	match(THEN);
	if (t != NULL) t->child[1] = stmt_sequence();
	if (token == ELSE) {
		match(ELSE);
		if (t != NULL) t->child[2] = stmt_sequence();
	}
	match(END);
	return t;

	/*
	Node* t = newStmtNode(IfK);
	match(IF);
	if (t != NULL)
	{
		match(LPAREN);
		t->child[0] = exp();
		match(RPAREN);
	}
	if (t != NULL) t->child[1] = stmt_sequence();
	if (token == ELSE) {
		match(ELSE);
		if (t != NULL) t->child[2] = stmt_sequence();
	}
	return t;
	*/
}

Node* repeat_stmt(void)
{
	Node* t = newStmtNode(RepeatK);
	match(REPEAT);
	if (t != NULL) t->child[0] = stmt_sequence();
	match(UNTIL);
	if (t != NULL) t->child[1] = exp();
	return t;
}

Node* assign_stmt(void)
{
	Node* t = newStmtNode(AssignK);
	if ((t != NULL) && (token == ID))
		t->attr.name = copyString(tokenString);
	match(ID);
	match(ASSIGN);
	if (t != NULL) t->child[0] = exp();
	return t;
}

Node* read_stmt(void)
{
	Node* t = newStmtNode(ReadK);
	match(READ);
	if ((t != NULL) && (token == ID))
		t->attr.name = copyString(tokenString);
	match(ID);
	return t;
}

Node* write_stmt(void)
{
	Node* t = newStmtNode(WriteK);
	match(WRITE);
	if (t != NULL) t->child[0] = exp();
	return t;
}

Node* exp(void)
{
	Node* t = simple_exp();
	if ((token == LT) || (token == EQ) || (token == GT) || (token== NGT) || (token== NLT) || (token== NEQ)  ){
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
		}
		match(token);
		if (t != NULL)
			t->child[1] = simple_exp();
	}
	/*
	else if (token == SUBMINUS)
	{
		assign_stmt();
	}
	*/
	return t;
}

Node* simple_exp(void)
{
	Node* t = term();
	while ((token == PLUS) || (token == MINUS))
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = term();
		}
	}
	return t;
}

Node* term(void)
{
	Node* t = factor();
	while ((token == TIMES) || (token == OVER) || (token == REMAINDER)||(token == POWER))//REMAINDER是求余,POWER是乘方
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			p->child[1] = factor();
		}
	}
	return t;
}

Node* factor(void)
{
	Node* t = NULL;
	switch (token) {
	case NUM:
		t = newExpNode(ConstK);
		if ((t != NULL) && (token == NUM))
			t->attr.val = atoi(tokenString);
		match(NUM);
		break;
	case ID:
		t = newExpNode(IdK);
		if ((t != NULL) && (token == ID))
			t->attr.name = copyString(tokenString);
		match(ID);
		break;
	case LPAREN:
		match(LPAREN);
		t = exp();
		match(RPAREN);
		break;
	default:
		AError("unexpected token -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	}
	return t;
}

//扩充While文法
Node* While_stmt(void)
{
	Node* t = newStmtNode(WhileK);
	match(WHILE);
	if (t != NULL) t->child[0] = exp();
	match(DO);
	if (t != NULL) t->child[1] = stmt_sequence();
	match(ENDWHILE);
	return t;
}

//扩充Dowhile文法
Node* Dowhile_stmt(void)
{
	Node* t = newStmtNode(DoWhileK);
	match(DO);
	if (t != NULL) t->child[0] = stmt_sequence();
	match(WHILE);
	match(LPAREN);
	if (t != NULL) t->child[1] = exp();
	match(RPAREN);
	return t;
}

//扩充for文法
Node* for_stmt(void)
{
	Node* t = newStmtNode(ForK);
	match(FOR);
	if (t != NULL) t->child[0] = assign_stmt();
	if (token == TO) {
		match(TO);
		if (t != NULL) t->child[1] = simple_exp();
	}
	else {
		match(DOWNTO);
		if (t != NULL) t->child[1] = simple_exp();
	}
	match(DO);
	if (t != NULL) t->child[2] = stmt_sequence();
	match(ENDDO);
	return t;
}

/****************************************/
/* the primary function of the parser   */
/****************************************/
/* Function parse returns the newly
 * constructed A tree
 */

Node* parse(void)
{
	Node* t;
	token = getToken();
	t = stmt_sequence();
	if (token != ENDFILE)
		AError("Code ends before file\n");
	return t;
}
