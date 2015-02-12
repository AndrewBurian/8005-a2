#include "server.h"


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
  int listenSocket;



  // create the server socket (common)
  listenSocket = create_server_socket(port);

  // this is a scalable server, listen big
  listen(listenSocket, 100);


  return 0;
}
