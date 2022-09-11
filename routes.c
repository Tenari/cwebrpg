#include "lib/cJSON.h"
#include "lib/cJSON.c"
#include "game.c"

typedef struct {
  char* (*routeFnPtr)(world);
  char *routename;
} route;

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`

char* state(world World)
{
  cJSON *j_world = cJSON_CreateObject();
  cJSON *entities = cJSON_CreateArray();
  cJSON *ent = NULL;
  cJSON *id = NULL;
  cJSON *e_type = NULL;
  cJSON *loc = NULL;
  cJSON_AddItemToObject(j_world, "entities", entities);
  for (int index = 0; index < MAX_ENTITIES; ++index) {
    if (World.Entities[index].Id != 0) {
      ent = cJSON_CreateObject();
      cJSON_AddItemToArray(entities, ent);

      id = cJSON_CreateNumber(World.Entities[index].Id);
      cJSON_AddItemToObject(ent, "id", id);

      e_type = cJSON_CreateNumber(World.Entities[index].Type);
      cJSON_AddItemToObject(ent, "type", e_type);
    }
  }
  return cJSON_Print(j_world);
}

const int routeCount = 1;
route stateR = { .routeFnPtr = &state, .routename = "/state.json"};
const route *routes[routeCount] = {
  &stateR
};
