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
#include <time.h>
#include <map>
#include <set>

// slide window size
#define N 3

/***
 * send out the message to remote addr
 * 
 * @param sockfd sockect 
 * @param addr send to this remote address
 * @param message send this message
 * @param length the length of the message
 */
void send_wrapper(int sockfd, struct sockaddr_in* addr, const char* message, ssize_t length) {
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
 * the length the message is 8
 *
 * @param sockfd sockect 
 * @param message save the message here
 * */
void recv_wrapper(int sockfd, char* message) {
    ssize_t recv_length = 0;
    ssize_t msglen = 8;
    while(recv_length < msglen) {
        ssize_t tmp_length = recvfrom(sockfd, message + recv_length, msglen - recv_length, 0, NULL, NULL);
        if(tmp_length <= 0) {
            perror(strerror(errno));
            close(sockfd);
            exit(1);
        }
        recv_length += tmp_length;
    }
}


/** create a packet */
char* packet(uint32_t seqnum, 
             unsigned char ack, //0x01 is ACK, 0x00 not 
             unsigned char control, //00data, 01connect, 02teardown
             unsigned short length,
             const char* data) {
    char* message = (char*)malloc(8 + length);
    int seqnum_net = htonl(seqnum);
    int length_net = htons(length);
    memcpy(message, &seqnum_net, 4);
    message[4] = ack;
    message[5] = control;
    memcpy(message + 6, &length_net, 2);
    if(data != NULL) {
        memcpy(message + 8, data, length);
    }
    return message;
}

/** send out */
class Sender {
    public:
        Sender(int fd, struct sockaddr_in* pservaddr) {
            this->fd = fd;
            seqnum = 0;
            this->pserv = pservaddr;
        }

        ~Sender() {
            close(fd);
            for(std::map<uint32_t, char*>::iterator iter = suspending_msg.begin(); iter != suspending_msg.end(); ++iter) {
                free(iter->second);
            }
            suspending_msg.clear();
            suspending.clear();
            resend.clear();
        }

        uint32_t connect_setup() {
            // send setup packet
            seqnum = 0; 
            char* message = packet(seqnum, 0x00, 0x01, 0, NULL);
            send_wrapper(fd, pserv, message, 8);
            return seqnum++; //return the seqnum
        }
        uint32_t connect_teardown() {
            // send tear down ticket
            char* message = packet(seqnum, 0x00, 0x02, 0, NULL);
            send_wrapper(fd, pserv, message, 8);
            return seqnum; //return the seqnum
        }

        /** send one data packet to server*/
        uint32_t send_data() {
            char data[1024];
            int ret = scanf("%[^\n]", data); //the message including blank
            if(ret <= 0) {
                raise(SIGINT);
            }
            char* message = packet(seqnum, 0x00, 0x00, strlen(data), data);
            // save it in suspending and suspending_msg
            suspending.insert(std::pair<uint32_t, time_t>(seqnum, time(NULL)));
            suspending_msg.insert(std::pair<uint32_t, char*>(seqnum, message));
            send_wrapper(fd, pserv, message, 8 + strlen(data));
            printf("send %d to server\n", seqnum);

            if(getchar() == EOF) { // the last \n char
                raise(SIGINT);  
            }
            return seqnum++;
        }

        bool isempty_resend() {
            return resend.empty();
        }

        /** with at most N suspending packet*/
        bool can_send() {
            return suspending.size() < N;
        }

        
        void do_ack(uint32_t seqack) {
            // find the seqnum in suspending
            if(suspending.find(seqack) != suspending.end()) {
                suspending.erase(seqack); //remove the seqnum
                free(suspending_msg.at(seqack));
                suspending_msg.erase(seqack);

                // find it in resend and remove it
                if(resend.find(seqack) != resend.end()) {
                    resend.erase(seqack);
                }
            }
        }

        uint32_t get_seqnum() {
            return seqnum;
        }

        /** scan the suspending map, put the time out seqnum into resend set*/
        void time_out() {
            time_t curr = time(NULL);
            for(std::map<uint32_t, time_t>::const_iterator iter = suspending.begin(); iter != suspending.end(); ++iter) {
                if(curr - iter->second > 3 && resend.find(iter->first) == resend.end()) {
                    resend.insert(iter->first);
                }
            }
        }

        /** send all packets in resend set
         * update the send time in suspending map
         * clear resend set*/
        void resend_msg() {
            time_t curr = time(NULL);
            for(std::set<uint32_t>::const_iterator iter = resend.begin(); iter != resend.end(); ++iter) {
                char* message = suspending_msg[*iter];
                uint16_t length = htons(*((short*)(message+6)));
                printf("resend %d\n", *iter);
                send_wrapper(fd, pserv, message, 8 + length);
                suspending[*iter] = curr; //update current time
            }
            resend.clear();
        }

    private:
        int fd; //udp socket fd
        uint32_t seqnum; //seqnum
        struct sockaddr_in* pserv; // server address
        std::map<uint32_t, time_t> suspending; //<seqnum, send time>, the packets are sent out but not ACK
        std::map<uint32_t, char*> suspending_msg; // <seqnum, data>, the packets are sent out but not ACK
        std::set<uint32_t> resend; //<seqnum> time out packets(no ACK in three seconds)
};

/** receive ACK*/
class Receiver {
    public:
        Receiver(int fd, struct sockaddr_in* pservaddr) {
            this->fd = fd;
            this->pserv = pservaddr;
        }
        ~Receiver() {
            close(fd);
        }


        bool expect_ack(uint32_t seqnum) {
            fd_set watch_socks;
            struct timeval tv;

            char message[8];
            // wait two seconds for the ack
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            FD_ZERO(&watch_socks);
            FD_SET(fd, &watch_socks); // //put socket to watchlist
            int ret = select(fd + 1, &watch_socks, NULL, NULL, &tv);
            if(ret <= 0) {
                return false;
            }
            if(FD_ISSET(fd, &watch_socks)) {
                recv_wrapper(fd, message);
                return (uint32_t)ntohl(*((int*)message)) == seqnum;
            }
            return false;
        }

        uint32_t get_ack() {
            char message[8];
            recv_wrapper(fd, message);
            return ntohl(*(int*)message);
        }

    private:
        int fd;
        struct sockaddr_in* pserv;
};

Sender* sender = NULL;
Receiver* receiver = NULL;

/***
 *  * signal handler
 *   */
void sig_handler(int s) {
    printf("caught signal %d\n", s);
    if(s == SIGINT) {
        //at most try three times for the ack
        for(int i = 0; i < 3; ++i) {
            if(receiver->expect_ack(sender->connect_teardown())) {
                break;
            }
        }
        printf("Bye bye!\n");
        if(sender != NULL) {
            delete sender;
        }
        if(receiver != NULL) {
            delete receiver;
        }
        exit(0);
    }
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

    struct sigaction sighandler;
    sighandler.sa_handler = sig_handler;
    sigemptyset(&sighandler.sa_mask);
    sighandler.sa_flags = 0;
    sigaction(SIGINT, &sighandler, NULL);

    struct sockaddr_in servaddr;
    struct hostent *server_host = gethostbyname(argv[1]);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = *((unsigned long *)server_host->h_addr);//inet_addr(argv[1]);
    servaddr.sin_port = htons(SERVERPORT);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        perror(strerror(errno));
        exit(1);
    }

    sender = new Sender(sockfd, &servaddr);
    receiver = new Receiver(sockfd, &servaddr);
    int i = 0;
    while(i < 3) {
        if(receiver->expect_ack(sender->connect_setup())) {
            break;
        }
        ++i;
    }
    if(i == 3) {
        printf("fail to connect to server\n");
        exit(1);
    }
    //printf("success connect with server\n");
    fd_set watch_socks;

    struct timeval tv;
    int ret;
    int last_status = -1, curr_status;
    uint32_t seqnum;
    while(true) {
        if(sender->isempty_resend()) {
            FD_ZERO(&watch_socks);
            FD_SET(sockfd, &watch_socks); // //put socket to watchlist
            curr_status = sender->can_send()? 1 : 0;
            if(curr_status == 1) { //put stdin to watchlist
                FD_SET(0, &watch_socks); //set stdin
            }
            if(curr_status != last_status) {
                last_status = curr_status;
                if(curr_status == 1) {
                    printf("input %d: \n", sender->get_seqnum());
                } else {
                    printf("waiting for ack\n");
                }
            }

            tv.tv_sec = 1;
            tv.tv_usec = 0;
            ret = select(sockfd + 1, &watch_socks, NULL, NULL, &tv);
            if(ret < 0) {
                break;
            }

            if(ret == 0) { //timeout
                sender->time_out();
            } else if(FD_ISSET(0, &watch_socks)) { //user input a message
                sender->send_data();
                last_status = -1;
            } else if(FD_ISSET(sockfd, &watch_socks)) { //sockefd is readable
                seqnum = receiver->get_ack(); 
                sender->do_ack(seqnum);
                last_status = -1;
            }

        } else { //resend something
            sender->resend_msg();
            last_status = -1;
        }
    }

    delete sender;
    delete receiver;
  
  return 0;
}
