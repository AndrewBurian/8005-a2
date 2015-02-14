/* ----------------------------------------------------------------------------
HEADER FILE

Name:		discover.h

Program:	8005-A2

Developer:	Andrew Burian

Created On:	2015-02-03

Description:
	A simple library for discovering other hosts on the same network via
  broadcast

Revisions:
	(none)

---------------------------------------------------------------------------- */
#ifndef DISCOVER_H
#define DISCOVER_H

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>

int discover(int broadcastPort, int serverPort, int* discoveredConnections,
    size_t maxConnections, struct timeval timeout);

int discoverable(int listenPort, struct timeval timout);


#endif
