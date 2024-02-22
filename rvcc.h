#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/** type defenitions */
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
  TokenKind kind; // 种类
  Token *next;    // 指向下一终结符
  int val;        // 值
  char *loc;      // 在解析的字符串内的位置
  int len;        // 长度
};


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
  NodeKind kind; // 节点种类
  Node *LHS;     // 左部，left-hand side
  Node *RHS;     // 右部，right-hand side
  int val;       // 存储ND_NUM种类的值
};


// tokenize.c
Token* tokenize(char *intpustring);
bool equal(Token *token, char *str);
Token *skip(Token *token, char *pattern);

// parse.c
Node *parse(Token *token);


// code_gen.c

void code_gen(Node *node);


// error handling
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_token(Token *token,char *fmt, ...);