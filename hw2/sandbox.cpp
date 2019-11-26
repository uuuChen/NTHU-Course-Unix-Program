# include "sandbox.h"

# define MAX_BUF_SIZE 128

extern char** environ;

void *handle = NULL;

string basedir = "";
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
	    parent_dir_path = parent_dir_path.assign(file_path, 0, file_path_len-file_name_len-1);
	    break;
	}else{
	    file_name_len ++;
	}
    }
    return parent_dir_path;
}

void make_sure_handle_exist(void){
    if (! handle){
        handle = dlopen("libc.so.6", RTLD_LAZY);
    }
}

void make_sure_basedir_exist(void){
    if (basedir.empty()){
        const char* env_basedir = getenv("basedir");
        basedir += get_resolved_path(env_basedir);
    }
}

void print_envp(void){
    char **envir = environ;
    while(*envir){
        fprintf(stdout, "%s\n", *envir);
        envir++;
    }
    fprintf(stderr, "\n\n\n");
}

void print_invalid_cmd_msg(string func_name, string cmd){
    fprintf(stderr, "[sandbox]: %s(%s): not allowed\n", func_name.c_str(), cmd.c_str());
}

void print_invalid_path_msg(string func_name, const char* path){ 
    fprintf(stderr, "[sandbox]: %s: access to %s is not allowed\n", func_name.c_str(), get_resolved_path(path).c_str());
}

string int_to_str(int num){
    char buf[MAX_BUF_SIZE];
    sprintf(buf, "%d", num);
    return string(buf);
}

string get_file_path_by_fd(int fd){
    string fd_path = "/proc/";
    int pid = getpid();
    char file_path[MAX_BUF_SIZE] = {'\0'};
    static READLINK ori_readlink = NULL;
    fd_path = fd_path + int_to_str(pid) + "/fd/" + int_to_str(fd);
    ori_readlink = (READLINK)dlsym(handle, "readlink");
    ori_readlink(fd_path.c_str(), file_path, sizeof(file_path));
    return string(file_path);

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
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_path(path)){
        static CHDIR ori_chdir = NULL;
        ori_chdir = (CHDIR)dlsym(handle, "chdir");
        return ori_chdir(path);
    }else {
        print_invalid_path_msg("chdir", path);
	return -1;
    }
}


int chmod(const char *pathname, mode_t mode){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static CHMOD ori_chmod = NULL;
        ori_chmod = (CHMOD)dlsym(handle, __func__);
        return ori_chmod(pathname, mode); 
    }else {
        print_invalid_path_msg(__func__, pathname); 
	return -1;
    }
}


int chown(const char *pathname, uid_t owner, gid_t group){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static CHOWN ori_chown = NULL;
        ori_chown = (CHOWN)dlsym(handle, __func__);
        return ori_chown(pathname, owner, group); 
    }else {
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int creat(const char *pathname, mode_t mode){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_file_path(pathname)){
        static CREAT ori_creat = NULL;
        ori_creat = (CREAT)dlsym(handle, __func__);
        return ori_creat(pathname, mode); 
    }else {
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

FILE* fopen(const char *pathname,const char *mode){
    printf("[sandbox]: in %s\n", __func__);
    make_sure_handle_exist();
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static FOPEN ori_fopen = NULL;
        ori_fopen = (FOPEN)dlsym(handle, __func__);
        return ori_fopen(pathname,mode);
    } else{
        print_invalid_path_msg(__func__, pathname);
	return NULL;
    }
}

int link(const char *oldpath, const char *newpath){
    printf("[sandbox]: in %s\n", __func__);
    bool valid_oldpath = is_valid_dir_or_file_path(oldpath, __func__);
    bool valid_newpath = is_valid_dir_or_file_path(newpath, __func__);
    if (valid_oldpath && valid_newpath){
        static LINK ori_link = NULL;
        ori_link = (LINK)dlsym(handle, __func__);
        return ori_link(oldpath, newpath); 
    }else {
	if (! valid_oldpath) print_invalid_path_msg(__func__, oldpath);
	if (! valid_newpath) print_invalid_path_msg(__func__, newpath);
	return -1;
    }
}

int mkdir(const char *pathname, mode_t mode){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_file_path(pathname)){
        static MKDIR ori_mkdir = NULL;
        ori_mkdir = (MKDIR)dlsym(handle, __func__);
        return ori_mkdir(pathname, mode);
    }else {
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int open(const char *pathname, int flags, ...){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static OPEN ori_open = NULL;
        ori_open = (OPEN)dlsym(handle, __func__);
        return ori_open(pathname, flags); 
    }else {
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int openat(int dirfd, const char *pathname, int flags, ...){
    printf("[sandbox]: in %s\n", __func__);
    string dir_file_path = get_file_path_by_fd(dirfd);
    string open_path = dir_file_path + "/" + pathname;
    printf("open_path = %s\n", open_path.c_str());
    if (is_valid_dir_or_file_path(open_path.c_str(), __func__)){
        static OPENAT ori_openat = NULL;
        ori_openat = (OPENAT)dlsym(handle, __func__);
        return ori_openat(dirfd, pathname, flags); 
    } else{
        print_invalid_path_msg(__func__, open_path.c_str());
	return -1;
    }
}

DIR* opendir(const char *name){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_path(name)){
        static OPENDIR ori_opendir = NULL;
        ori_opendir = (OPENDIR)dlsym(handle, __func__);
        return ori_opendir(name); 
    } else{
        print_invalid_path_msg(__func__, name);
	return NULL;
    }
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_file_path(pathname)){
        static READLINK ori_readlink = NULL;
        ori_readlink = (READLINK)dlsym(handle, __func__);
        return ori_readlink(pathname, buf, bufsiz); 
    } else{
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int remove(const char *pathname){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_file_path(pathname)){
        static REMOVE ori_remove = NULL;
        ori_remove = (REMOVE)dlsym(handle, __func__);
        return ori_remove(pathname); 	
    } else{
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int rename(const char *oldpath, const char *newpath){
    printf("[sandbox]: in %s\n", __func__);
    bool valid_oldpath = is_valid_file_path(oldpath);
    bool valid_newpath = is_valid_file_path(newpath);
    if (valid_oldpath && valid_newpath){
        static RENAME ori_rename = NULL;
        ori_rename = (RENAME)dlsym(handle, __func__);
        return ori_rename(oldpath, newpath); 
    } else{
	if (! valid_oldpath) print_invalid_path_msg(__func__, oldpath);
	if (! valid_newpath) print_invalid_path_msg(__func__, newpath);
	return -1;
    }
}

int rmdir(const char *pathname){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_file_path(pathname)){
        static RMDIR ori_rmdir = NULL;
        ori_rmdir = (RMDIR)dlsym(handle, __func__);
        return ori_rmdir(pathname); 
    } else{
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int __xstat(int ver, const char *pathname, struct stat *statbuf){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static __XSTAT ori_stat = NULL;
        ori_stat = (__XSTAT)dlsym(handle, __func__);
        return ori_stat(ver, pathname, statbuf); 
    } else{
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int symlink(const char *target, const char *linkpath){
    printf("[sandbox]: in %s\n", __func__);
    bool valid_target = is_valid_dir_or_file_path(target, __func__);
    bool valid_linkpath = is_valid_file_path(linkpath) || is_valid_dir_path(linkpath);
    if (valid_target && valid_linkpath){
        static SYMLINK ori_symlink = NULL;
        ori_symlink = (SYMLINK)dlsym(handle, __func__);
        return ori_symlink(target, linkpath); 
    } else{
	if (! valid_target) print_invalid_path_msg(__func__, target);
	if (! valid_linkpath) print_invalid_path_msg(__func__, linkpath);
	return -1;
    }
}

int unlink(const char *pathname){
    printf("[sandbox]: in %s\n", __func__);
    if (is_valid_dir_or_file_path(pathname, __func__)){
        static UNLINK ori_unlink = NULL;
        ori_unlink = (UNLINK)dlsym(handle, __func__);
        return ori_unlink(pathname); 
    } else{
        print_invalid_path_msg(__func__, pathname);
	return -1;
    }
}

int execl(const char *path, const char *arg, ...){
    print_invalid_cmd_msg(__func__, path);
    return -1;
}

int execle(const char *path, const char *arg, ...){
    print_invalid_cmd_msg(__func__, path);
    return -1;
}

int execlp(const char *file, const char *arg, ...){
    print_invalid_cmd_msg(__func__, file);
    return -1;
}

int execv(const char *path, char *const argv[]){
    print_invalid_cmd_msg(__func__, path);
    return -1;
}

int execve(const char *filename, char *const argv[], char *const envp[]){
    print_invalid_cmd_msg(__func__, filename);
    return -1;
}

int execvp(const char *file, char *const argv[]){
    print_invalid_cmd_msg(__func__, file);
    return -1;
}

int system(const char *command){
    print_invalid_cmd_msg(__func__, string(command)); 
    return -1;
}

