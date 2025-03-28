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

struct flagPVars {
/* 0x00 */ VECTOR basePos;
/* 0x10 */ short carrierIdx;
/* 0x12 */ short prevCarrierIdx;
/* 0x14 */ short team;
/* 0x16 */ char unk_16[2];
/* 0x18 */ int captureCuboid; 
/* 0x1c */ int timeFlagDropped;
/* 0x20 */ int unk_20;
/* 0x24 */ int unk_24;
/* 0x28 */ float unk_28;
/* 0x2c */ int unk_2c;
};

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

VariableAddress_t vaSpawnPart_059 = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004a0508,
    .Hoven = 0x004a2620,
    .OutpostX12 = 0x00497ef8,
    .KorgonOutpost = 0x00495690,
    .Metropolis = 0x004949e0,
    .BlackwaterCity = 0x00492278,
    .CommandCenter = 0x00492270,
    .BlackwaterDocks = 0x00494af0,
    .AquatosSewers = 0x00493df0,
    .MarcadiaPalace = 0x00493770,
#else
    .Lobby = 0,
    .Bakisi = 0x0049e150,
    .Hoven = 0x004a01a8,
    .OutpostX12 = 0x00495ac0,
    .KorgonOutpost = 0x004932d8,
    .Metropolis = 0x00492628,
    .BlackwaterCity = 0x0048fe40,
    .CommandCenter = 0x0048fff8,
    .BlackwaterDocks = 0x00492838,
    .AquatosSewers = 0x00491b78,
    .MarcadiaPalace = 0x004914b8,
#endif
};

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
	
	struct flagPVars* pvars = (struct flagPVars*)flagMoby->pVar;
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
	
	struct flagPVars* pvars = (struct flagPVars*)flagMoby->pVar;
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

struct PartInstance * flagSpawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))GetAddress(&vaSpawnPart_059))(position, color, opacity, 53, 0, 2, 0, 0, 1.5 + (0.5 * idx));
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
			particle = flagSpawnParticle(particlePosition, color, 100, i);
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
	struct flagPVars* pvars = (struct flagPVars*)flagMoby->pVar;

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
	// struct flagPVars* pvars = (struct flagPVars*)flagMoby->pVar;
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

    struct SpawnPoint* sp = spawnPointGet(i);
    VECTOR delta;
    vector_subtract(delta, position, &sp->M0[12]);
    float sqrDist = vector_sqrmag(delta);
    if (sqrDist < bestDist && sqrDist >= sqrDeadzone) {
      bestDist = sqrDist;
      bestIdx = i;
    }
  }

  return bestIdx;
}

enum flagIndex {
	FLAG_RED = 0,
	FLAG_BLUE = 1
};

typedef struct flagPositions {
	short mapId;
	short flag;
	VECTOR pos;
} flagPositions_t;

flagPositions_t midFlag_Flags[] = {
	{MAP_ID_HOVEN, FLAG_RED, 0, 0, 0}
};

typedef struct midFlagInfo {
	int setup;
	Moby *pRedFlag;
	Moby *pBlueFlag;
	short baseCuboid[2];
} midFlagInfo_t;

typedef struct ctfinfo {
	midFlagInfo_t midFlag;
} ctfInfo_t;
ctfInfo_t ctf;

void runMidFlag(void)
{
	// find and store flag moby address
	if (ctf.midFlag.setup == 0) {
		Moby *mobyStart = mobyListGetStart();
		Moby *mobyEnd = mobyListGetEnd();
		while (mobyStart < mobyEnd) {
			switch (mobyStart->oClass) {
				case MOBY_ID_CTF_RED_FLAG:
					ctf.midFlag.pRedFlag = mobyStart;
				case MOBY_ID_CTF_BLUE_FLAG:
					ctf.midFlag.pBlueFlag = mobyStart;
			}
			++mobyStart;
		}
		ctf.midFlag.setup = 1;
	}
	Moby *redFlag = ctf.midFlag.pRedFlag;
	Moby *blueFlag = ctf.midFlag.pBlueFlag;
	VECTOR v;
	VECTOR spawnFlag = {259.964, 253.611, 62.598, 0};
	if ((Moby *)redFlag == 0)
		return;
	
	// get flag pVars
	struct flagPVars *red = (Moby *)redFlag->pVar;
	struct flagPVars *blue = (Moby *)((u32)redFlag->pVar + 0x30);
	// setup bases and find center spawn for flag
	if (ctf.midFlag.setup == 1) {
		// save capture cuboids
		ctf.midFlag.baseCuboid[0] = red->captureCuboid;
		ctf.midFlag.baseCuboid[1] = blue->captureCuboid;
		// swap unk_28
		red->unk_28 = blue->unk_28;

		// make red flag glow
		redFlag->primaryColor = 0xffffffff;

		// disable and hide blue flag
		blueFlag->modeBits |= MOBY_MODE_BIT_DISABLED | MOBY_MODE_BIT_HIDDEN | MOBY_MODE_BIT_NO_UPDATE;

		// determine ball spawn
		// as closest player spawn
		// to median of bases
		VECTOR spMedianPosition = {0,0,0,0};
		vector_add(spMedianPosition, spMedianPosition, red->basePos);
		vector_add(spMedianPosition, spMedianPosition, blue->basePos);
		vector_scale(spMedianPosition, spMedianPosition, 0.5);
		int medianSpIdx = findClosestSpawnPointToPosition(spMedianPosition, 0);

		// set spawn point position for center spawn, and base spawn
		int centerSpawn = &spawnPointGet(medianSpIdx)->M0[12];
		vector_copy(red->basePos, centerSpawn);
		vector_copy(redFlag->position, centerSpawn);
		redFlag->position[2] += .75;

		ctf.midFlag.setup = 2;
	}

	// Flag Logic
	// if flag not carried, set team to 3 so all teams can pick it up.
	if (red->carrierIdx == -1 && red->team != 2) {
		red->team = 2;
		red->captureCuboid = -1;
	}
	// if carried, set red flag team to opposite of player team.
	else if (red->carrierIdx > -1) {
		Player *player = playerGetFromSlot(red->carrierIdx);
		// blue team captures: 1, red team captures: 0
		int mpTeam = !player->mpTeam;
		red->team = mpTeam;
		red->captureCuboid = ctf.midFlag.baseCuboid[mpTeam];
	}
}

void runCTF(void) {

	Player *player = playerGetFromSlot(0);
	if (player->pauseOn == 0 && playerPadGetButtonDown(player, PAD_CIRCLE) > 0) {
		printf("\nr+b: %08x, %08x", ctf.midFlag.pRedFlag, ctf.midFlag.pBlueFlag);
		printf("\ngd: %08x", gameGetData()->CTFGameData);
	}

	GameSettings *gs = gameGetSettings();
	if (gs->GameType != GAMERULE_CTF)
		return;

	runMidFlag();
}
