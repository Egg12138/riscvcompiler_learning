# -fno-common禁止了将未初始化的全局变量放入common段
CFLAGS=-std=c11 -g -fno-common 
CC=gcc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
rvAS=riscv64-linux-gnu-as
rvLD=riscv64-linux-gnu-ld
QM=qemu-riscv64

# default 
rvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): rvcc.h

test: rvcc
	sh ./test.sh

bare: tmp.S
	@cat tmp.S
	$(rvAS) -o tmp.o ./tmp.S
	$(rvLD) -o tmp tmp.o
	$(QM) ./tmp	

oldtest: rvcc
	sh ./old_test.sh

clean:
	rm -f rvcc *.o *.s tmp* *.out

.PHONY: test clean
