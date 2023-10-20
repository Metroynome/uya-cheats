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

void InfiniteChargeboot(void)
{
	int i;
	Player ** players = playerGetAll();
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * player = players[i];
		if (!player)
			continue;

		if (player->timers.IsChargebooting == 1 && playerPadGetButton(player, PAD_R2) > 0 && player->timers.state > 55)
			player->timers.state = 55;
	}
}

void InfiniteHealthMoonjump(void)
{
    static int _InfiniteHealthMoonjump_Init = 0;
    int Joker = *(u16*)0x00225982;
    if (Joker == 0xFDFB)
        _InfiniteHealthMoonjump_Init = 1;
    else if (Joker == 0xFFFD)
        _InfiniteHealthMoonjump_Init = 0;

    if (_InfiniteHealthMoonjump_Init)
    {
        Player * player = (Player*)PLAYER_STRUCT;
		if (playerGetHealth(player) < 15)
        	playerSetHealth(player, 15);

        if (Joker == 0xBFFF)
            (float)player->move.behavior[2] = 0.125;
    }
}

void Debug()
{
    static int Active = 0;
	static int Occlusion = 2; // Default Occlusion = 2
	Player * player = (Player*)PLAYER_STRUCT;
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;

    /*
		DEBUG OPTIONS:
		PAD_UP: Occlusion on/off
		PAD_DOWN: Gattling Turret Health to 1
		PAD_LEFT: Change Team (red <-> blue)
		PAD_RIGHT: Hurt Player
	*/
    if ((pad->btns & PAD_LEFT) == 0 && Active == 0)
	{
        Active = 1;
		// Swap Teams (blue <-> red)
		player->mpTeam = !player->mpTeam;

	}
    else if ((pad->btns & PAD_RIGHT) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerDecHealth(player, 15);
    }
	else if ((pad->btns & PAD_UP) == 0 && Active == 0)
	{
		Active = 1;
		Occlusion = (Occlusion == 2) ? 0 : 2;
		gfxOcclusion(Occlusion);
	}
	else if((pad->btns & PAD_DOWN) == 0 && Active == 0)
	{
		Active = 1;
	}
    if (!(pad->btns & PAD_LEFT) == 0 && !(pad->btns & PAD_RIGHT) == 0 && !(pad->btns & PAD_UP) == 0 && !(pad->btns & PAD_DOWN) == 0)
    {
        Active = 0;
    }
}



// ===============================================================

int BackupMapScoreboard = 0;
int ToggleMapScoreboard = -1;
int BtnActive = 0;
void toggleMapScoreboard(int a0, int toggle)
{
	if (ToggleMapScoreboard == -1)
	{
		return;
	}
	else if (ToggleMapScoreboard != ToggleMapScoreboard)
	{
		BackupMapScoreboard = ToggleMapScoreboard;
		toggle = 1;
	}
	else if (ToggleMapScoreboard == BackupMapScoreboard)
	{
		toggle = 0;
	}
	((void (*)(int, int))0x00548170)(a0, toggle);
}
void mapAndScoreboard_check_negative(int a0, int a1, int a2)
{
	if (ToggleMapScoreboard == -1)
		return;
	
	((void (*)(int, int, int))0x004af140)(a0, a1, a2);
}
int mapAndScoreboard_hook(int a0, int a1)
{
	int SecondPass = a1 == 0xA;
	// a1 equals 0xA second time through.
	if (SecondPass)
	{
		// Patch DM Func: Scoreboard or Map Check:
		*(u32*)0x004ac468 = 0x10000007;
		// Patch Seige/CTF Func: Scoreboard or Map Check:
		*(u32*)0x004af198 = 0x10000007;

		return ToggleMapScoreboard;
	}

	Player * player = (Player*)PLAYER_STRUCT;
	// Show Map
	if ((playerPadGetButtonUp(player, PAD_R3) > 0))
	{
		BtnActive = 1;
		ToggleMapScoreboard = 1;
		return 1;
	}
	// Show Scoreboard
	else if ((playerPadGetButtonUp(player, PAD_SELECT) > 0))
	{
		BtnActive = 1;
		ToggleMapScoreboard = 0;
		return 1;
	}
	else if (!SecondPass)
	{
		ToggleMapScoreboard = -1;
		return 2;
	}
}

void mapAndScoreboardBtns(void)
{
	// hook the show/hide function of the map.
	HOOK_JAL(0x005457f4, &toggleMapScoreboard);
	// hook into loading of the select button if pressed
	HOOK_JAL(0x00545804, &mapAndScoreboard_hook);
	// hook into Siege/CTF Map/Gamemode Check
	// HOOK_JAL(0x0053FBA4, &mapAndScoreboard_hook);
	// hook into DM Map/Gamemode Check
	HOOK_JAL(0x00548184, &mapAndScoreboard_hook);
	// hook into DM scoreboard
	HOOK_JAL(0x00548194, &mapAndScoreboard_check_negative);
}



// ===============================================================



int main()
{
	uyaPreUpdate();

	GameSettings * gameSettings = gameGetSettings();
	GameOptions * gameOptions = gameGetOptions();
	if (gameOptions || gameSettings || gameSettings->GameLoadStartTime > 0)
	{

	}

    if (isInGame())
    {
		// Force Normal Up/Down Controls
		*(u32*)0x001A5A70 = 0;

		// Set 1k kills
		// *(u32*)0x004A8F6C = 0x240703E8;
		// *(u32*)0x00539258 = 0x240203E8;
		// *(u32*)0x005392D8 = 0x240203E8;

		mapAndScoreboardBtns();
		InfiniteChargeboot();
		InfiniteHealthMoonjump();
        Debug();
    }
	
	uyaPostUpdate();

    return 0;
}
