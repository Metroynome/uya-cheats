#include <tamtypes.h>

#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/uya.h>
#include <libuya/interop.h>
#include <libuya/graphics.h>
#include <libuya/gamesettings.h>

void fastLoading(void)
{
	// if (!MapLoaderState.Enabled) return; // only custom maps
	GameSettings* gs = gameGetSettings();
	if (!gs) return;

	if (gs->GameStartTime < 0 && isSceneLoading()) {
		// faster loading into game
		// -- Transition_DoTransition():disable scegssync
		POKE_U32(0x006097a4, 0);
		POKE_U32(0x00609848, 0);
		// -- FadeToBlack(): disable scegssync
		POKE_U32(0x0058948c, 0);
		POKE_U32(0x0058955c, 0);
		POKE_U32(0x00589594, 0);
		// -- nwUpdate(): disable timebandithack
		// POKE_U32(0x00195d84, 0);
	}
	// if (!isSceneLoading()) {
	// 	POKE_U32(0x00195d84, 0xac620128);
	// }
}

int main()
{
	uyaPreUpdate();

	fastLoading();
	
	uyaPostUpdate();

    return 0;
}
