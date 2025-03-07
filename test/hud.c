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
#include <libuya/team.h>
#include <libuya/ui.h>
#include <libuya/time.h>
#include <libuya/gameplay.h>
#include <libuya/map.h>

typedef struct hud_vtable {
    void (*hud_setup)();
    void (*health)(int isShown, Player *player);
    void (*mapAndScore)(int index, int a1);
    int (*getGadgetId)();
} hud_vtable_t;

typedef struct hudInfo {
    int init;
    hud_vtable_t vtable;
} hudInfo_t;
hudInfo_t hudInfo;

VariableAddress_t vaHudSetup_Hook = {
#if UYA_PAL
	.Lobby = 0x0061cdcc,
	.Bakisi = 0x004eecc4,
	.Hoven = 0x004f0ddc,
	.OutpostX12 = 0x004e66b4,
	.KorgonOutpost = 0x004e3e4c,
	.Metropolis = 0x004e319c,
	.BlackwaterCity = 0x004e0a34,
	.CommandCenter = 0x004e09fc,
	.BlackwaterDocks = 0x004e327c,
	.AquatosSewers = 0x004e257c,
	.MarcadiaPalace = 0x004e1efc,
#else
	.Lobby = 0x0061a654,
	.Bakisi = 0x004ec5a4,
	.Hoven = 0x004ee5fc,
	.OutpostX12 = 0x004e3f14,
	.KorgonOutpost = 0x004e172c,
	.Metropolis = 0x004e0a7c,
	.BlackwaterCity = 0x004de294,
	.CommandCenter = 0x004de41c,
	.BlackwaterDocks = 0x004e0c5c,
	.AquatosSewers = 0x004dff9c,
	.MarcadiaPalace = 0x004df8dc,
#endif
};

void hudRun(void)
{
    if (runOriginalHUD) {
        hudInfo.vtable.hud_setup();
        return;
    }

    int i = 0;
    int playerIndex = 0;
    int localCount = playerGetLocalCount();
    if (!localCount)
        return;
    // Loop through each player
    for (playerIndex; playerIndex < localCount; ++playerIndex) {
        Player *player = playerGetFromSlot(playerIndex);
        if (!player)
            continue;
        
        // if paused
        if (player->pauseOn) {
            // Hide health
            hudInfo.vtable.health(1, player);
        }
        // if not paused
        else {
            // printf("\nhudInfo.health: %08x", hudInfo.health);
            hudInfo.vtable.mapAndScore(playerIndex, 10);
        }
    }
}
// hud_setup func: 0x005451c8
void hudSetup(void)
{
    // store original functions.  Converts JAL to address.
    u32 hook = GetAddress(&vaHudSetup_Hook);
    hudInfo.vtable.hud_setup = JAL2ADDR(*(u32*)hook);
    u32 start = (u32)hudInfo.vtable.hud_setup;
    hudInfo.vtable.health = JAL2ADDR(*(u32*)(start + 0x60));
    hudInfo.vtable.mapAndScore = JAL2ADDR(*(u32*)(start + 0x6b8));
    hudInfo.vtable.getGadgetId = JAL2ADDR(*(u32*)(start + 0x7c));

    // hook our function
    HOOK_JAL(hook, &hudRun);
}

void hudInit(void)
{
    if (!isInGame()) {
        // zero ui struct if not in game.
        if (hudInfo.init > 0)
            memset(&hudInfo, 0, sizeof(hudInfo_t));
        
        return;
    }
    
    if (hudInfo.init == 0) {
        hudSetup();
        hudInfo.init = 1;
    } else if (hudInfo.init == 1) {
        hudRun();
    }
}
