#ifndef __PRINTF_H__
#define __PRINTF_H__

int printf(const char* s, ...);
void uart_puts(char *s);
void panic(char *s);
int sys_printf(const char* s, ...);
size_t strlen(const char *str);
int strcmp(const char* lstr,const char* rstr);
#endif