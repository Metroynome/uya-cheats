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
    int (*getGadgetId)(Player *player);
    void (*ammo)(int ammoLeft, int amooMax, int isShown);
    void (*vehiclePlayerArrows)(int sprite, int isFirstArrowShown, int isSecondArrowShown);
    void (*weaponLevel)(int level, Player *player);
    void (*quickSelect)(int isShown, int ten);
    void (*weapons)(int sprite, int level, int slot, int isShown);
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

int deobfuscate(int src)
{
    return playerDeobfuscate(src, DEOBFUSCATE_ADDRESS_WEAPON, DEOBFUSCATE_MODE_WEAPON);
}

void hudRun(void)
{
    if (hudInfo.runOldHud) {
        hudInfo.vtable.hud_setup();
        return;
    }
    int quickSelectSlot = 0;
    int playerIndex = 0;
    int localCount = playerGetLocalCount();
    // Loop through each player
    for (playerIndex; playerIndex < localCount; ++playerIndex) {
        Player *player = playerGetFromSlot(playerIndex);
        if (!player)
            continue;
        
        // if paused, hide all hud.
        if (player->pauseOn) {
            hudInfo.vtable.health(0, player);
            return;
        }
        // hudInfo.vtable.mapAndScore(playerIndex, 10);
        int handGadgetId = hudInfo.vtable.getGadgetId(player);
        int weaponPVar = 0;
        GadgetDef * weapon = weaponGadgetList();
        int isWrench = handGadgetId == WEAPON_ID_WRENCH;
        int isSwingshot = handGadgetId == WEAPON_ID_SWINGSHOT;
        Vehicle *inVehicle = player->vehicle;
        int hideQuickSelect = (isWrench || isSwingshot) && !inVehicle == 1;
        // Quick Select and Vehicle HUD
        if (weapon && handGadgetId != -1) {
            for (quickSelectSlot; quickSelectSlot < 3; ++quickSelectSlot) {
                int sprite = -1;
                short ammo = 0, ammoMax = 0, exp = 0;
                short weaponIndex = deobfuscate(&player->quickSelect.Slot[quickSelectSlot]);
                if (weaponIndex && !inVehicle) {
                    sprite = &weapon[weaponIndex].sprite;
                    sprite = *(u16*)sprite;
                    ammo = deobfuscate(&player->weaponAmmo.Slot[weaponIndex]);
                    ammoMax = weapon[weaponIndex].ammoAmount;
                    exp = deobfuscate(&player->weaponMeter.Slot[weaponIndex]);
                }
                if (inVehicle) {
                    int vType = inVehicle->vehicleType;
                    ammo =  5;
                    ammoMax = (vType == 1) ? 8 : 5;
                    int vehSprite = (vType == 2) ? SPRITE_PLAYER_TURRET : SPRITE_TURBOSLIDER;
                    if (vType == 1) {
                        vehSprite = SPRITE_HOVERSHIP;
                        int vState =  (player->vehicleState == 3 || player->vehicleState == 5) ? 0x3d4 : 0x3dc;
                        ammo = *(int*)((u32)inVehicle->pMoby->pVar + (u32)vState);
                    }
                    if (quickSelectSlot == 0)
                        sprite = vehSprite;

                    hudInfo.vtable.health(10, player);
                    hudInfo.vtable.vehiclePlayerArrows(vehSprite, inVehicle->pDriver != 0, inVehicle->pPassenger != 0);  
                }
                if (quickSelectSlot == 0) {
                    if (handGadgetId != WEAPON_ID_MORPH || inVehicle)
                        hudInfo.vtable.ammo(ammo, ammoMax, 0);
                    if (handGadgetId != WEAPON_ID_HOLO)
                        hudInfo.vtable.weaponLevel(exp, player);
                }

                hudInfo.vtable.weapons(sprite, exp, quickSelectSlot, 0);
            }
        }
        // Healthbar HUD
        // pressing Triangle to show is done in HeroUpdate();  
        if (playerPadGetButtonDown(player, PAD_L3) > 0 && player->hudHealthTimer < 1 && deobfuscate(&player->state) != PLAYER_STATE_VEHICLE) {
            player->hudHealthTimer = 360;
        } else {
            hudInfo.vtable.health(player->hudHealthTimer, player);
            player->hudHealthTimer = 0;
        }
        hudInfo.vtable.quickSelect(hideQuickSelect, 10);
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
