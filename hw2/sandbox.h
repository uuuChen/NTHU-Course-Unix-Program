# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <dlfcn.h>

# include <iostream>

using namespace std;

static void beforeMain(void) __attribute__((constructor));
static void afterMain(void) __attribute__((destructor));

typedef int (*SYSTEM)(const char *command);
typedef int (*GETOPT)(int argc, char * const argv[], const char *optstring);


