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
#include <libuya/interop.h>

VariableAddress_t vaPadProcessInput = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x00496658,
	.Hoven = 0x00498770,
	.OutpostX12 = 0x0048e048,
    .KorgonOutpost = 0x0048b718,
	.Metropolis = 0x0048ab30,
	.BlackwaterCity = 0x004883c8,
	.CommandCenter = 0x004883c0,
    .BlackwaterDocks = 0x0048ac40,
    .AquatosSewers = 0x00489f40,
    .MarcadiaPalace = 0x004898c0,
#else
	.Lobby = 0,
	.Bakisi = 0x00494460,
	.Hoven = 0x004964b8,
	.OutpostX12 = 0x0048bdd0,
    .KorgonOutpost = 0x00489520,
	.Metropolis = 0x00488938,
	.BlackwaterCity = 0x00486150,
	.CommandCenter = 0x00486308,
    .BlackwaterDocks = 0x00488b48,
    .AquatosSewers = 0x00487e88,
    .MarcadiaPalace = 0x004877c8,
#endif
};

VariableAddress_t vaPadProcessAddr = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x00496a78,
	.Hoven = 0x00498b90,
	.OutpostX12 = 0x0048e468,
    .KorgonOutpost = 0x0048bb38,
	.Metropolis = 0x0048af50,
	.BlackwaterCity = 0x004887e8,
	.CommandCenter = 0x004887e0,
    .BlackwaterDocks = 0x0048b060,
    .AquatosSewers = 0x0048a360,
    .MarcadiaPalace = 0x00489ce0,
#else
	.Lobby = 0,
	.Bakisi = 0x00494880,
	.Hoven = 0x004968d8,
	.OutpostX12 = 0x0048c1f0,
    .KorgonOutpost = 0x00489940,
	.Metropolis = 0x00488d58,
	.BlackwaterCity = 0x00486570,
	.CommandCenter = 0x00486728,
    .BlackwaterDocks = 0x00488f68,
    .AquatosSewers = 0x004882a8,
    .MarcadiaPalace = 0x00487be8,
#endif
};

VariableAddress_t vaPadProcessValue = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x0c125996,
	.Hoven = 0x0c1261dc,
	.OutpostX12 = 0x0c123812,
    .KorgonOutpost = 0x0c122dc6,
	.Metropolis = 0x0c122acc,
	.BlackwaterCity = 0x0c1220f2,
	.CommandCenter = 0x0c1220f0,
    .BlackwaterDocks = 0x0c122b10,
    .AquatosSewers = 0x0c1227d0,
    .MarcadiaPalace = 0x0c122630,
#else
	.Lobby = 0,
	.Bakisi = 0x0c125118,
	.Hoven = 0x0c12592e,
	.OutpostX12 = 0x0c122f74,
    .KorgonOutpost = 0x0c122548,
	.Metropolis = 0x0c12224e,
	.BlackwaterCity = 0x0c121854,
	.CommandCenter = 0x0c1218c2,
    .BlackwaterDocks = 0x0c1222d2,
    .AquatosSewers = 0x0c121fa2,
    .MarcadiaPalace = 0x0c121df2,
#endif
};

struct TrainingTargetMobyPVar
{
	VECTOR SpawnPos;
	VECTOR Velocity;
	int State;
	u32 LifeTicks;
	int TimeCreated;
	float Jitter;
	float StrafeSpeed;
	float JumpSpeed;
};

typedef struct SimulatedPlayer {
	struct PAD Pad;
	struct TrainingTargetMobyPVar Vars;
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

const int SimPlayerCount = 3;
const int TargetTeam = 1; // Red Team
static int BotsInit = 0;

void modeInitTarget(SimulatedPlayer_t * sPlayer)
{
	GameSettings * gs = gameGetSettings();

	// init vars
	struct TrainingTargetMobyPVar * pvar = &sPlayer->Vars;

	sPlayer->Active = 1;

	sprintf(gs->PlayerNames[sPlayer->Player->PlayerId], "Fake %d", sPlayer->Idx);
}

void SpawnBots(void)
{
	int i;

	for (i = 0; i < SimPlayerCount; ++i)
	{
		if (!SimPlayers[i].Active || playerIsDead(SimPlayers[i].Player))
		{
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

	POKE_U32(0x002412f4 + (4 * id), (void*)&sPlayer->Pad);

	// local hero (Outpost x12)
	((void (*)(int))0x00528c30)(id);

	Player * newPlayer = players[id];
	sPlayer->Player = newPlayer;

	players[id] = sPlayer->Player;

	POKE_U32(0x002412f4 + (4 * id), 0);

	sPlayer->Player->Paddata = (void*)&sPlayer->Pad;
	sPlayer->Player->PlayerId = id;
	sPlayer->Player->MpIndex = id;
	sPlayer->Player->Team = TargetTeam;
	sPlayer->Idx = idx;
	POKE_U32(sPlayer->Player->TopOfPlayerStruct, (u32)sPlayer->Player);

	// if (sPlayer->Player->PlayerMoby) {
	// 	sPlayer->Player->PlayerMoby->NetObject = sPlayer->Player;
	// 	*(u32*)((u32)sPlayer->Player->PlayerMoby->PVar + 0x70) = (u32)sPlayer->Player;
	// }

	sPlayer->Active = 0;
	sPlayer->Created = 1;
}

void targetUpdate(SimulatedPlayer_t *sPlayer)
{
	Player* player = (Player*)PLAYER_STRUCT;
	int isOwner = gameAmIHost();
	if (!sPlayer || !player || !sPlayer->Active)
		return;

	Player* target = sPlayer->Player;
	Moby* targetMoby = target->PlayerMoby;

	// reset pad
	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
	pad->btns = 0xFFFF;
	pad->ljoy_h = 0x7f;
	pad->ljoy_v = 0x7f;
	pad->circle_p = 0;
	pad->cross_p = 0;
	pad->square_p = 0;
	pad->triangle_p = 0;
	pad->down_p = 0;
	pad->left_p = 0;
	pad->right_p = 0;
	pad->up_p = 0;
	pad->l1_p = 0;
	pad->l2_p = 0;
	pad->r1_p = 0;
	pad->r2_p = 0;
}

void spawnTarget(SimulatedPlayer_t* sPlayer)
{
	if (!sPlayer)
		return;

	// create
	if (!sPlayer->Created)
		createSimPlayer(sPlayer, sPlayer->Idx);

	// destroy existing
	if (sPlayer->Active)
		playerSetHealth(sPlayer, 0);

	// playerRespawn(sPlayer->Player);
	modeInitTarget(sPlayer);
}

void UpdateBots(void)
{
	int i;
	for (i = 0; i < SimPlayerCount; ++i)
	{
		if (playerIsDead(SimPlayers[i].Player))
		{
			updateTarget(&SimPlayers[i]);
		}
	}
}

void updateTarget(SimulatedPlayer_t* sPlayer)
{
	if (!sPlayer)
		return;

	// Respawn
	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
	// PadButtonStatus * pad = (PadButtonStatus*)sPlayer->Player->Paddata;
	int jumping = 0;
	if (!jumping)
	{
		pad->btns &= ~PAD_CROSS;
		jumping = 1;
	}
		
	// playerRespawn(sPlayer->Player);
}

void onSimulateHeros(void)
{
	int i;

	// update local hero first
	Player * lPlayer = (Player*)PLAYER_STRUCT;
	if (lPlayer) {
		PlayerVTable * pVTable = playerGetVTable(lPlayer);
		// HUD JAL: 004E3F14
		pVTable->Update(lPlayer); // update hero
	}

	// update remote clients
	for (i = 0; i < SimPlayerCount; ++i) {
		if (SimPlayers[i].Player) {
			PlayerVTable * pVTable = playerGetVTable(SimPlayers[i].Player);

			// targetUpdate(&SimPlayers[i]);

			// process input (Correct Address though)
			// Causes syscall (0) error!
			// ((void (*)(u32))GetAddress(&vaPadProcessInput))(&SimPlayers[i].Pad);

			// update pad (THIS IS CORRECT! :D)
			// Bakisi: 0x00493a68
			((void (*)(int, int, int))0x0048b3d8)(&SimPlayers[i].Pad, &SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			// pVTable->Update(SimPlayers[i].Player);
		}
	}
}

void InitBots(void)
{
	int i;

	// give a small delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 15;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimulatedPlayer_t) * SimPlayerCount);

	// run update functions for SimHeroes
	HOOK_J(0x004E3FC0, onSimulateHeros); // This one seems to work well.
	// HOOK_JAL(0x003D29F0, onSimulateHeros); // This one removes hud and such.

	// Stop player update functions.
	int Addr = 0x004E3EA8;
	int Value = 0x1860000E;
	if (*(u32*)Addr == Value)
		*(u32*)Addr = 0x1000000E;

	for (i = 0; i < SimPlayerCount; ++i) {
		SimPlayers[i].Idx = i;
	}

	BotsInit = 1;

	SpawnBots();
}

void StartBots(void)
{
	// How Many Local Players
	// *(u32*)0x001a5e5c = 3;

	if (!BotsInit && isInGame())
	 	InitBots();

	if (BotsInit && isInGame())
	{
		// UpdateBots();
	}
	
	// Stop player update functions
	// POKE_U32(0x005CD458, 0);
	// POKE_U32(0x005CD464, 0x90822FEA);
}