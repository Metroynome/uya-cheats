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
#include <libuya/map.h>
#include <libuya/collision.h>
#include <libuya/guber.h>

void StartBots(void);

#define HBOLT_MOBY_OCLASS			(0x3004)
#define HBOLT_PICKUP_RADIUS			(3)
#define HBOLT_PARTICLE_COLOR        (0x00ff608f)
#define HBOLT_SPRITE_COLOR          (0x00411313)
#define HBOLT_SCALE                 (1)

struct HBoltPVar {
	int DestroyAtTime;
	struct PartInstance* Particles[4];
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
		// ((void (*)(float, float, float))0x0049e2d8)(0, .13, 1);
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

// void CreateParticle(void)
// {	Moby * p = playerGetFromSlot(0)->PlayerMoby;
// 	VECTOR v = {2, 2, 2, 2};
// 	((void (*)(long, int, int, int, int))0x0049fb80)(p, p, v, 0x00246320, 1);
// }

VariableAddress_t vaSpawnPart_59 = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x004a0508,
    .Hoven = 0x004a2620,
    .OutpostX12 = 0x00497ef8,
    .KorgonOutpost = 0x00495690,
    .Metropolis = 0x004949e0,
    .BlackwaterCity = 0x00492278,
    .CommandCenter = 0x00492270,
    .BlackwaterDocks = 0x00494af0,
    .AquatosSewers = 0x00493df0,
    .MarcadiaPalace = 0x00493770,
#else
    .Lobby = 0,
    .Bakisi = 0x0049e150,
    .Hoven = 0x004a01a8,
    .OutpostX12 = 0x00495ac0,
    .KorgonOutpost = 0x004932d8,
    .Metropolis = 0x00492628,
    .BlackwaterCity = 0x0048fe40,
    .CommandCenter = 0x0048fff8,
    .BlackwaterDocks = 0x00492838,
    .AquatosSewers = 0x00491b78,
    .MarcadiaPalace = 0x004914b8,
#endif
};

struct PartInstance * scavHuntSpawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))GetAddress(&vaSpawnPart_59))(position, color, opacity, 0x35, 1, 2, 0, 0, idx);
}

void scavHuntHBoltUpdate(Moby* moby)
{
	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };
	VECTOR t;
	int i;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	if (!pvars)
		return;

	// handle particles
	u32 color = colorLerp(0, HBOLT_PARTICLE_COLOR, 1.0 / 4);
	color |= 0x80000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			particle = scavHuntSpawnParticle(moby->Position, color, opacities[i], i);
		}

		// update
		if (particle) {
			particle->rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
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
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0) {
		gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_SmallTurrets = 0;
		gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_Bots = 0;
		gameOptions->GameFlags.MultiplayerGameFlags.BaseDefense_GatlinTurrets = 0;
	}

	//int rawrs = ping();
	//printf("\nping: %d", rawrs);

    if (isInGame())
    {
		Player * p = (Player*)PLAYER_STRUCT;
		if (!p)
			return 0;

		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		scavHuntHBoltUpdate(p->PlayerMoby);

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
	// scavHuntRun();

	uyaPostUpdate();
    return 0;
}
