#include <tamtypes.h>
#include <string.h>

#include <libuya/time.h>
#include <libuya/game.h>
#include <libuya/gamesettings.h>
#include <libuya/player.h>
#include <libuya/camera.h>
#include <libuya/weapon.h>
#include <libuya/sha1.h>
#include <libuya/collision.h>
#include <libuya/stdio.h>
#include <libuya/graphics.h>
#include <libuya/spawnpoint.h>
#include <libuya/random.h>
#include <libuya/net.h>
#include <libuya/sound.h>
#include <libuya/color.h>
#include <libuya/pad.h>
#include <libuya/uya.h>
#include <libuya/utils.h>
#include <libuya/interop.h>
#include "training.h"


#define MAX_SPAWNED_TARGETS (3)
#define TARGET_SPAWN_DISTANCE (5)
#define TARGET_RESPAWN_DELAY (TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME (TIME_SECOND * 20)
#define TARGET_LIFETIME_TICKS (TPS * 60)
#define TICKS_TO_RESPAWN (TPS * 0.5)
#define TIMELIMIT_MINUTES (5)
#define SIZEOF_PLAYER_OBJECT (0x4500)
#define TARGET_TEAM (TEAM_RED)

// Distance-based behavior
// based off of bot to player distance
#define STRAFE_DISTANCE_MIN (4)
#define STRAFE_DISTANCE_MAX (15)
#define WANDER_DISTANCE_MIN (30)     
#define WANDER_DISTANCE_MAX (40)

VariableAddress_t vaResurrectSpawnDistance = {
#if UYA_PAL
	.Lobby = 0x00671760,
	.Bakisi = 0x00545108,
	.Hoven = 0x005472d0,
	.OutpostX12 = 0x0053cba8,
	.KorgonOutpost = 0x0053a290,
	.Metropolis = 0x00539690,
	.BlackwaterCity = 0x00536e78,
	.CommandCenter = 0x005366d0,
	.BlackwaterDocks = 0x00538f50,
	.AquatosSewers = 0x00538250,
	.MarcadiaPalace = 0x00537bd0,
#else
	.Lobby = 0x0066ee10,
	.Bakisi = 0x005427f8,
	.Hoven = 0x00544900,
	.OutpostX12 = 0x0053a218,
	.KorgonOutpost = 0x00537980,
	.Metropolis = 0x00536d80,
	.BlackwaterCity = 0x005344e8,
	.CommandCenter = 0x00533f18,
	.BlackwaterDocks = 0x00536758,
	.AquatosSewers = 0x00535a98,
	.MarcadiaPalace = 0x005353d8,
#endif
};

void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);

extern int Initialized;
extern struct TrainingMapConfig Config;

int TargetTeam = TARGET_TEAM;

int last_names_idx = 0;
int waiting_for_sniper_shot = 0;

const int SimPlayerCount = MAX_SPAWNED_TARGETS;
SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

void setSpawnDistance(float distance)
{
	short s = (short)distance;
	POKE_U16(GetAddress(&vaResurrectSpawnDistance), s);
}

void modeInitialize(void)
{
	// cheatsApplyNoPacks();
	// cheatsDisableHealthboxes();
}

void modeOnGadgetFired(int gadgetId)
{
	if (gadgetId == WEAPON_ID_FLUX) {
		State.ShotsFired += 1;
		waiting_for_sniper_shot = 10;
	}
}

int modeGetJumpTicks(void)
{
	if (rand(2))
		return randRangeInt(TPS * 2, TPS * 5);

	return randRangeInt(5, TPS * 1);
}

//--------------------------------------------------------------------------
void modeOnTargetHit(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{

}

//--------------------------------------------------------------------------
void modeOnTargetKilled(SimulatedPlayer_t* target, MobyColDamage* colDamage)
{
	target->TicksToRespawn = TICKS_TO_RESPAWN;
	State.TargetsDestroyed += 1;
	target->Active = 0;
}

void modeUpdateTarget(SimulatedPlayer_t *sPlayer)
{
    int i;
    VECTOR delta, targetPos;
    Player* player = playerGetFromSlot(0);
    if (!sPlayer || !player || !sPlayer->Active)
        return;

    struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
    Player* target = sPlayer->Player;
    Moby* targetMoby = target->pMoby;
    struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;
    if (!pvar)
        return;

    // set to lock-strafe
    *(u8*)(0x001A5a34 + (sPlayer->Idx * 4)) = 1;

    // face player
    vector_copy(delta, player->playerPosition);
    vector_subtract(delta, delta, target->playerPosition);
    delta[2] -= 1.5;
    
    // Calculate distance to player (horizontal only)
    float distanceToPlayer = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
    
    float horizontalDist = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
    float yaw = atan2f(delta[0], delta[1]);
    float len = sqrtf(horizontalDist * horizontalDist + delta[2] * delta[2]);
    float targetPitch = asinf(-delta[2] / len);
    
    sPlayer->Yaw = lerpfAngle(sPlayer->Yaw, yaw, 0.5);
    sPlayer->Pitch = targetPitch;

    MATRIX m;
    matrix_unit(m);
    matrix_rotate_y(m, m, sPlayer->Pitch);
    matrix_rotate_z(m, m, sPlayer->Yaw);
    memcpy(&target->camera->uMtx, m, sizeof(VECTOR) * 3);
    vector_copy(target->fps.cameraDir, &m[4]);
    target->fps.vars.cameraY.rotation = sPlayer->Pitch;
    target->fps.vars.cameraZ.rotation = sPlayer->Yaw;

    // Decrement all timers
    if (sPlayer->TicksToJump > 0) sPlayer->TicksToJump--;
    if (sPlayer->TicksToJumpFor > 0) sPlayer->TicksToJumpFor--;
    if (sPlayer->TicksToStrafeSwitch > 0) sPlayer->TicksToStrafeSwitch--;
    if (sPlayer->TicksToStrafeStop > 0) sPlayer->TicksToStrafeStop--;
    if (sPlayer->TicksToStrafeStopFor > 0) sPlayer->TicksToStrafeStopFor--;

    // Jump logic
    if (sPlayer->TicksToJump == 0) {
        sPlayer->TicksToJump = modeGetJumpTicks();
        sPlayer->TicksToJumpFor = 3;  // Press jump for 3 frames
    }

    VECTOR moveTarget;
	if (distanceToPlayer > WANDER_DISTANCE_MAX) {
		// sPlayer->TicksToStrafeStopFor = 1;
		sPlayer->TicksToJumpFor = 0;
        vector_copy(moveTarget, player->playerPosition);
	} else if (distanceToPlayer > WANDER_DISTANCE_MIN) {
        // Very far - wander around current position
        vector_fromyaw(moveTarget, yaw + MATH_PI/2);
        vector_scale(moveTarget, moveTarget, (sPlayer->StrafeDir ? -1 : 1) * 8);
        vector_add(moveTarget, target->playerPosition, moveTarget);
    } else if (distanceToPlayer > STRAFE_DISTANCE_MAX) {
        // Far - walk directly toward player
        vector_copy(moveTarget, player->playerPosition);
    } else if (distanceToPlayer >= STRAFE_DISTANCE_MIN) {
        // Close - strafe around player
        vector_fromyaw(moveTarget, yaw + MATH_PI/2);
        vector_scale(moveTarget, moveTarget, (sPlayer->StrafeDir ? -1 : 1) * 5);
        vector_add(moveTarget, player->playerPosition, moveTarget);
    } else {
        // Too close - back up
        vector_fromyaw(moveTarget, yaw + MATH_PI);
        vector_scale(moveTarget, moveTarget, 3);
        vector_add(moveTarget, target->playerPosition, moveTarget);
    }

    // Get direction to move target
    vector_subtract(delta, moveTarget, target->playerPosition);
    delta[2] = 0;
    float distanceToMoveTarget = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);

	// jump
    if (sPlayer->TicksToJumpFor > 0) {
        pad->btns &= ~PAD_CROSS;
    }

    // Switch strafe direction when close to target or timer expires
    if (distanceToMoveTarget < 0.5 || sPlayer->TicksToStrafeSwitch == 0) {
        sPlayer->StrafeDir = !sPlayer->StrafeDir;
        sPlayer->TicksToStrafeSwitch = randRangeInt(15, TPS * 1);
    }

    // Transform movement to local space
    matrix_unit(m);
    matrix_rotate_z(m, m, -yaw);
    vector_apply(delta, delta, m);
    delta[2] = 0;
    vector_normalize(delta, delta);

    // Random stop logic
    if (sPlayer->TicksToStrafeStop == 0) {
        sPlayer->TicksToStrafeStop = randRangeInt(TPS * 2, TPS * 5);
        sPlayer->TicksToStrafeStopFor = randRangeInt(5, 30);
    }
    
    // Apply stop if active
    if (sPlayer->TicksToStrafeStopFor > 0) {
        delta[0] = delta[1] = 0;
    }

    // Set analog stick values
    pad->ljoy_h = (u8)(((clamp(-delta[1] * 1.5, -1, 1) + 1) / 2) * 255);
    pad->ljoy_v = (u8)(((clamp(-delta[0] * 1.5, -1, 1) + 1) / 2) * 255);
}

void modeInitTarget(SimulatedPlayer_t * sPlayer)
{
	GameSettings * gs = gameGetSettings();

	// init vars
	struct TrainingTargetMobyPVar * pvar = &sPlayer->Vars;

	pvar->State = TARGET_STATE_IDLE;
	pvar->TimeCreated = gameGetTime();
	pvar->LifeTicks = TARGET_LIFETIME_TICKS;
	pvar->Jitter = 0;
	pvar->JumpSpeed = 5;
	pvar->StrafeSpeed = 5;
	vector_write(pvar->Velocity, 0);

	sPlayer->Active = 1;
	sPlayer->TicksToJump = modeGetJumpTicks();
	playerSetHealth(sPlayer->Player, 1);
	vector_copy(pvar->SpawnPos, sPlayer->Player->playerPosition);
	sPlayer->Points = 0;
	
	// set player name
	sprintf(gs->PlayerNames[sPlayer->Player->mpIndex], "Fake %d", sPlayer->Idx);

	// set spawn distance
	#ifdef TARGET_SPAWN_DISTANCE != 40.0f
	setSpawnDistance(5);
	#endif
}

void modeProcessPlayer(int pIndex)
{
	
}

void modeTick(void)
{
	// do tick stuff
}
