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

typedef struct message_file
{  
   char end_msg;
   char header[2];
   char file_data[253];
} message;

bool read_rudpfile(int sock, FILE *f)
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

             //printf("RECEIVED :\nMsg size : %d \n header : %s ,\nData : %s \n \n",recvd, recv_msg.header, recv_msg.file_data);

             //printf("seq num recv : %d \n \n",atoi(recv_msg.header));
             seq_num = atoi(recv_msg.header);
 
	    if(seq_num == next_seqnum) {
	/* Valid packet, write in the file */

	    int offset = 0;

	      /* since recvd buffer contains seqnum , which shouldn't be written to file */
            recvd = recvd - sizeof(recv_msg.header) - sizeof(recv_msg.end_msg);
	    //printf("will write %d bytes in file now \n", recvd);
	    do
	    {
		size_t written = fwrite(&recv_msg.file_data[offset], 1, recvd-offset, f);
		if (written < 1)
		    return false;
		offset += written;
	    }
	    while (offset < recvd);

	    filesize -= (recvd) - sizeof(recv_msg.header) - sizeof(recv_msg.end_msg);

	    bzero(ack_buffer, sizeof(recv_msg.header));

            snprintf(ack_buffer, buflen,
                         "%d",next_seqnum);

            //printf("sending ack : %s\n \n ", ack_buffer);

          int num = sendto(sock, ack_buffer, buflen, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
          if (num == -1)
         {
            return false;
         }

	  
	   next_seqnum = 1 - next_seqnum;
	 } else {
	
	/* InValid packet,  don't write in the file , just send ACK to it*/
	    bzero(ack_buffer, sizeof(recv_msg.header));

          //  int tmp_ack = 1 - next_seqnum;
            snprintf(ack_buffer, buflen,
                         "%d",seq_num);

            //printf("Only sending ack : %s\n \n ", ack_buffer);

          int num = sendto(sock, ack_buffer, buflen, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
          if (num == -1)
         {
            return false;
         }

	 }

	  //printf("flag : %c\n", recv_msg.end_msg);
	  if(recv_msg.end_msg > 'N') {

		//printf("This was the last msg, so closing down \n");
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

void stopandwait_server(char* iface, long port, FILE* fp) {

	
                /* Take the UDP code path */
     struct sockaddr_in serv_addr;
     int u_sockfd = -1;
     int optval = 1;

 //   UNUSED(iface);
    //printf("in stop and wait server updated\n");

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

    bool ok = read_rudpfile(u_sockfd, fp);

    if (!ok)
    {
        //printf("error read");
    }

    close(u_sockfd);
    exit(0);

}

bool send_rudpdata(int sock,struct sockaddr_in servaddr, void *buf, int buflen)
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

bool send_rudpfile(int sock, struct sockaddr_in servaddr, FILE *f)
{
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);

    message msg;
    char ack_buffer[sizeof(msg.header)];
    bool packet_acked = 0;
    int Seq_num = 0, Ack_num = 0, send_bytes = 0, recvd = 0, lastpacket_sent = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 70 * 1000;

    struct timeval last;
    socklen_t lastlen = sizeof(last);

    msg.end_msg = 'N';


    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        perror("setsockopt failed\n");


    int r = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &last, &lastlen);
        if (r < 0) return r;

  //  if (r == 0) { printf("socket option for RCVTIMOUT was set \n"); }

    //printf("filsize to be sent : %ld \n, intial end msg flag : %c ", filesize, msg.end_msg);
    //printf("Buffer size  : %ld, \n data size : %lu , \n header size : %lu \n end flag size : %zu  \n\n", sizeof(msg), sizeof(msg.file_data), sizeof(msg.header), sizeof(msg.end_msg));

    if (filesize == EOF)
        return false;

    if (filesize > 0)
    {
        char buffer[MAX];
        do
        {
            bzero(msg.header, sizeof(msg.header));

            snprintf(msg.header, sizeof(msg.header),
                                     "%d",Seq_num);

            bzero(msg.file_data, sizeof(msg.file_data));


            size_t num =  sizeof(msg.file_data);
            num = fread(msg.file_data, 1, num, f);
            if (num < 1)
                return false;

            //printf("Size read from file : %zu \n \n", num);

            if((num)  < sizeof(msg.file_data)) {
                //printf("Should be end packet \n ");
                msg.end_msg = 'Y';
                                               
           }

            send_bytes = num + sizeof(msg.header) + sizeof(msg.end_msg);
           //printf("SEND \nHEADER :  %s,\n DATA : %s\n \nBuffer size send :  %d \n \n \n", msg.header, msg.file_data, send_bytes);

            bzero(buffer, MAX);
            memcpy(buffer, &msg, sizeof(message));

        do
        {

            //printf("Sending packet \n \n ");
            if (!send_rudpdata(sock, servaddr,  buffer, send_bytes))
                return false;

            int recv_size =  sizeof(ack_buffer);

            bzero(ack_buffer, sizeof(ack_buffer));
            if((recvd = recvfrom(sock, ack_buffer,
                        recv_size, 0, (struct sockaddr *)&servaddr,
                        (socklen_t* )&recv_size))  > 0){

                 //printf("ack received  : %s \n", ack_buffer);
#if 0
	     	 const char s[4] = ":";
                 char *token;

                 token = strtok(ack_buffer,s);
#endif
                 //printf("ack num recv : %d \n",atoi(ack_buffer));

                Ack_num = atoi(ack_buffer);

                if(Ack_num == Seq_num){
                        //printf("Packet ACKED \n");
                        packet_acked = 1;
                }
             }
            if(msg.end_msg > 'N'){
                    //printf("LAST PACKET \n");
                    lastpacket_sent++;
                    if(lastpacket_sent > 4){
                            //printf("This is the last packet I am sending to end server \n");
                            packet_acked = 1;
                    }

            }
            //printf("was packet acked : %d \n", packet_acked);
        }
        while ( packet_acked == 0 );

        packet_acked = 0;
        lastpacket_sent = 0;
        Seq_num = 1 - Seq_num;
        filesize -= num;

  } while (filesize > 0);

}
    return true;
}

void stopandwait_client(char* host, long port, FILE* fp) {
        int f_sockfd;
    char send_buff[MAX],buffer[MAX];
    struct sockaddr_in     servaddr;
    int res = 0;
   
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
  
    send_rudpfile(f_sockfd, servaddr, fp);


   close(f_sockfd);     
   exit(0);
}


