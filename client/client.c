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

    printf("New master\n");

    // now connected to the controller,
    if(!prepTest(controllerSocket)){
      // run test returned 0, meaning a kill was received, break inf loop
      break;
    }
  }

  // done
  return 0;
}
