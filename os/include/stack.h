#ifndef __STACK_H__
#define __STACK_H__

#include "types.h"
// #include "task.h"
// #define USER_STACK_SIZE (4096*2)
// #define KERNEL_STACK_SIZE (4096*2)
// extern uint8_t KernelStack[MAX_TASK_NUM][KERNEL_STACK_SIZE];
// extern uint8_t UserStack[MAX_TASK_NUM][USER_STACK_SIZE];

#define MAX_SIZE 10000

typedef struct {
    u64 data[MAX_SIZE];
    int top;    // 不能定义成无符号类型，不然会导致 -1 > 0
} Stack;

bool isEmpty(Stack *stack);
bool isFull(Stack *stack);
void push(Stack *stack, u64 value);
u64 pop(Stack *stack);
u64 top(Stack *stack);
void initStack(Stack *stack);

#endif