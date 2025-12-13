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

#define HBOLT_MOBY_OCLASS			(0x3000)
#define HBOLT_PICKUP_RADIUS		    (3)
#define HBOLT_PARTICLE_COLOR        (0x0000ffff)
#define HBOLT_SPRITE_COLOR          (0x00ffffff)
#define HBOLT_SCALE                 (0.5)

#define DRAW_SCALE (1)
#define MAX_SEGMENTS (64)
#define MIN_SEGMENTS (6)
#define CYLINDER_HEIGHT (2)
#define TEXTURE_SCROLL_SPEED (0.007)
#define EDGE_OPACITY (0x40)           /* 0x00 = transparent, 0x80 = full opacity */
#define TEXTURE_EDGE_TRIM_U (0.0f)    /* Trim left/right edges (0.0 - 0.5) */
#define TEXTURE_EDGE_TRIM_V (0.25f)   /* Trim top/bottom edges (0.0 - 0.5) */

int spawned = 0;
int isCircle = 0;

struct HBoltPVar {
    int DestroyAtTime;
    PartInstance_t* Particles[4];
};

typedef struct hillPvar {
    int cuboidIndex[32];
    short state;
    short isCircle;
    Player *playersIn[GAME_MAX_PLAYERS];
    Cuboid *currentCuboid;
    int teamTime[8];
    u32 color;
} hillPvar_t;

Cuboid rawr = {
    .matrix = {
        {20, 0, 0, 0},
        {0, 30, 0, 0},
        {0, 0, 1, 0}
    },
    .pos = {519.58356, 398.7586, 201.38, 1},
    .imatrix = 0,
    .rot = {0, 0, 0, 0}
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

// Calculate optimal vertex count based on radius
int calculateVertexCount(float radius) {
    int vertices = (int)(radius * 0.75f);
    if (vertices < MIN_SEGMENTS) vertices = MIN_SEGMENTS;
    if (vertices > MAX_SEGMENTS) vertices = MAX_SEGMENTS;
    return vertices;
}

float scroll = 0;
void drawCylinder(VECTOR position, float radius, float height, u32 baseColor, int texture) {
    int numVertices = calculateVertexCount(radius);
    float angleStep = (2.0f * MATH_PI) / numVertices;
    
    vec3 p[2][(MAX_SEGMENTS)];
    u32 c[2][(MAX_SEGMENTS)];
    UV_t u[2][(MAX_SEGMENTS)];
    
    /* Initialize strip drawing */
    gfxDrawStripInit();
    gfxAddRegister(8, 0);
    gfxAddRegister(0x14, 0xff9000000260);
    gfxAddRegister(6, gfxGetEffectTex(texture));
    gfxAddRegister(0x47, 0x513f1);
    gfxAddRegister(0x42, 0x8000000044);
    
    /* Build cylinder side as a triangle strip */
    int i;
    for (i = 0; i <= numVertices; i++) {
        float angle = (i % numVertices) * angleStep;
        
        /* Bottom vertex */
        int idx = i * 2;
        p[0][idx][0] = position[0] + cosf(angle) * radius;
        p[0][idx][1] = position[1] + sinf(angle) * radius;
        p[0][idx][2] = position[2] - height;
        c[0][idx] = (0x80 << 24) | baseColor;
        u[0][idx].y = (float)i / numVertices;
        u[0][idx].x = scroll;
        
        /* Top vertex */
        p[0][idx + 1][0] = position[0] + cosf(angle) * radius;
        p[0][idx + 1][1] = position[1] + sinf(angle) * radius;
        p[0][idx + 1][2] = position[2];
        c[0][idx + 1] = (0x80 << 24) | baseColor;
        u[0][idx + 1].y = (float)i / numVertices;
        u[0][idx + 1].x = scroll + 1.0f;
    }
    
    scroll += 0.007f;
    
    /* Draw cylinder side */
    gfxDrawStrip(texture, &p, &c, &u, 1);
}

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

void drawLine_Original(VECTOR pointA, VECTOR pointB)
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

typedef enum LineStyles {
    LINE_STYLE_DEFAULT = 0,
    LINE_STYLE_CUSTOM = 1,
    LINE_STYLE_NUM_STYLES = 2
} LineStyles_e;

typedef struct LineStatic {
    /* 0x00 */ int numSegments;
    /* 0x04 */ int texture;
    /* 0x08 */ float textureRepeatDistance;
    /* 0x0c */ float lineWidth;
    /* 0x14 */ bool billboard;
    /* 0x15 */ bool rotateTexture;
    /* 0x16 */ char pad[2];
    /* 0x18 */ int scrollFrames;
    /* 0x1c */ float textureOffset;
} LineStatic_t;

typedef struct LineEndPoint {
    /* 0x00 */ VECTOR pos;
    /* 0x10 */ int color;
    /* 0x11 */ bool lerpColor;
    /* 0x12 */ char pad[3];
} LineEndPoint_t;

// Default style
LineStatic_t DefaultLineStyle = {
    .numSegments = 10,
    .texture = FX_BASELIGHT,
    .textureRepeatDistance = 0,
    .lineWidth = 1,
    .billboard = 1,
    .rotateTexture = 1,
    .scrollFrames = 1800,
    .textureOffset = 0,
};

void drawLine(LineEndPoint_t *pEndPoints, int numEndPoints, LineStatic_t *pStyle)
{
    if (numEndPoints < 2) return;
    if (!pStyle) pStyle = &DefaultLineStyle;

    // Initialize drawing state
    gfxDrawStripInit();
    gfxAddRegister(8, 0);
    gfxAddRegister(0x14, 0xff9000000260);
    gfxAddRegister(6, gfxGetEffectTex(pStyle->texture));
    gfxAddRegister(0x47, 0x513f1);
    gfxAddRegister(0x42, 0x8000000044);

    // Calculate animated texture offset
    float animatedUVOffset = pStyle->textureOffset;
    
    if (pStyle->scrollFrames != 0) {
        float scrollDirection = (pStyle->scrollFrames < 0) ? 1.0f : -1.0f;
        int absFrames = (pStyle->scrollFrames < 0) ? -pStyle->scrollFrames : pStyle->scrollFrames;
        
        animatedUVOffset += scrollDirection * ((float)(gameGetTime() % absFrames) / (float)absFrames);
    }

    // Count total segments needed
    int totalSegments = 0;
    int i;
    for (i = 0; i < numEndPoints - 1; i++) {
        VECTOR delta;
        vector_subtract(delta, pEndPoints[i + 1].pos, pEndPoints[i].pos);
        float len = vector_length(delta);

        int segs = pStyle->numSegments;
        if (segs <= 0) {
            segs = (int)(len / pStyle->textureRepeatDistance);
            if (segs < 1) segs = 1;
        }
        totalSegments += segs;
    }

    int numPoints = (totalSegments + 1) * 2;
    vec3 stripPos[numPoints];
    int stripColor[numPoints];
    UV_t stripUV[numPoints];

    float distanceAccum = 0.0f;
    int vertexIndex = 0;

    // Build geometry for each line segment
    for (i = 0; i < numEndPoints - 1; i++) {
        VECTOR a, b, delta, forward;
        vector_copy(a, pEndPoints[i].pos);
        vector_copy(b, pEndPoints[i + 1].pos);
        vector_subtract(delta, b, a);
        
        float segmentLength = vector_length(delta);
        vector_copy(forward, delta);
        vector_normalize(forward, forward);

        int numSegs = pStyle->numSegments;
        if (numSegs <= 0) {
            numSegs = (int)(segmentLength / pStyle->textureRepeatDistance);
            if (numSegs < 1) numSegs = 1;
        }

        // Generate vertices along this segment
        int s;
        for (s = 0; s <= numSegs; s++) {
            // Skip duplicate vertex at segment joins
            if (i > 0 && s == 0) continue;

            float t = (float)s / (float)numSegs;
            
            // Calculate position along segment
            VECTOR pos;
            pos[0] = a[0] + delta[0] * t;
            pos[1] = a[1] + delta[1] * t;
            pos[2] = a[2] + delta[2] * t;

            // Calculate perpendicular direction (right vector)
            VECTOR right;
            
            // Use miter joint at corner vertices
            if (i > 0 && s == 0) {
                // Get directions of previous and current segments
                VECTOR prevDir, currDir;
                vector_subtract(prevDir, pEndPoints[i].pos, pEndPoints[i - 1].pos);
                vector_subtract(currDir, pEndPoints[i + 1].pos, pEndPoints[i].pos);
                vector_normalize(prevDir, prevDir);
                vector_normalize(currDir, currDir);

                // Calculate right vectors for both segments
                VECTOR prevRight, currRight;
                
                if (pStyle->billboard) {
                    MATRIX *cameraMtx = &playerGetFromSlot(0)->camera->uMtx;
                    VECTOR camRight = {cameraMtx[0][0], cameraMtx[1][0], cameraMtx[2][0], 0};
                    
                    VECTOR temp;
                    vector_outerproduct(temp, prevDir, camRight);
                    vector_outerproduct(prevRight, temp, prevDir);
                    vector_normalize(prevRight, prevRight);
                    
                    vector_outerproduct(temp, currDir, camRight);
                    vector_outerproduct(currRight, temp, currDir);
                    vector_normalize(currRight, currRight);
                } else {
                    VECTOR up = {0, 1, 0, 0};
                    vector_outerproduct(prevRight, prevDir, up);
                    vector_normalize(prevRight, prevRight);
                    vector_outerproduct(currRight, currDir, up);
                    vector_normalize(currRight, currRight);
                }

                // Calculate miter vector (average of the two rights)
                VECTOR miter;
                miter[0] = prevRight[0] + currRight[0];
                miter[1] = prevRight[1] + currRight[1];
                miter[2] = prevRight[2] + currRight[2];
                miter[3] = 0;
                vector_normalize(miter, miter);

                // Scale miter to maintain consistent width
                float dot = vector_innerproduct(miter, currRight);
                if (dot < 0.25f) dot = 0.25f; // Clamp to prevent excessive scaling
                float miterScale = 1.0f / dot;
                
                vector_scale(right, miter, miterScale);
            } else {
                // Regular vertex - use standard perpendicular
                if (pStyle->billboard) {
                    MATRIX *cameraMtx = &playerGetFromSlot(0)->camera->uMtx;
                    right[0] = cameraMtx[0][0];
                    right[1] = cameraMtx[1][0];
                    right[2] = cameraMtx[2][0];
                    right[3] = 0;
                    
                    VECTOR temp;
                    vector_outerproduct(temp, forward, right);
                    vector_outerproduct(right, temp, forward);
                    vector_normalize(right, right);
                } else {
                    VECTOR up = {0, 1, 0, 0};
                    vector_outerproduct(right, forward, up);
                    vector_normalize(right, right);
                }
            }

            // Apply line width
            vector_scale(right, right, pStyle->lineWidth * 0.5f);

            // Create left and right vertices
            stripPos[vertexIndex][0] = pos[0] - right[0];
            stripPos[vertexIndex][1] = pos[1] - right[1];
            stripPos[vertexIndex][2] = pos[2] - right[2];

            stripPos[vertexIndex + 1][0] = pos[0] + right[0];
            stripPos[vertexIndex + 1][1] = pos[1] + right[1];
            stripPos[vertexIndex + 1][2] = pos[2] + right[2];

            // Calculate UVs
            float currentDistance = distanceAccum + segmentLength * t;
            float uvCoord;
            
            if (pStyle->numSegments > 0) {
                // Fixed segment count - use vertex index
                uvCoord = (float)(vertexIndex / 2);
            } else {
                // Auto-calculated - use actual distance
                uvCoord = currentDistance / pStyle->textureRepeatDistance;
            }

            // Apply UV coordinates with optional 90 degree rotation and animation
            if (pStyle->rotateTexture) {
                stripUV[vertexIndex].x = uvCoord + animatedUVOffset;
                stripUV[vertexIndex].y = 0.0f;
                stripUV[vertexIndex + 1].x = uvCoord + animatedUVOffset;
                stripUV[vertexIndex + 1].y = 1.0f;
            } else {
                stripUV[vertexIndex].x = 0.0f;
                stripUV[vertexIndex].y = uvCoord + animatedUVOffset;
                stripUV[vertexIndex + 1].x = 1.0f;
                stripUV[vertexIndex + 1].y = uvCoord + animatedUVOffset;
            }

            // Calculate color (with optional interpolation)
            int color;
            if (pEndPoints[i].lerpColor) {
                color = colorLerp(pEndPoints[i].color, pEndPoints[i + 1].color, t);
            } else {
                color = pEndPoints[i].color;
            }
            
            stripColor[vertexIndex] = color;
            stripColor[vertexIndex + 1] = color;

            vertexIndex += 2;
        }

        distanceAccum += segmentLength;
    }

    gfxDrawStrip(vertexIndex, stripPos, stripColor, stripUV, 1);
}

typedef struct LinesYo {
    int init;
    int numCubes;
    LineEndPoint_t *endpointsPtr;
    Cuboid *cubes[8];
} LinesYo_t;
LinesYo_t lines;

void getCuboidIndex(void)
{
    GameData *d = gameGetData();
    int i;
    for (i = 0; i < 8; ++i) {
        int cube = d->allYourBaseGameData->nodeResurrectionPts[i];
        lines.cubes[i] = spawnPointGet(cube);
        ++lines.numCubes;
    }
}

void doTheLines()
{
    if (!lines.init) {
        getCuboidIndex();
        lines.init = 1;
    }

	if (!lines.endpointsPtr) {
		lines.endpointsPtr = malloc(sizeof(LineEndPoint_t) * 32);
	}

    int i;
    // lines.numCubes = 2;
    for(i = 0; i < lines.numCubes; ++i) {
        vector_copy(lines.endpointsPtr[i].pos, lines.cubes[i]->pos);
        lines.endpointsPtr[i].color = 0x80000000 | TEAM_COLORS[i];
        lines.endpointsPtr[i].lerpColor = true;
    }
    vector_copy(lines.endpointsPtr[1].pos, playerGetFromSlot(0)->pMoby->position);
    // LineEndPoint_t endpoints[2];
    // vector_copy(endpoints[0].pos, playerGetFromSlot(0)->pMoby->position);
    // vector_copy(endpoints[1].pos, rawr.pos);
    // endpoints[0].color = 0x800000FF;
    // endpoints[0].lerpColor = true;
    // endpoints[1].color = 0x80FFFFFF;
    // endpoints[1].lerpColor = true;

    drawLine(lines.endpointsPtr, lines.numCubes, NULL); 
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
        drawLine_Original(corner[a], corner[b]);
    }
}

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




/* Prepare arrays for strip vertices */
vec3 positions[2][(MAX_SEGMENTS + 1) * 2];
int colors[2][(MAX_SEGMENTS + 1) * 2];
UV_t uvs[2][(MAX_SEGMENTS + 1) * 2];
int cachedSegments = -1;
int cachedIsCircle = -1;

float scrollQuad = 0;

void circleMeFinal_StripMe(Moby *this, Cuboid cube)
{
    hillPvar_t *pvar = (hillPvar_t*)this->pVar;
    int isCircle = pvar->isCircle;
    u32 baseColor = pvar->color;
    int i, edge, s;
    
    /* Setup rotation matrix from cube */
    MATRIX rotMatrix;
    matrix_unit(rotMatrix);
    matrix_rotate_x(rotMatrix, rotMatrix, cube.rot[0]);
    matrix_rotate_y(rotMatrix, rotMatrix, cube.rot[1]);
    matrix_rotate_z(rotMatrix, rotMatrix, cube.rot[2]);
    
    /* Extract axes from cube matrix */
    VECTOR center, xAxis, zAxis, yAxis, halfX, halfZ;
    vector_copy(center, cube.pos);
    vector_copy(xAxis, (VECTOR){cube.matrix.v0[0], cube.matrix.v1[0], cube.matrix.v2[0], 0});
    vector_copy(zAxis, (VECTOR){cube.matrix.v0[1], cube.matrix.v1[1], cube.matrix.v2[1], 0});
    vector_copy(yAxis, (VECTOR){cube.matrix.v0[2], cube.matrix.v1[2], cube.matrix.v2[2], 0});
    
    /* Apply rotation to axes */
    vector_apply(xAxis, xAxis, rotMatrix);
    vector_apply(zAxis, zAxis, rotMatrix);
    vector_apply(yAxis, yAxis, rotMatrix);
    
    vector_scale(halfX, xAxis, 0.5f);
    vector_scale(halfZ, zAxis, 0.5f);
    vector_normalize(yAxis, yAxis);
    
    float fRadius = vector_length(halfX);
    
    /* ===== Calculate segment count ===== */
    int segmentSize = 2;
    int segments;
    
    if (isCircle) {
        segments = clamp(fRadius * 2 / segmentSize, MIN_SEGMENTS, MAX_SEGMENTS);
    } else {
        /* For square, calculate total segments around perimeter */
        float sideLenX = vector_length(xAxis);
        float sideLenZ = vector_length(zAxis);
        float perimeter = (sideLenX + sideLenZ) * 2.0f;
        segments = clamp((int)(perimeter / segmentSize), MIN_SEGMENTS, MAX_SEGMENTS);
    }
    
    /* ===== Setup rendering ===== */
    int floorTex = isCircle ? FX_CIRLCE_NO_FADED_EDGE : FX_SQUARE_FLAT_1;
    int ringTex = FX_TIRE_TRACKS;
    
    /* UV trimming to remove transparent edges */
    float uMin = TEXTURE_EDGE_TRIM_U;
    float uMax = 1.0f - TEXTURE_EDGE_TRIM_U;
    float vMin = TEXTURE_EDGE_TRIM_V;
    float vMax = 1.0f - TEXTURE_EDGE_TRIM_V;
    float uRange = uMax - uMin;
    float vRange = vMax - vMin;
    
    /* Initialize strip drawing */
    gfxDrawStripInit();
    gfxAddRegister(8, 0);
    gfxAddRegister(0x14, 0xff9000000260);
    gfxAddRegister(6, gfxGetEffectTex(ringTex));
    gfxAddRegister(0x47, 0x513f1);
    gfxAddRegister(0x42, 0x8000000044);

    /* === Setup shape-specific parameters === */
    VECTOR corners[4], vRadius;
    int segmentsPerEdge[4];
    float thetaStep;
    
    if (isCircle) {
        thetaStep = (2.0f * MATH_PI) / segments;
    } else {
        /* Calculate corners */
        float hx0 = halfX[0], hx1 = halfX[1], hx2 = halfX[2];
        float hz0 = halfZ[0], hz1 = halfZ[1], hz2 = halfZ[2];
        
        vector_copy(corners[0], (VECTOR){center[0]-hx0-hz0, center[1]-hx1-hz1, center[2]-hx2-hz2, 0});
        vector_copy(corners[1], (VECTOR){center[0]+hx0-hz0, center[1]+hx1-hz1, center[2]+hx2-hz2, 0});
        vector_copy(corners[2], (VECTOR){center[0]+hx0+hz0, center[1]+hx1+hz1, center[2]+hx2+hz2, 0});
        vector_copy(corners[3], (VECTOR){center[0]-hx0+hz0, center[1]-hx1+hz1, center[2]-hx2+hz2, 0});
        
        /* Calculate segments per edge proportionally */
        float sideLenX = vector_length(xAxis);
        float sideLenZ = vector_length(zAxis);
        float perimeter = (sideLenX + sideLenZ) * 2.0f;
        
        segmentsPerEdge[0] = segmentsPerEdge[2] = (int)((sideLenX / perimeter) * segments);
        segmentsPerEdge[1] = segmentsPerEdge[3] = (int)((sideLenZ / perimeter) * segments);
        
        /* Ensure we use all segments */
        segmentsPerEdge[0] += segments - (segmentsPerEdge[0] + segmentsPerEdge[1] + segmentsPerEdge[2] + segmentsPerEdge[3]);
    }
    
    int numSegments = (segments + 1) * 2;
    VECTOR tempRight, tempUp;
    
    /* === Generate positions once (only if not cached) === */
    if (cachedSegments != segments || cachedIsCircle != isCircle) {
        if (isCircle) vector_copy(vRadius, halfX);
        
        for (i = 0; i <= segments; i++) {
            VECTOR pos;
            
            if (isCircle) {
                /* Calculate position on circle */
                vector_add(pos, center, vRadius);
                
                /* Calculate tangent for quad orientation */
                vector_outerproduct(tempRight, yAxis, vRadius);
                vector_normalize(tempRight, tempRight);
                vector_copy(tempUp, yAxis);
                
                /* Rotate radius for next segment */
                vector_rodrigues(vRadius, vRadius, yAxis, thetaStep);
            } else {
                /* Determine which edge we're on */
                int accumulatedSegs = 0;
                int currentEdge = 0;
                int localS = i;
                
                for (edge = 0; edge < 4; edge++) {
                    if (i <= accumulatedSegs + segmentsPerEdge[edge]) {
                        currentEdge = edge;
                        localS = i - accumulatedSegs;
                        break;
                    }
                    accumulatedSegs += segmentsPerEdge[edge];
                }
                
                VECTOR p0, p1, edgeDir;
                vector_copy(p0, corners[currentEdge]);
                vector_copy(p1, corners[(currentEdge + 1) & 3]);
                
                vector_subtract(edgeDir, p1, p0);
                vector_normalize(edgeDir, edgeDir);
                
                float t = (float)localS / segmentsPerEdge[currentEdge];
                vector_lerp(pos, p0, p1, t);
                
                /* For square, vertices are just offset in Z direction */
                vector_copy(tempRight, (VECTOR){0, 0, 0, 0});
                vector_copy(tempUp, (VECTOR){0, 0, 0, 0});
            }
            
            int idx = i * 2;
            
            /* === Upper ring positions === */
            positions[0][idx][0] = pos[0] + tempRight[0] - tempUp[0];
            positions[0][idx][1] = pos[1] + tempRight[1] - tempUp[1];
            positions[0][idx][2] = pos[2] + tempRight[2] - tempUp[2] - 1;
            
            positions[0][idx + 1][0] = pos[0] + tempRight[0] + tempUp[0];
            positions[0][idx + 1][1] = pos[1] + tempRight[1] + tempUp[1];
            positions[0][idx + 1][2] = pos[2] + tempRight[2] + tempUp[2] + 1;
            
            /* === Lower ring positions === */
            VECTOR offsetPos;
            vector_add(offsetPos, pos, (VECTOR){0, 0, 2, 0});
            
            positions[1][idx][0] = offsetPos[0] + tempRight[0] + tempUp[0];
            positions[1][idx][1] = offsetPos[1] + tempRight[1] + tempUp[1];
            positions[1][idx][2] = offsetPos[2] + tempRight[2] + tempUp[2] + 1;

            positions[1][idx + 1][0] = offsetPos[0] + tempRight[0] - tempUp[0];
            positions[1][idx + 1][1] = offsetPos[1] + tempRight[1] - tempUp[1];
            positions[1][idx + 1][2] = offsetPos[2] + tempRight[2] - tempUp[2] - 1;
        }
        
        cachedSegments = segments;
        cachedIsCircle = isCircle;
    }
    
    /* === Calculate distance-based opacity === */
    Player *player = playerGetFromSlot(0);
    VECTOR delta;
    vector_subtract(delta, center, player->playerPosition);
    float distance = vector_length(delta);
    
    float opacityFactor = 1.0f;
    // if (distance > 52.0f) {
    //     opacityFactor = 1.0f - clamp((distance - 52.0f) / 12.0f, 0.0f, 1.0f);
    // }
    
    /* === Update colors and UVs every frame === */
    float trimmedU = uMin + (scrollQuad - floorf(scrollQuad)) * uRange;
    
    for (i = 0; i <= segments; i++) {
        float trimmedV = vMin + (((float)i / segmentSize) - floorf((float)i / segmentSize)) * vRange;
        int idx = i * 2;

        /*
         === Order of colors:
         - top of upper strip
         - bottom of upper strip
         - top of lower strip
         - bottom oflower strip
        */
        colors[1][idx] = ((0x10 * (int)opacityFactor) << 24) | baseColor;
        colors[1][idx + 1] = ((0x30 * (int)opacityFactor) << 24) | baseColor;
        colors[0][idx + 1] = ((0x50 * (int)opacityFactor) << 24) | baseColor;
        colors[0][idx] = ((0x10 * (int)opacityFactor) << 24) | baseColor;

        uvs[0][idx].x = uvs[1][idx].x = trimmedU;
        uvs[0][idx].y = uvs[0][idx + 1].y = uvs[1][idx].y = uvs[1][idx + 1].y = trimmedV;
        uvs[0][idx + 1].x = trimmedU - 0.3f;
        uvs[1][idx + 1].x = trimmedU + 0.3f;
    }
    
    /* === Draw both rings === */
    gfxDrawStrip(numSegments, positions[0], colors[0], uvs[0], 1);
    gfxDrawStrip(numSegments, positions[1], colors[1], uvs[1], 1);
    
    /* Animate texture scroll */
    scrollQuad += TEXTURE_SCROLL_SPEED;
    if (scrollQuad >= 1.0f) scrollQuad -= 1.0f;
    
    /* ===== Draw floor quad ===== */
    QuadDef floorQuad;
    gfxSetupEffectTex(&floorQuad, floorTex, DRAW_TYPE_NORMAL, 0x30);
    
    /* Offset floor down slightly */
    VECTOR offset = {0, 0, -1, 0};
    VECTOR rotatedOffset;
    vector_apply(rotatedOffset, offset, rotMatrix);
    
    /* Create floor corners */
    VECTOR floorCorners[4];
    if (isCircle) {
        vector_copy(floorCorners[0], (VECTOR){center[0] - fRadius, center[1] - fRadius, center[2], 0});
        vector_copy(floorCorners[1], (VECTOR){center[0] + fRadius, center[1] - fRadius, center[2], 0});
        vector_copy(floorCorners[2], (VECTOR){center[0] + fRadius, center[1] + fRadius, center[2], 0});
        vector_copy(floorCorners[3], (VECTOR){center[0] - fRadius, center[1] + fRadius, center[2], 0});
    } else {
        vector_copy(floorCorners[0], (VECTOR){center[0]-halfX[0]-halfZ[0], center[1]-halfX[1]-halfZ[1], center[2]-halfX[2]-halfZ[2], 0});
        vector_copy(floorCorners[1], (VECTOR){center[0]+halfX[0]-halfZ[0], center[1]+halfX[1]-halfZ[1], center[2]+halfX[2]-halfZ[2], 0});
        vector_copy(floorCorners[2], (VECTOR){center[0]+halfX[0]+halfZ[0], center[1]+halfX[1]+halfZ[1], center[2]+halfX[2]+halfZ[2], 0});
        vector_copy(floorCorners[3], (VECTOR){center[0]-halfX[0]+halfZ[0], center[1]-halfX[1]+halfZ[1], center[2]-halfX[2]+halfZ[2], 0});
    }
    
    /* Apply offset to all corners */
    for (i = 0; i < 4; i++) {
        vector_add(floorCorners[i], floorCorners[i], rotatedOffset);
    }
    
    /* Setup floor quad vertices */
    u32 floorColor = (0x30 << 24) | baseColor;
    
    vector_copy(floorQuad.point[0], floorCorners[1]);
    vector_copy(floorQuad.point[1], floorCorners[0]);
    vector_copy(floorQuad.point[2], floorCorners[2]);
    vector_copy(floorQuad.point[3], floorCorners[3]);
    
    floorQuad.point[0][3] = 1.0f;
    floorQuad.point[1][3] = 1.0f;
    floorQuad.point[2][3] = 1.0f;
    floorQuad.point[3][3] = 1.0f;
    
    floorQuad.rgba[0] = floorColor;
    floorQuad.rgba[1] = floorColor;
    floorQuad.rgba[2] = floorColor;
    floorQuad.rgba[3] = floorColor;
    
    floorQuad.uv[0] = (UV_t){0, 0};
    floorQuad.uv[1] = (UV_t){0, 1};
    floorQuad.uv[2] = (UV_t){1, 0};
    floorQuad.uv[3] = (UV_t){1, 1};
    
    gfxDrawQuad(floorQuad, NULL);
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


int debugToPlayer = 0;
void drawTheThingJulie(Moby *moby)
{
    hillPvar_t *pvar = (hillPvar_t*)moby->pVar;
	VECTOR worldCorners[8];
	int i;
    // for debug, make player be center of cuboid.
    // if (!debugToPlayer) {
    //     vector_copy(tempMatrix.v3, playerGetFromSlot(0)->pMoby->position);
    //     debugToPlayer = 1;
    // }

	for (i = 0; i < 8; i++) {
        vector_transform(worldCorners[i], corners[i], (MATRIX*)&rawr.matrix);
		worldCorners[i][2] += rawr.matrix.v2[2] * .5;
    }

    // circleMeFinal_StripMe(moby, rawr); // rodrigues rotation (DO NOT EDIT, WORKS)
    // edgeMe(worldCorners);
    // drawCylinder(rawr.pos, 20, 6, 0x60ffffff, FX_RETICLE_1);
    doTheLines();

}

void postDraw(Moby *moby)
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

    drawTheThingJulie(moby);
}

void update(Moby* moby)
{
    VECTOR t;
    int i;
    hillPvar_t *pvars = (hillPvar_t*)moby->pVar;
    if (!pvars)
        return;

    gfxRegisterDrawFunction(&postDraw, moby);

    // handle players
    // hillPlayerUpdates(moby);
}

void spawn(VECTOR position)
{
    Moby *moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(hillPvar_t));
    if (!moby) return;

    hillPvar_t* pvars = (hillPvar_t*)moby->pVar;

    moby->pUpdate = &update;
    moby->modeBits = 0;
    moby->updateDist = -1;

    vector_copy(moby->position, rawr.pos);
    pvars->currentCuboid = &rawr;


    // update pvars
    // pvars->DestroyAtTime = gameGetTime() + (TIME_SECOND * 30);
    // memset(pvars, 0, sizeof(pvars));

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