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

extern VariableAddress_t vaPlayerRespawnFunc;

void StartBots(void);

short PlayerKills[GAME_MAX_PLAYERS];
short PlayerDeaths[GAME_MAX_PLAYERS];

VECTOR position;
VECTOR rotation;

void Debug()
{
    static int Active = 0;
	Player * player = (Player*)PLAYER_STRUCT;
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;

    /*
		DEBUG OPTIONS:
		PAD_UP: Occlusion on/off
		PAD_DOWN: Gattling Turret Health to 1
		PAD_LEFT: Change Team (red <-> blue)
		PAD_RIGHT: Hurt Player
	*/
    if ((pad->btns & PAD_LEFT) == 0 && Active == 0)
	{
        Active = 1;

        // WeaponStripMe (Hook: 0x004EF550)
        // ((void (*)(u32))0x004EF848)(PLAYER_STRUCT);

        // GiveMeRandomWeapons (Hook: 0x004EF56C)
        //((void (*)(u32, int))0x0050ED38)(PLAYER_STRUCT, 0x3);

        // Give Weapon (Hook: 0x0050EE0C)
        // - Quick Select Slots get set in order of which weapons are given first.
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x2, 2);
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x3, 2);
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x5, 2);

        // Equip Weapon: (Hook: 0x0050EE2C)
        // ((void (*)(u32, int))0x0053C2D0)((u32)PLAYER_STRUCT + 0x1a40, 0x2);
        // - Inside above address: 0x0053C398 (JAL)

		// Swap Teams
		int SetTeam = (player->Team < 7) ? player->Team + 1 : 0;
		playerSetTeam(player, SetTeam);

	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerDecHealth(player, 14);
    }
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		// static int Occlusion = (Occlusion == 2) ? 0 : 2;
		// gfxOcclusion(Occlusion);
		spawnPointGetRandom(player, &position, &rotation);
		playerSetPosRot(player, NULL, NULL);
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		// Set Gattling Turret Health to 1.
		DEBUGsetGattlingTurretHealth();
	}
    if (!(pad->btns & PAD_LEFT) == 0 && !(pad->btns & PAD_RIGHT) == 0 && !(pad->btns & PAD_UP) == 0 && !(pad->btns & PAD_DOWN) == 0)
    {
        Active = 0;
    }
}

void patchResurrectWeaponOrdering_HookWeaponStripMe(Player * player)
{
	// backup currently equipped weapons
	// if (player->IsLocal) {
	// 	weaponOrderBackup[player->LocalPlayerIndex][0] = playerGetLocalEquipslot(player->LocalPlayerIndex, 0);
	// 	weaponOrderBackup[player->LocalPlayerIndex][1] = playerGetLocalEquipslot(player->LocalPlayerIndex, 1);
	// 	weaponOrderBackup[player->LocalPlayerIndex][2] = playerGetLocalEquipslot(player->LocalPlayerIndex, 2);
	// }

	// call hooked WeaponStripMe function after backup
	playerStripWeapons(player);
}

void patchResurrectWeaponOrdering_HookGiveMeRandomWeapons(Player* player, int weaponCount)
{
	// int i, j, matchCount = 0;

	// call hooked GiveMeRandomWeapons function first
	((void (*)(Player*, int))0x005f7510)(player, weaponCount);

	// then try and overwrite given weapon order if weapons match equipped weapons before death
	// if (player->IsLocal) {

	// 	// restore backup if they match (regardless of order) newly assigned weapons
	// 	for (i = 0; i < 3; ++i) {
	// 		int backedUpSlotValue = weaponOrderBackup[player->LocalPlayerIndex][i];
	// 		for (j = 0; j < 3; ++j) {
	// 			if (backedUpSlotValue == playerGetLocalEquipslot(player->LocalPlayerIndex, j)) {
	// 				matchCount++;
	// 			}
	// 		}
	// 	}

	// 	// if we found a match, set
	// 	if (matchCount == 3) {
	// 		// set equipped weapon in order
	// 		for (i = 0; i < 3; ++i) {
	// 			playerSetLocalEquipslot(player->LocalPlayerIndex, i, weaponOrderBackup[player->LocalPlayerIndex][i]);
	// 		}

	// 		// equip first slot weapon
	// 		playerEquipWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][0]);
	// 	}
	// }
}

void patchResurrectWeaponOrdering(void)
{
	if (!isInGame())
		return;

	HOOK_JAL(0x004EF550, &patchResurrectWeaponOrdering_HookWeaponStripMe);
	HOOK_JAL(0x004EF56C, &patchResurrectWeaponOrdering_HookGiveMeRandomWeapons);
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

		if (player->IsChargebooting == 1 && playerPadGetButton(player, PAD_R2) > 0 && player->StateTimer > 55)
			player->StateTimer = 55;
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
            (float)player->Velocity[2] = 0.125;
    }
}

void setRespawnTimer(void)
{
	VariableAddress_t vaRespawnTimerFunc = {
	// Uses the start of the Respawn Timer Function
#if UYA_PAL
		.Lobby = 0,
		.Bakisi = 0x003f6098,
		.Hoven = 0x003f4f20,
		.OutpostX12 = 0x003ece18,
		.KorgonOutpost = 0x003eccd8,
		.Metropolis = 0x003eadb8,
		.BlackwaterCity = 0x003e7ba0,
		.CommandCenter = 0x003f6a88,
		.BlackwaterDocks = 0x003f89e8,
		.AquatosSewers = 0x003f85f0,
		.MarcadiaPalace = 0x003f6b88,
#else
		.Lobby = 0,
		.Bakisi = 0x003f5ba8,
		.Hoven = 0x003f49b0,
		.OutpostX12 = 0x003ec8a8,
		.KorgonOutpost = 0x003ec7c8,
		.Metropolis = 0x003ea8c8,
		.BlackwaterCity = 0x003e7650,
		.CommandCenter = 0x003f6580,
		.BlackwaterDocks = 0x003f84e0,
		.AquatosSewers = 0x003f80e8,
		.MarcadiaPalace = 0x003f6680,
#endif
	};
#if UYA_PAL
	int FPS = 50;
#else
	int FPS = 60;
#endif
	GameSettings * gameSettings = gameGetSettings();
	int Seconds = 0;
	int RespawnTime = Seconds / FPS;
	int RespawnAddr = GetAddress(&vaRespawnTimerFunc);
	if (gameSettings->GameType == GAMERULE_SIEGE || gameSettings->GameType == GAMERULE_CTF)
	{
		// Set Main Respawn timer
		*(u16*)(RespawnAddr + 0x10) = RespawnTime;
		// Set Default Siege/CTF Respawn Timer
		*(u16*)(RespawnAddr + 0x78) = RespawnTime;
		// Gatlin Turret Destroyed
		*(u16*)(RespawnAddr + 0x80) = RespawnTime;
		// Anti-Air Turret Destroyed
		*(u16*)(RespawnAddr + 0x8c) = RespawnTime;
		
	}
	else if (gameSettings->GameType == GAMERULE_DM)
	{
		// Set DM Default Respawn Timer
		*(u16*)(RespawnAddr + 0x10) = RespawnTime;
	}
}

void DEBUGsetGattlingTurretHealth(void)
{
    Moby * moby = mobyListGetStart();
    // Iterate through mobys and change health
    while ((moby = mobyFindNextByOClass(moby, MOBY_ID_GATTLING_TURRET)))
    {
        if (moby->PVar)
        {
			*(float*)((u32)moby->PVar + 0x30) = 1;
        }
        ++moby; // increment moby
    }
}

// void patchFluxNicking(void)
// {
// 	GadgetDef * weapon = weaponGadgetList();
// 	weapon[WEAPON_ID_FLUX].damage2 = weapon[WEAPON_ID_FLUX].damage;
// 	weapon[WEAPON_ID_FLUX_V2].damage2 = weapon[WEAPON_ID_FLUX_V2].damage;
// }

// void handleWeaponShotDelayed(Player * player, char a1, int a2, short a3, char t0, struct tNW_GadgetEventMessage * message)
// {
// 	VariableAddress_t vaHandleWeaponShotDelayedFunc = {
// #if UYA_PAL
// 		.Lobby = 0,
// 		.Bakisi = 0x00546510,
// 		.Hoven = 0x005486d8,
// 		.OutpostX12 = 0x0053dfb0,
// 		.KorgonOutpost = 0x0053b698,
// 		.Metropolis = 0x0053aa98,
// 		.BlackwaterCity = 0x00538280,
// 		.CommandCenter = 0x00537ad8,
// 		.BlackwaterDocks = 0x0053a358,
// 		.AquatosSewers = 0x00539658,
// 		.MarcadiaPalace = 0x00538fd8,
// #else
// 		.Lobby = 0,
// 		.Bakisi = 0x00543c00,
// 		.Hoven = 0x00545d08,
// 		.OutpostX12 = 0x0053b620,
// 		.KorgonOutpost = 0x00538d88,
// 		.Metropolis = 0x00538188,
// 		.BlackwaterCity = 0x005358f0,
// 		.CommandCenter = 0x00535320,
// 		.BlackwaterDocks = 0x00537b60,
// 		.AquatosSewers = 0x00536ea0,
// 		.MarcadiaPalace = 0x005367e0,
// #endif
// 	};
// 	if (player && message && message->GadgetEventType == 8) {
// 		int delta = a2 - gameGetTime();

// 		// client is not holding correct weapon on our screen
// 		// haven't determined a way to fix this yet but
// 		if (player->Gadgets[0].id != message->GadgetId) {
// 			//DPRINTF("remote gadgetevent %d from weapon %d but player holding %d\n", message->GadgetEventType, message->GadgetId, player->Gadgets[0].id);
// 			playerEquipWeapon(player, message->GadgetId);
// 		}

// 		// set weapon shot event time to now if its in the future
// 		// because the client is probably lagging behind
// 		if (player->Gadgets[0].id == message->GadgetId && (delta > 0 || delta < -TIME_SECOND)) {
// 			a2 = gameGetTime();
// 		}
// 	}

// 	((void (*)(Player *, char, int, short, char, struct tNW_GadgetEventMessage *))GetAddress(&vaHandleWeaponShotDelayedFunc))(player, a1, a2, a3, t0, message);
// }

void handleWeaponShots(int message, char GadgetEventType, int ActiveTime, short GadgetId, int t0, int StackPointer)
{
	VariableAddress_t vaGadgetEventFunc = {
#if UYA_PAL
		.Lobby = 0,
		.Bakisi = 0x00546510,
		.Hoven = 0x005486d8,
		.OutpostX12 = 0x0053dfb0,
		.KorgonOutpost = 0x0053b698,
		.Metropolis = 0x0053aa98,
		.BlackwaterCity = 0x00538280,
		.CommandCenter = 0x00537ad8,
		.BlackwaterDocks = 0x0053a358,
		.AquatosSewers = 0x00539658,
		.MarcadiaPalace = 0x00538fd8,
#else
		.Lobby = 0,
		.Bakisi = 0x00543c00,
		.Hoven = 0x00545d08,
		.OutpostX12 = 0x0053b620,
		.KorgonOutpost = 0x00538d88,
		.Metropolis = 0x00538188,
		.BlackwaterCity = 0x005358f0,
		.CommandCenter = 0x00535320,
		.BlackwaterDocks = 0x00537b60,
		.AquatosSewers = 0x00536ea0,
		.MarcadiaPalace = 0x005367e0,
#endif
	};
	Player * player = (Player*)((u32)message - 0x1a40);
	tNW_GadgetEventMessage * msg = (tNW_GadgetEventMessage*)message;
	if (msg && msg->GadgetEventType == 8)
	{
		int delta = ActiveTime - gameGetTime();
		// Make player hold correct weapon.
		if (player->WeaponHeldId != msg->GadgetId)
		{
			playerEquipWeapon(player, msg->GadgetId);
		}
		// Set weapon shot event time to now if its in the future
		if (player->WeaponHeldId == msg->GadgetId && (delta > 0 || delta < -TIME_SECOND))
		{
			ActiveTime = gameGetTime();
		}
		int GEF = GetAddress(&vaGadgetEventFunc);
		((void (*)(int, char, int, short, int, int))GEF)(message, GadgetEventType, ActiveTime, GadgetId, t0, StackPointer);
	}
}

void patchWeaponShotLag(void)
{
	VariableAddress_t vaGadgetEventHook = {
#if UYA_PAL
		.Lobby = 0,
		.Bakisi = 0x0054b23c,
		.Hoven = 0x0054d404,
		.OutpostX12 = 0x00542cdc,
		.KorgonOutpost = 0x005403c4,
		.Metropolis = 0x0053f7c4,
		.BlackwaterCity = 0x0053cfac,
		.CommandCenter = 0x0053c804,
		.BlackwaterDocks = 0x0053f084,
		.AquatosSewers = 0x0053e384,
		.MarcadiaPalace = 0x0053dd04,
#else
		.Lobby = 0,
		.Bakisi = 0x00548894,
		.Hoven = 0x0054a99c,
		.OutpostX12 = 0x005402b4,
		.KorgonOutpost = 0x0053da1c,
		.Metropolis = 0x0053ce1c,
		.BlackwaterCity = 0x0053a584,
		.CommandCenter = 0x00539fb4,
		.BlackwaterDocks = 0x0053c7f4,
		.AquatosSewers = 0x0053bb34,
		.MarcadiaPalace = 0x0053b474,
#endif
	};
	HOOK_JAL(GetAddress(&vaGadgetEventHook), &handleWeaponShots);
}

int main()
{

	uyaPreUpdate();

	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0)
	{
		
	}

    if (isInGame())
    {
		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

        //patchResurrectWeaponOrdering();

		// setRespawnTimer();
		// patchFluxNicking();
		patchWeaponShotLag();
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
        Debug();
    }
	
	// StartBots();

	uyaPostUpdate();

    return 0;
}
