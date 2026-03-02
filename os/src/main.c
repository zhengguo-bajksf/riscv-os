#include "os.h"

void os_main(){
    int a = 191002;
    printf("hello guo %d \n",a);

    // timer_init();
    // trap_context_init();
    // task_init();
    
    // start_task();
    // frame_allocator_test();

    frame_alloctor_init();
    kvminit();
    procinit();
    get_app_names();
    load_app(0);
    app_init(0);
    // load_app(1);
    // app_init(1);
    // load_app(2);
    // app_init(2);
    //防止在内核态发生trap  
    set_kernel_trap_entry();

    timer_init();
    

    start_task();

    
    // w_stvec((reg_t)__alltraps);

    // printf("num apps:%d\n",get_num_app());
    // printf("TRAMPOLINE:%p\n",TRAMPOLINE);
    

    


    while (1){
        printf("in os_main\n");
        // delay(10000);
    }

    

}

