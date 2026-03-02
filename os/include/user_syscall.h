#ifndef __USERSYSCALL_H__
#define __USERSYSCALL_H__

#include "types.h"
#define __SYS_PRINTF 1
#define __SYS_YIELD 2
#define __SET_TRIGGER 3
#define __SYS_GET_TIME__ 4
#define __SYS_GET_CHAR__ 5
#define __SYS_FORK 6
#define __SYS_EXEC 7
#define __SYS_EXIT 8
#define __SYS_RECYCLE 9

uint64_t user_syscall(size_t id, reg_t arg1, reg_t arg2, reg_t arg3);
void user_get_time();
void user_set_trigger();
void user_yield();
void delay(volatile int count);
char user_getchar();
int user_fork();
int sys_exec(char* name);
int sys_exit(uint64_t exit_code);
int user_recycle();

#endif