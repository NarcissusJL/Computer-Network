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
#include <sys/select.h>
#include <signal.h>
#include <netdb.h>

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

struct sockaddr_in servaddr;
int sockfd;
/***
 * signal handler
 */
void sig_handler(int s) {
    printf("caught signal %d\n", s);
    //send the EXIT message to server
    send_wrapper(sockfd, &servaddr, "EXIT", strlen("EXIT") + 1);
    close(sockfd);
    exit(0);
}


/*
 * usage client hostname
 **/
int main(int argc, char **argv)
{   
    if (argc < 2) 
    {
      fprintf(stderr, "client: You must specify the server hostname on the command line.");
      exit(1);
    }

    struct hostent *server_host = gethostbyname(argv[1]);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = *((unsigned long *)server_host->h_addr);//inet_addr(argv[1]);
    servaddr.sin_port = htons(SERVERPORT);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror(strerror(errno));
        exit(1);
    }

    char name[NAMELEN] = "abc";
    //send to server
    send_wrapper(sockfd, &servaddr, name, strlen(name) + 1);

    struct sigaction sighandler;
    sighandler.sa_handler = sig_handler;
    sigemptyset(&sighandler.sa_mask);
    sighandler.sa_flags = 0;
    sigaction(SIGINT, &sighandler, NULL);

    char message[BUFLEN];
    size_t msg_length;
    //get the first message, another client name
    recv_wrapper(sockfd, name, &msg_length);
    printf("-----------\n");

    fd_set watch_socks, all_socks;
    FD_ZERO(&all_socks);
    FD_SET(sockfd, &all_socks);
    FD_SET(0, &all_socks); //set stdin

    while(1) {
        // watch for the stdin and the sockfd
        memcpy(&watch_socks, &all_socks, sizeof(fd_set));

        if(select(sockfd + 1, &watch_socks, NULL, NULL, NULL) < 0) {
            close(sockfd);
            return 1;
        }
        if(FD_ISSET(0, &watch_socks)) { 
            scanf("%[^\n]", message); //the message including blank
            getchar(); //the last \n char

            //send the message
            send_wrapper(sockfd, &servaddr, message, strlen(message) + 1);
            if(strcmp(message, "EXIT") == 0) { //the client quit the chatroom
                close(sockfd);
                return 0;
            }
        } else if(FD_ISSET(sockfd, &watch_socks)) {
            // receive a message 
            recv_wrapper(sockfd, message, &msg_length);
            printf("RECV from %s: %s\n", name, message);
            if(strcmp(message, "EXIT") == 0) {
                close(sockfd);
                return 0;
            }
        }
    }
  
  return 0;
}
