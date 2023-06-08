/***************************************************
 * FILENAME :		cheats.c
 * 
 * DESCRIPTION :
 * 		Contains simple cheats/game settings for UYA.
 * 
 * NOTES :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */

#include <tamtypes.h>

#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/interop.h>


/*
 * NAME :		disableWeaponPacks
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Daniel "Dnawrkshp" Gerendasy
 */
void disableWeaponPacks(void)
{
    VariableAddress_t vaWeaponPackSpawnFunc = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004FA350,
        .Hoven = 0x004FC468,
        .OutpostX12 = 0x004F1D40,
        .KorgonOutpost = 0x004EF4D8,
        .Metropolis = 0x004EE828,
        .BlackwaterCity = 0x004EC0C0,
        .CommandCenter = 0x004EC088,
        .BlackwaterDocks = 0x004EE908,
        .AquatosSewers = 0x004EDC08,
        .MarcadiaPalace = 0x004ED588,
#else
        .Lobby = 0,
        .Bakisi = 0x004F7BD0,
        .Hoven = 0x004F9C28,
        .OutpostX12 = 0x004EF540,
        .KorgonOutpost = 0x004ECD58,
        .Metropolis = 0x004EC0A8,
        .BlackwaterCity = 0x004E98C0,
        .CommandCenter = 0x004E9A48,
        .BlackwaterDocks = 0x004EC288,
        .AquatosSewers = 0x004EB5C8,
        .MarcadiaPalace = 0x004EAF08,
#endif
    };

    u32 weaponPackSpawnFunc = GetAddress(&vaWeaponPackSpawnFunc);
    if (weaponPackSpawnFunc) {
        *(u32*)weaponPackSpawnFunc = 0;
        *(u32*)(weaponPackSpawnFunc - 0x7BF4) = 0;
    }
}

/*
 * NAME :		spawnWeaponPackOnDeath
 * 
 * DESCRIPTION :
 * 
 * 
 * NOTES :
 * 
 * ARGS : 
 * 
 * RETURN :
 * 
 * AUTHOR :			Troy "Agent Moose" Pruitt
 */

char SpawnedPack = 0;
void SpawnPack(u32 a0, u32 a1)
{
    VariableAddress_t vaRespawnFunc = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x0051a940,
        .Hoven = 0x0051ca58,
        .OutpostX12 = 0x00512330,
        .KorgonOutpost = 0x0050fac8,
        .Metropolis = 0x0050ee18,
        .BlackwaterCity = 0x0050c6b0,
        .CommandCenter = 0x0050c470,
        .BlackwaterDocks = 0x0050ecf0,
        .AquatosSewers = 0x0050dff0,
        .MarcadiaPalace = 0x0050d970,
#else
        .Lobby = 0,
        .Bakisi = 0x00518138,
        .Hoven = 0x0051a190,
        .OutpostX12 = 0x0050faa8,
        .KorgonOutpost = 0x0050d2c0,
        .Metropolis = 0x0050c610,
        .BlackwaterCity = 0x00509e28,
        .CommandCenter = 0x00509da8,
        .BlackwaterDocks = 0x0050c5e8,
        .AquatosSewers = 0x0050b928,
        .MarcadiaPalace = 0x0050b268,
#endif
    };

    VariableAddress_t vaSpawnWeaponPackFunc = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004fb188,
        .Hoven = 0x004fd2a0,
        .OutpostX12 = 0x004f2b78,
        .KorgonOutpost = 0x004f0310,
        .Metropolis = 0x004ef660,
        .BlackwaterCity = 0x004ecef8,
        .CommandCenter = 0x004ecec0,
        .BlackwaterDocks = 0x004ef740,
        .AquatosSewers = 0x004eea40,
        .MarcadiaPalace = 0x004ee3c0,
#else
        .Lobby = 0,
        .Bakisi = 0x004f8a08,
        .Hoven = 0x004faa60,
        .OutpostX12 = 0x004f0378,
        .KorgonOutpost = 0x004edb90,
        .Metropolis = 0x004ecee0,
        .BlackwaterCity = 0x004ea6f8,
        .CommandCenter = 0x004ea880,
        .BlackwaterDocks = 0x004ed0c0,
        .AquatosSewers = 0x004ec400,
        .MarcadiaPalace = 0x004ebd40,
#endif
    };
    // Spawn Pack if Health <= zero and if not spawned already.
    if (*(u32*)(TNW_PLAYERDATA + 0x44) <= 0 && SpawnedPack == 0)
    {
        SpawnedPack = 1;

        // Run normal function
        ((void (*)(u32, u32))GetAddress(&vaRespawnFunc))(a0, a1);

        // Spawn Pack
        ((void (*)(u32))GetAddress(&vaSpawnWeaponPackFunc))(PLAYER_STRUCT);
    }
}

void spawnWeaponPackOnDeath(void)
{
    VariableAddress_t vaRespawnPlayerHook = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x0052a09c,
        .Hoven = 0x0052c1b4,
        .OutpostX12 = 0x00521a8c,
        .KorgonOutpost = 0x0051f224,
        .Metropolis = 0x0051e574,
        .BlackwaterCity = 0x0051be0c,
        .CommandCenter = 0x0051bbcc,
        .BlackwaterDocks = 0x0051e44c,
        .AquatosSewers = 0x0051d74c,
        .MarcadiaPalace = 0x0051d0cc,
#else
        .Lobby = 0,
        .Bakisi = 0x0052781c,
        .Hoven = 0x00529874,
        .OutpostX12 = 0x0051f18c,
        .KorgonOutpost = 0x0051c9a4,
        .Metropolis = 0x0051bd08,
        .BlackwaterCity = 0x0051950c,
        .CommandCenter = 0x0051948c,
        .BlackwaterDocks = 0x0051bccc,
        .AquatosSewers = 0x0051b00c,
        .MarcadiaPalace = 0x0051a94c,
#endif
    };

    // Disable normal Weapon Pack spawns
    disableWeaponPacks();

    // if Health is greater than zero and pack has spawned
    if (*(u32*)(TNW_PLAYERDATA + 0x44) > 0 && SpawnedPack == 1)
        SpawnedPack = 0;

    // Hook SpawnPack (On Outpost x12 address: 0x0051F18C)
    HOOK_JAL(GetAddress(&vaRespawnPlayerHook), &SpawnPack);
}

void Debug()
{
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
    static int Active = 0;
    // DEBUG OPTIONS: L3 = Spawn Pack, R3 = Hurt Player
    if ((pad->btns & PAD_L3) == 0 && Active == 0)
	{
        Active = 1;
        // Spawn Pack (Outpost X12)
        //((void (*)(u32))0x004F0378)(0x002F7900);
        ((void (*)(u32, u32, u32, u32))0x004BADF0)(0, 0, 0x01C7D740, 0x10c3);
        // Heal Player
        //((void (*)(u32, u32))0x0041BE98)(0x01C78440, PlayerStruct);
        //((void (*)(int))0x004E3080)(1);
    }
    else if ((pad->btns & PAD_R3) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        ((void (*)(u32, u16, u16))0x00502658)(0x002F7900, 1, 0);
    }
    if (!(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0)
    {
        Active = 0;
    }
}

int main()
{
    if (isInGame())
    {
        spawnWeaponPackOnDeath();
        Debug();
    }
    return 0;
}
