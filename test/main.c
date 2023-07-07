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

void Debug()
{
    static int Active = 0;
	static int Occlusion = 2; // Default Occlusion = 2
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

		// Respawn Function
		//((void (*)(u32))0x004EF510)(PLAYER_STRUCT);
		// Remove save health to tnw player
		//*(u32*)0x004EF79C = 0;

		// Swap Teams (blue <-> red)
		player->Team = !player->Team;

	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerDecHealth(player, 15);
    }
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		Occlusion = (Occlusion == 2) ? 0 : 2;
		gfxOcclusion(Occlusion);
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		// Set Gattling Turret Health to 1.
		// DEBUGsetGattlingTurretHealth();

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

		PadButtonStatus * pad = (PadButtonStatus*)player->Paddata;
		if (player->IsChargebooting == 1 && ((pad->btns & PAD_R2) == 0) && player->StateTimer > 55)
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

// void disableWeaponPacks(void)
// {
//   VariableAddress_t vaWeaponPackSpawnFunc = {
// #if UYA_PAL
//     .Lobby = 0,
// 	  .Bakisi = 0x004FA350,
// 	  .Hoven = 0x004FC468,
// 	  .OutpostX12 = 0x004F1D40,
//     .KorgonOutpost = 0x004EF4D8,
// 	  .Metropolis = 0x004EE828,
// 	  .BlackwaterCity = 0x004EC0C0,
// 	  .CommandCenter = 0x004EC088,
//     .BlackwaterDocks = 0x004EE908,
//     .AquatosSewers = 0x004EDC08,
//     .MarcadiaPalace = 0x004ED588,
// #else
//     .Lobby = 0,
// 	  .Bakisi = 0x004F7BD0,
// 	  .Hoven = 0x004F9C28,
// 	  .OutpostX12 = 0x004EF540,
//     .KorgonOutpost = 0x004ECD58,
// 	  .Metropolis = 0x004EC0A8,
// 	  .BlackwaterCity = 0x004E98C0,
// 	  .CommandCenter = 0x004E9A48,
//     .BlackwaterDocks = 0x004EC288,
//     .AquatosSewers = 0x004EB5C8,
//     .MarcadiaPalace = 0x004EAF08,
// #endif
//   };

//   u32 weaponPackSpawnFunc = GetAddress(&vaWeaponPackSpawnFunc);
//   if (weaponPackSpawnFunc) {
//     *(u32*)weaponPackSpawnFunc = 0;
//     *(u32*)(weaponPackSpawnFunc - 0x7BF4) = 0;
//   }
// }

void patchDeadJumping(void)
{
	Player ** players = playerGetAll();
	int i;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
    	if (!players[i])
    		continue;

		Player * player = players[i];
		if (playerIsLocal(player) && playerIsDead(player))
		{
			player->CantMoveTimer = 10;
		}
	}
}

// int SpawnedPack[2] = {0,0};
// void SpawnPack(u32 a0, u32 a1)
// {
//   VariableAddress_t vaRespawnFunc = {
// #if UYA_PAL
//     .Lobby = 0,
//     .Bakisi = 0x0051a940,
//     .Hoven = 0x0051ca58,
//     .OutpostX12 = 0x00512330,
//     .KorgonOutpost = 0x0050fac8,
//     .Metropolis = 0x0050ee18,
//     .BlackwaterCity = 0x0050c6b0,
//     .CommandCenter = 0x0050c470,
//     .BlackwaterDocks = 0x0050ecf0,
//     .AquatosSewers = 0x0050dff0,
//     .MarcadiaPalace = 0x0050d970,
// #else
//     .Lobby = 0,
//     .Bakisi = 0x00518138,
//     .Hoven = 0x0051a190,
//     .OutpostX12 = 0x0050faa8,
//     .KorgonOutpost = 0x0050d2c0,
//     .Metropolis = 0x0050c610,
//     .BlackwaterCity = 0x00509e28,
//     .CommandCenter = 0x00509da8,
//     .BlackwaterDocks = 0x0050c5e8,
//     .AquatosSewers = 0x0050b928,
//     .MarcadiaPalace = 0x0050b268,
// #endif
//   };

//   VariableAddress_t vaSpawnWeaponPackFunc = {
// #if UYA_PAL
//     .Lobby = 0,
//     .Bakisi = 0x004fb188,
//     .Hoven = 0x004fd2a0,
//     .OutpostX12 = 0x004f2b78,
//     .KorgonOutpost = 0x004f0310,
//     .Metropolis = 0x004ef660,
//     .BlackwaterCity = 0x004ecef8,
//     .CommandCenter = 0x004ecec0,
//     .BlackwaterDocks = 0x004ef740,
//     .AquatosSewers = 0x004eea40,
//     .MarcadiaPalace = 0x004ee3c0,
// #else
//     .Lobby = 0,
//     .Bakisi = 0x004f8a08,
//     .Hoven = 0x004faa60,
//     .OutpostX12 = 0x004f0378,
//     .KorgonOutpost = 0x004edb90,
//     .Metropolis = 0x004ecee0,
//     .BlackwaterCity = 0x004ea6f8,
//     .CommandCenter = 0x004ea880,
//     .BlackwaterDocks = 0x004ed0c0,
//     .AquatosSewers = 0x004ec400,
//     .MarcadiaPalace = 0x004ebd40,
// #endif
//   };

//     // Run normal function
//     ((void (*)(u32, u32))GetAddress(&vaRespawnFunc))(a0, a1);

//     // get all players
//     Player ** players = playerGetAll();
// 	int i;
// 	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
// 	{
//         // Check to see if players are local
//     	if (!players[i] && !playerIsLocal(players[i]))
//     		continue;

// 		Player * player = players[i];
//         // if player health is zero, and pack hasn't spawned, spawn pack
// 		if (playerGetHealth(player) <= 0 && !SpawnedPack[i])
// 		{
//             // Spawn Pack
//             ((void (*)(u32))GetAddress(&vaSpawnWeaponPackFunc))(player);
//             // It now spawned pack, so set to true.
//             SpawnedPack[i] = 1;
//         }
// 	}
// }

// void spawnWeaponPackOnDeath(void)
// {
//   VariableAddress_t vaRespawnPlayerHook = {
// #if UYA_PAL
//     .Lobby = 0,
//     .Bakisi = 0x00533900,
//     .Hoven = 0x00535a18,
//     .OutpostX12 = 0x0052b2f0,
//     .KorgonOutpost = 0x00528a88,
//     .Metropolis = 0x00527dd8,
//     .BlackwaterCity = 0x00525670,
//     .CommandCenter = 0x00525430,
//     .BlackwaterDocks = 0x00527cb0,
//     .AquatosSewers = 0x00526fb0,
//     .MarcadiaPalace = 0x00526930,
// #else
//     .Lobby = 0,
//     .Bakisi = 0x00531080,
//     .Hoven = 0x005330d8,
//     .OutpostX12 = 0x005289f0,
//     .KorgonOutpost = 0x00526208,
//     .Metropolis = 0x00525558,
//     .BlackwaterCity = 0x00522d70,
//     .CommandCenter = 0x00522cf0,
//     .BlackwaterDocks = 0x00525530,
//     .AquatosSewers = 0x00524870,
//     .MarcadiaPalace = 0x005241b0,
// #endif
//   };

//   // Disable normal Weapon Pack spawns
//   disableWeaponPacks();

//   // Hook SpawnPack
//   if (*(u32*)GetAddress(&vaRespawnPlayerHook) != (0x0C000000 | ((u32)(&SpawnPack) >> 2)))
//     HOOK_JAL(GetAddress(&vaRespawnPlayerHook), &SpawnPack);

//   // if Health is greater than zero and pack has spawned
//     Player ** players = playerGetAll();
// 	int i;
// 	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
// 	{
//     	if (!players[i] && !playerIsLocal(players[i]))
//     		continue;

// 		Player * player = players[i];
// 		if (playerGetHealth(player) > 0)
// 		{
//             SpawnedPack[i] = 0;
// 		}
// 	}
// }

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

void patchWeaponShotLag(void)
{
// 	VariableAddress_t vaAllWeaponsUDPtoTCP = {
// #if UYA_PAL
// 		.Lobby = 0,
// 		.Bakisi = 0x00546760,
// 		.Hoven = 0x00548928,
// 		.OutpostX12 = 0x0053e200,
// 		.KorgonOutpost = 0x0053b8e8,
// 		.Metropolis = 0x0053ace8,
// 		.BlackwaterCity = 0x005384d0,
// 		.CommandCenter = 0x00537d28,
// 		.BlackwaterDocks = 0x0053a5a8,
// 		.AquatosSewers = 0x005398a8,
// 		.MarcadiaPalace = 0x00539228,
// #else
// 		.Lobby = 0,
// 		.Bakisi = 0x00543e54,
// 		.Hoven = 0x00545f5c,
// 		.OutpostX12 = 0x0053b874,
// 		.KorgonOutpost = 0x00538fdc,
// 		.Metropolis = 0x005383dc,
// 		.BlackwaterCity = 0x00535b44,
// 		.CommandCenter = 0x00535574,
// 		.BlackwaterDocks = 0x00537db4,
// 		.AquatosSewers = 0x005370f4,
// 		.MarcadiaPalace = 0x00536a34,
// #endif
// 	};

	VariableAddress_t vaFluxUDPtoTCP = {
#if UYA_PAL
		.Lobby = 0,
		.Bakisi = 0x0040931c,
		.Hoven = 0x00408c84,
		.OutpostX12 = 0x00400b7c,
		.KorgonOutpost = 0x003fff5c,
		.Metropolis = 0x003feb1c,
		.BlackwaterCity = 0x003fb904,
		.CommandCenter = 0x00409d0c,
		.BlackwaterDocks = 0x0040bc6c,
		.AquatosSewers = 0x0040b874,
		.MarcadiaPalace = 0x0040a8ec,
#else
		.Lobby = 0,
		.Bakisi = 0x00408c7c,
		.Hoven = 0x00408564,
		.OutpostX12 = 0x0040045c,
		.KorgonOutpost = 0x003ff89c,
		.Metropolis = 0x003fe47c,
		.BlackwaterCity = 0x003fb204,
		.CommandCenter = 0x00409654,
		.BlackwaterDocks = 0x0040b5b4,
		.AquatosSewers = 0x0040b1bc,
		.MarcadiaPalace = 0x0040a234,
#endif
	};

// 	VariableAddress_t vaHandleWeaponShotDelayedHook = {
// #if UYA_PAL
// 		.Lobby = 0,
// 		.Bakisi = 0x0054b23c,
// 		.Hoven = 0x0054d404,
// 		.OutpostX12 = 0x00542cdc,
// 		.KorgonOutpost = 0x005403c4,
// 		.Metropolis = 0x0053f7c4,
// 		.BlackwaterCity = 0x0053cfac,
// 		.CommandCenter = 0x0053c804,
// 		.BlackwaterDocks = 0x0053f084,
// 		.AquatosSewers = 0x0053e384,
// 		.MarcadiaPalace = 0x0053dd04,
// #else
// 		.Lobby = 0,
// 		.Bakisi = 0x00548894,
// 		.Hoven = 0x0054a99c,
// 		.OutpostX12 = 0x005402b4,
// 		.KorgonOutpost = 0x0053da1c,
// 		.Metropolis = 0x0053ce1c,
// 		.BlackwaterCity = 0x0053a584,
// 		.CommandCenter = 0x00539fb4,
// 		.BlackwaterDocks = 0x0053c7f4,
// 		.AquatosSewers = 0x0053bb34,
// 		.MarcadiaPalace = 0x0053b474,
// #endif
// 	};
	int TCP = 0x24040040;

	// Send all weapon shots reliably (Use TCP instead of UDP)
	// int AllWeaponsAddr = GetAddress(&vaAllWeaponsUDPtoTCP);
	// if (*(u32*)AllWeaponsAddr == 0x906407D4)
	// 	*(u32*)AllWeaponsAddr = TCP;

	// Send Flux shots reliably (Use TCP instead of UDP)
	int FluxAddr = GetAddress(&vaFluxUDPtoTCP);
	if (*(u32*)FluxAddr == 0x90A407D4)
		*(u32*)FluxAddr = TCP;

	// int WeaponShotDelayed = GetAddress(&vaHandleWeaponShotDelayedHook);
	// int hook = (0x0C000000 | ((u32)(&handleWeaponShotDelayed) >> 2))
	// if (*(u32*)WeaponShotDelayed != hook)
	// 	*(u32*)WeaponShotDelayed = hook;
}

void patchFluxNicking(void)
{
	// Bakisi (Original Value: 0x1060000D)
	// Main Branch I beleive
	// *(u32*)0x0040869C = 0x1000000D;

	// Never nik?
	*(u32*)0x004086D8 = 0;
	*(u32*)0x004086DC = 0;
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
		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

        //patchResurrectWeaponOrdering();

		// setRespawnTimer();
		// spawnWeaponPackOnDeath();
		// patchDeadJumping();
		patchFluxNicking();
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
        Debug();
    }

	uyaPostUpdate();

    return 0;
}
