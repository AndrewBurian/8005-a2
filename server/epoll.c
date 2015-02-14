/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		select.c

Program:	  C10K

Developer:	Andrew Burian

Created On:	2015-02-11

Functions:
  int epoll_server(int port, int threads)
  void* epoll_thread(void* epollfdptr)

Description:
  A multiplexing server centered around edge-triggered epoll, which can be
  safely run on multiple threads concurrently

Revisions:
  (none)

---------------------------------------------------------------------------- */

#include "server.h"

// function prototype for the worker
void* epoll_thread(void* args);

// the socket that will listen for connections
int listenSocket = 0;

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Epoll Server

Prototype:	int epoll_server(int port, int threads)

Developer:	Andrew Burian

Created On:	2015-02-11

Parameters:
  int port
    the port to listen on
  int threads
    the number of concurrent workers to run

Return Values:
  0   success
  -1  error conditions

Description:
  A simple epoll-centered echo server

Revisions:
  (none)

---------------------------------------------------------------------------- */
int epoll_server(int port, int threads){

  // the epoll instance descriptor
  int epollfd = 0;

  // the array of thread pointers
  pthread_t* threadPnts = 0;

  // counter
  int i = 0;

  // single epoll event
  struct epoll_event event = {0};

  // create the epoll descriptor
  if((epollfd = epoll_create1(0)) == -1){
    perror("Epoll create failed");
    return -1;
  }

  // create the socket (common)
  if((listenSocket = create_server_socket(port)) == -1){
    return -1;
  }

  // listen on the socket
  listen(listenSocket, LISTEN_LIMIT);

  // Set listen socket to non-blocking while keeping previous flags
  fcntl(listenSocket, F_SETFL, O_NONBLOCK | fcntl(listenSocket, F_GETFL, 0));

  // register listenSocket with epoll
  event.events = EPOLLIN | EPOLLERR | EPOLLET;
  event.data.fd = listenSocket;
  if(epoll_ctl(epollfd, EPOLL_CTL_ADD, listenSocket, &event) == -1){
    perror("epoll_ctl: conn_sock");
    return -1;
  }

  // allocate space for thread pointers
  threadPnts = (pthread_t*)malloc(sizeof(pthread_t) * threads);

  // create threads
  for(i = 0; i < threads; ++i){
   if(pthread_create(&threadPnts[i], 0, epoll_thread, &epollfd)){
     perror("Thread create failed");
     free(threadPnts);
     return -1;
   }
  }

  // wait on threads to terminate
  for(i = 0; i < threads; ++i){
    pthread_join(threadPnts[i], 0);
  }

  // cleanup
  free(threadPnts);
  return 0;

}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Epoll Thread

Prototype:	void* epoll_thread(void* epollfdptr)

Developer:	Andrew Burian

Created On:	2015-02-12

Parameters:
  void* args
    an int* pointing to the epoll descriptor

Return Values:
  0   always null

Description:
  The driver thread for the epoll server

Revisions:
  (none)

---------------------------------------------------------------------------- */
void* epoll_thread(void* epollfdptr){

  // data buffer, unique to each thread
  char buffer[BUFFER_SIZE] = {0};
  int buflen = 0;
  int thisRead = 0;

  // array of events for waiting
  struct epoll_event events[LISTEN_LIMIT];

  // epoll even for adding sockets
  struct epoll_event addEvent = {0};

  // the listen socket
  int epollfd = *(int*)epollfdptr;

  // new socket for connections
  int newSocket = 0;
  struct sockaddr_in remote = {0};
  socklen_t remoteLen = 0;

  // count of live descriptors
  int fdCount = 0;

  // counter
  int i = 0;

  // initialize the new socket event so it only needs to be done once
  addEvent.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;

  // epoll loop
  while(1){

    // epoll on descriptors up to listen limit with no timeout
    fdCount = epoll_wait (epollfd, events, LISTEN_LIMIT, -1);

    // error check
    if(fdCount == -1){
      perror("Epoll failed");
      return 0;
    }

    // loop through active events
    for(i = 0; i < fdCount; ++i){

      // Check this event for error
      if(events[i].events & EPOLLERR){
        fprintf(stderr, "Error on socket %d\n", events[i].data.fd);
        close(events[i].data.fd);
        // epoll will remove interest automatically on close
        continue;
      }

      // check for hangup
      if(events[i].events & EPOLLHUP){
        close(events[i].data.fd);
        // epoll will remove interest automatically on close
        continue;
      }

      // check for data
      if(events[i].events & EPOLLIN){

        // see if this is an accept
        if(events[i].data.fd == listenSocket){

          // loop accepting new connections and setting them non-blocking
          while((newSocket = accept4(listenSocket, (struct sockaddr*)&remote,
            &remoteLen, SOCK_NONBLOCK)) != -1){

            // add the new socket to the epoll set
            // events flags are already set to IN | ERR | HUP | ET
            addEvent.data.fd = newSocket;

            if(epoll_ctl(epollfd, EPOLL_CTL_ADD, newSocket, &addEvent) == -1){
              perror("Epoll ctl failed with new socket");
              return 0;
            }
          }

          // check to make sure the error was as expected
          if(errno != EAGAIN || errno != EWOULDBLOCK){
            perror("Error on accept4");
            return 0;
          }

          // done accepting, next socket
          continue;

        } // end of accept handler

        //read data from the socket
        buflen = 0;
        while((thisRead = recv(events[i].data.fd, &buffer[buflen],
          BUFFER_SIZE - buflen, 0)) > 0){

          // space taken from the
          buflen += thisRead;

          // check to see if the buffer is full
          if(buflen == BUFFER_SIZE){
            // send and flush
            send(events[i].data.fd, buffer, buflen, 0);
            buflen = 0;
          }
        }
        // end of reading
        if(thisRead == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
          // socket data exhaused with no read error
          send(events[i].data.fd, buffer, buflen, 0);
        } else if (thisRead == 0){
          // socket performed a shutdown
          close(events[i].data.fd);
          // epoll will remove interest automatically on close
        } else {
          // some error has occured
          perror("Receive error");
          return 0;
        }

      } // end of data handler

    } // end of events loop

  }

  return 0;
}
