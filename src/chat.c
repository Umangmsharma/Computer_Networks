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



#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6767
#define MAX 255

int sockfd = 0;

void* client_connection_handler(void* client_struct){

   int connfd =  *(int*)client_struct;
   char buff[MAX], sendbuff[MAX];
   int res =0, addrlen,n;
    struct sockaddr_in address;  

    while(1) {
        memset(sendbuff, 0, sizeof(sendbuff));
        memset(buff, 0, sizeof(buff));

        if(((n = read(connfd, buff, sizeof(buff))) > 0)){
		buff[n] ='\n';
		addrlen = sizeof(address);
                getpeername(connfd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                printf("got message from ('%s', %hu)\n",inet_ntoa(address.sin_addr),htons(address.sin_port));

                if(strncmp((const char*)buff,"hello",5) == 0) {
                         strncpy((char*)sendbuff,"world",7);

                         res = write(connfd, sendbuff, sizeof(sendbuff));
                         if(res < 0) { printf("write failure \n");}

        		bzero(sendbuff, MAX);
        		bzero(buff, MAX);

                 } else if(strncmp(buff,"goodbye",7) == 0) {
                         strncpy((char*)sendbuff,"farewell",10);

                         res = write(connfd, sendbuff, sizeof(sendbuff));
                         if(res < 0) { printf("write failure \n");}

                         close(connfd);
                        pthread_exit(NULL);
        		bzero(sendbuff, MAX);
        		bzero(buff, MAX);
                        pthread_exit(NULL);
			 break;

   		} else if(strncmp((const char*)buff,"exit",4) == 0) {
                         strncpy((char*)sendbuff,"ok",4);

                         res = write(connfd, sendbuff, sizeof(sendbuff));
                         if(res < 0) { printf("write failure \n");}

			close(connfd);
			close(sockfd);
        		bzero(sendbuff, MAX);
        		bzero(buff, MAX);
			
			exit(0);
	                
                        break;

                } else {
                        strncpy((char*)sendbuff,(char *)buff,15);

                        res = write(connfd, sendbuff, sizeof(sendbuff));
                        if(res < 0) { printf("write failure \n");}

        		bzero(sendbuff, MAX);
        		bzero(buff, MAX);
            }

}
}

    pthread_exit(NULL);

}

void tcp_chat_server(char* iface, long port){

 int  connfd, len;
    struct sockaddr_in servaddr, cli;
     int optval = true;

    int *client_sock;

  
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
 	    printf("REUSEADDR set\n");
    }
  
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,(const char*)SERVER_IP, (void*)&servaddr.sin_addr.s_addr);

   // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
 

     int num_conn = 0;
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
  
    // Now server is ready to listen and verification
    if ((listen(sockfd, 15)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }     

    len = sizeof(cli);

   
   while((connfd=accept(sockfd, (struct sockaddr *)&cli, (socklen_t *)&len))) {

    if (connfd < 0) {
        exit(0);
	break;
    }
    
    if (setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
            printf("REUSEADDR set\n");
    }

    printf("connection %d from ('%s', %hu)\n", num_conn++,inet_ntoa(cli.sin_addr), htons(cli.sin_port));
	
    pthread_t client_thread;
    client_sock = malloc(1);
    *client_sock = connfd;

    if( pthread_create( &client_thread , NULL , client_connection_handler ,(void *) client_sock) < 0)
    {
         printf("could not create thread");
         break;
    }
    
}
}

void udp_chat_server(long port){
     int res = 0, flag = 0;
     struct sockaddr_in serv_addr;
     struct sockaddr_in remote_addr ;
     socklen_t addrlen = sizeof(remote_addr);
     uint16_t recvlen;
     unsigned long rx_data_buf[MAX];
     static unsigned long tx_data_buf[MAX];
     int sockfd = -1;


    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        printf("Failed to get a vaild Socket File Descriptor\n");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;
    serv_addr.sin_port = htons(serv_addr.sin_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0) {
        printf("Transport: Config Socket Bind failed,"
                "errno : %d, error code : %s \n",
                errno, strerror(errno));
    }

    while(1) {
	    memset(rx_data_buf, 0, sizeof(rx_data_buf));
	    recvlen = recvfrom(sockfd, rx_data_buf,
			MAX, 0, (struct sockaddr *)&remote_addr,
			&addrlen);
	    if (recvlen <= 0 ) {
		printf("Config Packet Reception from Remote Peer failed\n");
		return;
	    }

       	    printf("got message from ('%s', %hu)\n",inet_ntoa(remote_addr.sin_addr),htons(remote_addr.sin_port));
	    
	    if(strncmp("hello",(const char*)rx_data_buf,4) == 0){
		
		strcpy((char*)tx_data_buf,"world");
	    } else if (strncmp("goodbye",(const char*)rx_data_buf,7) == 0){
		strcpy((char*)tx_data_buf,"farewell");
		
	    } else if (strncmp("exit",(const char*)rx_data_buf,4) == 0){
		strcpy((char*)tx_data_buf,"ok");
		flag = 1;
	    } else {
		strcpy((char*)tx_data_buf,(char*)rx_data_buf);
	    }

	    res = sendto(sockfd, tx_data_buf,
		   strlen((const char*)tx_data_buf), 0,
		   (struct sockaddr *)&remote_addr,
		   sizeof(remote_addr));
	    if (res <= 0 ) {
		printf("Failed to send data, "
				    "value returned : %d\n", res);
	    }
		
	    if(flag == 1) {
		close(sockfd);
		flag = 0;
		break;
		
	    }
}
}
/*
 *  Here is the starting point for your netster part.1 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */
void chat_server(char* iface, long port, int use_udp) {
 

	if(use_udp == 0) {
		/* Take the TCP code path */
		tcp_chat_server(iface,port);
	} else {
		/* Take the UDP code path */
		udp_chat_server(port);
	}
}

void tcp_chat_client(char* host,long port){
    int sockfd, res;
    struct sockaddr_in servaddr;
    char recv_buff[MAX];
    char send_buff[MAX];
  
    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
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
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }

      bzero(send_buff, sizeof(send_buff));
      bzero(recv_buff, sizeof(recv_buff));
        
 	while(1) {
		int n = 0;
		while ((send_buff[n++] = getchar()) != '\n')
 		;
		res = write(sockfd, send_buff, sizeof(send_buff));
	        if(res < 0) { printf("write failure \n");}
	
		if(read(sockfd, recv_buff, sizeof(recv_buff)) > 0){
			printf("%s\n",recv_buff);
		// print buffer which contains the client contents
			if((strncmp((const char*)recv_buff,"farewell",8) == 0) || (strncmp((const char*)recv_buff,"ok",2) == 0)){
				close(sockfd);
				break; 
			}
		} 
	}	
}

void udp_chat_client(char* host,long port){
        int sockfd;
    char send_buff[MAX],buffer[MAX];
    struct sockaddr_in     servaddr;
  
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
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
      
    int n=0, len;

    while(1) {
   
    bzero(send_buff, sizeof(send_buff));
    bzero(buffer, sizeof(buffer));
    n=0;
    while ((send_buff[n++] = getchar()) != '\n')
	;

	    sendto(sockfd, send_buff, strlen(send_buff),
		0,    (struct sockaddr*) &servaddr, 
		 sizeof(servaddr));
		  
	    n = recvfrom(sockfd, (char *)buffer, MAX, 
			MSG_WAITALL, (struct sockaddr *) &servaddr,
			(socklen_t *)(&len));
	    printf("%s\n",buffer);
	    if(strncmp("farewell",(const char*)buffer, 8) == 0) {
		close(sockfd);
		break;
            } else if(strncmp("ok",buffer, 2) == 0) {
		close(sockfd);
		break;
	    }
}
}

void chat_client(char* host, long port, int use_udp) {
  

     if(use_udp == 0){
	tcp_chat_client(host,port);
     } else {
	udp_chat_client(host,port);
     }

}
