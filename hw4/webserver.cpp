# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <iostream>
# include <signal.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/wait.h>
# include <sys/socket.h>
# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <errno.h>

# define CRLF "\r\n"

using namespace std;

extern char** environ;

char docroot[128];

struct Request {
    char* method;
    char* uri;
    char* proto;
    char* queryStr;
};

struct FileInfo {
    FILE* fp;
    char abs_file_path[128];
    char* MIME_type;
    char* status_code;
    char* errorMsg_html_code;
    int content_len;
};

struct {
    char *ext;
    char *MIME_type;
} extensions [] = {
    {(char*) "html",   (char*) "text/html"},
    {(char*) "htm",    (char*) "text/html"},
    {(char*) "txt",    (char*) "text/plain"},
    {(char*) "css",    (char*) "text/css"},
    {(char*) "gif",    (char*) "image/gif"},
    {(char*) "jpg",    (char*) "image/jpeg"},
    {(char*) "png",    (char*) "image/png"},
    {(char*) "bmp",    (char*) "image/x-ms-bmp"},
    {(char*) "doc",    (char*) "application/msword"},
    {(char*) "pdf",    (char*) "application/pdf"},
    {(char*) "mp4",    (char*) "video/mp4"},
    {(char*) "swf",    (char*) "application/x-shockwave-flash"},
    {(char*) "swfl",   (char*) "application/x-shockwave-flash"},
    {(char*) "ogg",    (char*) "audio/ogg"},
    {(char*) "bz2",    (char*) "application/x-bzip2"},
    {(char*) "gc",     (char*) "application/x-gzip"},
    {(char*) "tar.gz", (char*) "application/x-gzip"},
    {0, 0}
};

void print_usage(void){
    fprintf(stderr, "usage: ./webserver port docroot\n");
}

struct Request get_request(int connfd){
    struct Request req;
    char recv_buf[4096];
    char *method, *uri, *queryStr, *proto;
    int recv_bytes;
    if((recv_bytes = recv(connfd, recv_buf, 4096, 0)) < 0){
        fprintf(stderr, "recv error\n");
        exit(-1);
    }
    recv_buf[recv_bytes] = '\0';
    printf("-----------------recv buf-----------------\n%s\n", recv_buf);
    req.method = strtok(recv_buf, " \t\r\n");
    req.uri = strtok(NULL, " \t");
    req.proto = strtok(NULL,  " \t\r\n");
    req.queryStr = NULL;
    if(queryStr = strchr(req.uri, '?')){ // "?" exist in "req.uri", remove string after it 
	*queryStr++ = '\0'; 
	req.queryStr = queryStr; 
    }
    return req; 
}

bool check_file_exist(char* file_path){
    FILE* fp = fopen(file_path, "r");
    if(fp){
        fclose(fp);
	return true;
    }else{
        return false;
    }
}

char* get_file_ext(char* file_name){
    char *file_ext, *file_name_cpy;
    file_name_cpy = (char*) malloc((strlen(file_name)+1) * sizeof(char));
    strcpy(file_name_cpy, file_name);
    if((file_ext = strchr(file_name_cpy, '.')) == NULL){
	return NULL;
    }
    *file_ext++ = '\0';
    return file_ext;
}

char* get_file_MIME_type(char* file_name){
    char *file_ext;
    int i;
    file_ext = get_file_ext(file_name);
    for(i=0; extensions[i].ext != 0; i++){
        if(strcmp(file_ext, extensions[i].ext) == 0){
	    return extensions[i].MIME_type;
	}
    }
    return NULL;
}

char* get_fileName_from_charArr(char* array){
    int i, j, arr_len;
    char* file_name;
    file_name = (char*) malloc(512 * sizeof(char));
    arr_len = strlen(array);
    for(i=arr_len-1; i>=0; i--){
        if(array[i] == '/' && i != arr_len-1)
	    break;
    }
    for(j=0; j<arr_len-i; j++){
        file_name[j] = array[i+j];
    }
    return file_name;
}

char* get_errorMsg_html_code(char* status_code, char* file_path){
    char* html_code = (char*) malloc(0xfffff * sizeof(char));
    char* temp = (char*) malloc(0xfffff * sizeof(char));
    sprintf(html_code, "<html><head><title>%s</title></head>", status_code);
    strcat(html_code, "<body>");
    sprintf(temp, "<h1>%s</h1>", status_code);
    strcat(html_code, temp);
    if(strcmp(status_code, "301 Moved Permanently") == 0){
	sprintf(temp, "<h3> Move to %s.</h3>", file_path);
        strcat(html_code, temp);
    }else{
	sprintf(temp, "<h3>You don't have permission to access %s on this server.</h3>", file_path);
        strcat(html_code, temp);
    }
    strcat(html_code, "</body></html>");
    return html_code;
}

void get_errorFileInfo(struct FileInfo* fileInfo_ptr, char* status_code, char* file_path){
    fileInfo_ptr->status_code = status_code; 
    fileInfo_ptr->errorMsg_html_code = get_errorMsg_html_code(status_code, file_path);
    fileInfo_ptr->content_len = strlen(fileInfo_ptr->errorMsg_html_code);
}

void exec_ls_and_save_file(struct FileInfo* fileInfo, char* ls_file_path, 
		char* redirect_file_path){
    struct stat stat_buf;
    char command[512];
    FILE* fp;
    strcpy(command, "ls -al ");
    strcat(command, ls_file_path);
    strcat(command, " > ");
    strcat(command, redirect_file_path);
    if((fp = popen(command, "w")) == NULL){
        fprintf(stderr, "popen: %s error", strerror(errno));
	exit(-1);
    }
    pclose(fp);
    stat(redirect_file_path, &stat_buf); 
    fileInfo->MIME_type = (char*) "text/plain"; // .txt file
    fileInfo->content_len = stat_buf.st_size;
    fileInfo->fp = fopen(redirect_file_path, "r");
}

bool use_CGI(char* file_ext){
    if(file_ext){
        if(strcmp(file_ext, (char*) "php") == 0 || 
           strcmp(file_ext, (char*) "sh") == 0)
            return true;
        else
            return false;
    }
    return false;
}

char* get_cmdPath_in_shell(char* cmd){
    FILE *fp;
    char *cmd_dirs[2] = {(char*) "/bin/", (char*) "/usr/bin/"};
    char *cmd_path;
    int i;
    cmd_path = (char*) malloc(128 * sizeof(char));
    for(i=0; i<2; i++){
	strcpy(cmd_path, cmd_dirs[i]);
	strcat(cmd_path, cmd);
        if((fp=fopen(cmd_path, "r")) != NULL){
	    return cmd_path;
	}
    }
    return NULL;
}

int get_openedFile_bytes(FILE* fp){
    int size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    if(size < 0)
        fprintf(stderr, "read bytes error: %s\n", strerror(errno));
    fseek(fp, 0, SEEK_SET);
    printf("size: %d\n", size);
    return size;
}

void CGI(struct FileInfo* fileInfo, char* file_path, char* file_ext){
    int i, pid, readBytes, rtn_pid, stat_val, ser2cgiFd[2], cgi2serFd[2];
    char exec_filepath[128], line[1024];
    char **exec_argv;
    if(pipe(ser2cgiFd) < 0 || pipe(cgi2serFd) < 0){
        fprintf(stderr, "pipe error: %s", strerror(errno));
    }
    if((pid = fork()) < 0){
        fprintf(stderr, "fork error: %s", strerror(errno));
    }else if(pid > 0){ // parent
	close(ser2cgiFd[0]);
	close(cgi2serFd[1]);
        if((rtn_pid = waitpid(pid, &stat_val, 0)) != pid){  // block and wait for child process
	    if(errno != ECHILD){
   	        fprintf(stderr, "get wrong pid %d: %s\n", rtn_pid, strerror(errno));
	        exit(-1);
	    }
	}
  	fileInfo->MIME_type = (char*) "text/plain"; // .txt file 
        fileInfo->fp = fdopen(cgi2serFd[0], "r");
	fileInfo->content_len = get_openedFile_bytes(fileInfo->fp);
	fileInfo->content_len = 200;
	while(1){
	    if((readBytes = read(cgi2serFd[0], line, 1024) < 0)){
	        fprintf(stderr, "read error: %s\n", strerror(errno));
		exit(-1);
	    }
	    printf("%s\n", line);
	    if(readBytes == 0)
		break;
	}
    }else{ // child
	close(ser2cgiFd[1]);
	close(cgi2serFd[0]);
	// dup file descriptor to stdin, stdout
	if(ser2cgiFd[0] != STDIN_FILENO){
	    if(dup2(ser2cgiFd[0], STDIN_FILENO) != STDIN_FILENO){
	        fprintf(stderr, "dup2 error to stdin: %s\n", strerror(errno));
		exit(-1);
	    }
	    close(ser2cgiFd[0]);
	}
	if(cgi2serFd[1] != STDOUT_FILENO){
	    if(dup2(cgi2serFd[1], STDOUT_FILENO) != STDOUT_FILENO){
	        fprintf(stderr, "dup2 error to stdout: %s\n", strerror(errno));
		exit(-1);
	    }
	    close(cgi2serFd[1]);
	}
	// execute command
	strcpy(exec_filepath, get_cmdPath_in_shell(file_ext));
	if(!exec_filepath){
	    fprintf(stderr, "cmd not exist: %s\n", file_ext);
	    exit(-1);
	}
	exec_argv = (char**) malloc(3 * sizeof(char*) + 1);
	exec_argv[0] = exec_filepath;
	exec_argv[1] = file_path;
	exec_argv[2] = NULL;
	if(execve(exec_filepath, exec_argv, environ) < 0){
	    fprintf(stderr, "execve error: %s\n", strerror(errno));
	    exit(-1);
	}
    }
}

struct FileInfo get_fileInfo(char* file_name){
    FileInfo fileInfo;
    struct stat stat_buf;
    char file_path[128], abs_file_path[128], idxHtml_file_path[128];
    char *redirect_file_path, *file_ext;
    file_ext = get_file_ext(file_name);
    strcpy(file_path, docroot);
    strcat(file_path, file_name);
    realpath(file_path, abs_file_path);
    fileInfo.MIME_type = (char*) "text/html";
    fileInfo.errorMsg_html_code = NULL;
    if(use_CGI(file_ext)){  // use common gateway interface
        CGI(&fileInfo, abs_file_path, file_ext);
        return fileInfo;
    }
    if(stat(abs_file_path, &stat_buf) < 0){ // file does not exist, use 403 intentionally
        get_errorFileInfo(&fileInfo, (char*) "403 Forbidden", file_path);
	return fileInfo;
    }
    strcpy(fileInfo.abs_file_path, file_path);
    if((access(file_path, R_OK)) < 0){ // file does not have read permission, use 404 intentionally
        get_errorFileInfo(&fileInfo, (char*) "404 NOT FOUND", file_path);
        return fileInfo;
    }
    if(S_ISDIR(stat_buf.st_mode)){ // file is a directory
	if(file_path[strlen(file_path)-1] != '/'){ // directory without slash
            strcat(fileInfo.abs_file_path, "/");
            get_errorFileInfo(&fileInfo, (char*) "301 Moved Permanently", fileInfo.abs_file_path);
	    return fileInfo;
	}
	fileInfo.status_code = (char*) "200 OK";
	strcpy(idxHtml_file_path, file_path);
	strcat(idxHtml_file_path, "index.html");
        if(check_file_exist(idxHtml_file_path)){ // index.html exists
	    stat(idxHtml_file_path, &stat_buf); 
	    fileInfo.content_len = stat_buf.st_size;
	    fileInfo.fp = fopen(idxHtml_file_path, "rb");
	}else{  // index.html does not exist
	    redirect_file_path = (char*) "./ls.txt";
	    exec_ls_and_save_file(&fileInfo, fileInfo.abs_file_path, redirect_file_path);
	    return fileInfo;
	}
    }else{ // other file formats
        fileInfo.status_code = (char*) "200 OK";
        fileInfo.MIME_type = get_file_MIME_type(file_name);
        fileInfo.content_len = stat_buf.st_size;
        fileInfo.fp = fopen(file_path, "rb");
	
    }
    return fileInfo;
}

char* get_header(struct FileInfo fileInfo){
    string header = "";	    
    char *content_len = (char*) malloc(128 * sizeof(char));
    char *c_header = (char*) malloc(0xffff * sizeof(char));
    sprintf(content_len, "%d", fileInfo.content_len);
    header += string("HTTP/1.1 ") + string(fileInfo.status_code) + CRLF;
    header += string("Server: nginx") + CRLF;
    header += string("Content-Type: ") + string(fileInfo.MIME_type) + CRLF;
    header += string("Content-Length: ") + string(content_len) + CRLF;
    if(string(fileInfo.status_code) == string("301 Moved Permanently")){
        header += string("Location: ") + string(fileInfo.abs_file_path) + CRLF;
    }
    header += CRLF;
    strcpy(c_header, (char*) header.c_str());
    return c_header;
}

void server_respond(int connfd){
    struct Request req;
    struct FileInfo fileInfo;
    char header[0xfffff], body[0xfffff];
    char* file_name;
    int send_bytes;
    req = get_request(connfd); 
    if(strcmp(req.method, "GET") == 0){ // GET    
	file_name = get_fileName_from_charArr(req.uri);
	fileInfo = get_fileInfo(file_name);	
        strcpy(header, get_header(fileInfo));
        printf("-----------------send header-----------------\n%s\n", header);
        if(write(connfd, header, strlen(header)) < 0){
            fprintf(stderr, "send header error\n");
            exit(-1);
        }
        printf("-----------------send body-----------------\n");
	if(fileInfo.errorMsg_html_code){ // status is unnormal	
	    if(write(connfd, fileInfo.errorMsg_html_code, fileInfo.content_len) < 0){
                fprintf(stderr, "send error\n");
                exit(-1);
	    }
	    printf("%s\n", fileInfo.errorMsg_html_code);
	}else{ // status is normal
	    while(!feof(fileInfo.fp)){
	        send_bytes = fread(body, sizeof(char), sizeof(body), fileInfo.fp);
		printf("%s\n", body);
	        if(send_bytes == 0)
	            break;
	        if(write(connfd, body, send_bytes) < 0){
	            fprintf(stderr, "send body error\n");
	    	    exit(-1);
	        }
	    }
	}
	fclose(fileInfo.fp);
    }else if(strcmp(req.method, "POST") == 0){ // POST
    
    }else{ // UNDEFINED
        fprintf(stderr, "UNDEFINED METHOD: %s, RETURN...\n", req.method);
	exit(-1);
    }
}

int main(int argc, char*argv []){
    // get arguments
    int port, sockfd, connfd, pid, val, response_len; 
    struct sockaddr_in serverInfo, clientInfo;
    socklen_t addrlen;
    if(argc != 3){
        print_usage();
	return -1;
    }
    port = atoi(argv[1]);
    realpath(argv[2], docroot);

    // set signal
    signal(SIGCHLD, SIG_IGN);

    // init socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "socket init error\n");
	return -1;
    }
    val = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0){
        fprintf(stderr, "setsockopt error\n");
	return -1;
    }

    // bind
    bzero(&serverInfo, sizeof(serverInfo));
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);
    if(bind(sockfd, (struct sockaddr*) &serverInfo, sizeof(serverInfo)) < 0){
        fprintf(stderr, "bind error\n");
	return -1;
    }
	
    // listen
    if(listen(sockfd, SOMAXCONN) < 0){
        fprintf(stderr, "listen error\n");
	return -1;
    }

    while(1){ 
	printf("wait for connection...\n");
        bzero(&clientInfo, sizeof(clientInfo));
        addrlen = sizeof(clientInfo);
        // wait for connection
	if((connfd = accept(sockfd, (struct sockaddr*) &clientInfo, &addrlen)) < 0){
	    fprintf(stderr, "accept error\n");
	    return -1;
	}
	// connected
	if((pid = fork()) < 0){
	    fprintf(stderr, "fork error\n");
	    return -1;
	}
        if(pid == 0){ // child process
            close(sockfd);
	    server_respond(connfd);
	    exit(0);
        }else{
            close(connfd);
        }
    }
    return 0;
}
// http://localhost:8080/plain.txt?t=1464101346
