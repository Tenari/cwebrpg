#include "mytypes.h"

typedef struct {
  uint X;
  uint Y;
  uint RoomId;
} location;

cJSON * locationToJSON(location* Loc) {
  cJSON *jLoc = cJSON_CreateObject();
  cJSON_AddItemToObject(jLoc, "x", cJSON_CreateNumber(Loc->X));
  cJSON_AddItemToObject(jLoc, "y", cJSON_CreateNumber(Loc->Y));
  cJSON_AddItemToObject(jLoc, "roomid", cJSON_CreateNumber(Loc->RoomId));
  return jLoc;
}


#define PLAYER_ENTITY 1
#define EXIT_ENTITY 2
typedef struct {
  uint Id;
  ushort Type;
  location Location;
} entity;

cJSON * entityToJSON(entity* Entity) {
  cJSON *jEntity = cJSON_CreateObject();
  cJSON_AddItemToObject(jEntity, "id", cJSON_CreateNumber(Entity->Id));
  cJSON_AddItemToObject(jEntity, "type", cJSON_CreateNumber(Entity->Type));
  cJSON_AddItemToObject(jEntity, "location", locationToJSON(&(Entity->Location)));
  return jEntity;
}

#define FLOOR_NULL 0
#define FLOOR_GRASS 1
#define FLOOR_STONE 2
typedef struct {
  uint Id;
  uint Width;
  uint Height;
  uchar Floor[1000][1000];
} room;

#define MAX_ENTITIES 10
typedef struct {
//  entity Entities[65536];
  entity Entities[MAX_ENTITIES];
  room Rooms[2];
} world;

cJSON * worldToJSON(world* World) {
  cJSON *j_world = cJSON_CreateObject();
  cJSON *entities = cJSON_CreateArray();
  cJSON_AddItemToObject(j_world, "entities", entities);
  for (int index = 0; index < MAX_ENTITIES; ++index) {
    if (World->Entities[index].Id != 0) {
      cJSON_AddItemToArray(entities, entityToJSON(&(World->Entities[index])));
    }
  }
  return j_world;
}
