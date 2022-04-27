# CS 375 Project 1

Replace this file with your document explaining your design, etc.
-------------RUN------------------
1)command: hostname, get the hostname of the server, for example "ubuntu"
2)run server:   ./server
3)run client1: ./client serverhostname
4)run client2: ./client serverhostname
5)start talking....
6)input "EXIT" in any client, then all three processes exit.
  ctrl+c in in any client, then all three processes exit.

-----------DESIGN-----------------
1 the client and the sever use the same header file common.h,
  the server port is SERVERPORT = 39999; 

2 the server start first, it bind the socket the INADDR_ANY ip
  the server waiting for the first and second request                                  
                                                                                        
3 client convert the hostname to sockaddr_in struct
  Client1 starts and sends a request to the server                     
  Client2 starts and sends a request to the server                   

4 The server get two requests from two clients, then it sends 
  "SECOND" to client1 and "FIRST" to client2.                           

5 when the client gets the other's name from the server, they can talking.
  The client uses select to listen from both stdin and socket.
  When the stdin is ready, get the user input and send it to the server.
  When the socket is ready, read the message and display it.

6 When the server gets one message from a client, it finds the client address and
  transfers the message to another client.

7 When the user input "EXIT", the client sends the message to the server and exits.
  When the server receives "EXIT" from one client, it transfers the message to another 
  client and exits.
  When another client receives "EXIT", it exits.

8 I make a signal handler function. 
  When caught SIGINT(ctrl+C), it sends "EXIT" to server and the client exits.

