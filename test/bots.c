#include <tamtypes.h>
#include <string.h>

#include <libuya/time.h>
#include <libuya/game.h>
#include <libuya/gamesettings.h>
#include <libuya/player.h>
#include <libuya/camera.h>
#include <libuya/weapon.h>
#include <libuya/sha1.h>
#include <libuya/ui.h>
#include <libuya/stdio.h>
#include <libuya/graphics.h>
#include <libuya/net.h>
#include <libuya/pad.h>
#include <libuya/uya.h>
#include <libuya/utils.h>


typedef struct SimulatedPlayer {
	struct PAD Pad;
	// struct TrainingTargetMobyPVar Vars;
	Player* Player;
	u32 TicksToRespawn;
	u32 TicksToJump;
	u32 TicksToJumpFor;
	u32 TicksToStrafeSwitch;
	u32 TicksToStrafeQuickSwitchFor;
	u32 TicksToStrafeStop;
	u32 TicksToStrafeStopFor;
	u32 TicksToStrafeStoppedFor;
	u32 TicksToFire;
	u32 TicksFireDelay;
	u32 TicksToCycle;
	u32 TicksToAimYaw;
	u32 TicksToAimPitch;
	u32 TicksToTbag;
	u32 TicksAimNearPlayer;
	int SniperFireStopForTicks;
	int StrafeDir;
	int Idx;
	int CycleIdx;
	int Points;
	float YawOff;
	float Yaw;
	float YawVel;
	float YawAcc;
	float PitchOff;
	float Pitch;
	float PitchVel;
	float PitchAcc;
	char Created;
	char Active;
} SimulatedPlayer_t;

typedef struct TrainingState
{
	int InitializedTime;
	int Points;
	int Kills;
	int Hits;
	int ShotsFired;
	int BestCombo;
	int TargetsDestroyed;
	int GameOver;
	int WinningTeam;
	int IsHost;
	// enum TRAINING_TYPE TrainingType;
	int TargetLastSpawnIdx;
	int ComboCounter;
	long TimeLastSpawn;
	long TimeLastKill;
} TrainingState_t;

TrainingState_t State;
SimulatedPlayer_t SimPlayers[];

const int SimPlayerCount = 1;
const int TargetTeam = 1; // Red Team
int BotsInit = 0;

void SpawnBots(void)
{
	int i;
	// handle game end logic
	if (gameAmIHost())
	{
		// keep spawning
		for (i = 0; i < SimPlayerCount; ++i)
		{
			if (!SimPlayers[i].Active || playerIsDead(SimPlayers[i].Player))
				spawnTarget(&SimPlayers[i]);
		}
	}
}

void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx)
{
	int id = idx + 1;
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
	
	// HOOK_JAL(0x00610e38, &fixSimPlayerNetPlayerPtr);

	// reset
	memset(sPlayer, 0, sizeof(SimulatedPlayer_t));
	
	// spawn player
	memcpy(&sPlayer->Pad, players[0]->Paddata, sizeof(struct PAD));
	
	sPlayer->Pad.rdata[7] = 0x7F;
	sPlayer->Pad.rdata[6] = 0x7F;
	sPlayer->Pad.rdata[5] = 0x7F;
	sPlayer->Pad.rdata[4] = 0x7F;
	sPlayer->Pad.rdata[3] = 0xFF;
	sPlayer->Pad.rdata[2] = 0xFF;
	sPlayer->Pad.port = 10;
	sPlayer->Pad.slot = 10;
	sPlayer->Pad.id = id;
	
	// create hero
	gameSettings->PlayerClients[id] = 0;
	gameSettings->PlayerSkins[id] = 0;
	gameSettings->PlayerTeams[id] = TargetTeam;

    // local hero (Outpost x12)
    // Breakpoint on what writes to top of player struct
	((void (*)(int))0x00528c30)(id);

	Player * newPlayer = players[id];
	sPlayer->Player = newPlayer;

	players[id] = sPlayer->Player;

	// POKE_U32(0x0021DDDC + (4 * idx), 0);

	sPlayer->Player->Paddata = (void*)&sPlayer->Pad;
	sPlayer->Player->PlayerId = id;
	sPlayer->Player->MpIndex = id;
	sPlayer->Player->Team = TargetTeam;
	sPlayer->Idx = idx;
	// POKE_U32((u32)sPlayer->Player + 0x1AA0, (u32)sPlayer->Player);

	// if (sPlayer->Player->PlayerMoby) {
	// 	sPlayer->Player->PlayerMoby->NetObject = sPlayer->Player;
	// 	*(u32*)((u32)sPlayer->Player->PlayerMoby->PVar + 0xE0) = (u32)sPlayer->Player;
	// }

	sPlayer->Active = 0;
	sPlayer->Created = 1;
}

void spawnTarget(SimulatedPlayer_t* sPlayer)
{
	GameSettings* gs = gameGetSettings();

	if (!sPlayer)
		return;

	// create
	if (!sPlayer->Created)
		createSimPlayer(sPlayer, sPlayer->Idx);

	// destroy existing
	// if (sPlayer->Active)
	// 	modeOnTargetKilled(sPlayer, 0);

	// playerRespawn(sPlayer->Player);
	// modeInitTarget(sPlayer);
}

void onSimulateHeros(void)
{
	int i;


	// update local hero first
	Player * lPlayer = (Player*)PLAYER_STRUCT;
	if (lPlayer) {
		PlayerVTable* pVTable = playerGetVTable(lPlayer);

		pVTable->Update(lPlayer); // update hero
	}

	// update remote clients
	for (i = 0; i < SimPlayerCount; ++i) {
		if (SimPlayers[i].Player) {
			PlayerVTable * pVTable = playerGetVTable(SimPlayers[i].Player);

			// targetUpdate(&SimPlayers[i]);

			// // process input
			// ((void (*)(struct PAD*))0x00527e08)(&SimPlayers[i].Pad);

			// // update pad
			// ((void (*)(struct PAD*, void*, int))0x00527510)(&SimPlayers[i].Pad, SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			pVTable->Update(SimPlayers[i].Player);
		}
	}
}

void InitBots(void)
{
	Player** players = playerGetAll();
	int i;

	// give a small delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 15;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimulatedPlayer_t) * SimPlayerCount);

	HOOK_JAL(0x003D29F0, onSimulateHeros);

	for (i = 0; i < SimPlayerCount; ++i) {
		SimPlayers[i].Idx = i;
	}

	BotsInit = 1;
}

void StartBots(void)
{
	if (!BotsInit)
	{
		InitBots();
	}
	SpawnBots();
}