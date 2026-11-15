#include "EfiPlatform.h"

int _fltused = 1;
typedef struct Allocation {
    struct Allocation *prev, *next;
    size_t size, padding;
} Allocation;
static Allocation *allocations;

void *malloc(size_t n) {
    Allocation *a = NULL;
    if (!n) n = 1;
    if (n > (size_t)-1 - sizeof(*a)) return NULL;
    if (EFI_ERROR(zBS->AllocatePool(EfiLoaderData, n + sizeof(*a), (void **)&a))) return NULL;
    a->size = n; a->prev = NULL; a->next = allocations;
    if (allocations) allocations->prev = a;
    allocations = a;
    return a + 1;
}
void free(void *p) {
    if (!p) return;
    Allocation *a = (Allocation *)p - 1;
    if (a->prev) a->prev->next = a->next; else allocations = a->next;
    if (a->next) a->next->prev = a->prev;
    zBS->FreePool(a);
}
void *calloc(size_t n, size_t size) {
    if (size && n > (size_t)-1 / size) return NULL;
    void *p = malloc(n * size);
    if (p) memset(p, 0, n * size);
    return p;
}
void *realloc(void *p, size_t n) {
    if (!p) return malloc(n);
    if (!n) { free(p); return NULL; }
    void *q = malloc(n);
    if (q) { size_t old = ((Allocation *)p - 1)->size; memcpy(q,p,min(n,old)); free(p); }
    return q;
}
void ZumaFreeAllocations(void) {
    while (allocations) free(allocations + 1);
}
#pragma function(memcpy, memset, memcmp, strlen, strcpy, strcmp)
void *memcpy(void *d,const void *s,size_t n) { unsigned char *a=d; const unsigned char *b=s; while(n--) *a++=*b++; return d; }
void *memset(void *d,int c,size_t n) { unsigned char *a=d; while(n--) *a++=(unsigned char)c; return d; }
void *memmove(void *d,const void *s,size_t n) {
    unsigned char *a=d; const unsigned char *b=s;
    if (a<b) return memcpy(d,s,n);
    while(n) { --n; a[n]=b[n]; } return d;
}
int memcmp(const void *a,const void *b,size_t n) { const unsigned char *x=a,*y=b; while(n--) { if(*x!=*y) return *x-*y; ++x;++y; } return 0; }
size_t strlen(const char *s) { const char *p=s; while(*p) ++p; return (size_t)(p-s); }
char *strcpy(char *d,const char *s) { char *p=d; while((*p++=*s++)!=0) {} return d; }
int strcmp(const char *a,const char *b) { while(*a && *a==*b) {++a;++b;} return (unsigned char)*a-(unsigned char)*b; }
int strncmp(const char *a,const char *b,size_t n) { while(n--) { if(*a!=*b || !*a) return (unsigned char)*a-(unsigned char)*b; ++a;++b; } return 0; }
char *strchr(const char *s,int c) { do { if(*s==c) return (char *)s; } while(*s++); return NULL; }
char *strtok(char *s,const char *delim) {
    static char *next;
    if(s) next=s;
    if(!next) return NULL;
    while(*next && strchr(delim,*next)) ++next;
    if(!*next) { next=NULL; return NULL; }
    char *start=next;
    while(*next && !strchr(delim,*next)) ++next;
    if(*next) *next++=0; else next=NULL;
    return start;
}
static int space(int c) { return c==' ' || c=='\t' || c=='\r' || c=='\n'; }
int atoi(const char *s) {
    while(space(*s)) ++s; int sign=1, n=0;
    if(*s=='-' || *s=='+') { if(*s=='-') sign=-1; ++s; }
    while(*s>='0' && *s<='9') n=n*10+(*s++-'0'); return n*sign;
}
double atof(const char *s) {
    while(space(*s)) ++s; double sign=1,n=0,div=1;
    if(*s=='-' || *s=='+') { if(*s=='-') sign=-1; ++s; }
    while(*s>='0' && *s<='9') n=n*10+(*s++-'0');
    if(*s=='.') { ++s; while(*s>='0' && *s<='9') { div*=10; n+=(*s++-'0')/div; } }
    if(*s=='e' || *s=='E') { int exp=atoi(s+1); if(exp>308) exp=308; if(exp< -308) exp=-308; while(exp>0){n*=10;--exp;} while(exp<0){n/=10;++exp;} }
    return n*sign;
}
static unsigned randomState=1;
void srand(unsigned seed) { randomState=seed?seed:1; }
int rand(void) { randomState=randomState*1664525u+1013904223u; return (int)((randomState>>1)&0x7fffffff); }
clock_t clock(void) { return (clock_t)ZumaMilliseconds(); }
time_t time(time_t *out) {
    EFI_TIME now;time_t t=(time_t)(ZumaMilliseconds()/1000);
    if(!EFI_ERROR(zST->RuntimeServices->GetTime(&now,NULL)) && now.Year>=1970 && now.Month>=1 && now.Month<=12) {
        static const int daysBeforeMonth[]={0,31,59,90,120,151,181,212,243,273,304,334};
        int64_t days=0;for(int y=1970;y<now.Year;y++)days+=365+(y%4==0&&(y%100!=0||y%400==0));
        days+=daysBeforeMonth[now.Month-1]+now.Day-1;
        if(now.Month>2 && now.Year%4==0 && (now.Year%100!=0||now.Year%400==0))++days;
        t=((days*24+now.Hour)*60+now.Minute)*60+now.Second;
    }
    if(out)*out=t;return t;
}
_Noreturn void exit(int code) {
    zExitCode=code;
    ZumaRestoreDisplay();
    if(zRoot) { zRoot->Close(zRoot); zRoot=NULL; }
    ZumaFreeAllocations();
    zBS->Exit(zImage,code?EFI_ABORTED:EFI_SUCCESS,0,NULL);
    for(;;) {}
}

/* Buffered files: the curve reader makes many single-byte reads. */
struct ZumaFile { EFI_FILE_PROTOCOL *handle; unsigned char *data; size_t size,pos,capacity; int write; };
static EFI_GUID fileInfoGuid=EFI_FILE_INFO_ID;
FILE *fopen(const char *path,const char *mode) {
    CHAR16 name[768]; size_t n=0;
    while(zDirectory[n] && n<512) { name[n]=zDirectory[n]; ++n; }
    while(*path && n<767) { char c=*path++; name[n++]=(CHAR16)(c=='/'?'\\':(unsigned char)c); }
    name[n]=0; if(*path || !zRoot) return NULL;
    EFI_FILE_PROTOCOL *h=NULL; int write=mode[0]=='w';
    UINT64 flags=write?(EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE):EFI_FILE_MODE_READ;
    if(EFI_ERROR(zRoot->Open(zRoot,&h,name,flags,0))) return NULL;
    UINTN infoSize=0; EFI_FILE_INFO *info=NULL;
    EFI_STATUS status=h->GetInfo(h,&fileInfoGuid,&infoSize,NULL);
    if(status!=EFI_BUFFER_TOO_SMALL || !(info=malloc(infoSize))) {h->Close(h);return NULL;}
    status=h->GetInfo(h,&fileInfoGuid,&infoSize,info);
    if(EFI_ERROR(status) || (info->Attribute&EFI_FILE_DIRECTORY) || info->FileSize>256u*1024u*1024u) {free(info);h->Close(h);return NULL;}
    size_t length=(size_t)info->FileSize;
    if(write) { info->FileSize=0; status=h->SetInfo(h,&fileInfoGuid,infoSize,info); length=0; }
    free(info);
    if(EFI_ERROR(status)) {h->Close(h);return NULL;}
    FILE *f=calloc(1,sizeof(*f));
    if(!f) {h->Close(h);return NULL;}
    f->handle=h; f->write=write; f->capacity=write?1024:length+1; f->size=length;
    f->data=malloc(f->capacity);
    if(!f->data) {h->Close(h);free(f);return NULL;}
    if(!write) {
        UINTN count=length;
        status=h->Read(h,&count,f->data);
        if(EFI_ERROR(status) || count!=length) {fclose(f);return NULL;}
        f->data[length]=0;
        h->Close(h); f->handle=NULL;
    }
    return f;
}
int fclose(FILE *f) {
    if(!f) return EOF; int error=0;
    if(f->handle) {
        if(f->write) {UINTN n=f->size; if(EFI_ERROR(f->handle->Write(f->handle,&n,f->data)) || n!=f->size)error=EOF; if(EFI_ERROR(f->handle->Flush(f->handle)))error=EOF;}
        if(EFI_ERROR(f->handle->Close(f->handle)))error=EOF;
    }
    free(f->data); free(f); return error;
}
size_t fread(void *p,size_t size,size_t n,FILE *f) {
    if(!f || !size || !n || f->pos>f->size) return 0;
    n=min(n,(f->size-f->pos)/size); size_t bytes=n*size;
    memcpy(p,f->data+f->pos,bytes); f->pos+=bytes; return n;
}
size_t fwrite(const void *p,size_t size,size_t n,FILE *f) {
    if(!f || !f->write || !size || n>((size_t)-1-f->pos)/size) return 0;
    size_t end=f->pos+size*n;
    if(end>f->capacity) { unsigned char *q=realloc(f->data,end+1024); if(!q)return 0; f->data=q; f->capacity=end+1024; }
    memcpy(f->data+f->pos,p,size*n); f->pos=end; f->size=max(end,f->size); return n;
}
int fseek(FILE *f,long offset,int origin) {
    if(!f)return -1; int64_t p=(origin==SEEK_CUR?(int64_t)f->pos:origin==SEEK_END?(int64_t)f->size:0)+offset;
    if(p<0 || (uint64_t)p>f->size) return -1; f->pos=(size_t)p; return 0;
}
char *fgets(char *s,int n,FILE *f) { int i=0; if(n<1 || !f || f->pos>=f->size)return NULL; while(i<n-1 && f->pos<f->size) { s[i++]=(char)f->data[f->pos++]; if(s[i-1]=='\n')break; } s[i]=0; return s; }
int fputs(const char *s,FILE *f) { size_t n=strlen(s); return fwrite(s,1,n,f)==n?0:EOF; }
int fscanf(FILE *f,const char *fmt,...) {
    if(!f || f->pos>=f->size)return EOF;
    int matched=0; va_list args; va_start(args,fmt);
    while(*fmt) {
        if(space(*fmt)) {while(f->pos<f->size && space(f->data[f->pos]))++f->pos; ++fmt;continue;}
        if(*fmt++!='%')break;
        int width=0; while(*fmt>='0' && *fmt<='9')width=width*10+(*fmt++-'0');
        char type=*fmt++; if(type!='c')while(f->pos<f->size && space(f->data[f->pos]))++f->pos;
        if(f->pos>=f->size)break;
        if(type=='s') {char *p=va_arg(args,char *); int i=0; while(f->pos<f->size && !space(f->data[f->pos]) && (!width || i<width))p[i++]=(char)f->data[f->pos++]; p[i]=0; ++matched;}
        else if(type=='c') {*va_arg(args,char *)=(char)f->data[f->pos++]; ++matched;}
        else if(type=='d' || type=='u') {
            size_t start=f->pos; if(f->data[f->pos]=='-' || f->data[f->pos]=='+')++f->pos;
            size_t digits=f->pos; while(f->pos<f->size && f->data[f->pos]>='0' && f->data[f->pos]<='9')++f->pos;
            if(digits==f->pos){f->pos=start;break;}
            *va_arg(args,int *)=atoi((char *)f->data+start); ++matched;
        } else break;
    }
    va_end(args); return !matched && f->pos>=f->size ? EOF : matched;
}
double fabs(double x){return x<0?-x:x;}
double fmin(double x,double y){return x<y?x:y;}
double sqrt(double x) {
    if(x<=0)return 0;
    /* Exponent-based initial estimate, followed by Newton iterations. */
    union {double d; uint64_t u;} v;v.d=x;v.u=(v.u>>1)+0x1ff8000000000000ULL;
    double r=v.d;for(int i=0;i<6;i++)r=(r+x/r)*0.5;return r;
}
double sin(double x) {
    while(x>M_PI)x-=2*M_PI;while(x< -M_PI)x+=2*M_PI;
    if(x>M_PI/2)x=M_PI-x;if(x< -M_PI/2)x=-M_PI-x;
    double x2=x*x;return x*(1+x2*(-1.0/6+x2*(1.0/120+x2*(-1.0/5040+x2*(1.0/362880+x2*(-1.0/39916800))))));
}
double cos(double x){return sin(x+M_PI/2);}
static double arctan(double x) {
    int inverse=x>1;if(inverse)x=1/x;
    int reduce=x>0.414213562373095; if(reduce)x=(x-1)/(x+1);
    double sum=x,term=x,xx=x*x;for(int i=3;i<=31;i+=2){term*=-xx;sum+=term/i;}
    if(reduce)sum+=M_PI/4;return inverse?M_PI/2-sum:sum;
}
double atan2(double y,double x) {if(x==0)return y>0?M_PI/2:y<0?-M_PI/2:0;double a=arctan(fabs(y/x));if(x<0)a=M_PI-a;return y<0?-a:a;}
float modff(float x,float *whole){*whole=(float)(int64_t)x;return x-*whole;}
