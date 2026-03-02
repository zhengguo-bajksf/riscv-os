#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include "types.h"
#define __SYS_PRINTF 1
#define __SYS_YIELD 2
#define __SET_TRIGGER 3
size_t syscall(size_t id,reg_t arg1,reg_t arg2,reg_t arg3);//用户态使用系统调用
reg_t __SYSCALL(size_t id,reg_t arg1,reg_t arg2,reg_t arg3);//trap里用户系统调用的处理函数
void __sys_yield();

#endif