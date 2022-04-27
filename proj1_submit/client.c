// client.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

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

int main(int argc, char **argv)
{   
    if (argc < 2) 
    {
      fprintf(stderr, "client: You must specify the server hostname on the command line.");
      exit(1);
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(SERVERPORT);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror(strerror(errno));
        exit(1);
    }

    char name[NAMELEN] = "abc";
    //send to server
    send_wrapper(sockfd, &servaddr, name, strlen(name) + 1);

    char message[BUFLEN];
    size_t msg_length;
    //get the first message, local name
    recv_wrapper(sockfd, message, &msg_length);

    //receive one message
    strcpy(name, message + 2);
    if(message[0] == '2') {
        recv_wrapper(sockfd, message, &msg_length);
        printf("RECV from %s: %s\n", name, message);
    }

    while(1) {
        // prompt user the input a message
        printf("you SEND: ");
        scanf("%[^\n]", message); //the message including blank
        getchar(); //the last \n char

        //send the message
        send_wrapper(sockfd, &servaddr, message, strlen(message) + 1);
        if(strcmp(message, "q") == 0) { //the client quit the chatroom
            close(sockfd);
            return 0;
        }

        // receive a message
        recv_wrapper(sockfd, message, &msg_length);
        printf("RECV from %s: %s\n", name, message);
        if(strcmp(message, "q") == 0) {
            close(sockfd);
            return 0;
        }
    }
  
  return 0;
}
