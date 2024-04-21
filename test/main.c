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

#define TestMoby	(*(Moby**)0x00091004)

VECTOR position;
VECTOR rotation;

static int SPRITE_ME = 0;
static int EFFECT_ME = 21;
static int SOUND_ME = 0;
static int SOUND_ME_FLAG = 3;

int first = 1;

extern VariableAddress_t vaFlagUpdate_Func;
extern VariableAddress_t vaGiveWeaponFunc;
extern VariableAddress_t vaPlayerRespawnFunc;

void DebugInGame(Player* player)
{
    if (playerPadGetButtonDown(player, PAD_LEFT) > 0) {
		// Swap Teams
		// int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
		// playerSetTeam(player, SetTeam);
	} else if (playerPadGetButtonDown(player, PAD_RIGHT) > 0) {
        // Hurt Player
        playerDecHealth(player, 1);
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
	} else if (playerPadGetButtonDown(player, PAD_R3) > 0) {
		// int j;
		// u8* slot = (u8*)(u32)player + 0x1a32;
		// for(j = 0; j < 12; ++j)
		// 	playerGiveWeaponUpgrade(player, playerDeobfuscate(&slot[j], 1, 1));
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
	int i;
	Player ** players = playerGetAll();
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * player = players[i];
		if (!player)
			continue;

		if (player->timers.IsChargebooting == 1 && playerPadGetButton(player, PAD_R2) > 0 && player->timers.state > 55)
			player->timers.state = 55;
	}
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
        Player * player = (Player*)PLAYER_STRUCT;
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
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};

	// determine color
	u32 color = 0x80FFFFFF;

	// set draw args
	matrix_unit(m2);

	// init
	// gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
	quad.VertexUVs[0] = (struct UV){0,0};
	quad.VertexUVs[1] = (struct UV){1,0};
	quad.VertexUVs[2] = (struct UV){0,1};
	quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetEffectTex(texId, 1);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

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
	gfxDrawQuad(&quad, m2, 1);
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



struct FlagPVars
{
	VECTOR basePosition;
	short carrierIdx;
	short lastCarrierIdx;
	short team;
	char UNK_16[6];
	int timeFlagDropped;
};
/*
 * NAME :		flagHandlePickup
 * 
 * DESCRIPTION :
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void flagHandlePickup(Moby* flagMoby, int pIdx)
{
	Player** players = playerGetAll();
	Player* player = players[pIdx];
	if (!player || !flagMoby)
		return;
	
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;
	if (!pvars)
		return;

	// fi flag state isn't 1
	if (flagMoby->state != 1)
		return;

	// flag is currently returning
	if (flagIsReturning(flagMoby))
		return;

	// flag is currently being picked up
	if (flagIsBeingPickedUp(flagMoby))
		return;

	// only allow actions by living players
	if (playerIsDead(player) || playerGetHealth(player) <= 0)
		return;

	// Handle pickup/return
	if (player->mpTeam == pvars->team) {
		flagReturnToBase(flagMoby, 0, pIdx);
	} else {
		flagPickup(flagMoby, pIdx);
		player->flagMoby = flagMoby;
	}
	DPRINTF("player %d picked up flag %X at %d\n", player->mpIndex, flagMoby->oClass, gameGetTime());
}

/*
 * NAME :		flagRequestPickup
 * 
 * DESCRIPTION :
 * 			Requests to either pickup or return the given flag.
 * 			If host, this request is automatically passed to the handler.
 * 			If not host, this request is sent over the net to the host.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void flagRequestPickup(Moby* flagMoby, int pIdx)
{
	static int requestCounters[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0,0,0};
	Player** players = playerGetAll();
	Player* player = players[pIdx];
	if (!player || !flagMoby)
		return;
	
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;
	if (!pvars)
		return;

	if (gameAmIHost()) {
		// handle locally
		flagHandlePickup(flagMoby, pIdx);
	} else if (requestCounters[pIdx] == 0) {
		// send request to host
		// void* dmeConnection = netGetDmeServerConnection();
		// if (dmeConnection) {
		// 	ClientRequestPickUpFlag_t msg;
		// 	msg.GameTime = gameGetTime();
		// 	msg.PlayerId = player->mpIndex;
		// 	msg.FlagUID = guberGetUID(flagMoby);
		// 	netSendCustomAppMessage(dmeConnection, gameGetHostId(), CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP, sizeof(ClientRequestPickUpFlag_t), &msg);
		// 	requestCounters[pIdx] = 10;
		// 	DPRINTF("sent request flag pickup %d\n", gameGetTime());
		// }
	}
	else
	{
		requestCounters[pIdx]--;
	}
}

/*
 * NAME :		customFlagLogic
 * 
 * DESCRIPTION :
 * 			Reimplements flag pickup logic but runs through our host authoritative logic.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Troy "Metroynome" Pruitt
 */
void customFlagLogic(Moby* flagMoby)
{
	VECTOR t;
	int i;
	Player** players = playerGetAll();
	int gameTime = gameGetTime();
	GameOptions* gameOptions = gameGetOptions();

	// if not in game
	if (!isInGame())
		return;

	// if flagMoby doesn't exist
	if (!flagMoby)
		return;

	// if flag pvars don't exist
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;
	if (!pvars)
		return;

	// if flag state doesn't equal 1
	if (flagMoby->state != 1)
		return;

	// if flag is returning
	if (flagIsReturning(flagMoby))
		return;

	// if flag is being picked up
	if (flagIsBeingPickedUp(flagMoby))
		return;

	// return to base if flag has been idle for 40 seconds
	if ((pvars->timeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
		flagReturnToBase(flagMoby, 0, 0xFF);
		return;
	}
	
	// if flag didn't land on safe ground, and after .5s a player died
	static int flagIgnorePlayer = 0;
	// if(gameConfig.grFlagHotspots) {
	// 	if (!flagIsOnSafeGround(flagMoby) && !flagIsAtBase(flagMoby) && (flagIgnorePlayer + 300) < gameTime) {
	// 		flagReturnToBase(flagMoby, 0, 0xff);
	// 		return;
	// 	}
	// }

	// wait 1.5 seconds for last carrier to be able to pick up again
	if ((pvars->timeFlagDropped + 1500) > gameTime)
		return;

	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player* player = players[i];
		if (!player || !player->isLocal)
			continue;

		// only allow actions by living players, and non-chargebooting players
		if ((playerIsDead(player) || playerGetHealth(player) <= 0) || (player->timers.IsChargebooting == 1 && (playerPadGetButton(player, PAD_R2) > 0) && player->timers.state > 55)){
			// if flag holder died, update flagIgnorePlayer time.
			if (pvars->lastCarrierIdx == player->mpIndex)
				flagIgnorePlayer = gameTime;

			continue;
		}

		// skip player if they've only been alive for < 3 seconds
		if (player->timers.timeAlive <= 180)
			continue;

		// skip player if in vehicle
		if (player->vehicle && playerDeobfuscate(&player->state, 0, 0) == PLAYER_STATE_VEHICLE)
			continue;

		// skip if player state is in vehicle and critterMode is on
		if (player->camera && player->camera->camHeroData.critterMode)
			continue;

		// skip if player is on teleport pad
		// AQuATOS BUG: player->ground.pMoby points to wrong area
		if (player->ground.pMoby && player->ground.pMoby->oClass == MOBY_ID_TELEPORT_PAD)
			continue;

		// player must be within 2 units of flag
		vector_subtract(t, flagMoby->position, player->playerPosition);
		float sqrDistance = vector_sqrmag(t);
		if (sqrDistance > (2*2))
			continue;

		// player is on different team than flag and player isn't already holding flag
		if (player->mpTeam != pvars->team) {
			if (!player->flagMoby) {
				flagRequestPickup(flagMoby, i);
				return;
			}
		} else {
			// if player is on same team as flag and close enough to return it
			vector_subtract(t, pvars->basePosition, flagMoby->position);
			float sqrDistanceToBase = vector_sqrmag(t);
			if (sqrDistanceToBase > 0.1) {
				flagRequestPickup(flagMoby, i);
				return;
			}
		}
	}
}

/*
 * NAME :		patchCTFFlag
 * 
 * DESCRIPTION :
 * 			Patch CTF Flag update function with our own
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Troy "Metroynome" Pruitt
 */
void patchCTFFlag(void)
{
	VECTOR t;
	int i = 0;
	Player** players = playerGetAll();

	if (!isInGame())
		return;

	// netInstallCustomMsgHandler(CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP, &onRemoteClientRequestPickUpFlag);

	u32 FlagFunc = GetAddress(&vaFlagUpdate_Func);
	if (FlagFunc){
		*(u32*)FlagFunc = 0x03e00008;
		*(u32*)(FlagFunc + 0x4) = 0x0000102D;
	}

	GuberMoby* gm = guberMobyGetFirst();
	while (gm) {
		if (gm->Moby) {
			switch (gm->Moby->oClass) {
				case MOBY_ID_CTF_RED_FLAG:
				case MOBY_ID_CTF_BLUE_FLAG:
				{
					customFlagLogic(gm->Moby);
					// Master * master = masterGet(gm->Guber.Id.UID);
					// if (master)
					// 	masterDelete(master);

					break;
				}
			}
		}
		gm = (GuberMoby*)gm->Guber.Next;
	}
}

VariableAddress_t vaHypershotEquipBehavior_bits = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x00510f54,
	.Hoven = 0x0051306c,
	.OutpostX12 = 0x00508944,
	.KorgonOutpost = 0x005060dc,
	.Metropolis = 0x0050542c,
	.BlackwaterCity = 0x00502cc4,
	.CommandCenter = 0x00502c8c,
	.BlackwaterDocks = 0x0050550c,
	.AquatosSewers = 0x0050480c,
	.MarcadiaPalace = 0x0050418c,
#else
	.Lobby = 0,
	.Bakisi = 0x0050e73c,
	.Hoven = 0x00510794,
	.OutpostX12 = 0x005060ac,
	.KorgonOutpost = 0x005038c4,
	.Metropolis = 0x00502c14,
	.BlackwaterCity = 0x0050042c,
	.CommandCenter = 0x018059e8,
	.BlackwaterDocks = 0x00502df4,
	.AquatosSewers = 0x00502134,
	.MarcadiaPalace = 0x00501a74,
#endif
};

int confighypershotEquipButton = 1;
int configdisableDpadMovement = 1;

int hypershotGetButton(void)
{
	switch (confighypershotEquipButton) {
		case 1: return PAD_CIRCLE;
		case 2: return PAD_LEFT;
		case 3: return PAD_DOWN;
		case 4: return PAD_RIGHT;
		case 5: return PAD_UP;
		case 6: return PAD_L3;
		case 7: return PAD_R3;
		default: return 0;
	}
}

void hypershotEquipButton(void)
{
	// get Player 1 struct
	Player *p = playerGetFromSlot(0);
	// if player is found, and not holding flag, and prsses needed button.
	if (p && !p->flagMoby && playerPadGetButtonDown(p, hypershotGetButton()) > 0)
		playerEquipWeapon(p, WEAPON_ID_SWINGSHOT);
}

int remapButtons(pad)
{
	// disable the default action for the chosen hypershot button.
	if (confighypershotEquipButton) {
		u16 hypershot = hypershotGetButton();
		if ((pad & hypershot) == 0)
			return 0xffff & (pad | hypershot);
	}

	if (configdisableDpadMovement) {
		// Make a mask of the bits we want to filter
		u16 mask = (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN);
		// only grab the mask filter from the pad
		u16 dpad = (pad ^ mask) & 0x00f0;
		// if any dpad button is pressed, return data as if it's not pressed.
		if ((dpad & pad) == 0)
			return 0xffff & (pad | dpad);
	}


	switch (pad ^ 0xffff) {
		// EXAMPLE: if Presssing X, return by telling it to press circle instead.
		// case PAD_CROSS:
		// 	return PAD_CIRCLE ^ (0xffff & (pad | PAD_CROSS));

		// if dpad is pressed, return by acting as if they were not pressed.
		// case PAD_LEFT: return configdisableDpadMovement ? (0xffff & (pad | PAD_LEFT)) : pad;
		// case PAD_LEFT | PAD_UP: return configdisableDpadMovement ? (0xffff & (pad | (PAD_LEFT | PAD_UP))) : pad;
		// case PAD_LEFT | PAD_DOWN: return configdisableDpadMovement ? (0xffff & (pad | (PAD_LEFT | PAD_DOWN))) : pad;
		// case PAD_RIGHT: return configdisableDpadMovement ? (0xffff & (pad | PAD_RIGHT)) : pad;
		// case PAD_RIGHT | PAD_UP: return configdisableDpadMovement ? (0xffff & (pad | (PAD_RIGHT | PAD_UP))) : pad;
		// case PAD_RIGHT | PAD_DOWN: return configdisableDpadMovement ? (0xffff & (pad | (PAD_RIGHT | PAD_DOWN))) : pad;
		// case PAD_UP: return configdisableDpadMovement ? (0xffff & (pad | PAD_UP)) : pad;
		// case PAD_DOWN: return configdisableDpadMovement ? (0xffff & (pad | PAD_DOWN)) : pad;
		// // if nothing is pressed, then return original data.
		default: return pad;

	}
}

void patchSceReadPad_memcpy(void * destination, void * source, int num)
{
	Player * player = playerGetFromSlot(0);
	// make sure the pause menu is not open.  this way the pause menu can still be used.
	if (player && !player->pauseOn) {
		u32 paddata = (void*)((u32)source + 0x2);
		u16 mask = (PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN);
		u16 dpad = ((*(u16*)paddata ^ mask) & 0x00f0);
		printf("\nmask: 0x%04x", mask);
		printf("\ndata: 0x%04x", *(u16*)paddata);
		printf("\ndpad: 0x%04x", dpad); // checks dpad only

		// edit the pad data.
		*(u16*)paddata = remapButtons(*(u16*)paddata);
		// printf("\npad 2: 0x%04x", PAD_LEFT | PAD_UP);
	}
	// finish up by running the original function we took over.
	memcpy(destination, source, num);
}


int main(void)
{
	((void (*)(void))0x00126780)();

	uyaPreUpdate();

	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	// if (gameOptions || gameSettings) {
	// 	gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 0;
	// 	gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_GatlinTurrets = 0;
	// 	gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_SmallTurrets = 0;
	// }

    if (isInGame()) {
		Player * p = playerGetFromSlot(0);
		if (!p)
			return 0;

		if (confighypershotEquipButton)
			hypershotEquipButton();

		HOOK_JAL(0x0013cae0, &patchSceReadPad_memcpy);

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// hypershotEquipBehavior();

		// gfxStickyFX(&PostDraw, p->PlayerMoby);
		// drawEffectQuad(p->PlayerMoby->Position, 1, .5);
		// drawSomething(p->PlayerMoby);
		
		// base light follow player
		// ((void (*)(Moby*))0x003F6670)(p->PlayerMoby);

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// printf("\nState: %d", playerDeobfuscate(&p->state, 0, 0));
		// printf("\nPrevious State: %d", playerDeobfuscate(&p->previousState, 0, 0));
		// printf("\nPrePrevious State: %d", playerDeobfuscate(&p->prePreviousState, 0, 0));
		// printf("\nState Type: %d", playerDeobfuscate(&p->stateType, 0, 0));
		// printf("\nPrevious Type: %d", playerDeobfuscate(&p->previousType, 0, 0));
		// printf("\nPrePrevious Type: %d", playerDeobfuscate(&p->prePreviousType, 0, 0));
		// printf("\nground: %x", (u32)((u32)&p->ground - (u32)PLAYER_STRUCT));
		// printf("\nquickSelect: %x", (u32)((u32)&p->quickSelect - (u32)PLAYER_STRUCT));
		// printf("\nloopingSounds: %x", (u32)((u32)&p->loopingSounds - (u32)PLAYER_STRUCT));
		// printf("\nskinMoby3: %x", (u32)((u32)&p->skinMoby3 - (u32)PLAYER_STRUCT));
		// printf("\nmtxFxActive: %x", (u32)((u32)&p->mtxFxActive - (u32)PLAYER_STRUCT));
		// printf("\npnetplayer: %x", (u32)((u32)&p->pNetPlayer - (u32)PLAYER_STRUCT));

		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, FONT_ALIGN_CENTER_CENTER);
		
		// printf("Collision: %d\n", CollHotspot());
        
		// drawCollider(p->PlayerMoby);
		// patchCTFFlag();
        // runB6HitVisualizer();
		// v2_Setting(2, first);
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
    	DebugInGame(p);
    } else {
		DebugInMenus();
	}

	// StartBots();

	uyaPostUpdate();

	return 0;
}
