# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>

# include <iostream>
# include <string>

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

int main(int argc, char **argv)
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

    if (argc > optind){
	
	string user_cmds[MAX_CMDS_NUM_PER_INPUT];

        string user_cmd = argv[optind++];
	string user_cmd_args = "";

	int i = 0;
	for (i = optind; i < argc; i++){
	    user_cmd_args += " " + string(argv[i]);
	}
	
	if (user_cmd == "ls"){
	    system((user_cmd + user_cmd_args).c_str());
	}
	
    }
    return 0;
}
