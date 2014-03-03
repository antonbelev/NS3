/*
* Author: Anton Belev
* ID: 1103816b
* NS3 Exercise 1
*/

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
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"


#define BUFLEN 32 //TODO change that to 1024

static void parseRequest(char *buf, int fd);
static void returnResponse(int connfd, int error_type, char *filename, int content_type);
static void *readRequestByConnection();

Queue *q;

int main()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		printf("An error occurred while creating the server socket %d\n", errno);
	}

	struct sockaddr_in addr;

	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1108);

	if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		printf("An error occurred while binding the server to the socket %d\n", errno);
	}

	int backlog = 15;


	if (listen(fd, backlog) == -1) {
		printf("An error occurred while starting to listen for clients %d\n", errno);
	}

	int connfd;
	//int *connfd = malloc(sizeof(*connfd));
	struct sockaddr_in cliaddr;
	socklen_t cliaddr_len = sizeof(cliaddr); 

	//Create queue of connection fbs, which is going to be used by the threads
	
	q = queue_create();
	printf("queue_create q->size =%d...\n", q->size);

	if (q == NULL)
		printf("Queue is NULL...\n");

	int pthreads_created = 0;
	pthread_t *thread = malloc(sizeof(pthread_t)*backlog);
	int i;
	for (i = 0; i < backlog; i++){
		int ret = -1;
		printf("STARTING THREAD i=%d...\n", i);
	    ret = pthread_create(&thread[i], NULL, readRequestByConnection, NULL);

	    if(ret != 0) {
	        printf ("Create pthread error!\n");
	        exit (1);
	    }

	    //pthread_join(thread[i], NULL);
	}

    //Accept multiple connections
	while(1) {	

		connfd = accept(fd, (struct sockaddr*) &cliaddr, &cliaddr_len);

		if (connfd == -1) {
			printf("An error occurred while accepting new connections %d\n", errno);
			break;
		}

		//signal here after adding connfd into the queue
		printf("DISPACHER THREAD...\n");
		enqueue(q,connfd);
		printf("DISPACHER ENDED...\n\n");
	}
	// for (i = 0; i < backlog; i++){
	// 	pthread_exit(thread[i]);
	// }
	close(fd);

}

static void *readRequestByConnection() {
	printf("readRequestByConnection q->size =%d...\n", q->size);
	int connfd = dequeue(q);
	printf("readRequestByConnection connfd=%d...\n", connfd);
	
	ssize_t rcount;
	ssize_t buffer_size = BUFLEN;
	char *buf = (char*) malloc(buffer_size * sizeof(char));			
	ssize_t read_bytes = 0;

	while(1) {

		//exit when read returns 0
		while ((rcount = read(connfd, buf + read_bytes, buffer_size - read_bytes)) != 0) {
			read_bytes += rcount;
			if (read_bytes == buffer_size) {
				buffer_size += BUFLEN;
				buf = (char *) realloc(buf, buffer_size);
			}
			if (rcount < BUFLEN)
				break;
		}

		if (rcount == -1) {
			printf("Server: Could not read the message\n");
			break;
		}
		else {
			buf[read_bytes] = '\0';

			char *nextRequest = buf;
		  		//For each GET request in the buffer go to the next GET and write response for that single request
			while (nextRequest = strstr (nextRequest, "GET")) {					  
				parseRequest(nextRequest, connfd);
				nextRequest += strlen("GET");
			}
			close(connfd);
			break;
		}
	}
	close(connfd);
	free(buf);
	return NULL;
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

	if (!strcmp(subbuff,"GET") == 0){
	    returnResponse(fd, 1, NULL, 0); // BAD REQUEST
	    return;
	}
	
	//File exists check part will be done in te response function
	
	getcwd(cwd, sizeof(cwd));
	
	char *filenameStart = buf + 4;
	char *filenameEnd = strstr(filenameStart, " ");
	char *filename = malloc(sizeof(char) * (strlen(filenameStart) - strlen(filenameEnd) + 1));
	
	char *fileExtensionStart = strstr(filenameStart, ".");
	char *fileExtension = malloc(sizeof(char) * (strlen(fileExtensionStart) - strlen(filenameEnd) + 1));

	memcpy(fileExtension, fileExtensionStart, strlen(fileExtensionStart) - strlen(filenameEnd));
	fileExtension[strlen(fileExtensionStart) - strlen(filenameEnd)] = '\0';
	
	if (strcmp(fileExtension, ".gif") == 0)
		content_type = 2;
	else if (strcmp(fileExtension, ".html") == 0)
		content_type = 0;
	else if (strcmp(fileExtension, ".jpeg") == 0)
		content_type = 1;

	memcpy(filename, filenameStart, strlen(filenameStart) - strlen(filenameEnd));
	filename[strlen(filenameStart) - strlen(filenameEnd)] = '\0';
	

	strcat(cwd, filename);
	
	char *cwdfname = malloc(sizeof(char) * (strlen(cwd) + 1));	
	memcpy(cwdfname, cwd, strlen(cwd));
	cwdfname[strlen(cwd)] = '\0';
	free(filename);
	free(fileExtension);

	//Host name check part
	char *hostcur = strcasestr(buf, "host:");
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

static void returnResponse(int connfd, int error_type, char *filename, int content_type){
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
		okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_html) + 1));
		okBUFF[0] = '\0';
		strcat(okBUFF, response_ok);
		strcat(okBUFF, content_type_html);
		//strcat(okBUFF, connection_close);
	}
	else if (content_type == 1){
		okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_jpeg) + 1));
		okBUFF[0] = '\0';
		strcat(okBUFF, response_ok);
		strcat(okBUFF, content_type_jpeg);
		//strcat(okBUFF, connection_close);
	}	
	else if (content_type == 2){
		okBUFF = malloc(sizeof(char) * (strlen(response_ok) + strlen(content_type_gif) + 1));
		okBUFF[0] = '\0';
		strcat(okBUFF, response_ok);
		strcat(okBUFF, content_type_gif);
		//strcat(okBUFF, connection_close);
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
					//source[++newLen] = '\0';
				}
			}
		//bufsize = newLen;

			struct stat fs; //TODO free?
			fstat(fileno(file), &fs);

			bufsize = fs.st_size;

			content_length_buff = malloc(sizeof(char) * (strlen(content_length) + 25 + 5));
			content_length_buff[0] = '\0';

			char strBuff[25];
			sprintf(strBuff, "%d", bufsize);
			strBuff[25] = '\0';

			strcat(content_length_buff, content_length);
			strcat(content_length_buff, strBuff);
			strcat(content_length_buff, "\r\n\r\n");
			printf("content_length_buff |%s|\n", content_length_buff);

		//printf("content_length |%s|\n", content_length_buff);

			outputBUFF = malloc(sizeof(char) * (strlen(okBUFF) + strlen(content_length_buff) + bufsize + 1));
			outputBUFF[0] = '\0';
			strcat(outputBUFF, okBUFF);
			strcat(outputBUFF, content_length_buff);
			headerLen = strlen(outputBUFF);
			memcpy(outputBUFF + headerLen, source, bufsize);
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

