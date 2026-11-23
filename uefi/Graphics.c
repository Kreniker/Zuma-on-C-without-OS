#include "EfiPlatform.h"
#include "include/SDL2/SDL_Image.h"

static SDL_Window window;
static SDL_Renderer renderer;
static uint32_t *display;
static int screenW,screenH,viewX,viewY,viewW,viewH;
static int mouseX=640,mouseY=360;
static uint32_t physicalButtons,mouseButtons;
static SDL_Event events[18];
static int eventCount,eventPos,pumped;
static const char *error="UEFI graphics or resource error";
void ZumaBeginGraphics(void);
int SDL_Init(unsigned flags){(void)flags;return zGop?0:-1;}
void SDL_Quit(void){}
const char *SDL_GetError(void){return error;}
int SDL_ShowSimpleMessageBox(unsigned flags,const char *title,const char *message,SDL_Window *win){
    (void)flags;(void)win;ZumaRestoreDisplay();ZumaPrint(title);ZumaPrint(": ");ZumaPrint(message);ZumaPrint("\n");return 0;
}
SDL_Window *SDL_CreateWindow(const char *title,int x,int y,int w,int h,unsigned flags){
    (void)title;(void)x;(void)y;(void)flags;ZumaBeginGraphics();
    UINT32 choice=zGop->Mode->Mode;uint64_t best=(uint64_t)-1;
    for(UINT32 i=0;i<zGop->Mode->MaxMode;i++) {
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info=NULL;UINTN size=0;
        if(EFI_ERROR(zGop->QueryMode(zGop,i,&size,&info)))continue;
        int dx=(int)info->HorizontalResolution-w,dy=(int)info->VerticalResolution-h;
        uint64_t score=(uint64_t)((int64_t)dx*dx+(int64_t)dy*dy);
        if(score<best){best=score;choice=i;}zBS->FreePool(info);
    }
    if(EFI_ERROR(zGop->SetMode(zGop,choice)))return NULL;
    screenW=(int)zGop->Mode->Info->HorizontalResolution;screenH=(int)zGop->Mode->Info->VerticalResolution;
    if(screenW<1 || screenH<1 || screenW>8192 || screenH>8192)return NULL;
    renderer.w=w;renderer.h=h;
    viewW=screenW;viewH=screenW*h/w;
    if(viewH>screenH){viewH=screenH;viewW=screenH*w/h;}
    viewX=(screenW-viewW)/2;viewY=(screenH-viewH)/2;
    return &window;
}
void SDL_DestroyWindow(SDL_Window *w){(void)w;}
void SDL_GetWindowSize(SDL_Window *w,int *x,int *y){(void)w;if(x)*x=renderer.w;if(y)*y=renderer.h;}
int SDL_SetWindowFullscreen(SDL_Window *w,unsigned flags){(void)w;(void)flags;return 0;}
int SDL_SetHint(const char *n,const char *v){(void)n;(void)v;return 1;}
SDL_Renderer *SDL_CreateRenderer(SDL_Window *w,int index,unsigned flags){
    (void)index;(void)flags;if(!w)return NULL;
    renderer.pixels=calloc((size_t)renderer.w*renderer.h,4);display=calloc((size_t)screenW*screenH,4);
    renderer.color=(SDL_Color){0,0,0,255};
    if(!renderer.pixels || !display){error="Not enough memory for frame buffers";return NULL;}return &renderer;
}
void SDL_DestroyRenderer(SDL_Renderer *r){if(r){free(r->pixels);r->pixels=NULL;}free(display);display=NULL;}
int SDL_RenderSetLogicalSize(SDL_Renderer *r,int w,int h){return r && r->w==w && r->h==h?0:-1;}
void SDL_RenderGetLogicalSize(SDL_Renderer *r,int *w,int *h){if(w)*w=r->w;if(h)*h=r->h;}
int SDL_SetRenderDrawColor(SDL_Renderer *r,uint8_t red,uint8_t green,uint8_t blue,uint8_t alpha){r->color=(SDL_Color){red,green,blue,alpha};return 0;}
int SDL_RenderClear(SDL_Renderer *r){uint32_t c=(r->color.r<<16)|(r->color.g<<8)|r->color.b;for(int i=0;i<r->w*r->h;i++)r->pixels[i]=c;return 0;}
static void pixel(SDL_Renderer *r,int x,int y,uint32_t rgb,unsigned alpha){
    if(x<0 || y<0 || x>=r->w || y>=r->h || !alpha)return;
    uint32_t *d=r->pixels+y*r->w+x;
    if(alpha==255){*d=rgb&0xffffff;return;}
    unsigned inv=255-alpha,red=(((rgb>>16)&255)*alpha+((*d>>16)&255)*inv)/255;
    unsigned green=(((rgb>>8)&255)*alpha+((*d>>8)&255)*inv)/255;
    unsigned blue=((rgb&255)*alpha+(*d&255)*inv)/255;
    *d=(red<<16)|(green<<8)|blue;
}
void SDL_RenderPresent(SDL_Renderer *r){
    /* Cursor is composed after the game; all rendering remains in logical coordinates. */
    for(int y=0;y<17;y++)for(int x=0;x<=y/2;x++)pixel(r,mouseX+x,mouseY+y,(x==0||x==y/2||y==16)?0:0xffffff,255);
    for(int y=0;y<viewH;y++){
        uint32_t *dst=display+(y+viewY)*screenW+viewX;
        uint32_t *src=r->pixels+(size_t)y*r->h/viewH*r->w;
        if(viewW==r->w)memcpy(dst,src,(size_t)viewW*4);
        else for(int x=0;x<viewW;x++)dst[x]=src[(size_t)x*r->w/viewW];
    }
    zGop->Blt(zGop,(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)display,EfiBltBufferToVideo,0,0,0,0,screenW,screenH,(UINTN)screenW*4);
    pumped=0;
}
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *s){
    (void)r;if(!s)return NULL;SDL_Texture *t=malloc(sizeof(*t));if(!t)return NULL;*t=*s;s->pixels=NULL;return t;
}
void SDL_DestroyTexture(SDL_Texture *t){if(t){free(t->pixels);free(t);}}
void SDL_FreeSurface(SDL_Surface *s){SDL_DestroyTexture(s);}
int SDL_QueryTexture(SDL_Texture *t,void *fmt,void *access,int *w,int *h){(void)fmt;(void)access;if(!t)return -1;if(w)*w=t->w;if(h)*h=t->h;return 0;}
int SDL_SetTextureColorMod(SDL_Texture *t,uint8_t r,uint8_t g,uint8_t b){if(!t)return -1;t->r=r;t->g=g;t->b=b;return 0;}
int SDL_SetTextureAlphaMod(SDL_Texture *t,uint8_t a){if(!t)return -1;t->a=a;return 0;}
static void sample(SDL_Renderer *r,SDL_Texture *t,int x,int y,int sx,int sy){
    if(sx<0||sy<0||sx>=t->w||sy>=t->h)return;
    uint32_t c=t->pixels[(size_t)sy*t->w+sx];unsigned a=(c>>24)*t->a/255;
    if(!a)return;
    if(t->r!=255||t->g!=255||t->b!=255)c=((((c>>16)&255)*t->r/255)<<16)|((((c>>8)&255)*t->g/255)<<8)|((c&255)*t->b/255);
    pixel(r,x,y,c,a);
}
int SDL_RenderCopyF(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *source,const SDL_FRect *dest){
    if(!r||!t)return -1;SDL_Rect s=source?*source:(SDL_Rect){0,0,t->w,t->h};
    SDL_FRect d=dest?*dest:(SDL_FRect){0,0,(float)r->w,(float)r->h};if(d.w<=0||d.h<=0)return 0;
    int x0=max(0,(int)d.x),y0=max(0,(int)d.y),x1=min(r->w,(int)(d.x+d.w+0.999f)),y1=min(r->h,(int)(d.y+d.h+0.999f));
    float xs=s.w/d.w,ys=s.h/d.h;
    for(int y=y0;y<y1;y++){int sy=s.y+(int)((y+0.5f-d.y)*ys);for(int x=x0;x<x1;x++){int sx=s.x+(int)((x+0.5f-d.x)*xs);if(sx>=s.x&&sx<s.x+s.w&&sy>=s.y&&sy<s.y+s.h)sample(r,t,x,y,sx,sy);}}
    return 0;
}
int SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst){
    if(!dst)return SDL_RenderCopyF(r,t,src,NULL);SDL_FRect d={(float)dst->x,(float)dst->y,(float)dst->w,(float)dst->h};return SDL_RenderCopyF(r,t,src,&d);
}
int SDL_RenderCopyExF(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *source,const SDL_FRect *dest,double angle,const SDL_FPoint *center,int flip){
    (void)flip;if(!angle)return SDL_RenderCopyF(r,t,source,dest);if(!r||!t||!dest||dest->w<=0||dest->h<=0)return -1;
    SDL_Rect s=source?*source:(SDL_Rect){0,0,t->w,t->h};SDL_FRect d=*dest;
    float px=center?center->x:d.w/2,py=center?center->y:d.h/2;
    float cx=d.x+px,cy=d.y+py,c=(float)cos(angle*M_PI/180),sn=(float)sin(angle*M_PI/180);
    float left=(float)r->w,top=(float)r->h,right=0,bottom=0;
    for(int i=0;i<4;i++){float x=(i&1?d.w:0)-px,y=(i&2?d.h:0)-py;float rx=c*x-sn*y+cx,ry=sn*x+c*y+cy;left=min(left,rx);right=max(right,rx);top=min(top,ry);bottom=max(bottom,ry);}
    int x0=max(0,(int)(left-1)),y0=max(0,(int)(top-1)),x1=min(r->w,(int)(right+1)),y1=min(r->h,(int)(bottom+1));
    for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++){
        float dx=x+0.5f-cx,dy=y+0.5f-cy,lx=c*dx+sn*dy+px,ly=-sn*dx+c*dy+py;
        if(lx>=0&&ly>=0&&lx<d.w&&ly<d.h)sample(r,t,x,y,s.x+(int)(lx*s.w/d.w),s.y+(int)(ly*s.h/d.h));
    }return 0;
}
int SDL_RenderDrawLineF(SDL_Renderer *r,float x1,float y1,float x2,float y2){
    int steps=(int)max(fabs(x2-x1),fabs(y2-y1));if(steps>32768)return -1;uint32_t rgb=(r->color.r<<16)|(r->color.g<<8)|r->color.b;
    if(!steps){pixel(r,(int)x1,(int)y1,rgb,r->color.a);return 0;}
    for(int i=0;i<=steps;i++)pixel(r,(int)(x1+(x2-x1)*i/steps),(int)(y1+(y2-y1)*i/steps),rgb,r->color.a);return 0;
}
int SDL_RenderDrawRectF(SDL_Renderer *r,const SDL_FRect *d){SDL_RenderDrawLineF(r,d->x,d->y,d->x+d->w,d->y);SDL_RenderDrawLineF(r,d->x,d->y,d->x,d->y+d->h);SDL_RenderDrawLineF(r,d->x+d->w,d->y,d->x+d->w,d->y+d->h);SDL_RenderDrawLineF(r,d->x,d->y+d->h,d->x+d->w,d->y+d->h);return 0;}
int SDL_RenderFillRect(SDL_Renderer *r,const SDL_Rect *d){uint32_t rgb=(r->color.r<<16)|(r->color.g<<8)|r->color.b;for(int y=max(0,d->y);y<min(r->h,d->y+d->h);y++)for(int x=max(0,d->x);x<min(r->w,d->x+d->w);x++)pixel(r,x,y,rgb,r->color.a);return 0;}
uint32_t SDL_GetTicks(void){return (uint32_t)ZumaMilliseconds();}
void SDL_Delay(uint32_t ms){zBS->Stall((UINTN)ms*1000);}
static void addEvent(uint32_t type,uint8_t button){if(eventCount<18){events[eventCount].type=type;events[eventCount++].button.button=button;}}
static void pumpInput(void){
    eventCount=eventPos=0;pumped=1;uint32_t old=mouseButtons;
    if(zPointer){EFI_SIMPLE_POINTER_STATE p;for(int i=0;i<32&&!EFI_ERROR(zPointer->GetState(zPointer,&p));i++){mouseX+=p.RelativeMovementX*2;mouseY+=p.RelativeMovementY*2;physicalButtons=(p.LeftButton?SDL_BUTTON(SDL_BUTTON_LEFT):0)|(p.RightButton?SDL_BUTTON(SDL_BUTTON_RIGHT):0);}}
    if(zAbsolute){EFI_ABSOLUTE_POINTER_STATE p;for(int i=0;i<32&&!EFI_ERROR(zAbsolute->GetState(zAbsolute,&p));i++){
        EFI_ABSOLUTE_POINTER_MODE *m=zAbsolute->Mode;
        if(m->AbsoluteMaxX>m->AbsoluteMinX && m->AbsoluteMaxY>m->AbsoluteMinY){
            uint64_t ax=p.CurrentX<m->AbsoluteMinX?0:p.CurrentX-m->AbsoluteMinX,ay=p.CurrentY<m->AbsoluteMinY?0:p.CurrentY-m->AbsoluteMinY;
            ax=min(ax,m->AbsoluteMaxX-m->AbsoluteMinX);ay=min(ay,m->AbsoluteMaxY-m->AbsoluteMinY);
            mouseX=((int)(ax*(screenW-1)/(m->AbsoluteMaxX-m->AbsoluteMinX))-viewX)*renderer.w/viewW;
            mouseY=((int)(ay*(screenH-1)/(m->AbsoluteMaxY-m->AbsoluteMinY))-viewY)*renderer.h/viewH;
        }
        physicalButtons=(p.ActiveButtons&1?SDL_BUTTON(SDL_BUTTON_LEFT):0)|(p.ActiveButtons&2?SDL_BUTTON(SDL_BUTTON_RIGHT):0);
    }}
    mouseButtons=physicalButtons;
    EFI_INPUT_KEY key;for(int i=0;i<16&&!EFI_ERROR(zST->ConIn->ReadKeyStroke(zST->ConIn,&key));i++){
        if (key.UnicodeChar >= '1' && key.UnicodeChar <= '6'
            && eventCount < 18) {
            SDL_Event *event = &events[eventCount++];
            *event = (SDL_Event){0};
            event->type = SDL_KEYDOWN;
            event->key.keysym.sym = key.UnicodeChar;
        }
        if(key.ScanCode==SCAN_ESC || key.UnicodeChar==27)addEvent(SDL_QUIT,0);
        if(key.ScanCode==SCAN_LEFT || key.UnicodeChar=='a')mouseX-=20;
        if(key.ScanCode==SCAN_RIGHT || key.UnicodeChar=='d')mouseX+=20;
        if(key.ScanCode==SCAN_UP || key.UnicodeChar=='w')mouseY-=20;
        if(key.ScanCode==SCAN_DOWN || key.UnicodeChar=='s')mouseY+=20;
        if(key.UnicodeChar==' ' || key.UnicodeChar==13)mouseButtons|=SDL_BUTTON(SDL_BUTTON_LEFT);
        if(key.UnicodeChar=='x' || key.UnicodeChar=='X')mouseButtons|=SDL_BUTTON(SDL_BUTTON_RIGHT);
    }
    mouseX=max(0,min(renderer.w-1,mouseX));mouseY=max(0,min(renderer.h-1,mouseY));
    if((mouseButtons&SDL_BUTTON(SDL_BUTTON_LEFT))&&!(old&SDL_BUTTON(SDL_BUTTON_LEFT)))addEvent(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_LEFT);
    if((mouseButtons&SDL_BUTTON(SDL_BUTTON_RIGHT))&&!(old&SDL_BUTTON(SDL_BUTTON_RIGHT)))addEvent(SDL_MOUSEBUTTONDOWN,SDL_BUTTON_RIGHT);
}
int SDL_PollEvent(SDL_Event *e){if(!pumped)pumpInput();if(eventPos==eventCount)return 0;*e=events[eventPos++];return 1;}
uint32_t SDL_GetMouseState(int *x,int *y){if(x)*x=mouseX;if(y)*y=mouseY;return mouseButtons;}
int IMG_Init(int flags){return flags;}
void IMG_Quit(void){}
const char *IMG_GetError(void){return error;}
SDL_Surface *IMG_Load(const char *path){
    /* Build script converts PNG/JPEG to a BGRA file; original paths stay intact. */
    char name[512];size_t n=strlen(path);if(n>500)return NULL;strcpy(name,path);strcpy(name+n,".ztex");
    FILE *f=fopen(name,"rb");if(!f){error="Missing .ztex resource. Run uefi/build.ps1 and copy the complete esp directory.";return NULL;}
    uint32_t header[3];if(fread(header,4,3,f)!=3 || header[0]!=0x58544d5a || !header[1] || !header[2] || header[1]>8192 || header[2]>8192){fclose(f);error="Invalid .ztex image";return NULL;}
    SDL_Surface *s=calloc(1,sizeof(*s));if(!s){fclose(f);return NULL;}
    s->w=(int)header[1];s->h=(int)header[2];s->r=s->g=s->b=s->a=255;
    size_t count=(size_t)s->w*s->h;s->pixels=malloc(count*4);
    if(!s->pixels || fread(s->pixels,4,count,f)!=count){fclose(f);SDL_FreeSurface(s);return NULL;}
    fclose(f);return s;
}
