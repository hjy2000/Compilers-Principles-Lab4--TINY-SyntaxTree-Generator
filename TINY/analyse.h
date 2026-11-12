#pragma once
#include <string>
using namespace std;

/* 对源文件 argv 进行词法与语法分析，结果写入 str。
 * genTree 为 true 时在输出中打印语法树，为 false 时只输出分析结论。
 * 返回 0 表示分析成功，非 0 表示失败。 */
extern int analyse(string argv, string str, bool genTree = true);
