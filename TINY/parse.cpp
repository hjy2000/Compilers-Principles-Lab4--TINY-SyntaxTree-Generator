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

/* ---------------- token 流与回溯 ----------------
 * while 既是 do-while 的结尾关键字，又是 while 语句的开头关键字，
 * 因此需要少量向前看能力：解析器缓存已经读入的 token，
 * 在需要区分两种情况时先把 token 流"试探性"解析一遍，再回退。
 */
static TokenTypeInProjece* histToken = NULL;
static char (*histString)[MAXTOKENLEN + 1] = NULL;
static int histCap = 0;     /* 缓存容量 */
static int histCount = 0;   /* 已缓存 token 数 */
static int histPos = 0;     /* 下一个待取 token 的下标 */

static void resetTokenStream(void)
{
	histCount = 0;
	histPos = 0;
}

/* 向缓存追加一个 token，容量不足时扩容 */
static void pushHistory(TokenTypeInProjece tok, const char* lexeme)
{
	if (histCount >= histCap)
	{
		int newCap = (histCap == 0) ? 256 : histCap * 2;
		TokenTypeInProjece* nt = (TokenTypeInProjece*)
			realloc(histToken, newCap * sizeof(TokenTypeInProjece));
		char (*ns)[MAXTOKENLEN + 1] = (char(*)[MAXTOKENLEN + 1])
			realloc(histString, newCap * (MAXTOKENLEN + 1));
		if (nt == NULL || ns == NULL)
		{
			fprintf(listing, "Out of memory in token buffer\n");
			return;
		}
		histToken = nt;
		histString = ns;
		histCap = newCap;
	}
	histToken[histCount] = tok;
	strncpy(histString[histCount], lexeme, MAXTOKENLEN);
	histString[histCount][MAXTOKENLEN] = '\0';
	histCount++;
}

/* 读入下一个 token（优先从缓存中取） */
static void advance(void)
{
	if (histPos < histCount)
	{
		token = histToken[histPos];
		strncpy(tokenString, histString[histPos], MAXTOKENLEN);
		tokenString[MAXTOKENLEN] = '\0';
		histPos++;
	}
	else
	{
		token = getToken();
		pushHistory(token, tokenString);
		histPos = histCount;
	}
}

/* 当前 token 流的标记，与 rewindTo 配合使用 */
static int markTokenPos(void)
{
	return histPos;
}

/* 回退到标记位置，使 token 重新成为标记之前的那个 token */
static void rewindTo(int mark)
{
	if (mark <= 0 || mark > histCount) return;
	histPos = mark;
	token = histToken[mark - 1];
	strncpy(tokenString, histString[mark - 1], MAXTOKENLEN);
	tokenString[MAXTOKENLEN] = '\0';
}

/* function prototypes for recursive calls */
static Node* stmt_sequence(void);
static Node* statement(void);
static Node* if_stmt(void);
static Node* repeat_stmt(void);
static Node* id_stmt(void);
static Node* read_stmt(void);
static Node* write_stmt(void);
static Node* While_stmt(void);
static Node* Dowhile_stmt(void);
static Node* for_stmt(void);

/* 逻辑表达式：exp -> or_exp */
static Node* exp(void);
static Node* or_exp(void);
static Node* and_exp(void);
static Node* not_exp(void);
static Node* comparison(void);
static Node* simple_exp(void);
static Node* term(void);
static Node* power(void);
static Node* factor(void);

/* 正则表达式：regex_exp -> regex_term { | regex_term } */
static Node* regex_exp(void);
static Node* regex_term(void);
static Node* regex_factor(void);
static Node* regex_primary(void);

static void AError(char* message)
{
	fprintf(listing, "\n>>> ");
	fprintf(listing, "A error at line %d: %s", lineno, message);
	Error = TRUE;
}

static void match(TokenTypeInProjece expected)
{
	if (token == expected) advance();
	else {
		AError("unexpected token -> ");
		printToken(token, tokenString);
		fprintf(listing, "      ");
	}
}

/* 判断当前 token 是否会终止一个语句序列 */
static int isStmtTerminator(TokenTypeInProjece tok)
{
	return (tok == ENDFILE) || (tok == END) || (tok == ELSE) ||
		(tok == UNTIL) || (tok == ENDWHILE) || (tok == ENDDO);
}

/* while 既是 do-while 的结尾关键字，又是 while 语句的开头关键字。
 * 这里用纯 token 扫描做一次向前看：
 * 从 while 起扫描到括号深度 0，若紧接着的 token 是 do，
 * 则说明这是一个 while 语句的开头；否则是 do-while 的结尾。 */
static int doDepth = 0; /* 当前处于多少层 do-while 语句体内部 */

static int whileStartsStatement(void)
{
	int mark, depth = 0, result = 0, guard = 0;
	mark = markTokenPos();
	advance(); /* 跳过 while */
	if (token == LPAREN)
	{
		depth = 1;
		advance();
		while (depth > 0 && token != ENDFILE)
		{
			if (token == LPAREN) depth++;
			else if (token == RPAREN) depth--;
			advance();
		}
	}
	else
	{
		/* 无括号：while exp do ...，向后扫描到 do 或语句边界为止 */
		while (token != ENDFILE && guard++ < 4096)
		{
			if (token == DO) break;
			if (token == SEMI || isStmtTerminator(token)) break;
			advance();
		}
	}
	if (token == DO) result = 1; /* 条件表达式之后紧跟 do，说明是 while 语句 */
	rewindTo(mark);
	return result;
}

/* 当前 token 是否终结语句序列（考虑 do-while 内部的 while 歧义） */
static int atSequenceEnd(void)
{
	if (isStmtTerminator(token)) return 1;
	if (token == WHILE && doDepth > 0 && !whileStartsStatement()) return 1;
	return 0;
}

/* stmt_sequence -> statement { [;] statement }
 * 分号在两条语句之间可以省略，以兼容实验提供的测试文件 */
Node* stmt_sequence(void)
{
	Node* t;
	Node* p;
	if (atSequenceEnd()) return NULL; /* 允许空语句序列，如 do ... while(x) 之间没有语句 */
	t = statement();
	p = t;
	while (!atSequenceEnd())
	{
		Node* q;
		if (token == SEMI) match(SEMI);
		if (atSequenceEnd()) break; /* 允许序列末尾出现多余的分号 */
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
	case ID: t = id_stmt(); break;
	case READ: t = read_stmt(); break;
	case WRITE: t = write_stmt(); break;
	case WHILE: t = While_stmt(); break;
	case DO: t = Dowhile_stmt(); break;
	case FOR: t = for_stmt(); break;
	default: AError("unexpected token -> ");
		printToken(token, tokenString);
		advance();
		break;
	} /* end case */
	return t;
}

/* if_stmt -> IF [(] exp [)] [THEN] stmt_sequence [ELSE stmt_sequence] END */
Node* if_stmt(void)
{
	Node* t = newStmtNode(IfK);
	int paren;
	match(IF);
	paren = (token == LPAREN);
	if (paren) match(LPAREN);
	if (t != NULL) t->child[0] = exp();
	if (paren) match(RPAREN);
	if (token == THEN) match(THEN);
	if (t != NULL) t->child[1] = stmt_sequence();
	if (token == ELSE) {
		match(ELSE);
		if (t != NULL) t->child[2] = stmt_sequence();
	}
	match(END);
	return t;
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

/* id_stmt -> ID '=' exp | ID '-=' exp | ID ':=' regex_exp */
Node* id_stmt(void)
{
	Node* t = NULL;
	char* name;
	if (token != ID) {
		AError("unexpected token -> ");
		printToken(token, tokenString);
		return NULL;
	}
	name = copyString(tokenString);
	match(ID);
	if (token == ASSIGN) {
		t = newStmtNode(AssignK);
		if (t != NULL) t->attr.name = name;
		match(ASSIGN);
		if (t != NULL) t->child[0] = exp();
	}
	else if (token == SUBMINUS) {
		t = newStmtNode(SubAssignK);
		if (t != NULL) t->attr.name = name;
		match(SUBMINUS);
		if (t != NULL) t->child[0] = exp();
	}
	else if (token == REGASSIGN) {
		t = newStmtNode(RegexAssignK);
		if (t != NULL) t->attr.name = name;
		match(REGASSIGN);
		if (t != NULL) t->child[0] = regex_exp();
	}
	else {
		AError("unexpected token -> ");
		printToken(token, tokenString);
		t = newStmtNode(AssignK);
		if (t != NULL) t->attr.name = name;
	}
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

/* exp -> or_exp */
Node* exp(void)
{
	return or_exp();
}

/* or_exp -> and_exp { OR and_exp } */
Node* or_exp(void)
{
	Node* t = and_exp();
	while (token == OR)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = and_exp();
		}
		else match(token);
	}
	return t;
}

/* and_exp -> not_exp { AND not_exp } */
Node* and_exp(void)
{
	Node* t = not_exp();
	while (token == AND)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = not_exp();
		}
		else match(token);
	}
	return t;
}

/* not_exp -> NOT not_exp | comparison */
Node* not_exp(void)
{
	if (token == NOT)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->attr.op = NOT;
			match(NOT);
			p->child[0] = not_exp();
			return p;
		}
		match(NOT);
	}
	return comparison();
}

/* comparison -> simple_exp [ relop simple_exp ] */
Node* comparison(void)
{
	Node* t = simple_exp();
	if ((token == LT) || (token == EQ) || (token == GT) ||
		(token == NGT) || (token == NLT) || (token == NEQ)) {
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = simple_exp();
		}
		else match(token);
	}
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
		else match(token);
	}
	return t;
}

/* term -> power { (*|/|%) power } */
Node* term(void)
{
	Node* t = power();
	while ((token == TIMES) || (token == OVER) || (token == REMAINDER))
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = power();
		}
		else match(token);
	}
	return t;
}

/* power -> factor [ ^ power ]   乘方右结合且优先级高于乘除 */
Node* power(void)
{
	Node* t = factor();
	if (token == POWER)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = POWER;
			t = p;
			match(POWER);
			t->child[1] = power();
		}
		else match(POWER);
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
		advance();
		break;
	}
	return t;
}

/* regex_exp -> regex_term { '|' regex_term } */
Node* regex_exp(void)
{
	Node* t = regex_term();
	while (token == RE_OR)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = regex_term();
		}
		else match(token);
	}
	return t;
}

/* regex_term -> regex_factor { '&' regex_factor } */
Node* regex_term(void)
{
	Node* t = regex_factor();
	while (token == RE_CONCAT)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->child[0] = t;
			p->attr.op = token;
			t = p;
			match(token);
			t->child[1] = regex_factor();
		}
		else match(token);
	}
	return t;
}

/* regex_factor -> regex_primary [ '#' ] */
Node* regex_factor(void)
{
	Node* t = regex_primary();
	while (token == RE_CLOSURE)
	{
		Node* p = newExpNode(OpK);
		if (p != NULL) {
			p->attr.op = token;
			p->child[0] = t;
			t = p;
		}
		match(RE_CLOSURE);
	}
	return t;
}

/* regex_primary -> ID | NUM | '(' regex_exp ')' */
Node* regex_primary(void)
{
	Node* t = NULL;
	switch (token) {
	case ID:
		t = newExpNode(IdK);
		if (t != NULL) t->attr.name = copyString(tokenString);
		match(ID);
		break;
	case NUM:
		t = newExpNode(ConstK);
		if (t != NULL) t->attr.val = atoi(tokenString);
		match(NUM);
		break;
	case LPAREN:
		match(LPAREN);
		t = regex_exp();
		match(RPAREN);
		break;
	default:
		AError("unexpected token in regular expression -> ");
		printToken(token, tokenString);
		advance();
		break;
	}
	return t;
}

/* While_stmt -> WHILE exp DO stmt_sequence ENDWHILE */
Node* While_stmt(void)
{
	Node* t = newStmtNode(WhileK);
	int paren;
	match(WHILE);
	paren = (token == LPAREN);
	if (paren) match(LPAREN);
	if (t != NULL) t->child[0] = exp();
	if (paren) match(RPAREN);
	match(DO);
	if (t != NULL) t->child[1] = stmt_sequence();
	match(ENDWHILE);
	return t;
}

/* Dowhile_stmt -> DO stmt_sequence WHILE [(] exp [)] [;] */
Node* Dowhile_stmt(void)
{
	Node* t = newStmtNode(DoWhileK);
	int paren;
	match(DO);
	doDepth++; /* 语句体内部的 while 可能是 do-while 的结尾 */
	if (t != NULL) t->child[0] = stmt_sequence();
	doDepth--;
	match(WHILE);
	paren = (token == LPAREN);
	if (paren) match(LPAREN);
	if (t != NULL) t->child[1] = exp();
	if (paren) match(RPAREN);
	if (token == SEMI) match(SEMI);
	return t;
}

/* for_stmt -> FOR ID ('='|':=') simple_exp (TO|DOWNTO) simple_exp
                DO stmt_sequence ENDDO
 * 循环方向记录在 ForK 结点的 attr.op 中（TO 或 DOWNTO） */
Node* for_stmt(void)
{
	Node* t = newStmtNode(ForK);
	Node* init;
	match(FOR);
	if (t != NULL) {
		t->attr.op = TO; /* 缺省方向，出错时仍可打印语法树 */
		init = newStmtNode(AssignK);
		if (token == ID)
		{
			if (init != NULL) init->attr.name = copyString(tokenString);
		}
		else AError("for statement expects an identifier -> ");
		match(ID);
		if (token == REGASSIGN) match(REGASSIGN);
		else match(ASSIGN);
		if (init != NULL) init->child[0] = simple_exp();
		t->child[0] = init;
	}
	else {
		match(ID);
		if (token == REGASSIGN) match(REGASSIGN);
		else match(ASSIGN);
		simple_exp();
	}
	if (token == TO) {
		if (t != NULL) t->attr.op = TO;
		match(TO);
	}
	else {
		if (t != NULL) t->attr.op = DOWNTO;
		match(DOWNTO);
	}
	if (t != NULL) t->child[1] = simple_exp();
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
	resetTokenStream();
	advance();
	t = stmt_sequence();
	if (token == END) /* 允许整个程序以 end 结束 */
		match(END);
	if (token != ENDFILE)
		AError("Code ends before file\n");
	return t;
}
