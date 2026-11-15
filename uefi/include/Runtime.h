#ifndef ZUMA_RUNTIME_H
#define ZUMA_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define CLOCKS_PER_SEC 1000
#define M_PI 3.14159265358979323846
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
typedef int64_t clock_t;
typedef int64_t time_t;
typedef struct ZumaFile FILE;
void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);
void *memcpy(void *d, const void *s, size_t n);
void *memmove(void *d, const void *s, size_t n);
void *memset(void *d, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
char *strcpy(char *d, const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strchr(const char *s, int c);
char *strtok(char *s, const char *delim);
int atoi(const char *s);
double atof(const char *s);
int rand(void);
void srand(unsigned int seed);
_Noreturn void exit(int code);
clock_t clock(void);
time_t time(time_t *out);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *file);
size_t fread(void *p, size_t size, size_t n, FILE *file);
size_t fwrite(const void *p, size_t size, size_t n, FILE *file);
int fseek(FILE *file, long offset, int origin);
char *fgets(char *s, int n, FILE *file);
int fputs(const char *s, FILE *file);
int fscanf(FILE *file, const char *format, ...);
int fprintf(FILE *file, const char *format, ...);
int sprintf(char *s, const char *format, ...);
double sqrt(double x);
double fabs(double x);
double fmin(double x, double y);
double sin(double x);
double cos(double x);
double atan2(double y, double x);
float modff(float x, float *whole);
#endif
