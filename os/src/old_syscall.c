#include "os.h"

size_t syscall(size_t id,reg_t arg1,reg_t arg2,reg_t arg3){
    reg_t ret;
    asm volatile(
        "mv a7,%1\n\t"
        "mv a0,%2\n\t"
        "mv a1,%3\n\t"
        "mv a2,%4\n\t"
        "ecall\n\t"//进入trap处理函数
        "mv %0,a0"
        : "=r"(ret)
        : "r"(id),"r"(arg1),"r"(arg2),"r"(arg3)
        : "a7","a0","a1","a2","memory"
    );
    return ret;
    
}

// void __sys_yield(){
//     if(_top<=0){
//         panic("no task is running. can not yield");
//         return;
//     }
//     else{
//         int next = _current + 1;
//         next = next % _top;
//         if(tasks[next].task_state == Ready){
//             TaskContext* current_sptr = &tasks[_current].task_context;
//             TaskContext* next_sptr = &tasks[next].task_context;
//             tasks[next].task_state = Running;
//             tasks[_current].task_state = Ready;
//             _current = next;
//             __switch(current_sptr,next_sptr);

//         }
//     }


// }

// //参数在进trap时被存入了内存栈中，所以可以在栈上读取
// reg_t __SYSCALL(size_t id,reg_t arg1,reg_t arg2,reg_t arg3){
//     switch (id)
//     {
//     case __SYS_PRINTF:
//         uart_puts((char*) arg1);
//         break;
//     case __SYS_YIELD:
//         __sys_yield();
//         break;
//     case __SET_TRIGGER:
//         set_next_trigger();
//         break;
//     default:
//         panic("this syscall id is illegal");
//         break;
//     }
//     return 0;
// }


