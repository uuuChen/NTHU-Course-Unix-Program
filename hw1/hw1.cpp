# include <iostream>
# include <string.h>
# include <fstream>

# include <stdlib.h>
# include <stdio.h>
# include <getopt.h>
# include <stdbool.h>
# include <dirent.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <netinet/in.h>
# include <regex.h>

# define MAX_LINE_BUF 1024
# define MAX_LINK_PATH_BUF 256

using namespace std;

bool is_str_digit(string str){
    for(int i=0; i < str.size(); i++){
    	char tmp = str[i];
	if (tmp >= 48 && tmp <= 57)
	    continue;
	else
	    return false;
    }
    return true;
}

bool regex_match(const char* str, const char* pattern){
    int status;
    int cflags = REG_EXTENDED;
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    regex_t reg;
    bool match;
    regcomp(&reg, pattern, cflags);
    status = regexec(&reg, str, nmatch, pmatch, 0);
    if (status == 0)
        match = true;
    else
        match = false;
    regfree(&reg);
    return match;    
}

string get_pid_by_inode(const char* inode){
    DIR* proc_dir = opendir("/proc");
    string rtn_pid="";
    if (proc_dir){
    	struct dirent* proc_dirent;
	string pid;
    	while((proc_dirent = readdir(proc_dir)) != NULL){
	    if(is_str_digit(string(proc_dirent -> d_name))){
	    	pid = string(proc_dirent -> d_name);
		string fd_dir_path = "/proc/" + pid + "/fd";
		DIR* fd_dir = opendir(fd_dir_path.c_str());
		if (fd_dir != NULL){
		    struct dirent* fd_dirent;
		    while((fd_dirent = readdir(fd_dir)) != NULL){
		        string fd_entry = fd_dirent -> d_name;
			if (fd_entry != "." && fd_entry != ".."){
			    struct stat fd_entry_stat;
			    string fd_path = fd_dir_path + "/" + fd_entry; 
			    if (stat (fd_path.c_str(), &fd_entry_stat) == 0){				    
				if (S_ISSOCK(fd_entry_stat.st_mode)){
				    char fd_link_path[MAX_LINK_PATH_BUF];
				    if (readlink(fd_path.c_str(), fd_link_path, sizeof(fd_link_path)) != -1){
					if (strstr(fd_link_path, inode) != NULL){
					    return pid;
					}
				    }
			    	}
			    }
			}
		    }
		}
		closedir(fd_dir);
	    }	    
	}
    }else {
    	perror("Failed to open directory /proc. Exit...\n");
	exit(1);
    }
    closedir(proc_dir);
    if (rtn_pid == ""){
    	fprintf(stderr, "Unable to match inode: %s. Exit...\n", inode);
	exit(1);
    }
    return 0;
}

string get_pname_by_pid(const string& pid){
    string stat_file_path = "/proc/" + pid + "/status";
    FILE* stat_file = fopen(stat_file_path.c_str(), "r");
    char pname_key_value[MAX_LINE_BUF];
    char pname[MAX_LINE_BUF];
    fgets(pname_key_value, MAX_LINE_BUF, stat_file);
    sscanf(pname_key_value, "%*s%s\n", pname);
    fclose(stat_file);
    return string(pname);
}

string get_pargs_by_pid(const string& pid){
    string cmdline_file_path = "/proc/" + pid + "/cmdline";
    fstream cmdline_file;
    cmdline_file.open(cmdline_file_path.c_str(), ios::in);
    string ppath, per_parg, pargs;
    getline(cmdline_file, ppath, '\0');
    while(getline(cmdline_file, per_parg, '\0') != NULL)
        pargs += (" " + per_parg);
    cmdline_file.close();
    return pargs;
}

char* trans_Ipv4_hex_to_dec(unsigned int host_hex, unsigned int port_hex){
    char host_dec[INET_ADDRSTRLEN];
    char* ip_dec = new char[MAX_LINE_BUF];
    inet_ntop(AF_INET, &host_hex, host_dec, INET_ADDRSTRLEN);
    if (port_hex == 0)
        sprintf(ip_dec, "%s:*", host_dec);
    else
    	sprintf(ip_dec, "%s:%d", host_dec, port_hex);
    return ip_dec;
}


char* trans_Ipv6_hex_to_dec(unsigned int host_hex, unsigned int port_hex){
    char host_dec[INET6_ADDRSTRLEN];
    char* ip_dec = new char[MAX_LINE_BUF];
    inet_ntop(AF_INET6, &host_hex, host_dec, INET6_ADDRSTRLEN);
    sprintf(ip_dec, "%s:%d", host_dec, port_hex);
    return ip_dec;
}

void list_connections(const string& comm_type, string filter_str){
    string net_fpath, pid, pname, pargs, pid_pname_pargs;
    net_fpath = "/proc/net/" + comm_type;
    FILE* net_fp = fopen(net_fpath.c_str(), "r");
    unsigned int local_host_hex, local_port_hex, remote_host_hex, remote_port_hex;
    char inode[MAX_LINE_BUF];
    char *local_ip_dec, *remote_ip_dec;
    if (net_fp){
    	fgets(new char[MAX_LINE_BUF], MAX_LINE_BUF, net_fp);
	while(fscanf(net_fp, "%*s%x:%x%x:%x%*s%*s%*s%*s%*s%*s%s%*[^\n]%*c", &local_host_hex, &local_port_hex, &remote_host_hex, &remote_port_hex, inode) != EOF){
	     pid = get_pid_by_inode(inode);
	     pname = get_pname_by_pid(pid);
	     pargs = get_pargs_by_pid(pid);
	     if (comm_type == "tcp" || comm_type == "udp"){
	         local_ip_dec = trans_Ipv4_hex_to_dec(local_host_hex, local_port_hex);
	         remote_ip_dec = trans_Ipv4_hex_to_dec(remote_host_hex, remote_port_hex);
	     } else {
	         local_ip_dec = trans_Ipv6_hex_to_dec(local_host_hex, local_port_hex);
		 remote_ip_dec = trans_Ipv6_hex_to_dec(remote_host_hex, remote_port_hex);
	     }
	     pid_pname_pargs = pid + "/" + pname + " " + pargs;
	     if (filter_str.empty() || (!filter_str.empty() && regex_match(pname.c_str(), filter_str.c_str())))
	         printf("%-5s\t%-20s\t%-20s\t%-20s\n", comm_type.c_str(), local_ip_dec, remote_ip_dec, pid_pname_pargs.c_str());
	}   
	if(comm_type == "tcp6" || comm_type == "udp6") 
	    printf("\n");
    }else {
    	perror("Failed to open file. Exit...\n");
	exit(1);
    }
    fclose(net_fp);
}

int main(int argc, char *argv[])
{
    const char* short_options = "tu";
    struct option long_options[] = {
    	{"tcp", 0, NULL, 't'},
    	{"udp", 0, NULL, 'u'},
	{NULL, 0, NULL, 0}
    };
    int opt;
    bool list_tcp = false;
    bool list_udp = false;
    string filter_str;
    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1){
    	switch (opt){
	    case 't':
		list_tcp = true;
	    	break;
	    
	    case 'u':
		list_udp = true;
	    	break;

	    case '?':
	    	printf("Incorrect usage of the options!\n");
		break;
	}
    }
    
    if (argc == 1){
        list_tcp = true;
	list_udp = true;
    }

    if (argv[optind] != NULL){
	if (argc == 2){
	    list_tcp = true;
	    list_udp = true;
	}
	filter_str = string(argv[optind]);
	printf("filter string is : %s\n", argv[optind]);
    }

    if (list_tcp){
	printf("List of TCP connections:\n");
	printf("%-5s\t%-20s\t%-20s\t%-20s\n", "Proto", "Local Address", "Foreign Address", "PID/Program name and arguments");
	list_connections("tcp", filter_str);
	list_connections("tcp6", filter_str);
    }

    if (list_udp){
	printf("List of UDP connections:\n");
	printf("%-5s\t%-20s\t%-20s\t%-20s\n", "Proto", "Local Address", "Foreign Address", "PID/Program name and arguments");
	list_connections("udp", filter_str);
	list_connections("udp6", filter_str);
    }

}
