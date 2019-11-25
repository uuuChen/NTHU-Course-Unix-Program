# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <dlfcn.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <dirent.h>
# include <stdbool.h>

# include <iostream>
# include <regex>

using namespace std;

static void beforeMain(void) __attribute__((constructor));
static void afterMain(void) __attribute__((destructor));

// minimum list of monitored library functions
typedef int (*CHDIR)(const char *path);
typedef int (*CHMOD)(const char *pathname, mode_t mode);
typedef int (*CHOWN)(const char *pathname, uid_t owner, gid_t group);
typedef int (*CREAT)(const char *pathname, mode_t mode);
typedef FILE *(*FOPEN)(const char *pathname, const char *mode);
typedef int (*LINK)(const char *oldpath, const char *newpath);
typedef int (*MKDIR)(const char *pathname, mode_t mode);
typedef int (*OPEN)(const char *pathname, mode_t mode);
typedef int (*OPENAT)(int dirfd, const char *pathname, int flags);
typedef int (*OPENAT2)(int dirfd, const char *pathname, int flags, mode_t mode);
typedef DIR *(*OPENDIR)(const char *name);
typedef ssize_t (*READLINK)(const char *pathname, char *buf, size_t bufsiz);
typedef int (*REMOVE)(const char *pathname);
typedef int (*RENAME)(const char *oldpath, const char *newpath);
typedef int (*RMDIR)(const char *pathname);
typedef int (*__XSTAT)(int ver, const char *path, struct stat *buf);
typedef int (*SYMLINK)(const char *target, const char *linkpath);
typedef int (*UNLINK)(const char *pathname);

// reject following functions
typedef int (*EXECL)(const char *path, const char *arg, ...);
typedef int (*EXECLE)(const char *path, const char *arg, ...);
typedef int (*EXECLP)(const char *file, const char *arg, ...);
typedef int (*EXECV)(const char *path, char *const argv[]);
typedef int (*EXECVE)(const char *filename, char *const argv[], char *const envp[]);
typedef int (*EXECVP)(const char *file, char *const argv[]);
typedef int (*SYSTEM)(const char *command);

// additional fuctions
typedef int (*GETOPT)(int argc, char * const argv[], const char *optstring);







 



