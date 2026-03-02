#include "user_print.h"
#include "user_syscall.h"

int main(){
    user_printf("in test.c\n");

    sys_exec("user_shell");
    while (1)
    {
        user_printf("in atest.c\n");
    }
    

    return 0;
}