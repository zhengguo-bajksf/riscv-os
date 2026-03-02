#include "os.h"

static StackFrameAllocator memory_allocator;//内存分配器 页号范围  回收栈
PageTable kernel_pagetable;//某个进程的页表    页表基地址 


//56位的物理地址 取低56位
PhysAddr phys_addr_from_size_t(uint64_t v) {
    PhysAddr addr;
    addr = v & ((1ULL << PA_WIDTH_SV39) - 1);
    return addr;
}

/* 把物理地址转换为物理页号 */
PhysPageNum phys_page_num_from_size_t(uint64_t v) {
    PhysPageNum pageNum;
    pageNum = v & ((1ULL << PPN_WIDTH_SV39) - 1);
    return pageNum;
}

/* 从物理页号转换为实际物理地址 */
PhysAddr phys_addr_from_phys_page_num(PhysPageNum ppn)
{
    PhysAddr addr;
    addr = ppn << PAGE_SIZE_BITS ;
    return addr;
}

//取低39位
VirtAddr virt_addr_from_size_t(uint64_t v) {
    VirtAddr addr;
    addr = v & ((1ULL << VA_WIDTH_SV39) - 1);
    return addr;
}

VirtPageNum virt_page_num_from_size_t(uint64_t v) {
    VirtPageNum pageNum;
    pageNum = v & ((1ULL << VPN_WIDTH_SV39) - 1);
    return pageNum;
}

uint64_t size_t_from_virt_addr(VirtAddr v) {
        return v | ~((1ULL << VA_WIDTH_SV39) - 1);
}

/* 物理地址向下取整 */
PhysPageNum floor_phys(PhysAddr phys_addr) {
    PhysPageNum phys_page_num;
    phys_page_num = phys_addr / PAGE_SIZE;
    return phys_page_num;
}

/* 物理地址向上取整 */
PhysPageNum ceil_phys(PhysAddr phys_addr) {
    PhysPageNum phys_page_num;
    phys_page_num = (phys_addr + PAGE_SIZE - 1) / PAGE_SIZE;
    return phys_page_num;
}

/* 虚拟地址向下取整 */
VirtPageNum floor_virts(VirtAddr virt_addr) {
    VirtPageNum virt_page_num;
    virt_page_num = virt_addr / PAGE_SIZE;
    return virt_page_num;
}

/* 把虚拟地址转换为虚拟页号 */
VirtPageNum virt_page_num_from_virt_addr(VirtAddr virt_addr)
{
    VirtPageNum vpn;
    vpn =  virt_addr / PAGE_SIZE;
    return vpn;
}


void* memset(void *ptr,int ch,uint64_t count){
    uint8_t* bptr = ptr;
    while (count--)
    {
        *bptr++ = ch;
    }
    
}

// 从存储区 src 复制 n 个字节到存储区 dest。
void* memcpy(void *dest, const void *src, size_t count)
{
    uint8_t *ptr = dest;
    while (count--)
    {
        *ptr++ = *((uint8_t *)(src++));
    }
    return dest;
}


//新建内存分配器
void StackFrameAllocator_new(StackFrameAllocator* allocator,PhysPageNum start,PhysPageNum end) {
    allocator->current = start;
    allocator->end = end;
    initStack(&allocator->recycled);
}
/*分配内存
1.先从栈内找回收的页
2.如果不够，则开辟新页
*/
PhysPageNum StackFrameAllocator_alloc(StackFrameAllocator *allocator) {
    PhysPageNum ppn;
    if (allocator->recycled.top >= 0) {
        ppn = pop(&(allocator->recycled));
    } 
    else {// [start,end)
        if (allocator->current == allocator->end) {
            panic("memory is full");
        } else {
            ppn = allocator->current++;
        }
    }
    /* 清空此页内存 ： 注意不能覆盖内核代码区，分配的内存只能是未使用部分*/
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    memset((void*)addr,0,PAGE_SIZE);
    return ppn;
}

/*回收内存
1.看该页有没有被分配出去，分别要看待分配区域和回收栈里
2.回收页，放到回收栈里
*/

void StackFrameAllocator_dealloc(StackFrameAllocator *allocator, PhysPageNum ppn) {
    // 检查回收的页面之前一定被分配出去过
    if (ppn >= allocator->current) {
        printf("Frame ppn=%lx has not been allocated!\n", ppn);
        return;
    }
    // 检查未在回收列表中
    if(allocator->recycled.top>=0)
    {
        for (uint64_t i = 0; i <= allocator->recycled.top; i++)
        {
            if(ppn ==allocator->recycled.data[i] ){
                panic("this page has not been alloced");
                return;
            }
            
        }
    }
    // 回收物理内存页号
    push(&(allocator->recycled), ppn);
}



void frame_allocator_test(){
    //初始化内存分配器
    StackFrameAllocator_new(&memory_allocator,floor_phys(phys_addr_from_size_t(MEMORY_START)),ceil_phys(phys_addr_from_size_t(MEMORY_END)));
    printf("memory page start:%d\n",floor_phys(phys_addr_from_size_t(MEMORY_START)));
    printf("memory page end:%d\n",ceil_phys(phys_addr_from_size_t(MEMORY_END)));

    PhysPageNum frame[5];
    for (int i = 0; i < 5; i++)
    {
        frame[i] =  StackFrameAllocator_alloc(&memory_allocator);
        printf("%d\n",frame[i]);
    }
    printf("\n");

    for (int i = 0; i < 5; i++)
    {
        StackFrameAllocator_dealloc(&memory_allocator,frame[i]);
        printf("%d\n",frame[i]);
    }

    printf("\n");

    for (int i = 0; i < 5; i++)
    {
        frame[i] =  StackFrameAllocator_alloc(&memory_allocator);
        printf("%d\n",frame[i]);
    }


}


/* 新建一个页表项 */
PageTableEntry PageTableEntry_new(PhysPageNum ppn, uint8_t PTEFlags) {
    PageTableEntry entry;
    entry = (ppn << 10) | PTEFlags;
    return entry;
}

/* 把页表项置空，解除映射 */
PageTableEntry PageTableEntry_empty() {
    PageTableEntry entry;
    entry = 0;
    return entry;
}

/* 获取下级页表的物理页号 */
PhysPageNum PageTableEntry_ppn(PageTableEntry entry) {
    PhysPageNum ppn;
    ppn = (entry >> 10) & ((1ul << 44) - 1);
    return ppn;
}

/* 获取页表项的标志位 */
uint8_t PageTableEntry_flags(PageTableEntry entry) {
    return entry & 0xFF;
}

/* 判断页表项是否为空 */
bool PageTableEntry_is_valid(PageTableEntry entry) {
    uint8_t entryFlags = PageTableEntry_flags(entry);
    return (entryFlags & PTE_V) != 0;
}

uint8_t* get_bytes_array(PhysPageNum ppn)
{
    // 先从物理页号转换为物理地址
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    return (uint8_t*) addr;
}

PageTableEntry* get_pte_array(PhysPageNum ppn)
{
    // 先从物理页号转换为物理地址
    PhysAddr addr = phys_addr_from_phys_page_num(ppn);
    return (PageTableEntry*) addr;
}

//从虚拟地址之中取出三级页表的页内偏移   9+9+9+12=39
void get_page_offset(VirtPageNum vpn,uint64_t* page_offsets){
    for (int i = 2; i >= 0; i--)
    {
        page_offsets[i] =   vpn & ((1UL<<9)-1);
        vpn = vpn >> 9;
    }
}



/*去页表中查找，如果存在对应页表项则进入下一级，如果没有则新建*/
PageTableEntry* find_pte_create(PageTable* pt,VirtPageNum vpn){
    uint64_t idx[3];
    get_page_offset(vpn,&idx[0]);
    PhysPageNum ppn = pt->root_ppn;

    for (int i = 0; i < 3; i++)
    {
        PageTableEntry* pte_ptr = &get_pte_array(ppn)[idx[i]];
        if(i==2){//最后一级返回
            return pte_ptr;
        }
        if(!PageTableEntry_is_valid(*pte_ptr)){//页表项为空
            //分配内存页   返回页号
            PhysPageNum frame = StackFrameAllocator_alloc(&memory_allocator);
            //新建一个页表项  页号和10位标志位拼接得到 页表项
            *pte_ptr = PageTableEntry_new(frame,PTE_V);
            //压入页表的栈中  PageTable
            // push(&pt->frames,frame);

        }
        ppn = PageTableEntry_ppn(*pte_ptr);
    }
    
}


/*去页表中查找，如果存在对应页表项则进入下一级，如果没有也不新建*/
PageTableEntry* find_pte(PageTable* pt,VirtPageNum vpn){
    uint64_t idx[3];
    get_page_offset(vpn,&idx[0]);
    PhysPageNum ppn = pt->root_ppn;

    for (int i = 0; i < 3; i++)
    {
        PageTableEntry* pte_ptr = &get_pte_array(ppn)[idx[i]];
        if(!PageTableEntry_is_valid(*pte_ptr)){//页表项为空
            return NULL;
        }
        if(i==2){//最后一级返回
            return pte_ptr;
        }
        ppn = PageTableEntry_ppn(*pte_ptr);
    }
    
}

//将虚拟页表与物理页表对应
void PageTable_map_per_page(PageTable* pt,VirtPageNum vpn,PhysPageNum ppn, uint8_t pte_flags){
    PageTableEntry* pte_ptr = find_pte_create(pt,vpn);
    assert(!PageTableEntry_is_valid(*pte_ptr));
    *pte_ptr = PageTableEntry_new(ppn,PTE_V | pte_flags);
}


void PageTable_map(PageTable* pt,VirtAddr va,PhysAddr pa,uint64_t size,uint8_t pte_flags){
    if (size==0)
    {
        panic("page size is 0");
    }
    PhysPageNum ppn = floor_phys(pa);
    VirtPageNum vpn = floor_virts(va);
    uint64_t last_page =  (va + size -1)/PAGE_SIZE;
    do
    {
        PageTable_map_per_page(pt,vpn,ppn,pte_flags);
        vpn++;
        ppn++;
    } while (vpn<=last_page);

    
    
}

//将虚拟地址解除映射
void PageTable_unmap(PageTable* pt,VirtPageNum vpn){
    PageTableEntry* pte_ptr = find_pte(pt,vpn);
    assert(!PageTableEntry_is_valid(*pte_ptr));
    *pte_ptr = PageTableEntry_empty();

}

//初始化内存管理的范围，初始化回收栈
void frame_alloctor_init(){
    StackFrameAllocator_new(&memory_allocator,ceil_phys(phys_addr_from_size_t((uint64_t)kernelend)),ceil_phys(phys_addr_from_size_t(PHYSTOP)));
    printf("memony start:%p\n",kernelend);
    printf("memony end:%p\n",PHYSTOP);
}

/* 为每个应用程序映射内核栈,内核空间以及进行了映射 */
void proc_mapstacks(PageTable* kpgtbl)
{
  struct TaskControlBlock *p;
  
  for(p = tasks; p < &tasks[MAX_TASK_NUM]; p++) {
    char *pa = (char*)phys_addr_from_phys_page_num(StackFrameAllocator_alloc(&memory_allocator));
    if(pa == 0)
      panic("kalloc");
    u64 va = KSTACK((int) (p - tasks)); //两个相同类型的指针相减，为其数据类型的倍数
    PageTable_map(kpgtbl, virt_addr_from_size_t(va), phys_addr_from_size_t((u64)pa), \
                  PAGE_SIZE, PTE_R | PTE_W);
    // 给应用内核栈赋值  栈顶高位开始
    p->kstack = va +  PAGE_SIZE;
    printf("pa:%lx\n",pa);
  }
}

PhysPageNum kalloc(){
    return StackFrameAllocator_alloc(&memory_allocator);
}

//将内核的代码段和数据段映射到虚拟地址上  这里采用的是恒等映射  
PageTable kvm_make(){
    //存放根页表
    PhysPageNum root_ppn = StackFrameAllocator_alloc(&memory_allocator);
    kernel_pagetable.root_ppn = root_ppn;
    printf("root_ppn:%p\n",phys_addr_from_phys_page_num(root_ppn));
    printf("etext:%p\n",(uint64_t)etext);

    //内核数据段和代码段的恒等映射
    PageTable_map(&kernel_pagetable,virt_addr_from_size_t(KERNBASE),phys_addr_from_size_t(KERNBASE),(uint64_t)(etext-KERNBASE),PTE_R | PTE_X);
    printf("finish kernel text map\n");
    PageTable_map(&kernel_pagetable,virt_addr_from_size_t((uint64_t)etext),phys_addr_from_size_t((uint64_t)etext),(PHYSTOP-(u64)etext),PTE_R | PTE_W);
    printf("finish kernel data map\n");
    //跳板页的映射 映射到高地址的最后一页
    PageTable_map(&kernel_pagetable,virt_addr_from_size_t((uint64_t)TRAMPOLINE),phys_addr_from_size_t((uint64_t)trampoline),PAGE_SIZE,PTE_R | PTE_X);
    printf("finish trampoline map\n");
    //每个应用的内核栈的映射，紧跟着跳板页后面开始
    proc_mapstacks(&kernel_pagetable);
    printf("finish kernel stack map\n");

    return kernel_pagetable;
}





//建立内核的页表，映射内核代码、跳板页、各个应用的内核栈，然后开启虚拟地址
void kvminit()
{
    kernel_pagetable = kvm_make();
    PageTableEntry* pte_ptr = find_pte(&kernel_pagetable,virt_page_num_from_virt_addr(virt_addr_from_size_t(0xffffffffffffd000 )));
    printf("phy page:%p\n",( (*pte_ptr>>10) & ((1ul << 44) - 1) ));
    printf("vir page:%p\n",virt_page_num_from_virt_addr(virt_addr_from_size_t(0xffffffffffffd000) ));


    u64 kernel_satp = MAKE_SATP(kernel_pagetable.root_ppn);
    printf("kernel satp:%lx\n",kernel_satp);
    // wait for any previous writes to the page table memory to finish.
    sfence_vma();
    w_satp(kernel_satp);
    // flush stale entries from the TLB.
    sfence_vma();

}


void kfree(PhysPageNum ppn){
    StackFrameAllocator_dealloc(&memory_allocator,ppn);
}

//释放页空间，置空最后一级页表项
void page_unmap_free(PageTable* pt,VirtPageNum vpn,u64 num_pages,bool free){
    PageTableEntry* pte;
    for (VirtPageNum i = vpn; i < vpn+num_pages; i++)
    {
        pte = find_pte(pt,virt_page_num_from_size_t(i));
        if (pte != NULL)
        {
            if (free)
            {
                PhysPageNum ppn =  floor_phys(phys_addr_from_size_t(PTE_PA(*pte)));
                kfree(ppn);
            }
            //只制空最后一级
            *pte = PageTableEntry_empty();
        }
    }
}

//传入根页表物理页号，置空并回收整个三级页表
void free_page_table(PhysPageNum ppn){
    //递归查询 把页表中的页表项置零（如果有内容则会跳到下一级页表）
    //由于最后一级页表项已经被置零，所以最后一级页表为空，会被直接删除，然后返回
    //V=1 且 R/W/X 至少一个为 1 则为leaf page
    //V=1 且 R=W=X=0 则为中间页表项
    for (int i = 0; i < 512; i++)
    {
        PageTableEntry* pte = &get_pte_array(ppn)[i];
        //判断这是一个中间页表项
        if ((*pte & PTE_V) && (*pte & (PTE_R|PTE_W|PTE_X)) == 0 )
        {
            PhysPageNum child_ppn = PageTableEntry_ppn(*pte);
            free_page_table(child_ppn);
            *pte = PageTableEntry_empty();
        }
        else if(*pte & PTE_V){
            panic("free_page_table error\n");
        }
    }
    kfree(ppn);
}


//释放进程低半地址的占用内存
void exec_free_lowhalf(PageTable* pt,u64 sz){
    if (sz>0)
    {
        page_unmap_free(pt,virt_page_num_from_size_t(0),sz/PAGE_SIZE,1);
    }
    free_page_table(pt->root_ppn);
}

//释放进程内存
void free_proc_memory(PageTable* pt,u64 sz){
    //释放高地址内存空间 跳板页和trap页
    page_unmap_free(pt,floor_virts(virt_addr_from_size_t(TRAMPOLINE)),1,0);
    page_unmap_free(pt,floor_virts(virt_addr_from_size_t(TRAPFRAME)),1,0);
    
    exec_free_lowhalf(pt,sz);
}








