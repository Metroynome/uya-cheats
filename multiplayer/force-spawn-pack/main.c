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

int SpawnedPack = 0;
void SpawnPack(int a0, int a1, int a2, int a3)
{
    VariableAddress_t vaRespawnTimerFunc = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004e3690,
        .Hoven = 0x004e57a8,
        .OutpostX12 = 0x004db080,
        .KorgonOutpost = 0x004d8818,
        .Metropolis = 0x004d7b68,
        .BlackwaterCity = 0x004d5400,
        .CommandCenter = 0x004d53c8,
        .BlackwaterDocks = 0x004d7c48,
        .AquatosSewers = 0x004d6f48,
        .MarcadiaPalace = 0x004d68c8,
#else
        .Lobby = 0,
        .Bakisi = 0x004e0fd0,
        .Hoven = 0x004e3028,
        .OutpostX12 = 0x004D8940, // New address
        .KorgonOutpost = 0x004d6158,
        .Metropolis = 0x004d54a8,
        .BlackwaterCity = 0x004d2cc0,
        .CommandCenter = 0x004d2e48,
        .BlackwaterDocks = 0x004d5688,
        .AquatosSewers = 0x004d49c8,
        .MarcadiaPalace = 0x004d4308,
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

    // Run Original Respawn Timer Hook
	((void (*)(int, int, int, int))GetAddress(&vaRespawnTimerFunc))(a0, a1, a2, a3);

    // Spawn Pack if Health <= zero and if not spawned already.
    if (playerGetHealth(PLAYER_STRUCT) <= 0 && !SpawnedPack)
    {
        // Spawn Pack
        ((void (*)(u32))GetAddress(&vaSpawnWeaponPackFunc))(PLAYER_STRUCT);
        // It now spawned pack, so set to true.
        SpawnedPack = 1;
    }
}

void spawnWeaponPackOnDeath(void)
{
    VariableAddress_t vaRespawnTimerHook = {
#if UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004b00f4,
        .Hoven = 0x004b220c,
        .OutpostX12 = 0x004a7ae4,
        .KorgonOutpost = 0x004a527c,
        .Metropolis = 0x004a45cc,
        .BlackwaterCity = 0x004a1e64,
        .CommandCenter = 0x004a1e5c,
        .BlackwaterDocks = 0x004a46dc,
        .AquatosSewers = 0x004a39dc,
        .MarcadiaPalace = 0x004a335c,
#else
        .Lobby = 0,
        .Bakisi = 0x004adba4,
        .Hoven = 0x004afbfc,
        .OutpostX12 = 0x004a5514, // New Hook
        .KorgonOutpost = 0x004a2d2c,
        .Metropolis = 0x004a207c,
        .BlackwaterCity = 0x0049f894,
        .CommandCenter = 0x0049fa4c,
        .BlackwaterDocks = 0x004a228c,
        .AquatosSewers = 0x004a15cc,
        .MarcadiaPalace = 0x004a0f0c,
#endif
    };

    // Disable normal Weapon Pack spawns
    disableWeaponPacks();

    // Hook CTF/Siege SpawnPack
    if (*(u32*)GetAddress(&vaRespawnTimerHook) != (0x0C000000 | ((u32)(&SpawnPack) >> 2)))
        HOOK_JAL(GetAddress(&vaRespawnTimerHook), &SpawnPack);
    
    // Hook DM SpawnPack
    if (*(u32*)((u32)GetAddress(&vaRespawnTimerHook) + 0x2100) != (0x0C000000 | ((u32)(&SpawnPack) >> 2)))
        HOOK_JAL(((u32)GetAddress(&vaRespawnTimerHook) + 0x2100), &SpawnPack);

    // if Health is greater than zero and pack has spawned
    // This will be checking constantly, instead of just when the player dies.
    if (playerGetHealth(PLAYER_STRUCT) > 0)
        SpawnedPack = 0;
}

void Debug()
{
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
    static int Active = 0;
    // DEBUG OPTIONS: L3 = Spawn Pack, R3 = Hurt Player
    if ((pad->btns & PAD_L3) == 0 && Active == 0)
	{
        Active = 1;

    }
    else if ((pad->btns & PAD_R3) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerDecHealth(PLAYER_STRUCT, 15);
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
