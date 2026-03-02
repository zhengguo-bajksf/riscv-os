#include "types.h"
#include "user_syscall.h"
#include <stdarg.h>

static int _vsnprintf(char * out, size_t n, const char* s, va_list vl)
{
	int format = 0;
	int longarg = 0;
	size_t pos = 0;
	for (; *s; s++) {
		if (format) {
			switch(*s) {
			case 'l': {
				longarg = 1;
				break;
			}
			case 'p': {
				longarg = 1;
				if (out && pos < n) {
					out[pos] = '0';
				}
				pos++;
				if (out && pos < n) {
					out[pos] = 'x';
				}
				pos++;
			}
			case 'x': {
				long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				int hexdigits = 2*(longarg ? sizeof(long) : sizeof(int))-1;
				for(int i = hexdigits; i >= 0; i--) {
					int d = (num >> (4*i)) & 0xF;
					if (out && pos < n) {
						out[pos] = (d < 10 ? '0'+d : 'a'+d-10);
					}
					pos++;
				}
				longarg = 0;
				format = 0;
				break;
			}
			case 'd': {
				long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				if (num < 0) {
					num = -num;
					if (out && pos < n) {
						out[pos] = '-';
					}
					pos++;
				}
				long digits = 1;
				for (long nn = num; nn /= 10; digits++);
				for (int i = digits-1; i >= 0; i--) {
					if (out && pos + i < n) {
						out[pos + i] = '0' + (num % 10);
					}
					num /= 10;
				}
				pos += digits;
				longarg = 0;
				format = 0;
				break;
			}
			case 's': {
				const char* s2 = va_arg(vl, const char*);
				while (*s2) {
					if (out && pos < n) {
						out[pos] = *s2;
					}
					pos++;
					s2++;
				}
				longarg = 0;
				format = 0;
				break;
			}
			case 'c': {
				if (out && pos < n) {
					out[pos] = (char)va_arg(vl,int);
				}
				pos++;
				longarg = 0;
				format = 0;
				break;
			}
			default:
				break;
			}
		} else if (*s == '%') {
			format = 1;
		} else {
			if (out && pos < n) {
				out[pos] = *s;
			}
			pos++;
		}
    	}
	if (out && pos < n) {
		out[pos] = 0;
	} else if (out && n) {
		out[n-1] = 0;
	}
	return pos;
}



/*  用户态打印  */
static char user_print_buf[1000]; // buffer for _vprintf()

static int _user_vprintf(const char* s, va_list vl)
{
	int res = _vsnprintf(NULL, -1, s, vl);
	if (res+1 >= sizeof(user_print_buf)) {
		user_syscall(__SYS_PRINTF,(reg_t)"error: output string size overflow\n",0,0);
		while(1) {}
	}
	_vsnprintf(user_print_buf, res + 1, s, vl);
	user_syscall(__SYS_PRINTF,(reg_t)user_print_buf,0,0);
	return res;
}
int user_printf(const char* s, ...)
{
	int res = 0;
	va_list vl;
	va_start(vl, s);
	res = _user_vprintf(s, vl);
	va_end(vl);
	return res;
}

//在dest后面拼接一个字符串 规定数量的字符
void strncat(char *dest, const char *src, int n) {
    while (*dest) {//循环到'\0'
        dest++;
    }
    while (n > 0 && *src) {
        *dest++ = *src++;
        n--;
    }
    *dest = '\0';
}

//计算字符串的长度 
int user_strlen(const char *str)
{
    char *ptr = (char *)str;
    while (*ptr != EOS)
    {
        ptr++;
    }
    return ptr - str;
}

