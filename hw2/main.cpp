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
# include <string>
# include <regex>

# define DEFAULT_SOPATH "./sandbox.so"
# define DEFAULT_BASEDIR "."
# define MAX_CMD_BUFFER 128
# define MAX_CMDS_NUM_PER_INPUT 16

using namespace std;

void print_usage(void){
    printf("usage: ./sandbox [-p sopath] [-d basedir] [--] cmd [cmd args ...]\n \
    \t -p: set the path to sandbox.so, default = ./sandox.so\n \
    \t -d: restrict directory, default = .\n \
    \t --: seperate the arguments for sandbox and for the executed command\n\n");
}

bool is_cmd_valid(string cmd){
    regex reg("exec*"); 
    return ! (regex_match(cmd, reg) || cmd == "system");
}

int main(int argc, char *argv[], char** envp)
{
    int sandbox_cmd = 0;

    string sopath = DEFAULT_SOPATH;
    string basedir = DEFAULT_BASEDIR;

    while ((sandbox_cmd = getopt(argc, argv, "p:d:")) != -1){

        switch(sandbox_cmd){
	    case 'p':
	        sopath = optarg;
		break;

	    case 'd':
	        basedir = optarg;
		break;

	    case '?':
	        print_usage();
		exit(EXIT_FAILURE);
	}
    }

    const char* csopath = sopath.c_str();
    setenv("LD_PRELOAD", csopath, 1);

    if (argc > optind){
	string system_cmd = "";
        string user_cmd = "";
	string user_cmd_args = "";
	string redirect_file_path = "";
	int argv_arr_len = 0; 
	int argv_len = 0;
	int i = 0;
	char **exec_argv;
	argv_arr_len = argc - optind; 
	exec_argv = (char**)malloc(argv_arr_len*sizeof(char*)+1);
	while(optind < argc){
	    exec_argv[i++] = argv[optind++];
	}
	exec_argv[i] = NULL;
	char exec_pathname[100] = "/bin/";
	strcat(exec_pathname, exec_argv[0]);
	execve(exec_pathname, exec_argv, envp);
    } 
}
