#include "user_print.h"
#include "user_syscall.h"

#define LF 0x0a     
#define CR 0x0d   //enter
#define DL 0x7f   
#define BS 0x08   // backspace
#define BUFFER_SIZE 1024
int main()
{
    user_printf("User shell\n");
    user_printf(">> ");
    char line[BUFFER_SIZE];
    user_set_trigger();
    while (1)
    {
        char c = user_getchar();
        switch (c)
        {
        case CR://回车键
            user_printf("\n");
            if(user_strlen(line) > 0)
            {
                line[user_strlen(line)] = '\0';
                int pid = user_fork();
                user_printf("pid:%d\n",pid);
                if(pid==0)//子进程
                {
                    sys_exec(line);
                } else if(pid > 0){
                    //父进程继续执行
                    int status = user_recycle();
                    user_printf("child proc exit:%d\n", status);
                }
                line[0] = '\0';
                user_printf(">> ");
            }
            else{
                line[0] = '\0';
                user_printf(">> ");
            }
            break;
        case BS://退格键
            if (user_strlen(line) > 0) 
            {
                user_printf("\b \b");
                line[user_strlen(line) - 1] = '\0';
            }
            break;
        default://默认输入  打印输入的字符 显示在终端
            user_printf("%c",c);
            strncat(line,(char*)&c,1);
            break;
        }
    }
    
    return 0;
}