#ifndef SERVER_H
#define SERVER_H


// includes
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>

// defines
#define DEFAULT_PORT 7000

// different servers
int poll_server(int port);
int select_server(int port, int threads);
int epoll_server(int port, int threads);

// common functions
int create_server_socket(int serverPort);

#endif
