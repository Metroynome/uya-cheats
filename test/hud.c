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

typedef struct hudFunctions {
    int init;
    u32 hudAddr;
    u32 health;
    u32 mapAndScore;
} hudFunctions_t;
hudFunctions_t HUD;

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

// hud_setup func: 0x005451c8
void hudRun(void)
{
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
            ((void(*)(u32, Player *))HUD.health)(0, player);
        }
        // if not paused
        else {
            // printf("\nHUD.health: %08x", HUD.health);
            ((void(*)(int, u32))HUD.mapAndScore)(playerIndex, 10);
        }
    }
}

void hudSetup(void)
{
    // store original functions.  Converts JAL to address.
    u32 hook = GetAddress(&vaHudSetup_Hook);
    HUD.hudAddr = JAL2ADDR(*(u32*)hook);
    HUD.health = JAL2ADDR(*(u32*)(HUD.hudAddr + 0x60));
    HUD.mapAndScore = JAL2ADDR(*(u32*)(HUD.hudAddr + 0x6B8));

    // hook our function
    HOOK_JAL(hook, &hudRun);
}

void hudInit(void)
{
    if (!isInGame()) {
        // zero HUD struct if not in game.
        if (HUD.init > 0)
            memset(&HUD, 0, sizeof(hudFunctions_t));
        
        return;
    }
    
    if (HUD.init == 0) {
        hudSetup();
        HUD.init = 1;
    } else if (HUD.init == 1) {
        hudRun();
    }
}
