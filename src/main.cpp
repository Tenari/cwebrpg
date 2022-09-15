// system libs
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
//#include <sys/socket.h>
//#include <signal.h>

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

  // TODO: determine if this game_memory crap is necessary, since we are always just passing the *World anyway
  // initialize game
  game_memory GameMemory = {};
  GameMemory.PermanentStorageSize = Megabytes(64);
  GameMemory.PermanentStorage = calloc(1, GameMemory.PermanentStorageSize); //calloc(1, x) instead of malloc(x) b/c it will clear everything to 0 for us
  if (GameMemory.PermanentStorage == NULL) {
    fprintf(stderr, "couldn't calloc the game memory\n");
    exit(EXIT_FAILURE);
  }
  world *World = (world *)GameMemory.PermanentStorage;
  if (!GameMemory.IsInitialized) {
    setupWorld(World);
    GameMemory.IsInitialized = true;
  } else {
  }

  // turn on server and loop forever
  (void) signal(SIGINT, cleanupServer);
  startServer(atoi(argv[1]), atoi(argv[2]), World);
}
