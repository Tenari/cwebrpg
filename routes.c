#include "game.c"

typedef struct {
  char* (*routeFnPtr)(world*);
  char *routename;
} route;

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`

char* state(world *World) {
  return cJSON_Print(worldToJSON(World));
}

char* moveRight(world *World) {
  World->Entities[0].Location.X += 1;
  return "OK";
}

char* getUpdate(world *World) {
  sleep(3);
  // if world_has_changed
    // return the visible state
  // else
    // wait for update for up to 20? seconds, then return null
  return cJSON_Print(worldToJSON(World));
}

const int routeCount = 3;
route stateR = { .routeFnPtr = &state, .routename = "/state.json"};
route moveRightR = { .routeFnPtr = &moveRight, .routename = "/moveright"};
route getUpdateR = { .routeFnPtr = &getUpdate, .routename = "/update"};
const route *routes[routeCount] = {
  &stateR,
  &moveRightR,
  &getUpdateR
};
