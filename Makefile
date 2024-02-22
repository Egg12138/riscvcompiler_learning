# -fno-common禁止了将未初始化的全局变量放入common段
CFLAGS=-std=c11 -g -fno-common 
CC=gcc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

# default 
rvcc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): rvcc.h

test: rvcc
	sh ./test.sh

clean:
	rm -f rvcc *.o *.s tmp* *.out

.PHONY: test clean
