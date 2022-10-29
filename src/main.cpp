// system libs
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>

// local libs
#include "lib/cJSON.h"
#include "lib/cJSON.c"
#include "lib/sha1.cpp"
#include "lib/websocket.cpp"
// src
#include "mytypes.h"
#include "util.cpp"
#include "game.cpp"
#include "web.cpp"
#include "routes.cpp"
#include "server.cpp"

int main(int argc, char *argv[]) {
  if (argc < 3) { fprintf(stderr, "Must pass [port] then [max_websocket_connections].\n"); exit(EXIT_FAILURE);}

  // initialize game
  ulong PermanentStorageSize = Megabytes(64);
  world *GameWorld = (world *)calloc(1, PermanentStorageSize); //calloc(1, x) instead of malloc(x) b/c it will clear everything to 0 for us
  if (GameWorld == NULL) {
    fprintf(stderr, "couldn't calloc the game memory\n");
    exit(EXIT_FAILURE);
  }
  setupWorld(GameWorld);
  pthread_t GameThread;
  if (pthread_create(&GameThread, NULL, gameLoop, (void*)GameWorld)) {
    serverFail("Error creating thread\n");
  }

  // turn on server and loop forever
  (void) signal(SIGINT, cleanupServer);
  startServer(atoi(argv[1]), atoi(argv[2]), GameWorld);
}
