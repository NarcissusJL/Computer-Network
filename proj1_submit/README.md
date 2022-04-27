# CS 375 Project 1

Replace this file with your document explaining your design, etc.
1 the client and the sever use the same header file common.h,
  the server port is SERVERPORT = 39999; 

2 the server start first, it bind the socket the INADDR_ANY ip
  the server waiting for the first and second request                                  
                                                                                        
3 the first client(client1) start and send a request to the server                     
  the sencond client(client2) start and send a request to the server                   
         
4 the server send "1 SECOND" to client1 and "2 FIRST" to client2                           
  the server waits message from client1 and send the received message to client2.      
  then the server waits message from client2 and send the received message to client1.  

5 When client get "1 namexxx", it asks the user to enter a message.
  When client get "2 namexxx", it should receive message from others.
  client messages are strictly alternating. In other words, client 1 sends, then client 2 receives,
  then client 2 sends, then client 1 receives, etc.

6 If one client wants to exit, the user should enter "q" and send to server, the process exit.
  When the server receives message "q", it tells another client to exit. Then the server exit.
  When another client receives message "q", it exit.

