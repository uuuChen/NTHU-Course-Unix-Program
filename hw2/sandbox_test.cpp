# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <dlfcn.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <dirent.h>
# include <vector>
# include <stdbool.h>

# include <iostream>

using namespace std;

int main(int argc, char* argv[], char** envp){

    FILE *file;
    char test_msg[] = "Hello world!\n";
    char *command[] = {"ls", "-al", "./", NULL};
    int fd1, fd2, fd3;
    struct stat buf;
    chdir("../hw2/../");
    creat("./test.txt", 0755);
    chmod("./test.txt", 0777);
    chown("./test.txt", 1000, 1000);
    stat("./test.txt", &buf);
    fd1 = open("./test.txt", O_RDWR);
    write(fd1, test_msg, sizeof(test_msg));
    close(fd1);
    rename("./test.txt", "./test_rename.txt");
    file = fopen("./test_rename.txt", "rw");
    mkdir("../testdir", S_IRWXO);
    remove("./testdir");
    DIR* dir = opendir("../hw2/");
    fd1 = dirfd(dir);
    if ((fd2 = openat(fd1, "hw2", O_RDWR)) == -1){
        printf("openat error!\n");
    }
    if ((fd3 = openat(fd1, "test_rename.txt", O_RDWR, S_IRWXO)) == -1){
        printf("openat error!\n");
    }
    write(fd3, "surprise~~~", 13);
    close(fd2);
    close(fd3);
    closedir(dir);

    execl("/bin/ls", "ls", "-al", "./", (char *)0);
    execle("/bin/ls", "ls", "-al", "./", (char *)0, envp);
    execlp("ls", "ls", "-al", "./", (char *)0);
    execv("/bin/ls", command);
    execvp("ls", command);
    execve("/bin/ls", command, envp);
    system("ls -l");

    return 0;
}
