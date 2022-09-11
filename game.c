#include "mytypes.h"

struct location {
  uint X;
  uint Y;
  uint RoomId;
};

#define PLAYER_ENTITY 1
#define EXIT_ENTITY 2
struct entity {
  uint Id;
  ushort Type;
  struct location Location;
};

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
typedef struct {
//  entity Entities[65536];
  struct entity Entities[MAX_ENTITIES];
  struct room Rooms[2];
} world;

