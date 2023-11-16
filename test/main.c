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

VariableAddress_t vaMapScore_SelectBttn = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x005480fc,
    .Hoven = 0x0054a2c4,
    .OutpostX12 = 0x0053fb9c,
    .KorgonOutpost = 0x0053d284,
    .Metropolis = 0x0053c684,
    .BlackwaterCity = 0x00539e6c,
    .CommandCenter = 0x005396c4,
    .BlackwaterDocks = 0x0053bf44,
    .AquatosSewers = 0x0053b244,
    .MarcadiaPalace = 0x0053abc4,
#else
    .Lobby = 0,
    .Bakisi = 0x005457f4,
    .Hoven = 0x005478fc,
    .OutpostX12 = 0x0053d214,
    .KorgonOutpost = 0x0053a97c,
    .Metropolis = 0x00539d7c,
    .BlackwaterCity = 0x005374e4,
    .CommandCenter = 0x00536f14,
    .BlackwaterDocks = 0x00539754,
    .AquatosSewers = 0x00538a94,
    .MarcadiaPalace = 0x005383d4,
#endif
};
VariableAddress_t vaMapScore_SeigeCTFScoreboard_AlwaysRun = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004ae978,
    .Hoven = 0x004b0a90,
    .OutpostX12 = 0x004a6368,
    .KorgonOutpost = 0x004a3b00,
    .Metropolis = 0x004a2e50,
    .BlackwaterCity = 0x004a06e8,
    .CommandCenter = 0x004a06e0,
    .BlackwaterDocks = 0x004a2f60,
    .AquatosSewers = 0x004a2260,
    .MarcadiaPalace = 0x004a1be0,
#else
    .Lobby = 0,
    .Bakisi = 0x004ac428,
    .Hoven = 0x004ae480,
    .OutpostX12 = 0x004a3d98,
    .KorgonOutpost = 0x004a15b0,
    .Metropolis = 0x004a0900,
    .BlackwaterCity = 0x0049e118,
    .CommandCenter = 0x0049e2d0,
    .BlackwaterDocks = 0x004a0b10,
    .AquatosSewers = 0x0049fe50,
    .MarcadiaPalace = 0x0049f790,
#endif
};
VariableAddress_t vaMapScore_SeigeCTFMap_AlwaysRun = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004b16a8,
    .Hoven = 0x004b37c0,
    .OutpostX12 = 0x004a9098,
    .KorgonOutpost = 0x004a6830,
    .Metropolis = 0x004a5b80,
    .BlackwaterCity = 0x004a3418,
    .CommandCenter = 0x004a3410,
    .BlackwaterDocks = 0x004a5c90,
    .AquatosSewers = 0x004a4f90,
    .MarcadiaPalace = 0x004a4910,
#else
    .Lobby = 0,
    .Bakisi = 0x004af158,
    .Hoven = 0x004b11b0,
    .OutpostX12 = 0x004a6ac8,
    .KorgonOutpost = 0x004a42e0,
    .Metropolis = 0x004a3630,
    .BlackwaterCity = 0x004a0e48,
    .CommandCenter = 0x004a1000,
    .BlackwaterDocks = 0x004a3840,
    .AquatosSewers = 0x004a2b80,
    .MarcadiaPalace = 0x004a24c0,
#endif
};
VariableAddress_t vaMapScore_MapToggle = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004ae960,
    .Hoven = 0x004b0a78,
    .OutpostX12 = 0x004a6350,
    .KorgonOutpost = 0x004a3ae8,
    .Metropolis = 0x004a2e38,
    .BlackwaterCity = 0x004a06d0,
    .CommandCenter = 0x004a06c8,
    .BlackwaterDocks = 0x004a2f48,
    .AquatosSewers = 0x004a2248,
    .MarcadiaPalace = 0x004a1bc8,
#else
    .Lobby = 0,
    .Bakisi = 0x004ac410,
    .Hoven = 0x004ae468,
    .OutpostX12 = 0x004a3d80,
    .KorgonOutpost = 0x004a1598,
    .Metropolis = 0x004a08e8,
    .BlackwaterCity = 0x0049e100,
    .CommandCenter = 0x0049e2b8,
    .BlackwaterDocks = 0x004a0af8,
    .AquatosSewers = 0x0049fe38,
    .MarcadiaPalace = 0x0049f778,
#endif
};
VariableAddress_t vaMapScore_ScoreboardToggle = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004b1690,
    .Hoven = 0x004b37a8,
    .OutpostX12 = 0x004a9080,
    .KorgonOutpost = 0x004a6818,
    .Metropolis = 0x004a5b68,
    .BlackwaterCity = 0x004a3400,
    .CommandCenter = 0x004a33f8,
    .BlackwaterDocks = 0x004a5c78,
    .AquatosSewers = 0x004a4f78,
    .MarcadiaPalace = 0x004a48f8,
#else
    .Lobby = 0,
    .Bakisi = 0x004af140,
    .Hoven = 0x004b1198,
    .OutpostX12 = 0x004a6ab0,
    .KorgonOutpost = 0x004a42c8,
    .Metropolis = 0x004a3618,
    .BlackwaterCity = 0x004a0e30,
    .CommandCenter = 0x004a0fe8,
    .BlackwaterDocks = 0x004a3828,
    .AquatosSewers = 0x004a2b68,
    .MarcadiaPalace = 0x004a24a8,
#endif
};


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
		Active = 1;
		static int map = 1;
		((void (*)(int, int))GetAddress(&vaMapScore_MapToggle))(0, map);
		map = !map;

	}
	else if ((pad->btns & PAD_R3) == 0 && Active == 0)
	{
		Active = 1;
		static int score = 1;
		((void (*)(int, int))GetAddress(&vaMapScore_ScoreboardToggle))(0, score);
		score = !score;
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

int ping(void)
{
	static int ping_old = 0;
	static int myPing = 0;
	int ping_new = *(int*)0x001d3b5c;
	if (ping_old != ping_new) {
		myPing = ping_new - ping_old;
		ping_old = ping_new;
	}
	return myPing;
}

void showMapAndScore(void)
{
	// Top Function: map/scoreboard (Seige/CTF)
	// ((void (*)(int, int))0x00548208)(0, 10);

	// disable regular map/scoreboard toggle (select)
	*(u32*)GetAddress(&vaMapScore_SelectBttn) = 0;

	// Scoreboard (nested) (Seige/CTF)
	((void (*)(int, int))GetAddress(&vaMapScore_SeigeCTFScoreboard_AlwaysRun))(0, 10);
	// Map (nested) (Seige/CTF)
	((void (*)(int, int))GetAddress(&vaMapScore_SeigeCTFMap_AlwaysRun))(0, 10);
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

	//int rawrs = ping();
	//printf("\nping: %d", rawrs);

    if (isInGame())
    {
		Player * p = (Player*)PLAYER_STRUCT;
		if (!p)
			return 0;
	
		showMapAndScore();

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// playerSizeLogic();

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Allv2();

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// printf("\nfireDir: %x", (u32)((u32)&p->fireDir - (u32)PLAYER_STRUCT));
		// printf("\nmtxFxActive: %x", (u32)((u32)&p->mtxFxActive - (u32)PLAYER_STRUCT));
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
