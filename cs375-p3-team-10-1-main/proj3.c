// CS 375 Computer Networks
// Project 3 Test Program
//
// Created 1/28/03, Thomas C. Bressoud
// Modified 2/7/05, Jessen Havill
//          2/17/22, Jessen Havill
//
// This code can be used to test you layer 4 implementation
//   (which in turn can/should be used to test your layer 3, etc.)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LEN 100
#define MAX_VALUE_LEN 100

//-------------------------------------------------------------------------
// Layer 4 Function prototypes
//-------------------------------------------------------------------------
extern int l4_write(char*, int, char*, int);
extern int l4_read(char*, int*, char*, int*);

//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
// Main program.  This program can be either a sender or a receiver.  The
//   command line arguments are used to determine which, with the sender
//   processing arguments for the values of a username and password.
//   The given layer 1 fucntions communicate over stdin/stdout, so we can 
//   connect a sender and receiver by having the shell set up a pipe between
//   them, as follows (assuming the generated executable is named proj3:
//
//   ./proj3 <username> <password> | ./proj3
//-------------------------------------------------------------------------
int main(int argc, char** argv) 
{
  char namebuf[MAX_NAME_LEN + 1];    // Receiver's buffer for name of name/value pair
  char valuebuf[MAX_VALUE_LEN + 1];  // Recevier's buffer for value of name/value pair
  int  namelen;                      // Storage for length of name
  int  valuelen;                     // Storage for lenght of value

  // If there are 2 command line arguments, then this program will send the
  //   first (argv[1]) as a username, and the second (argv[2]) as a
  //   password, using l4_write to send the name/value pair.
  // If there are not 2 command line arguments, the program assumes it should
  //   be the receiver, and so it will call l4_read to receive the name/value pair.

  // We begin with the sender:

  if (argc == 3) 
  {
    // I am the sender, so I need to use the command line arguments as the
    // values for the username and the password.

    // Check argument length and send username

    if (strlen(argv[1]) > MAX_NAME_LEN)   // Check validity of first argument
    {
      fprintf(stderr, "Error -- username is too long\n");
      exit(1);
    }

    // Argument length is ok, so send it.
    
    int result = l4_write("username", strlen("username") + 1, argv[1], strlen(argv[1]) + 1);
    if (!result) 
	{
      fprintf(stderr, "l4_write error sending username\n");
      exit(1);
    }

    // Check argument length and send password

    if (strlen(argv[2]) > MAX_VALUE_LEN)   // Check validity of second argument
    {
      fprintf(stderr, "Error -- password is too long\n");
      exit(1);
    }

    // Argument length is ok, so send it.
    
    result = l4_write("password", strlen("password") + 1, argv[2], strlen(argv[2]) + 1);
    if (!result) 
    {
      fprintf(stderr, "l4_write error sending password\n");
      exit(1);
    }
  } 
  else if (argc == 1)     // No arguments indicates the receiver
  {    
    // Set up to read the two name/value pairs
    //   start by setting up our in/out length parameters to the maximum size
    //   we are willing to tolerate.

    namelen  = MAX_NAME_LEN;
    valuelen = MAX_VALUE_LEN;

    // Use our layer 4 interface to read the first name/value pair
    
    int result = l4_read(namebuf, &namelen, valuebuf, &valuelen);
    if (!result) 
    {
      fprintf(stderr, "l4_read error on first name/value pair read\n");
      exit(1);
    }

    // we expect C strings, with appropriate null termination, but we make
    // sure that each is null terminated.  (A malicious peer could be trying
    // to mess us up.)

    namebuf[namelen] = '\0';
    valuebuf[valuelen] = '\0';

    // verify that the application-level protocol expectation of the first
    // name/value pair to be the username is well-founded.

    if (strcmp(namebuf, "username") != 0) 
    {
      fprintf(stderr, "Error -- first name/value pair was not \"username\"\n");
      exit(1);
    }

    // We got something named "username", so print it out

    printf("Received username: %s\n", valuebuf);

    // Use our layer 4 interface to read the second name/value pair

    namelen  = MAX_NAME_LEN;       // reinitialize our in/out parameters
    valuelen = MAX_VALUE_LEN;
    
    result = l4_read(namebuf, &namelen, valuebuf, &valuelen);
    if (!result) 
    {
      fprintf(stderr, "l4_read error on second name/value pair read\n");
      exit(1);
    }

    // Ensure null termination

    namebuf[namelen] = '\0';
    valuebuf[valuelen] = '\0';

    // verify that the application-level protocol expectation of the second
    // name/value pair to be the password is well-founded.

    if (strcmp(namebuf, "password") != 0) 
    {
      fprintf(stderr, "Error -- first name/value pair was not \"password\"\n");
      exit(1);
    }

    // We got something named "password", so print it out

    printf("Received password: %s\n", valuebuf);

  } 
  else                    // Usage error when argc is neither 1 nor 3
  {
    fprintf(stderr, "Receiver usage: %s\n", argv[0]);
    fprintf(stderr, "Sender usage: %s <username> <password>\n", argv[0]);
    exit(1);
  }
  
  return 0;
} 
