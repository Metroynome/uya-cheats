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
#include <libuya/spawnpoint.h>
#include <libuya/team.h>
#include <libuya/ui.h>
#include <libuya/time.h>

int hideDebugInfo_Local = 0;

void debugLocal(Player *this)
{
	// toggle local debug info
	if (playerPadGetButtonDown(this, PAD_L3 | PAD_L1) > 0) {
		 hideDebugInfo_Local = !hideDebugInfo_Local;
	}

	// hide all info if paused, or if button combo to togggle was pressed
	if (this->pauseOn || hideDebugInfo_Local)
		return;

	int handGadgetId = this->gadget.weapon.id;
	int isWrench = handGadgetId == WEAPON_ID_WRENCH;
	int isSwingshot = handGadgetId == WEAPON_ID_SWINGSHOT;
	Vehicle *inVehicle = this->vehicle;
	int hideQuickSelect = (isWrench || isSwingshot) && !(inVehicle == 1);
	// show weapon ID's
	if (!hideQuickSelect) {
		char weaponIds[3][4];
		int i = 0;
		float y[3] = {SCREEN_HEIGHT * 0.13, SCREEN_HEIGHT * 0.22, SCREEN_HEIGHT * 0.28};
		for (i; i < 3; ++i) {
			snprintf(weaponIds[i], 4, "%i", playerGetGadgetId(this, i));
			gfxScreenSpaceText(SCREEN_WIDTH * 0.1, y[i], 1, 1, 0x80ffffff, weaponIds[i], -1, TEXT_ALIGN_BOTTOMRIGHT, FONT_BOLD);
		}
	}

	// show health intager
	if (this->hudHealthTimer) {
		char healthBuff[4];
		snprintf(healthBuff, 4, "%i", playerGetHealth(this));
		gfxScreenSpaceText(SCREEN_WIDTH * 0.632, SCREEN_HEIGHT * 0.09, 1, 1, 0x80ffffff, healthBuff, -1, TEXT_ALIGN_BOTTOMRIGHT, FONT_BOLD);
	}
}

void debugAll(Player **players)
{
	return 0;
}

void debug(void)
{
	GameData *gameData = gameGetData();
	GameSettings *gameSettings = gameGetSettings();
	Player **players = playerGetAll();
	Player *player = NULL;
	int i = 0;
	for (i; i < GAME_MAX_LOCALS; ++i) {
		player = players[i];
		if (!player)
			continue;

		// debug info for local player
		if (i == 0) {
			debugLocal(player);
		}
		// debug infor for all other players
		debugAll(i);
	}
}

int main()
{
	// ((void (*)(void))0x00126780)();
	uyaPreUpdate();

    if (isInGame())
		debug();
	
	uyaPostUpdate();

    return 0;
}
