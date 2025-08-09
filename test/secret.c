#include <tamtypes.h>

#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/uya.h>
#include <libuya/weapon.h>
#include <libuya/interop.h>
#include <libuya/moby.h>
#include <libuya/graphics.h>
#include <libuya/gamesettings.h>
#include <libuya/spawnpoint.h>
#include <libuya/team.h>
#include <libuya/ui.h>
#include <libuya/time.h>
#include <libuya/camera.h>
#include <libuya/gameplay.h>
#include <libuya/map.h>
#include <libuya/collision.h>
#include <libuya/guber.h>
#include <libuya/sound.h>
#include <libuya/music.h>

#define HBOLT_MOBY_OCLASS			(0x1C0D)
#define HBOLT_PICKUP_RADIUS		    (3)
#define HBOLT_PARTICLE_COLOR        (0x0000ffff)
#define HBOLT_SPRITE_COLOR          (0x00ffffff)
#define HBOLT_SCALE                 (0.5)

int spawned = 0;

struct HBoltPVar {
    int DestroyAtTime;
    PartInstance_t* Particles[4];
};

mtx3 tempMatrix = {
    {15, 0, 0, 0},
    {0, 15, 0, 0},
    {0, 0, 5, 0}
};

VECTOR corners[8] = {
    {-.5, -.5, -.5, 1},
    { .5, -.5, -.5, 1},
    {-.5, -.5,  .5, 1},
    { .5, -.5,  .5, 1},
    {-.5,  .5, -.5, 1},
    { .5,  .5, -.5, 1},
    {-.5,  .5,  .5, 1},
    { .5,  .5,  .5, 1},
};

int edges[12][2] = {
    {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Front
    {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Back
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Z
};

int faces[6][4] = {
    {0, 1, 4, 5}, // Bottom
    {2, 3, 6, 7}, // Top
    {0, 1, 2, 3}, // Front
    {4, 5, 6, 7}, // Back
    {0, 4, 2, 6}, // Left
    {1, 5, 2, 7}  // Right
};

void drawLine(VECTOR pointA, VECTOR pointB)
{
    int i, k;
    VECTOR edge;
    float x0, x1, y0, y1, z0, z1;
    x0 = pointA[0];
    y0 = pointA[2];
    z0 = pointA[1];
    x1 = pointB[0];
    y1 = pointB[2];
    z1 = pointB[1];

    // do logic
    float dx = fabsf(x1 - x0);
    float dy = fabsf(y1 - y0);
    float dz = fabsf(z1 - z0);
    float stepsf = maxf(maxf(dx, dy), dz);
    int steps = (int)(stepsf + 0.9999f);              // at least 1 if any delta > 0
    if (steps == 0) return;

    float sx = (x1 - x0) / steps;
    float sy = (y1 - y0) / steps;
    float sz = (z1 - z0) / steps;

    vector_copy(edge, pointA);
    for (k = 0; k <= steps; ++k) {
        gfxDrawBillboardQuad(.5, .5, MATH_PI, edge, 4, 0x80000000 | HBOLT_SPRITE_COLOR, 0);
        vector_add(edge, edge, (VECTOR){sx, sz, sy, 0});
    }
}


void circleMe(VECTOR center, mtx3 matrix, int segments)
{
    float radius = matrix.v0[0];
    if (segments < 6) segments = 16;
    float step = (2.0f * MATH_PI) / segments;

    VECTOR prev, first;
    int i;
    for (i = 0; i <= segments; ++i) {
        float a = i * step;

        VECTOR p = {center[0] + radius * cosf(a), center[1] + radius * sinf(a), center[2], 1};
        if (i == 0) {
            vector_copy(first, p);
        } else {
            drawLine(prev, p);
        }
        vector_copy(prev, p);
    }
    drawLine(first, prev);
}

void edgeMe(VECTOR corner[8])
{
    int i;
    for (i = 0; i < 12; ++i) {
        int a = edges[i][0];
        int b = edges[i][1];
        drawLine(corner[a], corner[b]);
    }
}

void drawTheThingJulie(Moby *moby)
{
	VECTOR worldCorners[8];
	int i;
	for (i = 0; i < 8; i++) {
		transform_vector(worldCorners[i], tempMatrix, corners[i], moby->position);
		worldCorners[i][2] += tempMatrix.v2[2] * .5;
    }
    // edgeMe(worldCorners);
    circleMe(moby->position, tempMatrix, 36);
}

void postDraw(Moby *moby)
{
    // QuadDef quad;
    // MATRIX m2;
    // VECTOR t;
    int opacity = 0x80;
    struct HBoltPVar* pvars = (struct HBoltPVar*)moby->pVar;
    if (!pvars)
        return;

    // determine color
    u32 color = HBOLT_SPRITE_COLOR;

    // fade as we approach destruction
    // int timeUntilDestruction = (pvars->DestroyAtTime - gameGetTime()) / TIME_SECOND;
    // if (timeUntilDestruction < 1)
    //     timeUntilDestruction = 1;

    // if (timeUntilDestruction < 10) {
    //     float speed = timeUntilDestruction < 3 ? 20.0 : 3.0;
    //     float pulse = (1 + sinf(clampAngle((gameGetTime() / 1000.0) * speed))) * 0.5;
    //     opacity = 32 + (pulse * 96);
    // }

    opacity = opacity << 24;
    color = opacity | (color & HBOLT_SPRITE_COLOR);
    moby->primaryColor = color;

    // u32 hook = (u32)GetAddress(&vaReplace_GetEffectTexJAL) + 0x20;
    // HOOK_JAL(hook, GetAddress(&vaGetFrameTex));
    // gfxDrawBillboardQuad(0.55, 0, MATH_PI, moby->position, 1, opacity, 0);
    // gfxDrawBillboardQuad(0.5, 0.01, MATH_PI, moby->position, 1, color, 0);
    drawTheThingJulie(moby);
    // HOOK_JAL(hook, GetAddress(&vaGetEffectTex));
}

void update(Moby* moby)
{
    const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
    const int opacities[] = { 64, 32, 44, 51 };
    VECTOR t;
    int i;
    struct HBoltPVar* pvars = (struct HBoltPVar*)moby->pVar;
    if (!pvars)
        return;

    gfxRegistserDrawFunction(&postDraw, moby);

    // handle particles
    // u32 color = colorLerp(0, HBOLT_PARTICLE_COLOR, 1.0 / 4);
    // color |= 0x80000000;
    // for (i = 0; i < 4; ++i) {
    //     PartInstance_t * particle = pvars->Particles[i];
    //     if (!particle) {
    //         particle = gfxSpawnParticle(moby->position, HBOLT_MOBY_OCLASS, color, 100, i);
    //     }

    //     // update
    //     if (particle) {
    //         particle->rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
    //     }
    // }

    // handle pickup
    for (i = 0; i < GAME_MAX_LOCALS; ++i) {
        Player* p = playerGetFromSlot(i);
        if (!p || playerIsDead(p))
            continue;

        vector_subtract(t, p->playerPosition, moby->position);
        if (vector_sqrmag(t) < (HBOLT_PICKUP_RADIUS * HBOLT_PICKUP_RADIUS)) {
            uiShowPopup(0, "HUG YOUR MOTHER!!\x0", 3);
            // soundPlayByOClass(2, 0, moby, MOBY_ID_OMNI_SHIELD);
            break;
        }
    }
    // handle auto destruct
    // if (pvars->DestroyAtTime && gameGetTime() > pvars->DestroyAtTime) {
    //     scavHuntHBoltDestroy(moby);
    // }
}

void spawn(VECTOR position)
{
    Moby* moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(struct HBoltPVar));
    if (!moby) return;

    moby->pUpdate = &update;
    vector_copy(moby->position, position);
    moby->updateDist = -1;
    moby->drawn = 1;
    moby->opacity = 0x00;
    moby->drawDist = 0x00;

    // update pvars
    struct HBoltPVar* pvars = (struct HBoltPVar*)moby->pVar;
    // pvars->DestroyAtTime = gameGetTime() + (TIME_SECOND * 30);
    memset(pvars->Particles, 0, sizeof(pvars->Particles));

    // mobySetState(moby, 0, -1);
    // scavHuntResetBoltSpawnCooldown();
    soundPlayByOClass(1, 0, moby, MOBY_ID_OMNI_SHIELD);
}

void runUltimateSecret(void)
{
    if (!spawned) {
        spawn((VECTOR){402.324, 366.073, 201.466, 1});
        spawned = 1;
    }
}

void secret(void)
{
    GameSettings *gs = gameGetSettings();
    GameData *gd = gameGetData();

    // only continue if enabled and in game
    if (!isInGame() || !gs || !gd) {
        return;
    }
    Player *p = playerGetFromSlot(0);

    runUltimateSecret();
}