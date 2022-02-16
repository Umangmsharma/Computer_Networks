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
#include <stdbool.h>



#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6767
#define MAX 256
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct gobackn_file
{  
   char end_msg;
   char header[3];
   char file_data[252];
} message;

bool read_gbn_udpfile(int sock, FILE *f)
{
	struct sockaddr_in remote_addr ;
	long filesize = 0;
	char buffer[MAX];
	int recvd = 0;
	int seq_num = 0, next_seqnum = 0;;

	message recv_msg;
	char ack_buffer[sizeof(recv_msg.header)];
	int buflen = sizeof(ack_buffer);


	int num =  sizeof(buffer);

	bzero(buffer, MAX);

         //printf("INITIAL :\nrecv msg header size : %lu \n recv file data size: %lu \n \n",sizeof(recv_msg.header), sizeof(recv_msg.file_data));
	while((recvd = recvfrom(sock, buffer,
			MAX, 0, (struct sockaddr *)&remote_addr,
			(socklen_t* )&num)) > -1)
	{
		
	     bzero(recv_msg.header, sizeof(recv_msg.header));
             bzero(recv_msg.file_data, sizeof(recv_msg.file_data));

             memcpy(&recv_msg, buffer, sizeof(buffer));

             printf("RECEIVED :\nMsg size : %d \n header : %s ,\nData : %s \n \n",recvd, recv_msg.header, recv_msg.file_data);

             printf("seq num recv : %d \n \n",atoi(recv_msg.header));
             seq_num = atoi(recv_msg.header);
 
	    if(seq_num == next_seqnum) {
	/* Valid packet, write in the file */

	    int offset = 0;

	      /* since recvd buffer contains seqnum , which shouldn't be written to file */
            recvd = recvd - sizeof(recv_msg.header) - sizeof(recv_msg.end_msg);
	    printf("will write %d bytes in file now \n", recvd);
	    do
	    {
		size_t written = fwrite(&recv_msg.file_data[offset], 1, recvd-offset, f);
		if (written < 1)
		    return false;
	    	printf("Written  %zu bytes in file now, offset : %d \n", written, offset);
		offset += written;
	    }
	    while (offset < recvd);

	    filesize -= (recvd) - sizeof(recv_msg.header) - sizeof(recv_msg.end_msg);

	    bzero(ack_buffer, sizeof(recv_msg.header));

            snprintf(ack_buffer, buflen,
                         "%d",next_seqnum);

            printf("sending ack : %s\n \n ", ack_buffer);

          int num = sendto(sock, ack_buffer, buflen, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
          if (num == -1)
         {
            return false;
         }
	  
	   next_seqnum = 1 + next_seqnum;
	 } else {
	
	/* InValid packet,  don't write in the file , just send ACK to it*/
	    bzero(ack_buffer, sizeof(recv_msg.header));

	    
	  if(next_seqnum > 0) {
          //  int tmp_ack = 1 - next_seqnum;
            snprintf(ack_buffer, buflen,
                         "%d",next_seqnum - 1);

            printf("Only sending ack : %s\n \n ", ack_buffer);

          int num = sendto(sock, ack_buffer, buflen, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
          if (num == -1)
         {
            return false;
         }

	 } else { printf("not sending anything since next_seqnum : %d \n", next_seqnum) ; }
	}

	  printf("flag : %c\n\n", recv_msg.end_msg);
	  if(recv_msg.end_msg > 'N' && (seq_num == (next_seqnum-1))) {

		printf("This was the last msg, so closing down \n");
		close(sock);
		exit(0);
	
	}
	    bzero(buffer, MAX);
	    bzero(ack_buffer, sizeof(recv_msg.header));
            bzero(recv_msg.header, sizeof(recv_msg.header));
            bzero(recv_msg.file_data, sizeof(recv_msg.file_data));
      }

    return true;
}


/*
 *  Here is the starting point for your netster part.1 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */

/* Add function definitions */


void gbn_server(char* iface, long port, FILE* fp) {
	
     /* Take the UDP code path */
     struct sockaddr_in serv_addr;
     int u_sockfd = -1;
     int optval = 1;

 //   UNUSED(iface);
    printf("in go and back server\n");

    u_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (u_sockfd < 0) {
        //printf("Failed to get a vaild Socket File Descriptor\n");
    }

    if (setsockopt(u_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
            //printf("REUSEADDR set\n");
    } else {} //printf("REUSEADDR not set \n");}


    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = port;
    serv_addr.sin_port = htons(serv_addr.sin_port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(u_sockfd, (struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0) {
        //printf("Transport: Config Socket Bind failed,"
        //        "errno : %d, error code : %s \n",
        //        errno, strerror(errno));
    }

    bool ok = read_gbn_udpfile(u_sockfd, fp);

    if (!ok)
    {
        //printf("error read");
    }

    close(u_sockfd);
    exit(0);

}

bool send_gbn_udpdata(int sock,struct sockaddr_in servaddr, void *buf, int buflen)
{
    unsigned char *pbuf = (unsigned char *) buf;

    while (buflen > 0)
    {
	int num = sendto(sock, pbuf, buflen, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
        if (num == -1)
        {
            return false;
        }

//	printf("buff sent : %s", pbuf);
        pbuf += num;
        buflen -= num;
    }

    return true;
}

bool send_gbn_udpfile(int sock, struct sockaddr_in servaddr, FILE *f)
{
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    message msg[128];
    char ack_buffer[sizeof(msg[0].header)];
    int base = 0 , next_seqnum = 0, window_size = 1, Ack_num = 0, send_bytes = 0, bytes_forthis_packet = 0, recvd = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 70 * 1000;

    struct timeval last;
    socklen_t lastlen = sizeof(last);

    msg[0].end_msg = 'N';


    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        perror("setsockopt failed\n");


    int r = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &last, &lastlen);
        if (r < 0) return r;

    long filesize_copy = filesize ;
  //  if (r == 0) { printf("socket option for RCVTIMOUT was set \n"); }

    printf("filsize to be sent : %ld \nIntial end msg flag : %c \n", filesize, msg[0].end_msg);
    printf("Buffer size  : %ld, \ndata size : %lu , \nheader size : %lu \nend flag size : %zu  \n\n", sizeof(msg[0]), sizeof(msg[0].file_data), sizeof(msg[0].header), sizeof(msg[0].end_msg));
	
    printf("base : %d, next_seqsum : %d , window size : %d \n \n", base, next_seqnum, window_size);

    size_t num =0;
    int last_packet = 0;
    if (filesize == EOF)
        return false;

    if (filesize > 0)
    {
	do 
	{
			char buffer[MAX];
	/*This loop is just to send all the packets in the window */
			do
			{
    				    msg[next_seqnum].end_msg = 'N';
				    bzero(msg[next_seqnum].header, sizeof(msg[next_seqnum].header));

				    snprintf(msg[next_seqnum].header, sizeof(msg[next_seqnum].header),
							     "%d",next_seqnum);

				    bzero(msg[next_seqnum].file_data, sizeof(msg[next_seqnum].file_data));


				    num =  sizeof(msg[next_seqnum].file_data);
				    num = fread(msg[next_seqnum].file_data, 1, num, f);
				    if (num < 1){
					printf("no bytes left in bytes to be read \n");
					break;
					//return false;
				    }
				    printf("Size read from file : %zu \n \n", num);

				    if((num)  < sizeof(msg[next_seqnum].file_data)) {
				     	printf("Should be end packet \n");
					msg[next_seqnum].end_msg = 'Y';
					last_packet = 1;
							       
			   	    }

				    send_bytes = num + sizeof(msg[next_seqnum].header) + sizeof(msg[next_seqnum].end_msg);
				   printf("SEND \nHEADER :  %s,\nDATA : %s\n \nBuffer size send :  %d \n \n \n", msg[next_seqnum].header, msg[next_seqnum].file_data, send_bytes);

				    bzero(buffer, MAX);
				    memcpy(buffer, &msg[next_seqnum], sizeof(message));


				    printf("Sending packet seq num : %d \n \n ",next_seqnum);
				    if (!send_gbn_udpdata(sock, servaddr,  buffer, send_bytes))
					return false;

				    next_seqnum = next_seqnum + 1;
				    printf("Next packet seqnum : %d, base : %d, window_size : %d, base_window_size : %d \nLast packet : %d \n \n ", next_seqnum, base, window_size,base + window_size, last_packet);

				    bzero(buffer, MAX);
				    
			 } while((next_seqnum <= base + window_size) && last_packet == 0);

		/* Receive a packet to know how many packets were acked by the server */

		    int recv_size =  sizeof(ack_buffer);
		 
		    printf("Check if there's any packet \n");

		    bzero(ack_buffer, sizeof(ack_buffer));
		    while((recvd = recvfrom(sock, ack_buffer,
				recv_size, 0, (struct sockaddr *)&servaddr,
				(socklen_t* )&recv_size))  > 0){

			 //printf("ack received  : %s \n", ack_buffer);
			 printf("ack num recv : %d \n",atoi(ack_buffer));

			Ack_num = atoi(ack_buffer);
			base = Ack_num + 1 ;

			printf("base updated : %d\n", base);
			if(base == next_seqnum){
				printf("Current seqnum :%d \n", next_seqnum);
			//	base = window_size + 1;
				window_size = window_size + 1;
			}
		     } 
		/* TIMEOUT and no acks received */
		/* Resend all the packets from base to nextseqnum */
		        if(base == next_seqnum) {
				printf("\nTimeout,  base : %d to next_seqnum : %d \n",base, next_seqnum);
				printf("all packets received , so will increase window size by 1 \n\n");
			} else {	
				if(window_size > 1) {
					window_size = window_size / 2;
				}
				printf("\nTimeout, decreased window_size : %d , will send packets from base : %d to next_seqnum : %d \n \n", window_size, base, next_seqnum);
				for(int i = base; i < next_seqnum ; i++){
				    bzero(buffer, MAX);
				    memcpy(buffer, &msg[i], sizeof(message));

				    if(i == (next_seqnum-1)){
					  bytes_forthis_packet = send_bytes;
					  printf("Sending packet : %d, size : %d\n", i, bytes_forthis_packet);
				    } else {
					  bytes_forthis_packet = sizeof(msg[base+i]);
					  printf("Sending packet : %d, size : %d\n", i, bytes_forthis_packet);

				    }

				    if (!send_gbn_udpdata(sock, servaddr,  buffer, bytes_forthis_packet))
					return false;
				}

			}

		bytes_forthis_packet = 0;

		filesize = filesize_copy - (base)*(sizeof(msg[0].file_data));
		printf("filesize remaining : %ld , base : %d \n \n",filesize, base);
	} while (filesize > 0);

   }
    return true;
}

void gbn_client(char* host, long port, FILE* fp) {

    int f_sockfd;
    char send_buff[MAX],buffer[MAX];
    struct sockaddr_in     servaddr;
    int res = 0;
   
    printf("gbn client");
    // Creating socket file descriptor
    if ( (f_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    int optval = 1;

   if (setsockopt(f_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval,
                         sizeof(optval))) {
            //printf("REUSEADDR set\n");
	   }
  
    memset(&servaddr, 0, sizeof(servaddr));
    bzero(send_buff, sizeof(send_buff));
    bzero(buffer, sizeof(buffer));
 
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    res = inet_pton(AF_INET, host,
                    &(servaddr.sin_addr));
    if (res != 1) {
            //printf("Failed to retrieve local chassis ip address,"
             //        "errno : %d, error code : %s",errno, strerror(errno));
            return;
    }
      
    bzero(send_buff, sizeof(send_buff));
    bzero(buffer, sizeof(buffer));
    
     // Sending the data
  
    send_gbn_udpfile(f_sockfd, servaddr, fp);


   close(f_sockfd);     
   exit(0);
}


