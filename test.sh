#/bin/bash
assert() {
    expected="$1" # the first argument
    input="$2"
    id="$3"
    ./rvcc "$input" > tmp.s || exit  # OR exit
    # RISCV64平台编译
    riscv64-linux-gnu-gcc -static -o tmp tmp.s  
    # riscv64-unknown-elf-gcc-9.3.0 -static -o tmp tmp.s
    # 在qemu上运行
    qemu-riscv64 ./tmp
    result="$?"
    echo "test $input..."
    if [ "$result" = "$expected" ]; then
        echo "\tleft: \`$input\` == right: \`$result\`"
        echo "\ttest:[$id]...Ok."
    else
        echo "\tleft:\`$input\` != right:\`$result\`, \`$expected\` is expected, but got \`$result\`!"
        echo "\ttest:[$id]...Failed."
        #exit 1
    fi
}

# assert 期待值 输入值
# [1] 返回指定数值
assert 0 '0;'
assert 42 '42;'

# [2] 支持+ -运算符
assert 34 '12-34+56;'

# [3] 支持空格
assert 41 ' 12 + 34 - 5 ;'

# [5] 支持* / ()运算符
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 17 '1-8/(2*2)+3*6;'

# [6] 支持一元运算的+ -
assert 10 '-10+20;'
assert 139 '139+1-1;' # if segmentation fault
assert 10 '- -10;'
assert 10 '- - +10;'
assert 48 '------12*+++++----++++++++++4;'

# [7] 支持条件运算符
assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'
assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'
assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'
assert 1 '5==2+3;'
assert 0 '6==4+3;'
assert 1 '0*9+5*2==4+4*(6/3)-2;'

# [Single Alpha Variable]; 
assert 3 'a = 3; a;'
assert 8 ' a = 3; z = 5; a + z;'
assert 6 'z=y=3; z + y;'
assert 5 'a=3;b=4;a=1;a+b;'
assert 0 'a=3;'

# TODO [最后总是期待一个返回值, 无;则返回(rust)]
assert 5 'a = 5; a'
assert 0 'b = 3; '
# 如果运行正常未提前退出，程序将显示OK
echo OK