/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		poll.c

Program:	  C10K

Developer:	Andrew Burian

Created On:	2015-02-11

Functions:
	int poll_server()
  void terminate(int sig)

Description:
	A polling server which forks a new process for each client

Revisions:
	(none)

---------------------------------------------------------------------------- */

#include "server.h"

void terminate(int sig);

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Poll Server

Prototype:	int poll_server()

Developer:	Andrew Burian

Created On:	2015-02-11

Parameters:
  int port
    the port to listen on

Return Values:
  0   success
  -1  error conditions

Description:
  A simple one process per client echo server

Revisions:
  (none)

---------------------------------------------------------------------------- */
int poll_server(int port){

  // the master listening socket
  int listenSocket = 0;

  // the client socket
  int newSocket = 0;

  // address struct and size for accept
  struct sockaddr_in remote = {0};
  socklen_t remotelen = 0;

  // data buffer of arbitrary size
  char buffer[BUFFER_SIZE];

  // length of data read
  int readData = 0;

  // create the server socket (common)
  if((listenSocket = create_server_socket(port)) == -1){
    return -1;
  }

  // this is a scalable server, listen big
  listen(listenSocket, 100);

  // before any children are spawned, put in place the signal handler
  // so that any children may be terminated properly
  signal(SIGINT, terminate);

  // loop accepting clients
  while(1){

    // accept a new client
    newSocket = accept(listenSocket, (struct sockaddr*)&remote, &remotelen);
    if(newSocket == -1){
      // error in accept
      perror("Accept failed");
      // terminate in case children have already started
      terminate(0);
    }

    // spool up process to handle client
    if(!fork()){

      // this now the child, remove signal handler
      signal(SIGINT, SIG_DFL);

      // read the data until the socket is closed
      while((readData = read(newSocket, buffer, BUFFER_SIZE))  > 0){
        // echo
        write(newSocket, buffer, readData);
      }

      // connection closed
      close(newSocket);
      return 0;
    }

  }

  return 0;
}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Terminate

Prototype:	void terminate(int sig)

Developer:	Andrew Burian

Created On:	2015-02-11

Parameters:
  int sig
    the signal captured

Description:
  Caputres SIGINT and terminates any running children

Revisions:
  (none)

---------------------------------------------------------------------------- */
void terminate(int sig){

  // kill children to avoid zombies
  fprintf(stderr, "\nSIGINT! Terminating children.\n");
  kill(0, SIGKILL);
  exit(1);

}
