#/bin/bash
assert() {
    expected="$1" # the first argument
    input="$2"
    ./rvcc "$input" > tmp.s || exit  # OR exit
    # RISCV64平台编译
    riscv64-linux-gnu-gcc -static -o tmp tmp.s  
    # 在qemu上运行
    qemu-riscv64 ./tmp
    actual="$?"
    echo "test $input..."
    if [ "$actual" = "$expected" ]; then
        echo "\tleft: \`$input\` == right: \`$actual\`"
        echo "\t.[Ok]."
    else
        echo "\tleft:\`$input\` != right:\`$actual\`, \`$expected\` is expected!"
        echo "\t.[Failed]."
        #exit 1
    fi
}


# assert 期待值 输入值
# [1] 返回指定数值
# assert 0 0
# assert 42 42

# [2] 支持+ -运算符
# assert 34 '12-34+56'

# [3] 支持空格
assert  120  "   70-58+108"
assert 128 '64+ 64'
assert 41 ' 12 + 34 - 5 '

echo "All done"
