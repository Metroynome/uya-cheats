#include <libuya/moby.h>
#include <libuya/math.h>
#include <libuya/math3d.h>
#include <libuya/player.h>
#include <libuya/interop.h>
#include <libuya/game.h>
#include <libuya/gamesettings.h>

// Trooper Definitions
#define TROOPER_OCLASS (MOBY_ID_TROOPER_LEGS)
#define TROOPER_PVAR_SIZE (sizeof(M4255_TrooperLegsPvar_t))
#define TROOPER_MAX_SPAWN (32)

// Shock Droid Spawner Definitions
#define SHOCK_DROID_SPAWNER_OCLASS (MOBY_ID_SHOCK_DROID_SPAWNER)
#define SHOCK_DROID_SPAWNER_PVAR_SIZE (sizeof(M6801_ShockDroidSpawnerPVar_t))
#define SHOCK_DROID_SPAWNER_MAX_SPAWN (32)

// Shock Droid Definitions
#define SHOCK_DROID_OCLASS (MOBY_ID_SHOCK_DROID)
#define SHOCK_DROID_PVAR_SIZE (sizeof(M6683_ShockDroidPVar_t))
#define SHOCK_DROID_MAX_SPAWN (32)

// Ball Bot Spawner Definitions
#define BALL_BOT_SPAWNER_OCLASS (MOBY_ID_BALL_BOT_SPAWNER)
#define BALL_BOT_SPAWNER_PVAR_SIZE (sizeof(M6646_BallBotSpawnerPVar_t))
#define BALL_BOT_SPAWNER_MAX_SPAWN (32)

// Ball Bot Definitions
#define BALL_BOT_OCLASS (MOBY_ID_BALL_BOT)
#define BALL_BOT_PVAR_SIZE (sizeof(M6646_BallBotPVar_t))
#define BALL_BOT_MAX_SPAWN (32)

typedef struct M4255_TrooperLegsPvar { // 0x3e0
/* 0x000 */ char unk_2b8[0x2b8];
/* 0x2b8 */ int team;
/* 0x2bc */ int unk_2bc;
/* 0x2c0 */ float aggroDistance;
/* 0x2c4 */ int unk_2c4;
/* 0x2c8 */ void *spline;
/* 0x2cc */ char unk_2cc[0x104];
/* 0x3d0 */ float health;
/* 0x3d4 */ float damage;
/* 0x3d8 */ char unk_3d8[0x8];
} M4255_TrooperLegsPvar_t;

typedef struct M6801_ShockDroidSpawnerPVar { // 0x470
/* 0x000 */ char unk_000[0x290];
/* 0x290 */ int team;
/* 0x294 */ char unk_294[0x20];
/* 0x2b4 */ float health;
/* 0x2b8 */ float spawnProximity;
/* 0x2bc */ char unk_2bc[0x198];
/* 0x454 */ void* spline;
/* 0x458 */ char unk_458[0xc];
/* 0x464 */ int mobyGroupId;
/* 0x468 */ int maxSpawnLimit;
/* 0x46c */ char unk_46c[0x4];
} M6801_ShockDroidSpawnerPVar_t;

typedef struct M6683_ShockDroidPVar { // 0x310
/* 0x000 */ char unk_000[0x2ac];
/* 0x2ac */ int team;
/* 0x2b0 */ char unk_2b0[0x50];
/* 0x300 */ float health;
/* 0x304 */ float damage;
/* 0x308 */ char unk_308[0x8];
} M6683_ShockDroidPVar_t;

typedef struct M6646_BallBotSpawnerPVar { // 0x1f0
/* 0x000 */ char unk_000[0x1a0];
/* 0x1a0 */ int team;
/* 0x1a4 */ char unk_1a4[0xc];
/* 0x1b0 */ int mode;
/* 0x1b4 */ int spawnState;
/* 0x1b8 */ char unk_1b8[0x8];
/* 0x1c0 */ int counter;
/* 0x1c4 */ float health;
/* 0x1c8 */ float spawnProximity;
/* 0x1cc */ int unk_1cc;
/* 0x1d0 */ int mobyIndex;
/* 0x1d4 */ char unk_1d4[0x4];
/* 0x1d8 */ int playerCountFlag;
/* 0x1dc */ char unk_1dc[0x14];
} M6646_BallBotSpawnerPVar_t;

typedef struct M4250_BallBotPVar { // 0x2e0
/* 0x000 */ char unk_000[0x2e0];
} M4250_BallBotPVar_t;

struct TrooperData {
    int count;
    Moby *moby[TROOPER_MAX_SPAWN];
};

struct ShockDroidSpawnerData {
    int count;
    Moby *moby[SHOCK_DROID_SPAWNER_MAX_SPAWN];
};

struct ShockDroidData {
    int count;
    Moby *moby[SHOCK_DROID_MAX_SPAWN];
};

struct BallBotSpawnerData {
    int count;
    Moby *moby[BALL_BOT_SPAWNER_MAX_SPAWN];
};

struct BallBotData {
    int count;
    Moby *moby[BALL_BOT_MAX_SPAWN];
};

struct DroidData {
    struct TrooperData *trooper;
    struct ShockDroidSpawnerData *shockDroidSpawner;
    struct shockDroidData *shockDroid;
    struct BallBotSpawnerData *ballBotSpawner;
    struct BallBotData *ballBot;
};

typedef struct DroidInfo {
    int init;
    int count;
    struct DroidData data;
} DroidInfo_t;
DroidInfo_t info;

void droidSpawn(u16 oClass, int size)
{
    Moby *m = mobySpawn(oClass, size);
    if (!m) return;

    switch(oClass) {
        case TROOPER_OCLASS: {
            M4255_TrooperLegsPvar_t *pvar = (M4255_TrooperLegsPvar_t*)m->pVar;
            struct TrooperData *bot = (struct TrooperData*)info.data.trooper;
            pvar->spline = -1;
            pvar->team = 2;
            pvar->aggroDistance = 30;
            bot->moby[bot->count] = (Moby*)m;
            ++bot->count;
        } break;
        case SHOCK_DROID_SPAWNER_OCLASS: {
            M6801_ShockDroidSpawnerPVar_t *pvar = (M6801_ShockDroidSpawnerPVar_t*)m->pVar;
            struct ShockDroidSpawnerData *bot = (struct ShockDroidSpawnerData*)info.data.shockDroidSpawner;
            pvar->spline = -1;
            pvar->health = 15;
            pvar->team = 2;
            pvar->maxSpawnLimit = 10;
            pvar->spawnProximity = 10;
            bot->moby[bot->count] = (Moby*)m;
            ++bot->count;
        } break;
        case SHOCK_DROID_OCLASS: {
            M6683_ShockDroidPVar_t *pvar = (M6683_ShockDroidPVar_t*)m->pVar;
            struct ShockDroidData *bot = (struct ShockDroidData*)info.data.shockDroid;
            bot->moby[bot->count] = (Moby*)m;
            ++bot->count;
        } break;
        case BALL_BOT_SPAWNER_OCLASS: {
            M6646_BallBotSpawnerPVar_t *pvar = (M6646_BallBotSpawnerPVar_t*)m->pVar;
            struct BallBotSpawnerData *bot = (struct BallBotSpawnerData*)info.data.ballBotSpawner;
            bot->moby[bot->count] = (Moby*)m;
            *(u32*)((u32)pvar + 0x00) = ((u32)pvar + 0x30);
            *(u32*)((u32)pvar + 0x10) = ((u32)pvar + 0xe0);
            pvar->health = 3;
            pvar->team = 0;
            pvar->mobyIndex = 16;
            pvar->spawnProximity = 10;
            m->state = 2; // state 2 makes it able to be destroyed
            ++bot->count;
        } break;
        case BALL_BOT_OCLASS: {
            M6646_BallBotSpawnerPVar_t *pvar = (M6646_BallBotSpawnerPVar_t*)m->pVar;
            struct BallBotData *bot = (struct BallBotData*)info.data.ballBot;
            bot->moby[bot->count] = (Moby*)m;
            ++bot->count;
        } break;
    }
    Player *player = playerGetFromSlot(0);
    vector_copy(m->position, player->playerPosition);
    m->modeBits = 0;
    m->triggers = m->triggers & 0xe7;
    m->updateDist = -1;
    ++info.count;
}

void droids_run(void)
{
	Player *player = playerGetFromSlot(0);
    if (!info.init) {
        // troopers only gamerule skip
        // POKE_U32(0x00409f34, 0x24020000);
        info.init = 1;
    }
    
    if (info.data.trooper->count < TROOPER_MAX_SPAWN && playerPadGetButtonDown(player, PAD_DOWN) > 0)
        droidSpawn(TROOPER_OCLASS, TROOPER_PVAR_SIZE);
	if (info.data.shockDroidSpawner->count < TROOPER_MAX_SPAWN && playerPadGetButtonDown(player, PAD_UP) > 0)
        droidSpawn(MOBY_ID_SHOCK_DROID_SPAWNER, 0x1f0);
	if (info.data.ballBotSpawner->count < BALL_BOT_SPAWNER_MAX_SPAWN && playerPadGetButtonDown(player, PAD_LEFT) > 0)
        droidSpawn(BALL_BOT_SPAWNER_OCLASS, 0x1f0);
}

void droids(void)
{
	GameSettings *gs = gameGetSettings();
	GameOptions *go = gameGetOptions();
	if (gs->GameLoadStartTime > 0) {
		go->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 1;
	}

    Player *p = playerGetFromSlot(0);
    if (!isInGame() && !p)
        return;

    printf("\nrunning droids");
    droids_run();
}
