#include "os.h"
int _current = 0;
int _top = 0;
TaskControlBlock tasks[MAX_TASK_NUM];
int next_pid = 0;
int alloc_pid(){
  int pid = next_pid;
  next_pid++;
  return pid;
}


void task_context_init(reg_t cptr,TaskContext* tptr,void(*ret)){
    tptr->ra = (reg_t)ret;
    tptr->sp = (reg_t)cptr;//用户x的内核栈虚拟地址
    tptr->s0 = 0;
    tptr->s1 = 0;
    tptr->s2 = 0;
    tptr->s3 = 0;
    tptr->s4 = 0;
    tptr->s5 = 0;
    tptr->s6 = 0;
    tptr->s7 = 0;
    tptr->s8 = 0;
    tptr->s9 = 0;
    tptr->s10 = 0;
    tptr->s11 = 0;

}



/* 为每个应用程序分配一页内存用与存放trap，同时初始化任务上下文 */
void proc_trap(TaskControlBlock *p)
{
  // 为每个程序分配一页trap物理内存
  p->trap_cx_ppn = phys_addr_from_phys_page_num(kalloc());
  // 初始化任务上下文全部为0
  memset(&p->task_context, 0 ,sizeof(p->task_context));
}


/* 为用户程序创建页表，映射跳板页和trap上下文页*/
void proc_pagetable(TaskControlBlock *p)
{
  // 创建一个空的用户的页表，分配一页内存
  PageTable pagetable;
  pagetable.root_ppn = kalloc();

  //映射跳板页
  PageTable_map(&pagetable,virt_addr_from_size_t(TRAMPOLINE),phys_addr_from_size_t((u64)trampoline),\
                PAGE_SIZE , PTE_R | PTE_X);
  
  //映射用户程序的trap页
  PageTable_map(&pagetable,virt_addr_from_size_t(TRAPFRAME),phys_addr_from_size_t(p->trap_cx_ppn), \
                PAGE_SIZE, PTE_R | PTE_W );
  p->pagetable = pagetable;
}


TaskControlBlock* task_create_pt(size_t app_id)
{
  if(_top < MAX_TASK_NUM)
  {

    //分配应用程序存放trap的物理页  初始化结构体里的任务上下文
    proc_trap(&tasks[app_id]);   // 
    //为应用程序创建根页表  映射跳板页到应用空间 映射trap物理页到应用空间
    proc_pagetable(&tasks[app_id]); 
    _top++;
  }
  
  return &tasks[app_id];
}

//返回当前应用上下文存放的物理地址
PhysAddr get_current_trap_stack(){
    return tasks[_current].trap_cx_ppn;
}

/* 返回当前执行的应用程序的satp token*/
uint64_t current_user_token()
{
   return MAKE_SATP(tasks[_current].pagetable.root_ppn);
}


void start_task(){
    tasks[0].task_state = Running;
    TaskContext *next_task = &(tasks[0].task_context);
    TaskContext unused;
    __switch(&unused,next_task);
    panic("task start fault");

}

//初始化任务状态为未分配
void procinit(){
  TaskControlBlock *p;
  for ( p = tasks; p < &tasks[MAX_TASK_NUM]; p++)
  {
    p->task_state = UnInit;
  }
  
}

TaskControlBlock* alloc_proc(){
  TaskControlBlock* p;
  for (p = tasks; p < &tasks[MAX_TASK_NUM]; p++)
  {
    if (p->task_state == UnInit)
    {
      //初始化进程结构体
      p->pid = alloc_pid();
      p->task_state = Ready;
      proc_trap(p);
      proc_pagetable(p);
      _top++;
      return p;
    }
  }
  return NULL;
}

int uvm_copy(PageTable* father,PageTable* child,u64 sz){
  PageTableEntry* pte;
  u8 flags;
  //从虚拟地址0开始到base_size 逐页检查父进程是否有映射
  for ( uint64_t i = 0; i < sz; i+=PAGE_SIZE)
  {
    VirtPageNum vpn = floor_virts(virt_addr_from_size_t(i));
    pte = find_pte(father,vpn);
    if(pte!=NULL){
      //从pte里得到页物理地址以及flag
      PhysAddr phyadd = PTE_PA(*pte);
      flags = PTE_FLAGS(*pte);

      PhysAddr child_phyadd = phys_addr_from_phys_page_num(kalloc());
      memcpy((void*)child_phyadd,(void*)phyadd,PAGE_SIZE);
      PageTable_map(child,virt_addr_from_size_t(i),child_phyadd,PAGE_SIZE,flags);
    }
  }
  return 0;
}

void child_proc_mv(TaskControlBlock* p){
  for (TaskControlBlock* i = tasks; i < &tasks[MAX_TASK_NUM]; i++)
  {
    if (i->parent == p)
    {
      i->parent = &tasks[0];
    }
    
  }
  
}

//退出停止当前进程
void exit_current_and_run_next(u64 exit_code){
  TaskControlBlock* p = &tasks[_current];
  if (p->pid==0)
  {
    panic("error:init exiting");
  }

  p->exit_code = exit_code;
  p->task_state = Zombie;//等待父进程回收的僵尸进程
  //把这个进程下的子进程挂到第一个进程下
  child_proc_mv(p);
  _top--;
  __sys_yield();
  panic("Zombie exit failed");
  
}

void free_child(TaskControlBlock* child){
    PageTable* pt = &child->pagetable;
    u64 sz = child->base_size;

    //释放高地址内存空间 跳板页和trap页
    page_unmap_free(pt,floor_virts(virt_addr_from_size_t(TRAMPOLINE)),1,0);
    page_unmap_free(pt,floor_virts(virt_addr_from_size_t(TRAPFRAME)),1,1);
    exec_free_lowhalf(pt,sz);

    child->pagetable.root_ppn = 0;
    child->trap_cx_ppn = 0;
    child->base_size = 0;
    child->parent = 0;
    child->ustack = 0;
    child->entry = 0;
    child->task_state = UnInit;
    child->exit_code = 0;
}





// void task0(){
//     sys_set_trigger();
//     while (1)
//     {
//         sys_printf("task0 is running\n");
//         delay(10000);
//     }

// }


// void task1(){
//     while (1)
//     {
//         sys_printf("task1 is running\n");
//         delay(10000);
//     }
// }

// void task2(){
//     while (1)
//     {
//         sys_printf("task2 is running\n");
//         delay(10000);
//     }
// }


// void task_init(){
//     w_stvec((reg_t)__alltraps);
//     task_create(task0);
//     task_create(task1);
//     task_create(task2);
// }

// reg_t sys_yield(){
//     syscall(__SYS_YIELD,0,0,0);
// }


//构造trap以及task上下文
// void task_create(void (*task_entry)(void)){
//     if(_top>=MAX_TASK_NUM){
//         panic("task num is full");
//         return;
//     }
//     //trap上下文
//     TrapContext* core_stack_ptr  = (TrapContext*)(&KernelStack[_top]+KERNEL_STACK_SIZE-sizeof(TrapContext));
//     reg_t user_sp = (reg_t)(&UserStack[_top]+USER_STACK_SIZE);
//     reg_t sstatus = r_sstatus();
//     sstatus &= ~(1U<<8);

//     core_stack_ptr->sepc = (reg_t)task_entry;
//     core_stack_ptr->sp = user_sp;
//     core_stack_ptr->sstatus = sstatus;

//     //任务上下文
//     task_context_init(core_stack_ptr,&tasks[_top].task_context,__first_restore);
//     tasks[_top].task_state = Ready;

//     _top++;
// }







