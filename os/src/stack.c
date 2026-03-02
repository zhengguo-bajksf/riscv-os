#include "os.h"

// uint8_t KernelStack[MAX_TASK_NUM][KERNEL_STACK_SIZE];
// uint8_t UserStack[MAX_TASK_NUM][USER_STACK_SIZE];


// 初始化栈
void initStack(Stack *stack) {
    stack->top = -1;
}

// 判断栈是否为空
bool isEmpty(Stack *stack) {
    return stack->top == -1;
}

// 判断栈是否已满
bool isFull(Stack *stack) {
    return stack->top == MAX_SIZE - 1;
}

// 入栈操作
void push(Stack *stack, u64 value) {
    if (isFull(stack)) {
        printf("Stack overflow\n");
        return;
    }
    stack->data[++stack->top] = value;
}

// 出栈操作
u64 pop(Stack *stack) {
    if (isEmpty(stack)) {
        printf("Stack underflow\n");
        return -1; // 表示栈为空或操作失败
    }
    return stack->data[stack->top--];
}

// 获取栈顶元素
u64 top(Stack *stack) {
    if (isEmpty(stack)) {
        printf("Stack is empty\n");
        return -1; // 表示栈为空
    }
    return stack->data[stack->top];
}