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
 * @param srcaddr the client addr
 * @param message save the message here
 * @param the length of the received message
 * */
void recv_wrapper(int sockfd, struct sockaddr_in *srcaddr, char* message, size_t *length) {
    ssize_t recv_length = 0;
    while(1) {
        socklen_t addrlen = sizeof(*srcaddr);
        ssize_t tmp_length = recvfrom(sockfd, message + recv_length, BUFLEN - recv_length, 0, (struct sockaddr*)srcaddr, &addrlen);
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
    
    // two clients
    struct sockaddr_in clientaddrs[2];
    char names[2][NAMELEN];
    socklen_t clientlens[2];
    // get two client requests
    for(int i = 0; i < 2; ++i) {
        clientlens[i] = sizeof(clientaddrs[i]);
        ssize_t recv_length = recvfrom(sockfd, names[i], NAMELEN, 0, (struct sockaddr*)&clientaddrs[i], &clientlens[i]);
        printf("Get client %s:%d\n", (char*)inet_ntoa(clientaddrs[i].sin_addr), ntohs(clientaddrs[i].sin_port));
        if(recv_length < 0) {
            perror(strerror(errno));
            close(sockfd);
            exit(1);
        }
    }

    //two names
    strcpy(names[0], "SECOND");
    strcpy(names[1], "FIRST");
    //send the names to clients
    for(int i = 0; i < 2; ++i) {
        send_wrapper(sockfd, &clientaddrs[i], names[i], strlen(names[i]) + 1);
    }

    char message[BUFLEN];
    size_t msg_length;
    struct sockaddr_in srcaddr;
    while(1) {
        //receive from any client
        recv_wrapper(sockfd, &srcaddr, message, &msg_length);
        // find the client, transfer the message to another client 
        for(int i = 0; i < 2; ++i) {
            if(srcaddr.sin_port == clientaddrs[i].sin_port && strcmp(inet_ntoa(srcaddr.sin_addr), inet_ntoa(clientaddrs[i].sin_addr)) == 0) {
                printf("%s send to %s: %s\n", names[i], names[(i+1)%2], message);
                send_wrapper(sockfd, &clientaddrs[(i+1)%2], message, msg_length);
                break;
            }
        }
        //one client exit, the server exit
        if(strcmp(message, "EXIT") == 0) {
            close(sockfd);
            return 0;
        }
    }

    close(sockfd);

    return 0;
}

