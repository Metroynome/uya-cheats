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

typedef struct TimerVars {
    float timer_x;
    float timer_y;
    float timerScale;
    float title_x;
    float title_y;
    float titleScale;
    char title[32];
    u32 colorBase;
    u32 colorHigh;
    int font;
    int timeValue;
    int timeStartTicking;
    SoundDef *tickSound;
    int startTime;
    int lastPlayedTickSound;
    short bDynamicScaleTime;
    short status;
} TimerVars_t;

SoundDef TimerTickSoundDef = {1000, 1000, 2000, 2000, 0, 0, 0, 0x10, 133, 0};

TimerVars_t allNodesTimer = {
    .timer_x = SCREEN_WIDTH / 2,
    .timer_y = SCREEN_HEIGHT * 0.2,
    .timerScale = 3,
    .title = "%s Team Wins In",
    .title_x = SCREEN_WIDTH / 2,
    .title_y = SCREEN_HEIGHT * 0.13,
    .titleScale = 1,
    .colorBase = 0x80ff0000,
    .colorHigh = 0x80ffffff,
    .font = FONT_DEMI,
    .timeValue = 10,
    .timeStartTicking = 5,
    .tickSound = &TimerTickSoundDef,
    .startTime = -1,
    .lastPlayedTickSound = -1,
    .bDynamicScaleTime = 1,
    .status = -1
};

TimerVars_t selectNodesTimer = {
    .timer_x = SCREEN_WIDTH / 2,
    .timer_y = SCREEN_HEIGHT * 0.7,
    .timerScale = 1,
    .title = 0,
    .title_x = SCREEN_WIDTH / 2,
    .title_y = SCREEN_HEIGHT * 0.63,
    .titleScale = 1,
    .colorBase = 0x80ffffff,
    .colorHigh = 0,
    .font = FONT_DEMI,
    .timeValue = 30,
    .timeStartTicking = -1,
    .tickSound = 0,
    .startTime = -1,
    .lastPlayedTickSound = -1,
    .bDynamicScaleTime = 0,
    .status = -1
};

void runCustomTimer(TimerVars_t *timer)
{
    int gameTime = gameGetTime();
    char timerBuf[16];

    if (timer->status == -1 || timer->status == 2)
        return;

    if (timer->status == 0) {
        timer->startTime = gameTime + (timer->timeValue * TIME_SECOND);
        timer->lastPlayedTickSound = gameTime + (timer->timeStartTicking * TIME_SECOND);
    }

    int timeLeft = (timer->startTime) - (gameTime);
    float timeSecondsLeft = timeLeft / (float)TIME_SECOND;
    float timerScale = timer->timerScale;
    u32 timerColor = timer->colorBase;
    // u32 allNodesTeamColor = 0x80000000 | TEAM_COLORS[playerGetFromSlot(0)->mpTeam];
    int timeSecondsLeftFloor = (int)timeSecondsLeft;
    float timeSecondsRounded = timeSecondsLeftFloor;
    if ((timeSecondsLeft - timeSecondsRounded) > 0.5)
        timeSecondsRounded += 1;

    if (timeLeft <= 0) {
        timer->status = 2;
    } else {
        // draw subtext
        if (strlen(timer->title) > 0)
            gfxScreenSpaceText(timer->title_x, timer->title_y, timer->titleScale, timer->titleScale, timer->colorBase, timer->title, -1, 4, timer->font);

        // draw timer
        if (timer->bDynamicScaleTime) {
            float t = 1-fabsf(timeSecondsRounded - timeSecondsLeft);
            float x = powf(t, 15);
            timerScale *= (1.0 + (0.3 * x));
            timerColor = colorLerp(timer->colorBase, timer->colorHigh, x);
        }
        sprintf(timerBuf, "%.02f", timeLeft / (float)TIME_SECOND);
        gfxScreenSpaceText(timer->timer_x, timer->timer_y, timerScale, timerScale, timerColor, timerBuf, -1, 4, timer->font);

        // tick timer
        if (timer->tickSound != 0 && timeSecondsLeftFloor < timer->lastPlayedTickSound && timeSecondsLeftFloor < timer->timeStartTicking) {
            timer->lastPlayedTickSound = timeSecondsLeftFloor;
            soundPlay(timer->tickSound, 0, playerGetFromSlot(0)->pMoby, 0, 1024);
        }

        // set timer status
        timer->status = 1;
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

    if (allNodesTimer.status == -1)
        allNodesTimer.status = 0;

    if (selectNodesTimer.status == -1)
        selectNodesTimer.status = 0;
    
    runCustomTimer(&allNodesTimer);
    runCustomTimer(&selectNodesTimer);

}