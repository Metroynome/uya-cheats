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
#include "training.h"


#define MAX_SPAWNED_TARGETS (3)
#define TARGET_RESPAWN_DELAY (TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME (TIME_SECOND * 20)
#define TARGET_LIFETIME_TICKS (TPS * 60)
#define TICKS_TO_RESPAWN (TPS * 0.5)
#define TIMELIMIT_MINUTES (5)
#define SIZEOF_PLAYER_OBJECT (0x4500)
#define TARGET_TEAM (TEAM_RED)

void createSimPlayer(SimulatedPlayer_t* sPlayer, int idx);

extern int Initialized;
extern struct TrainingMapConfig Config;

int TargetTeam = TARGET_TEAM;

int last_names_idx = 0;
int waiting_for_sniper_shot = 0;

const int SimPlayerCount = MAX_SPAWNED_TARGETS;
SimulatedPlayer_t SimPlayers[MAX_SPAWNED_TARGETS];

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

    Player* target = sPlayer->Player;
    Moby* targetMoby = target->pMoby;
    struct TrainingTargetMobyPVar* pvar = &sPlayer->Vars;
    if (!pvar)
        return;

    // set to lock-strafe
    *(u8*)(0x001A5a34 + (sPlayer->Idx * 4)) = 1;

	if (State.AggroMode == TRAINING_AGGRESSION_IDLE)
		return;

	u32 jumpTicks = timeDecTimerShort(&sPlayer->TicksToJump);
	u32 jumpTicksFor = timeDecTimerShort(&sPlayer->TicksToJumpFor);
	u32 strafeSwitchTicks = timeDecTimerShort(&sPlayer->TicksToStrafeSwitch);
	u32 strafeStopTicks = timeDecTimerShort(&sPlayer->TicksToStrafeStop);
	u32 strafeStopTicksFor = timeDecTimerShort(&sPlayer->TicksToStrafeStopFor);

    // face player
    vector_copy(delta, player->playerPosition);
    vector_subtract(delta, delta, target->playerPosition);
	delta[2] -= 1.5;
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

    struct padButtonStatus* pad = (struct padButtonStatus*)sPlayer->Pad.rdata;
	if (!jumpTicks) {
		// next jump
		sPlayer->TicksToJump = modeGetJumpTicks();
		sPlayer->TicksToJumpFor = randRangeInt(0, 5);
	}

	// jump
	if (jumpTicksFor) {
		pad->btns &= ~PAD_CROSS;
	}


	// determine current strafe target
	vector_fromyaw(targetPos, yaw + MATH_PI/2);
	vector_scale(targetPos, targetPos, (sPlayer->StrafeDir ? -1 : 1) * 5);
	vector_add(targetPos, pvar->SpawnPos, targetPos);

	// get distance to target
	vector_subtract(delta, targetPos, target->playerPosition);
	delta[3] = delta[2] = 0;
	float distanceToTargetPos = vector_length(delta);
	if (distanceToTargetPos < 0.5 || !strafeSwitchTicks) {
		sPlayer->StrafeDir = !sPlayer->StrafeDir;
		sPlayer->TicksToStrafeSwitch = randRangeInt(15, TPS * 1);
	}

	// move to target
	matrix_unit(m);
	matrix_rotate_z(m, m, -yaw);
	vector_apply(delta, delta, m);
	delta[3] = delta[2] = 0;
	vector_normalize(delta, delta);

	if (!strafeStopTicks) {
		sPlayer->TicksToStrafeStop = randRangeInt(TPS * 2, TPS * 5);
		sPlayer->TicksToStrafeStopFor = randRangeInt(5, 30);
	}

	if (strafeStopTicksFor) {
		delta[0] = delta[1] = 0;
	}

	pad->ljoy_h = (u8)(((clamp(-delta[1] * 1.5, -1, 1) + 1) / 2) * 255);
	pad->ljoy_v = (u8)(((clamp(-delta[0] * 1.5, -1, 1) + 1) / 2) * 255);


	// shoot
	pad->btns &= ~PAD_CIRCLE;
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
	// vector_copy(pvar->SpawnPos, sPlayer->Player->playerPosition);
	sPlayer->Points = 0;
	
	// set player name
	sprintf(gs->PlayerNames[sPlayer->Player->mpIndex], "Fake %d", sPlayer->Idx);
}

void modeProcessPlayer(int pIndex)
{
	
}

void modeTick(void)
{
	// do tick stuff
}
