# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <iostream>
# include <signal.h>
# include <sys/stat.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <unistd.h>
# include <netinet/in.h>
# include <arpa/inet.h>

# define CRLF "\r\n"

using namespace std;

char docroot[128];

struct Request {
    char* method;
    char* uri;
    char* proto;
    char* queryStr;
};

struct FileInfo {
    bool exist;
    FILE* fp;
    char* abs_file_path;
    char* MIME_type;
    char* status_code;
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
    req.method = strtok(recv_buf, " \t\r\n");
    req.uri = strtok(NULL, " \t");
    req.proto = strtok(NULL,  " \t\r\n");
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

char* get_file_MIME_type(char* file_name){
    char *file_ext;
    int i;
    if((file_ext = strchr(file_name, '.')) == NULL){
        fprintf(stderr, "strchr error\n"); 
	exit(-1);
    }
    *file_ext++ = '\0';
    for(i=0; extensions[i].ext != 0; i++){
        if(strcmp(file_ext, extensions[i].ext) == 0){
	    return extensions[i].MIME_type;
	}
    }
    return NULL;
}

struct FileInfo get_fileInfo(char* file_name){
    FileInfo fileInfo;
    struct stat stat_buf;
    char file_path[128], idxHTML_file_path[128];
    strcpy(file_path, docroot);
    strcat(file_path, file_name);
    fileInfo.MIME_type = (char*) "text/html";
    if(stat(file_path, &stat_buf) < 0){ // file does not exist
        fileInfo.exist = false;
	fileInfo.status_code = (char*) "403 Forbidden";  // use 403 intentionally
	return fileInfo;
    }
    fileInfo.exist = true;
    fileInfo.abs_file_path = file_path;
    if(S_ISDIR(stat_buf.st_mode)){ // file is a directory
	if(file_path[strlen(file_path)-1] != '/'){ // directory without slash
	    fileInfo.status_code = (char*) "301 Moved Permanently";
	    return fileInfo;
	}
	fileInfo.status_code = (char*) "200 OK";
	strcpy(idxHTML_file_path, file_path);
	strcat(idxHTML_file_path, "index.html");
        if(check_file_exist(idxHTML_file_path)){ // index.html exists
	    printf("index.html exists\n");
	}else{  // index.html does not exist
	    printf("index.html does not exist\n");
	}
    }else{ // other formats
        if((access(file_path, R_OK)) < 0){ // does not have read permission
	    fileInfo.status_code = (char*) "404 NOT FOUND";
	}else{ // has read permission
	    fileInfo.status_code = (char*) "200 OK";
	    fileInfo.MIME_type = get_file_MIME_type(file_name);
	    fileInfo.content_len = stat_buf.st_size;
	    fileInfo.fp = fopen(file_path, "rb");
	}
    }
    return fileInfo;
}

char* get_header(struct FileInfo fileInfo){
    char* header = (char*) malloc(0xffff * sizeof(char));
    char* content_len_buf = (char*) malloc(128 * sizeof(char));
    strcpy(header, "HTTP/1.1 ");
    strcat(header, fileInfo.status_code);
    strcat(header, CRLF);
    strcat(header, "Server: nginx");
    strcat(header, CRLF);
    strcat(header, "Content-Type: ");
    strcat(header, fileInfo.MIME_type);
    strcat(header, CRLF);
    strcat(header, "Content-Length: ");
    sprintf(content_len_buf, "%d", fileInfo.content_len);
    strcat(header, content_len_buf);
    strcat(header, CRLF);
    // CRLF
    strcat(header, CRLF);
    printf("header:\n%s\n", header);
    return header;
    // string header;	    
    // char* content_len_buf = (char*) malloc(128 * sizeof(char));
    // sprintf(content_len_buf, "%d", fileInfo.content_len);
    // header = (string("HTTP/1.0 ") + CRLF + 
    //           string("Server: nginx") + CRLF +
    //           string("Content-Type: ") + string(fileInfo.MIME_type) + CRLF + 
    //           string("Content-Length: ") + string(content_len_buf) + CRLF + CRLF);
    // return (char*) header.c_str();
}

void server_respond(int connfd){
    struct Request req;
    struct FileInfo fileInfo;
    char sendBuf[1000000];
    int send_bytes;
    req = get_request(connfd); 
    if(strcmp(req.method, "GET") == 0){ // GET    
	fileInfo = get_fileInfo(req.uri);	
        strcpy(sendBuf, get_header(fileInfo));
        if(write(connfd, sendBuf, strlen(sendBuf)) < 0){
            fprintf(stderr, "send error\n");
            exit(-1);
        }
	while(!feof(fileInfo.fp)){
	    send_bytes = fread(sendBuf, sizeof(char), sizeof(sendBuf), fileInfo.fp);
	    if(send_bytes == 0)
	        break;
	    if(write(connfd, sendBuf, send_bytes) < 0){
	        fprintf(stderr, "send error\n");
		exit(-1);
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
