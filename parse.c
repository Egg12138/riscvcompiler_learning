#include "rvcc.h"

// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num
static Node *expr_stmt(Token **rest, Token *token);
static Node *stmt(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);


// 新建一个节点
static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

// 新建一个单叉树
static Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->LHS = expr;
  // node->RHS = NULL;
  return node;
}

// 新建一个二叉树节点
static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->LHS = lhs;
  node->RHS = rhs;
  return node;
}

// 新建一个数字节点
static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}



// 解析表达式
// expr = equality
static Node *expr(Token **rest, Token *token) 
{ return equality(rest, token); }

static Node *stmt(Token **rest, Token *token) 
{ return expr_stmt(rest, token); }

static Node *expr_stmt(Token **rest, Token *token) 
{ 
  Node *node = new_unary(ND_EXPR_STMT, expr(&token, token));
  *rest = skip(token, ";");
  return node;
}

// 解析相等性
// equality = relational ("==" relational | "!=" relational)*
static Node *equality(Token **rest, Token *token) {
  // relational
  Node *node = relational(&token,token);

  // ("==" relational | "!=" relational)*
  while (true) {
    // "==" relational
    if (equal(token,"==")) {
      node = new_binary(ND_EQ, node, relational(&token,token->next));
      continue;
    }

    // "!=" relational
    if (equal(token,"!=")) {
      node = new_binary(ND_NE, node, relational(&token,token->next));
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
      node = new_binary(ND_LT, node, add(&token,token->next));
      continue;
    }

    // "<=" add
    if (equal(token,"<=")) {
      node = new_binary(ND_LE, node, add(&token,token->next));
      continue;
    }

    // ">" add
    // X>Y等价于Y<X
    if (equal(token,">")) {
      node = new_binary(ND_LT, add(&token,token->next), node);
      continue;
    }

    // ">=" add
    // X>=Y等价于Y<=X
    if (equal(token,">=")) {
      node = new_binary(ND_LE, add(&token,token->next), node);
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
      node = new_binary(ND_ADD, node, mul(&token,token->next));
      continue;
    }

    // "-" mul
    if (equal(token,"-")) {
      node = new_binary(ND_SUB, node, mul(&token,token->next));
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
      node = new_binary(ND_MUL, node, unary(&token,token->next));
      continue;
    }

    // "/" unary
    if (equal(token,"/")) {
      node = new_binary(ND_DIV, node, unary(&token,token->next));
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
    return unary(rest, token->next);

  // "-" unary
  if (equal(token,"-"))
    return new_unary(ND_NEG, unary(rest, token->next));

  // primary
  return primary(rest, token);
}

// 解析括号、数字
// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *token) {
  // "(" expr ")"
  if (equal(token,"(")) {
    Node *node = expr(&token,token->next);
    *rest = skip(token,")");
    return node;
  }

  // num
  if (token->kind == TK_NUM) {
    Node *node = new_num(token->val);
    *rest = token->next;
    return node;
  }

  error_token(token,"expected an expression");
  return NULL;
}




Node *parse(Token *token) {
  Node prog_head = {};
  Node *cur = &prog_head;
  while (token->kind != TK_EOF) {
    cur->next = stmt(&token, token);
    cur = cur->next;
  }
  return prog_head.next;
}
