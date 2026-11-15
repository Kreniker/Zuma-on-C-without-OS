#ifndef ZUMA_STDDEF_H
#define ZUMA_STDDEF_H
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned __int64 size_t;
#endif
#ifndef _PTRDIFF_T_DEFINED
#define _PTRDIFF_T_DEFINED
typedef __int64 ptrdiff_t;
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif
