#define HILL_OCLASS (0x3000)
#define MAX_SEGMENTS (64)
#define MIN_SEGMENTS (4)
#define TEXTURE_SCROLL_SPEED (0.007)
#define EDGE_OPACITY (0x40)
#define TEXTURE_EDGE_TRIM_U (0.0f)
#define TEXTURE_EDGE_TRIM_V (0.25f)

typedef struct kothInfo {
    int gameState;
    int foundMoby;
    Moby *kothMoby;
} kothInfo_t;

typedef struct hillPvar {
    int cuboidIndex[32];
    Cuboid *vanillaHills[32];
    bool foundMoby;
    bool isCircle;
    short pad;
    Player *playersIn[GAME_MAX_PLAYERS];
    Cuboid *currentCuboid;
    int teamTime[8];
    u32 color;
} hillPvar_t;

kothInfo_t kothInfo;

void getVanillaHills(hillPvar_t *this);
