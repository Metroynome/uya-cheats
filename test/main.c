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
#include <libuya/sound.h>

void StartBots(void);

#define HBOLT_MOBY_OCLASS			(0x1C0D)
#define HBOLT_PICKUP_RADIUS			(3)
#define HBOLT_PARTICLE_COLOR        (0x0000ffff)
#define HBOLT_SPRITE_COLOR          (0x0013141)
#define HBOLT_SCALE                 (1)

#define HBOLT_MIN_SPAWN_DELAY_TPS   (60 * 10)
#define HBOLT_MAX_SPAWN_DELAY_TPS   (60 * 20)
#define HBOLT_SPAWN_PROBABILITY     (0.25)

float scavHuntSpawnFactor = 1;
float scavHuntSpawnTimerFactor = 0.5;
int scavHuntShownPopup = 0;
int scavHuntHasGotSettings = 0;
int scavHuntEnabled = 0;
int scavHuntInitialized = 0;
int scavHuntBoltSpawnCooldown = 0;

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
extern VariableAddress_t vaGetFrameTex;
extern VariableAddress_t vaGetEffectTex;

char Teams[8];
short PlayerKills[GAME_MAX_PLAYERS];
short PlayerDeaths[GAME_MAX_PLAYERS];

VECTOR position;
VECTOR rotation;

static int SPRITE_ME = 0;
static int SOUND_ME = 0;
static int SOUND_ME_FLAG = 3;

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
		// spawnPointGetRandom(player, &position, &rotation);
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
		scavHuntSpawnRandomNearPlayer(0);
		// Player * par = playerGetFromSlot(0)->PlayerPosition;
		// soundPlayByOClass(SOUND_ME_FLAG, 0, par, SOUND_ME);
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

VariableAddress_t vaDeletePart = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x00496c58,
    .Hoven = 0x00498d70,
    .OutpostX12 = 0x0048e648,
    .KorgonOutpost = 0x0048bd18,
    .Metropolis = 0x0048b130,
    .BlackwaterCity = 0x004889c8,
    .CommandCenter = 0x004889c0,
    .BlackwaterDocks = 0x0048b240,
    .AquatosSewers = 0x0048a540,
    .MarcadiaPalace = 0x00489ec0,
#else
    .Lobby = 0,
    .Bakisi = 0x00494a60,
    .Hoven = 0x00496ab8,
    .OutpostX12 = 0x0048c3d0,
    .KorgonOutpost = 0x00489b20,
    .Metropolis = 0x00488f38,
    .BlackwaterCity = 0x00486750,
    .CommandCenter = 0x00486908,
    .BlackwaterDocks = 0x00489148,
    .AquatosSewers = 0x00488488,
    .MarcadiaPalace = 0x00487dc8,
#endif
};

VariableAddress_t vaReplace_GetEffectTexJAL = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x0045b2f0,
    .Hoven = 0x0045ce70,
    .OutpostX12 = 0x00453c70,
    .KorgonOutpost = 0x00451830,
    .Metropolis = 0x00450b70,
    .BlackwaterCity = 0x0044e370,
    .CommandCenter = 0x0044eff0,
    .BlackwaterDocks = 0x00451870,
    .AquatosSewers = 0x00450b70,
    .MarcadiaPalace = 0x004504f0,
#else
    .Lobby = 0,
    .Bakisi = 0x0045a220,
    .Hoven = 0x0045bce0,
    .OutpostX12 = 0x00452b20,
    .KorgonOutpost = 0x00450760,
    .Metropolis = 0x0044faa0,
    .BlackwaterCity = 0x0044d220,
    .CommandCenter = 0x0044e060,
    .BlackwaterDocks = 0x004508a0,
    .AquatosSewers = 0x0044fbe0,
    .MarcadiaPalace = 0x0044f520,
#endif
};

void scavHuntResetBoltSpawnCooldown(void)
{
  scavHuntBoltSpawnCooldown = randRangeInt(HBOLT_MIN_SPAWN_DELAY_TPS, HBOLT_MAX_SPAWN_DELAY_TPS) / scavHuntSpawnTimerFactor;
}

struct PartInstance * scavHuntSpawnParticle(VECTOR position, u32 color, char opacity, int idx)
{
	return ((struct PartInstance* (*)(VECTOR, u32, char, u32, u32, int, int, int, float))GetAddress(&vaSpawnPart_59))(position, color, opacity, 0x35, 1, 2, 0, 0, idx);
}

void scavHuntDestroyParticle(struct PartInstance* particle)
{
	((void (*)(struct PartInstance*))GetAddress(&vaDeletePart))(particle);
}

void scavHuntHBoltPostDraw(Moby* moby)
{
	printf("\npost draw function");
	// struct QuadDef quad;
	// MATRIX m2;
	// VECTOR t;
  int opacity = 0x80;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	if (!pvars)
		return;

	// determine color
	u32 color = HBOLT_SPRITE_COLOR;

	// fade as we approach destruction
	int timeUntilDestruction = (pvars->DestroyAtTime - gameGetTime()) / TIME_SECOND;
	if (timeUntilDestruction < 1)
		timeUntilDestruction = 1;

	if (timeUntilDestruction < 10) {
		float speed = timeUntilDestruction < 3 ? 20.0 : 3.0;
		float pulse = (1 + sinf(clampAngle((gameGetTime() / 1000.0) * speed))) * 0.5;
		opacity = 32 + (pulse * 96);
	}

  opacity = opacity << 24;
  color = opacity | (color & HBOLT_SPRITE_COLOR);
  moby->PrimaryColor = color;

  u32 hook = (u32)GetAddress(&vaReplace_GetEffectTexJAL) + 0x20;
  HOOK_JAL(hook, GetAddress(&vaGetFrameTex));
  gfxDrawBillboardQuad(0.55, 0, MATH_PI, moby->Position, 81, opacity, 0);
  gfxDrawBillboardQuad(0.5, 0.01, MATH_PI, moby->Position, 81, color, 0);
  HOOK_JAL(hook, GetAddress(&vaGetEffectTex));
}

void scavHuntHBoltUpdate(Moby* moby)
{
	printf("\nupdate function");
	const float rotSpeeds[] = { 0.05, 0.02, -0.03, -0.1 };
	const int opacities[] = { 64, 32, 44, 51 };
	VECTOR t;
	int i;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
	if (!pvars)
		return;
printf("\nbefore sticky fx");
  gfxStickyFX(&scavHuntHBoltPostDraw, moby);
printf("\nafter sticky fx");

	// handle particles
	u32 color = colorLerp(0, HBOLT_PARTICLE_COLOR, 1.0 / 4);
	color |= 0x80000000;
	for (i = 0; i < 4; ++i) {
		struct PartInstance * particle = pvars->Particles[i];
		if (!particle) {
			particle = scavHuntSpawnParticle(moby->Position, color, 100, i);
		}

		// update
		if (particle) {
			particle->rot = (int)((gameGetTime() + (i * 100)) / (TIME_SECOND * rotSpeeds[i])) & 0xFF;
		}
	}

  // handle pickup
  for (i = 0; i < GAME_MAX_LOCALS; ++i) {
    Player* p = playerGetFromSlot(i);
    if (!p || playerIsDead(p)) continue;

    vector_subtract(t, p->PlayerPosition, moby->Position);
    if (vector_sqrmag(t) < (HBOLT_PICKUP_RADIUS * HBOLT_PICKUP_RADIUS)) {
      uiShowPopup(0, "You found a Horizon Bolt!\x0", 3);
      soundPlayByOClass(2, 0, moby, MOBY_ID_OMNI_SHIELD);
      // scavHuntSendHorizonBoltPickedUpMessage();
      scavHuntHBoltDestroy(moby);
      break;
    }
  }

	// handle auto destruct
	if (pvars->DestroyAtTime && gameGetTime() > pvars->DestroyAtTime) {
		scavHuntHBoltDestroy(moby);
	}
}

void scavHuntSpawn(VECTOR position)
{
  Moby* moby = mobySpawn(HBOLT_MOBY_OCLASS, sizeof(struct HBoltPVar));
  if (!moby) return;

  moby->PUpdate = &scavHuntHBoltUpdate;
  vector_copy(moby->Position, position);
  moby->UpdateDist = -1;
  moby->Drawn = 1;
  moby->Opacity = 0x00;
  moby->DrawDist = 0x00;

  // update pvars
  struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;
  // pvars->DestroyAtTime = gameGetTime() + (TIME_SECOND * 30);
  memset(pvars->Particles, 0, sizeof(pvars->Particles));

  // mobySetState(moby, 0, -1);
  scavHuntResetBoltSpawnCooldown();
  soundPlayByOClass(1, 0, moby, MOBY_ID_OMNI_SHIELD);
  printf("hbolt spawned at %08X destroyAt:%d %04X\n", (u32)moby, pvars->DestroyAtTime, moby->ModeBits);
}

void scavHuntSpawnRandomNearPosition(VECTOR position)
{
  // try 4 times to generate a random point near given position
  int i = 0;
  while (i < 4)
  {
    // generate random position
    VECTOR from = {0,0,0,0}, to = {0,0,-6,0}, p = {0,0,1,0};
    float theta = randRadian();
    float radius = randRange(5, 8);
    vector_fromyaw(from, theta);
    vector_scale(from, from, radius);
    from[2] = 3;
    vector_add(from, from,  position);
    vector_add(to, to, from);

	vector_add(p, p, CollLine_Fix_GetHitPosition());
	scavHuntSpawn(from);
  	break;

    // snap to ground
    // and check if ground is walkable
    if (CollLine_Fix(from, to, 0, NULL, NULL)) {
      printf("\nSpawning Try: %d", i + 1);
      int colId = CollHotspot();
      if (colId == 1 || colId == 2 || colId == 4 || colId == 7 || colId == 9 || colId == 10) {
        printf("\ncollision id: %d", colId);
        vector_add(p, p, CollLine_Fix_GetHitPosition());
        scavHuntSpawn(p);
        break;
      }
    }
    ++i;
  }
}

void scavHuntSpawnRandomNearPlayer(int pIdx)
{
	Player** players = playerGetAll();
	Player* player = players[pIdx];
	if (!player) return;

	scavHuntSpawnRandomNearPosition(player->PlayerPosition);
}

void scavHuntHBoltDestroy(Moby* moby)
{
  if (mobyIsDestroyed(moby)) return;
	struct HBoltPVar* pvars = (struct HBoltPVar*)moby->PVar;

	// destroy particles
  int i;
	for (i = 0; i < 4; ++i) {
		if (pvars->Particles[i]) {
			scavHuntDestroyParticle(pvars->Particles[i]);
			pvars->Particles[i] = NULL;
		}
	}

  mobyDestroy(moby);
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

		// scavHuntHBoltUpdate(p->PlayerMoby);

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		// Allv2();

		// Test_Sprites(SCREEN_WIDTH * 0.3, SCREEN_HEIGHT * .50, 100);

		// printf("\nground: %x", (u32)((u32)&p->ground - (u32)PLAYER_STRUCT));
		// printf("\nground->ground.pMoby: %x", (u32)((u32)&p->ground.pMoby - (u32)PLAYER_STRUCT));
		// printf("\nmtxFxActive: %x", (u32)((u32)&p->mtxFxActive - (u32)PLAYER_STRUCT));
		// printf("\npnetplayer: %x", (u32)((u32)&p->pNetPlayer - (u32)PLAYER_STRUCT));

		// float x = SCREEN_WIDTH * 0.3;
		// float y = SCREEN_HEIGHT * 0.85;
		// gfxScreenSpaceText(x, y, 1, 1, 0x80FFFFFF, "TEST YOUR MOM FOR HUGS", -1, 4);
		
		// printf("Collision: %d\n", CollHotspot());

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
