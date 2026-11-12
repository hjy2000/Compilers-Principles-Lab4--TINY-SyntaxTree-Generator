/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
#include "stdafx.h"
#include "globals.h"
#include "util.h"
#include "scan.h"
/* states in scanner DFA */
typedef enum
{
	START, INASSIGN, INCOMMENT, INNUM, INID, INGT, INLT, INSUB, INCOLON, DONE
}
StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN + 1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
int linepos = 0; /* current position in LineBuf */
int bufsize = 0; /* current size of buffer string */
int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* resetScanner restores the scanner to its initial state so that
   analyse() may be called more than once in a single session */
void resetScanner(void)
{
	lineBuf[0] = '\0';
	linepos = 0;
	bufsize = 0;
	EOF_flag = FALSE;
	tokenString[0] = '\0';
}

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
static int getNextChar(void)
{
	if (!(linepos < bufsize))
	{
		lineno++;
		if (fgets(lineBuf, BUFLEN - 1, source))
		{
			if (EchoSource) fprintf(listing, "%4d: %s", lineno, lineBuf);
			bufsize = strlen(lineBuf);
			linepos = 0;
			return lineBuf[linepos++];
		}
		else
		{
			EOF_flag = TRUE;
			return EOF;
		}
	}
	else return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
static void ungetNextChar(void)
{
	if (!EOF_flag) linepos--;
}

/* lookup table of reserved words */
static struct
{
	char* str;
	TokenTypeInProjece tok;
} reservedWords[MAXRESERVED] = { {"if",IF},{"then",THEN},{"else",ELSE},{"end",END},
	{"repeat",REPEAT},{"until",UNTIL},{"read",READ},
	{"write",WRITE},
	{"while",WHILE},{"do",DO},{"endwhile",ENDWHILE},{"for",FOR},
	{"to",TO},{"downto",DOWNTO},{"enddo",ENDDO},
	{"and",AND},{"or",OR},{"not",NOT} };

/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
static TokenTypeInProjece reservedLookup(char* s)
{
	int i;
	for (i = 0; i < MAXRESERVED; i++)
		if (!strcmp(s, reservedWords[i].str))
			return reservedWords[i].tok;
	return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the
 * next token in source file
 */
TokenTypeInProjece getToken(void)
{  /* index for storing into tokenString */
	int tokenStringIndex = 0;
	/* holds current token to be returned */
	TokenTypeInProjece currentToken = ERROR_INP;
	/* current state - always begins at START */
	StateType state = START;
	/* flag to indicate save to tokenString */
	int save;
	while (state != DONE)
	{
		int c = getNextChar();
		save = TRUE;
		switch (state)
		{
		case START:
			if (isdigit((unsigned char)c))
				state = INNUM;
			else if (isalpha((unsigned char)c))
				state = INID;
			else if (c == '=')
				state = INASSIGN;
			else if (c == '>')
				state = INGT;
			else if (c == '<')
				state = INLT;
			else if (c == '-')
				state = INSUB;
			else if (c == ':')
				state = INCOLON;
			else if ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))
				save = FALSE;
			else if (c == '{')
			{
				save = FALSE;
				state = INCOMMENT;
			}
			else
			{
				state = DONE;
				switch (c)
				{
				case EOF:
					save = FALSE;
					currentToken = ENDFILE;
					break;
				case '+':
					currentToken = PLUS;
					break;
				case '*':
					currentToken = TIMES;
					break;
				case '/':
					currentToken = OVER;
					break;
				case '%':
					currentToken = REMAINDER;
					break;
				case '^':
					currentToken = POWER;
					break;
				case '(':
					currentToken = LPAREN;
					break;
				case ')':
					currentToken = RPAREN;
					break;
				case ';':
					currentToken = SEMI;
					break;
				case '|':
					currentToken = RE_OR;
					break;
				case '&':
					currentToken = RE_CONCAT;
					break;
				case '#':
					currentToken = RE_CLOSURE;
					break;
				default:
					currentToken = ERROR_INP;
					break;
				}
			}
			break;
		case INGT:
			/* ">" 或 ">=" */
			state = DONE;
			if (c == '=') currentToken = NLT;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = GT;
			}
			break;
		case INLT:
			/* "<" 或 "<=" 或 "<>" */
			state = DONE;
			if (c == '=') currentToken = NGT;
			else if (c == '>') currentToken = NEQ;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = LT;
			}
			break;
		case INSUB:
			/* "-" 或 "-=" */
			state = DONE;
			if (c == '=') currentToken = SUBMINUS;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = MINUS;
			}
			break;
		case INASSIGN:
			/* "=" 或 "==" */
			state = DONE;
			if (c == '=') currentToken = EQ;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = ASSIGN;
			}
			break;
		case INCOLON:
			/* 仅支持 ":="，用于正则表达式赋值 */
			state = DONE;
			if (c == '=') currentToken = REGASSIGN;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = ERROR_INP;
			}
			break;
		case INCOMMENT:
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
			}
			else if (c == '}') state = START;
			break;
		case INNUM:
			if (!isdigit((unsigned char)c))
			{ /* backup in the input */
				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = NUM;
			}
			break;
		case INID:
			if (!isalpha((unsigned char)c) && !isdigit((unsigned char)c))
			{ /* backup in the input */
				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = ID;
			}
			break;
		case DONE:
		default: /* should never happen */
			fprintf(listing, "Scanner Bug: state= %d\n", state);
			state = DONE;
			currentToken = ERROR_INP;
			break;
		}
		if ((save) && (tokenStringIndex < MAXTOKENLEN))
			tokenString[tokenStringIndex++] = (char)c;
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				currentToken = reservedLookup(tokenString);
		}
	}
	if (TraceScan) {
		fprintf(listing, "\t%d: ", lineno);
		printToken(currentToken, tokenString);
	}
	return currentToken;
} /* end getToken */
