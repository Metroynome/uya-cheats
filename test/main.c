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
extern VariableAddress_t vaPlayerObfuscateAddr;
extern VariableAddress_t vaPlayerObfuscateWeaponAddr;

char Teams[8];
short PlayerKills[GAME_MAX_PLAYERS];
short PlayerDeaths[GAME_MAX_PLAYERS];

VECTOR position;
VECTOR rotation;

static int SPRITE_ME = 0;
int enableFpsCounter = 1;
float lastFps = 0;
int renderTimeMs = 0;
float averageRenderTimeMs = 0;
int updateTimeMs = 0;
float averageUpdateTimeMs = 0;
int playerFov = 5;
int fovChange_Hook = 0;
int fovChange_Func = 0;
int VampireHeal[] = {3.78, 7, 12, 15};

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
		// Swap Teams
		int SetTeam = (player->mpTeam < 7) ? player->mpTeam + 1 : 0;
		playerSetTeam(player, SetTeam);
		// playerDecHealth(player, 1);
	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerDecHealth(player, 1);
	}
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		// static int Occlusion = (Occlusion == 2) ? 0 : 2;
		// gfxOcclusion(Occlusion);
		//spawnPointGetRandom(player, &position, &rotation);
		Player ** ps = playerGetAll();
		Player * p = ps[1];
		playerSetPosRot(player, &p->PlayerPosition, &p->PlayerRotation);
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

		// uiShowPopup(player, "HELLO MOMMY", 5);
		((void (*)())0x004661d0)();
		// FontWindow *buf;
		// buf->win_top = 0x87;
		// buf->win_bot = 0x1d;
		// buf->win_left = 0xce;
		// buf->win_right = 0x174;
		// buf->text_x = 1;
		// buf->text_y = 1;
		// buf->max_width = 0;
		// buf->max_height = 0;
		// buf->line_spacing = 0;
		// buf->flags = 0;
		// buf->sub_pixel_x = 0;
		// buf->sub_pixel_y = 0;
		// buf->drop_shadow_offset_x = 2;
		// buf->drop_shadow_offset_y = 2;
		// SetFont
		// ((void (*)(int))0x0045dc68)(1);
		// ((void (*)(float, float, int, int, const char*, int, int))0x0045f530)(0x3f800000,0x3f800000,&buf,0x8066ccff,"TESTING BOXY",0xffffffffffffffff,0x80000000);
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
		// ((int (*)(int, int, int))0x00685798)(0x21, 0, 0);
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
		// wad, 1, stack
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
            (float)player->move.behavior[2] = 0.125;
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

void Allv2()
{
	int i;
	// Give all weapons but Holos, and upgrade.
	for (i = 1; i < 10; ++i) {
		playerGiveWeapon(PLAYER_STRUCT, i);
		playerGiveWeaponUpgrade(PLAYER_STRUCT, i);
	};
	// give holo
	playerGiveWeapon(PLAYER_STRUCT, WEAPON_ID_HOLO);
}

void Test_Sprites(float x, float y, float scale)
{
	float small = scale * 0.75;
	float delta = (scale - small) / 2;

	gfxSetupGifPaging(0);
	u64 dreadzoneSprite = gfxGetFrameTex(SPRITE_ME);
	// gfxDrawSprite(x+2, y+2, scale, scale, 0, 0, 32, 32, 0x40000000, dreadzoneSprite);
	gfxDrawSprite(x,   y,   scale, scale, 0, 0, 32, 32, 0x80C0C0C0, dreadzoneSprite);
	gfxDrawSprite(x+delta, y+delta, small, small, 0, 0, 32, 32, 0x80000040, dreadzoneSprite);
	gfxDoGifPaging();
}

void drawEffectQuad(VECTOR position, int texId, float scale)
{
	struct QuadDef quad;
	MATRIX m2;
	VECTOR t;
	VECTOR pTL = {0.5,0,0.5,1};
	VECTOR pTR = {-0.5,0,0.5,1};
	VECTOR pBL = {0.5,0,-0.5,1};
	VECTOR pBR = {-0.5,0,-0.5,1};

	// determine color
	u32 color = 0x80FFFFFF;

	// set draw args
	matrix_unit(m2);

	// init
	gfxResetQuad(&quad);

	// color of each corner?
	vector_copy(quad.VertexPositions[0], pTL);
	vector_copy(quad.VertexPositions[1], pTR);
	vector_copy(quad.VertexPositions[2], pBL);
	vector_copy(quad.VertexPositions[3], pBR);
	quad.VertexColors[0] = quad.VertexColors[1] = quad.VertexColors[2] = quad.VertexColors[3] = color;
	quad.VertexUVs[0] = (struct UV){0,0};
	quad.VertexUVs[1] = (struct UV){1,0};
	quad.VertexUVs[2] = (struct UV){0,1};
	quad.VertexUVs[3] = (struct UV){1,1};
	quad.Clamp = 0;
	quad.Tex0 = gfxGetEffectTex(texId, 1);
	quad.Tex1 = 0xFF9000000260;
	quad.Alpha = 0x8000000044;

	GameCamera* camera = cameraGetGameCamera(0);
	if (!camera)
		return;

	// get forward vector
	vector_subtract(t, camera->pos, position);
	t[2] = 0;
	vector_normalize(&m2[4], t);

	// up vector
	m2[8 + 2] = 1;

	// right vector
	vector_outerproduct(&m2[0], &m2[4], &m2[8]);

	t[0] = t[1] = t[2] = scale;
	t[3] = 1;
	matrix_scale(m2, m2, t);

	// position
	memcpy(&m2[12], position, sizeof(VECTOR));

	// draw
	gfxDrawQuad((void*)0x00222590, &quad, m2, 1);
}

void disableRespawning(void)
{
    VariableAddress_t vaDM_RespawnUpdater = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004b21f4,
        .Hoven = 0x004b430c,
        .OutpostX12 = 0x004a9be4,
        .KorgonOutpost = 0x004a737c,
        .Metropolis = 0x004a66cc,
        .BlackwaterCity = 0x004a3f64,
        .CommandCenter = 0x004a3f5c,
        .BlackwaterDocks = 0x004a67dc,
        .AquatosSewers = 0x004a5adc,
        .MarcadiaPalace = 0x004a545c,
#else
        .Lobby = 0,
        .Bakisi = 0x004afca4,
        .Hoven = 0x004b1cfc,
        .OutpostX12 = 0x004a7614,
        .KorgonOutpost = 0x004a4e2c,
        .Metropolis = 0x004a417c,
        .BlackwaterCity = 0x004a1994,
        .CommandCenter = 0x004a1b4c,
        .BlackwaterDocks = 0x004a438c,
        .AquatosSewers = 0x004a36cc,
        .MarcadiaPalace = 0x004a300c,
#endif
    };
	// Disable Timer and respawn text.
    int RespawnUpdater = GetAddress(&vaDM_RespawnUpdater);
    if (*(u32*)RespawnUpdater != 0)
        *(u32*)RespawnUpdater = 0;

	// Disable Respawn Function
	int RespawnFunc = GetAddress(&vaPlayerRespawnFunc);
    if (*(u32*)RespawnFunc != 0)
	{
		*(u32*)RespawnFunc = 0x03e00008;
        *(u32*)(RespawnFunc + 0x4) = 0;
	}
}

void survivor(void)
{
	disableRespawning();

    static int DeadPlayers = 0;
	static int TeamCount = 0;
    int i;
	Player ** players = playerGetAll();
	GameData * gameData = gameGetData();
    GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	int teams = gameOptions->GameFlags.MultiplayerGameFlags.Teams;
    int playerCount = gameSettings->PlayerCount;
	if (teams) {
		if (!TeamCount) {
			for (i = 0; i < playerCount; ++i) {
				if (!players[i])
					continue;

				++Teams[players[i]->mpTeam];
				++TeamCount;
			}
		}
		for (i = 0; i < playerCount; ++i) {
			if (!players[i])
				continue;
					
		    // Save current deaths for all players, and how many players have died.
			if (gameData->PlayerStats[i].Deaths > PlayerDeaths[i]) {
				PlayerDeaths[i] = gameData->PlayerStats[i].Deaths;
				// Subtract player from their team.
				--Teams[players[i]->mpTeam];
				// if the team of the player who died noe equals zero, subject team coutn.
				if (Teams[players[i]->mpTeam] == 0)
					--TeamCount;
			}
			// If only one player in game, don't let game end until they die.
			if (playerCount == 1 && TeamCount == 0) {
				gameData->TimeEnd = 0;
				gameData->WinningTeam = i;
			}
			// if only one team remains
			else if (playerCount > 1 && TeamCount == 1 && Teams[i] != 0) {
				gameData->TimeEnd = 0;
                gameData->WinningTeam = i;
			}
		}
	}
	else {
		for (i = 0; i < (playerCount - 1); ++i) {
			// Save current deaths for all players, and how many players have died.
			if (gameData->PlayerStats[i].Deaths > PlayerDeaths[i]) {
				PlayerDeaths[i] = gameData->PlayerStats[i].Deaths;
				++DeadPlayers;
			}

			// If only one player in game, don't let game end until they die.
			if (playerCount == 1 && DeadPlayers == 1) {
				gameData->TimeEnd = 0;
				gameData->WinningTeam = i;
			}
			// if player count is greater than 1, and Dead Players == Player Count - 1
			else if (playerCount > 1 && DeadPlayers == (playerCount - 1)) {
				// Check to see who has not died
				if (gameData->PlayerStats[i].Deaths == 0) {
					gameData->TimeEnd = 0;
					gameData->WinningTeam = i;
				}
			}
		}
	}
}

void patchDeathBarrierBug(void)
{
	int i;
	// Grab All Players
	Player** players = playerGetAll();
	// Cycle through all
	for (i = 0; i < players[i]; ++i) {
		Player* player = players[i];
		// if player is local
		if (player && playerIsLocal(player)) {
			float deathbarrier = gameGetDeathHeight();
			float pY = player->PlayerPosition[2];
			DPRINTF("deathheight: %d\nplayery: %d\ninbasehack: %d\n", (int)deathbarrier, (int)pY, player->InBaseHack);
			// if player is above death barrier and inBaseHack equals 1.
			if (player->InBaseHack && deathbarrier < pY) {
				player->InBaseHack = 0;
			} else if (!player->InBaseHack && deathbarrier > pY) {
				player->InBaseHack = 1;
			}
		}
	}
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
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0)
	{
		// gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_SmallTurrets = 0;
	}

    if (isInGame())
    {
		Player * p = (Player*)PLAYER_STRUCT;
		if (!p)
			return 0;

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// playerSizeLogic();
		patchDeathBarrierBug();

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Allv2();

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// printf("\nfireDir: %x", (u32)((u32)&p->fireDir - (u32)PLAYER_STRUCT));
		// printf("\nweaponPosRec: %x", (u32)((u32)&p->weaponPosRec - (u32)PLAYER_STRUCT));
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
    } else {
		DebugInMenus();
	}

	// StartBots();
	// runSpectate();

	uyaPostUpdate();
    return 0;
}
