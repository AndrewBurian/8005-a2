#ifndef SERVER_H
#define SERVER_H


// includes
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// defines
#define DEFAULT_PORT 7000
#define BUFFER_SIZE 1024

// different servers
int poll_server(int port);
int select_server(int port, int threads);
int epoll_server(int port, int threads);

// common functions
int create_server_socket(int serverPort);

#endif
