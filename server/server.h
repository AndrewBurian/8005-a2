/* ----------------------------------------------------------------------------
HEADER FILE

Name:		server.h

Program:	C10K

Developer:	Andrew Burian

Created On:	2015-02-13

Description:
	A server with three different socket management strategies for comparing
  performance across numerous connections

Revisions:
  (none)

---------------------------------------------------------------------------- */

#ifndef SERVER_H
#define SERVER_H


// includes
#define _GNU_SOURCE
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

// defines
#define DEFAULT_PORT 7000
#define BUFFER_SIZE 1024
#define LISTEN_LIMIT 128

// different servers
int poll_server(int port);
int select_server(int port);
int epoll_server(int port, int threads);

// common functions
int create_server_socket(int serverPort);

#endif
