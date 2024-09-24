#include <libuya/moby.h>
#include <libuya/player.h>
#include <libuya/guber.h>
#include <libuya/net.h>
#include <libuya/time.h>
#include <libuya/interop.h>
#include <libuya/stdio.h>
#include <libuya/string.h>

struct FlagPVars {
/* 0x00 */ VECTOR BasePosition;
/* 0x10 */ short CarrierIdx;
/* 0x12 */ short LastCarrierIdx;
/* 0x14 */ short Team;
/*      */ char UNK_16[6];
/* 0x1c */ int TimeFlagDropped;
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
	if (player->mpTeam == pvars->Team) {
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
	
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;
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
	struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;

	// if flag moby or pvars don't exist, stop.
	if (!flagMoby || !pvars)
		return;

    // if flag state is not 1 (being picked up) and if flag is returning to base
    if (flagMoby->state != 1 || flagIsReturning(flagMoby))
        return;
    // flag is being picked up
    if (flagIsBeingPickedUp(flagMoby))
        return;
    
	// return to base if flag has been idle for 40 seconds and not already at base.
	if ((pvars->TimeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby) && !flagIsAtBase(flagMoby)) {
		flagReturnToBase(flagMoby, 0, 0xff);
		return;
	}

    if ((pvars->TimeFlagDropped + (TIME_SECOND * 1.5)) > gameTime)
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
		if (player->ground.pMoby || player->ground.pMoby->oClass == MOBY_ID_TELEPORT_PAD)
			continue;

		// player must be within 2 units of flag
		vector_subtract(t, flagMoby->position, player->playerPosition);
		float sqrDistance = vector_sqrmag(t);
		if (sqrDistance > (2*2))
			continue;

		// player is on different team than flag and player isn't already holding flag
		if (player->mpTeam != pvars->Team) {
			if (!player->flagMoby) {
				flagRequestPickup(flagMoby, i);
				return;
			}
		} else {
			// if player is on same team as flag and close enough to return it
			vector_subtract(t, pvars->BasePosition, flagMoby->position);
			float sqrDistanceToBase = vector_sqrmag(t);
			if (sqrDistanceToBase > 0.1) {
				flagRequestPickup(flagMoby, i);
				return;
			}
		}
	}

    /* ===============1:1 Code: Doesn't work===========*/
    // if ((flagMoby->state == 1 && !flagIsReturning(flagMoby)) && (!flagIsBeingPickedUp(flagMoby) && (pvars->TimeFlagDropped + (TIME_SECOND * 1.5)) < gameTime)) {
    //     FLAG_AT_BASE = 1;
    //     // Return to base if flag has been idle for 40 seconds
    //     if ((pvars->TimeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
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
    //                                     if (player->mpTeam != pvars->Team) {
    //                                         if (!player->flagMoby) {
    //                                             flagRequestPickup(flagMoby, player->mpIndex);
    //                                             return;
    //                                         }
    //                                     } else {
    //                                         // if player is on same team as flag and close enough to return it
    //                                         vector_subtract(t, pvars->BasePosition, flagMoby->position);
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
	// struct FlagPVars* pvars = (struct FlagPVars*)flagMoby->pVar;
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
	// if ((pvars->TimeFlagDropped + (TIME_SECOND * 40)) < gameTime && !flagIsAtBase(flagMoby)) {
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
	// if ((pvars->TimeFlagDropped + 1500) > gameTime)
	// 	return;

	// for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
	// 	Player* player = players[i];
	// 	if (!player)
	// 		continue;

	// 	// only allow actions by living players, and non-chargebooting players
	// 	if ((playerIsDead(player) || playerGetHealth(player) <= 0) || (player->timers.IsChargebooting == 1 && (playerPadGetButton(player, PAD_R2) > 0) && player->timers.state > 55)){
	// 		// if flag holder died, update flagIgnorePlayer time.
	// 		if (pvars->LastCarrierIdx == player->mpIndex)
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
	// 	if (player->ground.pMoby || player->ground.pMoby->oClass == MOBY_ID_TELEPORT_PAD)
	// 		continue;

	// 	// player must be within 2 units of flag
	// 	vector_subtract(t, flagMoby->position, player->playerPosition);
	// 	float sqrDistance = vector_sqrmag(t);
	// 	if (sqrDistance > (2*2))
	// 		continue;

	// 	// player is on different team than flag and player isn't already holding flag
	// 	if (player->mpTeam != pvars->Team) {
	// 		if (!player->flagMoby) {
	// 			flagRequestPickup(flagMoby, i);
	// 			return;
	// 		}
	// 	} else {
	// 		// if player is on same team as flag and close enough to return it
	// 		vector_subtract(t, pvars->BasePosition, flagMoby->position);
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
