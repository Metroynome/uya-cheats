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
    .timer_y = SCREEN_HEIGHT * 0.8,
    .timerScale = 3,
    .title = "",
    .title_x = SCREEN_WIDTH / 2,
    .title_y = SCREEN_HEIGHT * 0.73,
    .titleScale = 1,
    .colorBase = 0x80ff0000,
    .colorHigh = 0x80ffffff,
    .font = FONT_DEFAULT,
    .timeValue = 120,
    .timeStartTicking = 10,
    .tickSound = &TimerTickSoundDef,
    .startTime = -1,
    .lastPlayedTickSound = -1,
    .bDynamicScaleTime = 1,
    .status = -1
};

TimerVars_t selectNodesTimer = {
    .timer_x = SCREEN_WIDTH / 2,
    .timer_y = SCREEN_HEIGHT * 0.17,
    .timerScale = 1,
    .title = 0,
    .title_x = SCREEN_WIDTH / 2,
    .title_y = SCREEN_HEIGHT * 0.1,
    .titleScale = 1,
    .colorBase = 0x8060bfee,
    .colorHigh = 0,
    .font = FONT_DEFAULT,
    .timeValue = 10,
    .timeStartTicking = -1,
    .tickSound = 0,
    .startTime = -1,
    .lastPlayedTickSound = -1,
    .bDynamicScaleTime = 0,
    .status = -1
};

// short teamNodes[2] = {-1, -1};
int maxNodeCount = -1;
int gameOver = -1;
int selectedNode = 1;

void runTimer(TimerVars_t *timer)
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
        // sprintf(timerBuf, "%.02f", timeLeft / (float)TIME_SECOND);
        int formatTime = timeLeft * (60.0 / TIME_SECOND);
        if (timer->timeValue >= 60) {
            sprintf(timerBuf, "%02i:%02i:%02i", (formatTime / 60) / 60, (formatTime / 60) % 60, ((formatTime % 60) * 100) / 60);
        } else {
            sprintf(timerBuf, "%02i.%02i", (formatTime / 60) % 60, ((formatTime % 60) * 100) / 60);
        }
        
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

void runCheckNodes(void) {
    int i;
    GameOptions *gameOptions = gameGetOptions();
    GameSettings *gameSettings = gameGetSettings();
    GameData *gameData = gameGetData();
    short nodes[2] = {0, 0};
    char title[32];
    // get number of nodes
    if (maxNodeCount == -1) {
        maxNodeCount = 0;
        Moby* mobyStart = mobyListGetStart();
        Moby* mobyEnd = mobyListGetEnd();
        while (mobyStart < mobyEnd) {
            if (mobyStart->oClass == MOBY_ID_SIEGE_NODE) {
                ++maxNodeCount;
            }
            ++mobyStart;
        }
    }
    // check which team each node is, and save it into nodes
    for (i = 0; i < maxNodeCount; ++i) {
        int nodesTeam = gameData->allYourBaseGameData->nodeTeam[i];
        if (nodesTeam == 0 || nodesTeam == 1)
            ++nodes[nodesTeam];
    }
    // check if team has all nodes and set timer title and color
    int blue = nodes[0];
    int red = nodes[1];
    int blueTeamAll = blue == maxNodeCount && red == 0;
    int redTeamAll = red == maxNodeCount && blue == 0;
    int teamWithAllNodes = (blueTeamAll && !redTeamAll) ? 0 : (redTeamAll && !blueTeamAll) ? 1 : -1;
    if (teamWithAllNodes > -1 && allNodesTimer.status == -1) {
        char *whichTeam = !teamWithAllNodes ? "Blue" : "Red";
        sprintf(title, "%s Team Wins In", whichTeam);
        strncpy(allNodesTimer.title, title, 32);
        allNodesTimer.colorBase = 0x80000000 | TEAM_COLORS[teamWithAllNodes];
        allNodesTimer.status = 0;
    } else if (teamWithAllNodes == -1 && allNodesTimer.status > -1){
        strncpy(allNodesTimer.title, "", 32);
        allNodesTimer.status = -1;
    }
    // check status of timer.  if finished, end game.
    if (allNodesTimer.status == 2) {
        if (gameOver == 1)
            return;

        gameData->winningTeam = teamWithAllNodes;
        gameEnd(2);
        gameOver = 1;
    } else {
        runTimer(&allNodesTimer);
    }

    #ifdef DEBUG
    // left/right: select node; up/down: set node to blue/red team
    Player *player = playerGetFromSlot(0);
    if (playerPadGetButtonDown(player, PAD_LEFT) > 0) {
        if (selectedNode > 1)
            --selectedNode;
        else
            selectedNode = maxNodeCount;
    }
    if (playerPadGetButtonDown(player, PAD_RIGHT) > 0) {
        if (selectedNode < maxNodeCount)
            ++selectedNode;
        else
            selectedNode = 1;
    }
    if (playerPadGetButtonDown(player, PAD_UP) > 0) {
        gameData->allYourBaseGameData->nodeTeam[selectedNode - 1] = 0;
    }
    if (playerPadGetButtonDown(player, PAD_DOWN) > 0) {
        gameData->allYourBaseGameData->nodeTeam[selectedNode - 1] = 1;
    }
    // printf("\nm: %d, n: %d, t: %d, b: %d, r: %d, a: %d", maxNodeCount, selectedNode, gameData->allYourBaseGameData->nodeTeam[selectedNode - 1], nodes[0], nodes[1], teamWithAllNodes);
    #endif
}

void runSelectNodeTimer(void)
{
    Player *player = playerGetFromSlot(0);
    int isDead = playerIsDead(player);
    int status = selectNodesTimer.status;
    int resTimer = playerGetRespawnTimer(player);
    int state = playerGetState(player);
    if (state == PLAYER_STATE_WAIT_FOR_RESURRECT && status == -1) {
        selectNodesTimer.timeValue = 10;
        selectNodesTimer.status = 0;
    }
    printf("\n%02i.%02i", (resTimer / 60) % 60, ((resTimer % 60) * 100) / 60);
    if (status == 2 && resTimer <= 0) {
        playerRespawn(player);
        if (!isDead)
            selectNodesTimer.status = -1;
    } else {
        runTimer(&selectNodesTimer);
    }
}

void runSiege(void)
{
    Player *player = playerGetFromSlot(0);
	if (player->pauseOn == 0 && playerPadGetButtonDown(player, PAD_CIRCLE) > 0) {
		printf("\ngd: %08x", gameGetData()->CTFGameData);
	}

	GameSettings *gs = gameGetSettings();
	if (gs->GameType != GAMETYPE_SIEGE)
		return;

    // checks if all nodes are owned by 1 team, if so, run end game timer.
    runCheckNodes();
    runSelectNodeTimer();

    // if (allNodesTimer.status == -1)
    //     allNodesTimer.status = 0;

    // if (selectNodesTimer.status == -1)
    //     selectNodesTimer.status = 0;
    
    // runTimer(&allNodesTimer);
}