/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		main.c

Program:	C10K - Client

Developer:	Andrew Burian

Created On: 2015-02-14

Functions:
	Function prototypes !!!!!!

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
        send(controllerSocket, "ERR Not Ready to test", 22, 0);
        continue;
      }
      runTest(&test);
      reportTest(controllerSocket, &test);
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
