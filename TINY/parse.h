#pragma once
/****************************************************/
/* File: parse.h                                    */
/* The parser interface for the TINY compiler       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _PARSE_H_
#define _PARSE_H_
#include "stdafx.h"
#include "globals.h"

/* Function parse returns the newly
 * constructed A tree
 */
Node * parse(void);

#endif
