#include <libuya/moby.h>
#include <libuya/player.h>
#include <libuya/guber.h>
#include <libuya/net.h>
#include <libuya/time.h>
#include <libuya/interop.h>
#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/graphics.h>
#include <libuya/color.h>
#include <libuya/random.h>
#include <libuya/math.h>
#include <libuya/game.h>
#include <libuya/map.h>
#include <libuya/team.h>
#include <libuya/spawnpoint.h>
#include <libuya/ui.h>
#include <libuya/utils.h>

struct flagParticles {
	struct PartInstance* Particles[4];
};

typedef struct ClientRequestPickUpFlag {
    int GameTime;
    int PlayerId;
    u32 FlagUID;
} ClientRequestPickUpFlag_t;

enum CustomMessageId {
    CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP = 15
};

int ctfLogic = 0;
int grFlagHotspots = 0;

extern VariableAddress_t vaFlagUpdate_Func;
extern VariableAddress_t vaGetFrameTex;
extern VariableAddress_t vaGetEffectTex;
/*
 * NAME :		flagHandlePickup
 * DESCRIPTION :
 * NOTES :
 * ARGS : 
 * RETURN :
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void flagHandlePickup(Moby* flagMoby, int pIdx)
{
	Player** players = playerGetAll();
	Player* player = players[pIdx];
	if (!player || !flagMoby)
		return;
	
	flagPVars_t* pvars = (flagPVars_t*)flagMoby->pVar;
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
 * DESCRIPTION :
 * 			Requests to either pickup or return the given flag.
 * 			If host, this request is automatically passed to the handler.
 * 			If not host, this request is sent over the net to the host.
 * NOTES :
 * ARGS : 
 * RETURN :
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void flagRequestPickup(Moby* flagMoby, int pIdx)
{
	static int requestCounters[GAME_MAX_PLAYERS] = {0,0,0,0,0,0,0,0};
	Player** players = playerGetAll();
	Player* player = players[pIdx];
	if (!player || !flagMoby)
		return;
	
	flagPVars_t* pvars = (flagPVars_t*)flagMoby->pVar;
	if (!pvars)
		return;

	if (gameAmIHost()) {
		// handle locally
		flagHandlePickup(flagMoby, pIdx);
	} else if (requestCounters[pIdx] == 0) {
		// send request to host
		void* dmeConnection = netGetDmeServerConnection();
		if (dmeConnection) {
			ClientRequestPickUpFlag_t msg;
			msg.GameTime = gameGetTime();
			msg.PlayerId = player->mpIndex;
			msg.FlagUID = guberGetUID(flagMoby);
			netSendCustomAppMessage(dmeConnection, gameGetHostId(), CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP, sizeof(ClientRequestPickUpFlag_t), &msg);
			requestCounters[pIdx] = 10;
			DPRINTF("sent request flag pickup %d\n", gameGetTime());
		}
	}
	else
	{
		requestCounters[pIdx]--;
	}
}

void flagParticles(Moby *moby, u32 overrideColor)
{
	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };
	VECTOR particlePosition;
	int i;
	struct flagParticles* pvars = (struct flagParticles*)moby->pVar;
	if (!pvars)
		return;

	vector_copy(particlePosition, moby->position);
	(float)particlePosition[0] -= .2;
	(float)particlePosition[2] += 1.8;
	printf("\npartPos: %08x, %08x, %08x", (float)particlePosition[0], (float)particlePosition[1], (float)particlePosition[2]);

	// handle particles
	// u32 color = colorLerp(0, 0x0000ffff, 1.0 / 4);
	u32 color = (moby->oClass == MOBY_ID_CTF_RED_FLAG) ? 0x000000ff : 0x00ff0000;
	color |= 0x80000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			particle = gfxSpawnParticle(particlePosition, 53, color, 100, 1.5 + (0.5 * i));;
		}

		// update
		if (particle) {
			particle->rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
		}
	}
}

/*
 * NAME :		customFlagLogic
 * DESCRIPTION :
 * 			Reimplements flag pickup logic but runs through our host authoritative logic.
 * NOTES :
 * ARGS : 
 * RETURN :
 * AUTHOR :			Troy "Metroynome" Pruitt
 */
void customFlagLogic(Moby* flagMoby)
{
	VECTOR t;
	int i;
	Player** players = playerGetAll();
	int gameTime = gameGetTime();
	GameOptions* gameOptions = gameGetOptions();
	flagPVars_t* pvars = (flagPVars_t*)flagMoby->pVar;

	// if flag moby or pvars don't exist, stop.
	if (!flagMoby || !pvars)
		return;

	flagParticles(flagMoby, 0);

    // if flag state is not 1 (being picked up) and if flag is returning to base
    if (flagMoby->state != 1 || flagIsReturning(flagMoby))
        return;

    // flag is being picked up
    if (flagIsBeingPickedUp(flagMoby))
        return;
    
	// return to base if flag has been idle for 40 seconds and not already at base.
	if ((pvars->timeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby) && !flagIsAtBase(flagMoby)) {
		flagReturnToBase(flagMoby, 0, 0xff);
		return;
	}

    if ((pvars->timeFlagDropped + (TIME_SECOND * 1.5)) > gameTime)
        return;

    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        Player* player = players[i];
        if (!player)
            continue;
        // Don't allow input from players whom are dead
        if (playerDeobfuscate(&player->stateType, 0, 0) == PLAYER_TYPE_DEATH)
            return;

        // skip player if they've only been alive for < 180ms
        if (player->timers.timeAlive < 180)
            return;

        // skip if player state is in vehicle and critterMode is on
		if (player->camera && player->camera->camHeroData.critterMode)
			continue;

    	// skip player if in vehicle
		if (player->vehicle && playerDeobfuscate(&player->state, 0, 0) == PLAYER_STATE_VEHICLE)
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
			vector_subtract(t, pvars->basePos, flagMoby->position);
			float sqrDistanceToBase = vector_sqrmag(t);
			if (sqrDistanceToBase > 0.1) {
				flagRequestPickup(flagMoby, i);
				return;
			}
		}
	}

    /* ===============1:1 Code: Doesn't work===========*/
    // if ((flagMoby->state == 1 && !flagIsReturning(flagMoby)) && (!flagIsBeingPickedUp(flagMoby) && (pvars->timeFlagDropped + (TIME_SECOND * 1.5)) < gameTime)) {
    //     FLAG_AT_BASE = 1;
    //     // Return to base if flag has been idle for 40 seconds
    //     if ((pvars->timeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
    //         FLAG_AT_BASE = 0;
    //         flagReturnToBase(flagMoby, 0, 0xff);
    //     }
    //     if (FLAG_AT_BASE) {
    //         int stateType = 0;
    //         for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
    //             Player* player = players[i];
    //             if (player) {
    //                 if (player->stateType == 0) {
    //                     stateType = 0;
    //                 } else {
    //                     stateType = playerDeobfuscate(&player->stateType, 0, 0);
    //                     if (((stateType != PLAYER_TYPE_DEATH) && (player->timers.timeAlive > 180)) && !player->vehicle) {
    //                         FLAG_AT_BASE = 0;
    //                         if (player->camera && player->camera->camHeroData.critterMode) {
    //                             if (!player->state) {
    //                                 stateType = 0;
    //                             } else {
    //                                 stateType = playerDeobfuscate(&player->state, 0, 0);
    //                                 if (stateType == PLAYER_STATE_VEHICLE) {
    //                                     FLAG_AT_BASE = 1;
    //                                 }
    //                             }
    //                             if (!FLAG_AT_BASE && (!player->ground.pMoby || player->ground.pMoby->oClass != MOBY_ID_TELEPORT_PAD)) {
    //                                 vector_subtract(t, flagMoby->position, player->playerPosition);
	// 	                            float sqrDistance = vector_sqrmag(t);
	// 	                            if (sqrDistance < (2 * 2)) {
    //                                     // player is on different team than flag and player isn't already holding flag
    //                                     if (player->mpTeam != pvars->team) {
    //                                         if (!player->flagMoby) {
    //                                             flagRequestPickup(flagMoby, player->mpIndex);
    //                                             return;
    //                                         }
    //                                     } else {
    //                                         // if player is on same team as flag and close enough to return it
    //                                         vector_subtract(t, pvars->basePos, flagMoby->position);
    //                                         float sqrDistanceToBase = vector_sqrmag(t);
    //                                         if (sqrDistanceToBase < 0.1) {
    //                                             flagRequestPickup(flagMoby, player->mpIndex);
    //                                             return;
    //                                         }
    //                                     } 
    //                                 }
    //                             }
    //                         }
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

	// // if flagMoby doesn't exist
	// if (!flagMoby)
	// 	return;

	// // if flag pvars don't exist
	// flagPVars_t* pvars = (flagPVars_t*)flagMoby->pVar;
	// if (!pvars)
	// 	return;

	// // if flag state doesn't equal 1
	// if (flagMoby->state != 1)
	// 	return;

	// // if flag is returning
	// if (flagIsReturning(flagMoby))
	// 	return;

	// // if flag is being picked up
	// if (flagIsBeingPickedUp(flagMoby))
	// 	return;

	// // return to base if flag has been idle for 40 seconds
	// if ((pvars->timeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
	// 	flagReturnToBase(flagMoby, 0, 0xFF);
	// 	return;
	// }
	
	// // if flag didn't land on safe ground, and after .5s a player died
	// static int flagIgnorePlayer = 0;
	// if(grFlagHotspots) {
	// 	if (!flagIsOnSafeGround(flagMoby) && !flagIsAtBase(flagMoby) && (flagIgnorePlayer + 300) < gameTime) {
	// 		flagReturnToBase(flagMoby, 0, 0xff);
	// 		return;
	// 	}
	// }

	// // wait 1.5 seconds for last carrier to be able to pick up again
	// if ((pvars->timeFlagDropped + 1500) > gameTime)
	// 	return;

	// for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
	// 	Player* player = players[i];
	// 	if (!player)
	// 		continue;

	// 	// only allow actions by living players, and non-chargebooting players
	// 	if ((playerIsDead(player) || playerGetHealth(player) <= 0) || (player->timers.IsChargebooting == 1 && (playerPadGetButton(player, PAD_R2) > 0) && player->timers.state > 55)){
	// 		// if flag holder died, update flagIgnorePlayer time.
	// 		if (pvars->LastcrrierIdx == player->mpIndex)
	// 			flagIgnorePlayer = gameTime;

	// 		continue;
	// 	}

	// 	// skip player if they've only been alive for < 3 seconds
	// 	if (player->timers.timeAlive <= 180)
	// 		continue;

	// 	// skip player if in vehicle
	// 	if (player->vehicle && playerDeobfuscate(&player->state, 0, 0) == PLAYER_STATE_VEHICLE)
	// 		continue;

	// 	// skip if player state is in vehicle and critterMode is on
	// 	if (player->camera && player->camera->camHeroData.critterMode)
	// 		continue;

	// 	// skip if player is on teleport pad
	// 	// AQuATOS BUG: player->ground.pMoby points to wrong area
	// 	if (player->ground.pMoby && player->ground.pMoby->oClass == MOBY_ID_TELEPORT_PAD)
	// 		continue;

	// 	// player must be within 2 units of flag
	// 	vector_subtract(t, flagMoby->position, player->playerPosition);
	// 	float sqrDistance = vector_sqrmag(t);
	// 	if (sqrDistance > (2*2))
	// 		continue;

	// 	// player is on different team than flag and player isn't already holding flag
	// 	if (player->mpTeam != pvars->team) {
	// 		if (!player->flagMoby) {
	// 			flagRequestPickup(flagMoby, i);
	// 			return;
	// 		}
	// 	} else {
	// 		// if player is on same team as flag and close enough to return it
	// 		vector_subtract(t, pvars->basePos, flagMoby->position);
	// 		float sqrDistanceToBase = vector_sqrmag(t);
	// 		if (sqrDistanceToBase > 0.1) {
	// 			flagRequestPickup(flagMoby, i);
	// 			return;
	// 		}
	// 	}
	// }
}

/*
 * NAME :		onRemoteClientRequestPickUpFlag
 * DESCRIPTION :
 * 			Handles when a remote client sends the CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP message.
 * NOTES :
 * ARGS : 
 * RETURN :
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
int onRemoteClientRequestPickUpFlag(void * connection, void * data)
{
	int i;
	ClientRequestPickUpFlag_t msg;
	Player** players;
	memcpy(&msg, data, sizeof(msg));

	// ignore if not in game
	if (!isInGame() || !gameAmIHost())
		return sizeof(ClientRequestPickUpFlag_t);

	DPRINTF("remote player %d requested pick up flag %X at %d\n", msg.PlayerId, msg.FlagUID, msg.GameTime);

	// get list of players
	players = playerGetAll();

	// get remote player or ignore message
	Player** allRemote = playerGetAll();
	Player* remotePlayer = allRemote[msg.PlayerId];
	if (!remotePlayer)
		return sizeof(ClientRequestPickUpFlag_t);

	// get flag
	GuberMoby* gm = (GuberMoby*)guberGetObjectByUID(msg.FlagUID);
	if (gm && gm->Moby) {
		Moby* flagMoby = gm->Moby;
		flagHandlePickup(flagMoby, msg.PlayerId);
	}
	return sizeof(ClientRequestPickUpFlag_t);
}
/*
 * NAME :		patchCTFFlag
 * DESCRIPTION :
 * 			Patch CTF Flag update function with our own
 * NOTES :
 * ARGS : 
 * RETURN :
 * AUTHOR :			Troy "Metroynome" Pruitt
 */
void patchCTFFlag(void)
{
	if (!isInGame())
		return;

	VECTOR t;
	int i = 0;
	Player** players = playerGetAll();

	u32 flagFunc = 0;
	if (!ctfLogic) {
		netInstallCustomMsgHandler(CUSTOM_MSG_ID_FLAG_REQUEST_PICKUP, &onRemoteClientRequestPickUpFlag);
		if (!flagFunc)
			flagFunc = GetAddress(&vaFlagUpdate_Func);
		
		if (flagFunc) {
			*(u32*)flagFunc = 0x03e00008;
			*(u32*)(flagFunc + 0x4) = 0x0000102D;
		}
		ctfLogic = 1;
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

VariableAddress_t vaFlagEventUiPopup_Hook = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x00430568,
	.Hoven = 0x00431fc0,
	.OutpostX12 = 0x00428ef0,
	.KorgonOutpost = 0x00426a98,
	.Metropolis = 0x00425df0,
	.BlackwaterCity = 0x00421c58,
	.CommandCenter = 0x004263e8,
	.BlackwaterDocks = 0x00428c38,
	.AquatosSewers = 0x00427f50,
	.MarcadiaPalace = 0x004278b8,
#else
	.Lobby = 0,
	.Bakisi = 0x0042fae0,
	.Hoven = 0x00431470,
	.OutpostX12 = 0x004283b8,
	.KorgonOutpost = 0x00426008,
	.Metropolis = 0x00425368,
	.BlackwaterCity = 0x00421188,
	.CommandCenter = 0x00425a38,
	.BlackwaterDocks = 0x00428270,
	.AquatosSewers = 0x004275a0,
	.MarcadiaPalace = 0x00426ef0,
#endif
};

VariableAddress_t vaReplace_GetEffectTexJAL = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x2045b2f0,
	.Hoven = 0x2045ce70,
	.OutpostX12 = 0x20453c70,
	.KorgonOutpost = 0x20451830,
	.Metropolis = 0x20450b70,
	.BlackwaterCity = 0x2044e370,
	.CommandCenter = 0x2044eff0,
	.BlackwaterDocks = 0x20451870,
	.AquatosSewers = 0x20450b70,
	.MarcadiaPalace = 0x204504f0,
#else
	.Lobby = 0,
	.Bakisi = 0x2045a220,
	.Hoven = 0x2045bce0,
	.OutpostX12 = 0x20452b20,
	.KorgonOutpost = 0x20450760,
	.Metropolis = 0x2044faa0,
	.BlackwaterCity = 0x2044d220,
	.CommandCenter = 0x2044e060,
	.BlackwaterDocks = 0x204508a0,
	.AquatosSewers = 0x2044fbe0,
	.MarcadiaPalace = 0x2044f520,
#endif
};

typedef struct flagPositions {
	int mapId;
	float x;
	float z;
	float y;
} flagPositions_t;

flagPositions_t midFlagPos[] = {
	{MAP_ID_BAKISI, 402.324, 366.073, 200.466},
	{MAP_ID_HOVEN, 259.964, 253.611, 62.598},
	{MAP_ID_OUTPOST_X12, 427.247, 204.962, 108.406},
	{MAP_ID_METROPOLIS, 755.314, 339.311, 337.703},
	{MAP_ID_BLACKWATER_CITY, 219.817, 266.732, 87.722},
	{MAP_ID_BLACKWATER_DOCKS, 211.819, 191.808, 99.281},
	{MAP_ID_AQUATOS_SEWERS, 401.181, 336.686, 940.75}
};

typedef struct midFlagInfo {
	int setup;
	Moby *pRedFlag;
	Moby *pBlueFlag;
	short baseCuboid[2];
} midFlagInfo_t;
midFlagInfo_t midFlag;

int findClosestSpawnPointToPosition(VECTOR position, float deadzone)
{
  // find closest spawn point
  int bestIdx = -1;
  int i;
  float bestDist = 1000000;
  float sqrDeadzone = deadzone * deadzone;
  int spCount = spawnPointGetCount();
  for (i = 0; i < spCount; ++i) {
    if (!spawnPointIsPlayer(i)) continue;

    Cuboid* sp = spawnPointGet(i);
    VECTOR delta;
    vector_subtract(delta, position, &sp->pos);
    float sqrDist = vector_sqrmag(delta);
    if (sqrDist < bestDist && sqrDist >= sqrDeadzone) {
      bestDist = sqrDist;
      bestIdx = i;
    }
  }

  return bestIdx;
}

void mobyPostDraw(Moby* moby)
{
	int opacity = 0x80;
	u32 color = 0x00ffffff;

	// float pulse = (1 + sinf(clampAngle((gameGetTime() / 1000.0) * 1.0))) * 0.5;
	// opacity = 0x20 + (pulse * 0x50);
	opacity = opacity << 24;
	color = opacity | (color & 0x00ffffff);
	moby->primaryColor = color;

	u32 hook = (u32)GetAddress(&vaReplace_GetEffectTexJAL) + 0x20;
	HOOK_JAL(hook, GetAddress(&vaGetFrameTex));
	gfxDrawBillboardQuad(1 + .05, 0, MATH_PI, moby->position, SPRITE_FLAG, opacity, 0);
	gfxDrawBillboardQuad(1, 0.01, MATH_PI, moby->position, SPRITE_FLAG, color, 0);
	HOOK_JAL(hook, GetAddress(&vaGetEffectTex));
}

void mobyUpdate(Moby* moby)
{
	gfxRegistserDrawRoutine(&mobyPostDraw, moby);
}

void mobyTestSpawn(VECTOR position)
{
	Moby* moby = mobySpawn(MOBY_ID_TEST, 0x80);
	if (!moby) return;

	// update height to be above flag
	VECTOR add = {0, 0, 3, 0};
	vector_add(add, position, add);
	vector_copy(moby->position, add);

	moby->pUpdate = &mobyUpdate;
	moby->updateDist = -1;
	moby->drawn = 1;
	moby->opacity = 0x00;
	moby->drawDist = 0x00;	
}

void patchFlagUiPopup_Logic(short stringId, int seconds, int player)
{
	Moby *flag = midFlag.pRedFlag;
	flagPVars_t *flagVars = flag->pVar;
	Player *p = playerGetFromSlot(flagVars->carrierIdx);
	char buff[64];
	char *team[2] = {"Blue", "Red"};
	GameSettings *gs = gameGetSettings();
	int pIdx = p->mpIndex;
	int pTeam = p->mpTeam;
	switch (stringId) {
		case 0x1411:
		case 0x1412: {
			snprintf(buff, 64, "%s has picked up the %s Team's Flag!", gs->PlayerNames[pIdx], (char*)team[!pTeam]);
			break;
		}
		case 0x1413:
		case 0x1414: {
			snprintf(buff, 64, "%s has dropped the %s Team's Flag!", gs->PlayerNames[pIdx], (char*)team[!pTeam]);
			break;		
		}
		default: {
			strncpy(buff, uiMsgString(stringId), 64);
			break;
		}
	}
	// show mesage
	uiShowPopup(player, buff, seconds);
}

void patchFlagUiPopup(void)
{
	flagPVars_t * flag = &midFlag.pRedFlag->pVar;
	u32 hook = GetAddress(&vaFlagEventUiPopup_Hook);

	// stop original function nulling  flag carrierId
	// *(u32*)(hook - 0x44) = 0;
	// hook our function.
	HOOK_JAL(hook, &patchFlagUiPopup_Logic);
}

void runMidFlag(void)
{
	// find and store flag moby address
	if (midFlag.setup == 0) {
		Moby *mobyStart = mobyListGetStart();
		Moby *mobyEnd = mobyListGetEnd();
		while (mobyStart < mobyEnd) {
			switch (mobyStart->oClass) {
				case MOBY_ID_CTF_RED_FLAG:
					midFlag.pRedFlag = mobyStart;
				case MOBY_ID_CTF_BLUE_FLAG:
					midFlag.pBlueFlag = mobyStart;
			}
			++mobyStart;
		}
		midFlag.setup = 1;
	}
	Moby *redFlag = midFlag.pRedFlag;
	Moby *blueFlag = midFlag.pBlueFlag;
	if ((Moby *)redFlag == 0)
		return;
	
	// get flag pVars
	flagPVars_t *red = (Moby *)redFlag->pVar;
	flagPVars_t *blue = (Moby *)((u32)redFlag->pVar + 0x30);
	// setup bases and find center spawn for flag
	if (midFlag.setup == 1) {
		VECTOR spMedianPosition = {0, 0, 0, 0};
		int centerSpawn = 0;
		// save capture cuboids
		midFlag.baseCuboid[0] = red->captureCuboid;
		midFlag.baseCuboid[1] = blue->captureCuboid;
		// swap unk_28
		red->unk_28 = blue->unk_28;
		// make red flag glow
		redFlag->primaryColor = 0xffffffff;
		// disable and hide blue flag
		blueFlag->modeBits |= MOBY_MODE_BIT_DISABLED | MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_UPDATE;
		// check if flag spawn override.
		int mapId = gameGetSettings()->GameLevel;
		int i = 0;
		for (i; i < COUNT_OF(midFlagPos); ++i) {
			if (mapId == midFlagPos[i].mapId) {
				spMedianPosition[0] = midFlagPos[i].x;
				spMedianPosition[1] = midFlagPos[i].z;
				spMedianPosition[2] = midFlagPos[i].y;
				// set centerspawn pointer.
				centerSpawn = &spMedianPosition;
				break;
			}
		}
		// if centerSpawn wasn't overided by manual coordinates, find center spawn.
		if (centerSpawn == 0) {
			vector_add(spMedianPosition, spMedianPosition, red->basePos);
			vector_add(spMedianPosition, spMedianPosition, blue->basePos);
			vector_scale(spMedianPosition, spMedianPosition, 0.5);
			int medianSpIdx = findClosestSpawnPointToPosition(spMedianPosition, 0);
			centerSpawn = &spawnPointGet(medianSpIdx)->pos;
		}
		// set flag spawn
		vector_copy(red->basePos, centerSpawn);
		vector_copy(redFlag->position, centerSpawn);
		redFlag->position[2] += .25;

		// hook flag event ui popup so we can use our own strings.
		// patchFlagUiPopup();

		midFlag.setup = 2;
	}

	Player *player;
	// Flag Logic
	// if flag not carried, set team to 3 so all teams can pick it up.
	if (red->carrierIdx == -1 && red->team != 2) {
		red->team = 2;
		red->captureCuboid = -1;
	}
	// if carried, set red flag team to opposite of player team.
	else if (red->carrierIdx > -1) {
		player = playerGetFromSlot(red->carrierIdx);
		// blue team captures: 1, red team captures: 0
		int mpTeam = !player->mpTeam;
		red->team = mpTeam;
		red->captureCuboid = midFlag.baseCuboid[mpTeam];
	}
	// Sprite Logic
	player = playerGetFromSlot(0);
	int isPaused = player->pauseOn;
	// draw sprite for non-flag holders.
	if (!player->flagMoby) {
		int i = red->carrierIdx;
		u32 color = i > -1 ? TEAM_COLORS[!red->team] : 0x00ffffff;
		VECTOR t;
		// if not carried, set sprite to flag position.
		if (i == -1) {
			vector_copy(t, redFlag->position);
			t[2] += 3;
			t[0] += .07;
		}
		// if carried, set sprite to player position.
		else {
			player = playerGetFromSlot(i);
			vector_copy(t, player->playerPosition);
			t[2] += 2;
			t[0] += .07;	
		}
		if (t != 0 && !isPaused)
			gfxHelperDrawSprite_WS(t, 24, 24, SPRITE_FLAG, 0x80000000 | color, TEXT_ALIGN_MIDDLECENTER);
	}
}

void runCTF(void) {

	Player *player = playerGetFromSlot(0);
	if (player->pauseOn == 0 && playerPadGetButtonDown(player, PAD_CIRCLE) > 0) {
		printf("\nr+b: %08x, %08x", midFlag.pRedFlag, midFlag.pBlueFlag);
		printf("\ngd: %08x", gameGetData()->CTFGameData);
	}

	GameSettings *gs = gameGetSettings();
	if (gs->GameType != GAMERULE_CTF)
		return;

	runMidFlag();
}
