/* ----------------------------------------------------------------------------
HEADER FILE

Name:		controller.h

Program:	C10K - Controller

Developer:	Andrew Burian

Created On:	2015-02-14

Description:
	Libs and declarations for the C10K controller

Revisions:
	(none)

---------------------------------------------------------------------------- */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "discover.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <math.h>

struct stats{
  double low;
  double high;
  double cumulative;
};

void sendAll(int* clients, int count, char* buf);
int recvAll(int* clients, int count, struct stats* testStats);

#endif
