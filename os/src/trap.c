#include "os.h"

void __sys_yield(){
    if(_top<=0){
        panic("no task is running. can not yield");
        return;
    }
    else{
        int next = _current + 1;
        next = next % _top;
        if(tasks[next].task_state == Ready || tasks[next].task_state == Running){
            TaskContext* current_sptr = &tasks[_current].task_context;
            TaskContext* next_sptr = &tasks[next].task_context;
            tasks[next].task_state = Running;
			if (tasks[_current].task_state == Running)
			{//可能是僵尸进程的切换，所以不应该变成ready
				tasks[_current].task_state = Ready;
			}
            _current = next;
            __switch(current_sptr,next_sptr);
			// printf("next_task_kstack:%lx\n",tasks[_current].kstack);
        }
    }


}

//将输入的虚拟地址以当前运行的应用空间为基础进行翻译到内核空间
reg_t uptr_to_kptr(reg_t uptr){
	PageTable user_pagetable = tasks[_current].pagetable;
	//输入虚拟页号 查询页表项
	PageTableEntry* pte = find_pte(&user_pagetable,virt_page_num_from_virt_addr(virt_addr_from_size_t(uptr)) );
	//从页表项中取出44位的物理地址
	PhysPageNum pnn =  (*pte>>10) & ((1ul << 44) - 1);
	//与虚拟地址中的12位页内偏移组合得到56位物理地址
	PhysAddr phyadd = (pnn << 12) + (uptr & ((1ul << 12) - 1));

	return phyadd;
}

void __sys_getchar(reg_t fd,const char* data){
	if (fd==stdin)
	{
		int c;
		while (1)
		{
			c = sbi_console_getchar();
			if (c!=-1) break;
			__sys_yield();
		}
		char* str = (char*)uptr_to_kptr((reg_t)data);
		str[0] = c;
	}
	
}

//父进程使其trap返回子进程的pid号，子进程返回0
int __sys_fork(){
	TaskControlBlock* fa_proc = &tasks[_current];
	TaskControlBlock* child_proc;
	//进程已满分配失败
	if((child_proc = alloc_proc())==NULL){
		printf("child_proc failed!");
		return -1;
	}
	//copy用户段以及用户栈
	uvm_copy(&fa_proc->pagetable,&child_proc->pagetable,fa_proc->base_size);
	//copy用户的trap页
	memcpy((void*)child_proc->trap_cx_ppn,(void*)fa_proc->trap_cx_ppn,PAGE_SIZE);

	//设置子进程返回值为0
	((TrapContext*)child_proc->trap_cx_ppn)->a0 = 0;
	((TrapContext*)child_proc->trap_cx_ppn)->kernel_sp = child_proc->kstack;
	//复制进程控制结构体信息
	child_proc->entry = fa_proc->entry;
	child_proc->base_size = fa_proc->base_size;
	child_proc->parent = fa_proc;
	child_proc->ustack = fa_proc->ustack;
	

	// printf("child_sepc:%x,father:%x\n",((TrapContext*)child_proc->trap_cx_ppn)->sepc,((TrapContext*)fa_proc->trap_cx_ppn)->sepc);
	printf("running task num:%d\n",_top);
	//修改子进程的返回地址和内核栈
	task_context_init((reg_t)child_proc->kstack,&child_proc->task_context,(void*)trap_return);


	return child_proc->pid;

}

//覆盖当前进程，运行一个新的任务
void __sys_exec(char* name){
	//找到应用的elf文件地址
	AppMetadata metadata = get_app_data_by_name(name);
	elf64_ehdr_t* ehdr = (elf64_ehdr_t*)metadata.start;
	elf_check(ehdr);
	//保存旧进程的根页表地址和应用空间的占用大小，用于后面释放
	TaskControlBlock* proc = &tasks[_current];
	PageTable old_pagetable = proc->pagetable;
	u64 old_size = proc->base_size;

	//新建根页表，重新映射了trap和跳板页，其中trap是继承了旧进程的
	proc_pagetable(proc);
	//load程序段  用户栈
	load_segment(ehdr,proc);

	TrapContext* ctrap = (TrapContext*)proc->trap_cx_ppn;
	ctrap->sepc = (u64)ehdr->e_entry;
	ctrap->sp = proc->ustack;//应用大小不一样所以用户栈地址不一样

	//释放原本的页表以及内存，不释放trap和跳板页的物理页
	free_proc_memory(&old_pagetable,old_size);

}

//把当前进程下的子进程移动到进程0下，同时任务状态设置为Zombie，不再运行
//在父进程中去回收资源
void __sys_exit(u64 exit_code){
	exit_current_and_run_next(exit_code);
}

//父进程中回收子进程的资源
int __sys_recycle(){
	TaskControlBlock* father = &tasks[_current];
	TaskControlBlock* child;
	int pid;
	bool havechild = 0;

	while (1)
	{
		for (child = tasks; child < &tasks[MAX_TASK_NUM]; child++)
		{
			if(child->parent==father){
				havechild = 1;
				if (child->task_state==Zombie)
				{
					pid = child->pid;
					free_child(child);
					printf("free pid:%d\n",pid);
					return pid;
				}
			}
		}
		//没有子进程则返回错误
		if (!havechild)
		{
			printf("no child\n");
			return -1;
		}
		//如果有子进程但是并未变成僵尸则让出资源并等待
		__sys_yield();
	}
	
}

//参数在进trap时被存入了内存栈中，所以可以在栈上读取
reg_t __SYSCALL(size_t id,reg_t arg1,reg_t arg2,reg_t arg3){
    switch (id)
    {
    case __SYS_PRINTF:
		//user态ecall传进来的地址是用户空间的地址，所以我需要把它转化为内核态的地址
		arg1 = uptr_to_kptr(arg1);
        uart_puts((char*) arg1);
        break;
    case __SYS_YIELD:
        __sys_yield();
        break;
    case __SET_TRIGGER:
        set_next_trigger();
        break;
	case __SYS_GET_TIME__:
		printf("now time:%d\n",get_time_us());
		break;
	case __SYS_GET_CHAR__:
		__sys_getchar((reg_t)arg1,(char*)arg2);
		break;
	case __SYS_FORK:
		return __sys_fork();
	case __SYS_EXEC:
		arg2 = uptr_to_kptr(arg2);
		printf("exec app_name:%s\n",arg2);
		__sys_exec((char*)arg2);
		break;
	case __SYS_EXIT:
		__sys_exit((u64)arg1);
		break;
	case __SYS_RECYCLE:
		return __sys_recycle();
    default:
        panic("this syscall id is illegal");
        break;
    }
    return 0;
}


void trap_from_kernel(){
	reg_t scause = r_scause();
	printf("stval  = %lx\n", r_stval());
	printf("sepc   = %lx\n", r_sepc());
    printf("scause:%d\n",scause);
	panic("trap from kernel!\n");
}

void set_kernel_trap_entry(){
	w_stvec((reg_t)trap_from_kernel);
}
//设置trap入口为跳板页的虚拟地址
void set_user_trap_entry(){
	w_stvec((reg_t)TRAMPOLINE);
}

void trap_return(){
	set_user_trap_entry();
	//应用空间 存放trap上下文的起始地址
	uint64_t trap_ptr = TRAPFRAME;
	//要继续执行的应用空间 的根页表
	uint64_t user_satp = current_user_token();
	/* 计算_restore函数的虚拟地址 位于跳板页上
	__alltraps 刚好在开头 所以偏移一下就是__restore的虚拟地址*/
    uint64_t  restore_va = (u64)__restore - (u64)__alltraps + TRAMPOLINE;  
	asm volatile (    
			"fence.i\n\t"    //清空指令缓存
			"mv a0, %0\n\t"  // 将trap_ptr传递给a0寄存器  
			"mv a1, %1\n\t"  // 将user_satp传递给a1寄存器  
			"jr %2\n\t"      // 跳转到restore_va的位置执行代码  
			:    
			: "r" (trap_ptr),    
			"r" (user_satp),
			"r" (restore_va)
			: "a0", "a1"
		);
}


void trap_handler()
{
	//进入内核态以后，把内核态的异常入口地址修改
	//因为u的trap和s的trap处理不一样
	set_kernel_trap_entry();
	//得到存放当前应用trap上下文的物理地址
	TrapContext* cx = (TrapContext*)get_current_trap_stack();
    reg_t scause = r_scause();
    // printf("scause:%x\n",scause);
	reg_t cause_code = scause & 0xfff;
	//翻译地址空间
	// printf("address:%lx\n",uptr_to_kptr(0x14fe8));
	

	if(scause & 0x8000000000000000)
	{
		switch (cause_code)
		{
		/* rtc 中断*/
		case 5:
			__sys_yield();
			set_next_trigger();
			// printf("%d\n",get_time_us());
			break;
		default:
			printf("undfined interrrupt scause:%x\n",scause);
			break;
		}
	}
	else
	{
		switch (cause_code)
		{
		/* U模式下的syscall */
		case 8:
			//保证fork出来以后，直接跳转到trap_return
			//switch出来以后也是直接到trap_return，所以两者状态是一样的
			//其实相当于起到break的作用
			cx->sepc += 8;
			cx->a0 = __SYSCALL(cx->a7,cx->a0,cx->a1,cx->a2);
			//exec会使得之前进程分配的内存失效，所以要重新获取一下
			//对于其他的系统调用没有影响
			// cx = (TrapContext*)get_current_trap_stack();
			// cx->a0 = result;
			break;
		default:
			printf("undfined exception scause:%x\n",scause);
			panic("stop in trap_handler\n");
			break;
		}
	}

	trap_return();

    
}

//初始化了trap上下文 以及任务的上下文信息 设置任务返回地址为trap_return
void app_init(size_t app_id){
	//trap存放的物理地址
	TrapContext* trapcon_ptr = (TrapContext*)tasks[app_id].trap_cx_ppn;
	//设置为user模式
	reg_t sstatus = r_sstatus();
	sstatus &= ~(1U<<8);
	w_sstatus(sstatus);

	trapcon_ptr->sepc = tasks[app_id].entry;
	trapcon_ptr->sstatus = sstatus;
	trapcon_ptr->sp = tasks[app_id].ustack;
	trapcon_ptr->kernel_satp = MAKE_SATP(kernel_pagetable.root_ppn);
	trapcon_ptr->kernel_sp = tasks[app_id].kstack;
	printf("app_%d_kstack:%lx\n",app_id, trapcon_ptr->kernel_sp);
	trapcon_ptr->trap_handler = (reg_t)trap_handler;
	//初始化任务返回地址，以及任务内核栈地址
	task_context_init((reg_t)tasks[app_id].kstack,&(tasks[app_id].task_context),trap_return);
	tasks[app_id].task_state = Ready;
	tasks[app_id].pid = alloc_pid();
}




// void test_user(){
//     sys_printf("out of trap\n");
//     int a = 1234;
//     sys_printf("a=%d\n",a);

// 	// sys_set_trigger();
//     while (1)
//     {
//         /* code */
//     }
    
// }

// void trap_context_init(){
//     reg_t uesr_sp = (reg_t)(&UserStack[0] + USER_STACK_SIZE);
//     TrapContext* core_stack_ptr = (TrapContext*)(&KernelStack[0] + KERNEL_STACK_SIZE - sizeof(TrapContext));
//     w_stvec((reg_t)__alltraps);

//     reg_t sstatus = r_sstatus();
//     sstatus &= ~(1U<<8);
//     core_stack_ptr->sepc = (reg_t)test_user;
//     core_stack_ptr->sstatus = sstatus;
//     core_stack_ptr->sp = uesr_sp;

//     __restore(core_stack_ptr);

// }

