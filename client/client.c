/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		client.c

Program:	C10K - Client

Developer:	Andrew Burian

Created On: 2015-02-14

Functions:
	int main(int argc, char** argv)
  int prepTest(int controllerSocket)
  int runTest(struct testData* test)
  void reportTest(int controllerSocket, struct testData* test)

Description:
	A client designed to be controlled by the C10K controller, and to put pressure
  onto the C10K server, timing the responses and relaying the data back to the
  controller.

Revisions:
	(none)

---------------------------------------------------------------------------- */

// includes
#include "client.h"


/* ----------------------------------------------------------------------------
FUNCTION

Name:		Main

Prototype:	int main(int argc, char** argv)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
  int argc
    argument count
  char** argv
    arguments

Return Values:
  0   success
  -1  error conditions

Description:
  Entry point for the program. Parses args prepares the client

Revisions:
  (none)

---------------------------------------------------------------------------- */
int main(int argc, char** argv){

  // the port for being discoverable on
  int discoverPort = 0;

  // the socket for communicating with the controller
  int controllerSocket = 0;

  // Command line argument setup
  struct option longOpts[] = {
      {"port",        required_argument,  0,  'p'},
      {0,0,0,0}
  };
  char* shortOpts = "hp:";
  char c = 0;
  int optionIndex = 0;

  // process command line args
  while ((c = getopt_long(argc, argv, shortOpts,
      longOpts, &optionIndex)) != -1){
    switch (c){

      case 'p':
        if(sscanf(optarg, "%d", &discoverPort) != 1){
          fprintf(stderr, "Failed to read port argument\n");
          return -1;
        }
        break;

      default:
        // error
        return -1;
    }
  }

  // validate args
  discoverPort = (discoverPort ? discoverPort : DEFAULT_DISCOVER_PORT);

  // loop controller discovery and test running so this client may be reused
  while(1){

    // become discoverable for the controller
    if((controllerSocket = discoverable(discoverPort, 0)) == -1){
      // error
      return -1;
    }

    // now connected to the controller,
    if(!prepTest(controllerSocket)){
      // run test returned 0, meaning a kill was received, break inf loop
      break;
    }
  }

  // done
  return 0;
}

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
  struct testData test;

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

  // loop talking to the controller until it's done
  while(1){

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
    break;

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

    // check to see if the test is ready
    if(bufSet && serverSet && clientsSet && iterationsSet){
      ready = 1;
    }

  }

  // return to waiting for a connection
  close(controllerSocket);
  if(bufSet){
    free(test.dataBuf);
  }
  return (killed ? 0 : 1);;
}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Run Test

Prototype:	int runTest(struct testData* test)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
	struct testData* test
    the structure containing all the info needed to run the test

Return Values:
	1  Test completed successfully
  0  Test did not run, errors

Description:
	Run the provided test and return the results into the test structure
  Uses epoll to handle the responses as fast as possible

Revisions:
	(none)

---------------------------------------------------------------------------- */
int runTest(struct testData* test){

  // the epoll descriptor
  int epollfd = 0;

  // for epoll wait events
  struct epoll_event* events;
  int fdCount = 0;

  // for adding new connections
  struct epoll_event addEvent = {0};

  // the arrays of times
  struct timeval* startTimes = 0;
  struct timeval* endTimes = 0;

  // client sockets
  int* sockets = 0;

  // keep track of pending responses
  int repliesLeft = 0;

  // data dump buffer
  char* dumpBuf = 0;

  // iteration tracket
  int i = 0;
  int j = 0;
  int k = 0;


  // validate test data
  if(!test){
    return 0;
  }
  if(test->clients < 1){
    return 0;
  }
  if(test->iterations < 1){
    return 0;
  }
  if(!test->dataBuf){
    return 0;
  }
  if(test->bufLen < 1){
    return 0;
  }

  // set the test code to assume success for now
  test->code = 0;

  // set the low too high
  test->low.tv_sec = 9999;
  test->low.tv_usec = 0;

  // high and cumulative to zero
  test->high.tv_sec = 0;
  test->high.tv_usec = 0;
  test->cumulative.tv_sec = 0;
  test->cumulative.tv_usec = 0;

  // allocate the times
  startTimes = (struct timeval*)malloc(sizeof(struct timeval) * test->clients);
  endTimes = (struct timeval*)malloc(sizeof(struct timeval) * test->clients);

  // allocate the sockets
  sockets = (int*)malloc(sizeof(int) * test->clients);

  // allocate the epoll events
  events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * test->clients);

  // allocate the dump buffer
  dumpBuf = (char*)malloc(test->bufLen);

  // setup epoll
  if((epollfd = epoll_create1(0)) == -1){
    perror("Epoll create failed");
    // fatal
    exit(-1);
  }

  // setup add events once
  addEvent.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;

  // create all the sockets
  for(i = 0; i < test->clients; ++i){

    // create socket
    if((sockets[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
      perror("Socket create failed");
      // fatal
      exit(-1);
    }

    // make socket non-blocking
    fcntl(sockets[i], F_SETFL, O_NONBLOCK | fcntl(sockets[i], F_GETFL, 0));

    // set the recv low water mark to the expected echo size
    if(setsockopt(sockets[i], SOL_SOCKET, SO_RCVLOWAT, &test->bufLen, sizeof(int))
      == -1){

      perror("Failed to set SO_RCVLOWAT");
      //fatal
      exit(-1);
    }

    // add it to epoll
    addEvent.data.fd = sockets[i];
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockets[i], &addEvent) == -1){
      perror("Epoll ctl failed with new socket");
      // fatal
      exit(-1);
    }
  }


  // connect all the sockets
  for(i = 0; i < test->clients; ++i){

    if(connect(sockets[i], (struct sockaddr*)&test->server, sizeof(test->server))
      == -1){

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
          close(sockets[i]);
        }
        // break the for loop
        break;
        // nothing else will execute as the test code is checked
      } else {
        // probably a parameter error
        for(i = i; i >=0; --i){
          close(sockets[i]);
        }
        free(events);
        free(sockets);
        free(startTimes);
        free(endTimes);
        return 0;
      }
    }
  }


  // begin main test loop
  for(i = 0; i < test->iterations && test->code == 0; ++i){

    // send the data to all sockets
    for(j = 0; j < test->clients; ++j){

      // send
      send(sockets[j], test->dataBuf, test->bufLen, 0);

      // timestamp outbound
      gettimeofday(&startTimes[j], 0);
    }

    // track the responses expected
    repliesLeft = test->clients;

    // epoll on the response data
    while(repliesLeft && test->code == 0){

      // epoll on descriptors up to listen limit with no timeout
      fdCount = epoll_wait (epollfd, events, test->clients, -1);

      // loop through active events
      for(j = 0; j < fdCount; ++j){

        // Check this event for error
        if(events[j].events & EPOLLERR){
          fprintf(stderr, "Error on socket %d\n", events[i].data.fd);
          // fatal
          exit(-1);
        }

        // check for hangup
        if(events[j].events & EPOLLHUP){
          test->code = 4;
          break;
          // control will exit the events loop
          // fail the epoll wait loop
          // break out of the main loop
          // and exit cleanly
        }

        // data has been received

        // determine which socket
        for(k = 0; j < test->clients; ++j){
          if(sockets[k] == events[j].data.fd){
            break;
          }
        }

        // timestamp
        gettimeofday(&endTimes[k], 0);

        // clear the data
        if(recv(sockets[k], dumpBuf, test->bufLen, 0) != test->bufLen){
          fprintf(stderr, "Receive buffer size disagreement\n");
          // fatal
          exit(-1);
        }

        // note the reply
        --repliesLeft;

      }

    }

    // all replies received or failure

    // check failure
    if(test->code){
      break;
    }

    // time processing
    for(j = 0; j < test->clients; ++j){

      // process the difference in time
      endTimes[j].tv_sec -= startTimes[j].tv_sec;
      endTimes[j].tv_usec -= startTimes[j].tv_usec;

      // check for new highest
      if(endTimes[j].tv_sec >= test->high.tv_sec &&
        endTimes[j].tv_usec > test->high.tv_usec){

        test->high.tv_sec = endTimes[j].tv_sec;
        test->high.tv_usec = endTimes[j].tv_usec;
      }

      // check for new lowest
      if(endTimes[j].tv_sec <= test->low.tv_sec &&
        endTimes[j].tv_usec < test->low.tv_usec){

        test->low.tv_sec = endTimes[j].tv_sec;
        test->low.tv_usec = endTimes[j].tv_usec;
      }

      // add the cumulative
      test->cumulative.tv_sec += endTimes[j].tv_sec;
      test->cumulative.tv_usec += endTimes[j].tv_usec;

    }


  } // end main test loop


  // cleanup

  for(i = 0; i < test->clients; ++i){
    close(sockets[i]);
  }

  free(events);
  free(sockets);
  free(startTimes);
  free(endTimes);
  return 1;
}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Report Test

Prototype:	void reportTest(int controllerSocket, struct testData* test)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
	int controllerSocket
    the connection to the controller to report back to
  struct testData* test
    the test to report on

Description:
  Does all the long slow number crunching on the time and sends the data back
  to the controller

Revisions:
	(none)

---------------------------------------------------------------------------- */
void reportTest(int controllerSocket, struct testData* test){

  // the buffer for our response
  char response[45] = {0};

  // time values
  double min = 0;
  double max = 0;
  double cumulative = 0;

  // convert tv structs into a double
  min =  test->low.tv_sec * 1000000.0 + test->low.tv_usec;
  max =  test->high.tv_sec * 1000000.0 + test->high.tv_usec;
  cumulative =  test->cumulative.tv_sec * 1000000.0 + test->cumulative.tv_usec;

  // convert usec into ms
  min /= 1000.0;
  max /= 1000.0;
  cumulative /= 1000.0;

  // print it into a string
  sprintf(response, "RESULT %d %10.3f %10.3f %10.3f\n",
    test->code, min, max, cumulative);

  // send the string
  send(controllerSocket, response, strlen(response), 0);

}
