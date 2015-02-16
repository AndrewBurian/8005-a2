#include "client.h"

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Prep Test

Prototype:	int prepTest(int controllerSocket)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
  int controllerSocket
    The socket linked to the client controller

Return Values:
  1  continue looping
  0  kill received, terminate

Description:
  Handles the interfacing between the controller and this client to prepare for
  a server test

Revisions:
  (none)

---------------------------------------------------------------------------- */
int prepTest(int controllerSocket){

  // the test data struct to accumulate
  struct testData test = {{0}, 0, 0, 0, 0, 0, 0,
    {0}, {0}, {0}, 0};

  // boolean ready to test flag
  int ready = 0;

  // boolean for what parts of the test are ready
  int bufSet = 0;
  int serverSet = 0;
  int clientsSet = 0;
  int iterationsSet = 0;

  // boolean controller killed flag
  int killed = 0;

  // command buffers
  char cmd[32] = {0};
  char cArg[32] = {0};
  int iArg = 0;

  // message buffer for talking to the controller
  char msgBuf[256] = {0};

  // counters for reading in a loop
  int bufPos = 0;
  int thisRead = 0;

  // general counter
  int i = 0;

  // setup epoll
  if((test.epollfd = epoll_create1(0)) == -1){
    perror("Epoll create failed");
    // fatal
    exit(-1);
  }

  // loop talking to the controller until it's done
  while(1){

    // check to see if the test is ready
    if(bufSet && serverSet && clientsSet && iterationsSet){
      ready = 1;
    }

    // reset buf pos
    bufPos = 0;

    // read the message, terribly inefficiently, but safe and targeting \n
    while((thisRead = recv(controllerSocket, &msgBuf[bufPos], 1, 0)) > 0){
      // check to see if \n was reached and inc bufPos after
      if(msgBuf[bufPos++] == '\n'){
        break;
      }
      if(bufPos == 256){
        // message too long
        break;
      }
    }

    // if read was 0, controller has disconnected for some reason
    if(!thisRead){
      break;
    }

    // check for too long
    if(bufPos == 256){
      continue;
    }

    // parse command
    if(sscanf(msgBuf, "%32s", cmd) != 1){
      // error
      continue;
    }

    // command arguments
    if(strncmp(cmd, "TARGET", 32) == 0){
      // TARGET 192.168.0.0 8000
      if(sscanf(msgBuf, "%32s %32s %d", cmd, cArg, &iArg) != 3){
        continue;
      }
      test.server.sin_family = AF_INET;
      test.server.sin_port = htons(iArg);
      test.server.sin_addr.s_addr = inet_addr(cArg);
      serverSet = 1;
      continue;
    }

    if(strncmp(cmd, "SIZE", 32) == 0){
      // SIZE 23
      if(sscanf(msgBuf, "%32s %d", cmd, &iArg) != 2){
        continue;
      }
      // allocate buffer
      test.bufLen = iArg;
      test.dataBuf = (char*)malloc(iArg);
      // fill with alphabet, because why not
      for(i = 0; i < iArg; ++i){
        test.dataBuf[i] = (i % 26) + 65;
      }
      bufSet = 1;
      continue;
    }

    if(strncmp(cmd, "COUNT", 32) == 0){
      // COUNT 5
      if(sscanf(msgBuf, "%32s %d", cmd, &iArg) != 2){
        continue;
      }

      // make new client connections if nessesary
      if(create_and_connect(&test, iArg) == -1)
      {
        clientsSet = 0;
        continue;
      }

      // update the number of clients connected
      test.clients = iArg;
      clientsSet = 1;
      continue;
    }

    if(strncmp(cmd, "CYCLES", 32) == 0){
      // CYCLES 3
      if(sscanf(msgBuf, "%32s %d", cmd, &iArg) != 2){
        continue;
      }
      test.iterations = iArg;
      iterationsSet = 1;
      continue;
    }

    if(strncmp(cmd, "TEST", 32) == 0){
      if(!ready){
        send(controllerSocket, "ERR Not Ready to test\n", 22, 0);
        continue;
      }
      if(runTest(&test)){
        reportTest(controllerSocket, &test);
      } else {
        fprintf(stderr, "Test failed to run properly\n");
      }
    }

    if(strncmp(cmd, "DONE", 32) == 0){
      // break out of while loop
      break;
    }

    if(strncmp(cmd, "KILL", 32) == 0){
      killed = 1;
      break;
    }

  }

  // return to waiting for a connection
  printf("Released from master\n");
  close(controllerSocket);
  if(bufSet){
    free(test.dataBuf);
  }
  return (killed ? 0 : 1);;
}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Create And Connect

Prototype:	int create_and_connect(struct testData* test, int newSockCount)

Developer:	Andrew Burian

Created On:	2015-02-16

Parameters:
  struct testData* test
    the test data structure containing all the sockets to be created
  int
    the total of sockets to create up to

Description:
  Adds any new socket nessesary to the test data

Revisions:
  (none)

---------------------------------------------------------------------------- */
int create_and_connect(struct testData* test, int newSockCount){

  // counter
  int i = 0;

  // for adding new connections
  struct epoll_event addEvent = {0};

  // reuse addr enable
  int resueaddr = 1;


  // setup add events once
  addEvent.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;

  // create all the sockets
  for(i = test->clients; i < newSockCount; ++i){

    // create socket
    if((test->sockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
      perror("Socket create failed");
      // fatal
      exit(-1);
    }

    // set reuse addr to avoid network errors
    if(setsockopt(test->sockets[i], SOL_SOCKET, SO_REUSEADDR, &resueaddr, sizeof(int))
      == -1){

      perror("Failed to set SO_RCVLOWAT");
      //fatal
      exit(-1);
    }

    // set the recv low water mark to the expected echo size
    if(setsockopt(test->sockets[i], SOL_SOCKET, SO_RCVLOWAT, &test->bufLen, sizeof(int))
      == -1){

      perror("Failed to set SO_RCVLOWAT");
      //fatal
      exit(-1);
    }

    // add it to epoll
    addEvent.data.fd = test->sockets[i];
    if(epoll_ctl(test->epollfd, EPOLL_CTL_ADD, test->sockets[i], &addEvent) == -1){
      perror("Epoll ctl failed with new socket");
      // fatal
      exit(-1);
    }
  }


  // connect all the sockets
  for(i = test->clients; i < newSockCount; ++i){

    if(connect(test->sockets[i], (struct sockaddr*)&test->server,
      sizeof(test->server)) == -1){

      // failed to connect
      perror("Failed to connect");
      // check to see if it was a server error
      if(i > 0){
        if(errno == ETIMEDOUT){
          test->code = 2;
        } else if(errno == ECONNREFUSED){
          test->code = 3;
        }
        // shutdown anything that opened so far
        for(i = i; i >=0; --i){
          close(test->sockets[i]);
        }
        // break the for loop
        break;
        // nothing else will execute as the test code is checked
      } else {
        // probably a parameter error
        for(i = i; i >=0; --i){
          close(test->sockets[i]);
        }
        return -1;
      }
    }

    // make socket non-blocking
    // this is done after connect so connect won't error with E_INPROGRESS
    if(fcntl(test->sockets[i], F_SETFL, O_NONBLOCK |
        fcntl(test->sockets[i], F_GETFL, 0)) == -1){

      perror("Set non-blocking failed");
      // fatal
      exit(-1);
    }
  }

  return 1;

}
