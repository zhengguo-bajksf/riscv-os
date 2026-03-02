#ifndef __USER_PRINT_H__
#define __USER_PRINT_H__

/* 文件描述符 */
typedef enum std_fd_t
{
    stdin,  
    stdout,
    stderr,
} std_fd_t;
int user_printf(const char* s, ...);
//在dest后面拼接一个字符串 规定数量的字符
void strncat(char *dest, const char *src, int n);
//计算字符串的长度 
int user_strlen(const char *str);
#endif