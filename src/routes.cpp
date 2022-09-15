struct route {
  char* (*FnPtr)(world*);
  const char *Path;
};

// handles `/render.json` request for initial gamestate
internal char* getVisibleState(world *World) {
  return cJSON_Print(worldToJSON(World));
}

// handles `/newupdates.json` long-running stall-until-world-state-is-changed and return the diff
// TODO

// handles `/input.json` when users send input to game
// TODO

// TODO define some routes for login/signup etc bullshit

/*
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
*/

// TO ADD A ROUTE:
// 1. define a function that returns a char* here (that is the json content)
// 2. add a route declaration at the bottom
//    like: `route stateR = { .routeFnPtr = &state, .routename = "/state.json"};`
global_variable const int RouteCount = 1;
global_variable route StateRoute = { .FnPtr = &getVisibleState, .Path = "/state.json" };
//route moveRightR = { .routeFnPtr = &moveRight, .routename = "/moveright"};
//route getUpdateR = { .routeFnPtr = &getUpdate, .routename = "/update"};
global_variable const route Routes[RouteCount] = {
  StateRoute
//  &moveRightR,
//  &getUpdateR
};
