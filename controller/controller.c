/* ----------------------------------------------------------------------------
SOURCE FILE

Name:		controller.c

Program:	C10K - Controller

Developer:	Andrew Burian

Created On: 2015-02-14

Functions:
  int main(int argc, char** argv)
  void sendAll(int* clients, int count, char* buf)
  int recvAll(int* clients, int count, struct stats* testStats)

Description:
  A controller, which can discover and interface with any clients available
  to test the C10K server

Revisions:
  (none)

---------------------------------------------------------------------------- */

#include "controller.h"


/* ----------------------------------------------------------------------------
FUNCTION

Name:		Main

Prototype:	int main(int argc, char** argv)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
  int argc
    argument count
  char** argv
    arguments

Return Values:
  0   success
  -1  error conditions

Description:
  Entry point for the program. Parses args and discovers clients

Revisions:
  (none)

---------------------------------------------------------------------------- */
int main(int argc, char** argv){

  // port to discover clients on
  int discoverPort = 0;

  // port for clients to connect to
  int listenPort = 0;

  // target server address
  char* serverAddress = 0;

  // target server port
  int serverPort = 0;

  // data size to test with
  int dataSize = 0;

  // increment rate
  int increment = 0;

  // how many vollies of connections to send
  int vollies = 0;

  // max clients to discover
  int maxClients = 0;

  // clients array
  int* clients = 0;

  // starting connections to test server with
  int baseConnections = 0;

  // timeout for waiting for clients (10 seconds)
  struct timeval timeout = { 10, 0};

  // command buffer for clients
  char cmd[256] = {0};

  // response from the clients
  int responseCode = 0;

  // set how many connections each client should generate
  int totalConnections = 0;
  int connectionsPer = 0;

  // ouput file
  FILE* outputFile = 0;

  // time stats
  struct stats testStats = {0};

  // kill flag
  int kill = 0;


  // Command line argument setup
  struct option longOpts[] = {
      {"discover-port",   required_argument,  0,  'p'},
      {"server",          required_argument,  0,  'a'},
      {"data-size",       required_argument,  0,  's'},
      {"server-port",     required_argument,  0,  'l'},
      {"increment",       required_argument,  0,  'i'},
      {"clients",         required_argument,  0,  'c'},
      {"base-connects",   required_argument,  0,  'b'},
      {"vollies",         required_argument,  0,  'v'},
      {"output",          required_argument,  0,  'o'},
      {"kill",            no_argument,        0,  'k'},
      {0,0,0,0}
  };
  char* shortOpts = "p:a:s:l:i:c:b:v:o:k";
  char c = 0;
  int optionIndex = 0;

  // process command line args
  while ((c = getopt_long(argc, argv, shortOpts,
      longOpts, &optionIndex)) != -1){
    switch (c){

      case 'p':
        if(sscanf(optarg, "%d", &discoverPort) != 1){
          fprintf(stderr, "Failed to read discover port argument\n");
          return -1;
        }
        listenPort = discoverPort + 1;
        break;

      case 'a':
        serverAddress = optarg;
        break;

      case 's':
        if(sscanf(optarg, "%d", &dataSize) != 1){
          fprintf(stderr, "Failed to read data size argument\n");
          return -1;
        }
        break;

      case 'l':
        if(sscanf(optarg, "%d", &serverPort) != 1){
          fprintf(stderr, "Failed to read server port argument\n");
          return -1;
        }
        break;

      case 'i':
      if(sscanf(optarg, "%d", &increment) != 1){
        fprintf(stderr, "Failed to read increment argument\n");
        return -1;
      }
        break;

      case 'c':
      if(sscanf(optarg, "%d", &maxClients) != 1){
        fprintf(stderr, "Failed to read clients argument\n");
        return -1;
      }
        break;

      case 'b':
      if(sscanf(optarg, "%d", &baseConnections) != 1){
        fprintf(stderr, "Failed to read base connections argument\n");
        return -1;
      }
        break;

      case 'v':
      if(sscanf(optarg, "%d", &vollies) != 1){
        fprintf(stderr, "Failed to read vollies argument\n");
        return -1;
      }
        break;

      case 'o':
        if(!(outputFile = fopen(optarg, "w"))){
          perror("Output file failed to open");
          return -1;
        }
        break;

      case 'k':
        kill = 1;
        break;

      default:
        // error
        return -1;
    }
  }

  // validate args

  // if kill, exempt the unnessesary args
  if(kill && discoverPort && maxClients){
    serverPort = 1;
    increment = 1;
    baseConnections = 1;
    serverAddress = (char*)1;
    vollies = 1;
    dataSize = 1;
  }
  // if not kill, all must be set
  if(!discoverPort || !serverPort || !increment || !maxClients || !baseConnections
    || !serverAddress || !vollies || !dataSize){

    fprintf(stderr, "Invocation must contain all args\n"
    "discover-port\n"
    "server\n"
    "data-size\n"
    "server-port\n"
    "increment\n"
    "clients\n"
    "base-connects\n"
    "vollies\n");
    return -1;
  }

  // if no output file, use stdout
  outputFile = (outputFile ? outputFile : stdout);

  // allocate space for clients
  clients = (int*)malloc(sizeof(int) * maxClients);

  printf("Discovering Clients (max %d)... ", maxClients);
  fflush(stdout);

  // discover clients
  maxClients = discover(discoverPort, listenPort, clients, maxClients, timeout);

  printf("%d found\n", maxClients);

  if(!maxClients){
    printf("Failed to find any clients.\n");
    return 0;
  }

  // if this is just a kill, run and quit
  if(kill){
    printf("Killing clients\n");
    sprintf(cmd, "KILL\n%c", 0);
    sendAll(clients, maxClients, cmd);
    return 0;
  }

  // begin tests

  // first set the target
  printf("Setting target ip to %s\n", serverAddress);
  sprintf(cmd, "TARGET %16s %d\n%c", serverAddress, serverPort, 0);
  sendAll(clients, maxClients, cmd);

  // set the data size
  printf("Setting data size to %d\n", dataSize);
  sprintf(cmd, "SIZE %d\n%c", dataSize, 0);
  sendAll(clients, maxClients, cmd);

  // set the vollies
  printf("Testing in vollies of %d\n", vollies);
  sprintf(cmd, "CYCLES %d\n%c", vollies, 0);
  sendAll(clients, maxClients, cmd);

  // set initial connections
  totalConnections = baseConnections;

  printf("Beginning test\n");
  fprintf(outputFile, "Connections, minTime, maxTime, cumulative\n");

  // keep pushing until death!
  while(1){

    // calculate even total across clients
    connectionsPer = ceil(totalConnections / maxClients);
    totalConnections = connectionsPer * maxClients;

    // write to output
    fprintf(outputFile, "%6d,", totalConnections);

    // send to clients
    sprintf(cmd, "COUNT %d\n%c", connectionsPer, 0);
    sendAll(clients, maxClients, cmd);

    // run test
    sprintf(cmd, "TEST\n%c", 0);
    sendAll(clients, maxClients, cmd);

    // gather replies
    responseCode = recvAll(clients, maxClients, &testStats);

    // when there's an error, print and break
    if(responseCode){
      switch(responseCode){
        case 1:
          printf("Server stopped responding\n");
          break;
        case 2:
          printf("Server stopped connecting\n");
          break;
        case 3:
          printf("Server refused further connections\n");
          break;
        case 4:
          printf("Server disconnected\n");
          break;
        case -1:
          printf("Test Client Stopped unexpectedly\n");
          break;
      }
      break;
    }

    // no error
    fprintf(outputFile, "%10.3f, %10.3f, %10.3f\n", testStats.low,
      testStats.high, testStats.cumulative);

    // increase connections
    totalConnections += increment;

  }

  printf("Test done\n");
  sprintf(cmd, "DONE\n%c", 0);
  sendAll(clients, maxClients, cmd);

  return 0;
}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Send to All

Prototype:	void sendAll(int* clients, int count, char* buf)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
	int* clients
    the array of clients to send to
  int count
    the number of elements in the clients array
  char* buf
    the null terminated data buffer to send out

Description:
	Sends a null terminated buffer to all clients in the array, then zeros the
  data buffer

Revisions:
	(none)

---------------------------------------------------------------------------- */
void sendAll(int* clients, int count, char* buf){

  int i = 0;
  int len = strlen(buf);
  for(i = 0; i < count; ++i){
    if(clients[i]){
      send(clients[i], buf, len, 0);
    }
  }
  bzero(buf, len);

}

/* ----------------------------------------------------------------------------
FUNCTION

Name:		Receive From All

Prototype:	int recvAll(int* clients, int count, struct stats* testStats)

Developer:	Andrew Burian

Created On:	2015-02-14

Parameters:
	int* clients
    the array of sockets to read from
  int count
    the size of the clients array
  struct stats* testStats
    the structure to populate and return

Return Values:
	int > 0    the first non-zero error code encountered
  0          success
  -1         client shutdown

Description:
	Processes the results being sent back from the clients

Revisions:
	(none)

---------------------------------------------------------------------------- */
int recvAll(int* clients, int count, struct stats* testStats){

  // for socket reading
  int i = 0;
  char msgBuf[256] = {0};
  int thisRead = 0;
  int bufPos = 0;

  // parsing client responses
  double low, high, cumulative;
  int code = 0;
  char cmd[7] = {0};

  testStats->low = 9999;
  testStats->high = 0;
  testStats->cumulative = 0;

  // on all the sockets
  for(i = 0; i < count; ++i){

    // read the message, terribly inefficiently, but safe and targeting \n
    while((thisRead = recv(clients[i], &msgBuf[bufPos], 1, 0)) > 0){

      // check to see if \n was reached and inc bufPos after
      if(msgBuf[bufPos++] == '\n'){
        break;
      }

      if(bufPos == 256){
        // message too long
        break;
      }

    }

    // overflow ignored
    if(bufPos == 256){
      continue;
    }

    // client lost
    if(!thisRead){
      close(clients[i]);
      clients[i] = 0;
      return -1;
    }

    // parse the message
    if(sscanf(msgBuf, "%6s", cmd) != 1){
      continue;
    }

    // valid test data
    if(!strcmp(cmd, "RESULT") &&
      sscanf(msgBuf, "%6s %d %lf %lf %lf", cmd, &code, &low, &high, &cumulative)
      == 5){

      // check for error
      if(code){
        return code;
      }

      // min
      if(low < testStats->low){
        testStats->low = low;
      }

      // max
      if(high > testStats->high){
        testStats->high = high;
      }

      // cumulative
      testStats->cumulative += cumulative;

    }

    // client error message
    if(!strcmp(cmd, "ERR")){
      fprintf(stderr, "%s", msgBuf);
      return -1;
    }

  }

  return code;
}
