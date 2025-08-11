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

#define DRAW_SCALE                  (0.5)

int spawned = 0;

struct HBoltPVar {
    int DestroyAtTime;
    PartInstance_t* Particles[4];
};

mtx4 tempMatrix = {
    {15, 0, 0, 0},
    {0, 15, 0, 0},
    {0, 0, 5, 0},
    {402.324, 366.073, 201.466, 1}
};

VECTOR corners[8] = {
    {-DRAW_SCALE, -DRAW_SCALE, -DRAW_SCALE, 1},
    { DRAW_SCALE, -DRAW_SCALE, -DRAW_SCALE, 1},
    {-DRAW_SCALE, -DRAW_SCALE,  DRAW_SCALE, 1},
    { DRAW_SCALE, -DRAW_SCALE,  DRAW_SCALE, 1},
    {-DRAW_SCALE,  DRAW_SCALE, -DRAW_SCALE, 1},
    { DRAW_SCALE,  DRAW_SCALE, -DRAW_SCALE, 1},
    {-DRAW_SCALE,  DRAW_SCALE,  DRAW_SCALE, 1},
    { DRAW_SCALE,  DRAW_SCALE,  DRAW_SCALE, 1},
};

void testEffectQuad(float scale, MATRIX position, int texId, int color, int drawStyle)
{
	QuadDef quad;
	VECTOR t;
	VECTOR pTL = {1, 0, 1, 1};
	VECTOR pTR = {-1, 0, 1, 1};
	VECTOR pBL = {1, 0, -1, 1};
	VECTOR pBR = {-1 ,0 , -1, 1};

    // get texture info (tex0, tex1, clamp, alpha)
    gfxSetupEffectTex(&quad, texId, drawStyle, 0x80);

	vector_copy(quad.point[0], pBL);
	vector_copy(quad.point[1], pBR);
	vector_copy(quad.point[2], pTL);
	vector_copy(quad.point[3], pTR);
	quad.rgba[0] = quad.rgba[1] = quad.rgba[2] = quad.rgba[3] = color;
	quad.uv[0] = (UV_t){0, 0};
	quad.uv[1] = (UV_t){0, 1};
	quad.uv[2] = (UV_t){1, 0};
	quad.uv[3] = (UV_t){1, 1};


	t[0] = t[1] = t[2] = scale;
	t[3] = 1;
	matrix_scale(position, position, t);

	// position
	gfxDrawQuad(quad, position);
}

void drawLine(VECTOR pointA, VECTOR pointB)
{
    int i, k;
    MATRIX rotPos;
    VECTOR edge;
    float x0, x1, y0, y1, z0, z1;
    x0 = pointA[0], y0 = pointA[2], z0 = pointA[1];
    x1 = pointB[0], y1 = pointB[2], z1 = pointB[1];

    float dx = fabsf(x1 - x0);
    float dy = fabsf(y1 - y0);
    float dz = fabsf(z1 - z0);
    float stepsf = maxf(maxf(dx, dy), dz);
    int steps = (int)(stepsf + 0.9999f);
    if (steps == 0) return;

    float sx = (x1 - x0) / steps;
    float sy = (y1 - y0) / steps;
    float sz = (z1 - z0) / steps;

    // do rotation
    // setup matrix (rot + pos)
    matrix_unit(rotPos);
    if (dx >= dy && dx >= dz) {
        matrix_rotate_x(rotPos, rotPos, 1);
    } else if (dy >= dx && dy >= dz) {
        matrix_rotate_y(rotPos, rotPos, 1);
    } else {
        matrix_rotate_z(rotPos, rotPos, 1);;
    }

    vector_copy(edge, pointA);
    memcpy(&rotPos[12], edge, sizeof(VECTOR));
    for (k = 0; k <= steps; ++k) {
        // gfxDrawBillboardQuad(.5, .5, MATH_PI, edge, 24, 0x80000000 | HBOLT_SPRITE_COLOR, 0);
        testEffectQuad(1, rotPos, 24, 0x40ffffff, 0);
        vector_add(edge, edge, (VECTOR){sx, sz, sy, 0});
        memcpy(&rotPos[12], edge, sizeof(VECTOR));
    }
}

void faceMe(VECTOR point[8])
{
    int faces[6][4] = {
        {2, 3, 6, 7}, // Top
        {0, 1, 4, 5}, // Bottom
        {0, 1, 2, 3}, // Front
        {4, 5, 6, 7}, // Back
        {0, 4, 2, 6}, // Left
        {1, 5, 2, 7}  // Right
    };
    int i;
    MATRIX rotPos;
    matrix_unit(rotPos);
    for (i = 2; i < 6; i++) {
        float x = point[faces[i][0]][0] + point[faces[i][1]][0] + point[faces[i][2]][0] + point[faces[i][3]][0];
        float z = point[faces[i][0]][1] + point[faces[i][1]][1] + point[faces[i][2]][1] + point[faces[i][3]][1];
        float y = point[faces[i][0]][2] + point[faces[i][1]][2] + point[faces[i][2]][2] + point[faces[i][3]][2];
        // gfxDrawBillboardQuad(2, 1, MATH_PI, (VECTOR){x * 0.25, z * 0.25, y * 0.25, 1}, 24, 0x40000000 | HBOLT_SPRITE_COLOR, 0);
        switch (i) {
            case 0: matrix_rotate_y(rotPos, rotPos, 1); break;
            case 1: matrix_rotate_y(rotPos, rotPos, -1); break;
            case 2: matrix_rotate_z(rotPos, rotPos, 1); break;
            case 3: matrix_rotate_z(rotPos, rotPos, -1); break;
            case 4: matrix_rotate_x(rotPos, rotPos, -1); break;
            case 5: matrix_rotate_x(rotPos, rotPos, 1); break;
        }
        memcpy(&rotPos[12], (VECTOR){x * 0.25, z * 0.25, y * 0.25, 1}, sizeof(VECTOR));
        testEffectQuad(1, rotPos, 24, 0x80ffffff, 0);
    }
}

void circleMe(mtx4 matrix, int segments)
{
    float radius = matrix.v0[0] * .5;
    if (segments < 6) segments = 16;
    float step = (2 * MATH_PI) / segments;

    VECTOR prev, first;
    int i;
    for (i = 0; i <= segments; ++i) {
        float a = (i * step) - MATH_PI;
        VECTOR p = {matrix.v3[0] + radius * cosf(a), matrix.v3[1] + radius * sinf(a), matrix.v3[2], 1};
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
    int edges[2][12][2] = {
        {
            {2, 3}, {3, 7}, {7, 6}, {6, 2}, // Top
            {0, 1}, {1, 5}, {5, 4}, {4, 0}, // Bottom
            {0, 2}, {1, 3}, {4, 6}, {5, 7}  // Verticals
        }, {
            {0, 1}, {1, 3}, {3, 2}, {2, 0}, // Front
            {4, 5}, {5, 7}, {7, 6}, {6, 4}, // Back
            {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Z
        }
    };
    int i;
    for (i = 0; i < 12; ++i) {
        int a = edges[0][i][0];
        int b = edges[0][i][1];
        drawLine(corner[a], corner[b]);
    }
}

void drawTheThingJulie(Moby *moby)
{
	VECTOR worldCorners[8];
	int i;
	for (i = 0; i < 8; i++) {
        vector_transform(worldCorners[i], corners[i], (MATRIX*)&tempMatrix);
		worldCorners[i][2] += tempMatrix.v2[2] * .5;
    }
    // edgeMe(worldCorners);
    faceMe(worldCorners);
    // circleMe(tempMatrix, 180);
}

void postDraw(Moby *moby)
{
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

	((void(*)(Moby *))0x003f7d50)(moby);

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
        spawn(tempMatrix.v3);
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