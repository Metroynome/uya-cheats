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
#include <libuya/camera.h>
#include <libuya/gameplay.h>

void StartBots(void);

extern VariableAddress_t vaPlayerRespawnFunc;
extern VariableAddress_t vaPlayerSetPosRotFunc;
extern VariableAddress_t vaGameplayFunc;
extern VariableAddress_t vaGiveWeaponFunc;

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

int enableFpsCounter = 1;
float lastFps = 0;
int renderTimeMs = 0;
float averageRenderTimeMs = 0;
int updateTimeMs = 0;
float averageUpdateTimeMs = 0;
int playerFov = 5;
int fovChange_Hook = 0;
int fovChange_Func = 0;

void DebugInGame()
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
		int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
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
		// Allv2();
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		// Set Gattling Turret Health to 1.
		DEBUGsetGattlingTurretHealth();
		// setGattlingTurretHealth(1);
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

void DebugInMenus(void)
{
	static int Active = 0;
	Player * player = (Player*)PLAYER_STRUCT;
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
    /*
		DEBUG_InMenus OPTIONS:
	*/
    if ((pad->btns & PAD_LEFT) == 0 && Active == 0)
	{
        Active = 1;
		((int (*)(int, int, int))0x00685798)(0x21, 0, 0);
	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
    }
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		// kick
		// 6bec18 jal v0
		// ((int (*)(int, int, int, int))0x006c0c60)(, 1, 0, 0x1600);
		// leave game
		// ((int (*)(int, int, int, int))0x006c0c60)(, 0, 1, -1);
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		Active = 1;
	}
	else if ((pad->btns & PAD_L3) == 0 && Active == 0)
	{
		Active = 1;
	}
	else if ((pad->btns & PAD_R3) == 0 && Active == 0)
	{
		Active = 1;
	}
    if (!(pad->btns & PAD_LEFT) == 0 && !(pad->btns & PAD_RIGHT) == 0 && !(pad->btns & PAD_UP) == 0 && !(pad->btns & PAD_DOWN) == 0 && !(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0)
    {
        Active = 0;
    }
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

		if (player->timers.IsChargebooting == 1 && playerPadGetButton(player, PAD_R2) > 0 && player->timers.state > 55)
			player->timers.state = 55;
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

// void Allv2()
// {
// 	int i;
// 	// Give all weapons but Holos, and upgrade.
// 	for (i = 1; i < 10; ++i) {
// 		playerGiveWeapon(PLAYER_STRUCT, i);
// 		playerGiveWeaponUpgrade(PLAYER_STRUCT, i);
// 	};
// 	// give holo
// 	playerGiveWeapon(PLAYER_STRUCT, WEAPON_ID_HOLO);
// }


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

int patchUnkick_Logic(u32 a0, int a1)
{
	int i;
	GameSettings * gs = gameGetSettings();
	if (!gs) {
		int clientId = gameGetMyClientId();
		int popup = uiGetActiveSubPointer(UIP_UNK_POPUP);
		for (i = 1; i < GAME_MAX_PLAYERS; ++i) {
			if (gs->PlayerClients[i] == clientId && gs->PlayerStates[i] == 5) {
				return ((int (*)(u32, int, int, int))0x006c0c60)(a0, 1, 0, 0x1600);
				// if (popup != 0 && *(u32*)((u32)popup + 0x32c) != 0x64656B63) {
			}
		}
	}
	return ((int (*)(u32, int))0x006bec18)(a0, a1);
}

void patchUnkick(void)
{
	HOOK_JAL(0x00683a10, &patchUnkick_Logic);
}

void v2_logic(void)
{
	Player ** players = playerGetAll();
	int i, j;
	for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
		Player * player = players[i];
		if (!player)
			return;

		for(j = 1; j < 10; ++j)
			playerGiveWeaponUpgrade(player, j);
	}
}

void v2_hook()
{
		static int GaveV2s = 0;
		if (GaveV2s)
			return;

		int GiveWeapon_JRRA = (u32)GetAddress(&vaGiveWeaponFunc) + 0x538;
		if (*(u32*)GiveWeapon_JRRA == 0x03e00008)
			HOOK_J(GiveWeapon_JRRA, &v2_logic);
		
		v2_logic();
		GaveV2s = 1;
}

int main()
{
	// run normal hook
	((void (*)(void))0x00126780)();

	if (!musicIsLoaded())
		return 1;

	uyaPreUpdate();

	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	Player * p = (Player*)PLAYER_STRUCT;
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0)
	{
		// gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 0;
	}

    if (isInGame())
    {

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// vampireLogic(VampireHealRate[0]);
		// hideRadarBlips();

		// printf("\nPlayer State: %d", playerDeobfuscate(&p->State, 0, 0));
		// printf("\nstickRawAngle: %x", (u32)((u32)&p->stickRawAngle - (u32)PLAYER_STRUCT));
		// printf("\npnetplayer: %x", (u32)((u32)&p->pNetPlayer - (u32)PLAYER_STRUCT));
		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, 4);
		
		v2_hook();

		InfiniteChargeboot();
		InfiniteHealthMoonjump();
    	DebugInGame();

		// float high = 340282346638528859811704183484516925440.00;
		// float low = -340282346638528859811704183484516925440.00;
		// p->fps.Vars.CameraYaw.target_slowness_factor_quick = 0;
		// p->fps.Vars.CameraYaw.target_slowness_factor_aim = high;
		// p->fps.Vars.CameraPitch.target_slowness_factor = 0;
		// p->fps.Vars.CameraPitch.strafe_turn_factor = high;
		// // p->fps.Vars.CameraPitch.strafe_tilt_factor = high;
		// p->fps.Vars.CameraPitch.max_target_angle = high;
    } else if (isInMenus()) {
		// patchUnkick();
		DebugInMenus();
	}
	
	// StartBots();

	uyaPostUpdate();
    return 0;
}
