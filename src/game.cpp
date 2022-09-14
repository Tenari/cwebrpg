struct location {
  uint X;
  uint Y;
  uint RoomId;
};

cJSON * locationToJSON(location* Loc) {
  cJSON *jLoc = cJSON_CreateObject();
  cJSON_AddItemToObject(jLoc, "x", cJSON_CreateNumber(Loc->X));
  cJSON_AddItemToObject(jLoc, "y", cJSON_CreateNumber(Loc->Y));
  cJSON_AddItemToObject(jLoc, "roomid", cJSON_CreateNumber(Loc->RoomId));
  return jLoc;
}

#define PLAYER_ENTITY 1
#define EXIT_ENTITY 2
struct entity {
  uint Id;
  ushort Type;
  location Location;
};

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
struct room {
  uint Id;
  uint Width;
  uint Height;
  uchar Floor[1000][1000];
};

#define MAX_ENTITIES 10
struct world {
//  entity Entities[65536];
  entity Entities[MAX_ENTITIES];
  room Rooms[2];
};

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

world setupWorld() {
  world World;
  World.Rooms[0].Id = 1;
  World.Rooms[0].Width = 10;
  World.Rooms[0].Height = 10;
  for (uchar i = 0; i < World.Rooms[0].Width; i++) {
    for (uchar j = 0; j < World.Rooms[0].Height; j++) {
      World.Rooms[0].Floor[i][j] = FLOOR_GRASS;
    }
  }

  World.Entities[0].Id = 1;
  World.Entities[0].Type = PLAYER_ENTITY;
  World.Entities[0].Location.X = 3;
  World.Entities[0].Location.Y = 3;
  World.Entities[0].Location.RoomId = 1;

  World.Entities[1].Id = 2;
  World.Entities[1].Type = EXIT_ENTITY;
  World.Entities[1].Location.X = 0;
  World.Entities[1].Location.Y = 0;
  World.Entities[1].Location.RoomId = 1;

  return World;
}
