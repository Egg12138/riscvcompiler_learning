#include "rvcc.h"

static char *current_input;

// 输出错误信息
// static文件内可以访问的函数
// Fmt为传入的字符串， ... 为可变参数，表示Fmt后面所有的参数
void error(char *fmt, ...) {
  // 定义一个va_list变量
  va_list va;
  // VA获取Fmt后面的所有参数
  va_start(va, fmt);
  // vfprintf可以输出va_list类型的参数
  vfprintf(stderr, fmt, va);
  // 在结尾加上一个换行符
  fprintf(stderr, "\n");
  // 清除VA
  va_end(va);
  // 终止程序
  exit(1);
}

// 输出错误出现的位置
static void verror_at(char *loc, char *fmt, va_list va) {
  // 先输出源信息
  fprintf(stderr, "%s\n", current_input);

  // 输出出错信息
  // 计算出错的位置，Loc是出错位置的指针，CurrentInput是当前输入的首地址
  int Pos = loc - current_input;
  // 将字符串补齐为Pos位，因为是空字符串，所以填充Pos个空格。
  fprintf(stderr, "%*s", Pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, va);
  fprintf(stderr, "\n");
  va_end(va);
}

// 字符解析出错，并退出程序
void error_at(char *loc, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  verror_at(loc, fmt, va);
  exit(1);
}

// Tok解析出错，并退出程序
void error_token(Token *token,char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  verror_at(token->loc, fmt, va);
  exit(1);
}
// 判断Tok的值是否等于指定值，没有用char，是为了后续拓展
bool equal(Token *token,char *Str) {
  // 比较字符串LHS（左部），RHS（右部）的前N位，S2的长度应大于等于N.
  // 比较按照字典序，LHS<RHS回负值，LHS=RHS返回0，LHS>RHS返回正值
  // 同时确保，此处的Op位数=N
  return memcmp(token->loc, Str, token->len) == 0 && Str[token->len] == '\0';
}

// 跳过指定的Str
Token *skip(Token *token,char *Str) {
  if (!equal(token,Str))
    error_token(token,"expect '%s'", Str);
  return token->next;
}

// 返回TK_NUM的值
static int get_number(Token *token) {
  if (token->kind != TK_NUM)
    error_token(token,"expect a number");
  return token->val;
}

// 生成新的Token
static Token *new_token(TokenKind Kind, char *Start, char *End) {
  // 分配1个Token的内存空间
  Token *token = calloc(1, sizeof(Token));
  token->kind = Kind;
  token->loc = Start;
  token->len = End - Start;
  return token;
}

// 判断Str是否以SubStr开头
static bool starts_with(char *str, char *substr) {
  // 比较LHS和RHS的N个字符是否相等
  return strncmp(str, substr, strlen(substr)) == 0;
}

// 读取操作符
static int read_punct(char *ptr) {
  // 判断2字节的操作符
  if (starts_with(ptr, "==") || starts_with(ptr, "!=") || starts_with(ptr, "<=") ||
      starts_with(ptr, ">="))
    return 2;

  // 判断1字节的操作符
  return ispunct(*ptr) ? 1 : 0;
}

// 终结符解析
Token *tokenize(char *p) {
  current_input = p;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // 跳过所有空白符如：空格、回车
    if (isspace(*p)) {
      ++p;
      continue;
    }

    // 解析数字
    if (isdigit(*p)) {
      // 初始化，类似于C++的构造函数
      // 我们不使用Head来存储信息，仅用来表示链表入口，这样每次都是存储在Cur->next
      // 否则下述操作将使第一个Token的地址不在Head中。
      cur->next = new_token(TK_NUM, p, p);
      // 指针前进
      cur = cur->next;
      char *old_ptr = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - old_ptr;
      continue;
    }

    // 解析操作符
    int punctlen = read_punct(p);
    if (punctlen) {
      cur->next = new_token(TK_PUNCT, p, p + punctlen);
      cur = cur->next;
      // 指针前进Punct的长度位
      p += punctlen;
      continue;
    }

    // 处理无法识别的字符
    error_at(p, "invalid token");
  }

  // 解析结束，增加一个EOF，表示终止符。
  cur->next = new_token(TK_EOF, p, p);
  // Head无内容，所以直接返回Next
  return head.next;
}

