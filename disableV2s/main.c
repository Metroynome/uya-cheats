#include <tamtypes.h>

#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/interop.h>


void disableV2s(void)
{
    VariableAddress_t vaWeaponMeterAddress = {
#ifdef UYA_PAL
        .Lobby = 0,
        .Bakisi = 0x004fb6d0,
        .Hoven = 0x004fd7e8,
        .OutpostX12 = 0x004f30c0,
        .KorgonOutpost = 0x004f0858,
        .Metropolis = 0x004efba8,
        .BlackwaterCity = 0x004ed440,
        .CommandCenter = 0x004ed408,
        .BlackwaterDocks = 0x004efc88,
        .AquatosSewers = 0x004eef88,
        .MarcadiaPalace = 0x004ee908,
#else
        .Lobby = 0,
        .Bakisi = 0x004f8f50,
        .Hoven = 0x004fafa8,
        .OutpostX12 = 0x005018C0,
        .KorgonOutpost = 0x004ee0d8,
        .Metropolis = 0x004ed428,
        .BlackwaterCity = 0x004eac40,
        .CommandCenter = 0x004eadc8,
        .BlackwaterDocks = 0x004ed608,
        .AquatosSewers = 0x004ec948,
        .MarcadiaPalace = 0x004ec288,
#endif
    };
    u32 addr = GetAddress(&vaWeaponMeterAddress);
    if (*(u32*)addr == 0xA1220000) // sw v0, 0x0(t0)
    {
        // Meter Kills: 0 to 2:
        *(u32*)addr = 0xA1200000; // sw zero, 0x0(t0)
        // Meter Kills: 3:
        *(u32*)(addr + 0x10) = 0xA1200000; // sw zero, 0x0(t0)
         // jal to fully upgrade to v2
        *(u32*)(addr + 0x16C) = 0;
    }
}

int main()
{
    if (isInGame())
    {
        disableV2s();
    }
    return 0;
}
