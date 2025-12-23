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
#define WANDER_DISTANCE_MIN -1 // (30.0f)     
#define WANDER_DISTANCE_MAX -1// (40.0f)

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

void modeUpdateTarget_AvoidHazards(SimulatedPlayer_t *sPlayer, Player* target, VECTOR moveTarget, float yaw)
{
    // ONLY check hazards if we're moving a significant distance
    VECTOR delta;
    vector_subtract(delta, moveTarget, target->playerPosition);
    delta[2] = 0;
    float moveDistance = vector_length(delta);
    
    // If barely moving, don't bother checking
    if (moveDistance < 1.0f) {
        return;
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
        vector_copy(moveTarget, testPos);
        return;
    }
    
    // No safe direction - stay put
    vector_copy(moveTarget, target->playerPosition);
}




// Add to your #defines at the top
#define SWINGSHOT_DETECTION_DISTANCE 15.0f
#define SWINGSHOT_MIN_HEIGHT 2.0f
#define SWINGSHOT_MAX_HEIGHT 8.0f

// Function to check if there's a swingshot orb nearby
Moby* modeUpdateTarget_FindSwingshotOrb(SimulatedPlayer_t *sPlayer, Player* target, VECTOR direction, float searchDistance)
{
    Moby* moby;
    Moby* closestOrb = NULL;
    float closestDist = 999999.0f;
    VECTOR targetPos, delta;
    
    // Get the first moby in the list
    moby = mobyListGetStart();
    Moby* end = mobyListGetEnd();
    
    // Calculate search position ahead of bot
    vector_copy(targetPos, target->playerPosition);
    vector_scale(direction, direction, searchDistance);
    vector_add(targetPos, targetPos, direction);
    
    // Search for swingshot orbs
    while (moby < end) {
        if (moby->oClass == MOBY_ID_SWINGSHOT_ORB) {
            // Check if orb is in front of us
            vector_subtract(delta, moby->position, target->playerPosition);
            delta[2] = 0;  // Only horizontal distance
            float dist = vector_length(delta);
            
            if (dist < searchDistance && dist < closestDist) {
                // Check if orb is at a reasonable height above bot
                float heightDiff = moby->position[2] - target->playerPosition[2];
                
                if (heightDiff >= SWINGSHOT_MIN_HEIGHT && heightDiff <= SWINGSHOT_MAX_HEIGHT) {
                    // Check if orb is roughly in the direction we're moving
                    vector_normalize(delta, delta);
                    VECTOR dirNorm;
                    vector_copy(dirNorm, direction);
                    vector_normalize(dirNorm, dirNorm);
                    
                    float dot = vector_innerproduct(delta, dirNorm);
                    if (dot > 0.7f) {  // Within ~45 degrees
                        closestOrb = moby;
                        closestDist = dist;
                    }
                }
            }
        }
        moby++;
    }
    
    return closestOrb;
}

// Check if there's a gap beyond the swingshot
int modeUpdateTarget_IsGapBeyondSwingshot(SimulatedPlayer_t *sPlayer, Player* target, Moby* swingshotOrb)
{
    VECTOR checkPos, direction, rayStart, rayEnd;
    
    // Get direction from bot to swingshot
    vector_subtract(direction, swingshotOrb->position, target->playerPosition);
    direction[2] = 0;
    vector_normalize(direction, direction);
    
    // Check position beyond the swingshot (5 units past it)
    vector_copy(checkPos, swingshotOrb->position);
    vector_scale(direction, direction, 5.0f);
    vector_add(checkPos, checkPos, direction);
    
    // Raycast down from beyond swingshot
    vector_copy(rayStart, checkPos);
    rayStart[2] = swingshotOrb->position[2];  // Start at swingshot height
    
    vector_copy(rayEnd, checkPos);
    rayEnd[2] = target->playerPosition[2] - 10.0f;  // Check way down
    
    int hit = CollLine_Fix(rayStart, rayEnd, 0x0840, target->pMoby, 0);
    
    if (!hit) {
        return 1;  // No ground = gap exists
    }
    
    // Check if ground is very far down (big drop)
    float* hitPos = CollLine_Fix_GetHitPosition();
    float dropDistance = swingshotOrb->position[2] - hitPos[2];
    
    if (dropDistance > 5.0f) {
        return 1;  // Big drop = gap
    }
    
    return 0;  // No gap
}

// Main swingshot logic
int modeUpdateTarget_HandleSwingshot(SimulatedPlayer_t *sPlayer, Player* target, VECTOR moveDirection, struct padButtonStatus* pad)
{
    VECTOR direction;
    Moby* swingshotOrb;
    
    // If already swinging, just hold R1
    if (sPlayer->IsSwinging) {
        // Check if we should release
        VECTOR deltaToOrb;
        vector_subtract(deltaToOrb, sPlayer->SwingshotOrb->position, target->playerPosition);
        float distToOrb = vector_length(deltaToOrb);
        
        // Release when we're past the orb and moving away
        if (distToOrb < 3.0f) {
            sPlayer->SwingReleaseTimer = 5;  // Release in 5 frames
        }
        
        if (sPlayer->SwingReleaseTimer > 0) {
            sPlayer->SwingReleaseTimer--;
            if (sPlayer->SwingReleaseTimer == 0) {
                // Release swing
                pad->btns |= PAD_R1;  // Stop pressing R1
                sPlayer->IsSwinging = 0;
                sPlayer->SwingshotOrb = NULL;
                return 0;
            }
        }
        
        // Keep holding R1 to swing
        pad->btns &= ~PAD_R1;
        return 1;  // Still swinging
    }
    
    // Not swinging yet - check if we should start
    vector_copy(direction, moveDirection);
    vector_normalize(direction, direction);
    
    // Look for swingshot orb ahead
    swingshotOrb = modeUpdateTarget_FindSwingshotOrb(sPlayer, target, direction, SWINGSHOT_DETECTION_DISTANCE);
    
    if (!swingshotOrb) {
        return 0;  // No swingshot nearby
    }
    
    // Check if there's a gap beyond the swingshot
    if (!modeUpdateTarget_IsGapBeyondSwingshot(sPlayer, target, swingshotOrb)) {
        return 0;  // No gap, don't need to swing
    }
    
    // Found swingshot over gap! Start swinging
    VECTOR deltaToOrb;
    vector_subtract(deltaToOrb, swingshotOrb->position, target->playerPosition);
    float distToOrb = sqrtf(deltaToOrb[0] * deltaToOrb[0] + deltaToOrb[1] * deltaToOrb[1]);
    
    // Start swinging when close enough
    if (distToOrb < 8.0f) {
        sPlayer->IsSwinging = 1;
        sPlayer->SwingshotOrb = swingshotOrb;
        sPlayer->SwingReleaseTimer = 0;
        pad->btns &= ~PAD_R1;  // Press R1
        return 1;  // Started swinging
    }
    
    return 0;  // Not close enough yet
}





// Add to your #defines
#define JUMP_PAD_SEARCH_DISTANCE 20.0f
#define HEIGHT_DIFF_FOR_JUMP_PAD 1.0f

// Add to SimulatedPlayer_t struct:
// Moby* targetJumpPad;

void modeUpdateTarget_HandleJumpPad(SimulatedPlayer_t *sPlayer, Player* player, Player* target, VECTOR moveTarget, struct padButtonStatus* pad)
{
    Moby *moby;
    VECTOR delta;

    // Decrement cooldown timer
    if (sPlayer->jumpPadSearchCooldown > 0) {
        --sPlayer->jumpPadSearchCooldown;
    }

    // Only search if we don't have a target AND cooldown expired
    if (!sPlayer->targetJumpPad && sPlayer->jumpPadSearchCooldown == 0) {
        // check height distance difference for of player and bot
        float heightDiff = player->playerPosition[2] - sPlayer->Player->playerPosition[2];
        if (heightDiff >= HEIGHT_DIFF_FOR_JUMP_PAD) {
            // check if bot is horizontally close to player
            vector_copy(delta, player->playerPosition);
            vector_subtract(delta, delta, sPlayer->Player->playerPosition);
            delta[2] = 0;
            float horizontalDist = vector_length(delta);
            
            if (horizontalDist <= 30.0f) {
                // Look for nearby jump pad    
                moby = mobyListGetStart();
                Moby* end = mobyListGetEnd();
                while (moby < end) {
                    if (moby->oClass == MOBY_ID_JUMP_PAD) {
                        // Calculate distance to jump pad
                        vector_subtract(delta, moby->position, sPlayer->Player->playerPosition);
                        float dist = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);
                        if (dist < JUMP_PAD_SEARCH_DISTANCE && (sPlayer->Player->playerPosition[2] + 0.5f) > moby->position[2]) {
                            sPlayer->targetJumpPad = moby;
                            break;
                        }
                    }
                    moby++;
                }
                
                // Set cooldown whether we found one or not
                sPlayer->jumpPadSearchCooldown = TPS * 5; // Search every 5s
            }
        }
    }

    // Use the jump pad if we have one
    if (sPlayer->targetJumpPad) {
        vector_copy(moveTarget, sPlayer->targetJumpPad->position);
        
        if (sPlayer->Player->mobys.ground == sPlayer->targetJumpPad) {
            pad->btns &= ~PAD_CROSS;
            sPlayer->targetJumpPad = NULL;
        }
    }
}

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
        sPlayer->TicksToJumpFor = 0;
        vector_copy(moveTarget, player->playerPosition);
    } else if (distanceToPlayer >= STRAFE_DISTANCE_MIN) {
        vector_fromyaw(moveTarget, yaw + MATH_PI/2);
        vector_scale(moveTarget, moveTarget, (sPlayer->StrafeDir ? -1 : 1) * 5);
        vector_add(moveTarget, player->playerPosition, moveTarget);
    } else {
        vector_fromyaw(moveTarget, yaw + MATH_PI);
        vector_scale(moveTarget, moveTarget, 3);
        vector_add(moveTarget, target->playerPosition, moveTarget);
    }
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

    *(u8*)(0x001A5a34 + (sPlayer->Idx * 4)) = 1;

    // set easy to get deobfuscated state
    sPlayer->state = playerGetState(sPlayer->Player);

    if (sPlayer->TicksToJump > 0) sPlayer->TicksToJump--;
    if (sPlayer->TicksToJumpFor > 0) sPlayer->TicksToJumpFor--;
    if (sPlayer->TicksToStrafeSwitch > 0) sPlayer->TicksToStrafeSwitch--;
    if (sPlayer->TicksToStrafeStop > 0) sPlayer->TicksToStrafeStop--;
    if (sPlayer->TicksToStrafeStopFor > 0) sPlayer->TicksToStrafeStopFor--;
    if (sPlayer->TicksToThrowWrench > 0) sPlayer->TicksToThrowWrench--;

    vector_copy(delta, player->playerPosition);
    vector_subtract(delta, delta, target->playerPosition);
    delta[2] -= 1.5;
    float distanceToPlayer = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);

    float previousDistance = sPlayer->LastDistanceToPlayer;
    float approachSpeed = previousDistance - distanceToPlayer;
    sPlayer->LastDistanceToPlayer = distanceToPlayer;

    modeUpdateTarget_Aiming(sPlayer, player, target, delta);
    
    float yaw = atan2f(delta[0], delta[1]);

    // check if ledge grab, if so, jump.
    if (sPlayer->state >= 23 && sPlayer->state <= 26)
        sPlayer->vtable->UpdateState(sPlayer->Player, PLAYER_STATE_LEDGE_JUMP, 1, 1, 0);

    if (modeUpdateTarget_Wrench(sPlayer, player, pad, distanceToPlayer, approachSpeed, yaw)) {
        return;
    }

    VECTOR moveTarget;

    // Handle jump pad logic
    modeUpdateTarget_HandleJumpPad(sPlayer, player, target, moveTarget, pad);
    
    // If we have a jump pad target, go straight to it with no other behaviors
    if (sPlayer->targetJumpPad) {
        // Calculate direction to jump pad
        vector_subtract(delta, moveTarget, target->playerPosition);
        delta[2] = 0;
        
        // Transform to local space
        MATRIX m;
        matrix_unit(m);
        matrix_rotate_z(m, m, -yaw);
        vector_apply(delta, delta, m);
        delta[2] = 0;
        vector_normalize(delta, delta);
        
        // Move straight toward jump pad - no strafing, no jumping
        pad->ljoy_h = (u8)(((clamp(-delta[1], -1, 1) + 1) / 2) * 255);
        pad->ljoy_v = (u8)(((clamp(-delta[0], -1, 1) + 1) / 2) * 255);
        
        return;  // Skip all other movement logic
    }
    
    // Normal movement if no jump pad
    modeUpdateTarget_CalculateMovement(sPlayer, player, target, moveTarget, distanceToPlayer, yaw, approachSpeed);

    // Check for swingshot BEFORE hazard avoidance
    VECTOR moveDirection;
    vector_subtract(moveDirection, moveTarget, target->playerPosition);
    moveDirection[2] = 0;
    
    int isSwinging = modeUpdateTarget_HandleSwingshot(sPlayer, target, moveDirection, pad);
    
    if (isSwinging) {
        // While swinging, keep moving forward
        vector_normalize(moveDirection, moveDirection);
        MATRIX m;
        matrix_unit(m);
        matrix_rotate_z(m, m, -yaw);
        vector_apply(moveDirection, moveDirection, m);
        
        pad->ljoy_h = (u8)(((clamp(-moveDirection[1], -1, 1) + 1) / 2) * 255);
        pad->ljoy_v = (u8)(((clamp(-moveDirection[0], -1, 1) + 1) / 2) * 255);
        return;  // Skip rest of movement logic while swinging
    }

    // Continue with normal movement...
    modeUpdateTarget_AvoidHazards(sPlayer, target, moveTarget, yaw);

    
    // Normal jump logic (only if not ledge jumping)
    if (sPlayer->TicksToJump == 0) {
        sPlayer->TicksToJump = modeGetJumpTicks();
        sPlayer->TicksToJumpFor = 3;
    }
    
    if (sPlayer->TicksToJumpFor > 0) {
        pad->btns &= ~PAD_CROSS;
    }

    vector_subtract(delta, moveTarget, target->playerPosition);
    delta[2] = 0;
    float distanceToMoveTarget = sqrtf(delta[0] * delta[0] + delta[1] * delta[1]);

    if (distanceToMoveTarget < 0.5 || sPlayer->TicksToStrafeSwitch == 0) {
        sPlayer->StrafeDir = !sPlayer->StrafeDir;
        sPlayer->TicksToStrafeSwitch = randRangeInt(15, TPS * 1);
    }

    MATRIX m;
    matrix_unit(m);
    matrix_rotate_z(m, m, -yaw);
    vector_apply(delta, delta, m);
    delta[2] = 0;
    vector_normalize(delta, delta);

    if (sPlayer->TicksToStrafeStop == 0) {
        sPlayer->TicksToStrafeStop = randRangeInt(TPS * 2, TPS * 5);
        sPlayer->TicksToStrafeStopFor = randRangeInt(5, 30);
    }
    
    if (sPlayer->TicksToStrafeStopFor > 0) {
        delta[0] = delta[1] = 0;
    }

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
