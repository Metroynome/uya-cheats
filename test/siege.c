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
#include <libuya/gameplay.h>
#include <libuya/map.h>
#include <libuya/sound.h>

#define TIMER_DEFAULT_TIME (30)
#define TIMER_TEXT_SCALE (3)
#define TIMER_BASE_COLOR1 (0x80aabbcc)
#define TIMER_BASE_COLOR2 (0x80808080)
#define TIMER_HIGH_COLOR (0x80ffffff)

// soundID ticking ideas:
// 11, 103, 133,

// last time second timer tick sound was played.
int lastPlaySoundSecond = 0;
int startTime = 0;
int soundId = 0;
SoundDef TimerTickSoundDef;

// TimerTickSoundDef = {
//     1000.0,
//     1000.0,
//     2000,
//     2000,
//     0,
//     0,
//     0,
//     0x10,
//     soundId, // 0xF4,
//     3
// };

void runCustomTimer(void)
{

    TimerTickSoundDef.minRange = 1000.0;
    TimerTickSoundDef.maxRange = 1000.0;
    TimerTickSoundDef.minVolume = 2000;
    TimerTickSoundDef.maxVolume = 2000;
    TimerTickSoundDef.minPitch = 0;
    TimerTickSoundDef.maxPitch = 0;
    TimerTickSoundDef.loop = 0;
    TimerTickSoundDef.flags = 0x10;
    TimerTickSoundDef.index = soundId,
    TimerTickSoundDef.bank_index = 0;

    int gameTime = gameGetTime();
    char timerBuf[16];
    char teamBuff[4];

    if (1) {

        if (startTime <= gameTime) {
            lastPlaySoundSecond = gameTime + (TIMER_DEFAULT_TIME * TIME_SECOND);
            startTime = gameTime + (TIMER_DEFAULT_TIME * TIME_SECOND);
        }

        int timeLeft = (startTime) - (gameTime); // - SNDState.BombPlantedTicks);
        float timeSecondsLeft = timeLeft / (float)TIME_SECOND;
        float scale = TIMER_TEXT_SCALE;
        u32 color = 0x80ffffff;
        u32 allNodesTeamColor = 0x80000000 | TEAM_COLORS[playerGetFromSlot(0)->mpTeam];
        int timeSecondsLeftFloor = (int)timeSecondsLeft;
        float timeSecondsRounded = timeSecondsLeftFloor;
        if ((timeSecondsLeft - timeSecondsRounded) > 0.5)
            timeSecondsRounded += 1;

        if (timeLeft <= 0) {
            // set end
            // setRoundOutcome(SND_OUTCOME_BOMB_DETONATED);
        } else {
            // update scale
            float t = 1-fabsf(timeSecondsRounded - timeSecondsLeft);
            float x = powf(t, 15);
            float dynamicScale = scale * (1.0 + (0.3 * x));

            // update color
            color = colorLerp(allNodesTeamColor, TIMER_HIGH_COLOR, x);

            // draw subtext
            // sprintf(teamBuf, "%s", winningTeam);
            gfxScreenSpaceText(SCREEN_WIDTH/2, SCREEN_HEIGHT * 0.13, scale / 3, scale / 3, allNodesTeamColor, "Blue Team Wins in", -1, 4, FONT_DEMI);

            // draw timer
            sprintf(timerBuf, "%.02f", timeLeft / (float)TIME_SECOND);
            gfxScreenSpaceText(SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.2, dynamicScale, dynamicScale, color, timerBuf, -1, 4, FONT_DEMI);

            // tick timer
            if (timeSecondsLeftFloor < lastPlaySoundSecond) {
                lastPlaySoundSecond = timeSecondsLeftFloor;
                // soundMobyPlay(soundId, 0, playerGetFromSlot(0)->pMoby);
                soundPlay(&TimerTickSoundDef, 0, playerGetFromSlot(0)->pMoby, 0, 1024);
            }
        }
    }
}

void runSiege()
{
    Player *player = playerGetFromSlot(0);
	if (player->pauseOn == 0 && playerPadGetButtonDown(player, PAD_CIRCLE) > 0) {
		printf("\ngd: %08x", gameGetData()->CTFGameData);
	}

	GameSettings *gs = gameGetSettings();
	if (gs->GameType != GAMETYPE_SIEGE)
		return;

    if (playerPadGetButtonDown(playerGetFromSlot(0), PAD_LEFT) > 0) {
        soundId -= 1;
        printf("\n-------------");
        printf("\nsoundId: %d", soundId);
        printf("\n-------------");
    }
    if (playerPadGetButtonDown(playerGetFromSlot(0),PAD_RIGHT) > 0) {
        soundId += 1;
        printf("\n-------------");
        printf("\nsoundId: %d", soundId);
        printf("\n-------------");
    }

    runCustomTimer();
}