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

typedef struct hud_frames {
    // 0: hidden, 1: shown, 2: always shown
    char weapons;
} hud_frames_t;

typedef struct hud_vtable {
    void (*hud_setup)();
    void (*health)(int isShown, Player *player);
    int (*getGadgetId)(Player * player);
    void (*ammo)(int ammoLeft, int amooMax, int isShown);
    void (*vehiclePlayerArrows)(int isFirstArrowShown, int isSecondArrowShown);
    void (*weaponLevel)(int a0, Player *player);
    void (*quickSelect)(int unk_24c9, int ten);
    void (*weapons)(int sprite, int weapon, int num, int isShown);
    int (*checkMapScore)(int playerIndex);
    void (*radar)(int unk_24c9, int ten);
    void (*timer)(int playerIndex, int ten);
    void (*mapAndScore)(int index, int a1);
    void (*ctfAndSiege)(int ten, u32 CTF_BlueDivRed, u32 local_90);
    void (*localPlayers_2)(int ten);
    void (*localPlayers_3)(int ten);
    void (*timeLeft)();
} hud_vtable_t;

typedef struct hudInfo {
    short init;
    short runOldHud;
    hud_frames_t frames;
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
    if (hudInfo.runOldHud) {
        hudInfo.vtable.hud_setup();
        return;
    }
    int i = 0;
    int playerIndex = 0;
    int localCount = playerGetLocalCount();
    // Loop through each player
    for (playerIndex; playerIndex < localCount; ++playerIndex) {
        Player *player = playerGetFromSlot(playerIndex);
        if (!player)
            continue;
        
        int handGadgetId = hudInfo.vtable.getGadgetId(player);
        int weaponPVar = 0;
        // if paused
        if (player->pauseOn) {
            // Hide health
            hudInfo.vtable.health(1, player);
        }
        // if not paused
        else if (!player->pauseOn) {
            // hudInfo.vtable.mapAndScore(playerIndex, 10);
            GadgetDef * weapon = (handGadgetId) ? weaponGadgetList() : 0;
            short mainWeapon = playerDeobfuscate(&player->quickSelect.Slot[0], 1, 1);
            printf("\nhandgadgetId: %d", handGadgetId);
            if (!player->vehicle && weapon != 0) {
                int j = handGadgetId;
                for (i; i < 3; ++i) {
                    short weaponSlot = playerDeobfuscate(&player->quickSelect.Slot[i], 1, 1);
                    int sprite = mainWeapon != 0 ? &weapon[j].sprite : 0;
                    short ammo = mainWeapon != 0 ? playerDeobfuscate(&player->weaponAmmo.Slot[j], 1, 1) : 0;
                    short ammoMax = weapon[j].ammoAmount;
                    short exp = playerDeobfuscate(&player->weaponMeter.Slot[j], 1, 1);
                    hudInfo.vtable.weapons(*(u16*)sprite, ammo, i, 0);
                    if (i == 0) {
                        printf("\nsprite: %08x %08x", &weapon[j].sprite, weapon[j].sprite);
                        printf("\nammo: %08x %08x", &player->weaponAmmo.Slot[j], ammo);
                        printf("\nammoMax: %08x %08x", &weapon[j].ammoAmount, weapon[j].ammoAmount);
                        printf("\nexp: %08x %08x", &player->weaponMeter.Slot[j], exp);

                        if (handGadgetId != WEAPON_ID_MORPH)
                            hudInfo.vtable.ammo(ammo, ammoMax, 0);
                        if (handGadgetId != WEAPON_ID_HOLO)
                            hudInfo.vtable.weaponLevel(exp, player);
                        
                        hudInfo.vtable.quickSelect(0, 10);
                    }
                }
            }
        }
    }
}
// hud_setup func: 0x005451c8
int hudInit(void)
{
    // store original functions.  Converts JAL to address.
    u32 hook = GetAddress(&vaHudSetup_Hook);
    u32 start = JAL2ADDR(*(u32*)hook);
    hudInfo.vtable.hud_setup = start;
    hudInfo.vtable.health = JAL2ADDR(*(u32*)(start + 0x60));
    hudInfo.vtable.getGadgetId = JAL2ADDR(*(u32*)(start + 0x7c));
    hudInfo.vtable.ammo = JAL2ADDR(*(u32*)(start + 0x140));
    hudInfo.vtable.vehiclePlayerArrows = JAL2ADDR(*(u32*)(start + 0x174));
    hudInfo.vtable.weaponLevel = JAL2ADDR(*(u32*)(start + 0x42c));
    hudInfo.vtable.quickSelect = JAL2ADDR(*(u32*)(start + 0x454));
    hudInfo.vtable.weapons = JAL2ADDR(*(u32*)(start + 0x470));
    hudInfo.vtable.checkMapScore = JAL2ADDR(*(u32*)(start + 0x664));
    hudInfo.vtable.radar = JAL2ADDR(*(u32*)(start + 0x68c));
    hudInfo.vtable.timer = JAL2ADDR(*(u32*)(start + 0x6a8));
    hudInfo.vtable.mapAndScore = JAL2ADDR(*(u32*)(start + 0x6b8));
    hudInfo.vtable.ctfAndSiege = JAL2ADDR(*(u32*)(start + 0x950));
    hudInfo.vtable.localPlayers_2 = JAL2ADDR(*(u32*)(start + 0x970));
    hudInfo.vtable.localPlayers_3 = JAL2ADDR(*(u32*)(start + 0x990));
    hudInfo.vtable.timeLeft = JAL2ADDR(*(u32*)(start + 0x9E4));

    // hook our function
    HOOK_JAL(hook, &hudRun);
    return 1;
}

void hud(void)
{
    if (!isInGame()) {
        // zero ui struct if not in game.
        if (hudInfo.init > 0)
            memset(&hudInfo, 0, sizeof(hudInfo_t));
        
        return;
    }
    
    if (hudInfo.init == 0)
        hudInfo.init = hudInit();

    hudRun();
}
