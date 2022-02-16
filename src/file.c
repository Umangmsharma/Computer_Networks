#include<stdio.h>
#include <sys/socket.h>
#include "chat.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include<stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include<stdbool.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>



#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6767
#define MAX 255
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

struct sockaddr_in remote_addr ;
socklen_t addrlen = sizeof(remote_addr);
int f_sockfd = 0;

int readdata(int sock, void *buf, int buflen)
{
    unsigned char *pbuf = (unsigned char *) buf;
    int num_bytes = 0;

    printf("buflen : %d\n", buflen);
    while (buflen > 0)
    {
        int num = read(sock, pbuf, buflen);
	printf("num after : %d\n",num);
	num_bytes += num;

        if (num == -1)
        {
            return false;
        }
         else if(num == 0){
 	    return num_bytes;

	}

        printf("buff recv : %s, num : %d\n", pbuf, num);
        pbuf += num;
        buflen -= num;
    }

    return num_bytes;
}

bool readfile(int sock, FILE *f)
{
    long filesize = 256;
    int read = 0;

    char buffer[MAX];
    int num =  sizeof(buffer);

        while ((read = (readdata(sock, buffer, num)))  > 0 )
        {

	int offset = 0;
            do
            {
		printf("writing %d bytes \n", read);
                size_t written = fwrite(&buffer[offset], 1, (read) - offset , f);
                if (written < 1)
                    return false;
                offset += written;
            }
            while (offset < read);

            filesize -= (read);
	    bzero(buffer, MAX);
        }

    return true;
}

void f_tcp_chat_server(char* iface, long port, FILE *fp){

 int  connfd, len;
    struct sockaddr_in servaddr, cli;
     int optval = true;

    // socket create and verification
    f_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (f_sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));

    if (setsockopt(f_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
 	    printf("REUSEADDR set\n");
    }
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,(const char*)SERVER_IP, (void*)&servaddr.sin_addr.s_addr);

   // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
 

    if ((bind(f_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
  
    // Now server is ready to listen and verification
    if ((listen(f_sockfd, 15)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }     

    len = sizeof(cli);

   
    if((connfd=accept(f_sockfd, (struct sockaddr *)&cli, (socklen_t *)&len))) {

    if (connfd < 0) {
        exit(0);
    }
    
    if (setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
            printf("REUSEADDR set\n");
    }

    printf("calling read file \n");
    bool ok = readfile(connfd, fp);

    if (!ok)
    {
	printf("error read");
    }
}

  close(connfd);
    close(f_sockfd);
    exit(0);
    return;	
}

bool read_udpfile(int sock, FILE *f)
{
	long filesize = 0;
	char buffer[MAX];
	int recvd = 0;

	int num =  sizeof(buffer);

	while((recvd = recvfrom(sock, buffer,
			MAX, 0, (struct sockaddr *)&remote_addr,
			(socklen_t* )&num)) > -1)
	{
	  
	    if(strncmp((const char*)buffer, "END",3) == 0) {
		printf("exit udp server, recvd_bytes : %d \n", recvd);
		close(sock);
		exit(0);

	    }

	    int offset = 0;
	    printf("will write %d bytes in file now \n", recvd);
	    do
	    {
		size_t written = fwrite(&buffer[offset], 1, recvd-offset, f);
		if (written < 1)
		    return false;
		offset += written;
	    }
	    while (offset < recvd);

	    filesize -= (recvd);
	    bzero(buffer, MAX);
	}

    return true;
}


void f_udp_chat_server(long port, FILE* fp){
     struct sockaddr_in serv_addr;
     int u_sockfd = -1;


    u_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (f_sockfd < 0) {
        printf("Failed to get a vaild Socket File Descriptor\n");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;
    serv_addr.sin_port = htons(serv_addr.sin_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(u_sockfd, (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0) {
        printf("Transport: Config Socket Bind failed,"
                "errno : %d, error code : %s \n",
                errno, strerror(errno));
    }

    bool ok = read_udpfile(u_sockfd, fp);

    if (!ok)
    {
	printf("error read");
    }

    close(u_sockfd);
    exit(0);
}

/*
 *  Here is the starting point for your netster part.1 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */
void file_server(char* iface, long port, int use_udp, FILE* fp) {

	if(use_udp == 0) {
		/* Take the TCP code path */
		f_tcp_chat_server(iface,port, fp);
	} else {
		/* Take the UDP code path */
		f_udp_chat_server(port, fp);
	}
}

bool senddata(int sock, void *buf, int buflen)
{
    unsigned char *pbuf = (unsigned char *) buf;

    while (buflen > 0)
    {
        int num = send(sock, pbuf, buflen, 0);
        if (num == -1)
        {
            return false;
        }

        pbuf += num;
        buflen -= num;
    }

    return true;
}

bool send_file(int sock, FILE *f)
{
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
	
    printf("filesize being sent : %ld\n", filesize);
    rewind(f);
    if (filesize == EOF)
        return false;

    if (filesize > 0)
    {
        char buffer[MAX];
        do
        {
            size_t num =  sizeof(buffer);
            num = fread(buffer, 1, num, f);
            if (num < 1)
                return false;

            if (!senddata(sock, buffer, num))
                return false;

            filesize -= num;
        }
        while (filesize > 0);
    }
    return true;
}


void f_tcp_chat_client(char* host,long port, FILE* fp){
  
    int f_sockfd;
    struct sockaddr_in servaddr;
    char size_buff[MAX];
    char send_buff[MAX];


    // socket create and varification
    f_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (f_sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
//   servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(port);
  //  servaddr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, (const char*)SERVER_IP, (void*)&servaddr.sin_addr.s_addr);
  
    // connect the client socket to server socket
    if (connect(f_sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }

      bzero(send_buff, sizeof(send_buff));
      bzero(size_buff, sizeof(size_buff));

    send_file(f_sockfd, fp);

   close(f_sockfd);
   exit(1);

}

bool send_udpdata(int sock,struct sockaddr_in servaddr, void *buf, int buflen)
{
    unsigned char *pbuf = (unsigned char *) buf;

    while (buflen > 0)
    {
	int num = sendto(sock, pbuf, buflen, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (num == -1)
        {
            return false;
        }

	printf("buff sent : %s", pbuf);
        pbuf += num;
        buflen -= num;
    }

    return true;
}

bool send_udpfile(int sock, struct sockaddr_in servaddr, FILE *f)
{
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    if (filesize == EOF)
        return false;

    if (filesize > 0)
    {
        char buffer[MAX];
        do
        {
            size_t num =  sizeof(buffer);
            num = fread(buffer, 1, num, f);
            if (num < 1)
                return false;

            if (!send_udpdata(sock, servaddr,  buffer, num))
                return false;

            filesize -= num;
        }
        while (filesize > 0);
    }
    return true;
}

void f_udp_chat_client(char* host,long port, FILE* fp){
        int f_sockfd;
    char send_buff[MAX],buffer[MAX];
    struct sockaddr_in     servaddr;
  
    // Creating socket file descriptor
    if ( (f_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }
  
    memset(&servaddr, 0, sizeof(servaddr));
    bzero(send_buff, sizeof(send_buff));
    bzero(buffer, sizeof(buffer));
 
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;
      
    bzero(send_buff, sizeof(send_buff));
    bzero(buffer, sizeof(buffer));
    
     // Sending the data
  

    send_udpfile(f_sockfd, servaddr, fp);

    const char *tmp = "END";
    /* for terminating the server, send "END" */
    memset(buffer, 0, sizeof buffer);
    strncpy(buffer, tmp, sizeof buffer - 1);

    if(sendto(f_sockfd, buffer, MAX, 0, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
      perror("[ERROR] sending data to the server.");
      exit(1);
    }
    bzero(buffer, MAX);

   close(f_sockfd);     
   exit(0);
}

void file_client(char* host, long port, int use_udp, FILE* fp) {


     if(use_udp == 0){
	f_tcp_chat_client(host,port,fp);
     } else {
	f_udp_chat_client(host,port,fp);
     }

}
