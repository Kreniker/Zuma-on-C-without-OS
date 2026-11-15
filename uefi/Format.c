#include "include/Runtime.h"

static char *number(char *p,uint64_t n) {char tmp[24];int i=0;do{tmp[i++]=(char)('0'+n%10);n/=10;}while(n);while(i)*p++=tmp[--i];return p;}
static int formatText(char *out,const char *fmt,va_list args) {
    char *p=out;
    while(*fmt) {
        if(*fmt!='%'){*p++=*fmt++;continue;} ++fmt;
        int width=0,zero=0;
        if(*fmt=='0'){zero=1;++fmt;}
        while(*fmt>='0'&&*fmt<='9'){int digit=*fmt++-'0';width=min(128,width*10+digit);}
        int precision=6; if(*fmt=='.'){++fmt;precision=atoi(fmt);while(*fmt>='0'&&*fmt<='9')++fmt;precision=min(precision,9);}
        int lng=0; if(*fmt=='l'){lng=1;++fmt;}
        if(!*fmt)break;
        char type=*fmt++;char *start=p;
        if(type=='s') {const char *s=va_arg(args,const char *);if(!s)s="(null)";while(*s)*p++=*s++;}
        else if(type=='c')*p++=(char)va_arg(args,int);
        else if(type=='d' || type=='i') {int64_t n=lng?va_arg(args,long):va_arg(args,int); if(n<0){*p++='-';n=-n;} p=number(p,(uint64_t)n);}
        else if(type=='u')p=number(p,lng?va_arg(args,unsigned long):va_arg(args,unsigned int));
        else if(type=='f' || type=='g') {double v=va_arg(args,double);if(v<0){*p++='-';v=-v;} if(v>1e15 || v!=v){*p++='?';continue;} uint64_t n=(uint64_t)v;p=number(p,n);if(precision){*p++='.';v-=n;for(int i=0;i<precision;i++){v*=10;int d=(int)v;*p++=(char)('0'+d);v-=d;}}}
        else if(type=='%')*p++='%';
        int length=(int)(p-start),padding=width-length;
        if(padding>0){
            if(zero && *start=='-'){memmove(start+1+padding,start+1,length-1);memset(start+1,'0',padding);}
            else {memmove(start+padding,start,length);memset(start,zero?'0':' ',padding);}
            p+=padding;
        }
    }
    *p=0;return (int)(p-out);
}
int sprintf(char *s,const char *fmt,...) {va_list a;va_start(a,fmt);int n=formatText(s,fmt,a);va_end(a);return n;}
int fprintf(FILE *f,const char *fmt,...) {char s[1024];va_list a;va_start(a,fmt);int n=formatText(s,fmt,a);va_end(a);return fwrite(s,1,n,f)==(size_t)n?n:EOF;}
