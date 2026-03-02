#include "user_print.h"
#include "user_syscall.h"

int main(){

    user_printf("hello! user guo\n");
    delay(5000);
    int i=0;
    int pid = user_fork();
    user_set_trigger();
    while (1)
    {
        if(pid>0){
            user_printf("father\n");
            int ret =  user_recycle();
            user_printf("child proc exit:%d\n",ret);
            break;
        }
        else if(pid==0){
            i++;
            if (i>5)
            {
                sys_exit(0);
            }
            user_printf("child\n");
        }
        else{
            user_printf("failed\n");
        }
        delay(5000);

    }

    sys_exit(0);

    while (1)
    {
        // user_printf("out\n");
        // delay(5000);
    }
    

    return 0;
}