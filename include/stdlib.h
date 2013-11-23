#ifndef _STDLIB_H
#define _STDLIB_H
#include <defs.h>

int main(int argc, char* argv[]);
void exit(int status);

int printf(const char *format, ...);
void scanf(const char *format, ...);

void *malloc(int elem_size);
void free(void *p);

pid_t getpid();
pid_t getppid();
pid_t fork();

#endif
