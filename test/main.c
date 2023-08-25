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

void StartBots(void);

extern VariableAddress_t vaPlayerRespawnFunc;

int enableFpsCounter = 1;
float lastFps = 0;
int renderTimeMs = 0;
float averageRenderTimeMs = 0;
int updateTimeMs = 0;
float averageUpdateTimeMs = 0;

char weaponOrderBackup[2][3] = { {0,0,0}, {0,0,0} };
char weapons[3];

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

		// ((int (*)(int, int, int, char *, int))0x00460a70)(1, 1, 0x80ffffff, "TESTING", -1);

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
	if (player->IsLocal) {
		weaponOrderBackup[player->LocalPlayerIndex][0] = playerDeobfuscate(&player->QuickSelect.Slot[0]);
		weaponOrderBackup[player->LocalPlayerIndex][1] = playerDeobfuscate(&player->QuickSelect.Slot[1]);
		weaponOrderBackup[player->LocalPlayerIndex][2] = playerDeobfuscate(&player->QuickSelect.Slot[2]);
		sce_printf("\nBackup 0: %d - %d", playerDeobfuscate(&player->QuickSelect.Slot[0]), weaponOrderBackup[player->LocalPlayerIndex][0]);
		sce_printf("\nBackup 1: %d - %d", playerDeobfuscate(&player->QuickSelect.Slot[1]), weaponOrderBackup[player->LocalPlayerIndex][1]);
		sce_printf("\nBackup 2: %d - %d", playerDeobfuscate(&player->QuickSelect.Slot[2]), weaponOrderBackup[player->LocalPlayerIndex][2]);
	}

	// call hooked WeaponStripMe function after backup
	playerStripWeapons(player);
}

void patchResurrectWeaponOrdering_HookGiveMeRandomWeapons(Player* player, int weaponCount)
{
	int i, j, matchCount = 0;

	// call hooked GiveMeRandomWeapons function first
	playerGiveRandomWeapons(player, weaponCount);

	// then try and overwrite given weapon order if weapons match equipped weapons before death
	if (player->IsLocal) {
		// restore backup if they match (regardless of order) newly assigned weapons
		for (i = 0; i < 3; i++) {
			u8 backedUpSlotValue = weaponOrderBackup[player->LocalPlayerIndex][i];
			for(j = 0; j < 3; j++) {
				if (backedUpSlotValue == playerDeobfuscate(&player->QuickSelect.Slot[j])) {
					sce_printf("\nMatched %d: %d - %d", j, playerDeobfuscate(&player->QuickSelect.Slot[j]), backedUpSlotValue);
					matchCount++;
				}
			}
		}

		// if we found a match, set
		if (matchCount == 3) {
			// set equipped weapon in order
			for (i = 3; i > 0; --i) {
				sce_printf("\nGive Weapon %d: %d - %d", i, playerDeobfuscate(&player->QuickSelect.Slot[i]), weaponOrderBackup[player->LocalPlayerIndex][i]);
				playerGiveWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][i]);
				playerEquipWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][0]);

			}

			// equip first slot weapon
			// sce_printf("\nEquiped 0: %d - %d", playerDeobfuscate(&player->QuickSelect.Slot[0]), weaponOrderBackup[player->LocalPlayerIndex][0]);
			// playerEquipWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][0]);
		}
	}
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

void runFpsCounter(void)
{
	char buf[64];
	static int lastGameTime = 0;
	static int tickCounter = 0;

	if (!isInGame())
		return;

	// initialize time
	if (tickCounter == 0 && lastGameTime == 0)
		lastGameTime = gameGetTime();
	
	// update fps every FPS frames
	++tickCounter;
	if (tickCounter >= GAME_FPS)
	{
		int currentTime = gameGetTime();
		lastFps = tickCounter / ((currentTime - lastGameTime) / (float)TIME_SECOND);
		lastGameTime = currentTime;
		tickCounter = 0;
	}

	// render if enabled
	if (enableFpsCounter)
	{
		if (averageRenderTimeMs > 0) {
			snprintf(buf, 64, "EE: %.1fms GS: %.1fms FPS: %.2f", &averageUpdateTimeMs, &averageRenderTimeMs, &lastFps);
		} else {
			snprintf(buf, 64, "FPS: %.2f", &lastFps);
		}

		gfxScreenSpaceText(SCREEN_WIDTH - 5, 5, 0.75, 0.75, 0x80FFFFFF, buf, -1, 2);
	}
}


void drawHook(void)
{
	static int renderTimeCounterMs = 0;
	static int frames = 0;
	static long ticksIntervalStarted = 0;

	long t0 = timerGetSystemTime();
	((void (*)(void))0x004552B8)();
	long t1 = timerGetSystemTime();

	int difference = t1 - t0;
	renderTimeMs = difference / SYSTEM_TIME_TICKS_PER_MS;

	renderTimeCounterMs += renderTimeMs;
	++frames;

	// update every 500 ms
	if ((t1 - ticksIntervalStarted) > (SYSTEM_TIME_TICKS_PER_MS * 500))
	{
		averageRenderTimeMs = renderTimeCounterMs / (float)frames;
		renderTimeCounterMs = 0;
		frames = 0;
		ticksIntervalStarted = t1;
	}
}

void updateHook(void)
{
	static int updateTimeCounterMs = 0;
	static int frames = 0;
	static long ticksIntervalStarted = 0;

	long t0 = timerGetSystemTime();
	((void (*)(void))0x005986b0)();
	long t1 = timerGetSystemTime();

	updateTimeMs = (t1-t0) / SYSTEM_TIME_TICKS_PER_MS;

	updateTimeCounterMs += updateTimeMs;
	frames++;

	// update every 500 ms
	if ((t1 - ticksIntervalStarted) > (SYSTEM_TIME_TICKS_PER_MS * 500))
	{
		averageUpdateTimeMs = updateTimeCounterMs / (float)frames;
		updateTimeCounterMs = 0;
		frames = 0;
		ticksIntervalStarted = t1;
	}
}


extern float _lodScale;
extern void* _correctTieLod;
int lastLodLevel = 2;

VariableAddress_t vaLevelOfDetailHook = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x004c9018,
	.Hoven = 0x004cb130,
	.OutpostX12 = 0x004c0a08,
    .KorgonOutpost = 0x004be1a0,
	.Metropolis = 0x004bd4f0,
	.BlackwaterCity = 0x004bad88,
	.CommandCenter = 0x004bad80,
    .BlackwaterDocks = 0x004bd600,
    .AquatosSewers = 0x004bc900,
    .MarcadiaPalace = 0x004bc280,
#else
	.Lobby = 0,
	.Bakisi = 0x004c68d8,
	.Hoven = 0x004c8930,
	.OutpostX12 = 0x004be248,
    .KorgonOutpost = 0x004bba60,
	.Metropolis = 0x004badb0,
	.BlackwaterCity = 0x004b85c8,
	.CommandCenter = 0x004b8780,
    .BlackwaterDocks = 0x004bafc0,
    .AquatosSewers = 0x004ba300,
    .MarcadiaPalace = 0x004b9c40,
#endif
};
void patchLevelOfDetail(void)
{
	if (!isInGame()) {
		lastLodLevel = -1;
		return;
	}

	if (*(u32*)GetAddress(&vaLevelOfDetailHook) == 0x02C3B020) {
		HOOK_J(GetAddress(&vaLevelOfDetailHook), &_correctTieLod);
		// patch jump instruction in correctTieLod to jump back to needed address.
		u32 val = ((u32)GetAddress(&vaLevelOfDetailHook) + 0x8);
		*(u32*)(&_correctTieLod + 4) = 0x08000000 | (val / 4);
	}

	int lod = 0;
	int lodChanged = lod != lastLodLevel;
	switch (lod) {
		case 0: _lodScale = 0.2; break;
		case 1: _lodScale = 0.4; break;
		case 2: _lodScale = 1.0; break;
		case 3: _lodScale = 5.0; break;
	}
}

int main()
{
	// run normal hook
	((void (*)(void))0x00126780)();

	uyaPreUpdate();

	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player * p = (Player*)PLAYER_STRUCT;
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0)
	{

	}

	patchLevelOfDetail();

    if (isInGame())
    {
		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// sce_printf("\nfunc: %x", &_correctTieLod);
		// sce_printf("\njump: %x", (u32)(&_correctTieLod + 4));

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

		// HOOK_JAL(0x004A84B0, &updateHook);
		// HOOK_JAL(0x00441CE8, &drawHook);
		// runFpsCounter();

		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, 4);
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
    	Debug();
    }
	
	// StartBots();

	uyaPostUpdate();
    return 0;
}
