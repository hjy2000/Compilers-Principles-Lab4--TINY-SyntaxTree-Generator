/****************************************************/
/* File: analyse.cpp                                */
/* Main program for TINY compiler                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "stdafx.h"
#include "globals.h"
#include "util.h"
#include "parse.h"
#include "scan.h"
#include "analyse.h"
#include <string>
using namespace std;
#define _CRT_SECURE_NO_WARNINGS

/* allocate global variables */
int lineno = 0;
FILE* source = NULL;
FILE* listing = NULL;
FILE* code = NULL;

/* allocate and set tracing flags */
int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = TRUE;
int TraceAnalyze = FALSE;
int TraceCode = FALSE;
int Error = FALSE;

/* analyse 对源文件 argv 进行词法与语法分析，
 * 结果（含错误信息与语法树）写入 str；
 * genTree 为 FALSE 时只报告分析结果，不生成语法树。
 * 返回 0 表示分析成功，非 0 表示存在词法/语法错误或文件错误。 */
int analyse(string argv, string str, bool genTree)
{
	Node* ATree;
	char pgm[512]; /* source code file name */
	if (argv.size() >= sizeof(pgm))
	{
		fprintf(stderr, "File name too long\n");
		return 1;
	}
	strcpy(pgm, argv.c_str());

	/* 仅在文件名（而非目录名）没有扩展名时补上 .tny */
	{
		const char* base = pgm;
		const char* p;
		for (p = pgm; *p; p++)
			if (*p == '\\' || *p == '/') base = p + 1;
		if (strchr(base, '.') == NULL && strlen(pgm) + 4 < sizeof(pgm))
			strcat(pgm, ".tny");
	}
	source = fopen(pgm, "r");
	if (source == NULL)
	{
		listing = fopen(str.c_str(), "w");
		if (listing != NULL)
		{
			fprintf(listing, "\nTINY Analyse Failed!\n");
			fprintf(listing, "File %s not found\n", pgm);
			fclose(listing);
			listing = NULL;
		}
		fprintf(stderr, "File %s not found\n", pgm);
		return 1;
	}
	listing = fopen(str.c_str(), "w");
	if (listing == NULL)
	{
		fprintf(stderr, "Cannot open listing file %s\n", str.c_str());
		fclose(source);
		source = NULL;
		return 1;
	}

	/* 每次分析前重置全局状态，保证同一进程内可重复分析 */
	lineno = 0;
	Error = FALSE;
	resetScanner();

	ATree = parse();

	if (genTree)
	{
		fprintf(listing, "\nTINY syntax tree:\n");
		printTree(ATree);
	}

	fprintf(listing, "\n%s\n", Error ? "TINY Analyse Failed!" : "TINY Analyse Succeeded!");

	fclose(listing);
	fclose(source);
	listing = NULL;
	source = NULL;

	return Error ? 1 : 0;
}
