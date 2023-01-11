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

static void error(char *fmt, ...) {
  va_list VA;
  va_start(VA, fmt);
  vfprintf(stderr, fmt, VA);
  fprintf(stderr, "\n");
  va_end(VA);
  //exit(1);
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
static Token *tokenize(char *p) {
    Token head = {};
    Token *cur = &head;
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
            cur->val = strtol(p, &p, 10);//base = 10 
            const char *prev_ptr = p;
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

        error("invalid token: %c", *p);
    } 
    // add an EOF
    cur->next = newToken(TK_EOF, p, p);
    return head.next;
}

int numberFrom(Token *token) {
   if (token->kind != TK_NUM) {
       printf("got [%d]", token->val);
      error("expect a number"); 
   }
   return token->val;
}

static bool eq(Token *token, char *str) {
    return memcmp(token->loc,str, token->len) == 0 && str[token->len] == '\0';
}
static Token *skip(Token *token, char *str) {
  if (!eq(token, str)) {
     error("expect '%s'", str); 
  }
  return token->next;
}

int
main(int argc, char **argv)
{

    if (argc != 2) {
        error("%s: invalid number of arguments!", argv[0]);
    }

    Token *token = tokenize(argv[1]);
    char *p = argv[1];
    printf("  .globl main\n");
    printf("main:\n");
    printf(" li a0, %d\n", numberFrom(token)); 
    token = token->next;
    // 第一个肯定是数字
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

