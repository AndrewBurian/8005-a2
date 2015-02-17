#include "client.h"

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

  // for epoll wait events
  struct epoll_event* events;
  int fdCount = 0;

  // the arrays of times
  struct timeval* startTimes = 0;
  struct timeval* endTimes = 0;

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

  // allocate the epoll events
  events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * test->clients);

  // allocate the dump buffer bigger than expected for disagreement
  dumpBuf = (char*)malloc(test->bufLen * 1.5);

  // begin main test loop
  for(i = 0; i < test->iterations && test->code == 0; ++i){

    // send the data to all sockets
    for(j = 0; j < test->clients; ++j){

      // send
      send(test->sockets[j], test->dataBuf, test->bufLen, 0);

      // timestamp outbound
      gettimeofday(&startTimes[j], 0);
    }

    // track the responses expected
    repliesLeft = test->clients;

    // epoll on the response data
    while(repliesLeft && test->code == 0){

      // epoll on descriptors up to listen limit with no timeout
      fdCount = epoll_wait (test->epollfd, events, test->clients, 10000);

      // check for timeout
      if(fdCount == 0){
        test->code = 101;
        break;
      }

      // loop through active events
      for(j = 0; j < fdCount; ++j){

        // Check this event for error
        if(events[j].events & EPOLLERR){
          fprintf(stderr, "Error on socket %d\n", events[i].data.fd);
          test->code = 201;
          break;
        }

        // check for hangup
        if(events[j].events & EPOLLHUP){
          test->code = 104;
          break;
          // control will exit the events loop
          // fail the epoll wait loop
          // break out of the main loop
          // and exit cleanly
        }

        // data has been received

        // determine which socket
        for(k = 0; k < test->clients; ++k){
          if(test->sockets[k] == events[j].data.fd){
            break;
          }
        }

        // timestamp
        gettimeofday(&endTimes[k], 0);

        // clear the data
        if(recv(test->sockets[k], dumpBuf, test->bufLen * 1.5, 0) != test->bufLen){
          fprintf(stderr, "Receive buffer size disagreement\n");
          test->code = 105;
          break;
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
      if(endTimes[j].tv_sec > test->high.tv_sec ||
        (endTimes[j].tv_sec == test->high.tv_sec &&
        endTimes[j].tv_usec > test->high.tv_usec)){

        test->high.tv_sec = endTimes[j].tv_sec;
        test->high.tv_usec = endTimes[j].tv_usec;
      }

      // check for new lowest
      if(endTimes[j].tv_sec < test->low.tv_sec ||
        (endTimes[j].tv_sec == test->low.tv_sec &&
        endTimes[j].tv_usec < test->low.tv_usec)){

        test->low.tv_sec = endTimes[j].tv_sec;
        test->low.tv_usec = endTimes[j].tv_usec;
      }

      // add the cumulative
      test->cumulative.tv_sec += endTimes[j].tv_sec;
      test->cumulative.tv_usec += endTimes[j].tv_usec;
      if(test->cumulative.tv_usec > 1000000){
        test->cumulative.tv_usec -= 1000000;
        test->cumulative.tv_sec += 1;
      }

    }


  } // end main test loop

  free(events);
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
