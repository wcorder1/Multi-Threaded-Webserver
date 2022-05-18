#ifndef __WEBSERVER
#define __WEBSERVER

#define NDEBUG

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG(%s:%d) " M, __FILE__, __LINE__, ##__VA_ARGS__)
#endif
extern int CRASH;
int process(int fd);
int gettid();

#endif
