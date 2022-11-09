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
  int Health;
  int MaxHealth;
  int XP;
  uint CreatedById; //entity id this entity was created by
  ushort MoveFrame;
  location MoveGoal;
#define AVAILABLE_SPELLS_MAX 32
  ushort AvailableSpells[AVAILABLE_SPELLS_MAX];
#define KNOWN_SPELLS_MAX 128
  ushort KnownSpells[KNOWN_SPELLS_MAX];
};
#define FIREBALL_SPELL 1
#define FIRESTREAM_SPELL 2
#define INFERNO_SPELL 3

cJSON * entityToJSON(entity* Entity) {
  cJSON *jEntity = cJSON_CreateObject();
  cJSON_AddItemToObject(jEntity, "id", cJSON_CreateNumber(Entity->Id));
  cJSON_AddItemToObject(jEntity, "type", cJSON_CreateNumber(Entity->Type));
  if (Entity->DeathCount)
    cJSON_AddItemToObject(jEntity, "deathCount", cJSON_CreateNumber(Entity->DeathCount));
  if (Entity->Health)
    cJSON_AddItemToObject(jEntity, "health", cJSON_CreateNumber(Entity->Health));
  if (Entity->MaxHealth)
    cJSON_AddItemToObject(jEntity, "maxHealth", cJSON_CreateNumber(Entity->MaxHealth));
  if (Entity->Dead)
    cJSON_AddItemToObject(jEntity, "dead", cJSON_CreateBool(Entity->Dead));
  if (Entity->XP)
    cJSON_AddItemToObject(jEntity, "xp", cJSON_CreateNumber(Entity->XP));
  if (Entity->MoveFrame > 0) {
    cJSON_AddItemToObject(jEntity, "moveFrame", cJSON_CreateNumber(Entity->MoveFrame));
    cJSON_AddItemToObject(jEntity, "moveGoal", locationToJSON(&(Entity->MoveGoal)));
  }
  cJSON_AddItemToObject(jEntity, "location", locationToJSON(&(Entity->Location)));
  if (strlen(Entity->Name) > 0) {
    cJSON_AddItemToObject(jEntity, "name", cJSON_CreateString(Entity->Name));
  }
  if (!ushortArrIsEmpty(Entity->AvailableSpells, AVAILABLE_SPELLS_MAX)){
    cJSON *spells_arr = cJSON_CreateArray();
    for (int i = 0; i < AVAILABLE_SPELLS_MAX; ++i) {
      if (Entity->AvailableSpells[i])
        cJSON_AddItemToArray(spells_arr, cJSON_CreateNumber(Entity->AvailableSpells[i]));
    }
    cJSON_AddItemToObject(jEntity, "availableSpells", spells_arr);
  }
  if (!ushortArrIsEmpty(Entity->KnownSpells, AVAILABLE_SPELLS_MAX)){
    cJSON *spells_arr = cJSON_CreateArray();
    for (int i = 0; i < KNOWN_SPELLS_MAX; ++i) {
      if (Entity->KnownSpells[i])
        cJSON_AddItemToArray(spells_arr, cJSON_CreateNumber(Entity->KnownSpells[i]));
    }
    cJSON_AddItemToObject(jEntity, "knownSpells", spells_arr);
  }
  return jEntity;
}

//currently, this always clears the AvailableSpells if the spell is actually valid/learned
internal bool learnSpell(entity* Entity, ushort SpellKey) {
  bool DidSomething = false;
  bool KeyIsValid = false;
  for (int i = 0; i < AVAILABLE_SPELLS_MAX; ++i) {
    if (Entity->AvailableSpells[i] == SpellKey)
      KeyIsValid = true;
  }
  if (KeyIsValid) {
    for (int i = 0; i < KNOWN_SPELLS_MAX; ++i) {
      if (Entity->KnownSpells[i] == 0) {
        Entity->KnownSpells[i] = SpellKey;
        clearUshortArr(Entity->AvailableSpells, AVAILABLE_SPELLS_MAX);
        DidSomething = true;
        break;
      }
    }
  }
  return DidSomething;
}
#define FLOOR_NULL 0
#define FLOOR_GRASS 1
#define FLOOR_STONE 2
#define MAX_ROOM_DIMENSION 512
struct room {
  uint Id;
  uint Width;
  uint Height;
  uchar Floor[MAX_ROOM_DIMENSION][MAX_ROOM_DIMENSION];
};

#define MAX_ENTITIES 512
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

internal cJSON * worldToJSON(world* World, uint RoomId) {
  cJSON *j_world = cJSON_CreateObject();
  cJSON_AddItemToObject(j_world, "lastUpdate", cJSON_CreateNumber(World->LastUpdate));

  cJSON *entities = cJSON_CreateArray();
  cJSON_AddItemToObject(j_world, "entities", entities);
  for (int i = 0; i < MAX_ENTITIES; ++i) {
    if (World->Entities[i].Type != EMPTY_ENTITY_TYPE && World->Entities[i].Location.RoomId == RoomId) {
      cJSON_AddItemToArray(entities, entityToJSON(&(World->Entities[i])));
    }
  }
  return j_world;
}

internal cJSON * roomToJSON(room* Room) {
  cJSON *j_room = cJSON_CreateObject();
  cJSON_AddItemToObject(j_room, "id", cJSON_CreateNumber(Room->Id));
  cJSON_AddItemToObject(j_room, "width", cJSON_CreateNumber(Room->Width));
  cJSON_AddItemToObject(j_room, "height", cJSON_CreateNumber(Room->Height));
  cJSON *j_floor = cJSON_CreateArray();
  for (int i = 0; i < Room->Width; ++i) {
    cJSON *j_floor_col = cJSON_CreateArray();
    for (int j = 0; j < Room->Height; ++j) {
      cJSON_AddItemToArray(j_floor_col, cJSON_CreateNumber(Room->Floor[i][j]));
    }
    cJSON_AddItemToArray(j_floor, j_floor_col);
  }
  cJSON_AddItemToObject(j_room, "floor", j_floor);

  return j_room;
}

internal world *setupWorld(world *World) {
  World->Rooms[0].Id = 1;
  World->Rooms[0].Width = 25;
  World->Rooms[0].Height = 19;
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

internal entity* findPlayer(world* World, location Location) {
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

entity* findPlayer(world* World, uint RoomId, uint X, uint Y) {
  return findPlayer(World, (location){ .X = X, .Y = Y, .RoomId = RoomId});
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

internal bool killEntity(world* World, entity* Player, uint KillerId) {
  timeval CurrentTime;
  gettimeofday(&CurrentTime,NULL);
  bool DidSomething = false;
  if (Player != NULL && !Player->Dead) {
    Player->Dead = true;
    Player->DiedAt = CurrentTime.tv_sec;
    Player->DeathCount ++;
    entity* Killer = findPlayer(World, KillerId);
    if (Killer != NULL) {
      Killer->XP += 1;
    }
    DidSomething = true;
  }
  return DidSomething;
}

internal bool reviveEntity(entity* Entity) {
  if (Entity->Dead) {
    Entity->Location.X = 1;
    Entity->Location.Y = 1;
    Entity->Dead = false;
    if (Entity->MaxHealth > 0) {
      Entity->Health = Entity->MaxHealth;
    }
    return true;
  }
  return false;
}

// returns true if damage was dealt and kills them if they reached 0 health
internal bool damageEntity(world* World, entity* Entity, int Dmg, uint DamageDealerId) {
  if (Entity != NULL && Entity->Health >= 0) {
    Entity->Health -= Dmg;
    if (Entity->Health <= 0) {
      killEntity(World, Entity, DamageDealerId);
    }
    // later there might be shields which negate the damage or something
    return true;
  }
  return false;
}


entity* createFireball(world* World, uint X, uint Y, uint RoomId, uint CreatorId) {
  entity* Result = NULL;
  for (uint i =0; i < MAX_ENTITIES; i++) {
    // find first empty entity and initialize as fireball
    if (World->Entities[i].Type == EMPTY_ENTITY_TYPE) {
      timeval CurrentTime;
      gettimeofday(&CurrentTime,NULL);

      World->Entities[i].Id              = i;
      World->Entities[i].Type            = FIREBALL_ENTITY;
      World->Entities[i].CreatedAt       = CurrentTime.tv_sec;
      World->Entities[i].CreatedById     = CreatorId;
      World->Entities[i].Location.X      = X;
      World->Entities[i].Location.Y      = Y;
      World->Entities[i].Location.RoomId = RoomId;

      Result = &(World->Entities[i]);
//      printf("fireball created: (%d, %d)\n",Result->Location.X, Result->Location.Y);
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
      World->Entities[i].Health = 10;
      World->Entities[i].MaxHealth = 10;
      World->Entities[i].AvailableSpells[0] = 1;
      World->Entities[i].AvailableSpells[1] = 2;
      World->Entities[i].AvailableSpells[2] = 3;
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
  cJSON * JWorld = worldToJSON(World, UserEntity->Location.RoomId);
  char * Result = cJSON_PrintUnformatted(JWorld);
  cJSON_Delete(JWorld);
  return Result;
}

#define PLAYER_MOVE_FRAMES 30
internal bool tryToStartMovingPlayer(entity* Player, room* Room, char Direction) {
  if (Player->MoveFrame > 0) return false; // already moving

  bool DidSomething = false;
  if (Direction == 'N') {        //north
    if (Player->Location.Y > 0) {
      Player->MoveFrame = 1;
      Player->MoveGoal.X = Player->Location.X;
      Player->MoveGoal.Y = Player->Location.Y - 1;
      Player->MoveGoal.RoomId = Player->Location.RoomId;
      DidSomething = true;
    }
  } else if (Direction == 'S') { //south
    if (Player->Location.Y < (Room->Height-1)) {
      Player->MoveFrame = 1;
      Player->MoveGoal.X = Player->Location.X;
      Player->MoveGoal.Y = Player->Location.Y + 1;
      Player->MoveGoal.RoomId = Player->Location.RoomId;
      DidSomething = true;
    }
  } else if (Direction == 'E') { //east
    if (Player->Location.X < (Room->Width-1)) {
      Player->MoveFrame = 1;
      Player->MoveGoal.X = Player->Location.X + 1;
      Player->MoveGoal.Y = Player->Location.Y;
      Player->MoveGoal.RoomId = Player->Location.RoomId;
      DidSomething = true;
    }
  } else if (Direction == 'W') { //west
    if (Player->Location.X > 0) {
      Player->MoveFrame = 1;
      Player->MoveGoal.X = Player->Location.X - 1;
      Player->MoveGoal.Y = Player->Location.Y;
      Player->MoveGoal.RoomId = Player->Location.RoomId;
      DidSomething = true;
    }
  }
  return DidSomething;
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
      DidSomething = tryToStartMovingPlayer(UserEntity, Room, Input->Text[i]) || DidSomething;

      if (Input->Text[i] == 'P') { //punch
#define PUNCH_DMG 2
        entity* OpponentN = findPlayer(World, Room->Id, UserEntity->Location.X, UserEntity->Location.Y-1);
        entity* OpponentS = findPlayer(World, Room->Id, UserEntity->Location.X, UserEntity->Location.Y+1);
        entity* OpponentE = findPlayer(World, Room->Id, UserEntity->Location.X+1, UserEntity->Location.Y);
        entity* OpponentW = findPlayer(World, Room->Id, UserEntity->Location.X-1, UserEntity->Location.Y);
        if (OpponentN != NULL) { // north
          DidSomething = damageEntity(World, OpponentN, PUNCH_DMG, UserEntity->Id);
        } else if (OpponentS != NULL) {//south
          DidSomething = damageEntity(World, OpponentS, PUNCH_DMG, UserEntity->Id);
        } else if (OpponentE != NULL) {//east
          DidSomething = damageEntity(World, OpponentE, PUNCH_DMG, UserEntity->Id);
        } else if (OpponentW != NULL) {//west
          DidSomething = damageEntity(World, OpponentW, PUNCH_DMG, UserEntity->Id);
        }
      } else if (Input->Text[i] == 'C') { //cast
        // expected format: 'C SpellKey X Y'
#define SPELL_INPUT_PARAM_LEN 4
        char Params[3][SPELL_INPUT_PARAM_LEN] = {
          {0,0,0,0},
          {0,0,0,0},
          {0,0,0,0}
        };
        ushort ParamIndex = 0;
        ushort StrIndex = 0;
        //start j=2 because 0='C' and 1=' ' and 2 is where the numbers start
        for (int j=2; j+i < MAX_USER_INPUT_LEN; j++) { 
          if (Input->Text[i+j] == '\0') {
            break;
          }
          if (Input->Text[i+j] == ' ') {
            ParamIndex += 1;
            StrIndex = 0;
          } else {
            Params[ParamIndex][StrIndex] = Input->Text[i+j];
            StrIndex ++;
          }
        }
        ushort SpellKey = parseUshortFromCharStar(Params[0], SPELL_INPUT_PARAM_LEN);
        int X = parseIntFromCharStar(Params[1], SPELL_INPUT_PARAM_LEN);
        int Y = parseIntFromCharStar(Params[2], SPELL_INPUT_PARAM_LEN);
        if (SpellKey == FIREBALL_SPELL) {
          DidSomething = DidSomething || (createFireball(World, X, Y, UserEntity->Location.RoomId, UserEntity->Id) != NULL);
        } else if (SpellKey == FIRESTREAM_SPELL) {
          //detect intended firestream direction
          bool HorizontalFirestream = true;
          if (X == UserEntity->Location.X) { // vertical firestream
            HorizontalFirestream = false;
          }
          for (int k = 0; k < 5; k++) {
            int UseX = HorizontalFirestream ? X+k : X;
            int UseY = HorizontalFirestream ? Y : Y+k;
            if (UseX < 0 || UseY < 0) 
              continue;
            bool MadeFire = createFireball(World, UseX, UseY,UserEntity->Location.RoomId, UserEntity->Id) != NULL;
            DidSomething = DidSomething || MadeFire;
          }
        } else if (SpellKey == INFERNO_SPELL) {
          for (int k = -2; k < 3; k++) {
            for (int l = -2; l < 3; l++) {
              if (X+k < 0 || Y+l < 0 || (k==0 && l==0)) 
                continue;
              bool MadeFire = createFireball(World, X+k, Y+l, UserEntity->Location.RoomId, UserEntity->Id) != NULL;
              DidSomething = DidSomething || MadeFire;
            }
          }
        }
      } else if (Input->Text[i] == 'L') { //learn (spell)
        char SpellKeyAsStr[SPELL_INPUT_PARAM_LEN] = {0,0,0,0};
        ushort StrIndex = 0;
        //start j=2 because 0='L' and 1=' ' and 2 is where the number starts
        for (int j=2; j+i < MAX_USER_INPUT_LEN; j++) { 
          if (Input->Text[i+j] == '\0') {
            break;
          }
          SpellKeyAsStr[StrIndex] = Input->Text[i+j];
          StrIndex ++;
        }
        ushort SpellKey = parseUshortFromCharStar(SpellKeyAsStr, SPELL_INPUT_PARAM_LEN);
        DidSomething = learnSpell(UserEntity, SpellKey);
      }
    }
  }

  // clear the input from the queue, because it has been processed now
  Input->Id = 0;
  Input->Text[0] = '\0';

  return DidSomething;
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
      DidSomething = processInput(&(World->Inputs[i]), World) || DidSomething;
    }
    
    // process other entities
    for(int i=0; i<MAX_ENTITIES; i++) {
      entity *Entity = &(World->Entities[i]);
      if (Entity->Type == FIREBALL_ENTITY) {
        if (LoopStartTime.tv_sec > (Entity->CreatedAt+3)) {
          *Entity = (entity){0}; // remove the fireball if it's been around for at least 3 seconds
          DidSomething = true;
        }
        entity* OverlappingPlayer = findPlayer(World, Entity->Location);
        DidSomething = damageEntity(World, OverlappingPlayer, 1, Entity->CreatedById) || DidSomething;
      }

      if (Entity->Type == PLAYER_ENTITY) {
        if (Entity->Dead && LoopStartTime.tv_sec > (Entity->DiedAt + 10)) {
          DidSomething = reviveEntity(Entity) || DidSomething;
        }
        if (Entity->MoveFrame > 0) {
          if (Entity->MoveFrame < PLAYER_MOVE_FRAMES) {
            Entity->MoveFrame += 1;
          } else {
            Entity->MoveFrame = 0;
            Entity->Location.X = Entity->MoveGoal.X;
            Entity->Location.Y = Entity->MoveGoal.Y;
            Entity->Location.RoomId = Entity->MoveGoal.RoomId;
            DidSomething = true;
          }
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
