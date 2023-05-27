#include <tamtypes.h>
#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/ui.h>
#include <libuya/interop.h>
#include <libuya/utils.h>

const char * msg = "PATCH CONFIG";

  VariableAddress_t vaPauseMenuAddr = {
#if UYA_PAL
    .Lobby = 0,
    .Bakisi = 0x003c073c,
    .Hoven = 0x003bfd7c,
    .OutpostX12 = 0x003b7c7c,
    .KorgonOutpost = 0x003b7a38,
    .Metropolis = 0x003b77bc,
    .BlackwaterCity = 0x003b79fc,
    .CommandCenter = 0x003c7f7c,
    .BlackwaterDocks = 0x003c837c,
    .AquatosSewers = 0x003c8afc,
    .MarcadiaPalace = 0x003c807c,
#else
    .Lobby = 0,
    .Bakisi = 0x003c087c,
    .Hoven = 0x003bfebc,
    .OutpostX12 = 0x003b7dbc,
    .KorgonOutpost = 0x003b7b7c,
    .Metropolis = 0x003b78fc,
    .BlackwaterCity = 0x003b7b3c,
    .CommandCenter = 0x003c80bc,
    .BlackwaterDocks = 0x003c84bc,
    .AquatosSewers = 0x003c8c3c,
    .MarcadiaPalace = 0x003c81bc,
#endif
  };

void func(void)
{
  printf("HELLO YES I AM FUNCTION");
}

int main(void)
{
	if (isInGame())
	{
        // Get Menu address via current map.
        u32 Addr = GetAddress(&vaPauseMenuAddr);
        // Insert needed ID, returns string.
        int s = uiMsgString(0x1115); // Washington, D.C. string ID
        // Replace "Washington, D.C." string with ours.
        strncpy((char*)s, msg, 13);
        // Set "CONTINUE" string ID to our ID.
        *(u32*)Addr = 0x1115;
        // Pointer to "CONTINUE" function
        u32 ReturnFunction = *(u32*)(Addr + 0x8);
        // Hook Patch Config into end of "CONTINUE" function.
        HOOK_J((ReturnFunction + 0x54), &func);

	}
	return 0;
}
