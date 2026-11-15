#include "include/Runtime.h"
#include "include/expat.h"
/* Handles the bundled levels.xml: elements, quoted attributes, comments,
   processing instructions and the five predefined entities. DTDs are rejected. */
struct ZumaXml {
    char *data; size_t size; int error;
    void (*start)(void *,const char *,const char **);
    void (*end)(void *,const char *);
};
XML_Parser XML_ParserCreate(const char *encoding){(void)encoding;return calloc(1,sizeof(struct ZumaXml));}
void XML_SetElementHandler(XML_Parser p,void (*start)(void *,const char *,const char **),void (*end)(void *,const char *)){if(p){p->start=start;p->end=end;}}
int XML_GetErrorCode(XML_Parser p){return p?p->error:1;}
const char *XML_ErrorString(int code){(void)code;return "Invalid or unsupported resource XML";}
void XML_ParserFree(XML_Parser p){if(p){free(p->data);free(p);}}
static int white(char c){return c==' '||c=='\t'||c=='\r'||c=='\n';}
static int nameChar(char c){return(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c==':';}
static char *after(char *p,const char *end){size_t n=strlen(end);for(;*p;p++)if(!strncmp(p,end,n))return p+n;return NULL;}
static int entities(char *p){
    char *out=p;
    while(*p){
        if(*p!='&'){*out++=*p++;continue;}
        const char *names[]={"&amp;","&lt;","&gt;","&quot;","&apos;"};const char values[]="&<>\"'";int i;
        for(i=0;i<5;i++)if(!strncmp(p,names[i],strlen(names[i]))){*out++=values[i];p+=strlen(names[i]);break;}
        if(i==5)return 0;
    }*out=0;return 1;
}
static int parse(XML_Parser parser){
    char *p=parser->data;char stack[32][128];int depth=0,roots=0;
    while(p&&*p){
        if(white(*p)){++p;continue;}
        if(*p!='<')return 0;
        if(!strncmp(p,"<!--",4)){p=after(p+4,"-->");continue;}
        if(!strncmp(p,"<?",2)){p=after(p+2,"?>");continue;}
        ++p;int closing=(*p=='/');if(closing)++p;
        char *name=p;while(nameChar(*p))++p;
        if(p==name || p-name>=128)return 0;
        char delimiter=*p; if(!delimiter)return 0; *p++=0;
        if(closing){
            if(delimiter!='>'){if(!white(delimiter))return 0;while(white(*p))++p;if(*p++!='>')return 0;}
            if(!depth || strcmp(stack[depth-1],name))return 0;
            --depth;if(parser->end)parser->end(NULL,name);continue;
        }
        const char *attributes[129];int count=0,selfClosing=0;
        if(delimiter=='/'){if(*p++!='>')return 0;selfClosing=1;}
        else if(delimiter!='>'){
            if(!white(delimiter))return 0;
            while(1){
                while(white(*p))++p;
                if(*p=='>'){++p;break;}
                if(*p=='/'&&p[1]=='>'){p+=2;selfClosing=1;break;}
                if(count>=126)return 0;
                char *attr=p;while(nameChar(*p))++p;if(p==attr)return 0;
                char delim=*p; if(!delim)return 0;*p++=0;
                if(delim!='='){if(!white(delim))return 0;while(white(*p))++p;if(*p++!='=')return 0;}
                while(white(*p))++p;char quote=*p++;if(quote!='\''&&quote!='"')return 0;
                char *value=p;while(*p&&*p!=quote)++p;if(!*p)return 0;*p++=0;
                if(!entities(value))return 0;
                attributes[count++]=attr;attributes[count++]=value;
            }
        }
        attributes[count]=NULL;
        if(!depth && ++roots!=1)return 0;
        if(depth>=32)return 0;strcpy(stack[depth++],name);
        if(parser->start)parser->start(NULL,name,attributes);
        if(selfClosing){--depth;if(parser->end)parser->end(NULL,name);}
    }
    return p && !depth && roots==1;
}
enum XML_Status XML_Parse(XML_Parser p,const char *data,int length,int final){
    if(!p || length<0)return XML_STATUS_ERROR;
    if(p->error || p->size+(size_t)length>1024*1024){p->error=1;return XML_STATUS_ERROR;}
    char *buffer=realloc(p->data,p->size+(size_t)length+1);
    if(!buffer){p->error=1;return XML_STATUS_ERROR;}
    p->data=buffer;memcpy(buffer+p->size,data,(size_t)length);p->size+=(size_t)length;buffer[p->size]=0;
    if(final && !parse(p)){p->error=1;return XML_STATUS_ERROR;}
    return XML_STATUS_OK;
}
