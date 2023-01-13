#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define NUM_BASE 10
/* 这里我们统一用load double : ld rd, offset(rs1) 所以是8 bytes = 64bits*/
#define SP_PUSH_SHIFT -8
#define SP_POP_SHIFT  8

typedef enum {
    TK_PUNCT,
    TK_DIGIT,
    TK_EOF,
} TokenKind;

typedef enum {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_DIGIT,
} NodeKind;

typedef struct Node ASTNode;
struct Node {
    NodeKind kind;
    ASTNode *lhs;
    ASTNode *rhs;
    int val;
};
typedef ASTNode* AST;

typedef struct Token Token;
struct Token {
    TokenKind kind;
    Token *next;
    int val;
    char *loc;
    int len;
};
static char *input_string;
static void error(char *fmt, ...) {
  va_list VA;
  va_start(VA, fmt);
  vfprintf(stderr, fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
  //exit(1);
}

// 位置显示只能正常支持ASCII字符。
// TODO: 可以考虑支持Unicode
static void verror_at(char *loc, char *fmt, va_list VA) {
    fprintf(stderr, "%s\n", input_string); 
    int position = loc - input_string;  
    fprintf(stderr, "%*s", position, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
}

static void error_at(char *loc, char *fmt, ...) {
    va_list VArgs;
    va_start(VArgs, fmt);
    verror_at(loc, fmt, VArgs);
    exit(1);
}

static void error_at_token(Token *token, char *fmt, ...) {
    va_list VArgs;
    va_start(VArgs, fmt);
    verror_at(token->loc, fmt, VArgs);
    exit(1);
}

Token *newToken(TokenKind kind, char *token_head, char *token_tail) {
    // init内存为0
    Token *token = (Token *)calloc(1, sizeof(Token));
    token->kind = kind;
    token->loc = token_head;
    token->len = token_tail - token_head;
    return token;
} 


// 将输入str整体作为一个string,传到tokenize里
static Token *tokenize(){
    Token head = {};
    Token *cur = &head;
    char *p = input_string;

    while (*p) {
        if (isspace(*p)) {
            /* skip */
            p++;
            continue;
        }

        if (isdigit(*p)) {
            cur->next = newToken(TK_DIGIT, p, p);
            cur = cur->next;
            const char *prev_ptr = p;
            // NOTICE: strtol函数才是捕捉str的关键，*p++这样只是得到每一个char. 
            // strtol是把传入指针从头开始的连续数字进行解析，如果开头就不是数字那就不解析了
            // strtol这里将p进行了移动。
            cur->val = strtol(p, &p, NUM_BASE);//base = 10 
            cur->len = p - prev_ptr; 
            continue;
        }
        if (ispunct(*p)){
            cur->next = newToken(TK_PUNCT, p, p+1);
            cur = cur->next;
            p++;
            continue; 
        }

        error_at(p, "invalid token: %c", *p);
    } 
    // add an EOF
    cur->next = newToken(TK_EOF, p, p);
    return head.next;
}

int numberFrom(Token *token) {
   if (token->kind != TK_DIGIT) {
      error("Got [%d], expect a number", token->val); 
   }
   return token->val;
}


static bool eq(Token *token, char *str) {
    return memcmp(token->loc,str, token->len) == 0 && str[token->len] == '\0';
}
static Token *skip(Token *token, char *str) {
  if (!eq(token, str)) {
     error_at_token(token, "expect '%s'", str); 
  }
  return token->next;
}

static ASTNode *newNode(NodeKind kind) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->kind = kind;
    return node;
}

static ASTNode *newBTNode(NodeKind kind, ASTNode *lhs, ASTNode *rhs) {
    ASTNode *node = newNode(kind); 
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static ASTNode *newDigitNode(int number) {
    ASTNode *node = newNode(ND_DIGIT);
    node->val = number;
    return node;
}

static ASTNode *expr(Token **rest, Token *token);
static ASTNode *mul(Token **rest, Token *token);
static ASTNode *primary(Token **rest, Token *token);

static ASTNode *expr(Token **rest, Token *token) {
    ASTNode *node = mul(&token, token);
    while (true) {
        if (eq(token, "+")) {
            node = newBTNode(ND_ADD, node, mul(&token, token->next));
            continue;
        }

        if (eq(token, "-")) {
            node = newBTNode(ND_SUB, node, mul(&token, token->next));
            continue;
        }
        // 是+/-就会分割exprssion
        *rest = token;
        return node;
    }
}

static ASTNode *mul(Token **rest, Token *token) {
    ASTNode *node = primary(&token, token);
    while (true) {
        if (eq(token, "*")) {
            node = newBTNode(ND_MUL, node, primary(&token, token->next));
            continue;
        }
        if (eq(token, "/")) {
            node = newBTNode(ND_DIV, node, primary(&token, token->next));
            continue;
        }
        *rest = token;
        return node;
    } 
}

static ASTNode *primary(Token **rest, Token *token) {
    ASTNode *node;
    if (eq(token, "(")) {
        node = expr(&token, token->next);
        *rest = skip(token, ")"); // the next token
        return node;
    } 

    if (token->kind == TK_DIGIT) {
        node = newDigitNode(token->val);
        *rest = token->next;
        return node;
    }

    error_at_token(token, "expected an expression");
    return NULL;
}

static int depth = 0;
// sd rs2, offset(rs1): 将x[rs2]中的8bytes->x[rs1]+extend_offset
static void push_to_stack(void) {
    printf(" addi sp, sp, %d\n", SP_PUSH_SHIFT);
    printf(" sd a0, 0(sp)\n");
    depth++;
}

static void pop_to(char *target_rs) {
    printf(" ld %s, 0(sp)\n", target_rs); // 将sp + offset:0处的双字(2*4bytes)写到target_rs中
    printf(" addi sp, sp, %d\n", SP_POP_SHIFT);                      // 然后再移动栈指针
    depth--; 
} 

// TODO: 这种递归在这种小算式中效率没有什么问题。
// 递归地生成，右枝优先. 最顶层地传入为AST ast根节点. LHS->a1, RHS->a0
static void gen_expr(ASTNode *node) {
    if (node->kind == ND_DIGIT) {
        printf(" li a0, %d\n", node->val);
        return;
    } 
    // if puncts:
    gen_expr(node->rhs);
    push_to_stack();
    gen_expr(node->lhs);
    pop_to("a1");

    switch (node->kind)
    {
    case ND_ADD:
        printf(" add a0, a0, a1\n");
        return;
    case ND_SUB: 
        printf(" sub a0, a0, a1\n");
        return;
    case ND_MUL:
        printf(" mul a0, a0, a1\n");
        return;
    case ND_DIV:
        printf(" div a0, a0, a1\n");
        return;
    default:
        error("invalid expression!");
    }

}
/**
 * 1. check input => valid arguments
 * 2. parse arguments => token series  
 * 3. parse token series & move the tokens' marker to EOF => AST 
 * 4. parse AST => expression
 * 5. generate expr => tmp.s
 * 6. some other things.
*/
int
main(int argc, char **argv)
{

    //1
    if (argc != 2) {
        error("%s: invalid number of arguments!", argv[0]);
    }
    input_string = argv[1];
    //2
    Token *token = tokenize();
    //3
    AST ast = expr(&token, token); 
    if (token->kind != TK_EOF)
        error_at_token(token, "extra token after EOF");

    printf("  .globl main\n");
    printf("main:\n");

    gen_expr(ast);
    // printf(" li a0, %d\n", numberFrom(token)); 
    // token = token->next;
    // while (token->kind != TK_EOF) {
    //     if (eq(token, "+")) {
    //         token = token->next;
    //         printf(" addi a0, a0, %d\n", numberFrom(token));
    //         token = token->next;
    //         continue;
    //     } 
    //     // else => '-'
    //     token = skip(token, "-");
    //     printf(" addi a0, a0, -%d\n", numberFrom(token));
    //     token = token->next;
    // }
    printf(" ret\n");
    assert(depth==0);
    return 0;
}

