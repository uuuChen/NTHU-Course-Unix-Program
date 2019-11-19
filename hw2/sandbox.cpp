# include "sandbox.h"

# define MAX_BUF_SIZE 128
# define DEFAULT_SOPATH "./sandbox.so"
# define DEFAULT_BASEDIR "."

void *handle = NULL;

string sopath = DEFAULT_SOPATH;
string basedir = DEFAULT_BASEDIR;

void print_invalid_msg(string func_name, string cmd){
    printf("\nINVALID!!!\n");
    printf("function name: %s\ncommand  line: %s\n\n", func_name.c_str(), cmd.c_str());
    exit(EXIT_FAILURE);
}

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

int chdir(const char *path){
    static CHDIR ori_chdir = NULL;
    ori_chdir = (CHDIR)dlsym(handle, "chdir");
    return ori_chdir(path); 
}


int chmod(const char *pathname, mode_t mode){
    static CHMOD ori_chmod = NULL;
    ori_chmod = (CHMOD)dlsym(handle, "chmod");
    return ori_chmod(pathname, mode); 
}


int chown(const char *pathname, uid_t owner, gid_t group){
    static CHOWN ori_chown = NULL;
    ori_chown = (CHOWN)dlsym(handle, "chown");
    return ori_chown(pathname, owner, group); 
}


int creat(const char *pathname, mode_t mode){
    static CREAT ori_creat = NULL;
    ori_creat = (CREAT)dlsym(handle, "creat");
    return ori_creat(pathname, mode); 
}

// FILE* fopen(const char *pathname, const char *mode){
//     static FOPEN ori_fopen = NULL;
//     ori_fopen = (FOPEN)dlsym(handle, "fopen");
//     return ori_fopen(pathname, mode); 
// }
// 
// int link(const char *oldpath, const char *newpath){
//     static LINK ori_link = NULL;
//     ori_link = (LINK)dlsym(handle, "link");
//     return ori_link(oldpath, newpath); 
// }
// 
int mkdir(const char *pathname, mode_t mode){
    static MKDIR ori_mkdir = NULL;
    ori_mkdir = (MKDIR)dlsym(handle, "mkdir");
    return ori_mkdir(pathname, mode);
}
// 
// int open(const char *pathname, mode_t mode){
//     static OPEN ori_open = NULL;
//     ori_open = (OPEN)dlsym(handle, "open");
//     return ori_open(pathname, mode); 
// }
// 
// int openat(int dirfd, const char *pathname, int flags){
//     static OPENAT ori_openat = NULL;
//     ori_openat = (OPENAT)dlsym(handle, "openat");
//     return ori_openat(dirfd, pathname, flags); 
// }
// 
// int openat(int dirfd, const char *pathname, int flags, mode_t mode){
//     static OPENAT2 ori_openat = NULL;
//     ori_openat = (OPENAT2)dlsym(handle, "openat");
//     return ori_openat(dirfd, pathname, flags, mode); 
// }
// 
// DIR* opendir(const char *name){
//     static OPENDIR ori_opendir = NULL;
//     ori_opendir = (OPENDIR)dlsym(handle, "opendir");
//     return ori_opendir(name); 
// }
// 
// ssize_t readlink(const char *pathname, char *buf, size_t bufsiz){
//     static READLINK ori_readlink = NULL;
//     ori_readlink = (READLINK)dlsym(handle, "readlink");
//     return ori_readlink(pathname, buf, bufsiz); 
// }
// 
// int remove(const char *pathname){
//     static REMOVE ori_remove = NULL;
//     ori_remove = (REMOVE)dlsym(handle, "remove");
//     return ori_remove(pathname); 
// }
// 
// int rename(const char *oldpath, const char *newpath){
//     static RENAME ori_rename = NULL;
//     ori_rename = (RENAME)dlsym(handle, "rename");
//     return ori_rename(oldpath, newpath); 
// }
// 
// int rmdir(const char *pathname){
//     static RMDIR ori_rmdir = NULL;
//     ori_rmdir = (RMDIR)dlsym(handle, "rmdir");
//     return ori_rmdir(pathname); 
// }
// 
// int stat(const char *pathname, struct stat *statbuf){
//     static STAT ori_stat = NULL;
//     ori_stat = (STAT)dlsym(handle, "stat");
//     return ori_stat(pathname, statbuf); 
// }
// 
// int symlink(const char *target, const char *linkpath){
//     static SYMLINK ori_symlink = NULL;
//     ori_symlink = (SYMLINK)dlsym(handle, "symlink");
//     return ori_symlink(target, linkpath); 
// }
// 
// int unlink(const char *pathname){
//     static UNLINK ori_unlink = NULL;
//     ori_unlink = (UNLINK)dlsym(handle, "unlink");
//     return ori_unlink(pathname); 
// }
// 
// int execl(const char *path, const char *arg, ...){
//     // print_invalid_msg();
// }
// 
// int execle(const char *path, const char *arg, ...){
//     // print_invalid_msg();
// }
// 
// int execlp(const char *file, char *const argv[]){
//     // print_invalid_msg();
// }
// 
// int execv(const char *path, char *const argv[]){
//     // print_invalid_msg();
// }
// 
// int execve(const char *filename, char *const argv[], char *const envp[]){
//     // print_invalid_msg();
// }
// 
// int execvp(const char *file, char *const argv[]){
//     // print_invalid_msg();
// }

int system(const char *command){
    string check_str;
    check_str = check_str.assign(string(command), 0, 5);
    string ori_command;
    if(check_str == "valid"){
        ori_command = string(command).substr(5);
        static SYSTEM ori_system = NULL;
        ori_system = (SYSTEM)dlsym(handle, "system");
        return ori_system(ori_command.c_str());
    }else{
       print_invalid_msg("system", string(command)); 
    }
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
