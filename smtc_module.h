#include <stdio.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"


#define LOG_RED(X) printf("%s%s%s",RED,X,RESET)
#define LOG_GREEN(X) printf("%s%s%s",GREEN,X,RESET)
#define LOG_YELLOW(X) printf("%s%s%s",YELLOW,X,RESET)
#define LOG_BLUE(X) printf("%s%s%s",BLUE,X,RESET)

int command_proc(const char *command);
