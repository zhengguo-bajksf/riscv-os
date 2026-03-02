#include "user_syscall.h"
#include "user_print.h"

int main(){
    int i=0;
    while (1)
    {
        user_get_time();
        i++;
        if (i==5)
        {
            sys_exit(0);
        }
        
    }
    
    
    return 0;
}