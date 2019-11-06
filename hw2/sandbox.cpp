# include "sandbox.h"

# define MAX_BUF_SIZE 128
# define DEFAULT_SOPATH "./sandbox.so"
# define DEFAULT_BASEDIR "."

void *handle = NULL;

string sopath = DEFAULT_SOPATH;
string basedir = DEFAULT_BASEDIR;

static void beforeMain(void){
    if (! handle){
        handle = dlopen("libc.so.6", RTLD_LAZY);
    }
}

static void afterMain(void){
    if (dlclose(handle) != 0){
	printf("Close Handle Error...\n");
	exit(EXIT_FAILURE);
    }
}

int system(const char *command){
    static SYSTEM ori_system = NULL;
    ori_system = (SYSTEM)dlsym(handle, "system");
    return ori_system(command);
}

int getopt(int argc, char * const argv[], const char *optstring){
    static GETOPT ori_getopt = NULL;
    int sandbox_cmd = 0;
    ori_getopt = (GETOPT)dlsym(handle, "getopt");
    sandbox_cmd = ori_getopt(argc, argv, optstring);
    switch(sandbox_cmd){
         case 'p':
             sopath = string(optarg);
             break;

         case 'd':
             basedir = string(optarg);
             break;
    }
    return sandbox_cmd;
}
