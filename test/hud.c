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

#define MAPSCORE_BUTTON (0x001fc87d)

enum MapScore_Button {
    MAPSCORE_SELECT = 0,
    MAPSCORE_R3 = 1,
    MAPSCORE_BOTH = 2
};

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
    void (*ctfAndSiege)(int ten, void *blueTeamPoints, void *redTeamPoints);
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
    float blueTeam;
    int redTeam = 0;
    float *blue = &blueTeam;
    int *red = redTeam;
    int quickSelectSlot = 0;
    int playerIndex = 0;
    int localCount = playerGetLocalCount();
    GameData *gameData = gameGetData();
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
        register int gp asm("gp");
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
        if (player->hudHealthTimer == 0 && playerPadGetButtonDown(player, PAD_L3) != 0 && !inVehicle) {
            #if UYA_PAL
            player->hudHealthTimer = 324; // 360 - 10% = 324
            #else
            player->hudHealthTimer = 360;
            #endif
        } else if (player->hudHealthTimer > 0) {
            hudInfo.vtable.health(player->hudHealthTimer, player);
            player->hudHealthTimer = 0;
        }
        int MapScoreButton = 0;
        switch (*(u8*)MAPSCORE_BUTTON) {
            case MAPSCORE_SELECT: MapScoreButton = player->pPad->hudBitsOn >> 10 & 1; break;
            case MAPSCORE_R3: MapScoreButton = player->pPad->hudBitsOn >> 8 & 1; break;
            case MAPSCORE_BOTH: MapScoreButton = (player->pPad->hudBitsOn & 0x5000U) != 0; break;
        }
        if (MapScoreButton == 0) {
            int mapTimer = timeDecTimer(&player->LocalHero.mapTimer);
            if (mapTimer == 2 && (player->pPad->hudBits & 0x400U) != 0) {
                int showMapScore = hudInfo.vtable.checkMapScore(playerIndex);
                hudInfo.vtable.mapAndScore(playerIndex, showMapScore ^ 1);
            }
        } else {
            player->LocalHero.mapTimer = 10;
            if ((player->pPad->hudBitsOn & 0x100U) != 0) {
                int showMapScore = hudInfo.vtable.checkMapScore(playerIndex);
                hudInfo.vtable.mapAndScore(playerIndex, showMapScore ^ 1);
            }
        }
        int check = hudInfo.vtable.checkMapScore(playerIndex);
        if (check == 0) {
            hudInfo.vtable.radar(playerIndex, 10);
            // Timer HUD
            if (gameData->timeEnd != -1) {
                hudInfo.vtable.timeLeft(playerIndex, 10);
            }
        } else {
            hudInfo.vtable.mapAndScore(playerIndex, 10);
        }
        // Seige/CTF HUD
        int gameModeHud = 1;
        GameSettings *gs = gameGetSettings();
        float blueScore = 0.0;
        float redScore = 0.0;
        if (gs->GameType == GAMERULE_SIEGE) {
            // *test: value
            // test: address
            red = redTeam;
            int *gp_e508 = (int *)(gp - 0x1af8);
            int *gp_e4e8 = (int *)(gp - 0x1a18);
            int *unk = gp_e508;
            int end = 0x00246650;
            int rawr;
            int i = 0;
            for (i; i < GAME_MAX_PLAYERS; ++i) {
                float *prevBaseHealth = &gameData->allYourBaseGameData->prevHudHealth[i];
                float *hudBaseHealth = &gameData->allYourBaseGameData->hudHealth[i];
                if (*prevBaseHealth == *hudBaseHealth) {
                    rawr = *unk;
                } else {
                    *unk = (int *)(gp - 0x1ad8);
                    *prevBaseHealth = *hudBaseHealth;
                    rawr = *unk;
                }
                if (rawr < 1) {
                    rawr = *gp_e4e8;
                } else if (*gp_e4e8 < 1) {
                    *gp_e4e8 = *(int*)(gp - 0x1ad4);
                    rawr = *gp_e4e8;
                } else {
                    rawr = *gp_e4e8;
                }
                *red = (float)((int)rawr / *(int*)(gp - 0x1ad4));
                float prevHealth = gameData->allYourBaseGameData->prevHudHealth[i];
                float totalBaseHealth = gameData->allYourBaseGameData->totalHudHealth[i]; 
                *blue = prevHealth / totalBaseHealth;
                if (1.0f < (prevHealth / totalBaseHealth)) {
                    *blue = 1.0f;
                }
                if (0.0f > *blue) {
                    *blue = 0.0f;
                }
                blue = blue + 1;
                timeDecTimer(gp_e4e8);
                red = red + 1;
                timeDecTimer(gp_e508);
                gp_e508 = gp_e508 + 1;
                unk = unk + 1;
            }
        } else if (gs->GameType == GAMERULE_DM) {
            gameModeHud = 0;
        }
        // Enable gamemode HUD
        if (gameModeHud) {
            if (redTeam == 0 || *(int*)(gp - 0x6ea0) == 2)
                hudInfo.vtable.ctfAndSiege(10, &blueTeam, redTeam);
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
    hudInfo.vtable.mapAndScore = JAL2ADDR(*(u32*)(start + 0x62c));
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
