#define MAX_UPDATE_WAIT_MS 5000
struct route {
  char* (*FnPtr)(world*, char*);
  const char *Path;
  bool Slow;
};

// handles `/render.json` request for initial gamestate
internal char* getVisibleState(world *World, char* Query) {
  char Name[32];
  parseUsernameFromQuery(Query, Name);
  if (strlen(Name) == 0) return (char *)"{}";

  return printVisibleWorldState(World, Name);
}

// handles `/updates.json` long-running stall-until-world-state-is-changed and return the diff
internal char* getUpdate(world *World, char* Query) {
  ulong LastKnownTime = parseUnixTimeFromQuery(Query);
  if (World->LastUpdate <= LastKnownTime) {
    // maybe do something?
  }
  cJSON * JWorld = worldToJSON(World, 1);
  char * Result = cJSON_Print(JWorld);
  cJSON_Delete(JWorld);
  return Result;
}

// handles `/room.json` required `id=` query param
internal char* renderRoom(world *World, char* Query) {
  uint Id = parseIntFromQuery(Query, (char *)"id=");
  room * Room = findRoom(World, Id);
  cJSON * Json = roomToJSON(Room);
  char * Result = cJSON_PrintUnformatted(Json);
  cJSON_Delete(Json);
  return Result;
}

// handles `/input.json` when users send input to game
// DEPRECATED
internal char* readInput(world *World, char* Query) {
  char Name[32];
  Name[0] = '\0';
  parseUsernameFromQuery(Query, Name);
  if (Name[0] == '\0') return (char *) "OK";
  entity* UserEntity = findPlayer(World, Name);

  char Input[32];
  Input[0] = '\0';
  char InputParam[32] = "i=";
  parseChar32FromQuery(Query, Input, InputParam);
  if (Input[0] == '\0') return (char *) "OK";

  for (int i = 0; i < 32; i++) {
    if (Input[i] == '\0') {
      i = 32;
    } else {
      if (Input[i] == 'N') {
        UserEntity->Location.Y -= 1;
      } else if (Input[i] == 'S') {
        UserEntity->Location.Y += 1;
      } else if (Input[i] == 'E') {
        UserEntity->Location.X -= 1;
      } else if (Input[i] == 'W') {
        UserEntity->Location.X += 1;
      }
    }
  }

  World->LastUpdate = (ulong)time(NULL);
  pthread_mutex_lock( &(World->HasUpdatedMutex) );
  pthread_cond_signal( &(World->HasUpdated) );
  pthread_mutex_unlock( &(World->HasUpdatedMutex) );

  return (char*)"OK";
}


// TODO define some routes for login/signup etc bullshit

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`
global_variable const int RouteCount = 4;
global_variable route StateRoute = { .FnPtr = &getVisibleState, .Path = "/render.json", .Slow = false };
global_variable route ReadInputR = { .FnPtr = &readInput, .Path = "/input", .Slow = false };
global_variable route GetUpdateRoute = { .FnPtr = &getUpdate, .Path = "/updates.json", .Slow = true };
global_variable route GetRoomRoute = { .FnPtr = &renderRoom, .Path = "/room.json", .Slow = false };
global_variable const route Routes[RouteCount] = {
  StateRoute,
  GetUpdateRoute,
  GetRoomRoute,
  ReadInputR
};
