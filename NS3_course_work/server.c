#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define BUFLEN 5000


static void parseRequest(char *buf, int fd);
static char* returnResponse(int connfd, int error_type, char *filename, int content_type);

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
	printf("An error occurred while creating the server socket\n");
    }

    struct sockaddr_in addr;

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1108);

    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
	printf("An error occurred while binding the server to the socket\n");
    }

    int backlog = 15;
    if (listen(fd, backlog) == -1) {
	printf("An error occurred while starting to listen for clients\n");
    }
    
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddr_len = sizeof(cliaddr);   
    
    
    //Accept multiple connections
    while(1) {	
	
	connfd = accept(fd, (struct sockaddr*) &cliaddr, &cliaddr_len);
	if (connfd == -1) {
	    printf("An error occurred while accepting new connections\n");
	    break;
	}
      
	ssize_t rcount;
	char buf[BUFLEN];
	while(1) {
	      rcount = read(connfd, buf, BUFLEN);
	      printf("rcount %d \n", rcount);
	      if (rcount == -1) {
		  printf("Server: Could not read the message\n");
	      }
	      else if (rcount == 0) {
		//the browser should close the connection
		break;		
	      }
	      else {
		  buf[rcount] = '\0';
		  //printf("%s\n", buf);
		  //Parse the request here
		  //char *requestBuff = malloc(sizeof(char) * (strlen(buf) + 1));
		  //requestBuff[0] = '\0';
		  //strcat(requestBuff, buf);
		  parseRequest(buf, connfd);
		  //free(requestBuff);
		  close(connfd);
		  break;
	      }
	}
	close(connfd);
    }
    close(fd);
}

static void parseRequest(char *buf, int fd)
{
	printf("Parse Request starting...\n");
	int content_type = 0;
	//GET check part
	char cwd[1024];
	char subbuff[4];
	memcpy(subbuff, buf, 3);
	subbuff[3] = '\0';
	char hostname[BUFLEN]; 
	gethostname(hostname, sizeof hostname); 
	//printf("My hostname is: %s\n", hostname);
	if (strcmp(subbuff,"GET") == 0){
	  ;//returnOK(fd); - ok continue
	}    
	else {
	    printf("Problem with GET\n");
	    returnResponse(fd, 1, NULL, 0); // BAD REQUEST
	    return;
	}
	
	//File exists check part will be done in te response function
	
	getcwd(cwd, sizeof(cwd));
	
	//printf("buff: %s\n", buf);
	char *filenameStart = buf + 4;
	//printf("filenameLine: %s\n", filenameStart);
	char *filenameEnd = strstr(filenameStart, " ");
	//printf("filenameEnd: %s\n", filenameEnd);
	char *filename = malloc(sizeof(char) * (strlen(filenameStart) - strlen(filenameEnd) + 1));
	
	char *fileExtensionStart = strstr(filenameStart, ".");
	char *fileExtension = malloc(sizeof(char) * (strlen(fileExtensionStart) - strlen(filenameEnd) + 1));
	
	//printf("strlen(fileExtensionStart) - strlen(filenameEnd) = %d\n", strlen(fileExtensionStart) - strlen(filenameEnd));
	memcpy(fileExtension, fileExtensionStart, strlen(fileExtensionStart) - strlen(filenameEnd));
	fileExtension[strlen(fileExtensionStart) - strlen(filenameEnd)] = '\0';
	
	if (strcmp(fileExtension, ".gif") == 0)
	  content_type = 2;
	else if (strcmp(fileExtension, ".html") == 0)
	  content_type = 0;
	else if (strcmp(fileExtension, ".jpeg") == 0)
	  content_type = 1;
	
	//printf("fileExtension: %s\n", fileExtension);
	
	//printf("strlen(filenameStart) - strlen(filenameEnd) = %d\n", strlen(filenameStart) - strlen(filenameEnd));
	memcpy(filename, filenameStart, strlen(filenameStart) - strlen(filenameEnd));
	filename[strlen(filenameStart) - strlen(filenameEnd)] = '\0';
	
	//char *endOfString = '\0';
	//strcat(filename, endOfString);
	
	//printf("filename: %s\n", filename);
	strcat(cwd, filename);
	
	char *cwdfname = malloc(sizeof(char) * (strlen(cwd) + 1));	
	memcpy(cwdfname, cwd, strlen(cwd));
	cwdfname[strlen(cwd)] = '\0';
	//strcat(cwdfname, endOfString);
	//printf("cwdfname before free: %s\n", cwdfname);
	free(filename);
	free(fileExtension);
	//printf("cwdfname after free: %s\n", cwdfname);
		
	//Host name check part
	char *hostcur = strcasestr(buf, "host:");\
	if (hostcur == NULL){
	      printf("host: not found\n");
	      returnResponse(fd, 1, NULL, 0); //BAD REQUEST
	}
	else {
	      
	      hostcur += 6; //to start of host name
	      
	      char *hostnamereqStart = hostcur;
	      char *hostnamereqEnd = strstr(hostnamereqStart, "\r\n");
	      /*if (strcmp(hostcur, "\n") != 0)
		      returnBAD(fd);*/
	      
	      char hostnamereq[120];
	      memcpy(hostnamereq, hostnamereqStart, hostnamereqEnd - hostnamereqStart);      

	      if (strcmp(hostnamereq, hostname) != 0){
		//printf("Host name doesn't match 1: hostnamereq %s, hostname %s\n", hostnamereq, hostname);
		strcat(hostname, ".dcs.gla.ac.uk:1108\0");
		if (strcmp(hostnamereq, hostname) != 0){
		    //printf("Host name doesn't match 2: hostnamereq %s, hostname %s\n", hostnamereq, hostname);
		    returnResponse(fd, 1, NULL, 0); //BAD REQUEST
		} else {
		  returnResponse(fd, 0, cwdfname, content_type);//OK
		}
	      }else {
		returnResponse(fd, 0, cwdfname, content_type);//OK
	      }
	}
	
	//returnResponse(fd, 0); //OK
}

static char* returnResponse(int connfd, int error_type, char *filename, int content_type){
	printf("Return Response starting...content_type = %d\n", content_type);
	//content_type = 0 - text/html
	//content_type = 1 - image/jpeg
	//content_type = 2 - image/gif
	
	//error_type = 0 - OK
	//error_type = 1 - Bad Request
	//error_type = 2 - Not Found
	//error_type = 3 - Internal Server Error
	char *content_type_html = "Content-Type: text/html\r\n";	
	char *content_type_jpeg = "Content-Type: image/jpeg\r\n";
	char *content_type_gif = "Content-Type: image/gif\r\n";
	 
	char *content_length = "Content-Length: ";	
	char *content_length_buff = NULL;
	
	char *connection_close = "Connection: close\r\n";
	char *response_ok = "HTTP/1.1 200 OK\r\n";
	char *response_bad = "HTTP/1.1 400 Bad Request\r\n";
	char *response_not_found = "HTTP/1.1 404 Not Found\r\n";
	char *response_internal_error = "HTTP/1.1 500 Internal Server Error\r\n";
	
	char *okBUFF;
	char *badBUFF = malloc(sizeof(char) * (strlen(response_bad) + strlen(content_type_html) + strlen(connection_close) + 1));
	badBUFF[0] = '\0';
	strcat(badBUFF, response_bad);
	strcat(badBUFF, content_type_html);
	strcat(badBUFF, connection_close);
	
	char *nfBUFF = malloc(sizeof(char) * (strlen(response_not_found) + strlen(content_type_html) + strlen(connection_close)+  1));
	nfBUFF[0] = '\0';
	strcat(nfBUFF, response_not_found);
	strcat(nfBUFF, content_type_html);
	strcat(nfBUFF, connection_close);
	
	char *internalBUFF =  malloc(sizeof(char) * (strlen(response_internal_error) + strlen(content_type_html) + strlen(connection_close)+  1));
	nfBUFF[0] = '\0';
	strcat(internalBUFF, response_internal_error);
	strcat(internalBUFF, content_type_html);
	strcat(internalBUFF, connection_close);
	
	if (content_type == 0){
	    okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_html) + strlen(connection_close) + 1));
	    okBUFF[0] = '\0';
	    strcat(okBUFF, response_ok);
	    strcat(okBUFF, content_type_html);
	    strcat(okBUFF, connection_close);
	}
	else if (content_type == 1){
	    okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_jpeg) + strlen(connection_close)+ 1));
	    okBUFF[0] = '\0';
	    strcat(okBUFF, response_ok);
	    strcat(okBUFF, content_type_jpeg);
	    strcat(okBUFF, connection_close);
	}	
	else if (content_type == 2){
	    okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_gif) + strlen(connection_close)+ 1));
	    okBUFF[0] = '\0';
	    strcat(okBUFF, response_ok);
	    strcat(okBUFF, content_type_gif);
	    strcat(okBUFF, connection_close);
	}	
	
	char *outputBUFF;
	char *source = NULL;
	char *fopenMode;
	
	
	if ( content_type == 1 || content_type == 2)
	    fopenMode = "rb";
	else
	    fopenMode = "r";
	
	//printf("filename: %s, content_type = %d, fopenMode = %s\n", filename, content_type, fopenMode);
	
	long bufsize = 0;
	long headerLen = 0;
	if (filename != NULL)
	    printf("filename response %s \n", filename);
	if (error_type == 0) {
	    FILE *file;
	    if ((file = fopen(filename, fopenMode)) == NULL) {
		  printf("File not found\n");
		  returnResponse(connfd, 2, filename, 0); // NOT FOUND
		  return;
	    } else {
		  size_t newLen;
		  if (fseek(file, 0L, SEEK_END) == 0) {
		      bufsize = ftell(file);
		      if (bufsize == -1) { 
			  returnResponse(connfd, 3, NULL, 0); // Internal Server error
		      }
		      source = malloc(sizeof(char) * (bufsize + 1));

		      if (fseek(file, 0L, SEEK_SET) != 0) { 
			  returnResponse(connfd, 3, NULL, 0); // Internal Server error
		      }
		      newLen = fread(source, sizeof(char), bufsize, file);
		      if (newLen == 0) {
			  returnResponse(connfd, 3, NULL, 0); // Internal Server error
		      } else {
			  source[++newLen] = '\0';
		      }
		  }
		  bufsize = newLen;
		  printf("predi content_length malloca sum\n");
		  content_length_buff = malloc(sizeof(char) * (strlen(content_length) + bufsize + 3));
		  content_length_buff[0] = '\0';
		  sprintf(content_length_buff, content_length);
		  sprintf(content_length_buff + strlen(content_length_buff), "%d", bufsize);
		  sprintf(content_length_buff, "\r\n");
		  
		  printf("content_length |%s|\n", content_length_buff);
		  outputBUFF = malloc(sizeof(char) * (strlen(okBUFF) + strlen(content_length_buff) + bufsize + 1));
		  outputBUFF[0] = '\0';
		  strcat(outputBUFF, okBUFF);
		  printf("predi strcat na content_length_buff\n");
		  strcat(outputBUFF, content_length_buff);
		  headerLen = strlen(outputBUFF);
		  memcpy(outputBUFF + headerLen, source, bufsize);
		  printf("SOURCE \n%s\n", source);
		  fclose(file);
	    }	    
	}
	else if (error_type == 1){
	    outputBUFF = malloc(sizeof(char) * (strlen(badBUFF) + 1));
	    outputBUFF[0] = '\0';
	    headerLen = strlen(badBUFF);
	    strcat(outputBUFF, badBUFF);
	}
	else if (error_type == 2){
	    outputBUFF = malloc(sizeof(char) * (strlen(nfBUFF) + 1));
	    outputBUFF[0] = '\0';
	    headerLen = strlen(nfBUFF);
	    strcat(outputBUFF, nfBUFF);
	}
	else if (error_type == 3){
	    outputBUFF = malloc(sizeof(char) * (strlen(nfBUFF) + 1));
	    outputBUFF[0] = '\0';
	    headerLen = strlen(nfBUFF);
	    strcat(outputBUFF, nfBUFF);
	}
	
	
	
	int datalen = headerLen + bufsize;
	int bytes_count;
	if ((bytes_count = write(connfd, outputBUFF, datalen)) == -1) {
	    printf("Server: Couldn't write the message. Bytes written: %d. Message length: %d.\n", bytes_count, datalen);
	}
	
	outputBUFF[0]= '\0';
	//free(outputBUFF);
	if (content_length_buff != NULL)
	    free(content_length_buff);
	free(okBUFF);
	free(badBUFF);
	free(nfBUFF);
	free(internalBUFF);
	free(source);
	free(filename);
}
 
  