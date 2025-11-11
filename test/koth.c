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

#define MAX_SEGMENTS (64)
#define MIN_SEGMENTS (4)

int spawned = 0;

typedef struct hillPvar {
    short state;
    short isCircle;
    Player *playersIn[GAME_MAX_PLAYERS];
    Cuboid *currentCuboid;
    int teamTime[GAME_MAX_PLAYERS];
    u32 color;
} hillPvar_t;

Cuboid rawr = {
    .matrix = {
        {30, 0, 0, 0},
        {0, 30, 0, 0},
        {0, 0, 2, 0}
    },
    .pos = {519.58356, 398.7586, 201.38, 1},
    .imatrix = 0,
    .rot = {0, 0, 0, 0}
};

int hillCheckIfInside(Cuboid cube, VECTOR playerPos)
{
    MATRIX rotMatrix;
    matrix_unit(rotMatrix);
    matrix_rotate_x(rotMatrix, rotMatrix, cube.rot[0]);
    matrix_rotate_y(rotMatrix, rotMatrix, cube.rot[1]);
    matrix_rotate_z(rotMatrix, rotMatrix, cube.rot[2]);
    
    // get imatrix
    MATRIX invMatrix;
    matrix_unit(invMatrix);
    matrix_inverse(invMatrix, rotMatrix);
    memcpy(&cube.imatrix, invMatrix, sizeof(mtx3));

    VECTOR delta;
    vector_subtract(delta, playerPos, (VECTOR){cube.pos[0], cube.pos[1], cube.pos[2], 0});
    vector_apply(delta, delta, invMatrix);
    VECTOR xAxis = {cube.matrix.v0[0], cube.matrix.v1[0], cube.matrix.v2[0], 0};
    VECTOR zAxis = {cube.matrix.v0[1], cube.matrix.v1[1], cube.matrix.v2[1], 0};
    VECTOR yAxis = {cube.matrix.v0[2], cube.matrix.v1[2], cube.matrix.v2[2], 0};

    float halfWidth = vector_length(xAxis) * .5;
    float halfDepth = vector_length(zAxis) * .5;
    float yHeight = vector_length(yAxis);
    
    if (delta[2] < -1.25 || delta[2] > yHeight + 6) {
        return 0;
    }
    if (isCircle) {
        float radius = halfWidth;
        float distSq = delta[0] * delta[0] + delta[1] * delta[1];
        return (distSq <= radius * radius);
    } else {
        int x = delta[0] < -halfWidth || delta[0] > halfWidth;
        int z = delta[1] < -halfDepth || delta[1] > halfDepth;
        return (x || z) ? 0 : 1;
    }
}

void hillPlayerUpdates(Moby *this)
{
    hillPvar_t *pvar = (hillPvar_t*)this->pVar;
    GameSettings *gs = gameGetSettings();
    int playerCount = gs->PlayerCount;
    int i;
    for (i = 0; i < playerCount; ++i) {
        Player *player = playerGetFromSlot(i);
        if (!player || playerIsDead(player))
            continue;

        int in = hillCheckIfInside(*pvar->currentCuboid, player->playerPosition);
        printf("\ninside hill: %d", in);
        if (in) {
            pvar->color = 0x00ffffff; // TEAM_COLORS[player->mpTeam];
        } else {
            pvar->color = 0x00ffffff;
        }

    }
}

void vector_rodrigues(VECTOR output, VECTOR v, VECTOR axis, float angle)
{
    VECTOR k, v_cross, term1, term2, term3;
    float cosTheta = cosf(angle);
    float sinTheta = sinf(angle);

    // normalize axis into k
    vector_normalize(k, axis);

    // term1 = v * cos(theta)
    vector_scale(term1, v, cosTheta);

    // term2 = (k x v) * sin(theta)
    vector_outerproduct(v_cross, k, v);  // cross product
    vector_scale(term2, v_cross, sinTheta);

    // term3 = k * (k . v) * (1 - cos(theta))
    float dot = vector_innerproduct(k, v);
    vector_scale(term3, k, dot * (1.0f - cosTheta));

    // output = term1 + term2 + term3
    vector_add(output, term1, term2);
    vector_add(output, output, term3);

    // preserve homogeneous component
    output[3] = v[3];
}

float scrollQuad = 0;
void hill_drawShape(Moby *this, Cuboid cube)
{
    hillPvar_t *pvar = (hillPvar_t*)this->pVar;
    int isCircle = pvar->isCircle;
    u32 baseColor = pvar->color; // TEAM_COLORS[playerGetFromSlot(0)->mpTeam];
    

    int i, k, j, s;
    QuadDef quad[3];
    // get texture info (tex0, tex1, clamp, alpha)
    int floorTex = isCircle ? FX_CIRLCE_NO_FADED_EDGE : FX_SQUARE_FLAT_1;
    gfxSetupEffectTex(&quad[0], FX_TIRE_TRACKS + 1, 0, 0x80);
    gfxSetupEffectTex(&quad[2], floorTex, 0, 0x80);

    quad[0].uv[0] = (UV_t){0, 0}; // bottom left (-, -)
    quad[0].uv[1] = (UV_t){0, 1}; // top left (-, +)
    quad[0].uv[2] = (UV_t){1, 0}; // bottom right (+, -)
    quad[0].uv[3] = (UV_t){1, 1}; // top right (+, +)

    // copy quad 0 to quad 2
    memcpy(quad[2].uv, &quad[0].uv, sizeof(quad[0].uv));

    // modify top and bottom level UVs Y.  (uv is turned 90 degrees)
    float uvOffset = isCircle ? 0 : 0; // .04;
    quad[0].uv[0].y += uvOffset;
    quad[0].uv[1].y -= uvOffset;
    quad[0].uv[2].y += uvOffset;
    quad[0].uv[3].y -= uvOffset;

    // copy quad 0 uv to quad 1;
    quad[1] = quad[0];

    // set seperate rgbas
    quad[0].rgba[0] = quad[0].rgba[1] = (0x00 << 24) | baseColor;
    quad[0].rgba[2] = quad[0].rgba[3] = (0x30 << 24) | baseColor;
    quad[1].rgba[0] = quad[1].rgba[1] = (0x50 << 24) | baseColor;
    quad[1].rgba[2] = quad[1].rgba[3] = (0x20 << 24) | baseColor;
    quad[2].rgba[0] = quad[2].rgba[1] = quad[2].rgba[2] = quad[2].rgba[3] = (0x30 << 24) | baseColor;

    MATRIX rotMatrix;
    matrix_unit(rotMatrix);
    matrix_rotate_x(rotMatrix, rotMatrix, cube.rot[0]);
    matrix_rotate_y(rotMatrix, rotMatrix, cube.rot[1]);
    matrix_rotate_z(rotMatrix, rotMatrix, cube.rot[2]);

    VECTOR center, tempCenter, tempRight, tempUp, halfX, halfZ, vRadius;
    vector_copy(center, cube.pos);
    VECTOR xAxis = {cube.matrix.v0[0], cube.matrix.v1[0], cube.matrix.v2[0], 0};
    VECTOR zAxis = {cube.matrix.v0[1], cube.matrix.v1[1], cube.matrix.v2[1], 0};
    VECTOR yAxis = {cube.matrix.v0[2], cube.matrix.v1[2], cube.matrix.v2[2], 0};
    
    // Apply rotation to axes
    vector_apply(xAxis, xAxis, rotMatrix);
    vector_apply(zAxis, zAxis, rotMatrix);
    vector_apply(yAxis, yAxis, rotMatrix);
    
    vector_scale(halfX, xAxis, .5);
    vector_scale(halfZ, zAxis, .5);
    float fRadius = vector_length(halfX);
    int signs[4][2] = {{1, -1}, {-1, -1}, {1, 1}, {-1, 1}};
    vector_normalize(yAxis, yAxis);
    VECTOR corners[4];
    if (isCircle) {
        vector_copy(corners[0], (VECTOR){center[0] - fRadius, center[1] - fRadius, center[2], 0});
        vector_copy(corners[1], (VECTOR){center[0] + fRadius, center[1] - fRadius, center[2], 0});
        vector_copy(corners[2], (VECTOR){center[0] + fRadius, center[1] + fRadius, center[2], 0});
        vector_copy(corners[3], (VECTOR){center[0] - fRadius, center[1] + fRadius, center[2], 0});
    } else {
        vector_copy(corners[0], (VECTOR){center[0]-halfX[0]-halfZ[0], center[1]-halfX[1]-halfZ[1], center[2]-halfX[2]-halfZ[2], 0});
        vector_copy(corners[1], (VECTOR){center[0]+halfX[0]-halfZ[0], center[1]+halfX[1]-halfZ[1], center[2]+halfX[2]-halfZ[2], 0});
        vector_copy(corners[2], (VECTOR){center[0]+halfX[0]+halfZ[0], center[1]+halfX[1]+halfZ[1], center[2]+halfX[2]+halfZ[2], 0});
        vector_copy(corners[3], (VECTOR){center[0]-halfX[0]+halfZ[0], center[1]-halfX[1]+halfZ[1], center[2]-halfX[2]+halfZ[2], 0});
    }

    // get tangent
    vector_outerproduct(tempRight, yAxis, halfX);
    vector_normalize(tempRight, tempRight);

    // scale x, y of texture
    vector_scale(tempRight, tempRight, 1);
    vector_scale(tempUp, yAxis, 1);

    float segmentSize = 1;
    int segments = (int)((2 * MATH_PI * fRadius) / segmentSize);
    float thetaStep = 2 * MATH_PI / clamp((float)segments, MIN_SEGMENTS, MAX_SEGMENTS);
    int segPerSide = 0;  // for squares.

    for (k = 0; k < 2; ++k) {
        if (isCircle) {
            // draw top and botom quad
            // copy vRadius into r
            vector_copy(vRadius, halfX);
            for (i = 0; i < segments; ++i) {
                vector_add(tempCenter, center, vRadius);
                // offset quad[1] by 3
                tempCenter[2] += k * 2;
                // create vector for each point.
                for (j = 0; j < 4; ++j) {
                    quad[k].point[j][0] = tempCenter[0] + signs[j][0] * tempRight[0] + signs[j][1] * tempUp[0];
                    quad[k].point[j][1] = tempCenter[1] + signs[j][0] * tempRight[1] + signs[j][1] * tempUp[1];
                    quad[k].point[j][2] = tempCenter[2] + signs[j][0] * tempRight[2] + signs[j][1] * tempUp[2];
                    quad[k].point[j][3] = 1;
                }

                quad[k].uv[0].x = quad[k].uv[1].x = 0 - scrollQuad;
                quad[k].uv[2].x = quad[k].uv[3].x = 1 - scrollQuad;
                
                // maybe for later:  put points in array.
                // quadPos[k][i] = quad[k].point;
                
                gfxDrawQuad(quad[k], NULL);

                // rotate radius and tangent
                vector_rodrigues(vRadius, vRadius, yAxis, thetaStep);
                vector_outerproduct(tempRight, yAxis, vRadius);
                vector_normalize(tempRight, tempRight);            }
       } else {
            // segment counts
            float sideLenX = vector_length(xAxis);
            float sideLenZ = vector_length(zAxis);
            int segX = clamp((int)(sideLenX / segmentSize), MIN_SEGMENTS, MAX_SEGMENTS);
            int segZ = clamp((int)(sideLenZ / segmentSize), MIN_SEGMENTS, MAX_SEGMENTS);

            for (i = 0; i < 4; ++i) {
                VECTOR p0, p1;
                vector_copy(p0, corners[i]);
                vector_copy(p1, corners[(i + 1) & 3]);

                // choose per-edge segments
                int segCount = (i % 2 == 0) ? segX : segZ;

                vector_subtract(tempRight, p1, p0);
                vector_normalize(tempRight, tempRight);       // quad local X (along edge)

                for (s = 0; s < segCount; ++s) {
                    float t0 = (float)s       / segCount;
                    float t1 = (float)(s + 1) / segCount;

                    VECTOR a, b;
                    vector_lerp(a, p0, p1, t0);
                    vector_lerp(b, p0, p1, t1);

                    // center between segment endpoints
                    vector_add(tempCenter, a, b);
                    vector_scale(tempCenter, tempCenter, 0.5f);

                    tempCenter[2] += k * 2;
                    for (j = 0; j < 4; ++j) {
                        quad[k].point[j][0] = tempCenter[0] + signs[j][0] * tempRight[0] + signs[j][1] * tempUp[0];
                        quad[k].point[j][1] = tempCenter[1] + signs[j][0] * tempRight[1] + signs[j][1] * tempUp[1];
                        quad[k].point[j][2] = tempCenter[2] + signs[j][0] * tempRight[2] + signs[j][1] * tempUp[2];
                        quad[k].point[j][3] = 1;
                    }
                    quad[k].uv[0].x = quad[k].uv[1].x = 0 - scrollQuad;
                    quad[k].uv[2].x = quad[k].uv[3].x = 1 - scrollQuad;

                    gfxDrawQuad(quad[k], NULL);
                }
            }
        }
    }
    // scroll quad to animate
    scrollQuad += .007f;

    // draw floor quad
    VECTOR offset = {0, 0, -1, 0};
    VECTOR rotatedOffset;
    vector_apply(rotatedOffset, offset, rotMatrix);
    for (i = 0; i < 4; ++i) {
        vector_add(corners[i], corners[i], rotatedOffset);
    }
    vector_copy(quad[2].point[0], corners[1]);
    vector_copy(quad[2].point[1], corners[0]);
    vector_copy(quad[2].point[2], corners[2]);
    vector_copy(quad[2].point[3], corners[3]);

    gfxDrawQuad(quad[2], NULL);
}

int debugToPlayer = 0;
void hill_doDraw(Moby *moby)
{
    hillPvar_t *pvar = (hillPvar_t*)moby->pVar;
	VECTOR worldCorners[8];
	int i;
    // Check if needs to be circle or square
    float len = vector_length(rawr.matrix.v2);
    if (len > 1.0001) {
        pvar->isCircle = 1;
    }
    // for debug, make player be center of cuboid.
    // if (!debugToPlayer) {
    //     vector_copy(tempMatrix.v3, playerGetFromSlot(0)->pMoby->position);
    //     debugToPlayer = 1;
    // }

    hill_drawShape(moby, rawr);

}







// void drawTexturedRibbon(VECTOR startPos)
// {
//     gfxDrawStripInit();

//     gfxAddRegister(8, 0);
//     gfxAddRegister(0x14, 0xff9000000260);
//     gfxAddRegister(6, gfxGetEffectTex(FX_LENS_FLARE_1));
//     // blend == 0
//     gfxAddRegister(0x47, 0x513f1);
//     gfxAddRegister(0x42, 0x8000000044);
    
//     float x = rawr.pos[0];
//     float z = rawr.pos[1];
//     float y = rawr.pos[2];

//     vec3 pos[5] = {
//         {x - 20f, z, rawr.pos[2]}, // 0, 0
//         {x - 20f, z, rawr.pos[2] + 20f}, // 0, 1
//         {x, z, rawr.pos[2]}, // 1, 0
//         {x, rawr.pos[1], rawr.pos[2] + 20f} // 1, 1
//         {x + 20f, z, rawr.pos[2]} // 0, 0
//     }

//     // Solid cxxxolor
//     int colors[5] = {
//         0x80FFFFFF,
//         0x80FFFFFF,
//         0x80FFFFFF,
//         0x80FFFFFF,
//         0x80ffffff
//     };
    
//     // UV mapping for ribbon
//     struct UV uvs[5] = {
//         {0.0f, 0.0f},  // Start left
//         {0.0f, 1.0f},  // Start right
//         {1.0f, 0.0f},  // End left
//         {1.0f, 1.0f},   // End right
//         {0f, 0f}
//     };
    
//     gfxDrawStrip(FX_LENS_FLARE_1, pos, colors, uvs, 1);
// }

// // Example usage in a draw callback
// void myDrawCallback(void)
// {
//     // drawTexturedRibbon(rawr.pos);
// }






void hill_postDraw(Moby *moby)
{
    int opacity = 0x80;
    hillPvar_t* pvars = (hillPvar_t*)moby->pVar;
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

    hill_doDraw(moby);
}

void hill_update(Moby* moby)
{
    VECTOR t;
    int i;
    hillPvar_t *pvars = (hillPvar_t*)moby->pVar;
    if (!pvars)
        return;

    gfxRegistserDrawFunction(&hill_postDraw, moby);

    // force set cuboid for now
    pvars->currentCuboid = &rawr;

    // handle players
    hillPlayerUpdates(moby);

    // handle auto destruct
    // if (pvars->DestroyAtTime && gameGetTime() > pvars->DestroyAtTime) {
    //     scavHuntHBoltDestroy(moby);
    // }
}

void spawnHillMoby(VECTOR position)
{
    Moby* moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(hillPvar_t));
    if (!moby) return;

    moby->pUpdate = &hill_update;
    vector_copy(moby->position, position);
    moby->updateDist = -1;
    moby->drawn = 1;
    moby->opacity = 0x00;
    moby->drawDist = 0x00;

    // update pvars
    hillPvar_t* pvars = (hillPvar_t*)moby->pVar;
    // pvars->DestroyAtTime = gameGetTime() + (TIME_SECOND * 30);
    memset(pvars, 0, sizeof(pvars));

    // mobySetState(moby, 0, -1);
    // scavHuntResetBoltSpawnCooldown();
    soundPlayByOClass(1, 0, moby, MOBY_ID_OMNI_SHIELD);
}

void hill_init(void)
{
    if (!spawned) {
        spawnHillMoby(rawr.pos);
        spawned = 1;
    }
}

void koth(void)
{
    GameSettings *gs = gameGetSettings();
    GameData *gd = gameGetData();

    // only continue if enabled and in game
    if (!isInGame() || !gs || !gd) {
        return;
    }
    Player *p = playerGetFromSlot(0);

    hill_init();
}