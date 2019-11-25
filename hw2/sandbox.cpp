# include "sandbox.h"

# define MAX_BUF_SIZE 128

extern char** environ;

void *handle = NULL;

string basedir = "";
static bool is_cmd_valid = false;

void print_envp(void){
    char **envir = environ;
    while(*envir){
        fprintf(stdout, "%s\n", *envir);
        envir++;
    }
    fprintf(stderr, "\n\n\n");
}

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
	    parent_dir_path = parent_dir_path.assign(file_path, 0, file_path_len-file_name_len-1);
	    break;
	}else{
	    file_name_len ++;
	}
    }
    return parent_dir_path;
}

void print_invalid_cmd_msg(string func_name, string cmd){
    fprintf(stderr, "[sandbox] | %s(%s): not allowed\n", func_name.c_str(), cmd.c_str());
}

void print_invalid_path_msg(string func_name, const char* path){ 
    fprintf(stderr, "[sandbox] %s: access to %s is not allowed\n", func_name.c_str(), get_resolved_path(path).c_str());
}

void make_sure_handle_exist(void){
    if (! handle){
        handle = dlopen("libc.so.6", RTLD_LAZY);
    }
}

void make_sure_basedir_exist(void){
    if (basedir.empty()){
        const char* env_basedir = getenv("basedir");
        basedir = get_resolved_path(env_basedir);
    }
}
bool is_valid_dir_path(const char* dir_path){
    make_sure_basedir_exist();
    return basedir == get_resolved_path(dir_path);
}

bool is_valid_file_path(const char* file_path){
    make_sure_basedir_exist();
    return basedir == get_parent_dir_path(get_resolved_path(file_path));
}

bool is_valid_dir_or_file_path(const char* path, string func_name){ 
    struct stat stat_buf;
    static __XSTAT ori_stat = NULL;
    ori_stat = (__XSTAT) dlsym(handle, "__xstat");
    if (ori_stat(_STAT_VER_LINUX, path, &stat_buf) == 0){
        if(S_ISREG(stat_buf.st_mode)){ // the path is a directory
            return is_valid_file_path(path);
        }else if(S_ISDIR(stat_buf.st_mode)){
            return is_valid_dir_path(path);
        }else{
	    return false;
	}
    }else{ // the path is not exist
	fprintf(stderr, "%s: %s\n", func_name.c_str(), strerror(errno));
        return false;
    }
}

static void beforeMain(void){
    make_sure_handle_exist();
    make_sure_basedir_exist();
}

static void afterMain(void){
    if (dlclose(handle) != 0){
	printf("Close Handle Error...\n");
	exit(EXIT_FAILURE);
    }
}

int chdir(const char *path){
    if (is_valid_dir_path(path)){
        static CHDIR ori_chdir = NULL;
        ori_chdir = (CHDIR)dlsym(handle, "chdir");
        return ori_chdir(path);
    }else {
        print_invalid_path_msg("chdir", path);
    }
}


int chmod(const char *pathname, mode_t mode){
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static CHMOD ori_chmod = NULL;
        ori_chmod = (CHMOD)dlsym(handle, __func__);
        return ori_chmod(pathname, mode); 
    }else {
        print_invalid_path_msg(__func__, pathname); 
    }
}


int chown(const char *pathname, uid_t owner, gid_t group){
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static CHOWN ori_chown = NULL;
        ori_chown = (CHOWN)dlsym(handle, __func__);
        return ori_chown(pathname, owner, group); 
    }else {
        print_invalid_path_msg(__func__, pathname);
    }
}

int creat(const char *pathname, mode_t mode){
    if (is_valid_file_path(pathname)){
        static CREAT ori_creat = NULL;
        ori_creat = (CREAT)dlsym(handle, __func__);
        return ori_creat(pathname, mode); 
    }else {
        print_invalid_path_msg(__func__, pathname);
    }
}

FILE* fopen(const char *pathname,const char *mode){
    make_sure_handle_exist();
    if (! is_valid_dir_or_file_path(pathname, __func__)){
       print_invalid_path_msg(__func__, pathname);
    }
    static FOPEN ori_fopen = NULL;
    ori_fopen = (FOPEN)dlsym(handle, __func__);
    return ori_fopen(pathname,mode);
}

int link(const char *oldpath, const char *newpath){
    bool valid_oldpath = is_valid_dir_or_file_path(oldpath, __func__);
    bool valid_newpath = is_valid_dir_or_file_path(newpath, __func__);
    if (valid_oldpath && valid_newpath){
        static LINK ori_link = NULL;
        ori_link = (LINK)dlsym(handle, __func__);
        return ori_link(oldpath, newpath); 
    }else {
	if (! valid_oldpath) print_invalid_path_msg(__func__, oldpath);
	if (! valid_newpath) print_invalid_path_msg(__func__, newpath);
    }
}

int mkdir(const char *pathname, mode_t mode){
    if (is_valid_file_path(pathname)){
        static MKDIR ori_mkdir = NULL;
        ori_mkdir = (MKDIR)dlsym(handle, __func__);
        return ori_mkdir(pathname, mode);
    }else {
        print_invalid_path_msg(__func__, pathname);
    }
}

int open(const char *pathname, mode_t mode){
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static OPEN ori_open = NULL;
        ori_open = (OPEN)dlsym(handle, __func__);
        return ori_open(pathname, mode); 
    }else {
        print_invalid_path_msg(__func__, pathname);
    }
}

int openat(int dirfd, const char *pathname, int flags){
    static OPENAT ori_openat = NULL;
    ori_openat = (OPENAT)dlsym(handle, __func__);
    return ori_openat(dirfd, pathname, flags); 
}

int openat(int dirfd, const char *pathname, int flags, mode_t mode){
    static OPENAT2 ori_openat = NULL;
    ori_openat = (OPENAT2)dlsym(handle, __func__);
    return ori_openat(dirfd, pathname, flags, mode); 
}

DIR* opendir(const char *name){
    static OPENDIR ori_opendir = NULL;
    ori_opendir = (OPENDIR)dlsym(handle, __func__);
    return ori_opendir(name); 
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz){
    static READLINK ori_readlink = NULL;
    ori_readlink = (READLINK)dlsym(handle, __func__);
    return ori_readlink(pathname, buf, bufsiz); 
}

int remove(const char *pathname){
    static REMOVE ori_remove = NULL;
    ori_remove = (REMOVE)dlsym(handle, __func__);
    return ori_remove(pathname); 
}

int rename(const char *oldpath, const char *newpath){
    static RENAME ori_rename = NULL;
    ori_rename = (RENAME)dlsym(handle, __func__);
    return ori_rename(oldpath, newpath); 
}

int rmdir(const char *pathname){
    static RMDIR ori_rmdir = NULL;
    ori_rmdir = (RMDIR)dlsym(handle, __func__);
    return ori_rmdir(pathname); 
}

int __xstat(int ver, const char *pathname, struct stat *statbuf){
    static __XSTAT ori_stat = NULL;
    ori_stat = (__XSTAT)dlsym(handle, __func__);
    return ori_stat(ver, pathname, statbuf); 
}

int symlink(const char *target, const char *linkpath){
    static SYMLINK ori_symlink = NULL;
    ori_symlink = (SYMLINK)dlsym(handle, __func__);
    return ori_symlink(target, linkpath); 
}

int unlink(const char *pathname){
    static UNLINK ori_unlink = NULL;
    ori_unlink = (UNLINK)dlsym(handle, __func__);
    return ori_unlink(pathname); 
}

int execl(const char *path, const char *arg, ...){
    cout << "in execl\n";
    // print_invalid_cmd_msg();
}

int execle(const char *path, const char *arg, ...){
    cout << "in execle\n";
    // print_invalid_cmd_msg();
}

int execlp(const char *file, char *const argv[]){
    cout << "in execlp\n";
    // print_invalid_cmd_msg();
}

int execv(const char *path, char *const argv[]){
    cout << "in execv\n";
    // print_invalid_cmd_msg();
}

int execve(const char *filename, char *const argv[], char *const envp[]){
    print_invalid_cmd_msg("execve", filename);
}

int execvp(const char *file, char *const argv[]){
    cout << "in execvp\n";
    // print_invalid_cmd_msg();
}

int system(const char *command){
    printf("in system\n");
    // string check_str = "";
    // string ori_command = "";
    // static SYSTEM ori_system = NULL;
    // check_str = check_str.assign(string(command), 0, 5);
    // ori_command = string(command).substr(5);;
    // ori_system = (SYSTEM)dlsym(handle, "system");
    // if(check_str == "valid"){
    //     is_cmd_valid = true;
    //     int return_value = ori_system(ori_command.c_str());
    //     is_cmd_valid = false;
    //     return return_value;
    // }else{
       print_invalid_cmd_msg("system", string(command)); 
    // }
}

// int getopt(int argc, char * const argv[], const char *optstring){
//     static GETOPT ori_getopt = NULL;
//     int sandbox_cmd = 0;
//     ori_getopt = (GETOPT)dlsym(handle, "getopt");
//     sandbox_cmd = ori_getopt(argc, argv, optstring);
//     sopath = get_resolved_path(DEFAULT_SOPATH);
//     basedir = get_resolved_path(DEFAULT_BASEDIR);
//     switch(sandbox_cmd){
//          case 'p':
//              sopath = get_resolved_path(optarg);
//              break;
// 
//          case 'd': 
//              basedir = get_resolved_path(optarg);
//              break;
//     }
//     printf("[getopt] basedir: %s\n", basedir);
//     return sandbox_cmd;
// }
