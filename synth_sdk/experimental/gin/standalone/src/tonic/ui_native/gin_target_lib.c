/* Copyright (c) 2026 SquareWave Studio. SDK-derived portions (c) 2026 pcppcp.
 * GPL-3.0-or-later with OP1R additional permission; see LICENSE and NOTICE. */



#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
void *memcpy(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}
void *memset(void *d,int c,size_t n){unsigned char *a=d;while(n--)*a++=(unsigned char)c;return d;}
void *memchr(const void *s,int c,size_t n){const unsigned char *p=s;while(n--){if(*p==(unsigned char)c)return(void *)p;++p;}return 0;}
char *strcpy(char *d,const char *s){char *p=d;while((*p++=*s++)){}return d;}
int strcmp(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return (int)(unsigned char)*a-(int)(unsigned char)*b;}
static void put(char *out,size_t cap,size_t *n,char c){if(*n+1<cap)out[*n]=c;++*n;}
static void number(char *out,size_t cap,size_t *n,uint32_t v){char b[10];unsigned k=0;do{b[k++]=(char)('0'+v%10);v/=10;}while(v&&k<10);while(k)put(out,cap,n,b[--k]);}
int snprintf(char *out,size_t cap,const char *fmt,...){
 size_t n=0;va_list ap;va_start(ap,fmt);
 while(*fmt){int plus=0,precision=0;if(*fmt!='%'){put(out,cap,&n,*fmt++);continue;}++fmt;
  if(*fmt=='+'){plus=1;++fmt;}if(*fmt=='.'){++fmt;if(*fmt=='1')precision=1;if(*fmt=='0'||*fmt=='1')++fmt;}
  if(*fmt=='s'){const char *s=va_arg(ap,const char *);if(s)while(*s)put(out,cap,&n,*s++);}
  else if(*fmt=='f'){double x=va_arg(ap,double);uint32_t v;if(x<0){put(out,cap,&n,'-');x=-x;}else if(plus)put(out,cap,&n,'+');if(!(x>=0)||x>1000000)x=0;v=(uint32_t)(x*(precision?10:1)+.5);number(out,cap,&n,precision?v/10:v);if(precision){put(out,cap,&n,'.');put(out,cap,&n,(char)('0'+v%10));}}
  else if(*fmt=='u')number(out,cap,&n,va_arg(ap,unsigned));
  else if(*fmt=='d'){int v=va_arg(ap,int);if(v<0){put(out,cap,&n,'-');number(out,cap,&n,0u-(uint32_t)v);}else number(out,cap,&n,(uint32_t)v);}
  else if(*fmt=='%')put(out,cap,&n,'%');
  if(*fmt)++fmt;
 }
 va_end(ap);if(cap)out[n<cap?n:cap-1]=0;return(int)n;
}
