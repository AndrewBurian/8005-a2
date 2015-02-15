/* ----------------------------------------------------------------------------
HEADER FILE

Name:		client.h

Program:	C10K - Client

Developer:	Andrew Burian

Created On:	2015-02-14

Description:
  A client designed to be controlled by the C10K controller, and to put pressure
  onto the C10K server, timing the responses and relaying the data back to the
  controller.

Revisions:
	(none)

---------------------------------------------------------------------------- */

#ifndef CLIENT_H
#define CLIENT_H

// includes
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>

#include "discover.h"

// defines
#define DEFAULT_DISCOVER_PORT 7002

// data types
struct testData{
  struct sockaddr_in  server;     // the server to test
  int                 clients;    // how many connections to use
  int                 iterations; // how many iterations to test
  char*               dataBuf;    // the data buffer to test with
  int                 bufLen;     // how long the data buffer is
  struct timeval      high;       // the longest response time
  struct timeval      low;        // the shortest response time
  struct timeval      cumulative; // the total response time
  int                 code;       // result code
};

// functions
int prepTest(int controllerSocket);
int runTest(struct testData* test);
void reportTest(int controllerSocket, struct testData* test);

#endif
