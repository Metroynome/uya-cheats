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

int VampireHealRate[] = {
	PLAYER_MAX_HEALTH * 0.25,
	PLAYER_MAX_HEALTH * 0.50,
	PLAYER_MAX_HEALTH * 1.00
};

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
		// playerSetHealth(player, clamp(((int)player->pNetPlayer->pNetPlayerData->hitPoints + VampireHealRate[1]), 0, PLAYER_MAX_HEALTH));
    }
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		// static int Occlusion = (Occlusion == 2) ? 0 : 2;
		// gfxOcclusion(Occlusion);
		//spawnPointGetRandom(player, &position, &rotation);
		// Player ** ps = playerGetAll();
		// Player * p = ps[1];
		// playerSetPosRot(player, &p->PlayerPosition, &p->PlayerRotation);
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		// Set Gattling Turret Health to 1.
		DEBUGsetGattlingTurretHealth();
	}
	else if ((pad->btns & PAD_L3) == 0 && Active == 0)
	{
		// Show Map
		// This one doesn't update until select button map is updated.
		// ((void (*)(u32, u32, int, u32))0x004A3B70)(-1, 0x00248B38, 0x1f0, 0x00248A38);
		// ((void (*)(int, int, int))0x0053FC28)(0, 10, 0x1f0);
	}
	else if ((pad->btns & PAD_R3) == 0 && Active == 0)
	{
		// Show Scoreboard
		// ((void (*)(int))0x004A3B70)(-1);
	}
    if (!(pad->btns & PAD_LEFT) == 0 && !(pad->btns & PAD_RIGHT) == 0 && !(pad->btns & PAD_UP) == 0 && !(pad->btns & PAD_DOWN) == 0 && !(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0)
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

void DEBUGsetGattlingTurretHealth(void)
{
    Moby * moby = mobyListGetStart();
    // Iterate through mobys and change health
    while ((moby = mobyFindNextByOClass(moby, MOBY_ID_GATTLING_TURRET)))
    {
        if (moby->PVar)
        {
			*(float*)((u32)moby->PVar + 0x30) = 0;
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

void handleGadgetEvents(int message, char GadgetEventType, int ActiveTime, short GadgetId, int t0, int StackPointer)
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
	int GEF = GetAddress(&vaGadgetEventFunc);
	Player * player = (Player*)((u32)message - 0x1a40);
	tNW_GadgetEventMessage * msg = (tNW_GadgetEventMessage*)message;
	printf("\nMessage: 0x%p", message);
	printf("\nPlayer: 0x%p", player);
	printf("\nGadgetEvent: 0x%x", GadgetEventType);
	printf("\nTargetUID: 0x%x\n", msg->TargetUID);
	// GadgetEventType 7 = Niked, or splash damage.
	if (msg && GadgetEventType == 7)
	{
		if(GadgetId == 3)
		{
			GadgetEventType = 8;
		}
	}
	// GadgetEventType 8 = Hit Something
	// else if (msg && msg->GadgetEventType == 8)
	// {
	// 	int delta = ActiveTime - gameGetTime();
	// 	// Make player hold correct weapon.
	// 	if (player->WeaponHeldId != msg->GadgetId)
	// 	{
	// 		playerEquipWeapon(player, msg->GadgetId);
	// 	}
	// 	// Set weapon shot event time to now if its in the future
	// 	if (player->WeaponHeldId == msg->GadgetId && (delta > 0 || delta < -TIME_SECOND))
	// 	{
	// 		ActiveTime = gameGetTime();
	// 	}
	// }
	// run base command
	((void (*)(int, char, int, short, int, int))GEF)(message, GadgetEventType, ActiveTime, GadgetId, t0, StackPointer);
}

void patchGadgetEvents(void)
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
	HOOK_JAL(GetAddress(&vaGadgetEventHook), &handleGadgetEvents);
}


void vampireLogic(float healRate)
{
	int i;
	Player ** playerObjects = playerGetAll();
	Player * player;
	GameData * gameData = gameGetData();
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		player = playerObjects[i];
		if (!player)
			return;

		// Check if player has killed someone
		if (gameData->PlayerStats[i].Kills > PlayerKills[i])
		{
			// Heal player
            playerSetHealth(player, clamp(player->pNetPlayer->pNetPlayerData->hitPoints + healRate, 0, PLAYER_MAX_HEALTH));
			// Update our cached kills count
			PlayerKills[i] = gameData->PlayerStats[i].Kills;
		}
	}
}

void disableDrones(void)
{
	// Moby * a = mobyListGetStart();
	// // Remove drone cluster update function. (this is for main configuration)
	// while ((a = mobyFindNextByOClass(a, MOBY_ID_DRONE_BOT_CLUSTER_CONFIG))) {
	// 	a->PUpdate = 0;
	// 	mobyDestroy(a);
	// 	++a;
	// }
	// Moby * b = mobyListGetStart();
	// // Remove drone update function. (This is for the player activator)
	// while ((b = mobyFindNextByOClass(b, MOBY_ID_DRONE_BOT_CLUSTER_UPDATER))) {
	// 	b->PUpdate = 0;
	// 	mobyDestroy(b);
	// 	++b;
	// }
	Moby * c = mobyListGetStart();
	// move drones to zero and delete pvar pointer.
	while ((c = mobyFindNextByOClass(c, MOBY_ID_DRONE_BOT))) {
		c->PVar = 0;
		c->PUpdate = 0;
		c->Scale = 0;
		// mobyDestroy(c);
		memset(c->BSphere, 0, sizeof(c->BSphere));
		//memset(c->Position, 0, sizeof(c->Position));
		++c;
	}
}

void hideRadarBlips(void)
{

}

void healthConversion(void)
{
	// LOCATION AT BAKISI: 0x004a771c
	// Outpost x12 rand data: 0x003b74a0
	// Bakisi rand data - Health: 0x003bff61

	int StackAddr = 0x000f4fe0;
	u32 XORAddr = *(u32*)(StackAddr);
	float XORHealth = *(u32*)(StackAddr + 0x4);
	float Health;
	int Rand_Health_Data = 0x003bff61;
	Player * player = (Player*)PLAYER_STRUCT;
	int PlayerHealth_Value = player->Health;
	int PlayerHealth_Addr = &player->Health; // v1 (Original data: (&DAT_00002476)[iVar2])
	int n = 0; // t3
	u32 Offset;
	do {
		Offset = (((u32)PlayerHealth_Addr - (u32)PlayerHealth_Value) & 7) + n; // a3
		n = n + 5;
		*(u8*)StackAddr = *(u8*)((u32)Rand_Health_Data + ((u32)PlayerHealth_Value + (Offset & 7) * 0xff));
		// *(u8*)StackAddr = (&DAT_003bff61)[(uint)PlayerHealth_Value + (uVar5 & 7) * 0xff];
		StackAddr = (int)StackAddr + 1;
		// StackAddr = (uint *)((int)StackAddr + 1); // a2
	} while (n < 0x28);
	XORAddr = (u32)(XORAddr) ^ (u32)PlayerHealth_Addr;
	Health = (float)((u32)XORHealth ^ (u32)XORAddr);

	*(u32*)(StackAddr) = Health;
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

        // patchResurrectWeaponOrdering();

		// patchFluxNicking();
		// patchGadgetEvents();
		// disableDrones();
		// vampireLogic(VampireHealRate[0]);
		// hideRadarBlips();
		healthConversion();
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
        Debug();
    }
	
	// StartBots();

	uyaPostUpdate();
    return 0;
}
