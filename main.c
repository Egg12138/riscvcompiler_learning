#include "rvcc.h"



// 输入的字符串
// BUG remove the static 
// char *InputString;


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
  // InputString = argv[1];
  Token *token = tokenize(argv[1]);
  Node *node = parse(token);
  code_gen(node);
  return 0;
}