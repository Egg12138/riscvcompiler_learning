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
    if [ "$actual" = "$expected" ]; then
        echo "left: \`$input\` == \`$actual\`"
        echo "test [Ok]..."
    else
        echo "left:\`$input\` != right:\`$actual\`. \`$expected\` is expected!"
        echo "test [Failed]..."
        #exit 1
    fi
}

assert 0 0
assert 42 42
