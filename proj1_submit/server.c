// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

/***
 * send out the message to remote addr
 * 
 * @param sockfd sockect 
 * @param addr send to this remote address
 * @param message send this message
 * @param length the length of the message
 */
void send_wrapper(int sockfd, struct sockaddr_in* addr, char* message, ssize_t length) {
    ssize_t send_length = 0;
    while(send_length < length) {
        ssize_t tmp_length = sendto(sockfd, message + send_length, length - send_length, 0, (struct sockaddr*)addr, sizeof(*addr));
        if(tmp_length <= 0) {
            perror(strerror(errno));
            close(sockfd);
            exit(1);
        }
        send_length += tmp_length;
    }
}

/**
 * recevive a message from remote addr
 * the message must end with '\0'
 *
 * @param sockfd sockect 
 * @param message save the message here
 * @param the length of the received message
 * */
void recv_wrapper(int sockfd, char* message, size_t *length) {
    ssize_t recv_length = 0;
    while(1) {
        ssize_t tmp_length = recvfrom(sockfd, message + recv_length, BUFLEN - recv_length, 0, NULL, NULL);
        if(tmp_length <= 0) {
            perror(strerror(errno));
            close(sockfd);
            exit(1);
        }
        recv_length += tmp_length;
        if(message[recv_length - 1] == '\0') {
            *length = recv_length;
            break;
        }
    }
}

int main()
{
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr1, clientaddr2;
    char name1[NAMELEN], name2[NAMELEN];
    socklen_t clientlen1 = sizeof(clientaddr1);
    socklen_t clientlen2 = sizeof(clientaddr2);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVERPORT);

    // create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror(strerror(errno));
        exit(1);
    }
    
    //bind the socket
    if(bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror(strerror(errno));
        return 1;
    }
    
    //get the first request
    ssize_t recv_length = recvfrom(sockfd, name1, NAMELEN, 0, (struct sockaddr*)&clientaddr1, &clientlen1);
    if(recv_length < 0) {
        perror(strerror(errno));
        close(sockfd);
        exit(1);
    }
    
    printf("get the first request\n");

    // get another request
    recv_length = recvfrom(sockfd, name2, NAMELEN, 0, (struct sockaddr*)&clientaddr2, &clientlen2);
    if(recv_length < 0) {
        perror(strerror(errno));
        close(sockfd);
        exit(1);
    }
    printf("get the second request\n");

    //1 start, then 2
    strcpy(name1, "1 SECOND");
    strcpy(name2, "2 FIRST");
    send_wrapper(sockfd, &clientaddr1, name1, strlen(name1) + 1);
    send_wrapper(sockfd, &clientaddr2, name2, strlen(name2) + 1);

    char message[BUFLEN];
    size_t msg_length;
    while(1) {
        //receive from client1
        printf("waiting for client1\n");
        recv_wrapper(sockfd, message, &msg_length); 
        printf("RECV from client1: %s\n", message);

        //send to client2
        send_wrapper(sockfd, &clientaddr2, message, msg_length); 
        if(strcmp(message, "q") == 0) {
            close(sockfd);
            return 0;
        }

        printf("waiting for client2\n");
        //receive from client2
        recv_wrapper(sockfd, message, &msg_length); 
        printf("RECV from client2: %s\n", message);

        //send to client1
        send_wrapper(sockfd, &clientaddr1, message, msg_length); 
        if(strcmp(message, "q") == 0) {
            close(sockfd);
            return 0;
        }
    }

    close(sockfd);

    return 0;
}
