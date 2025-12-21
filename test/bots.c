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
#include <libuya/math3d.h>
#include "training.h"

#define BOT_PAD_LOCATION ((u32)PAD_POINTER + 0xc)

void modeProcessPlayer(int pIndex);
void modeTick(void);
void modeInitTarget(SimulatedPlayer_t *sPlayer);
void modeUpdateTarget(SimulatedPlayer_t *sPlayer);

VariableAddress_t vaInit_RunUpdateFunctions = {
#if UYA_PAL
	.Lobby = 0x0061ce78,
	.Bakisi = 0x004eed70,
	.Hoven = 0x004f0e88,
	.OutpostX12 = 0x004e6760,
	.KorgonOutpost = 0x004e3ef8,
	.Metropolis = 0x004e3248,
	.BlackwaterCity = 0x004e0ae0,
	.CommandCenter = 0x004e0aa8,
	.BlackwaterDocks = 0x004e3328,
	.AquatosSewers = 0x004e2628,
	.MarcadiaPalace = 0x004e1fa8,
#else
	.Lobby = 0x0061a700,
	.Bakisi = 0x004ec650,
	.Hoven = 0x004ee6a8,
	.OutpostX12 = 0x004e3fc0,
	.KorgonOutpost = 0x004e17d8,
	.Metropolis = 0x004e0b28,
	.BlackwaterCity = 0x004de340,
	.CommandCenter = 0x004de4c8,
	.BlackwaterDocks = 0x004e0d08,
	.AquatosSewers = 0x004e0048,
	.MarcadiaPalace = 0x004df988,
#endif
};

VariableAddress_t vaInit_StopPlayerUpdateFunctions = {
#if UYA_PAL
	.Lobby = 0x0061cd60,
	.Bakisi = 0x004eec58,
	.Hoven = 0x004f0d70,
	.OutpostX12 = 0x004e6648,
	.KorgonOutpost = 0x004e3de0,
	.Metropolis = 0x004e3130,
	.BlackwaterCity = 0x004e09c8,
	.CommandCenter = 0x004e0990,
	.BlackwaterDocks = 0x004e3210,
	.AquatosSewers = 0x004e2510,
	.MarcadiaPalace = 0x004e1e90,
#else
	.Lobby = 0x0061a5e8,
	.Bakisi = 0x004ec538,
	.Hoven = 0x004ee590,
	.OutpostX12 = 0x004e3ea8,
	.KorgonOutpost = 0x004e16c0,
	.Metropolis = 0x004e0a10,
	.BlackwaterCity = 0x004de228,
	.CommandCenter = 0x004de3b0,
	.BlackwaterDocks = 0x004e0bf0,
	.AquatosSewers = 0x004dff30,
	.MarcadiaPalace = 0x004df870,
#endif
};

VariableAddress_t vaCreateHero = {
#if UYA_PAL
	.Lobby = 0x00661a40,
	.Bakisi = 0x00533b40,
	.Hoven = 0x00535c58,
	.OutpostX12 = 0x0052b530,
	.KorgonOutpost = 0x00528cc8,
	.Metropolis = 0x00528018,
	.BlackwaterCity = 0x005258b0,
	.CommandCenter = 0x00525670,
	.BlackwaterDocks = 0x00527ef0,
	.AquatosSewers = 0x005271f0,
	.MarcadiaPalace = 0x00526b70,
#else
	.Lobby = 0x0065f168,
	.Bakisi = 0x005312c0,
	.Hoven = 0x00533318,
	.OutpostX12 = 0x00528c30,
	.KorgonOutpost = 0x00526448,
	.Metropolis = 0x00525798,
	.BlackwaterCity = 0x00522fb0,
	.CommandCenter = 0x00522f30,
	.BlackwaterDocks = 0x00525770,
	.AquatosSewers = 0x00524ab0,
	.MarcadiaPalace = 0x005243f0,
#endif
};

VariableAddress_t vaPAD_PadUpdate = {
#if UYA_PAL
	.Lobby = 0x005c34a8,
	.Bakisi = 0x00495c60,
	.Hoven = 0x00497d78,
	.OutpostX12 = 0x0048d650,
	.KorgonOutpost = 0x0048ad20,
	.Metropolis = 0x0048a138,
	.BlackwaterCity = 0x004879d0,
	.CommandCenter = 0x004879c8,
	.BlackwaterDocks = 0x0048a248,
	.AquatosSewers = 0x00489548,
	.MarcadiaPalace = 0x00488ec8,
#else
	.Lobby = 0x005c1258,
	.Bakisi = 0x00493a68,
	.Hoven = 0x00495ac0,
	.OutpostX12 = 0x0048b3d8,
	.KorgonOutpost = 0x00488b28,
	.Metropolis = 0x00487f40,
	.BlackwaterCity = 0x00485758,
	.CommandCenter = 0x00485910,
	.BlackwaterDocks = 0x00488150,
	.AquatosSewers = 0x00487490,
	.MarcadiaPalace = 0x00486dd0,
#endif
};

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

int Initialized = 0;

extern int TargetTeam;
extern const int SimPlayerCount;
extern SimulatedPlayer_t SimPlayers[];

const char* TRAINING_AGGRO_NAMES[] = {
	[TRAINING_AGGRESSION_AGGRO] "AGGRO",
	[TRAINING_AGGRESSION_AGGRO_NO_DAMAGE] "NO DAMAGE",
	[TRAINING_AGGRESSION_PASSIVE] "PASSIVE",
	[TRAINING_AGGRESSION_IDLE] "IDLE",
};

void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx)
{
	int id = idx + 1;
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
	
	// HOOK_JAL(0x00610e38, &fixSimPlayerNetPlayerPtr);

	// reset
	memset(sPlayer, 0, sizeof(SimulatedPlayer_t));
	
	// spawn player
	memcpy(&sPlayer->Pad, players[0]->pPad, sizeof(struct PAD));
	
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

	POKE_U32(BOT_PAD_LOCATION + (4 * id), (void*)&sPlayer->Pad);

	// local hero
	((void (*)(int))GetAddress(&vaCreateHero))(id);

	Player * newPlayer = players[id];
	sPlayer->Player = newPlayer;

	players[id] = sPlayer->Player;

	POKE_U32(BOT_PAD_LOCATION + (4 * id), 0);

	sPlayer->Player->pPad = (void*)&sPlayer->Pad;
	sPlayer->Player->fps.vars.cam_slot = id;
	sPlayer->Player->mpIndex = id;
	sPlayer->Player->mpTeam = TargetTeam;
	sPlayer->Idx = idx;
	POKE_U32(sPlayer->Player->fps.vars.pHero, (u32)sPlayer->Player);

	// if (sPlayer->Player->pMoby) {
	// 	sPlayer->Player->pMoby->NetObject = sPlayer->Player;
	// 	*(u32*)((u32)sPlayer->Player->pMoby->PVar + 0x70) = (u32)sPlayer->Player;
	// }

	sPlayer->Active = 0;
	sPlayer->Created = 1;
}

void targetUpdate(SimulatedPlayer_t *sPlayer)
{
	Player* player = playerGetFromSlot(0);
	int isOwner = gameAmIHost();
	if (!sPlayer || !player || !sPlayer->Active)
		return;

	Player* target = sPlayer->Player;
	Moby* targetMoby = target->pMoby;

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

	// send to mode for custom logic
	modeUpdateTarget(sPlayer);
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
		playerSetHealth(sPlayer->Player, 0);

	playerRespawn(sPlayer->Player);
	modeInitTarget(sPlayer);
}

void onSimulateHeros(void)
{
	int i;

	// update local hero first
	// Player * lPlayer = playerGetFromSlot(0);
	// if (lPlayer) {
	// 	PlayerVTable * pVTable = playerGetVTable(lPlayer);
	// 	// HUD JAL: 004E3F14
	// 	pVTable->Update(lPlayer); // update hero
	// }

	// update remote clients
	for (i = 0; i < SimPlayerCount; ++i) {
		if (SimPlayers[i].Player) {
			PlayerVTable * pVTable = playerGetVTable(SimPlayers[i].Player);

			targetUpdate(&SimPlayers[i]);

			// process input (Correct Address though)
			// ((void (*)(u32))GetAddress(&vaPadProcessInput))(&SimPlayers[i].Pad);

			// update pad (THIS IS CORRECT! :D)
			((void (*)(int, int, int))GetAddress(&vaPAD_PadUpdate))(&SimPlayers[i].Pad, &SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			// pVTable->Update(SimPlayers[i].Player);
		}
	}
}

void InitBots(void)
{
	int i;

	// Stop player update functions.
	// Lets us control when to update players
	// int Addr = GetAddressImmediate(&vaInit_StopPlayerUpdateFunctions);
	// int Value = 0x1860000E;
	// if (*(u32*)Addr == Value)
	// 	*(u32*)Addr = 0x1000000E;


	modeInitialize();

	// give a small delay before finalizing the initialization.
	// this helps prevent the slow loaders from desyncing
	static int startDelay = 15;
	if (startDelay > 0) {
		--startDelay;
		return;
	}

	memset(SimPlayers, 0, sizeof(SimulatedPlayer_t) * SimPlayerCount);
	memset(&State, 0, sizeof(State));

	State.TimeLastKill = timerGetSystemTime();
	State.InitializedTime = gameGetTime();

	// run update functions for SimHeroes
	HOOK_J(GetAddressImmediate(&vaInit_RunUpdateFunctions), onSimulateHeros); // This one seems to work well.
	// HOOK_JAL(0x003D29F0, onSimulateHeros); // This one removes hud and such.

	for (i = 0; i < SimPlayerCount; ++i) {
		SimPlayers[i].Idx = i;
	}

	Initialized = 1;
}

void frameTick(void)
{
	int i = 0;
	char buf[32];
	int nowTime = gameGetTime();
	GameData* gameData = gameGetData();
	GameOptions* gameOptions = gameGetOptions();

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		modeProcessPlayer(i);
	}

	// draw counter
	//snprintf(buf, 32, "%d/%d", State.TargetsDestroyed, TARGET_GOAL);
	//gfxScreenSpaceText(15, SCREEN_HEIGHT - 15, 1, 1, 0x80FFFFFF, buf, -1, 6);

	// compute time left
	// float secondsLeft = ((gameOptions->GameFlags.MultiplayerGameFlags.Timelimit * TIME_MINUTE) - (nowTime - gameData->TimeStart)) / (float)TIME_SECOND;
	// if (secondsLeft < 0)
	// 	secondsLeft = 0;
	// int secondsLeftInt = (int)secondsLeft;
	// float timeSecondsRounded = secondsLeftInt;
	// if ((secondsLeft - timeSecondsRounded) > 0.5)
	// 	timeSecondsRounded += 1;

	// draw timer
	// if (shouldDrawHud()) {
	// 	sprintf(buf, "%02d:%02d", secondsLeftInt/60, secondsLeftInt%60);
	// 	gfxScreenSpaceText(479+1, 57+1, 0.8, 0.8, 0x80000000, buf, -1, 1);
	// 	gfxScreenSpaceText(479, 57, 0.8, 0.8, 0x80FFFFFF, buf, -1, 1);
	// }

	modeTick();
}

void gameTick(void)
{
	int i;
	for (i = 0; i < SimPlayerCount; ++i) {
		if (!SimPlayers[i].Active || playerIsDead(SimPlayers[i].Player)) {
			spawnTarget(&SimPlayers[i]);
			// if (!playerGetRespawnTimer(SimPlayers[i].Player)) {
			// 	spawnTarget(&SimPlayers[i]);
			// }
		}
	}
}

void StartBots(void) // gameStart
{
	// How Many Local Players
	// *(u32*)0x001a5e5c = 3;

	// Ensure in game
	GameSettings * gameSettings = gameGetSettings();
	if (!gameSettings || !isInGame())
		return;

	if (!Initialized) {
		InitBots(); // initialize()
		return;
	}

	frameTick();
	gameTick();
}
