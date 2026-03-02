#include "os.h"


/* 设置下次时钟中断的cnt值 */
void set_next_trigger()
{
    sbi_set_timer(r_mtime() + CLOCK_FREQ / TICKS_PER_SEC);
}

/* 开启S模式下的时钟中断 */
void timer_init()
{
   //开启S模式下的中断  SIE位
   reg_t sstatus = r_sstatus();
   sstatus |= (1L<<1);
   w_sstatus(sstatus);
   //开启时钟中断
   reg_t sie = r_sie();
   sie |= SIE_STIE;
   w_sie(sie);

}


/* 以us为单位返回时间 */
uint64_t get_time_us()
{
    reg_t time =  r_mtime() / (CLOCK_FREQ / TICKS_PER_SEC);
    return time;
}

