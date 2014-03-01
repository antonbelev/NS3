#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h> /* memset */
#include <unistd.h> /* close */

#define BUFLEN 5000

int main()
{   
  struct addrinfo hints;
  struct addrinfo* ai0;
  int i;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC; // Can use either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // Want a TCP socket
  if ((i = getaddrinfo("bo720-2-02.dcs.gla.ac.uk", "1108", &hints, &ai0)) != 0) {
      printf("Error: unable to lookup IP address: %s", gai_strerror(i));
  }
  // ai0 is a pointer to the head of a linked list of struct addrinfo
  // values containing the possible addresses of the server; interate
  // through the list, trying to connect to each turn, stopping when
  // a connection succeeds:
  struct addrinfo* ai;
  int fd;
  for (ai = ai0; ai != NULL; ai = ai->ai_next) {
    fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd == -1) {
	printf(" Unable to create socket, try next address in list \n");
	continue;
    }
    if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
	printf(" couldn?t connect to the address, try next in list \n");
	close(fd);
	continue;
    }
    break; // successfully connected
  }
  if (ai == NULL) {
      printf("Couldn't connect to any of the addresses");
  } else {
  // at this point, fd is a file descriptor of a socket connected
  // to the server; use the connection
    /*char *data = "Hello World";
    int datalen = strlen(data);
    int bytes_count;
    if ((bytes_count = write(fd, data, datalen)) == -1) {
      printf("Client: Couldn't write the message. Bytes written: %d. Message length: %d.\n", bytes_count, datalen);
    }
    printf("Bytes written: %d. Message length: %d.\n", bytes_count, datalen);
    
    while(1) {
      ssize_t rcount;
      char buf[BUFLEN];
      
      rcount = read(fd, buf, BUFLEN);
      if (rcount == -1) {
	printf("Client: Could not read the message");
	perror("here");
      }
      else {
	  buf[rcount] = '\0';
	  printf("%d\n", strlen(buf));
	  printf("%s\n", buf);
	  printf("breaking \n");
	  break;
      }      
    }*/
    
    ssize_t rcount;
    char buf[BUFLEN];
	    

    while(1){
	 //char *data;
	 char data[BUFLEN] = "GET /home.html HTTP/1.1\r\nHost: bo720-2-02\r\n";
         //scanf("%s", data);
         int datalen = strlen(data);
         int bytes_count;
         if ((bytes_count = write(fd, data, datalen)) == -1) {
            printf("Server: Couldn't write the message. Bytes written: %d. Message length: %d.\n", bytes_count, datalen);
         }

    	rcount = read(fd, buf, BUFLEN);
    	if (rcount == -1) {
     	 printf("Server: Could not read the message");
     	 perror("here");
    	}
    	else {
     	 buf[rcount] = '\0';
      	 printf("%s\n", buf);
    	}
        break; 
    }

    close(fd);
    
  }
}
