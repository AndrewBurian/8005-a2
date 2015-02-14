/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		main.c

Program:	C10K

Developer:	Andrew Burian

Created On:	2015-02-11

Functions:
	int main(int argc, char** argv);

Description:
	The startup logic for the C10K server. Handles command line args and selects
  the appropriate server instance to launch

Revisions:
	(none)

---------------------------------------------------------------------------- */

#include "server.h"

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Main

Prototype:	int main(int argc, char** argv)

Developer:	Andrew Burian

Created On:	2015-02-11

Parameters:
	int argc
    argument count
  char** argv
    arguments

Return Values:
	0   success
  -1  error conditions

Description:
	Entry point for the program. Parses args and launches the right server

Revisions:
	(none)

---------------------------------------------------------------------------- */
int main(int argc, char** argv){


  // flags
  int usePoll = 0, useSelect = 0, useEpoll = 0;
  int threadCount = 0;
  int port = DEFAULT_PORT;

  // Command line argument setup
  struct option longOpts[] = {
      {"poll",        no_argument,        0,  'p'},
      {"select",      no_argument,        0,  's'},
      {"epoll",       no_argument,        0,  'e'},
      {"threads",     required_argument,  0,  't'},
      {"port",        required_argument,  0,  'l'},
      {0,0,0,0}
  };
  char* shortOpts = "pset:l";
  char c = 0;
  int optionIndex = 0;

  // process command line args
  while ((c = getopt_long(argc, argv, shortOpts,
      longOpts, &optionIndex)) != -1){
    switch (c){

    case 'p':
      usePoll = 1;
      break;

    case 's':
      useSelect = 1;
      break;

    case 'e':
      useEpoll = 1;
      break;

    case 't':
      if(sscanf(optarg, "%d", &threadCount) != 1){
        fprintf(stderr, "%s: Could not read thread count value\n", argv[0]);
        return -1;
      }
      break;

    case 'l':
      if(sscanf(optarg, "%d", &port) != 1){
        fprintf(stderr, "%s: Could not read port value\n", argv[0]);
        return -1;
      }
      break;

    default:
      // getoptlong will print its own error
      return -1;
    }
  }

  // check that argument combination is valid
  if((usePoll + useSelect + useEpoll) != 1){
    fprintf(stderr, "One server type may be specified\n");
    return -1;
  }

  // threads is not honored in polling server
  if((usePoll + useSelect) && threadCount){
    fprintf(stderr, "Warning: Attempting to set threads with non-epoll ignored\n");
  }

  // set threads to at least one
  threadCount = (threadCount ? threadCount : 1);

  // set the port to default if not set
  port = (port ? port : DEFAULT_PORT);

  if(usePoll){
    return poll_server(port);
  }

  if(useSelect){
    return select_server(port);
  }

  if(useEpoll){
    return epoll_server(port, threadCount);
  }

  // not sure how you'd get here
  return -1;
}
