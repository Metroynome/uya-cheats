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

#define TestMoby	(*(Moby**)0x00091004)
VECTOR position;
VECTOR rotation;

int SPRITE_ME = 0;
int EFFECT_ME = 1;
int SOUND_ME = 0;
int SOUND_ME_FLAG = 3;
int first = 1;

void patchCTFFlag(void);
void hudInit(void);

extern VariableAddress_t vaGiveWeaponFunc;
extern VariableAddress_t vaPlayerRespawnFunc;
extern VariableAddress_t vaSpawnPointsPtr;

void DebugInGame(Player* player)
{
    if (playerPadGetButtonDown(player, PAD_LEFT) > 0) {
		// Swap Teams
		int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
		// playerSetTeam(player, SetTeam);
		playerObfuscate(&player->hitPoints, 15, 0);
	} else if (playerPadGetButtonDown(player, PAD_RIGHT) > 0) {
        // Hurt Player
        // playerDecHealth(player, 1);
		playerObfuscate(&player->hitPoints, 7, 0);
	} else if (playerPadGetButtonDown(player, PAD_UP) > 0) {
		// static int Occlusion = (Occlusion == 2) ? 0 : 2;
		// gfxOcclusion(Occlusion);
		// spawnPointGetRandom(player, &position, &rotation);
		// Player ** ps = playerGetAll();
		// Player * p = ps[1];
		// playerSetPosRot(player, &p->playerPosition, &p->playerRotation);
	} else if(playerPadGetButtonDown(player, PAD_DOWN) > 0) {
		// Set Gattling Turret Health to 1.
		DEBUGsetGattlingTurretHealth();
	} else if (playerPadGetButtonDown(player, PAD_L3) > 0) {
		// Nothing Yet!
		int id = 0x6b;
		void * cuboid = (void*)(*(u32*)GetAddress(&vaSpawnPointsPtr) + id * 0x80 + 0x30);
		((void (*)(u8, void*))0x0043bd98)(player->unk_24c9, cuboid);
	} else if (playerPadGetButtonDown(player, PAD_R3) > 0) {
		// printf("\npZ: %f", player->cheatZ);
		// printf("\npY: %f", player->cheatY);
		// Moby* hbMoby = mobySpawn(MOBY_ID_HEALTH_BOX_MP, 0);
		// Moby* orbMoby = mobySpawn(MOBY_ID_HEALTH_ORB_MP, 0);
		// if (hbMoby) {
		// 	vector_copy(hbMoby->position, player->playerPosition);

		// 	hbMoby->updateDist = 0xFF;
		// 	hbMoby->drawn = 0x01;
		// 	hbMoby->drawDist = 0x0080;
		// 	hbMoby->opacity = 0x80;
		// 	hbMoby->state = 1;
		// }
		// if (orbMoby) {
		// 	vector_copy(orbMoby->position, hbMoby->position);
		// 	orbMoby->updateDist = 0xFF;
		// 	orbMoby->drawn = 0x01;
		// 	orbMoby->drawDist = 0x0080;
		// 	orbMoby->opacity = 0x80;
		// 	orbMoby->state = 1;	
		// }
	}
}

void DebugInMenus(void)
{
    if (padGetButtonDown(0, PAD_LEFT) > 0) {
		// Nothing Yet!
	} else if (padGetButtonDown(0, PAD_RIGHT) > 0) {
		// Nothing Yet!
	} else if (padGetButtonDown(0, PAD_UP) > 0) {
		// Nothing Yet!
	} else if(padGetButtonDown(0, PAD_DOWN) > 0) {
		// Nothing Yet!
	} else if (padGetButtonDown(0, PAD_L3) > 0) {
		// Nothing Yet!
	} else if (padGetButtonDown(0, PAD_R3) > 0) {
		// Nothing Yet!
	}
}

void InfiniteChargeboot(void)
{
	Player *player = playerGetFromSlot(0);
	if (!player)
		return;

	if (player->timers.noFpsCamTimer == 1 && playerPadGetButton(player, PAD_R2) > 0 && player->timers.state > 55)
		player->timers.state = 55;
}

void InfiniteHealthMoonjump(void)
{
    static int _InfiniteHealthMoonjump_Init = 0;
    int Joker = *(u16*)0x00225982;
    if (Joker == 0xFDFB)
        _InfiniteHealthMoonjump_Init = 1;
    else if (Joker == 0xFFFD)
        _InfiniteHealthMoonjump_Init = 0;

    if (_InfiniteHealthMoonjump_Init)
    {
        Player * player = playerGetFromSlot(0);
		if (playerGetHealth(player) < 15)
        	playerSetHealth(player, 15);

        if (Joker == 0xBFFF)
            (float)player->move.behavior[2] = 0.125;
    }
}

void DEBUGsetGattlingTurretHealth(void)
{
    Moby* moby = mobyListGetStart();
    // Iterate through mobys and change health
    while ((moby = mobyFindNextByOClass(moby, MOBY_ID_GATLING_TURRET))) {
        if (moby->pVar) {
			*(float*)((u32)moby->pVar + 0x30) = 0;
        }
        ++moby;
    }
}


void Test_Sprites(float x, float y, float scale)
{
	float small = scale * 0.75;
	float delta = (scale - small) / 2;

	gfxSetupGifPaging(0);
	u64 dreadzoneSprite = gfxGetFrameTex(SPRITE_ME);
	// gfxDrawSprite(x+2, y+2, scale, scale, 0, 0, 32, 32, 0x40000000, dreadzoneSprite);
	gfxDrawSprite(x,   y,   scale, scale, 0, 0, 32, 32, 0x80C0C0C0, dreadzoneSprite);
	gfxDrawSprite(x+delta, y+delta, small, small, 0, 0, 32, 32, 0x80000040, dreadzoneSprite);
	gfxDoGifPaging();
}

void drawEffectQuad(VECTOR position, int texId, float scale)
{
	QuadDef quad;
	MATRIX m2;
	VECTOR t;
	// points: [x, z, y, r]?
	VECTOR pTL = {1, 0, 1, 1};
	VECTOR pTR = {-1, 0, 1, 1};
	VECTOR pBL = {1, 0, -1, 1};
	VECTOR pBR = {-1 ,0 , -1, 1};

	// determine color
	u32 color = 0x80FFFFFF;

	// set draw args
	matrix_unit(m2);

	// get texture
	// QuadDef texture;
	// ((void(*)(int, int, int, int))0x0045a220)(&quad, texId, 0, 0x80);

	// color of each corner?
	vector_copy(quad.xzyw[0], pTL);
	vector_copy(quad.xzyw[1], pTR);
	vector_copy(quad.xzyw[2], pBL);
	vector_copy(quad.xzyw[3], pBR);
	quad.rgba[0] = quad.rgba[1] = quad.rgba[2] = quad.rgba[3] = color;
	quad.uv[0] = (struct UV){0, 0};
	quad.uv[1] = (struct UV){0, 1};
	quad.uv[2] = (struct UV){1, 0};
	quad.uv[3] = (struct UV){1, 1};
	quad.clamp = 0;
	quad.tex0 = gfxGetFrameTex(texId);
	quad.tex1 = 0xff9000000260;
	quad.alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;

	// get forward vector
	vector_subtract(t, camera->pos, position);
	t[2] = 0;
	vector_normalize(&m2[4], t);

	// up vector
	m2[8 + 2] = 1;

	// right vector
	vector_outerproduct(&m2[0], &m2[4], &m2[8]);

	t[0] = t[1] = t[2] = scale;
	t[3] = 1;
	matrix_scale(m2, m2, t);

	// position
	memcpy(&m2[12], position, sizeof(VECTOR));

	// draw
	gfxSetupGifPaging(0);
	gfxDrawQuad(&quad, m2);
	gfxDoGifPaging();
}

int ping(void)
{
	static int ping_old = 0;
	static int myPing = 0;
	int ping_new = *(int*)0x001d3b5c;
	if (ping_old != ping_new) {
		myPing = ping_new - ping_old;
		ping_old = ping_new;
	}
	return myPing;
}

VECTOR b6ExplosionPositions[64];
int b6ExplosionPositionCount = 0;
void drawB6Visualizer(void)
{
	int i;

	for (i = 0; i < b6ExplosionPositionCount; ++i) {
		drawEffectQuad(b6ExplosionPositions[i], 4, 1);
	}
}

void renderB6Visualizer(Moby* m)
{
	gfxStickyFX(&drawB6Visualizer, m);
}
void runB6HitVisualizer(void)
{
	VECTOR off = {0,0,2,0};
	if (!TestMoby) {
		Moby *m = TestMoby = mobySpawn(MOBY_ID_TEST, 0);
		m->scale *= 0.1;
		m->pUpdate = &renderB6Visualizer;
		vector_add(m->position, playerGetFromSlot(0)->playerPosition, off);
	}

  // 
  TestMoby->drawDist = 0xFF;
  TestMoby->updateDist = 0xFF;

	// check for b6
	Moby* b = mobyListGetStart();
	Moby* mEnd = mobyListGetEnd();
	while (b < mEnd) {
		if (!mobyIsDestroyed(b) && b->oClass == MOBY_ID_SHOT_GRAVITY_BOMB1 && b->state == 2) {
			DPRINTF("%08X\n", (u32)b->pUpdate);
			vector_copy(b6ExplosionPositions[b6ExplosionPositionCount], b->position);
			b6ExplosionPositionCount = (b6ExplosionPositionCount + 1) % 64;
		}
		++b;
	}
}

void drawCollider(Moby* moby)
{
	VECTOR pos,t,o = {0,0,0.7,0};
	int i,j = 0;
	int x,y;
	char buf[12];
	Player * p = playerGetFromSlot(0);

	const int steps = 10 * 2;
	const float radius = 2;

	for (i = 0; i < steps; ++i) {
		for (j = 0; j < steps; ++j) {
			float theta = (i / (float)steps) * MATH_TAU;
			float omega = (j / (float)steps) * MATH_TAU;

			vector_copy(pos, p->playerPosition);
			pos[0] += radius * sinf(theta) * cosf(omega);
			pos[1] += radius * sinf(theta) * sinf(omega);
			pos[2] += radius * cosf(theta);

			vector_add(t, p->playerPosition, o);

			if (CollLine_Fix(pos, t, 1, NULL, 0)) {
				vector_copy(t, CollLine_Fix_GetHitPosition());
				drawEffectQuad(t, EFFECT_ME, 0.1);
			}
		}
	}
}

void drawSomething(Moby* moby)
{
	VECTOR pos,t,o = {0,0,0.7,0};
	int i,j = 0;
	int x,y;
	char buf[12];
	Player * p = playerGetFromSlot(0);

	const int steps = 10 * 2;
	const float radius = 2;

	vector_copy(pos, p->playerPosition);
	for (i = 0; i < steps; ++i) {
		for (j = 0; j < steps; ++j) {
			float theta = (i / (float)steps) * MATH_TAU;
			float omega = (j / (float)steps) * MATH_TAU;

			vector_copy(pos, p->playerPosition);
			pos[0] += radius * sinf(theta) * cosf(omega);
			pos[1] += radius * sinf(theta) * sinf(omega);
			pos[2] += radius * cosf(theta);

			vector_add(t, p->playerPosition, o);

			if (CollLine_Fix(pos, t, 1, NULL, 0)) {
				vector_copy(t, CollLine_Fix_GetHitPosition());
				// drawEffectQuad(t, EFFECT_ME, 0.1);
			}
		}
	}
	drawEffectQuad(pos, EFFECT_ME, 2);
}

void patchAFK(void)
{
	int isAFK = 0;
	// netInstallCustomMsgHandler(CUSTOM_MSG_ID_PLAYER_AFK, &onClientAFKRemote);
	int AFK_Wait_Timer = 10; // in Minutes
	int gameTime = gameGetTime();
	static int afk_time = 0;
	if (afk_time == 0)
		afk_time = gameTime + (AFK_Wait_Timer * TIME_MINUTE);

	PAD * src = (PAD*)((u32)P1_PAD - 0x80);
	// if no buttons/analogs are pressed
	if (src->handsOff && src->handsOffStick) {
		// if is already AFK, return.
		if (isAFK)
			return;

		// if time left to go afk is greater than the game time, then not afk yet, return.
		if (afk_time > gameTime)
			return;

		void* connection = netGetLobbyServerConnection();
		if (!connection)
			return;

		// netSendCustomAppMessage(netGetLobbyServerConnection(), NET_LOBBY_CLIENT_INDEX, CUSTOM_MSG_ID_CLIENT_AFK, 0, NULL);
		printf("\ni'm afk!");
		isAFK = 1;
	} else {
		afk_time = gameTime + (TIME_SECOND * AFK_Wait_Timer);
		isAFK = 0;
	}
}


struct HealthBoxIndexAndPosition {
	int index;
	float x;
	float y;
	float z;
};
typedef struct CompactFlagReplacement {
	u8 mapId;
	struct HealthBoxIndexAndPosition move[4];
} CompactFlagReplacement_t;

typedef struct HealthboxReplacement {
	short mapId;
	short index;
	float x;
	float z;
	float y;
} HealthboxReplacementt_t;

HealthboxReplacementt_t betterHealthBoxesRules[] = {
	{
		.mapId = MAP_ID_BAKISI,
		.index = 0,
		.x = 0,
		.z = 0,
		.y = 0
	},
	{MAP_ID_BAKISI, 1, 1, 1, 1},
	{MAP_ID_BAKISI, -1, 449.23907, 375.7771, 207.75024},
};

static int _betterHealthBoxes = 0;
void betterHealthBoxes_Move(void)
{
	if (_betterHealthBoxes)
		return;

	int j;
	int mapId = gameGetSettings()->GameLevel;
	Moby* mobyStart = mobyListGetStart();
	Moby* mobyEnd = mobyListGetEnd();
	while (mobyStart < mobyEnd) {
		if (mobyStart->oClass == MOBY_ID_HEALTH_BOX_MP) {
			for (j = 0; j < COUNT_OF(betterHealthBoxesRules); ++j) {
				if (mapId == betterHealthBoxesRules[j].mapId) {
					int index = betterHealthBoxesRules[j].index;
					Moby * moby = mobyStart + index;
					if (index > -1) {
						moby->position[0] = betterHealthBoxesRules[j].x;
						moby->position[1] = betterHealthBoxesRules[j].z;
						moby->position[2] = betterHealthBoxesRules[j].y;
					}
					// else if (index == -1) {
					// 	Moby* hbMoby = mobySpawn(MOBY_ID_HEALTH_BOX_MP, 0);
					// 	if (hbMoby) {
					// 		hbMoby->position[0] = betterHealthBoxesRules[j].x;
					// 		hbMoby->position[1] = betterHealthBoxesRules[j].z;
					// 		hbMoby->position[2] = betterHealthBoxesRules[j].y;
					// 		hbMoby->pUpdate = moby->pUpdate;
					// 		hbMoby->updateDist = 0xFF;
					// 		hbMoby->drawn = 0x01;
					// 		hbMoby->drawDist = 0x0080;
					// 		hbMoby->opacity = 0x80;
					// 		hbMoby->state = 1;
					// 	}
					// }
				}
			}
			break;
		}
		++mobyStart;
	}
	_betterHealthBoxes = 1;
}

// const int betterHealthBoxesRulesCount = COUNT_OF(betterHealthBoxesRules);
// static int _betterHealthBoxes = 0;
// void betterHealthBoxes_Move(void)
// {
// 	if (_betterHealthBoxes)
// 		return;

// 	int i, j = 0;
// 	int mapId = gameGetSettings()->GameLevel;
// 	struct CompactHealthBoxReplacement* rule = NULL;
// 	for (i = 0; i < betterHealthBoxesRulesCount; ++i) {
// 		if (betterHealthBoxesRules[i].mapId == mapId) {
// 			rule = &betterHealthBoxesRules[i];
// 			break;
// 		}
// 	}

// 	if (rule) {
// 		Moby* mobyStart = mobyListGetStart();
// 		Moby* mobyEnd = mobyListGetEnd();
// 		while (mobyStart < mobyEnd) {
// 			if (mobyStart->oClass == MOBY_ID_HEALTH_BOX_MP) {
// 				for (j = 0; j < COUNT_OF(rule->move); ++j) {
// 					int index = rule->move[j].index;
// 					if (index > -1) {
// 						Moby * moby = mobyStart + index;
// 						printf("POINT %d: 0x%08x\n", index, moby);
// 						moby->position[0] = (float)rule->move[j].x;
// 						printf("P0: %f\n", (float)moby->position[0]);
// 						moby->position[1] = (float)rule->move[j].z;
// 						printf("P1: %f\n", (float)moby->position[1]);
// 						moby->position[2] = (float)rule->move[j].y;
// 						printf("P2: %f\n", (float)moby->position[2]);			
// 					}
// 				}
// 				break;
// 			}
// 			++mobyStart;
// 		}
// 	}
// 	_betterHealthBoxes = 1;
// }

void testSpritesOrEffects(int SpriteOrEffect, int tex,float x, float y, float scale)
{
	float small = scale * 0.75;
	float delta = (scale - small) / 2;
	u64 sprite = gfxGetFrameTex(tex);
	u64 effect = gfxGetEffectTex(tex);

	u64 texture = (SpriteOrEffect) ? effect : sprite;

	gfxSetupGifPaging(0);
	// gfxDrawSprite(x+2, y+2, scale, scale, 0, 0, 32, 32, 0x40000000, dreadzoneSprite);
	gfxDrawSprite(x, y, scale, scale, 0, 0, 32, 32, 0x80c0c0c0, texture);
	gfxDrawSprite(x+delta, y+delta, scale, scale, 0, 0, 32, 32, 0x80ffffff, texture);
	gfxDoGifPaging();
}

int debugTextures(void)
{
	static int SpriteOrEffect = 0;
	static u64 texture;
	Player *player = playerGetFromSlot(0);
	if (playerPadGetButtonDown(player, PAD_L1 | PAD_UP) > 0)
		SpriteOrEffect = !SpriteOrEffect;
	if (playerPadGetButtonDown(player, PAD_L1 + PAD_RIGHT) > 0)
		texture += 1;
	if (playerPadGetButtonDown(player, PAD_L1 + PAD_LEFT) > 0)
		texture -= 1;

	testSpritesOrEffects(SpriteOrEffect, texture, SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);
}

#define HBOLT_MOBY_OCLASS (MOBY_ID_TEST)
#define HBOLT_PICKUP_RADIUS	 (3)
#define HBOLT_PARTICLE_COLOR (0x0000ffff)
#define HBOLT_SPRITE_COLOR (0x00ffffff)
#define HBOLT_SCALE (1)

struct HBoltPVar {
	int DestroyAtTime;
	struct PartInstance* Particles[4];
};

// void mobyPostDraw(Moby* moby)
// {
// 	struct QuadDef quad;
// 	MATRIX m2;
// 	VECTOR t;
// 	int opacity = 0x80;
// 	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->pVar;
// 	struct PartInstance * particle = pvars->Particles[0];
// 	if (!pvars)
// 		return;

// 	// determine color
// 	u32 color = HBOLT_SPRITE_COLOR;
// 	u32 partColor = 0x80ff00ff;
// 	float pulse = (1 + sinf(clampAngle((gameGetTime() / 1000.0) * 1.0))) * 0.5;
// 	opacity = 0x20 + (pulse * 0x50);
// 	opacity = opacity << 24;
// 	color = opacity | (color & HBOLT_SPRITE_COLOR);
// 	moby->primaryColor = color;

// 	VECTOR pTL = {1, 0, 1, 1};
// 	VECTOR pTR = {-1, 0, 1, 1};
// 	VECTOR pBL = {1, 0, -1, 1};
// 	VECTOR pBR = {-1 ,0 , -1, 1};
// 	matrix_unit(m2);
// 	vector_copy(quad.xzyw[0], pTL);
// 	vector_copy(quad.xzyw[1], pTR);
// 	vector_copy(quad.xzyw[2], pBL);
// 	vector_copy(quad.xzyw[3], pBR);
// 	quad.rgba[0] = quad.rgba[1] = quad.rgba[2] = quad.rgba[3] = color;
// 	quad.uv[0] = (struct UV){0, 0};
// 	quad.uv[1] = (struct UV){1, 0};
// 	quad.uv[2] = (struct UV){0, 1};
// 	quad.uv[3] = (struct UV){1, 1};
// 	quad.clamp = 0;
// 	quad.tex0 = 0;
// 	quad.tex1 = 0;
// 	quad.alpha = 0;
// 	VECTOR pos;
// 	vector_copy(pos, moby->position);
// 	pos[1] += .5;
// 	memcpy(&m2[12], &pos, sizeof(VECTOR));
// 	static int texture = 53;
// 	Player *player = playerGetFromSlot(0);
// 	if (playerPadGetButtonDown(player, PAD_L1 + PAD_RIGHT) > 0)
// 		texture += 1;
// 	if (playerPadGetButtonDown(player, PAD_L1 + PAD_LEFT) > 0)
// 		texture -= 1;
// 	if (playerPadGetButtonDown(player, PAD_L1 + PAD_RIGHT) > 0 || playerPadGetButtonDown(player, PAD_L1 + PAD_LEFT) > 0)
// 		printf("\n========\n tex: %d", texture);

// 	// u32 hook = (u32)GetAddress(&vaReplace_GetEffectTexJAL) + 0x20;
// 	// HOOK_JAL(hook, GetAddress(&vaGetFrameTex));
// 	// gfxDrawBillboardQuad(HBOLT_SCALE + .05, 0, MATH_PI, moby->position, texture, opacity, 0);
// 	// gfxDrawQuad(&quad, m2);
// 	gfxDrawBillboardQuad(HBOLT_SCALE, 0.01, MATH_PI, moby->position, 20, color, 0);
// 	// HOOK_JAL(hook, GetAddress(&vaGetEffectTex));

// 	if (!particle) {
// 		particle = mobySpawnParticle(moby->position, texture, partColor, 100, 1.5);
// 	}
// 	particle->tex = texture;
// 	// if (particle) {
// 	// 	particle->rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
// 	// }
// }

// void mobyUpdate(Moby* moby)
// {
// 	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
// 	const int opacities[] = { 64, 32, 44, 51 };
// 	VECTOR t;
// 	int i;
// 	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->pVar;
// 	if (!pvars)
// 		return;

// 	gfxStickyFX(&mobyPostDraw, moby);
// }

// void mobyTestSpawn(VECTOR position)
// {
// 	Moby* moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(struct HBoltPVar));
// 	if (!moby) return;

// 	moby->pUpdate = &mobyUpdate;
// 	vector_copy(moby->position, position);
// 	moby->updateDist = -1;
// 	moby->drawn = 1;
// 	moby->opacity = 0x00;
// 	moby->drawDist = 0x00;
// 	moby->lights = 0;
// 	// soundPlayByOClass(1, 0, moby, MOBY_ID_OMNI_SHIELD);
// }



	// switch (mode) {
	// 	case DEOBFUSCATE_MODE_HEALTH: {
	// 		stack.step = 5;
	// 		stack.max = 0x28;
	// 		stack.multiplyVar = 0xff;
	// 		stack.randData = GetAddress(&vaPlayerObfuscateAddr);
	// 		break;
	// 	}
	// 	case DEOBFUSCATE_MODE_WEAPON: {
	// 		stack.step = 3;
	// 		stack.max = 0x18;
	// 		stack.multiplyVar = 0xd1;
	// 		stack.randData = GetAddress(&vaPlayerObfuscateWeaponAddr);
	// 		break;
	// 	}
	// 	case DEOBFUSCATE_MODE_TIMER: {
	// 		stack.step = 3;
	// 		stack.max = 0x18;
	// 		stack.multiplyVar = 0xff;
	// 		stack.randData = GetAddress(&vaPlayerObfuscateAddr);
	// 		break;
	// 	}
	// }

extern VariableAddress_t vaPlayerObfuscateAddr;
extern VariableAddress_t vaPlayerObfuscateWeaponAddr;
typedef struct deobfuscate {
	char *randData;
	int max;
	int step;
	int multiplyVal;
	union {
		int data[2];
		struct {
			int addr;
			int val;
		}
	}
} deobfuscate;
int pDeob_working(int src, int addr, int mode)
{
	char *i = src;
	// i = address, *i = data
	if (!*i)
		return 0;

	deobfuscate stack;
	u8 *data = &stack.data;
	int n = 0;
	int m = 0;
	stack.max = 0x28;
	stack.step = 5;
	stack.multiplyVal = 0xff;
    stack.randData = !addr ? GetAddress(&vaPlayerObfuscateAddr) : GetAddress(&vaPlayerObfuscateWeaponAddr);
	int converted;
	for (n; n < stack.max; n += stack.step) {
		u32 offset = (u32)((int)i - (u32)*i & 7) + n;
		*data = stack.randData[(*i + (offset & 7) * stack.multiplyVal)];
        ++data;
	}
	stack.addr = (u32)(stack.addr) ^ (u32)i;
	stack.val = (u32)((u32)stack.val ^ stack.addr);
	converted = stack.val;
	return converted;
}

int pDeob(int src, int mode)
{
	char *i = src;
	// i = address, *i = data
	if (!*i && mode == 0)
		return 0;

	deobfuscate stack;
	switch (mode) {
		case 0: {
			stack.max = 0x28;
			stack.step = 5;
			stack.multiplyVal = 0xff;
			stack.randData = GetAddress(&vaPlayerObfuscateAddr);
			break;
		}
		case 1: {
			stack.max = 0x18;
			stack.step = 3;
			stack.multiplyVal = 0xd1;
    		stack.randData = GetAddress(&vaPlayerObfuscateWeaponAddr);
			break;
		}
		case 2: {
			stack.max = 0x18;
			stack.step = 3;
			stack.multiplyVal = 0xff;
    		stack.randData = GetAddress(&vaPlayerObfuscateAddr);
			break;
		}
	}
	u8 *data = &stack.data;
	int n = 0;
	for (n; n < stack.max; n += stack.step) {
		u32 offset = (u32)((int)i - (u32)*i & 7) + n;
		*data = stack.randData[(*i + (offset & 7) * stack.multiplyVal)];
        ++data;
	}
	stack.addr = (u32)((u32)stack.val ^ ((u32)(stack.addr) ^ (u32)i));
	stack.val = (u32)stack.addr >> 0x10;
	if (mode == 0) {
		return stack.addr;
	} else if (mode == 1) {
        return stack.val & 0xff;
	} else if (mode == 2) {
		return stack.val;
	}
	// all other modes, return -1
	return -1;
}

void playerObfuscate(int src, u32 value, DeobfuscateMode_e mode)
{
	Deobfuscate_t stack;
    float v = value;
	char *i = src; // i: address, *i: value
	char *data = &stack.data;
    stack.randData = GetAddress(&vaPlayerObfuscateAddr);
	stack.step = 5;
	stack.max = 0x28;
	stack.multiplyVal = 0xff;
    u32 xord = *(u32*)(((u32)stack.randData - 0x1) + (((int)i * gameGetGSFrame()) % 0x1fe) * 4);
	stack.addr = xord ^ (u32)i;
    stack.val = xord ^ *(u32*)&v;
    int n = 0;
	int m = 0;
    for (n; n < stack.max; n += stack.step) {
        u32 offset = (u32)(((int)i - (u32)*i & 7) + n);
        stack.randData[(*i + (offset & 7) * stack.multiplyVal)] = *data;
        ++data;
    }
}

int main(void)
{
	((void (*)(void))0x00126780)();

	uyaPreUpdate();

	// GameSettings * gameSettings = gameGetSettings();
	// GameOptions * gameOptions = gameGetOptions();
	// if (gameSettings->GameLoadStartTime > 0) {
	// 	gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 0;
	// 	gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_GatlinTurrets = 1;
	// 	// gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_SmallTurrets = 1;
	// }

	// hud();

    if (isInGame()) {
		Player * p = playerGetFromSlot(0);
		if (!p)
			return 0;

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;
		// gameGetLocalSettings()->Wide = 1;

		// printf("\no: %d, n: %d | wo: %d, wn: %d | to: %03d, tn: %03d", playerDeobfuscate(&p->state, 0), pDeob(&p->state, 0), playerDeobfuscate(&p->quickSelect.Slot[0], 1), pDeob(&p->quickSelect.Slot[0], 1), playerGetRespawnTimer(p), pDeob(&p->timers.resurrectWait, 2));
		
		// hypershotEquipBehavior();

		// gfxStickyFX(&PostDraw, p->PlayerMoby);
		// drawEffectQuad(p->pMoby->position, EFFECT_ME, 1);
		// drawSomething(p->pMoby);
		
		// cycle through sprite/effect textures
		// debugTextures();
		// if (playerPadGetButtonDown(p, PAD_R3) > 0)
		// 	mobyTestSpawn(p->playerPosition);

		// base light follow player
		// ((void (*)(Moby*))0x003F6670)(p->pMoby);

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// Only print state if it's diferent from last state.
		static int nowState = -1;
		int currentState = playerDeobfuscate(&p->state, 0);
		if (currentState != nowState) {
			nowState = currentState;
			// printf("\n------------------");
			// printf("\nState: %d", nowState);
			// printf("\n------------------");
		}

		// if (playerPadGetButtonDown(p, PAD_DOWN) > 0) {
			// printf("\n------------------");
			// printf("\nPrevious State: %d", playerDeobfuscate(&p->previousState, 0, 0));
			// printf("\nPrePrevious State: %d", playerDeobfuscate(&p->prePreviousState, 0, 0));
			// printf("\nState Type: %d", playerDeobfuscate(&p->stateType, 0, 0));
			// printf("\nPrevious Type: %d", playerDeobfuscate(&p->previousType, 0, 0));
			// printf("\nPrePrevious Type: %d", playerDeobfuscate(&p->prePreviousType, 0, 0));
			// printf("\nground: %08x", (u32)((u32)&p->ground - (u32)p));
			// printf("\nhead: %08x", (u32)((u32)&p->head - (u32)p));
			// printf("\nquickSelect: %08x", (u32)((u32)&p->quickSelect - (u32)p));
			// printf("\nloopingSounds: %08x", (u32)((u32)&p->loopingSounds - (u32)p));
			// printf("\nmtxFxActive: %08x", (u32)((u32)&p->mtxFxActive - (u32)p));
			// printf("\npnetplayer: %08x", (u32)((u32)&p->pNetPlayer - (u32)p));
			// printf("\n------------------");
		// }

		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, FONT_ALIGN_CENTER_CENTER);
		
		// printf("Collision: %d\n", CollHotspot());
        
		// drawCollider(p->PlayerMoby);
        // runB6HitVisualizer();
		// v2_Setting(2, first);

		// betterHealthBoxes_Move();

		// runCTF();
		// runSiege();

		InfiniteChargeboot();
		InfiniteHealthMoonjump();
    	DebugInGame(p);
    } else {
		// DebugInMenus();
	}

	// StartBots();

	uyaPostUpdate();

	return 0;
}
