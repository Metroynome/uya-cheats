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
#define TARGET_SPAWN_DISTANCE (5.0f)
#define TARGET_HEALTH (1)
#define TARGET_RESPAWN_DELAY (TIME_SECOND * 1)
#define TARGET_POINTS_LIFETIME (TIME_SECOND * 20)
#define TARGET_LIFETIME_TICKS (TPS * 60)
#define TICKS_TO_RESPAWN (TPS * 0.5)
#define TIMELIMIT_MINUTES (5)
#define SIZEOF_PLAYER_OBJECT (0x4500)
#define TARGET_TEAM (TEAM_RED)

// Distance-based behavior
// based off of bot to player distance
#define STRAFE_DISTANCE_MIN (4.0f)
#define STRAFE_DISTANCE_MAX (15.0f)
#define WANDER_DISTANCE_MIN (30.0f)     
#define WANDER_DISTANCE_MAX (40.0f)

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

// Helper function for wrench behavior
int modeUpdateTarget_Wrench(SimulatedPlayer_t *sPlayer, Player* player, struct padButtonStatus* pad, float distanceToPlayer, float approachSpeed, float yaw)
{
    // Check if currently wrenching
    int shouldWrench = (sPlayer->UsingWrench && sPlayer->TicksToThrowWrench > 0);
    
    if (shouldWrench) {
        // Stop all movement while wrenching
        pad->ljoy_h = 0x7F;
        pad->ljoy_v = 0x7F;
        
        // Press square to wrench
        pad->btns &= ~PAD_SQUARE;
        
        return 1;  // Return 1 to signal we're wrenching (caller should return early)
    } else if (sPlayer->UsingWrench) {
        // Done wrenching
        sPlayer->UsingWrench = 0;
    }
    
    // Check if we should switch back to gun after wrenching
    if (sPlayer->HasWrenched && !sPlayer->UsingWrench && distanceToPlayer >= STRAFE_DISTANCE_MIN) {
        pad->btns &= ~PAD_TRIANGLE;
        sPlayer->HasWrenched = 0;
    }
    
    // Trigger wrench if player rushing in
    if (distanceToPlayer <= 1.5 && approachSpeed > 0.1 && sPlayer->TicksToThrowWrench == 0 && !sPlayer->HasWrenched) {
        sPlayer->TicksToThrowWrench = 30;
        sPlayer->UsingWrench = 1;
        sPlayer->HasWrenched = 1;
    }
    
    return 0;  // Not currently wrenching
}

// Helper function for movement target calculation
void modeUpdateTarget_CalculateMovement(SimulatedPlayer_t *sPlayer, Player* player, Player* target, VECTOR moveTarget, float distanceToPlayer, float yaw, float approachSpeed)
{
    if (distanceToPlayer > WANDER_DISTANCE_MAX) {
        sPlayer->TicksToJumpFor = 0;
        vector_copy(moveTarget, player->playerPosition);
    } else if (distanceToPlayer > WANDER_DISTANCE_MIN) {
        vector_fromyaw(moveTarget, yaw + MATH_PI/2);
        vector_scale(moveTarget, moveTarget, (sPlayer->StrafeDir ? -1 : 1) * 8);
        vector_add(moveTarget, target->playerPosition, moveTarget);
    } else if (distanceToPlayer > STRAFE_DISTANCE_MAX) {
        vector_copy(moveTarget, player->playerPosition);
    } else if (distanceToPlayer >= STRAFE_DISTANCE_MIN) {
        vector_fromyaw(moveTarget, yaw + MATH_PI/2);
        vector_scale(moveTarget, moveTarget, (sPlayer->StrafeDir ? -1 : 1) * 5);
        vector_add(moveTarget, player->playerPosition, moveTarget);
    } else {
        // Default: back away to maintain safe distance
        vector_fromyaw(moveTarget, yaw + MATH_PI);
        vector_scale(moveTarget, moveTarget, 3);
        vector_add(moveTarget, target->playerPosition, moveTarget);
    }
}

// Helper function for aiming
void modeUpdateTarget_Aiming(SimulatedPlayer_t *sPlayer, Player* player, Player* target, VECTOR delta)
{
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
}

// Helper function for jumping
void modeUpdateTarget_Jump(SimulatedPlayer_t *sPlayer, struct padButtonStatus* pad)
{
    if (sPlayer->TicksToJump == 0) {
        sPlayer->TicksToJump = modeGetJumpTicks();
        sPlayer->TicksToJumpFor = 3;
    }
    
    if (sPlayer->TicksToJumpFor > 0) {
        pad->btns &= ~PAD_CROSS;
    }
}






// Fixed: Always check from bot's height, not target height
int modeUpdateTarget_IsPathSafe(SimulatedPlayer_t *sPlayer, Player* target, VECTOR from, VECTOR to)
{
    VECTOR adjustedFrom, adjustedTo;
    
    // Copy positions but keep them at bot's current height for path check
    vector_copy(adjustedFrom, from);
    vector_copy(adjustedTo, to);
    adjustedFrom[2] = from[2] + 1.0f;  // Bot's height + 1
    adjustedTo[2] = from[2] + 1.0f;    // Keep at bot's height, NOT target height!
    
    // Use moving sphere to check path horizontally
    float result = CollMovingSphere(0.5f, adjustedFrom, adjustedTo, 0x0840, target->pMoby);
    
    if (result < 1.0f) {
        // Wall/obstacle in the way
        return 0;
    }
    
    // Check ground below the horizontal destination point
    VECTOR rayStart, rayEnd;
    vector_copy(rayStart, to);
    rayStart[2] = from[2] + 2.0f;  // Start from bot's current height + 2
    
    vector_copy(rayEnd, to);
    rayEnd[2] = from[2] - 6.0f;    // Check down from bot's height
    
    int hit = CollLine_Fix(rayStart, rayEnd, 0x0840, target->pMoby, 0);
    
    if (!hit) {
        return 0;  // No ground = void/gap
    }
    
    // Check surface type
    int hotspot = CollHotspot();
    if (hotspot == 0x1 || hotspot == 0x2) {
        return 0;  // Death plane or water
    }
    
    // Check if ground is too far below bot's current position
    float* hitPos = CollLine_Fix_GetHitPosition();
    float dropDistance = from[2] - hitPos[2];  // Use bot's height, not target
    
    if (dropDistance > 4.0f) {
        return 0;  // Too big a drop
    }
    
    // Check if ground is too far above (can't climb)
    float climbDistance = hitPos[2] - from[2];
    if (climbDistance > 2.0f) {
        return 0;  // Too high to climb
    }
    
    return 1;  // Path is safe
}

// Updated avoidance using the better collision detection
void modeUpdateTarget_AvoidHazards(SimulatedPlayer_t *sPlayer, Player* target, VECTOR moveTarget, float yaw)
{
    // Check if path to move target is safe
    if (modeUpdateTarget_IsPathSafe(sPlayer, target, target->playerPosition, moveTarget)) {
        return;  // Path is safe, no changes needed
    }
    
    // Path is unsafe! Try alternative directions
    VECTOR testPos;
    float testAngles[] = {
        yaw - MATH_PI/4,   // 45° left
        yaw + MATH_PI/4,   // 45° right  
        yaw - MATH_PI/2,   // 90° left
        yaw + MATH_PI/2,   // 90° right
        yaw + MATH_PI      // 180° behind
    };
    
	int i;
    for (i = 0; i < 5; i++) {
        vector_fromyaw(testPos, testAngles[i]);
        vector_scale(testPos, testPos, 3.0f);
        vector_add(testPos, target->playerPosition, testPos);
        
        if (modeUpdateTarget_IsPathSafe(sPlayer, target, target->playerPosition, testPos)) {
            // Found safe direction
            vector_copy(moveTarget, testPos);
            return;
        }
    }
    
    // No safe direction - stay put
    vector_copy(moveTarget, target->playerPosition);
}

// Main update function (now much cleaner!)
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

    // Decrement all timers
    if (sPlayer->TicksToJump > 0) sPlayer->TicksToJump--;
    if (sPlayer->TicksToJumpFor > 0) sPlayer->TicksToJumpFor--;
    if (sPlayer->TicksToStrafeSwitch > 0) sPlayer->TicksToStrafeSwitch--;
    if (sPlayer->TicksToStrafeStop > 0) sPlayer->TicksToStrafeStop--;
    if (sPlayer->TicksToStrafeStopFor > 0) sPlayer->TicksToStrafeStopFor--;
    if (sPlayer->TicksToThrowWrench > 0) sPlayer->TicksToThrowWrench--;

    // Calculate distance
    vector_copy(delta, player->playerPosition);
    vector_subtract(delta, delta, target->playerPosition);
    delta[2] -= 1.5;
    float distanceToPlayer = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);

    // Track approach speed
    float previousDistance = sPlayer->LastDistanceToPlayer;
    float approachSpeed = previousDistance - distanceToPlayer;
    sPlayer->LastDistanceToPlayer = distanceToPlayer;

    // Always aim at player
    modeUpdateTarget_Aiming(sPlayer, player, target, delta);
    
    float yaw = atan2f(delta[0], delta[1]);  // Recalculate yaw for other functions

    // Handle wrench behavior (returns 1 if currently wrenching)
    if (modeUpdateTarget_Wrench(sPlayer, player, pad, distanceToPlayer, approachSpeed, yaw)) {
        return;  // Exit early if wrenching
    }

    // Jump logic
    // modeUpdateTarget_Jump(sPlayer, pad);

    // Calculate movement target
    VECTOR moveTarget;
    modeUpdateTarget_CalculateMovement(sPlayer, player, target, moveTarget, distanceToPlayer, yaw, approachSpeed);

    modeUpdateTarget_AvoidHazards(sPlayer, target, moveTarget, yaw);

    // Get direction to move target
    vector_subtract(delta, moveTarget, target->playerPosition);
    delta[2] = 0;
    float distanceToMoveTarget = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);

    // Switch strafe direction
    if (distanceToMoveTarget < 0.5 || sPlayer->TicksToStrafeSwitch == 0) {
        sPlayer->StrafeDir = !sPlayer->StrafeDir;
        sPlayer->TicksToStrafeSwitch = randRangeInt(15, TPS * 1);
    }

    // Transform movement to local space
    MATRIX m;
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
	playerSetHealth(sPlayer->Player, TARGET_HEALTH);
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
