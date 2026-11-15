#ifndef ZUMA_SDL_IMAGE_H
#define ZUMA_SDL_IMAGE_H
#include "SDL.h"
#define IMG_INIT_JPG 1
#define IMG_INIT_PNG 2
int IMG_Init(int flags);
void IMG_Quit(void);
const char *IMG_GetError(void);
SDL_Surface *IMG_Load(const char *path);
#endif
