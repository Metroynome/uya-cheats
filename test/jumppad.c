#include <libuya/moby.h>
#include <libuya/math.h>
#include <libuya/math3d.h>
#include <libuya/player.h>
#include <libuya/game.h>
#include <libuya/utils.h>
#include <libuya/spawnpoint.h>>
#include <libuya/sound.h>

#define SOUND_ID_JUMPPAD_LOOP (4)
#define SOUND_ID_JUMPPAD_LAUNCH (1)
#define HELP_ID_JUMPPAD (19)
#define MIN_LAUNCH_DISTANCE (0.5)

typedef struct M4292_Update_JumpPad { // 0x1d0
    int targetCuboidIndex;
    int targetSplineIndex;
    int unk_08;
    int soundChannelId;
    VECTOR targetVelocities[GAME_MAX_PLAYERS];
    VECTOR targetPositions[GAME_MAX_PLAYERS];
    int playerTimers[GAME_MAX_PLAYERS];
    int playerEffectIds[GAME_MAX_PLAYERS];
    int launchPatternIndices[GAME_MAX_PLAYERS];
    Player *pLaunchedPlayers[GAME_MAX_PLAYERS];
    float playerLaunchHeights[GAME_MAX_PLAYERS];
    int playerLaunchedFlags[GAME_MAX_PLAYERS];
} M4292_Update_JumpPad_t;

typedef struct jumpPad_vtable {
    void (*update)(Moby *this);
    void (*drawEffects)(Moby *this);
} jumpPad_vtable_t;

typedef struct jumpPadInfo {
    short init;
    short runOldFunction;
    int numJumpPads;
    jumpPad_vtable_t vtable;
} jumpPadInfo_t;
jumpPadInfo_t jumpPadInfo;

void jumpPadUpdate(Moby *this)
{
    int i;
    M4292_Update_JumpPad_t *pvar = (M4292_Update_JumpPad_t*)this->pVar;
    // run original function if specified
    if (jumpPadInfo.runOldFunction) {
        jumpPadInfo.vtable.update(this);
    }

    // State 0: Initialize jump pad
    if (this->state == 0) {
        // Delete if moby and spline index is negative 1.
        if (pvar->targetCuboidIndex == -1 && pvar->targetSplineIndex == -1) {
            mobyDestroy(this);
            return;
        }
        // rotate jump pad towards cuboid/spline.
        if (pvar->targetSplineIndex == -1) {
            Cuboid* cuboid = spawnPointGet(pvar->targetCuboidIndex);
            this->rotation[2] = atan2f(this->position[0] - cuboid->pos[0], this->position[1] - cuboid->pos[1]);
        } else {
            // Spline* spline = Spline_Get(pvar->targetSplineId);
            // this->rotation.y = atan2f(this->pos.x - spline->pos.x, this->pos.z - spline->pos.z);
        }
        this->modeBits |= 0x2000;
        for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
            pvar->playerTimers[i] = (this->unk_ac[6] + i + 7) / 8 * 8 - (this->unk_ac[6] + i);
            pvar->playerEffectIds[i] = -1;
            pvar->playerLaunchHeights[i] = 0.0f;
            pvar->playerLaunchedFlags[i] = 0;
        }
        this->updateDist = -1;
        this->unk_4c[0x16] = 0x40;
        this->drawDist = 0x80;
        pvar->soundChannelId = -1;
        mobySetState(this, 1, -1);
        return;
    }
    if (this->state != 1)
        return;

    // handle sounds
    int soundChannel = pvar->soundChannelId;
    if (soundChannel == -1 || SOUND_GLOBALS->channels[soundChannel].pos[1] != (float)this->position[1] || SOUND_GLOBALS->channels[soundChannel].pitchMod == 0)
        soundMobyPlay(SOUND_ID_JUMPPAD_LOOP, 4, this);

    for (i = 0; i < GAME_MAX_PLAYERS; ++i) {
        Player *p = playerGetFromSlot(i);
        if (!p) continue;

        if (pvar->pLaunchedPlayers[i] == p) {
            int state = playerGetState(p);
            if (state != PLAYER_STATE_MOON_JUMP)
                return;
    
            VECTOR delta;
            vector_subtract(delta, p->playerPosition, pvar->pLaunchedPlayers[i]->playerPosition);
            float len = vector_length(delta);
            if (len > MIN_LAUNCH_DISTANCE) {
                float pos = vector_length(p->playerPosition);                
                if (pos) {
                    VECTOR dirToTarget;
                    vector_subtract(dirToTarget, pvar->targetPositions[i], p->playerPosition);
                    dirToTarget[2] = 0;
                   float dot =  vector_innerproduct(dirToTarget, pvar->targetVelocities[i]);
                    if (dot < 0) len = 0;
                }
            } else {
                len = 0;
            }
        }
    }

    // if drawn, also draw effects.
    if (this->drawn) {
        jumpPadInfo.vtable.drawEffects(this);
    }
}

void jumpPadFindAndHook(void)
{
    Moby *start = mobyListGetStart();
    Moby *end = mobyListGetEnd();
    while (start < end) {
        if (start->oClass == MOBY_ID_JUMP_PAD) {
            if (!jumpPadInfo.vtable.update)
                jumpPadInfo.vtable.update = start->pUpdate;

            (void*)start->pUpdate = (void*)jumpPadUpdate;
        }
        ++start;
    }
}

int jumpPadInit(void)
{
    // find all jump pads, and update moby update function to ours.
    jumpPadFindAndHook();

    // store original functions.  Converts JAL to address.
    u32 start = jumpPadInfo.vtable.update;
    jumpPadInfo.vtable.drawEffects = JAL2ADDR(*(u32*)(start + 0x640));
    return 1;
}

void jumpPad(void)
{
    if (!isInGame()) {
        // zero ui struct if not in game.
        if (jumpPadInfo.init > 0)
            memset(&jumpPadInfo, 0, sizeof(jumpPadInfo_t));
        
        return;
    }
    
    if (jumpPadInfo.init == 0)
        jumpPadInfo.init = jumpPadInit();
}
