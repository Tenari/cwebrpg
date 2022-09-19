struct route {
  char* (*FnPtr)(world*, char*);
  const char *Path;
  bool Slow;
};

// handles `/render.json` request for initial gamestate
internal char* getVisibleState(world *World, char* Query) {
  char Name[32];
  parseUsernameFromQuery(Query, Name);
  if (strlen(Name) == 0) return "{}";

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
  // TODO actually only get updates
  sleep(3);
  // if world_has_changed
    // return the visible state
  // else
    // wait for update for up to 20? seconds, then return null
  return cJSON_Print(worldToJSON(World));
}

// handles `/input.json` when users send input to game
// TODO
/*
char* moveRight(world *World) {
  World->Entities[0].Location.X += 1;
  return "OK";
}
*/


// TODO define some routes for login/signup etc bullshit

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`
global_variable const int RouteCount = 2;
global_variable route StateRoute = { .FnPtr = &getVisibleState, .Path = "/render.json", .Slow = false };
//route moveRightR = { .routeFnPtr = &moveRight, .routename = "/moveright"};
global_variable route GetUpdateRoute = { .FnPtr = &getUpdate, .Path = "/updates.json", .Slow = true };
global_variable const route Routes[RouteCount] = {
  StateRoute,
  GetUpdateRoute
//  &moveRightR,
};
