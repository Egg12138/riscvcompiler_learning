#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//
// 终结符分析，词法分析
//

// 为每个终结符都设置种类来表示
typedef enum {
  TK_PUNCT, // 操作符如： + -
  TK_NUM,   // 数字
  TK_EOF,   // 文件终止符，即文件的最后
} TokenKind;

// 终结符结构体
typedef struct token Token;
struct token {
  TokenKind Kind; // 种类
  Token *Next;    // 指向下一终结符
  int Val;        // 值
  char *Loc;      // 在解析的字符串内的位置
  int Len;        // 长度
};

// 输入的字符串
static char *InputString;

// 输出错误信息
// static文件内可以访问的函数
// Fmt为传入的字符串， ... 为可变参数，表示Fmt后面所有的参数
static void error(char *Fmt, ...) {
  // 定义一个va_list变量
  va_list VA;
  // VA获取Fmt后面的所有参数
  va_start(VA, Fmt);
  // vfprintf可以输出va_list类型的参数
  vfprintf(stderr, Fmt, VA);
  // 在结尾加上一个换行符
  fprintf(stderr, "\n");
  // 清除VA
  va_end(VA);
  // 终止程序
  exit(1);
}

// 输出错误出现的位置
static void verrorAt(char *Loc, char *Fmt, va_list VA) {
  // 先输出源信息
  fprintf(stderr, "%s\n", InputString);

  // 输出出错信息
  // 计算出错的位置，Loc是出错位置的指针，CurrentInput是当前输入的首地址
  int Pos = Loc - InputString;
  // 将字符串补齐为Pos位，因为是空字符串，所以填充Pos个空格。
  fprintf(stderr, "%*s", Pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, Fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
}

// 字符解析出错，并退出程序
static void errorAt(char *Loc, char *Fmt, ...) {
  va_list VA;
  va_start(VA, Fmt);
  verrorAt(Loc, Fmt, VA);
  exit(1);
}

// Tok解析出错，并退出程序
static void errorTok(Token *token,char *Fmt, ...) {
  va_list VA;
  va_start(VA, Fmt);
  verrorAt(token->Loc, Fmt, VA);
  exit(1);
}

// 判断Tok的值是否等于指定值，没有用char，是为了后续拓展
static bool equal(Token *token,char *Str) {
  // 比较字符串LHS（左部），RHS（右部）的前N位，S2的长度应大于等于N.
  // 比较按照字典序，LHS<RHS回负值，LHS=RHS返回0，LHS>RHS返回正值
  // 同时确保，此处的Op位数=N
  return memcmp(token->Loc, Str, token->Len) == 0 && Str[token->Len] == '\0';
}

// 跳过指定的Str
static Token *skip(Token *token,char *Str) {
  if (!equal(token,Str))
    errorTok(token,"expect '%s'", Str);
  return token->Next;
}

// 返回TK_NUM的值
static int getNumber(Token *token) {
  if (token->Kind != TK_NUM)
    errorTok(token,"expect a number");
  return token->Val;
}

// 生成新的Token
static Token *newToken(TokenKind Kind, char *Start, char *End) {
  // 分配1个Token的内存空间
  Token *token = calloc(1, sizeof(Token));
  token->Kind = Kind;
  token->Loc = Start;
  token->Len = End - Start;
  return token;
}

// 判断Str是否以SubStr开头
static bool startsWith(char *Str, char *SubStr) {
  // 比较LHS和RHS的N个字符是否相等
  return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

// 读取操作符
static int readPunct(char *Ptr) {
  // 判断2字节的操作符
  if (startsWith(Ptr, "==") || startsWith(Ptr, "!=") || startsWith(Ptr, "<=") ||
      startsWith(Ptr, ">="))
    return 2;

  // 判断1字节的操作符
  return ispunct(*Ptr) ? 1 : 0;
}

// 终结符解析
static Token *tokenize() {
  char *P = InputString;
  Token Head = {};
  Token *Cur = &Head;

  while (*P) {
    // 跳过所有空白符如：空格、回车
    if (isspace(*P)) {
      ++P;
      continue;
    }

    // 解析数字
    if (isdigit(*P)) {
      // 初始化，类似于C++的构造函数
      // 我们不使用Head来存储信息，仅用来表示链表入口，这样每次都是存储在Cur->Next
      // 否则下述操作将使第一个Token的地址不在Head中。
      Cur->Next = newToken(TK_NUM, P, P);
      // 指针前进
      Cur = Cur->Next;
      const char *OldPtr = P;
      Cur->Val = strtoul(P, &P, 10);
      Cur->Len = P - OldPtr;
      continue;
    }

    // 解析操作符
    int PunctLen = readPunct(P);
    if (PunctLen) {
      Cur->Next = newToken(TK_PUNCT, P, P + PunctLen);
      Cur = Cur->Next;
      // 指针前进Punct的长度位
      P += PunctLen;
      continue;
    }

    // 处理无法识别的字符
    errorAt(P, "invalid token");
  }

  // 解析结束，增加一个EOF，表示终止符。
  Cur->Next = newToken(TK_EOF, P, P);
  // Head无内容，所以直接返回Next
  return Head.Next;
}

//
// 生成AST（抽象语法树），语法解析
//

// AST的节点种类
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // 负号-
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, // 整形
} NodeKind;

// AST中二叉树节点
typedef struct Node Node;
struct Node {
  NodeKind Kind; // 节点种类
  Node *LHS;     // 左部，left-hand side
  Node *RHS;     // 右部，right-hand side
  int Val;       // 存储ND_NUM种类的值
};

// 新建一个节点
static Node *newNode(NodeKind Kind) {
  Node *node = calloc(1, sizeof(Node));
  node->Kind = Kind;
  return node;
}

// 新建一个单叉树
static Node *newUnary(NodeKind Kind, Node *Expr) {
  Node *node = newNode(Kind);
  node->LHS = Expr;
  return node;
}

// 新建一个二叉树节点
static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS) {
  Node *node = newNode(Kind);
  node->LHS = LHS;
  node->RHS = RHS;
  return node;
}

// 新建一个数字节点
static Node *newNum(int Val) {
  Node *node = newNode(ND_NUM);
  node->Val = Val;
  return node;
}

// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num
static Node *expr(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

// 解析表达式
// expr = equality
static Node *expr(Token **rest, Token *token) { return equality(rest, token); }

// 解析相等性
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *token) {
  // relational
  Node *node = relational(&token,token);

  // ("==" relational | "!=" relational)*
  while (true) {
    // "==" relational
    if (equal(token,"==")) {
      node = newBinary(ND_EQ, node, relational(&token,token->Next));
      continue;
    }

    // "!=" relational
    if (equal(token,"!=")) {
      node = newBinary(ND_NE, node, relational(&token,token->Next));
      continue;
    }

    *rest = token;
    return node;
  }
}

// 解析比较关系
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *token) {
  // add
  Node *node = add(&token,token);

  // ("<" add | "<=" add | ">" add | ">=" add)*
  while (true) {
    // "<" add
    if (equal(token,"<")) {
      node = newBinary(ND_LT, node, add(&token,token->Next));
      continue;
    }

    // "<=" add
    if (equal(token,"<=")) {
      node = newBinary(ND_LE, node, add(&token,token->Next));
      continue;
    }

    // ">" add
    // X>Y等价于Y<X
    if (equal(token,">")) {
      node = newBinary(ND_LT, add(&token,token->Next), node);
      continue;
    }

    // ">=" add
    // X>=Y等价于Y<=X
    if (equal(token,">=")) {
      node = newBinary(ND_LE, add(&token,token->Next), node);
      continue;
    }

    *rest = token;
    return node;
  }
}

// 解析加减
// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {
  // mul
  Node *node = mul(&token,token);

  // ("+" mul | "-" mul)*
  while (true) {
    // "+" mul
    if (equal(token,"+")) {
      node = newBinary(ND_ADD, node, mul(&token,token->Next));
      continue;
    }

    // "-" mul
    if (equal(token,"-")) {
      node = newBinary(ND_SUB, node, mul(&token,token->Next));
      continue;
    }

    *rest = token;
    return node;
  }
}

// 解析乘除
// mul = unary ("*" unary | "/" unary)*
static Node *mul(Token **rest, Token *token) {
  // unary
  Node *node = unary(&token,token);

  // ("*" unary | "/" unary)*
  while (true) {
    // "*" unary
    if (equal(token,"*")) {
      node = newBinary(ND_MUL, node, unary(&token,token->Next));
      continue;
    }

    // "/" unary
    if (equal(token,"/")) {
      node = newBinary(ND_DIV, node, unary(&token,token->Next));
      continue;
    }

    *rest = token;
    return node;
  }
}

// 解析一元运算
// unary = ("+" | "-") unary | primary
static Node *unary(Token **rest, Token *token) {
  // "+" unary
  if (equal(token,"+"))
    return unary(rest, token->Next);

  // "-" unary
  if (equal(token,"-"))
    return newUnary(ND_NEG, unary(rest, token->Next));

  // primary
  return primary(rest, token);
}

// 解析括号、数字
// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *token) {
  // "(" expr ")"
  if (equal(token,"(")) {
    Node *node = expr(&token,token->Next);
    *rest = skip(token,")");
    return node;
  }

  // num
  if (token->Kind == TK_NUM) {
    Node *node = newNum(token->Val);
    *rest = token->Next;
    return node;
  }

  errorTok(token,"expected an expression");
  return NULL;
}

//
// 语义分析与代码生成
//

// 记录栈深度
static int Depth;

// 压栈，将结果临时压入栈中备用
// sp为栈指针，栈反向向下增长，64位下，8个字节为一个单位，所以sp-8
// 当前栈指针的地址就是sp，将a0的值压入栈
// 不使用寄存器存储的原因是因为需要存储的值的数量是变化的。
static void push(void) {
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

// 弹栈，将sp指向的地址的值，弹出到a1
static void pop(char *Reg) {
  printf("  ld %s, 0(sp)\n", Reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

// 生成表达式
static void gen_expr(Node *node) {
  // 生成各个根节点
  switch (node->Kind) {
  // 加载数字到a0
  case ND_NUM:
    printf("  li a0, %d\n", node->Val);
    return;
  // 对寄存器取反
  case ND_NEG:
    gen_expr(node->LHS);
    // neg a0, a0是sub a0, x0, a0的别名, 即a0=0-a0
    printf("  neg a0, a0\n");
    return;
  default:
    break;
  }

  // 递归到最右节点
  gen_expr(node->RHS);
  // 将结果压入栈
  push();
  // 递归到左节点
  gen_expr(node->LHS);
  // 将结果弹栈到a1
  pop("a1");

  // 生成各个二叉树节点
  switch (node->Kind) {
  case ND_ADD: // + a0=a0+a1
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB: // - a0=a0-a1
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL: // * a0=a0*a1
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV: // / a0=a0/a1
    printf("  div a0, a0, a1\n");
    return;
  case ND_EQ:
  case ND_NE:
    // a0=a0^a1，异或指令
    printf("  xor a0, a0, a1\n");

    if (node->Kind == ND_EQ)
      // a0==a1
      // a0=a0^a1, sltiu a0, a0, 1
      // 等于0则置1
      printf("  seqz a0, a0\n");
    else
      // a0!=a1
      // a0=a0^a1, sltu a0, x0, a0
      // 不等于0则置1
      printf("  snez a0, a0\n");
    return;
  case ND_LT:
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    // a0<=a1等价于
    // a0=a1<a0, a0=a1^1
    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

int main(int argc, char **argv) {
  // 判断传入程序的参数是否为2个，argv[0]为程序名称，argv[1]为传入的第一个参数
  if (argc != 2) {
    // 异常处理，提示参数数量不对。
    // fprintf，格式化文件输出，往文件内写入字符串
    // stderr，异常文件（Linux一切皆文件），用于往屏幕显示异常信息
    // %s，字符串
    error("%s: invalid number of arguments", argv[0]);
  }

  // 解析argv[1]，生成终结符流
  InputString = argv[1];
  Token *token = tokenize();

  // 解析终结符流
  Node *node = expr(&token,token);

  if (token->Kind != TK_EOF)
    errorTok(token,"extra token");

  // 声明一个全局main段，同时也是程序入口段
  printf("  .globl main\n");
  // main段标签
  printf("main:\n");

  // 遍历AST树生成汇编
  gen_expr(node);

  // ret为jalr x0, x1, 0别名指令，用于返回子程序
  // 返回的为a0的值
  printf("  ret\n");

  // 如果栈未清空则报错
  assert(Depth == 0);

  return 0;
}