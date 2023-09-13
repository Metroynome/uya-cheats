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

void StartBots(void);

extern VariableAddress_t vaPlayerRespawnFunc;

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

int playerFov = 5;

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
		// int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
		// playerSetTeam(player, SetTeam);

		playerFov = playerFov - 1;

	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        // playerDecHealth(player, 14);
		// playerSetHealth(player, clamp(((int)player->pNetPlayer->pNetPlayerData->hitPoints + VampireHealRate[1]), 0, PLAYER_MAX_HEALTH));
		playerFov = playerFov + 1;
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

VariableAddress_t vaFieldOfView_FluxRA = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x0040682c,
	.Hoven = 0x00406194,
	.OutpostX12 = 0x003fe08c,
    .KorgonOutpost = 0x003fd46c,
	.Metropolis = 0x003fc02c,
	.BlackwaterCity = 0x003f8e14,
	.CommandCenter = 0x0040721c,
    .BlackwaterDocks = 0x0040917c,
    .AquatosSewers = 0x00408d84,
    .MarcadiaPalace = 0x00407dfc,
#else
	.Lobby = 0,
	.Bakisi = 0x004061c4,
	.Hoven = 0x00405aac,
	.OutpostX12 = 0x003fd9a4,
    .KorgonOutpost = 0x003fcde4,
	.Metropolis = 0x003fb9c4,
	.BlackwaterCity = 0x003f874c,
	.CommandCenter = 0x00406b9c,
    .BlackwaterDocks = 0x00408afc,
    .AquatosSewers = 0x00408704,
    .MarcadiaPalace = 0x0040777c,
#endif
};
void writeFov(int cameraIdx, int a1, int a2, u32 ra, float fov, float f13, float f14, float f15)
{
	static float lastFov = 0;

	GameCamera* camera = cameraGetGameCamera(cameraIdx);
	if (!camera)
		return;

	// save last fov
	// or reuse last if fov passed is 0
	if (fov > 0)
		lastFov = fov;
	else if (lastFov > 0)
		fov = lastFov;
	else
		fov = lastFov = camera->fov.ideal;

	// apply our fov modifier
	// only if not scoping with sniper
	if (ra != GetAddress(&vaFieldOfView_FluxRA))
		fov += (playerFov / 10.0) * 1;

	if (a2 > 2) {
		if (a2 != 3) return;
		camera->fov.limit = f15;
		camera->fov.changeType = a2;
		camera->fov.ideal = fov;
		camera->fov.state = 1;
		camera->fov.gain = f13;
		camera->fov.damp = f14;
		return;
	}
	else if (a2 < 1) {
		if (a2 != 0) return;
		camera->fov.ideal = fov;
		camera->fov.changeType = 0;
		camera->fov.state = 1;
		return;
	}

	if (a1 == 0) {
		camera->fov.ideal = fov;
		camera->fov.changeType = 0;
	}
	else {
		camera->fov.changeType = a2;
		camera->fov.init = camera->fov.actual;
		camera->fov.timer = (short)a2;
		camera->fov.timerInv = 1.0 / (float)a2;
	}
	camera->fov.state = 1;
}

/*
 * NAME :		patchFov
 * 
 * DESCRIPTION :
 * 			Installs SetFov override hook.
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
VariableAddress_t vaFieldOfView_Hook = {
#if UYA_PAL
	.Lobby = 0,
	.Bakisi = 0x004452e0,
	.Hoven = 0x00446e60,
	.OutpostX12 = 0x0043dc60,
    .KorgonOutpost = 0x0043b820,
	.Metropolis = 0x0043ab60,
	.BlackwaterCity = 0x00438360,
	.CommandCenter = 0x00438fe0,
    .BlackwaterDocks = 0x0043b860,
    .AquatosSewers = 0x0043ab60,
    .MarcadiaPalace = 0x0043a4e0,
#else
	.Lobby = 0,
	.Bakisi = 0x00444468,
	.Hoven = 0x00445f28,
	.OutpostX12 = 0x0043cd68,
    .KorgonOutpost = 0x0043a9a8,
	.Metropolis = 0x00439ce8,
	.BlackwaterCity = 0x00437468,
	.CommandCenter = 0x004382a8,
    .BlackwaterDocks = 0x0043aae8,
    .AquatosSewers = 0x00439e28,
    .MarcadiaPalace = 0x00439768,
#endif
};
void patchFov(void)
{
	static int ingame = 0;
	static int lastFov = 0;
	if (!isInGame()) {
		ingame = 0;
		return;
	}

	// replace SetFov function
	HOOK_J(GetAddress(&vaFieldOfView_Hook), &writeFov);
	POKE_U32((u32)GetAddress(&vaFieldOfView_Hook) + 0x4, 0x03E0382d);

	// initialize fov at start of game
	if (!ingame || lastFov != playerFov) {
		GameCamera* camera = cameraGetGameCamera(0);
		if (!camera)
			return;

		writeFov(0, 0, 3, 0, 0, 0.05, 0.2, 0);
		lastFov = playerFov;
		ingame = 1;
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
		// gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 0;
	}

	patchFov();
	
    if (isInGame())
    {
		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// disableDrones();
		// vampireLogic(VampireHealRate[0]);
		// hideRadarBlips();

		// printf("\nPlayer State: %d", playerDeobfuscate(&p->State, 0, 0));
		// printf("\nstickRawAngle: %x", (u32)((u32)&p->stickRawAngle - (u32)PLAYER_STRUCT));
		// printf("\npnetplayer: %x", (u32)((u32)&p->pNetPlayer - (u32)PLAYER_STRUCT));
		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, 4);
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
