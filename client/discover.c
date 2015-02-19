/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		discover.c

Program:	8005 - A2

Developer:	Andrew Burian

Created On:	2015-02-03

Functions:
	size_t discover(int broadcastPort, int serverPort, int* discoveredConnections,
    size_t maxConnections);
  int discoverable(int listenPort, int connectionPort);

Description:
	A function library designed for discovering peers over a boadcast enabled
  network

Revisions:
	(none)

---------------------------------------------------------------------------- */

#include "discover.h"

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Discover

Prototype:	size_t discover(int broadcastPort, int serverPort,
              int* discoveredConnections, size_t maxConnections);

Developer:	Andrew Burian

Created On:	2015-02-03

Parameters:
	int broadcastPort
      The port on which the searches will be broadcast to/from
  int serverPort
      The port on which connections will be established to
  int* discoveredConnections
      An array of descriptors whic should be pre-allocated to maxConnections in
      length, and will be filled with the connections as they are established
  size_t maxConnections
      The size of the array, and the maximum number of connections that will
      be established

Return Values:
	int   the number of connections established
  -1    error conditions

Description:
	Attempts to discover up to maxConnections number of connections via broadcast
  over the network. As discoverable hosts connect, they are added to the
  discoveredConnections array. It then returns the number of connections.

Revisions:
	(none)

---------------------------------------------------------------------------- */
int discover(int broadcastPort, int serverPort, int* discoveredConnections,
  size_t maxConnections, struct timeval timeout){

  // socket for broadcasting discovers on
  int broadcastSocket = 0;

  // socket for receiving response connections on
  int listenSocket = 0;

  // addressing structure (multi purpose)
  struct sockaddr_in address = {0};
  socklen_t addr_size = sizeof(address);

  // count of received connections
  int receivedConnections = 0;

  // argument for enabling socket options
  int arg = 1;

  // select structure for accepting with timeouts
  fd_set listenSet;

  // for capturing return of select
  int selectRet = 0;


  // validate parameters
  if(broadcastPort == serverPort){
    fprintf(stderr, "Broadcast and listen ports conflict\n");
    return -1;
  }
  if(!discoveredConnections){
    fprintf(stderr, "Null array for connections\n");
    return -1;
  }
  if(!maxConnections){
    // doing what was asked
    return maxConnections;
  }
  if(!timeout.tv_sec && !timeout.tv_usec){
    // default timeout of 3 seconds
    timeout.tv_sec = 3;
  }


  // set up the listening socket to recieve responses
  listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(listenSocket == -1){
    perror("Listening socket failed to create");
    return -1;
  }

  if(setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int))
    == -1){
    perror("Reuse Addr failed");
    close(listenSocket);
    return -1;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(serverPort);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(listenSocket, (struct sockaddr*)&address, addr_size) == -1){
    perror("Binding listen socket failed");
    close(listenSocket);
    return -1;
  }

  // create broadcasting socket
  broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(broadcastSocket == -1){
    perror("Broadcast socket failed to create");
    close(listenSocket);
    return -1;
  }
  if(setsockopt(broadcastSocket, SOL_SOCKET, SO_BROADCAST, &arg, sizeof(int))
    == -1){
    perror("Enabling broadcast failed");
    close(listenSocket);
    close(broadcastSocket);
    return -1;
  }

  // listen for incoming responses
  // as all responses could be received nearly instantly, listen for all
  listen(listenSocket, maxConnections);
  FD_ZERO(&listenSet);
  FD_SET(listenSocket, &listenSet);

  // broadcast listen port out for discovery
  address.sin_port = htons(broadcastPort);
  address.sin_addr.s_addr = inet_addr("255.255.255.255");
  if(sendto(broadcastSocket, &serverPort, sizeof(int), 0,
    (struct sockaddr*)&address, addr_size) == -1){

    perror("Broadcast failed to send");
    close(listenSocket);
    close(broadcastSocket);
    return -1;
  }

  // done with the broadcast socket
  close(broadcastSocket);

  // now keep selecting on the socket until the timout zeros
  while((selectRet = select(listenSocket + 1, &listenSet, 0, 0, &timeout)) > 0){

    // select has got an accept
    discoveredConnections[receivedConnections++] =
      accept(listenSocket, (struct sockaddr*)&address, &addr_size);
    printf("Discovered Connection at %s\n", inet_ntoa(address.sin_addr));
    if(receivedConnections == maxConnections){
      break;
    }

  }

  // check for select errors
  if(selectRet == -1){
    perror("Select failed");
    // as this is at the end, the listen socket will be closed anyways and
    // some connections may have been recieved before failure
  }

  // server socket is no longer needed
  close(listenSocket);

  return receivedConnections;
}


/* ----------------------------------------------------------------------------
FUNCTION

Name:		Discoverable

Prototype:	int discoverable(int listenPort, int connectionPort);

Developer:	Andrew Burian

Created On:	2015-02-03

Parameters:
	int listenPort
      The port on which to listen for incomming discovers

Return Values:
	int    A socket descriptor connected to the discoverer
  0      Timeout before discovered
  -1     Error conditions

Description:
	Begins listening for incoming broadcast discovers. If one is detected,
  connects back to the discoverer, and retuns a connected socket.

Revisions:
	(none)

---------------------------------------------------------------------------- */
int discoverable(int listenPort, struct timeval* timeout){

  // the socket to listen for broadcasts on
  int listenSocket = 0;

  // the socket to re-establish a connection on
  int connectSocket = 0;

  // the server port to respond to
  int responsePort = 0;

  // the address structure for capturing discoverer's address
  struct sockaddr_in address = {0};
  socklen_t addr_size = sizeof(address);

  // argument for enabling socket options
  int arg = 1;

  // select set for timeout listening
  fd_set listenSet;

  // boolean for if timeout should wait indefinetly
  int infTimeout = 0;


  // verify arguments
  if(!listenPort){
    fprintf(stderr, "Null listen port");
    return -1;
  }
  if(!timeout){
    infTimeout = 1;
  }

  // set up our listening socket
  listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(listenSocket == -1){
    perror("Listen socket failed to create");
    return -1;
  }
  FD_ZERO(&listenSet);
  FD_SET(listenSocket, &listenSet);

  // allow it to recieve broadcasts
  if(setsockopt(listenSocket, SOL_SOCKET, SO_BROADCAST, &arg, sizeof(int))
    == -1){
    perror("Enabling broadcast failed");
    close(listenSocket);
    return -1;
  }

  // bind listen socket
  address.sin_family = AF_INET;
  address.sin_port = htons(listenPort);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(listenSocket, (struct sockaddr*)&address, addr_size) == -1){
    perror("Failed to bind listen socket");
    close(listenSocket);
    return -1;
  }

  // while loop to allow for failed connect-backs
  while(1){

    // listen for incoming
    switch(select(listenSocket + 1, &listenSet, 0, 0, (infTimeout ? 0 : timeout)))
    {
      case -1:
        perror("Select failed");
        close(listenSocket);
        return -1;
      case 0:
        //timeout
        close(listenSocket);
        return 0;
    }

    // incoming broadcast
    recvfrom(listenSocket, &responsePort, sizeof(int), 0,
      (struct sockaddr*)&address, &addr_size);

    printf("Incoming discover from %s\n", inet_ntoa(address.sin_addr));

    // swap the source port for the response port
    address.sin_port = htons(responsePort);

    // create the connect socket
    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(connectSocket == -1){
      perror("Connect socket failed to create");
      close(listenSocket);
      return -1;
    }

    // connect back
    if(connect(connectSocket, (struct sockaddr*)&address, addr_size) == -1){
      perror("Could not connect to server");
      continue;
    }

    // done with listen socket
    close(listenSocket);

    break;
  }

  // return connected socket
  return connectSocket;
}
