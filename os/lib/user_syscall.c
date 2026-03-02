#include "types.h"
#include "user_syscall.h"
#include "user_print.h"

uint64_t user_syscall(size_t id, reg_t arg1, reg_t arg2, reg_t arg3) {

    register uintptr_t a0 asm ("a0") = (uintptr_t)(arg1);
    register uintptr_t a1 asm ("a1") = (uintptr_t)(arg2);
    register uintptr_t a2 asm ("a2") = (uintptr_t)(arg3);
    register uintptr_t a7 asm ("a7") = (uintptr_t)(id);

    asm volatile ("ecall"
		      : "+r" (a0)
		      : "r" (a1), "r" (a2), "r" (a7)
		      : "memory");
    return a0;
}


void user_yield(){
    user_syscall(__SYS_YIELD,0,0,0);
}


void user_get_time(){
    user_syscall(__SYS_GET_TIME__,0,0,0);
}

void user_set_trigger(){
    user_syscall(__SET_TRIGGER,0,0,0);
}

char user_getchar(){
    char data[1];
    user_syscall(__SYS_GET_CHAR__,(reg_t)stdin,(reg_t)data,0);
    return data[0];
}

int user_fork(){
    return user_syscall(__SYS_FORK,0,0,0);
}

int sys_exec(char* name){
    return user_syscall(__SYS_EXEC,0,(reg_t)name,0);
}

int sys_exit(uint64_t exit_code){
    return user_syscall(__SYS_EXIT,exit_code,0,0);
}

int user_recycle(){
    return user_syscall(__SYS_RECYCLE,0,0,0);
}

void delay(volatile int count){
    count *= 50000;
    while (count--);
}