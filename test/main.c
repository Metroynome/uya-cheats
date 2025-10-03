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
		// int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
		// playerSetTeam(player, SetTeam);
        // Hurt Player
        playerDecHealth(player, 1);
	} else if (playerPadGetButtonDown(player, PAD_RIGHT) > 0) {
		playerSetHealth(player, 15);
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
		((void (*)(u8, void*))0x0043bd98)(player->isLocal2, cuboid);
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
	VECTOR pTL = {1, 0, 1, 1};
	VECTOR pTR = {-1, 0, 1, 1};
	VECTOR pBL = {1, 0, -1, 1};
	VECTOR pBR = {-1 ,0 , -1, 1};

	// determine color
	u32 color = 0x80FFFFFF;

	// set draw args
	matrix_unit(m2);

	// color of each corner?
	vector_copy(quad.point[0], pTL);
	vector_copy(quad.point[1], pTR);
	vector_copy(quad.point[2], pBL);
	vector_copy(quad.point[3], pBR);
	quad.rgba[0] = quad.rgba[1] = quad.rgba[2] = quad.rgba[3] = color;
	quad.uv[0] = (UV_t){0, 0};
	quad.uv[1] = (UV_t){0, 1};
	quad.uv[2] = (UV_t){1, 0};
	quad.uv[3] = (UV_t){1, 1};
	quad.clamp = 0;
	quad.tex0 = gfxGetEffectTex(texId);
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
	gfxDrawQuad(quad, &m2);
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
	gfxDrawSprite(x, y, scale, scale, 0, 0, 32, 32, 0x80ffffff, texture);
	// gfxDrawSprite(x+delta, y+delta, scale, scale, 0, 0, 32, 32, 0x80ffffff, texture);
	
	// VECTOR o = {0, 0, 1, 0};
	// vector_add(o, o, playerGetFromSlot(0)->playerPosition);
	// gfxDrawBillboardQuad(scale, scale, MATH_PI, o, texture, 0x80ffffff, 0);
	
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

	testSpritesOrEffects(SpriteOrEffect, texture, SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 0);
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

    if (isInGame()) {
		Player * p = playerGetFromSlot(0);
		if (!p)
			return 0;

		// force lock-strafe (controller 1)
		// *(u8*)0x001A5a34 = 1;
		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;
		// gameGetLocalSettings()->Wide = 1;
		
		// cycle through sprite/effect textures
		// debugTextures();

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// print deobfuscated: state, health, weapon[0], ressurrect time
		// int state = playerGetState(p);
		// int health = playerGetHealth(p);
		// int weapon = playerDeobfuscate(&p->quickSelect.Slot[0], 1);
		// int timer = playerGetRespawnTimer(p);
		// printf("s: %d, h: %d, w: %d, r: %04d\n", state, health, weapon, timer);


		// Only print state if it's diferent from last state.
		// static int nowState = -1;
		// int currentState = playerDeobfuscate(&p->state, 0);
		// if (currentState != nowState) {
		// 	nowState = currentState;
		// 	printf("\n------------------");
		// 	printf("\nState: %d", nowState);
		// 	printf("\n------------------");
		// }

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
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, TEXT_ALIGN_MIDDLECENTER, FONT_BOLD);
		
		// printf("Collision: %d\n", CollHotspot());
        
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
	// hud();
	secret();

	uyaPostUpdate();

	return 0;
}
