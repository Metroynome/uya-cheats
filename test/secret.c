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

#define DRAW_SCALE (1)
#define MAX_SEGMENTS (64)
#define MIN_SEGMENTS (4)

int spawned = 0;
int isCircle = 0;

struct HBoltPVar {
    int DestroyAtTime;
    PartInstance_t* Particles[4];
};

Cuboid rawr = {
    .matrix = {
        {64, 0, 0, 0},
        {0, 100, 0, 0},
        {0, 0, 2, 0}
    },
    .pos = {519.58356, 398.7586, 201.38, 1},
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
        testEffectQuad(1, rotPos, 22, 0x40ffffff, 0);
        vector_add(edge, edge, (VECTOR){sx, sz, sy, 0});
        memcpy(&rotPos[12], edge, sizeof(VECTOR));
    }
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
// vec4 quadPos[2][MAX_SEGMENTS];
void circleMeFinal(Cuboid cube)
{
    u32 baseColor = 0x00ffffff; // TEAM_COLORS[playerGetFromSlot(0)->mpTeam];

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

    VECTOR center, tempCenter, tempRight, tempUp, halfX, halfZ, vRadius;
    vector_copy(center, cube.pos);
    VECTOR xAxis = {cube.matrix.v0[0], cube.matrix.v1[0], cube.matrix.v2[0], 0};
    VECTOR zAxis = {cube.matrix.v0[1], cube.matrix.v1[1], cube.matrix.v2[1], 0};
    VECTOR yAxis = {cube.matrix.v0[2], cube.matrix.v1[2], cube.matrix.v2[2], 0};
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
    for (i = 0; i < 4; ++i) {
        vector_add(corners[i], corners[i], offset);
    }
    vector_copy(quad[2].point[0], corners[1]);
    vector_copy(quad[2].point[1], corners[0]);
    vector_copy(quad[2].point[2], corners[2]);
    vector_copy(quad[2].point[3], corners[3]);

    gfxDrawQuad(quad[2], NULL);
}

void drawCircleFan(mtx4 matrix, int segments)
{
    int texId = 7;
    float thetaStep = 2 * (float)MATH_PI / (float)segments;

    // Setup a quad/triangle def
    QuadDef tri;
    gfxSetupEffectTex(&tri, texId, 0, 0x80);

    // Solid color (semi-transparent white)
    int i;
    for (i = 0; i < 4; i++)
        tri.rgba[i] = 0x30ffffff;

    // Axis and radius
    VECTOR r = {matrix.v0[0] * 0.5f, matrix.v1[0] * 0.5f, matrix.v2[0] * 0.5f, 0};
    VECTOR axis = {matrix.v0[2], matrix.v1[2], matrix.v2[2], 1};
    vector_normalize(axis, axis);

    // Tangent (initial right vector)
    VECTOR right;
    vector_outerproduct(right, axis, r);
    vector_normalize(right, right);

    // Center of the fan (bottom circle’s plane)
    VECTOR center;
    vector_copy(center, matrix.v3);

    // Make a copy so we can iterate rim points
    VECTOR rim;
    vector_copy(rim, r);

    // First rim point
    VECTOR prev;
    vector_add(prev, center, rim);
    for (i = 1; i <= segments; i++) {
        VECTOR next;
        vector_rodrigues(rim, rim, axis, thetaStep);
        vector_add(next, center, rim);

        // Define triangle: center–prev–next
        tri.point[0][0] = center[0];
        tri.point[0][1] = center[1];
        tri.point[0][2] = center[2];
        tri.point[0][3] = 1.0f;

        tri.point[1][0] = prev[0];
        tri.point[1][1] = prev[1];
        tri.point[1][2] = prev[2];
        tri.point[1][3] = 1.0f;

        tri.point[2][0] = next[0];
        tri.point[2][1] = next[1];
        tri.point[2][2] = next[2];
        tri.point[2][3] = 1.0f;

        // dummy UVs (whole texture)
        tri.uv[0] = (UV_t){0.5f, 0.5f};
        tri.uv[1] = (UV_t){0.0f, 1.0f};
        tri.uv[2] = (UV_t){1.0f, 1.0f};

        // Draw triangle
        gfxDrawQuad(tri, NULL);

        // Move forward
        vector_copy(prev, next);
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
UV_t uvs[4];
float rgbas[4];
void stripMe(VECTOR point[8])
{
    // front
    memcpy(lePoints[0], &point[0], sizeof(vec3));
    memcpy(lePoints[1], &point[1], sizeof(vec3));
    memcpy(lePoints[2], &point[2], sizeof(vec3));
    memcpy(lePoints[3], &point[3], sizeof(vec3));
    // back
    // memcpy(backPoints[0], &point[4], sizeof(vec3));
    // memcpy(backPoints[1], &point[5], sizeof(vec3));
    // memcpy(backPoints[2], &point[6], sizeof(vec3));
    // memcpy(backPoints[3], &point[7], sizeof(vec3));
    rgbas[0] = 0x80ffffff;
    rgbas[1] = 0x80ffffff;
    rgbas[2] = 0x80ffffff;
    rgbas[3] = 0x80ffffff;
    uvs[0].x = 0;
    uvs[0].y = 0;
    uvs[1].x = 0;
    uvs[1].y = 1;
    uvs[2].x = 1;
    uvs[2].y = 0;
    uvs[3].x = 1;
    uvs[3].y = 1;

    gfxDrawStripInit();
    gfxDrawStrip(FX_GADGETRON, lePoints, rgbas, uvs, 1);
    // gfxDrawStrip(0x17, &backPoints, &rgbas, &uvs, 4);
}

int debugToPlayer = 0;
void drawTheThingJulie(Moby *moby)
{
	VECTOR worldCorners[8];
	int i;
    // Check if needs to be circle or square
    float len = vector_length(rawr.matrix.v2);
    if (len > 1.0001) {
        isCircle = 1;
    }
    // for debug, make player be center of cuboid.
    // if (!debugToPlayer) {
    //     vector_copy(tempMatrix.v3, playerGetFromSlot(0)->pMoby->position);
    //     debugToPlayer = 1;
    // }

	for (i = 0; i < 8; i++) {
        vector_transform(worldCorners[i], corners[i], (MATRIX*)&rawr.matrix);
		worldCorners[i][2] += rawr.matrix.v2[2] * .5;
    }
    if (isCircle) {
        // circleMe3(tempMatrix); // rodrigues rotation (DO NOT EDIT, WORKS)
        // circleMeFull(tempMatrix, NUM_SEGMENTS);
    } else {
        // edgeMe(worldCorners);
        // faceMe(worldCorners);
    }
    // stripMe(worldCorners);
    circleMeFinal(rawr); // rodrigues rotation (DO NOT EDIT, WORKS)
    // edgeMe(worldCorners);
    // myDrawCallback(worldCorners);
}



void drawTexturedRibbon(VECTOR startPos, VECTOR endPos, float width)
{
    gfxDrawStripInit();
    
    // Calculate perpendicular vector for width
    VECTOR direction, perpendicular;
    vector_subtract(direction, endPos, startPos);
    vector_normalize(direction, direction);
    
    // Create perpendicular vector (UYA format: {x, z, y} - assuming z-up)
    perpendicular[0] = -direction[2];  // x = -y (from standard)
    perpendicular[1] = 0;              // z = 0 (ground level)
    perpendicular[2] = direction[0];   // y = x (from standard)
    vector_scale(perpendicular, perpendicular, width * 0.5f);
    
    // 4 vertices for ribbon - CORRECT TRISTRIP ORDER
    vec3 positions[4];
    
    // Bottom-left (start)
    positions[0][0] = startPos[0] - perpendicular[0];
    positions[0][1] = startPos[1] - perpendicular[1];
    positions[0][2] = startPos[2] - perpendicular[2];
    
    // Bottom-right (start)
    positions[1][0] = startPos[0] + perpendicular[0];
    positions[1][1] = startPos[1] + perpendicular[1];
    positions[1][2] = startPos[2] + perpendicular[2];
    
    // Bottom-left (end)
    positions[2][0] = endPos[0] - perpendicular[0];
    positions[2][1] = endPos[1] - perpendicular[1];
    positions[2][2] = endPos[2] - perpendicular[2];
    
    // Bottom-right (end)
    positions[3][0] = endPos[0] + perpendicular[0];
    positions[3][1] = endPos[1] + perpendicular[1];
    positions[3][2] = endPos[2] + perpendicular[2];
    
    int colors[4] = {
        0x80FFFFFF,
        0x80FFFFFF,
        0x80FFFFFF,
        0x80FFFFFF
    };
    
    // UV mapping - matches vertex order
    struct UV uvs[4] = {
        {0.0f, 0.0f},  // Start left
        {1.0f, 0.0f},  // Start right
        {0.0f, 1.0f},  // End left
        {1.0f, 1.0f}   // End right
    };
    
    gfxDrawStrip(FX_BASELIGHT, positions, colors, uvs, 1);
}

// Example usage in a draw callback
void myDrawCallback(float points[8])
{
    VECTOR start, end;
    VECTOR offsetEnd = {20, 0, 20, 0};
    VECTOR offsetStart = {1, 0, 1, 0};
    vector_add(start, offsetStart, rawr.pos);
    vector_add(end, offsetEnd, start);
    drawTexturedRibbon(start, end, 10.0f);
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
        spawn(rawr.pos);
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