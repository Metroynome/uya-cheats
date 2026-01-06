#include <libuya/moby.h>
#include <libuya/math.h>
#include <libuya/math3d.h>
#include <libuya/player.h>
#include <libuya/game.h>
#include <libuya/utils.h>

#define SOUND_ID_JUMPPAD_LOOP (4)
#define SOUND_ID_JUMPPAD_LAUNCH (1)
#define HELP_ID_JUMPPAD (19)

typedef struct M4292_Update_JumpPad { // 0x1d0
    int targetMobyIndex;
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
    if (this->drawn) {
        // jumpPadInfo.vtable.drawEffects(this);
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
