#include <tamtypes.h>
#include <libuya/player.h>
#include <libuya/spawnpoint.h>
#include <libuya/moby.h>
#include <libuya/common.h>

#define HILL_MOBY_OCLASS (0x3000)
#define HILL_MOBY_SIZE (0x200)

#define HILL_VANILLA_SCALE (20.0f)

#define HILL_RING_TEXTURE (FX_TIRE_TRACKS)
#define HILL_RING_SCROLL_SPEED (0.007)
#define HILL_RING_OPACITY (0x40)
#define HILL_RING_TRIM_U (0.0f)
#define HILL_RING_TRIM_V (0.25f)
#define HILL_RING_MAX_SEGMENTS (64)
#define HILL_RING_MIN_SEGMENTS (4)

#define KOTH_HUD_HILL_SPRITE (SPRITE_HUD_BOLT)
#define KOTH_HUD_TIMER_ICON_SPRITE (0x63) // clock_widget
#define KOTH_HUD_CONTAINER_ID (0xacdc0000)
#define KOTH_HUD_FRAME_ID (KOTH_HUD_CONTAINER_ID + 1)

typedef struct config {
    char grKothScoreLimit;
    char grKothHillDuration;
    char grKothRespawnOutside;
    char grKothRespawnInside;
    char grKothHillSizeIdx;
    char grKothContestedStopsScore;
    char grKothPointStacking;
    char pad;
    int grSeed;
} config_t;

typedef struct kothInfo {
    int gameState;
    bool init;
    bool lastGameUsedMalloc;
    bool radar;
    bool foundMoby;
    int hillScoreLimit;
    Moby *hillMoby;
} kothInfo_t;

typedef struct hillPvar { // 0x200
/* 0x000 */ Cuboid *hillCuboids[32];
/* 0x100 */ bool isCircle;
/* 0x101 */ char pad[0x3];
/* 0x104 */ Player *playersIn[GAME_MAX_PLAYERS];
/* 0x124 */ Cuboid *currentCuboid;
/* 0x144 */ int teamTime[8];
/* 0x164 */ u32 color;
/* 0x108 */ char empty[0x98];
} hillPvar_t;

kothInfo_t kothInfo;

void getHillCuboids(hillPvar_t *this, bool isCustomMap);
