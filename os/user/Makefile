CROSS_COMPILE = riscv64-unknown-elf-
CFLAGS = -nostdlib -fno-builtin -mcmodel=medany

CC = ${CROSS_COMPILE}gcc
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump
INCLUDE:=-I../include

LIB = ../lib
BIN_DIR = bin
SRC = ../src

all: time print atest user_shell

# 创建bin目录的规则
# $(BIN_DIR):
# 	mkdir -p $@

# write: write.c $(LIB)/*.c
# 	${CC} ${CFLAGS} $(INCLUDE) -T user.ld $^ -o bin/write 

time: time.c $(LIB)/*.c
	${CC} ${CFLAGS} $(INCLUDE) -T user.ld -Wl,-Map=time.map -o bin/time $^

print: print.c $(LIB)/*.c
	${CC} ${CFLAGS} $(INCLUDE) -T user.ld $^ -o bin/print

atest: atest.c $(LIB)/*.c
	${CC} ${CFLAGS} $(INCLUDE) -T user.ld -Wl,-Map=atest.map -o bin/atest $^
user_shell: user_shell.c $(LIB)/*.c
	${CC} ${CFLAGS} $(INCLUDE) -T user.ld -Wl,-Map=user_shell.map -o bin/user_shell $^

debug: objdump_print objdump_time

objdump_print:
	${OBJDUMP} -d bin/print > print.txt
objdump_time:
	${OBJDUMP} -d bin/time > time.txt
objdump_atest:
	${OBJDUMP} -d bin/atest > atest.txt