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

  entity* UserEntity = findPlayer(World, Name);
  if (UserEntity == NULL) {
    // create a new user
    UserEntity = createPlayer(World, Name);
  }

  // TODO limit result to only the state that's visible by the user
  return cJSON_Print(worldToJSON(World));
}

// handles `/updates.json` long-running stall-until-world-state-is-changed and return the diff
char* getUpdate(world *World, char* Query) {
  ulong LastKnownTime = parseUnixTimeFromQuery(Query);
  if (World->LastUpdate <= LastKnownTime) {
    struct timeval tv;
    struct timespec ts;

    gettimeofday(&tv, NULL);
    ts.tv_sec = time(NULL) + 5000 / 1000;
    ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (5000 % 1000);
    ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    ts.tv_nsec %= (1000 * 1000 * 1000);
    // lock and wait for signal
    pthread_mutex_lock( &(World->HasUpdatedMutex) );
    pthread_cond_timedwait( &(World->HasUpdated), &(World->HasUpdatedMutex), &ts );
    pthread_mutex_unlock( &(World->HasUpdatedMutex) );
  }
  return cJSON_Print(worldToJSON(World));
}

// handles `/input.json` when users send input to game
// TODO

char* moveCharacter(world *World, char* Query) {
  char Name[32];
  Name[0] = '\0';
  parseUsernameFromQuery(Query, Name);
  if (Name[0] == '\0') return (char *) "OK";
  entity* UserEntity = findPlayer(World, Name);

  char DirectionParam[32] = "d=";
  char Direction = parseCharFromQuery(Query, DirectionParam);
  if (Direction == 'N') {
    UserEntity->Location.Y -= 1;
  } else if (Direction == 'S') {
    UserEntity->Location.Y += 1;
  } else if (Direction == 'E') {
    UserEntity->Location.X -= 1;
  } else if (Direction == 'W') {
    UserEntity->Location.X += 1;
  }

  World->LastUpdate = (ulong)time(NULL);
  pthread_mutex_lock( &(World->HasUpdatedMutex) );
  pthread_cond_broadcast( &(World->HasUpdated) );
  pthread_mutex_unlock( &(World->HasUpdatedMutex) );

  return (char*)"OK";
}



// TODO define some routes for login/signup etc bullshit

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`
global_variable const int RouteCount = 3;
global_variable route StateRoute = { .FnPtr = &getVisibleState, .Path = "/render.json", .Slow = false };
global_variable route moveCharacterR = { .FnPtr = &moveCharacter, .Path = "/move", .Slow = false };
global_variable route GetUpdateRoute = { .FnPtr = &getUpdate, .Path = "/updates.json", .Slow = true };
global_variable const route Routes[RouteCount] = {
  StateRoute,
  GetUpdateRoute,
  moveCharacterR
};
