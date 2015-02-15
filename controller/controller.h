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
