#ifndef __TASK_H__
#define __TASK_H__

#include "trap.h"
#include "address.h"

#define MAX_TASK_NUM 10
extern int _current;//当前任务
extern int _top;//目前任务分配到


typedef enum TaskState
{
	UnInit, // 未初始化
    Ready, // 准备运行
    Running, // 正在运行
    Zombie, // 已退出
}TaskState;

/*struct TaskControlBlock 是结构体标签
TaskControlBlock 是 typedef 别名
typedef 别名在整个 struct 定义结束后才生效*/
typedef struct TaskControlBlock
{
    TaskState task_state;       //任务状态
    int pid;                    //process id
    struct TaskControlBlock* parent;  //父进程
    TaskContext task_context;   //任务上下文
    u64  trap_cx_ppn;            //Trap 上下文所在物理地址
    u64  base_size;             //应用数据大小
    u64  kstack;                //应用内核栈的虚拟地址
    u64  ustack;                //应用用户栈的虚拟地址
    u64  entry;                 //应用程序入口地址
    PageTable pagetable;       //应用的根页表地址
    u64 exit_code;              //进程退出码
}TaskControlBlock;
extern TaskControlBlock tasks[MAX_TASK_NUM];

void task_init();
void start_task();
// void delay(volatile int count);
void task_context_init(reg_t cptr,TaskContext* tptr,void(*ret));
TaskControlBlock* task_create_pt(size_t app_id);
PhysAddr get_current_trap_stack();
/* 返回当前执行的应用程序的satp token*/
u64 current_user_token();
int alloc_pid();
TaskControlBlock* alloc_proc();
int uvm_copy(PageTable* father,PageTable* child,u64 sz);
void procinit();
/* 为用户程序创建页表，映射跳板页和trap上下文页*/
void proc_pagetable(TaskControlBlock *p);
void task_context_init(reg_t cptr,TaskContext* tptr,void(*ret));
void exit_current_and_run_next(u64 exit_code);
void free_child(TaskControlBlock* child);

#endif