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
#include <map>
#include <list>

#define BUFLEN 1032

#define N 2

/***
 * send out the message to remote addr
 * 
 * @param sockfd sockect 
 * @param addr send to this remote address
 */
void send_wrapper(int sockfd, struct sockaddr_in* addr, uint32_t seqnum, unsigned char control) {
    //lose some packet
    if(rand() % 3 == 0) {
        printf("++++++++++++not ack for %d\n", seqnum);
        return;
    }
    char message[8];
    memset(message, 0, 8);
    int seqnum_net = htonl(seqnum);
    memcpy(message, &seqnum_net, 4);
    message[4] = 0x01;
    message[5] = control;
    ssize_t send_length = 0;
    while(send_length < 8) {
        ssize_t tmp_length = sendto(sockfd, message + send_length, 8 - send_length, 0, (struct sockaddr*)addr, sizeof(*addr));
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
 *
 * @param sockfd sockect 
 * @param srcaddr the client addr
 * @param message save the message here
 * */
void recv_wrapper(int sockfd, struct sockaddr_in *srcaddr, char* message) {
    ssize_t recv_length = 0;
    int length = -1;
    while(1) {
        socklen_t addrlen = sizeof(*srcaddr);
        ssize_t tmp_length = recvfrom(sockfd, message + recv_length, BUFLEN - recv_length, 0, (struct sockaddr*)srcaddr, &addrlen);
        if(tmp_length <= 0) {
            perror(strerror(errno));
            close(sockfd);
            exit(1);
        }
        recv_length += tmp_length;
        if(length == -1 && recv_length >= 8) {
            length = ntohs(*((int*)(message + 6)));
        }
        if(length >= 0 && recv_length == 8 + length) {
            break;
        }
    }
}

void show_flag(char* recv_flag ) {
    for(int i = 0; i < N; ++i) {
        printf("%d ", recv_flag[i]);
    }
    printf("\n---\n");
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

    char message[BUFLEN];
    struct sockaddr_in clientaddr;
    std::map<uint32_t, char*> recv_buffer; //seqnum, data, at most N data
    char recv_flag[N]; //0 not receiverd, 1 received

    uint32_t seqnum;
    uint32_t head_seq; //the first expected seqnum
    int index;
    FILE* fp = NULL;
    char filename[256];
    while(true) {
        //get the first connect_setup
        recv_wrapper(sockfd, &clientaddr, message);
        seqnum = ntohl(*((int*)message));
        printf("get seq %d, ", seqnum);
        switch(message[5]) {
            case 0x01: // connection setup
                printf("client setup\n");
                printf("%s %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                sprintf(filename, "%s_%d.txt", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
                fp = fopen(filename, "w");
                if(fp == NULL) {
                    printf("something wrong for openfile\n");
                    exit(1);
                }

                for(index = 0; index < N; ++index) {
                    recv_flag[index] = 0;
                }
                head_seq = seqnum+1;
                send_wrapper(sockfd, &clientaddr, seqnum, 0x01);
                break;
            case 0x02: //connection teardown
                printf("client tear down\n");
                send_wrapper(sockfd, &clientaddr, seqnum, 0x02);
                if(fp != NULL) {
                    fclose(fp);
                }
                fp = NULL;
                break;
            case 0x00:
                //send ack
                if(seqnum < head_seq) {
                    printf("too older, ack it\n");
                    send_wrapper(sockfd, &clientaddr, seqnum, 0x00);
                } else if(seqnum >= head_seq + N) {
                    printf("client too fast, unexpeced %d\n", seqnum);
                } else {
                    //save it
                    index = seqnum - head_seq;
                    if(recv_flag[index] == 0) {
                        uint16_t datalen = ntohs(*((short*)(message+6)));
                        char* data = (char*)malloc((size_t)datalen + 1);  
                        memcpy(data, message + 8, datalen);
                        data[datalen] = '\0';
                        recv_buffer.insert(std::pair<uint32_t, char*>(seqnum, data));
                        recv_flag[index] = 1;
                        printf("save to buffer %d\n", seqnum);
                    }

                    //write to file?
                    int last_index = N;
                    for(index = 0; index < N; ++index) {
                        if(recv_flag[index] == 1) {
                            //write to buffer
                            printf("write to file %d\n", head_seq + index);
                            char* data = recv_buffer[head_seq + index];
                            fprintf(fp, "%d:%s\n", head_seq + index, data);
                            free(data);
                            recv_buffer.erase(head_seq + index);
                        } else {
                            last_index = index; //have not received
                            break;
                        }
                    }

                    head_seq += last_index;
                    
                    for(index = 0; index < N; ++index) {
                        if(index + last_index < N) {
                            recv_flag[index] = recv_flag[index + last_index];
                        } else {
                            recv_flag[index] = 0;
                        }
                    }
                    printf("head:%d, ", head_seq);
                    printf("send ack\n");
                    show_flag(recv_flag);

                    send_wrapper(sockfd, &clientaddr, seqnum, 0x00);
                }
                break;
        }
    }
    
    close(sockfd);

    return 0;
}

