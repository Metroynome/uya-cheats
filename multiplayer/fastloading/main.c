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
		#ifdef UYA_PAL
		POKE_U32(0x0060bea4, 0); // Transition_DoTransition():disable scegssync 1
		POKE_U32(0x0060bf48, 0); // Transition_DoTransition():disable scegssync 2
		POKE_U32(0x0058a5fc, 0); // FadeToBlack(): disable scegssync 1
		POKE_U32(0x0058a6d0, 0); // FadeToBlack(): disable scegssync 2
		POKE_U32(0x0058a704, 0); // FadeToBlack(): disable scegssync 3
		POKE_U32(0x00195c94, 0); // nwUpdate(): disable timebandithack
		#else
		POKE_U32(0x006097a4, 0); // Transition_DoTransition():disable scegssync 1
		POKE_U32(0x00609848, 0); // Transition_DoTransition():disable scegssync 2
		POKE_U32(0x0058948c, 0); // FadeToBlack(): disable scegssync 1
		POKE_U32(0x0058955c, 0); // FadeToBlack(): disable scegssync 2
		POKE_U32(0x00589594, 0); // FadeToBlack(): disable scegssync 3
		POKE_U32(0x00195d84, 0); // nwUpdate(): disable timebandithack
		#endif
	}
	if (!isSceneLoading()) {
		#ifdef UYA_PAL
		POKE_U32(0x00195c94, 0xac620128); // reenable timebandit
		#else
		POKE_U32(0x00195d84, 0xac620128); // reenable timebandit
		#endif
	}
}

int main()
{
	uyaPreUpdate();

	fastLoading();
	
	uyaPostUpdate();

    return 0;
}
