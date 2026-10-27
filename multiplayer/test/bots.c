#include <tamtypes.h>
#include <stddef.h>
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

typedef enum botTestMode {
	BOT_TEST_IDLE,
	BOT_TEST_PATROL,
	BOT_TEST_VEHICLES
} botTestMode_e;

typedef char botVehicleMobyOffsetCheck[(offsetof(Vehicle, pMoby) == 0x2d0) ? 1 : -1];
typedef char botVehicleDriverOffsetCheck[(offsetof(Vehicle, pDriver) == 0x2d8) ? 1 : -1];
typedef char botVehiclePassengerOffsetCheck[(offsetof(Vehicle, pPassenger) == 0x2dc) ? 1 : -1];
typedef char botVehicleHealthOffsetCheck[(offsetof(Vehicle, hitPoints) == 0x308) ? 1 : -1];
typedef char botVehicleSizeCheck[(sizeof(Vehicle) == 0x340) ? 1 : -1];

void FlushCache(int mode);
void modeInitialize(void);
Player **playerGetAll(void);
PlayerVTable *playerGetVTable(Player *player);
void modeProcessPlayer(int pIndex);
void modeTick(void);
void modeInitTarget(SimulatedPlayer_t *sPlayer);
void modeUpdateTarget(SimulatedPlayer_t *sPlayer);
void modeUpdateSpectatePatrol(SimulatedPlayer_t *sPlayer);
static void updateBotTestMode(void);
static void setBotLockStrafe(Player *player);

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

/* PNACH reapplies .data every frame; mutable state must live in BSS. */
int Initialized;
static int botsStartTicks;
static botTestMode_e botTestMode;

/* Vehicle-seat setup is verified only for NTSC-U Bakisi. Other overlays
 * retain the patrol test instead of calling unverified native addresses. */
static VariableAddress_t botGetVehicle = {
#ifndef UYA_PAL
	.Bakisi = 0x00536808,
#endif
};
static VariableAddress_t botAssignDriver = {
#ifndef UYA_PAL
	.Bakisi = 0x00536dc0,
#endif
};
static VariableAddress_t botAssignPassenger = {
#ifndef UYA_PAL
	.Bakisi = 0x00536b40,
#endif
};
static VariableAddress_t botExitDriver = {
#ifndef UYA_PAL
	.Bakisi = 0x00536f80,
#endif
};
static VariableAddress_t botExitPassenger = {
#ifndef UYA_PAL
	.Bakisi = 0x00536c68,
#endif
};

extern int TargetTeam;
extern const int SimPlayerCount;
extern SimulatedPlayer_t SimPlayers[];

const char* TRAINING_AGGRO_NAMES[] = {
	[TRAINING_AGGRESSION_AGGRO] "AGGRO",
	[TRAINING_AGGRESSION_AGGRO_NO_DAMAGE] "NO DAMAGE",
	[TRAINING_AGGRESSION_PASSIVE] "PASSIVE",
	[TRAINING_AGGRESSION_IDLE] "IDLE",
};

static int botVehicleFunctionsReady(void)
{
	u32 get = GetAddress(&botGetVehicle);
	u32 driver = GetAddress(&botAssignDriver);
	u32 passenger = GetAddress(&botAssignPassenger);
	u32 exitDriver = GetAddress(&botExitDriver);
	u32 exitPassenger = GetAddress(&botExitPassenger);
	return get && driver && passenger && exitDriver && exitPassenger &&
		*(u32 *)get == 0x8f82e238 && *(u32 *)(get + 0x10) == 0x24030340 &&
		*(u32 *)driver == 0x27bdffe0 && *(u32 *)passenger == 0x27bdffe0 &&
		*(u32 *)exitDriver == 0x27bdffc0 && *(u32 *)exitPassenger == 0x27bdffc0;
}

static void setBotLockStrafe(Player *player)
{
	u32 create = GetAddress(&vaCreateHero);
	u32 modes;
	if (!player || player->mpIndex < 1 || player->mpIndex >= GAME_MAX_PLAYERS || !create)
		return;
	/* CreateHero initializes the native requested-camera-mode array here.
	 * Changing only the bot's requested mode lets UYA perform its own FPS
	 * camera transition; the real player's control profile is untouched. */
	if ((*(u32 *)(create + 0x268) & 0xffff0000) != 0x3c060000 ||
		(*(u32 *)(create + 0x26c) & 0xffff0000) != 0x24c60000 ||
		*(u32 *)(create + 0x280) != 0x904400a4 ||
		*(u32 *)(create + 0x290) != 0xac640000)
		return;
	modes = (*(u32 *)(create + 0x268) << 16) + *(s16 *)(create + 0x26c);
	if (modes >= 0x00100000 && modes < 0x01ffffe0 && !(modes & 3))
		((int *)modes)[player->mpIndex] = CAMERA_TYPE_LOCK_STRAFE;
}

static void botLeaveVehicle(Player *player)
{
	Vehicle *vehicle = player->vehicle;
	u32 exit;
	if (!vehicle || !botVehicleFunctionsReady())
		return;
	if ((Player *)vehicle->pDriver == player)
		exit = GetAddress(&botExitDriver);
	else if ((Player *)vehicle->pPassenger == player)
		exit = GetAddress(&botExitPassenger);
	else
		return;
	((void (*)(Vehicle *, Player *, int))exit)(vehicle, player, 1);
}

static VariableAddress_t botCameraUpdateHook = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x005361a8,
	.Hoven = 0x005382c0,
	.OutpostX12 = 0x0052db98,
	.KorgonOutpost = 0x0052b330,
	.Metropolis = 0x0052a680,
	.BlackwaterCity = 0x00527f18,
	.CommandCenter = 0x00527cd8,
	.BlackwaterDocks = 0x0052a558,
	.AquatosSewers = 0x00529858,
	.MarcadiaPalace = 0x005291d8,
#else
	.Lobby = 0,
	.Bakisi = 0x00533928,
	.Hoven = 0x00535980,
	.OutpostX12 = 0x0052b298,
	.KorgonOutpost = 0x00528ab0,
	.Metropolis = 0x00527e00,
	.BlackwaterCity = 0x00525618,
	.CommandCenter = 0x00525598,
	.BlackwaterDocks = 0x00527dd8,
	.AquatosSewers = 0x00527118,
	.MarcadiaPalace = 0x00526a58,
#endif
};

static u32 botCameraHookAddress;
static u32 botCameraOriginalCall;

/* Remote hero updates normally decode camera state from network packets.
 * Simulated heroes instead use their own native FPS aim state. */
static void updateBotCamera(Player *player)
{
	int i;
	for (i = 0; i < SimPlayerCount; ++i) {
		if (SimPlayers[i].Player == player) {
			MATRIX matrix;
			player->camRot[0] = 0;
			player->camRot[1] = player->fps.vars.cameraY.rotation;
			player->camRot[2] = player->fps.vars.cameraZ.rotation;
			player->camRot[3] = 0;
			matrix_unit(matrix);
			matrix_rotate_y(matrix, matrix, player->camRot[1]);
			matrix_rotate_z(matrix, matrix, player->camRot[2]);
			memcpy(player->camUMtx, matrix, sizeof(VECTOR) * 3);
			vector_copy(player->camPos, player->fps.cameraPos);
			return;
		}
	}
	((void (*)(Player *))((botCameraOriginalCall & 0x03ffffff) << 2))(player);
}

static void restoreBotCameraHook(void)
{
	if (botCameraHookAddress && *(u32 *)botCameraHookAddress ==
		(0x0c000000 | ((u32)updateBotCamera >> 2))) {
		*(u32 *)botCameraHookAddress = botCameraOriginalCall;
		FlushCache(0);
		FlushCache(2);
	}
	botCameraHookAddress = 0;
	botCameraOriginalCall = 0;
}

static int installBotCameraHook(void)
{
	u32 address = GetAddress(&botCameraUpdateHook);
	u32 target;
	if (!address || *(u32 *)address >> 26 != 3 ||
		*(u32 *)(address + 4) != 0x0200202d)
		return 0;
	target = (*(u32 *)address & 0x03ffffff) << 2;
	if (*(u32 *)target != 0x27bdffc0 || *(u32 *)(target + 0x1c) != 0x8e042558)
		return 0;
	botCameraHookAddress = address;
	botCameraOriginalCall = *(u32 *)address;
	*(u32 *)address = 0x0c000000 | ((u32)updateBotCamera >> 2);
	FlushCache(0);
	FlushCache(2);
	return 1;
}
static void botEnterTestVehicle(SimulatedPlayer_t *bot)
{
	Vehicle *vehicle;
	int i;
	int oClass = bot->Idx == 2 ? MOBY_ID_HOVERSHIP : MOBY_ID_TURBOSLIDER;
	if (bot->Player->vehicle || !botVehicleFunctionsReady())
		return;
	if (bot->Idx == 1) {
		vehicle = SimPlayers[0].Player ? SimPlayers[0].Player->vehicle : 0;
		if (vehicle && !vehicle->pPassenger && (Player *)vehicle->pDriver == SimPlayers[0].Player)
			((void (*)(Vehicle *, Player *))GetAddress(&botAssignPassenger))(vehicle, bot->Player);
		return;
	}
	for (i = 0; i < 32; ++i) {
		vehicle = ((Vehicle *(*)(int))GetAddress(&botGetVehicle))(i);
		if (!vehicle)
			break;
		if (!vehicle->pMoby || mobyIsDestroyed(vehicle->pMoby) ||
			vehicle->pMoby->oClass != oClass || vehicle->pDriver ||
			vehicle->pPassenger || vehicle->hitPoints <= 0 || vehicle->resurrectTimer)
			continue;
		((void (*)(Vehicle *, Player *))GetAddress(&botAssignDriver))(vehicle, bot->Player);
		/* Simulated heroes are not split-screen canvases. The native vehicle
		 * update accepts flags 0x06 as locally simulated even when its driver
		 * is outside CheckLocalPlayers; do not use remote extrapolation. */
		vehicle->flags = (vehicle->flags & ~0x20) | 6;
		break;
	}
}

static void updateBotTestMode(void)
{
	Player *local = playerGetFromSlot(0);
	botTestMode_e next = botTestMode;
	int i;
	if (!local || !local->pPad)
		return;
	if (playerPadGetButtonDown(local, PAD_LEFT) > 0)
		next = BOT_TEST_IDLE;
	else if (playerPadGetButtonDown(local, PAD_RIGHT) > 0)
		next = BOT_TEST_PATROL;
	else if (playerPadGetButtonDown(local, PAD_UP) > 0 && botVehicleFunctionsReady())
		next = BOT_TEST_VEHICLES;
	if (next == botTestMode)
		return;
	for (i = 0; i < SimPlayerCount; ++i) {
		SimulatedPlayer_t *bot = &SimPlayers[i];
		if (!bot->Player)
			continue;
		botLeaveVehicle(bot->Player);
		if (botTestMode == BOT_TEST_VEHICLES) {
			playerRespawn(bot->Player);
			modeInitTarget(bot);
		}
	}
	botTestMode = next;
}

void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx)
{
	int id = idx + 1;
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
	
	// HOOK_JAL(0x00610e38, &fixSimPlayerNetPlayerPtr);

	// Keep the requested slot even when creation must wait for a free player slot.
	memset(sPlayer, 0, sizeof(SimulatedPlayer_t));
	sPlayer->Idx = idx;
	
	if (!players[0] || !players[0]->pPad || players[id])
		return;

	// spawn player
	memcpy(&sPlayer->Pad, players[0]->pPad, sizeof(struct PAD));
	
	sPlayer->Pad.rdata[7] = 0x7f;
	sPlayer->Pad.rdata[6] = 0x7f;
	sPlayer->Pad.rdata[5] = 0x7f;
	sPlayer->Pad.rdata[4] = 0x7f;
	sPlayer->Pad.rdata[3] = 0xff;
	sPlayer->Pad.rdata[2] = 0xff;
	sPlayer->Pad.port = 10;
	sPlayer->Pad.slot = 10;
	sPlayer->Pad.id = id;
	
	// create hero
	gameSettings->PlayerClients[id] = 0;
	gameSettings->PlayerSkins[id] = 0;
	gameSettings->PlayerTeams[id] = TargetTeam;

	POKE_U32(BOT_PAD_LOCATION + (4 * id), (u32)&sPlayer->Pad);

	// local hero
	((void (*)(int))GetAddress(&vaCreateHero))(id);

	Player * newPlayer = players[id];
	if (!newPlayer) {
		POKE_U32(BOT_PAD_LOCATION + (4 * id), 0);
		return;
	}
	sPlayer->Player = newPlayer;
	sPlayer->vtable = playerGetVTable(sPlayer->Player);

	players[id] = sPlayer->Player;

	POKE_U32(BOT_PAD_LOCATION + (4 * id), 0);

	sPlayer->Player->pPad = (void*)&sPlayer->Pad;
	sPlayer->Player->fps.vars.cam_slot = id;
	sPlayer->Player->mpIndex = id;
	sPlayer->Player->mpTeam = TargetTeam;
	sPlayer->Idx = idx;
	sPlayer->Player->fps.vars.pHero = sPlayer->Player;
	setBotLockStrafe(sPlayer->Player);

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
	if (!sPlayer || !player || !sPlayer->Active)
		return;


	// reset pad
	struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
	pad->btns = 0xffff;
	pad->rjoy_h = 0x7f;
	pad->rjoy_v = 0x7f;
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
	#ifndef SPECTATE
	modeUpdateTarget(sPlayer);
	#else
	if (botTestMode == BOT_TEST_PATROL)
		modeUpdateSpectatePatrol(sPlayer);
	else if (botTestMode == BOT_TEST_VEHICLES) {
		botEnterTestVehicle(sPlayer);
		/* Short, gentle steering pulses exercise moving vehicle cameras.
		 * The passenger remains unarmed and does not switch seats. */
		if (sPlayer->Player->vehicle && sPlayer->Idx != 1 &&
			(gameGetTime() / 1000) % 8 < 2) {
			pad->ljoy_h = 0x9f;
			pad->ljoy_v = 0x5f;
			if (sPlayer->Idx == 0) {
				/* Turboslider accelerates with Cross, not the left stick. */
				pad->btns &= ~PAD_CROSS;
				pad->cross_p = 0xff;
			}
		}
	}
	#endif
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

	if (!sPlayer->Created || !sPlayer->Player)
		return;

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

			targetUpdate(&SimPlayers[i]);

			// process input (Correct Address though)
			// ((void (*)(u32))GetAddress(&vaPadProcessInput))(&SimPlayers[i].Pad);

			// update pad (THIS IS CORRECT! :D)
			((void (*)(struct PAD *, void *, int))GetAddress(&vaPAD_PadUpdate))(&SimPlayers[i].Pad, &SimPlayers[i].Pad.rdata, 0x14);

			// run game's hero update
			// pVTable->Update(SimPlayers[i].Player);
		}
	}
}

void initBots(void)
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
	if (botsStartTicks < 15) {
		++botsStartTicks;
		return;
	}

	#ifdef SPECTATE
	if (!installBotCameraHook())
		return;
	#endif

	memset(SimPlayers, 0, sizeof(SimulatedPlayer_t) * SimPlayerCount);
	memset(&State, 0, sizeof(State));

	State.TimeLastKill = timerGetSystemTime();
	State.InitializedTime = gameGetTime();
	botTestMode = BOT_TEST_PATROL;

	// run update functions for SimHeroes
	#ifndef SPECTATE
	HOOK_J(GetAddressImmediate(&vaInit_RunUpdateFunctions), onSimulateHeros);
	#endif
	// HOOK_JAL(0x003D29F0, onSimulateHeros); // This one removes hud and such.

	for (i = 0; i < SimPlayerCount; ++i) {
		SimPlayers[i].Idx = i;
	}

	Initialized = 1;
}

void frameTick(void)
{
	int i = 0;

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

void startBots(void) // gameStart
{
	// How Many Local Players
	// *(u32*)0x001a5e5c = 3;

	// Ensure in game
	GameSettings * gameSettings = gameGetSettings();
	if (!gameSettings || !isInGame()) {
		#ifdef SPECTATE
		restoreBotCameraHook();
		#endif
		Initialized = 0;
		botsStartTicks = 0;
		return;
	}

	if (!Initialized) {
		initBots(); // initialize()
		return;
	}

	#ifndef SPECTATE
	frameTick();
	#else
	updateBotTestMode();
	onSimulateHeros();
	#endif
	gameTick();
}
