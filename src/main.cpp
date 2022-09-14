// system libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
//#include <sys/socket.h>
//#include <signal.h>
#include <arpa/inet.h>
// local libs
#include "lib/cJSON.h"
#include "lib/cJSON.c"
// src
#include "mytypes.h"
#include "util.cpp"
#include "game.cpp"
#include "routes.cpp"
#include "server.cpp"

int main(int argc, char *argv[]) {
  if (argc < 3) { fprintf(stderr, "Must pass [port] then [threads].\n"); exit(EXIT_FAILURE);}

  (void) signal(SIGINT, cleanupServer);

  startServer(atoi(argv[1]), atoi(argv[2]));
}
