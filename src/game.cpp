struct user_input {
  uint Id;
  uchar Text[MAX_USER_INPUT_LEN];
};
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

#define EMPTY_ENTITY_TYPE 0
#define PLAYER_ENTITY 1
#define EXIT_ENTITY 2
#define FIREBALL_ENTITY 3
struct entity {
  uint Id;
  ushort Type;
  location Location;
  char Name[32];
  time_t CreatedAt;
  bool Dead;
  time_t DiedAt;
  int DeathCount;
};

cJSON * entityToJSON(entity* Entity) {
  cJSON *jEntity = cJSON_CreateObject();
  cJSON_AddItemToObject(jEntity, "id", cJSON_CreateNumber(Entity->Id));
  cJSON_AddItemToObject(jEntity, "type", cJSON_CreateNumber(Entity->Type));
  cJSON_AddItemToObject(jEntity, "deathCount", cJSON_CreateNumber(Entity->DeathCount));
  cJSON_AddItemToObject(jEntity, "dead", cJSON_CreateBool(Entity->Dead));
  cJSON_AddItemToObject(jEntity, "location", locationToJSON(&(Entity->Location)));
  if (strlen(Entity->Name) > 0) {
    cJSON_AddItemToObject(jEntity, "name", cJSON_CreateString(Entity->Name));
  }
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

#define MAX_ENTITIES 100
#define MAX_PLAYERS 10
#define ROOM_COUNT 1
struct world {
  entity Entities[MAX_ENTITIES];
  room Rooms[ROOM_COUNT];
  ulong LastUpdate;
  pthread_cond_t HasUpdated;
  pthread_mutex_t HasUpdatedMutex;
  user_input Inputs[MAX_PLAYERS];
};

cJSON * worldToJSON(world* World) {
  cJSON *j_world = cJSON_CreateObject();
  cJSON_AddItemToObject(j_world, "lastUpdate", cJSON_CreateNumber(World->LastUpdate));

  cJSON *entities = cJSON_CreateArray();
  cJSON_AddItemToObject(j_world, "entities", entities);
  for (int index = 0; index < MAX_ENTITIES; ++index) {
    if (World->Entities[index].Type != EMPTY_ENTITY_TYPE) {
      cJSON_AddItemToArray(entities, entityToJSON(&(World->Entities[index])));
    }
  }
  return j_world;
}

world *setupWorld(world *World) {
  World->Rooms[0].Id = 1;
  World->Rooms[0].Width = 40;
  World->Rooms[0].Height = 30;
  for (uchar i = 0; i < World->Rooms[0].Width; i++) {
    for (uchar j = 0; j < World->Rooms[0].Height; j++) {
      World->Rooms[0].Floor[i][j] = FLOOR_GRASS;
    }
  }

  // clear entities array
  for (int i =0; i < MAX_ENTITIES; i++) {
    World->Entities[i].Id = i;
    World->Entities[i].Type = EMPTY_ENTITY_TYPE;
  }

  World->Entities[0].Id = 0;
  World->Entities[0].Type = EXIT_ENTITY;
  World->Entities[0].Location.X = 1;
  World->Entities[0].Location.Y = 1;
  World->Entities[0].Location.RoomId = 1;
  
  World->LastUpdate      = (ulong)time(NULL);
  World->HasUpdated      = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
  World->HasUpdatedMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  return World;
}

internal room* findRoom(world* World, uint Id) {
  room* Result = NULL;
  if (Id == 0) {
    return Result;
  }
  for (int i =0; i < ROOM_COUNT; i++) {
    if (World->Rooms[i].Id == Id) {
      Result = &(World->Rooms[i]);
      break;
    }
  }
  return Result;
}
entity* findPlayer(world* World, uint Id) {
  entity* Result = NULL;
  if (Id == 0) {
    return Result;
  }
  for (int i =0; i < MAX_ENTITIES; i++) {
    if (World->Entities[i].Type == PLAYER_ENTITY && World->Entities[i].Id == Id) {
      Result = &(World->Entities[i]);
      i = MAX_ENTITIES; // exit loop
    }
  }
  return Result;
}
entity* findPlayer(world* World, char Name[32]) {
  entity* Result = NULL;
  if (Name == NULL) {
    return Result;
  }
  for (int i =0; i < MAX_ENTITIES; i++) {
    if (World->Entities[i].Type == PLAYER_ENTITY && strcmp(World->Entities[i].Name, Name) == 0) {
      Result = &(World->Entities[i]);
      i = MAX_ENTITIES; // exit loop
    }
  }
  return Result;
}
entity* createFireball(world* World, uint X, uint Y, uint RoomId) {
  entity* Result = NULL;
  for (uint i =0; i < MAX_ENTITIES; i++) {
    // find first empty entity and initialize as fireball
    if (World->Entities[i].Type == EMPTY_ENTITY_TYPE) {
      timeval CurrentTime;
      gettimeofday(&CurrentTime,NULL);

      World->Entities[i].Id              = i;
      World->Entities[i].Type            = FIREBALL_ENTITY;
      World->Entities[i].CreatedAt       = CurrentTime.tv_sec;
      World->Entities[i].Location.X      = X;
      World->Entities[i].Location.Y      = Y;
      World->Entities[i].Location.RoomId = RoomId;

      Result = &(World->Entities[i]);
      printf("fireball created: (%d, %d)\n",Result->Location.X, Result->Location.Y);
      i = MAX_ENTITIES; // exit loop
    }
  }
  return Result;
}

entity* createPlayer(world* World, char Name[32]) {
  entity* Result = NULL;
  for (uint i =0; i < MAX_ENTITIES; i++) {
    // find first empty entity and initialize as player
    if (World->Entities[i].Type == EMPTY_ENTITY_TYPE) {
      World->Entities[i].Id = i;
      World->Entities[i].Type = PLAYER_ENTITY;
      strcpy(World->Entities[i].Name, Name);
      World->Entities[i].Location.X = 3;
      World->Entities[i].Location.Y = 3;
      World->Entities[i].Location.RoomId = 1;
      Result = &(World->Entities[i]);
      printf("player created: %s\n",Result->Name);
      i = MAX_ENTITIES; // exit loop
    }
  }
  return Result;
}


internal char* printVisibleWorldState(world *World, char Name[32]) {
  entity* UserEntity = findPlayer(World, Name);
  if (UserEntity == NULL) {
    // create a new user
    UserEntity = createPlayer(World, Name);
  }

  // TODO limit result to only the state that's visible by the user
  return cJSON_PrintUnformatted(worldToJSON(World));
}

// returns true if it changed the world in some way
internal bool processInput(user_input *Input, world* World) {
  if (Input->Text[0] == '\0') return false;
  entity* UserEntity = findPlayer(World, Input->Id);
  if (UserEntity == NULL) return false;
  room* Room = findRoom(World, UserEntity->Location.RoomId);
  if (Room == NULL) return false;
  bool DidSomething = false;

  if (UserEntity->Dead) {
    Input->Id = 0;
    Input->Text[0] = '\0';
    return false;
  }

  for (int i = 0; i < MAX_USER_INPUT_LEN; i++) {
    if (Input->Text[i] == '\0') {
      i = 32;
    } else {
      if (Input->Text[i] == 'N') {        //north
        if (UserEntity->Location.Y > 0) {
          UserEntity->Location.Y -= 1;
          DidSomething = true;
        }
      } else if (Input->Text[i] == 'S') { //south
        if (UserEntity->Location.Y < (Room->Height-1)) {
          UserEntity->Location.Y += 1;
          DidSomething = true;
        }
      } else if (Input->Text[i] == 'E') { //east
        if (UserEntity->Location.X > 0) {
          UserEntity->Location.X -= 1;
          DidSomething = true;
        }
      } else if (Input->Text[i] == 'W') { //west
        if (UserEntity->Location.X < (Room->Width-1)) {
          UserEntity->Location.X += 1;
          DidSomething = true;
        }
      } else if (Input->Text[i] == 'F') { //fire
#define FIRE_INPUT_COORD_LEN 4
        char XAsStr[FIRE_INPUT_COORD_LEN] = {0,0,0,0};
        char YAsStr[FIRE_INPUT_COORD_LEN] = {0,0,0,0};
        bool OnX = true;
        ushort StrIndex = 0;
        //start j=2 because 0='F' and 1=' ' and 2 is where the number starts
        for (int j=2; j+i < MAX_USER_INPUT_LEN; j++) { 
          if (Input->Text[i+j] == '\0') {
            break;
          }
          if (Input->Text[i+j] == ' ') {
            OnX = false;
            StrIndex = 0;
          } else {
            if (OnX) {
              XAsStr[StrIndex] = Input->Text[i+j];
            } else {
              YAsStr[StrIndex] = Input->Text[i+j];
            }
            StrIndex ++;
          }
        }
        int X = parseIntFromCharStar(XAsStr, FIRE_INPUT_COORD_LEN);
        int Y = parseIntFromCharStar(YAsStr, FIRE_INPUT_COORD_LEN);
        createFireball(World, X, Y, UserEntity->Location.RoomId);
        DidSomething = true;
      }
    }
  }

  // clear the input from the queue, because it has been processed now
  Input->Id = 0;
  Input->Text[0] = '\0';

  return DidSomething;
}

internal entity* getPlayerAt(world* World, location Location) {
  entity * Result = NULL;
  for(int i=0; i<MAX_ENTITIES; i++) {
    entity *Entity = &(World->Entities[i]);
    if (
        Entity->Type == PLAYER_ENTITY 
        && Entity->Location.X == Location.X
        && Entity->Location.Y == Location.Y
        && Entity->Location.RoomId == Location.RoomId
       ) {
       return Entity;
    }
  }
  return Result;
}

// run in a pthread by main()
#define FRAMES_PER_SECOND 60
internal void* gameLoop(void*args) {
  timeval LoopStartTime;
  timeval LoopEndTime;
  suseconds_t GoalLoopTime = (1000*1000) / FRAMES_PER_SECOND;
  world *World = (world*)args;
  while (true) {
    gettimeofday(&LoopStartTime,NULL);
    bool DidSomething = false;
    // operate on user inputs
    for (int i = 0; i < MAX_PLAYERS; i++) {
      DidSomething = DidSomething || processInput(&(World->Inputs[i]), World);
    }
    
    // process other entities
    for(int i=0; i<MAX_ENTITIES; i++) {
      entity *Entity = &(World->Entities[i]);
      if (Entity->Type == FIREBALL_ENTITY) {
        if (LoopStartTime.tv_sec > (Entity->CreatedAt+3)) {
          *Entity = (entity){0}; // remove the fireball if it's been around for at least 3 seconds
          DidSomething = true;
        }
        entity* OverlappingPlayer = getPlayerAt(World, Entity->Location);
        if (OverlappingPlayer != NULL && !OverlappingPlayer->Dead) {
          OverlappingPlayer->Dead = true;
          OverlappingPlayer->DiedAt = LoopStartTime.tv_sec;
          OverlappingPlayer->DeathCount ++;
          DidSomething = true;
        }
      }

      if (Entity->Type == PLAYER_ENTITY) {
        if (Entity->Dead && LoopStartTime.tv_sec > (Entity->DiedAt + 10)) {
          Entity->Location.X = 1;
          Entity->Location.Y = 1;
          Entity->Dead = false;
          DidSomething = true;
        }
      }
    }

    if (DidSomething) {
      World->LastUpdate = (ulong)time(NULL);
      pthread_mutex_lock( &(World->HasUpdatedMutex) );
      pthread_cond_broadcast( &(World->HasUpdated) );
      pthread_mutex_unlock( &(World->HasUpdatedMutex) );
    }

    gettimeofday(&LoopEndTime,NULL);
    usleep(GoalLoopTime - (LoopEndTime.tv_usec - LoopStartTime.tv_usec));
  }
}
