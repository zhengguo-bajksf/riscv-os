#ifndef __ADDRESS_H__
#define __ADDRESS_H__

#include "types.h"
#include "stack.h"

#define PAGE_SIZE 0x1000      // 4kb  一页的大小
#define PAGE_SIZE_BITS   0xc  // 12   页内偏移地址长度

#define PA_WIDTH_SV39 56      //物理地址长度
#define VA_WIDTH_SV39 39      //虚拟地址长度
#define PPN_WIDTH_SV39 (PA_WIDTH_SV39 - PAGE_SIZE_BITS)  // 物理页号 44位 [55:12]
#define VPN_WIDTH_SV39 (VA_WIDTH_SV39 - PAGE_SIZE_BITS)  // 虚拟页号 27位 [38:12]

#define MEMORY_END 0x80800000
#define MEMORY_START 0x80400000
extern uint8_t kernelend[];
extern uint8_t etext[];
extern uint8_t trampoline[];
//内核开始地址
#define KERNBASE 0x80200000L  
//物理地址结束地址
#define PHYSTOP (KERNBASE + 128*1024*1024)

typedef uint64_t PhysAddr;/* 物理地址 */
typedef uint64_t VirtAddr;/* 虚拟地址 */
typedef uint64_t PhysPageNum;/* 物理页号 */
typedef uint64_t VirtPageNum;/* 虚拟页号 */
typedef uint64_t PageTableEntry;/* 定义页表项 即页表中的内容，储存了下一级页表和属性标志位 */

// 定义位掩码常量
#define PTE_V (1 << 0)   //有效位
#define PTE_R (1 << 1)   //可读属性
#define PTE_W (1 << 2)   //可写属性
#define PTE_X (1 << 3)   //可执行属性
#define PTE_U (1 << 4)   //用户访问模式
#define PTE_G (1 << 5)   //全局映射
#define PTE_A (1 << 6)   //访问标志位
#define PTE_D (1 << 7)   //脏位

//Sv39 分页机制
#define SATP_SV39 (8L << 60)
//把stap寄存器的最高位置为1，开启SV39分页模式
#define MAKE_SATP(pagetable) (SATP_SV39 | (((u64)pagetable))) 
#define MAKE_PAGETABLE(stap) (stap & (SATP_SV39-1))
// Sv39的最大地址空间 512G
#define MAXVA (1L << (9 + 9 + 9 + 12))
// #define MAXVA (1L << (9 + 9 + 9 + 12 -1))

//跳板页开始位置
#define TRAMPOLINE ((MAXVA - PAGE_SIZE) | ~((1UL<<39)-1))
// #define TRAMPOLINE (MAXVA - PAGE_SIZE)

//用户空间中 Trap页开始位置
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)

//计算应用内核栈的地址，每个应用的内核栈下都有一个无效的守卫页
//刚好从跳板页的后面一页开始
#define KSTACK(p) (TRAMPOLINE - ((p)+1)* 2*PAGE_SIZE)


#define PGROUNDUP(sz)  (((sz)+PAGE_SIZE-1) & ~(PAGE_SIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PAGE_SIZE-1))

/* 内存分配器 */
typedef struct 
{
    uint64_t current;   //空闲内存的起始物理页号
    uint64_t  end;      //空闲内存的结束物理页号
    Stack recycled;     //储存回收页号的栈
}StackFrameAllocator;

//代表一个进程的页表
typedef struct
{
    PhysPageNum root_ppn;//页表根节点的页号
    // Stack frames;//页帧
}PageTable;

extern PageTable kernel_pagetable;

void frame_allocator_test();
void frame_alloctor_init();
void kvminit();
PhysPageNum kalloc();
/* 从物理页号转换为实际物理地址 */
PhysAddr phys_addr_from_phys_page_num(PhysPageNum ppn);
void PageTable_map(PageTable* pt,VirtAddr va,PhysAddr pa,uint64_t size,uint8_t pte_flags);
//将虚拟地址解除映射
void PageTable_unmap(PageTable* pt,VirtPageNum vpn);
//取低39位
VirtAddr virt_addr_from_size_t(uint64_t v);
//56位的物理地址 取低56位
PhysAddr phys_addr_from_size_t(uint64_t v);
// 从存储区 src 复制 n 个字节到存储区 dest。
void* memcpy(void *dest, const void *src, size_t count);
void* memset(void *ptr,int ch,uint64_t count);
/*去页表中查找，如果存在对应页表项则进入下一级，如果没有也不新建*/
PageTableEntry* find_pte(PageTable* pt,VirtPageNum vpn);

/* 把虚拟地址转换为虚拟页号 */
VirtPageNum virt_page_num_from_virt_addr(VirtAddr virt_addr);
/* 虚拟地址向下取整 */
VirtPageNum floor_virts(VirtAddr virt_addr);
//释放进程内存
void free_proc_memory(PageTable* pt,u64 sz);

//释放页空间，置空最后一级页表项
void page_unmap_free(PageTable* pt,VirtPageNum vpn,u64 num_pages,bool free);
//释放进程低半地址的占用内存
void exec_free_lowhalf(PageTable* pt,u64 sz);

#endif