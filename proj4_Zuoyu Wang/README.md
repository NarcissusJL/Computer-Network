# CS 375 Project 4

-------------RUN------------------
1)command: hostname, get the hostname of the server, for example "ubuntu"
2)run server:  ./server
3)run client: ./client ubuntu
5)input and send some messages at the client
6)click ctrl+c at the client, then the client exits.
the messages for the client with be put in a txt in the server directory.
7) run another client

-----------DESIGN for server----------------
1 the client and the sever use the same header file common.h,
  the server port is SERVERPORT = 39999; 

2 the server start first, it bind the socket the INADDR_ANY ip.
  the server waiting the packet.

3 There are some variables in the server.
  (1)N - slide window size
  (2)char recv_flag[N] - the expeceted packet has recevived or not, 1 is received and 0 is not.
  (3)uint32_t head_seq, the first expeceted packet seqnum.
  (4)map<uint32_t, char*> recv_buffer - the received buffer of the client.
  
3 when the server receives an packet, it check the control byte.

4 If control byte is 01, it means a client makes connection setup.
  The server creates a txt file, with ip_port.txt as the file name. 
  "ip" is the IP of the client, "port" is the port of the client.  
  The server initializes all recv_flag chars with zero 0.
  If the first packet no is seqnum, sets head_seq = seqnum+1. 
  The server sends back ACK packet.

5 If control byte is 02, it means a client makes connection teardown.
  The server closes the txt file descriptor for the client. 
  The server sends back ACK packet.

6 If control byte is 00, it means a client sends data packet.
  (a)If the seqnum of the packet is less than head_seq, it is an old packet.
  The server sends back ACK packet. 
  (b) If the seqnum of the packet is equal or more than head_seq+N, the client sends too fast.
    The server just ignores the packet.
  (c) If the seqnum of the packet is between head_seq(including) and head_seq+N(excluding).
  The server sends back ACK packet. 
       If the recv_flag[seqnum-head_seq] is zero, save the data to recv_buffer and set recv_flag=1
       If there is "no hole" in the sequence of the packet, write the contineous packet data to file and delete the data in recv_buffer.
       Update the head_seq and recv_flag.   

7 Before sending back ACK,  randomly loss the ACK. if(rand() % 3 == 0) 
 
8 example of the client message txt:
more 127.0.0.1_33001.txt
1:ab
2:c
3:d
4:e
5:f
6:g
7:kk
8:kk
9:tt
 -----------DESIGN for client---------------- 
1 I make a signal handler function. 
  When caught SIGINT(ctrl+C), it sends connection teardown packet to server and waits for ACK.
  It exits after getting ACK.
  If it cannot get ACK in TWO SECONDS, it sends teardown packet again. 
  The clients trys at most three times then exits.

2 When starts up, the client send connection setup packet to server and waits for ACK.
  If it cannot get ACK in TWO SECONDS, it sends connection setup packet again. 
  The clients trys at most three times then exits.
  If it gets ACK, it prompts user to input the some message. One line for each message.
  If the screen shows "input 1:", the user can input a message(one line).
  If there is no "input *" in the screen, the user should not input anything.
  
3 There are some variables in the client.
   N is the slide window size of the client.
  (1) set<uint32_t> resend set - with all timeout packet(cannot get ACK in THREE SECONDS).
  (2) map<uint32_t, time_t> suspending - the packet already sent out and no ACK. 
         It stores the seqnum and the sendout time. 
  (3) map<uint32_t, char*> suspending_msg, the packet already sent out and no ACK. 
        It stores the seqnum and the packet data.   

4 while until the ctrl+C SIGINT signal.
  (4.1) IF resend set is empty, put the socket fd in watchlist.
        If the suspending size is less than N, also put stdin in watchlist, prompts the user to input one line message.
	Select for watchlist with ONE SECOND time out.
	(4.1) IF the select times out, scan all suspending packets and check the time, 
	      put the THREE SECONDS packet into resend set, goto 4.
        (4.2) The user input a message, update the seqnum and send it to server. 
              Add the (seqnum,time) pair to suspending.
	      Add the (seqnum, data) pair to suspending msg, goto 4.
	(4.3) The socket fd is readable, read the ACK seqnum.
	      If the seqnum is in suspending, delete it from both suspending and suspending msg.
	      If the seqnum is in resend set, delete it from resend set.
	
  (4.2) resend set is not empty, the client sends out all packets in resend set and updates the sendtime in suspending map.
         Clear the resend set.
---------------                          

