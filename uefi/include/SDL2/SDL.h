#ifndef ZUMA_SDL_H
#define ZUMA_SDL_H
#include "Runtime.h"
/* The subset of SDL used by this game, implemented with UEFI protocols. */
typedef struct { int unused; } SDL_Window;
typedef struct { uint8_t r,g,b,a; } SDL_Color;
typedef struct { int x,y; } SDL_Point;
typedef struct { float x,y; } SDL_FPoint;
typedef struct { int x,y,w,h; } SDL_Rect;
typedef struct { float x,y,w,h; } SDL_FRect;
typedef struct { int w,h; uint32_t *pixels; uint8_t r,g,b,a; } SDL_Texture;
typedef SDL_Texture SDL_Surface;
typedef struct { int w,h; SDL_Color color; uint32_t *pixels; } SDL_Renderer;
typedef struct {
    uint32_t type;
    struct { uint8_t button; } button;
    struct {
        struct { int sym; } keysym;
    } key;
} SDL_Event;

#define SDL_INIT_VIDEO 1
#define SDL_WINDOWPOS_UNDEFINED 0
#define SDL_WINDOW_OPENGL 0
#define SDL_WINDOW_FULLSCREEN 1
#define SDL_RENDERER_ACCELERATED 0
#define SDL_RENDERER_PRESENTVSYNC 0
#define SDL_HINT_RENDER_SCALE_QUALITY ""
#define SDL_MESSAGEBOX_ERROR 0
#define SDL_QUIT 0x100
#define SDL_KEYDOWN 0x300
#define SDL_MOUSEBUTTONDOWN 0x401
#define SDL_BUTTON_LEFT 1
#define SDL_BUTTON_RIGHT 3
#define SDL_BUTTON(x) (1u << ((x)-1))
#define SDL_FLIP_NONE 0
int SDL_Init(unsigned flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
int SDL_ShowSimpleMessageBox(unsigned flags,const char *title,const char *message,SDL_Window *win);
SDL_Window *SDL_CreateWindow(const char *title,int x,int y,int w,int h,unsigned flags);
void SDL_DestroyWindow(SDL_Window *win);
void SDL_GetWindowSize(SDL_Window *win,int *w,int *h);
int SDL_SetWindowFullscreen(SDL_Window *win,unsigned flags);
int SDL_SetHint(const char *name,const char *value);
SDL_Renderer *SDL_CreateRenderer(SDL_Window *win,int index,unsigned flags);
void SDL_DestroyRenderer(SDL_Renderer *r);
int SDL_RenderSetLogicalSize(SDL_Renderer *r,int w,int h);
void SDL_RenderGetLogicalSize(SDL_Renderer *r,int *w,int *h);
int SDL_SetRenderDrawColor(SDL_Renderer *r,uint8_t red,uint8_t green,uint8_t blue,uint8_t alpha);
int SDL_RenderClear(SDL_Renderer *r);
void SDL_RenderPresent(SDL_Renderer *r);
SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *r,SDL_Surface *surface);
void SDL_FreeSurface(SDL_Surface *surface);
void SDL_DestroyTexture(SDL_Texture *t);
int SDL_QueryTexture(SDL_Texture *t,void *format,void *access,int *w,int *h);
int SDL_SetTextureColorMod(SDL_Texture *t,uint8_t r,uint8_t g,uint8_t b);
int SDL_SetTextureAlphaMod(SDL_Texture *t,uint8_t a);
int SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_Rect *dst);
int SDL_RenderCopyF(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_FRect *dst);
int SDL_RenderCopyExF(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *src,const SDL_FRect *dst,double angle,const SDL_FPoint *center,int flip);
int SDL_RenderDrawLineF(SDL_Renderer *r,float x1,float y1,float x2,float y2);
int SDL_RenderDrawRectF(SDL_Renderer *r,const SDL_FRect *rect);
int SDL_RenderFillRect(SDL_Renderer *r,const SDL_Rect *rect);
uint32_t SDL_GetTicks(void);
void SDL_Delay(uint32_t ms);
int SDL_PollEvent(SDL_Event *event);
uint32_t SDL_GetMouseState(int *x,int *y);
#endif
