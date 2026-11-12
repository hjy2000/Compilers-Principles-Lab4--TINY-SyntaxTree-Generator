此实验为华南师范大学编译原理课程实验四：TINY语法扩充及语法树生成器的实现

具体实验要求见仓库中文档

## 功能状态

实验要求的全部语法扩充均已实现，`tests/` 下的 23 个用例全部通过：

| 实验要求 | 状态 |
|---------|------|
| do while 循环 | ✅ |
| for 循环（to / downto，步长 ±1） | ✅ |
| `-=` 减法赋值 | ✅ |
| `%` 求余、`^` 乘方 | ✅ |
| `==`、`>`、`<=`、`>=`、`<>` 比较运算符 | ✅ |
| 赋值 `:=` 改为 `=` | ✅ |
| 正则表达式 `ID := regex`（`\|` `&` `#` `()`） | ✅ |
| 逻辑表达式 `and` / `or` / `not` | ✅ |
| 用户可选是否生成语法树 | ✅（界面新增“生成语法树”复选框） |
| 源程序打开 / 保存 | ✅ |

## TINY+ 语法

```text
program         -> stmt_sequence [END]
stmt_sequence   -> [statement { [;] statement }]

statement       -> if_stmt | repeat_stmt | while_stmt | dowhile_stmt
                 | for_stmt | assign_stmt | regex_assign_stmt
                 | read_stmt | write_stmt

if_stmt         -> IF [(] exp [)] [THEN] stmt_sequence [ELSE stmt_sequence] END
repeat_stmt     -> REPEAT stmt_sequence UNTIL exp
while_stmt      -> WHILE [(] exp [)] DO stmt_sequence ENDWHILE
dowhile_stmt    -> DO stmt_sequence WHILE [(] exp [)] [;]
for_stmt        -> FOR ID = simple_exp (TO|DOWNTO) simple_exp
                     DO stmt_sequence ENDDO
assign_stmt     -> ID = exp
regex_assign    -> ID := regex_exp          { := 专用于正则赋值 }
read_stmt       -> READ ID
write_stmt      -> WRITE exp

exp             -> or_exp
or_exp          -> and_exp { OR and_exp }              (优先级最低)
and_exp         -> not_exp { AND not_exp }
not_exp         -> NOT not_exp | comparison
comparison      -> simple_exp [ (<|>|==|<>|<=|>=) simple_exp ]
simple_exp      -> term { (+|-) term }
term            -> power { (*|/|%) power }
power           -> factor [ ^ power ]                  (右结合，优先级最高)
factor          -> NUM | ID | ( exp )

regex_exp       -> regex_term { | regex_term }
regex_term      -> regex_factor { & regex_factor }
regex_factor    -> regex_primary [ # ]
regex_primary   -> ID | NUM | ( regex_exp )
```

运算符优先级（低 → 高）：`or` < `and` < `not` < 比较运算 < `+ -` < `* / %` < `^` < 一元 `#`；
正则表达式中 `|` < `&` < `#`。

## 本次修复

原实现存在以下缺陷，现已全部修正：

1. **扫描器 `> < -` 三个状态缺少 `break`**，导致状态贯穿执行，`<=` 被错误识别为 `-=`；
2. **运算符必须后跟空格才合法**：`x>0`、`x=1`、`a-b` 这类正常写法会产生未初始化 token；
3. **`for` 语句的 `down` 方向和 `-=`、正则、逻辑表达式未实现**；
4. **乘方 `^` 结合性错误**（原实现按左结合处理，且与 `* /` 同级）；
5. **`printToken` 未覆盖 while/do/endwhile/for/to/downto/enddo**，报错时打印 `Unknown token: N`；
6. **`analyse()` 未重置 `lineno` / `Error` / 扫描器状态**，同一进程内重复分析会出错；
7. **无论分析成功与否都提示“生成成功”**；
8. **`stmt_sequence` 强制要求分号**，实验提供的两个测试文件（语句间省略分号）均无法通过；
9. **程序末尾的 `end` 未被接受**；
10. **文件对话框过滤器为 `*.cpp`**，且打开文件时会多读一行。

## 测试

`tests/` 目录下按功能组织，文件名前缀即编号：

| 文件 | 覆盖内容 |
|------|---------|
| `01_dowhile.TNY` | 实验测试文件 2：do while + `if(...)` 无 then |
| `02_for.TNY` | 实验测试文件 1：for … downto + 运算符无空格 |
| `03_for_to.TNY` | for … to 递增 + 嵌套 for |
| `04_for_downto.TNY` | for … downto 递减 |
| `05_arith.TNY` | `+ - * / % ^` 与优先级、结合性 |
| `06_compare.TNY` | `== <> < > <= >=` |
| `07_subassign.TNY` | `-=` |
| `08_logic.TNY` | `and` / `or` / `not` 优先级 |
| `09_while.TNY` | while … do … endwhile |
| `10_repeat.TNY` | repeat … until |
| `11_regex.TNY` | 正则表达式 `\| & # ()` |
| `12_mixed.TNY` | 综合混合测试 |
| `13_err_lex.TNY` | 词法错误定位 |
| `14_err_syntax.TNY` | 语法错误恢复 |
| `15_nosemi.TNY` | 语句间分号全部省略 |
| `16`–`17` | while 嵌套于 do-while、do-while 嵌套 |
| `18_nospace.TNY` | 全部运算符两侧无空格 |
| `19_ids.TNY` | 标识符含数字、关键字前缀（`notable`/`order`） |
| `20`–`21` | 空程序 |
| `22`–`23` | 无括号 `while exp do` |

## 构建与运行

1. 用 Visual Studio 打开 `TINY/TINY.sln`（工程为 MFC 对话框程序，需安装“使用 C++ 的桌面开发”工作负载）；
2. 选择 `x64 | Debug`（或 `Release`）生成；
3. 运行后：`打开源程序` 载入 `.tny` 文件 → 勾选/取消 `生成语法树` → 点击 `语法分析` → 点击 `查看语法树` 查看结果与错误信息。

命令行说明：`analyse()` 返回 `0` 表示分析成功，非 `0` 表示存在词法/语法错误。

注：实验用的应该是递归下降算法实现的语法树优先级

仅供学习参考，请勿直接抄作业，谢谢！
