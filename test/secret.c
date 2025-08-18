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

void testDrawStrip(float scale, MATRIX position, int texId, int color, int drawStyle)
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
    // if (dx >= dy && dx >= dz) {
    //     matrix_rotate_x(rotPos, rotPos, 1);
    // } else if (dy >= dx && dy >= dz) {
    //     matrix_rotate_y(rotPos, rotPos, 1);
    // } else {
    //     matrix_rotate_z(rotPos, rotPos, 1);;
    // }

    vector_copy(edge, pointA);
    memcpy(&rotPos[12], edge, sizeof(VECTOR));
    for (k = 0; k <= steps; ++k) {
        // gfxDrawBillboardQuad(.5, .5, MATH_PI, edge, 24, 0x80000000 | HBOLT_SPRITE_COLOR, 0);
        testEffectQuad(1, rotPos, 24, 0x40ffffff, 0);
        vector_add(edge, edge, (VECTOR){sx, sz, sy, 0});
        memcpy(&rotPos[12], edge, sizeof(VECTOR));
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

void drawSegment(VECTOR pointA, VECTOR pointB, VECTOR circleCenter)
{
    MATRIX rotPos;
    VECTOR centerPoint, direction;
    // setup matrix (rot + pos)
    matrix_unit(rotPos);
    
    // add points and scale to get center.
    vector_add(centerPoint, pointA, pointB);
    vector_scale(centerPoint, centerPoint, .5);
    
    // direction from circle center to midpoint (XZ only)
    vector_subtract(direction, centerPoint, circleCenter);
    float angle = atan2f(direction[0], direction[2]);
    // rotate so +Z of the quad faces outward
    matrix_rotate_y(rotPos, rotPos, angle);

    // copy position vector to matrix
    memcpy(&rotPos[12], centerPoint, sizeof(VECTOR));
    // draw
    // testDrawStrip(1, rotPos, FX_TIRE_TRACKS + 1, 0x40ffffff, 0);
    testEffectQuad(1, rotPos, FX_TIRE_TRACKS + 1, 0x40ffffff, 0);
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
    for (i = 0; i < 1; i++) {
        float x = .25 * (point[faces[i][0]][0] + point[faces[i][1]][0] + point[faces[i][2]][0] + point[faces[i][3]][0]);
        float z = .25 * (point[faces[i][0]][1] + point[faces[i][1]][1] + point[faces[i][2]][1] + point[faces[i][3]][1]);
        float y = .25 * (point[faces[i][0]][2] + point[faces[i][1]][2] + point[faces[i][2]][2] + point[faces[i][3]][2]);
        VECTOR scaled = {x, z, y, 0};
        memcpy(&rotPos[12], scaled, sizeof(VECTOR));
        switch (i) {
            case 0: matrix_rotate_y(&rotPos, rotPos, 1); break;
            case 1: matrix_rotate_y(&rotPos, rotPos, -1); break;
            case 2: matrix_rotate_z(&rotPos, rotPos, 1); break;
            case 3: matrix_rotate_z(&rotPos, rotPos, -1); break;
            case 4: matrix_rotate_x(&rotPos, rotPos, -1); break;
            case 5: matrix_rotate_x(&rotPos, rotPos, 1); break;
        }
        testEffectQuad(1, rotPos, FX_GADGETRON, 0x80ffffff, 0);
    }
}

// void circleMe(mtx4 matrix, int segments)
// {
//     VECTOR prev, first, n;
//     int i;
//     float radius = matrix.v0[0] * .5;
//     float step = (2 * MATH_PI) / segments;

//     vector_copy(n, matrix.v3);
//     // vector_normalize(n, n);

//     for (i = 0; i <= segments; ++i) {
//         float a = (i * step) - MATH_PI;
//         VECTOR p = {n[0] + radius * cosf(a), n[1] + radius * sinf(a), n[2] + 2, 0};
//         if (i == 0) {
//             vector_copy(first, p);
//         } else {
//             drawSegment(prev, p);
//         }
//         vector_copy(prev, p);
//     }
//     drawSegment(first, prev);
// }

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

void drawCircle(mtx4 position, int segments)
{
    VECTOR from, to, centerPoint;
    VECTOR points[segments];
    float step = (2 * MATH_PI) / (float)segments;
    float radius = position.v0[0] * .5;
    // vector_copy(r, (VECTOR){position.v0[0], position.v1[0], position.v2[0], 0}); // radius vector
    // vector_normalize(axis, (VECTOR){position.v0[1], position.v1[1], position.v2[1], 0});
    int i;
    for (i = 0; i < segments; ++i) {
        float theta1 = (step * i);
        float theta2 = step * (i + 1);

        // First point
        vector_fromyaw(from, theta1 - MATH_PI);
        vector_scale(from, from, radius);
        vector_add(from, from, position.v3);

        // Second point
        vector_fromyaw(to, theta2 - MATH_PI);
        vector_scale(to, to, radius);
        vector_add(to, to, position.v3);
        drawSegment(from, to, position.v3);
    }
}

void circleMe3(mtx4 matrix, int segments)
{
    int color = 0x40ffffff;
    float scale = 1.5;
    int texId = FX_TIRE_TRACKS + 1;

    
    QuadDef quad;
    VECTOR pos, right, up;
    float thetaStep = 2.0f * (float)MATH_PI / (float)segments;

    // get texture info (tex0, tex1, clamp, alpha)
    gfxSetupEffectTex(&quad, texId, 0, 0x80);

	quad.rgba[0] = quad.rgba[1] = quad.rgba[2] = quad.rgba[3] = color;
	quad.uv[0] = (UV_t){0, 0};
	quad.uv[1] = (UV_t){0, 1};
	quad.uv[2] = (UV_t){1, 0};
	quad.uv[3] = (UV_t){1, 1};

    // get radius (half of x)
    VECTOR r = {matrix.v0[0] *.5, matrix.v1[0] * .5, matrix.v2[0] * .5, 0}; // x-axis halved.
    // axis of which to go around (y in this case)
    VECTOR axis = {matrix.v0[2], matrix.v1[2], matrix.v2[2], 1}; // y-axis

    // normalize axis
    vector_normalize(axis, axis);

    // get tangent
    vector_outerproduct(right, axis, r);            // tangent = axis × r   (swap order if winding is flipped)
    vector_normalize(right, right);

    // Not needed: scale x, y of texture
    // vector_scale(right, right, 1);
    // vector_scale(up, axis, 1);
    
    int i;
    for (i = 0; i < segments; ++i) {
        VECTOR center;
        vector_add(center, matrix.v3, r);
        int signs[4][2] = {
            { 1, -1 },  // corner 0: +right -up
            { -1, -1 }, // corner 1: -right -up
            { 1, 1 },   // corner 2: +right +up
            { -1, 1 }   // corner 3: -right +up
        };

        // create vector for each point.
        int j;
        for (j = 0; j < 4; ++j) {
            quad.point[j][0] = center[0] + signs[j][0] * right[0] + signs[j][1] * up[0];
            quad.point[j][1] = center[1] + signs[j][0] * right[1] + signs[j][1] * up[1];
            quad.point[j][2] = center[2] + signs[j][0] * right[2] + signs[j][1] * up[2];
            quad.point[j][3] = 1.0f;
        }

        gfxDrawQuad(quad, NULL);

        // rotate radius and tangent
        vector_rodrigues(r, r, axis, thetaStep);
        vector_rodrigues(right, right, axis, thetaStep);
    }
}

// uses gfxDrawStrip
typedef struct Vertex {
    vec3 pos;
    UV_t uv;
    int rgba;
} Vertex_t;
void circleMe4(mtx4 matrix, int segments)
{
    int MAX_SEGS = 100;
    int color = 0x40ffffff;
    float scale = 1;
    int texId = FX_TIRE_TRACKS + 1;

    int isCircle = 0;
    Vertex_t top[MAX_SEGS + 2];
    Vertex_t bottom[MAX_SEGS + 2];

    // check length of Y to see if it's a circle or square.
    float len = vector_length(matrix.v2);
    if (len > 1.0001) {
        isCircle = 1;
    }
    int n = isCircle ? (segments > 3 ? (segments < MAX_SEGS ? segments : MAX_SEGS) : 4) : 4;   

    VECTOR r = {matrix.v0[0] * .5, matrix.v1[0] * .5, matrix.v2[0] * .5, 1}; // x-axis halved.
    VECTOR axis = {matrix.v0[2], matrix.v1[2], matrix.v2[2], 1}; // y-axis
    float timePhase = gameGetTime() / 120.0f;
    float sStep = 16.0f / (float)n;
    float thetaStep = 2.0f * (float)MATH_PI / (float)n;
    VECTOR pos;
    vector_copy(pos, matrix.v3);
    int i;
    gfxDrawStripInit();
    for (i = 0; i < n; ++i) {
        VECTOR pw;
        vector_add(pw, pos, r);

        float s = sStep * (float)i - 8.0f;
        memcpy(top[i].pos, &pw, sizeof(vec3));
        top[i].uv.x = s;
        top[i].uv.y = timePhase;
        top[i].rgba = color;
        bottom[i] = top[i];
        // get next point
        vector_rodrigues(r, r, axis, thetaStep);
    }
    int k;
    for (k = 0; k <= n; ++k) {
        VECTOR p;
        VECTOR pt;
        vector_copy(p, (VECTOR){top[k].pos[0], top[k].pos[1], top[k].pos[2], 0});
        vector_scale(pt, p, 3);
        vector_add(pt, pt, p);
        memcpy(top[k].pos, &pt, sizeof(vec3));
        top[k].rgba = color;

        // bottom strip offset
        VECTOR pb;
        vector_scale(pb, p, -1.5);
        vector_add(pb, pb, p);
        memcpy(bottom[k].pos, &pt, sizeof(vec3));
        bottom[k].rgba = color;
    
        gfxDrawStrip(FX_TIRE_TRACKS + 1, &top[k].pos, &top[k].rgba, &top[k].uv, 0);
    }
}

vec3 lePoints[4];
vec3 backPoints[4];
void stripMe(VECTOR point[8])
{
    // front
    memcpy(lePoints[0], &point[0], sizeof(vec3));
    memcpy(lePoints[1], &point[1], sizeof(vec3));
    memcpy(lePoints[2], &point[2], sizeof(vec3));
    memcpy(lePoints[3], &point[3], sizeof(vec3));
    // back
    memcpy(backPoints[0], &point[4], sizeof(vec3));
    memcpy(backPoints[1], &point[5], sizeof(vec3));
    memcpy(backPoints[2], &point[6], sizeof(vec3));
    memcpy(backPoints[3], &point[7], sizeof(vec3));
    float rgbas[4];
    rgbas[0] = 0x80ffffff;
    rgbas[1] = 0x80ffffff;
    rgbas[2] = 0x80ffffff;
    rgbas[3] = 0x80ffffff;
    UV_t uvs[4];
    uvs[0].x = 0;
    uvs[0].y = 0;
    uvs[1].x = 0;
    uvs[1].y = 1;
    uvs[2].x = 1;
    uvs[2].y = 0;
    uvs[3].x = 1;
    uvs[3].y = 1;
    gfxDrawStripInit();
    gfxDrawStrip(0x16, &lePoints, &rgbas, &uvs, 1);
    gfxDrawStrip(0x17, &backPoints, &rgbas, &uvs, 1);
}

void drawTheThingJulie(Moby *moby)
{
    int isCircle = 0;
	VECTOR worldCorners[8];
	int i;
    // Check if needs to be circle or square
    float len = vector_length(tempMatrix.v2);
    if (len > 1.0001) {
        isCircle = 1;
    }
	for (i = 0; i < 8; i++) {
        vector_transform(worldCorners[i], corners[i], (MATRIX*)&tempMatrix);
		worldCorners[i][2] += tempMatrix.v2[2] * .5;
    }
    if (isCircle) {
        circleMe3(tempMatrix, 50); // rodrigues rotation
        // circleMe(tempMatrix, 100);
    } else {
        // edgeMe(worldCorners);
        // faceMe(worldCorners);
    }
    // faceMe(worldCorners);
    // drawCircle(tempMatrix, 100);
    // circleMe(tempMatrix, 100);
    // stripMe(worldCorners);
    // circleMe4(tempMatrix, 100); // just like DL
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
            uiShowPopup(0, "HUG YOUR MOTHER!!\x0", 0);
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