#include <tamtypes.h>
#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/uya.h>
#include <libuya/ui.h>

int remapButtons(pad)
{
	// unmask the pad data into its bits.
	switch (*(u16*)pad ^ 0xffff) {
		// if Presssing X, return by telling it to press circle instead.
		case PAD_CROSS: return PAD_CIRCLE ^ (0xffff & (*(u16*)pad | PAD_CROSS));

		// if dpad is pressed, return by acting as if they were not pressed.
		case PAD_LEFT: return (0xffff & (*(u16*)pad | PAD_LEFT));
		case PAD_RIGHT: return (0xffff & (*(u16*)pad | PAD_RIGHT));
		case PAD_UP: return (0xffff & (*(u16*)pad | PAD_UP));
		case PAD_DOWN: return (0xffff & (*(u16*)pad | PAD_DOWN));

		// if nothing is pressed, then return original data.
		default: return *(u16*)pad;
	}
}

void patchRemapButtons(void * destination, void * source, int num)
{
	Player * player = playerGetFromSlot(0);
	// make sure the pause menu is not open.  this way the pause menu can still be used.
	if (player && !player->pauseOn) {
		u32 paddata = (void*)((u32)source + 0x2);
		// edit the pad data.
		*(u16*)paddata = remapButtons(paddata);
	}
	// finish up by running the original command we took over.
	memcpy(destination, source, num);
}

int main()
{
	uyaPreUpdate();

	// Start by checking if we are in game.  If so, hook our function!
	// Lucky these addresses are the same in NTSC and PAL!
	if (isInGame())
		HOOK_JAL(0x0013cae0, &patchRemapButtons);
	
	uyaPostUpdate();

    return 0;
}
