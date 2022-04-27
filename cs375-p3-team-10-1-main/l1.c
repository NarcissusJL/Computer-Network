// l1.c

#include <unistd.h>

//-------------------------------------------------------------------------
// Layer 1: Simple inplementation.  
//          Provides half-duplex communication by using the shell to 
//          create a pipe between a sender process and a receiver process.
//-------------------------------------------------------------------------
// Function: l1_read
// Simply reads a single character on stdin

int l1_read(char* b) 
{
  return(read(STDIN_FILENO, b, 1));
}

//-------------------------------------------------------------------------
// Function: l1_write
// Write the given character to stdout

int l1_write(char b) 
{
  return(write(STDOUT_FILENO, &b, 1));
}
