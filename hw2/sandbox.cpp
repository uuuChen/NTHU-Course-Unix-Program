# include "sandbox.h"

# define MAX_BUF_SIZE 128
# define DEFAULT_SOPATH "./sandbox.so"
# define DEFAULT_BASEDIR "./"

void *handle = NULL;

string sopath = DEFAULT_SOPATH;
string basedir = DEFAULT_BASEDIR;
static bool is_cmd_valid = false;

string get_resolved_path(const char* path){ 
     char resolved_path[MAX_BUF_SIZE];
     realpath(path, resolved_path);
     return string(resolved_path);
}

string get_parent_dir_path(string file_path){
    int i, file_path_len, file_name_len=0;
    string parent_dir_path = "";
    file_path_len = file_path.size();
    for(i = file_path_len-1; i >= 0; i--){
        if (file_path[i] == '/'){
	    parent_dir_path = parent_dir_path.assign(file_path, 0, file_path_len-file_name_len);
	}else{
	    file_name_len ++;
	}
    }
    cout << "parent_dir_path: " << parent_dir_path << endl;
    return parent_dir_path;
}

void print_invalid_cmd_msg(string func_name, string cmd){
    printf("[sandbox] | function name: %s | command  line: %s | is not allowed\n", func_name.c_str(), cmd.c_str());
    exit(EXIT_FAILURE);
}

void print_invalid_path_msg(string func_name, const char* path){ 
    printf("[sandbox] %s: access to %s is not allowed\n", func_name.c_str(), get_resolved_path(path).c_str());
    exit(EXIT_FAILURE);
}

void make_sure_handle_exist(void){
    if (! handle){
        handle = dlopen("libc.so.6", RTLD_LAZY);
    }
}

bool is_valid_path(const char* path){
    struct stat stat_buf;
    if (stat(path, &stat_buf) == -1){
	printf("stat error...\n");
        exit(EXIT_FAILURE);
    }
    bool valid = false;
    if (S_ISDIR(stat_buf.st_mode)){ // the path is a directory
        valid = (basedir == get_resolved_path(path));
    }else if (S_ISREG(stat_buf.st_mode)){ // the path is a regular file
        valid = (basedir == get_parent_dir_path(string(path)));
    }else{
        printf("the path isn't directory or regular file, exit....\n");
	exit(EXIT_FAILURE);
    } 
    return valid;
}

static void beforeMain(void){
    make_sure_handle_exist();
}

static void afterMain(void){
    if (dlclose(handle) != 0){
	printf("Close Handle Error...\n");
	exit(EXIT_FAILURE);
    }
}

int chdir(const char *path){
    static CHDIR ori_chdir = NULL;
    if (! is_valid_path(path)){
        print_invalid_path_msg("chdir", path);
    }
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
    CREAT ori_creat = NULL;
    ori_creat = (CREAT)dlsym(handle, "creat");
    return ori_creat(pathname, mode); 
}

FILE* fopen(const char *pathname, const char *mode){
    static FOPEN ori_fopen = NULL;
    make_sure_handle_exist();
    ori_fopen = (FOPEN)dlsym(handle, "fopen");
    return ori_fopen(pathname, mode); 
}


int link(const char *oldpath, const char *newpath){
    static LINK ori_link = NULL;
    ori_link = (LINK)dlsym(handle, "link");
    return ori_link(oldpath, newpath); 
}

int mkdir(const char *pathname, mode_t mode){
    static MKDIR ori_mkdir = NULL;
    ori_mkdir = (MKDIR)dlsym(handle, "mkdir");
    return ori_mkdir(pathname, mode);
}

int open(const char *pathname, mode_t mode){
    static OPEN ori_open = NULL;
    ori_open = (OPEN)dlsym(handle, "open");
    return ori_open(pathname, mode); 
}

int openat(int dirfd, const char *pathname, int flags){
    static OPENAT ori_openat = NULL;
    ori_openat = (OPENAT)dlsym(handle, "openat");
    return ori_openat(dirfd, pathname, flags); 
}

int openat(int dirfd, const char *pathname, int flags, mode_t mode){
    static OPENAT2 ori_openat = NULL;
    ori_openat = (OPENAT2)dlsym(handle, "openat");
    return ori_openat(dirfd, pathname, flags, mode); 
}

DIR* opendir(const char *name){
    static OPENDIR ori_opendir = NULL;
    ori_opendir = (OPENDIR)dlsym(handle, "opendir");
    return ori_opendir(name); 
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz){
    static READLINK ori_readlink = NULL;
    ori_readlink = (READLINK)dlsym(handle, "readlink");
    return ori_readlink(pathname, buf, bufsiz); 
}

int remove(const char *pathname){
    static REMOVE ori_remove = NULL;
    ori_remove = (REMOVE)dlsym(handle, "remove");
    return ori_remove(pathname); 
}

int rename(const char *oldpath, const char *newpath){
    static RENAME ori_rename = NULL;
    ori_rename = (RENAME)dlsym(handle, "rename");
    return ori_rename(oldpath, newpath); 
}

int rmdir(const char *pathname){
    static RMDIR ori_rmdir = NULL;
    ori_rmdir = (RMDIR)dlsym(handle, "rmdir");
    return ori_rmdir(pathname); 
}

int __xstat(int ver, const char *pathname, struct stat *statbuf){
    static __XSTAT ori_stat = NULL;
    ori_stat = (__XSTAT)dlsym(handle, "__xstat");
    return ori_stat(ver, pathname, statbuf); 
}

int symlink(const char *target, const char *linkpath){
    static SYMLINK ori_symlink = NULL;
    ori_symlink = (SYMLINK)dlsym(handle, "symlink");
    return ori_symlink(target, linkpath); 
}

int unlink(const char *pathname){
    static UNLINK ori_unlink = NULL;
    ori_unlink = (UNLINK)dlsym(handle, "unlink");
    return ori_unlink(pathname); 
}

// int execl(const char *path, const char *arg, ...){
//     cout << "in execl\n";
//     // print_invalid_cmd_msg();
// }
// 
// int execle(const char *path, const char *arg, ...){
//     cout << "in execle\n";
//     // print_invalid_cmd_msg();
// }
// 
// int execlp(const char *file, char *const argv[]){
//     cout << "in execlp\n";
//     // print_invalid_cmd_msg();
// }
// 
// int execv(const char *path, char *const argv[]){
//     cout << "in execv\n";
//     // print_invalid_cmd_msg();
// }
// 
// int execve(const char *filename, char *const argv[], char *const envp[]){
//     cout << "in execve\n";
//     cout << is_cmd_valid << endl;
//     if (is_cmd_valid){
//         static EXECVE ori_execve = NULL;
//         ori_execve = (EXECVE)dlsym(handle, "execve");
//         return ori_execve(filename, argv, envp); 
//     }else{
//         print_invalid_cmd_msg("execve", "test test");
//     }
// }
// 
// int execvp(const char *file, char *const argv[]){
//     cout << "in execvp\n";
//     // print_invalid_cmd_msg();
// }

int system(const char *command){
    string check_str = "";
    string ori_command = "";
    static SYSTEM ori_system = NULL;
    check_str = check_str.assign(string(command), 0, 5);
    ori_command = string(command).substr(5);;
    ori_system = (SYSTEM)dlsym(handle, "system");
    if(check_str == "valid"){
	is_cmd_valid = true;
	int return_value = ori_system(ori_command.c_str());
	is_cmd_valid = false;
        return return_value;
    }else{
       check_str = check_str.assign(string(command), 0, 6);
       print_invalid_cmd_msg("system", string(command)); 
    }
}

int getopt(int argc, char * const argv[], const char *optstring){
    static GETOPT ori_getopt = NULL;
    int sandbox_cmd = 0;
    ori_getopt = (GETOPT)dlsym(handle, "getopt");
    sandbox_cmd = ori_getopt(argc, argv, optstring);
    sopath = get_resolved_path(DEFAULT_SOPATH);
    basedir = get_resolved_path(DEFAULT_BASEDIR);
    switch(sandbox_cmd){
         case 'p':
             sopath = get_resolved_path(optarg);
             break;

         case 'd': 
             basedir = get_resolved_path(optarg);
             break;
    }

    return sandbox_cmd;
}
