#include <tamtypes.h>
#include <libuya/game.h>
#include <libuya/player.h>
#include <libuya/pad.h>
#include <libuya/interop.h>

#if UYA_PAL

VariableAddress_t vaPlayerUpdateHealthJAL = {
	.Lobby = 0,
	.Bakisi = 0x0050d4d8,
	.Hoven = 0x0050f5f0,
	.OutpostX12 = 0x00504ec8,
    .KorgonOutpost = 0x00502660,
	.Metropolis = 0x005019b0,
	.BlackwaterCity = 0x004ff248,
	.CommandCenter = 0x004ff210,
    .BlackwaterDocks = 0x00501a90,
    .AquatosSewers = 0x00500d90,
    .MarcadiaPalace = 0x00500710,
};

#else

VariableAddress_t vaPlayerUpdateHealthFunc = {
	.Lobby = 0,
	.Bakisi = 0x0050ace8,
	.Hoven = 0x0050cd40,
	.OutpostX12 = 0x00502658,
    .KorgonOutpost = 0x004ffe70,
	.Metropolis = 0x004ff1c0,
	.BlackwaterCity = 0x004fc9d8,
	.CommandCenter = 0x004fcb60,
    .BlackwaterDocks = 0x004ff3a0,
    .AquatosSewers = 0x004fe6e0,
    .MarcadiaPalace = 0x004fe020,
};

#endif

int main(void)
{
    static int _InfiniteHealthMoonjump_Init = 0;

	if (isInGame())
	{
        int Joker = *(u16*)0x00225982;
        if (Joker == 0xFDFB){
		    _InfiniteHealthMoonjump_Init = 1;
        }
        else if (Joker == 0xFFFD)
        {
            _InfiniteHealthMoonjump_Init = 0;
        }

        if (!_InfiniteHealthMoonjump_Init)
            return -1;
        
        int UpdateHealth = GetAddress(&vaPlayerUpdateHealthFunc);
        int PlayerGravity = (u32)PLAYER_STRUCT + 0x128;
        if (_InfiniteHealthMoonjump_Init)
        {
            if (*(u32*)UpdateHealth != 0x03e00008)
            {
                *(u32*)UpdateHealth = 0x03e00008;
                *(u32*)(UpdateHealth + 0x4) = 0;
            }
            if (Joker == 0xBFFF)
                *(float*)PlayerGravity = 0.125;
        }
        else
        {
            if (*(u32*)UpdateHealth == 0x03e00008)
            {
                *(u32*)UpdateHealth = 0x0080602D;
                *(u32*)(UpdateHealth + 0x4) = 0x91821A14;
            }
        }
	}
	return 0;
}
