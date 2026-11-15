#include "../../src/gameplay/Level.h"
/* Host-only adapters: these tests load the manifest and save file, not graphics. */
Engine engine;
static int errors;
int ZumaTestFormat(void){
    char text[64];
    if(sprintf(text,"%9d",42)!=9 || strcmp(text,"       42"))return 1;
    sprintf(text,"%d:%02d",3,7);if(strcmp(text,"3:07"))return 2;
    sprintf(text,"%05d",-7);if(strcmp(text,"-0007"))return 3;
    sprintf(text,"%s %u %.2f","score",123u,1.25);if(strcmp(text,"score 123 1.25"))return 4;
    return 0;
}
void Engine_PushError(const char *a,const char *b){(void)a;(void)b;++errors;}
void Engine_PushErrorFile(const char *a,const char *b){(void)a;(void)b;++errors;}
SDL_Texture *Engine_TextureLoad(const char *p){(void)p;return NULL;}
void Engine_DrawTextureSDL(SDL_Texture *t,float x,float y){(void)t;(void)x;(void)y;}
void SDL_DestroyTexture(SDL_Texture *t){(void)t;}
void SDL_RenderGetLogicalSize(SDL_Renderer *r,int *w,int *h){(void)r;*w=1280;*h=720;}
int SDL_RenderCopy(SDL_Renderer *r,SDL_Texture *t,const SDL_Rect *s,const SDL_Rect *d){(void)r;(void)t;(void)s;(void)d;return 0;}
int SDL_SetRenderDrawColor(SDL_Renderer *r,uint8_t a,uint8_t b,uint8_t c,uint8_t d){(void)r;(void)a;(void)b;(void)c;(void)d;return 0;}
int SDL_RenderDrawLineF(SDL_Renderer *r,float a,float b,float c,float d){(void)r;(void)a;(void)b;(void)c;(void)d;return 0;}
int SDL_RenderFillRect(SDL_Renderer *r,const SDL_Rect *d){(void)r;(void)d;return 0;}
int SDL_RenderDrawRectF(SDL_Renderer *r,const SDL_FRect *d){(void)r;(void)d;return 0;}
int ZumaTestLevels(void){
    if(!LevelMgr_LoadLevels("levels.xml") || errors)return 1;
    if(levelMgr.stages[0].levelsLen!=18 || levelMgr.graphicsLen!=22 || levelMgr.settingsLen<25 || xmlDepth!=0)return 2;
    for(int i=0;i<18;i++)if(!LevelMgr_GetLevelGraphics(&levelMgr.stages[0].levels[i]))return 3;
    levelMgr.bestScore[17][3]=123456;
    levelMgr.bestTime[17][3]=78;
    if(!LevelMgr_SaveProgress())return 4;
    levelMgr.bestScore[17][3]=0;levelMgr.bestTime[17][3]=0;
    if(!LevelMgr_LoadProgress() || levelMgr.bestScore[17][3]!=123456 || levelMgr.bestTime[17][3]!=78)return 5;
    for(int i=0;i<levelMgr.stages[0].levelsLen;i++)free(levelMgr.stages[0].levels[i].settingsID);
    LevelMgr_Free();
    return 0;
}
