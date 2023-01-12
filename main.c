#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    TK_OPERATOR,
    TK_NUM,
    TK_EOF,
} TokenKind;

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
            /* parse digit */
            // TODO:`nextToken(kind: TokenKind, ptr: char *)`
            cur->next = newToken(TK_NUM, p, p);
            cur = cur->next;
            // NOTICE: strtol函数才是捕捉str的关键，*p++这样只是得到每一个char. 
            // strtol是把传入指针从头开始的连续数字进行解析，如果开头就不是数字那就不解析了
            // strtol这里将p进行了移动。
            const char *prev_ptr = p;
            cur->val = strtol(p, &p, 10);//base = 10 
           // NOTICE: loc = 0？
            cur->len = p - prev_ptr; 
            continue;
        }
        if (*p == '+' || *p == '-') {
            cur->next = newToken(TK_OPERATOR, p, p+1);
            cur = cur->next;
            p++;
           continue; 
        }

        error_at(p, "invalid token: %c");
    } 
    // add an EOF
    cur->next = newToken(TK_EOF, p, p);
    return head.next;
}

int numberFrom(Token *token) {
   if (token->kind != TK_NUM) {
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


int
main(int argc, char **argv)
{

    if (argc != 2) {
        error("%s: invalid number of arguments!", argv[0]);
    }

    input_string = argv[1];
    Token *token = tokenize();
    // char *p = " 12 + 34 - 27 ";
    // Token *token = tokenize(" 12 + 34 -27");
    printf("  .globl main\n");
    printf("main:\n");
    printf(" li a0, %d\n", numberFrom(token)); 
    token = token->next;
    while (token->kind != TK_EOF) {
        if (eq(token, "+")) {
            token = token->next;
            printf(" addi a0, a0, %d\n", numberFrom(token));
            token = token->next;
            continue;
        } 
        // else => '-'
        token = skip(token, "-");
        printf(" addi a0, a0, -%d\n", numberFrom(token));
        token = token->next;
    }
    printf(" ret\n");
      return 0;
}

