/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		common.c

Program:	C10K

Developer:	Andrew Burian

Created On:	2015-02-11

Functions:
	Function prototypes

Description:
	Functions that are common to all types of servers

Revisions:
	(none)

---------------------------------------------------------------------------- */

#include "server.h"

int create_server_socket(int serverPort){

  int serverSocket = 0;
  int opt = 1;
  struct sockaddr_in local = {0};

  // Create the socket
  if((serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
    perror("Server Socket failed");
    return -1;
  }

  // set it to resuse addr for speed in repeated execution
  if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
    perror("Reuse addr failed");
    fprintf(stderr, "Not fatal. Continuing\n");
  }


  // set up the address
  local.sin_family = AF_INET;
  local.sin_port = htons(serverPort);
  local.sin_addr.s_addr = htonl(INADDR_ANY);

  // bind
  if(bind(serverSocket, (struct sockaddr*)&local, sizeof(local)) == -1){
    perror("Bind failed");
    return -1;
  }

  // done, ready to listen
  return serverSocket;
}
