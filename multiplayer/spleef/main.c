#include <tamtypes.h>

#include <libuya/stdio.h>
#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/interop.h>
#include <libuya/moby.h>
#include <libuya/time.h>
#include <libuya/math.h>
#include <libuya/math3d.h>
#include <libuya/gamesettings.h>
#include "game.h"

#define SPLEEF_SPAWN_MOBY                   (MOBY_ID_CRATE_RANDOM_PICKUP)
#define SPLEEF_BOARD_DIMENSION              (10)
#define SPLEEF_BOARD_LEVELS					(2)
#define SPLEEF_BOARD_LEVEL_OFFSET           (40.0)
#define SPLEEF_BOARD_BOX_SIZE               (4.0)
#define SPLEEF_BOARD_SPAWN_RADIUS           (SPLEEF_BOARD_BOX_SIZE * ((SPLEEF_BOARD_DIMENSION + SPLEEF_BOARD_DIMENSION) / 5))
#define SPLEEF_BOARD_BOX_MAX                (SPLEEF_BOARD_DIMENSION * SPLEEF_BOARD_DIMENSION * SPLEEF_BOARD_LEVELS)

const char * SPLEEF_ROUND_WIN = "First!";
const char * SPLEEF_ROUND_SECOND = "Second!";
const char * SPLEEF_ROUND_THIRD = "Third!";
const char * SPLEEF_ROUND_LOSS = "Better luck next time!";

int Initialized = 0;

SpleefState_t SpleefState;
Moby * SpleefBox[SPLEEF_BOARD_DIMENSION * SPLEEF_BOARD_DIMENSION * SPLEEF_BOARD_LEVELS];
// Moby * SourceBox;
// Position that boxes are spawned to.
VECTOR StartPos = {
	400,
	400,
	800,
	0
};

void boxUpdate(Moby * moby)
{
	MobyColDamage * colDamage = mobyGetDamage(moby, 0xfffffff, 0);
	if (moby->State == 2)
		mobyDestroy(moby);

	((void (*)(Moby*))0x0041e3c8)(moby);
}

void resetRoundState(void)
{
    GameSettings * gameSettings = gameGetSettings();
	Player * p = (Player*)PLAYER_STRUCT;
	int gameTime = gameGetTime();
	int i,j,k, count=0;
	VECTOR pos, rot = {0,0,0,0}, center;
	Moby * hbMoby = 0;

    SpleefState.RoundInitialized = 0;
	SpleefState.RoundStartTicks = gameTime;
	SpleefState.RoundEndTicks = 0;
	SpleefState.RoundResult[0] = 0;
	SpleefState.RoundResult[1] = -1;
	SpleefState.RoundResult[2] = -1;
	SpleefState.RoundResult[3] = -1;

    // Center
	center[0] = StartPos[0] + (SPLEEF_BOARD_BOX_SIZE * (SPLEEF_BOARD_DIMENSION / (float)2.0));
	center[1] = StartPos[1] + (SPLEEF_BOARD_BOX_SIZE * (SPLEEF_BOARD_DIMENSION / (float)2.0));
	center[2] = StartPos[2];

	pos[0] = center[0];
	pos[1] = center[1];
	pos[2] = center[2] + (float)30;

	// playerRespawn(PLAYER_STRUCT);
	playerSetPosRot(p, &pos, &p->PlayerRotation);

    vector_copy(pos, StartPos);
	memset(rot, 0, sizeof(rot));

    // Spawn boxes
	for (k = 0; k < SPLEEF_BOARD_LEVELS; ++k)
	{
		for (i = 0; i < SPLEEF_BOARD_DIMENSION; ++i)
		{
			for (j = 0; j < SPLEEF_BOARD_DIMENSION; ++j)
			{
				// delete old one
				int boxId = (k * SPLEEF_BOARD_DIMENSION * SPLEEF_BOARD_DIMENSION) + (i * SPLEEF_BOARD_DIMENSION) + j;
				if (!SpleefBox[boxId] || SpleefBox[boxId]->OClass != SPLEEF_SPAWN_MOBY/* || mobyIsDestroyed(SpleefBox[boxId])*/)
				{
					// spawn
					SpleefBox[boxId] = hbMoby = mobySpawn(SPLEEF_SPAWN_MOBY, 0);

					if (hbMoby)
					{
						vector_copy(hbMoby->Position, pos);

						hbMoby->UpdateDist = 0xFF;
						hbMoby->Drawn = 0x01;
						hbMoby->DrawDist = 0x0080;
						hbMoby->Opacity = 0x80;
						hbMoby->State = 1;
						hbMoby->Scale = (float)0.0418 * SPLEEF_BOARD_BOX_SIZE;
						// hbMoby->Lights = 0x202;
						// hbMoby->GuberMoby = 0;
						hbMoby->PUpdate = &boxUpdate;
						++count;
					}
				}

				pos[1] += SPLEEF_BOARD_BOX_SIZE;
			}

			pos[0] += SPLEEF_BOARD_BOX_SIZE;
			pos[1] = StartPos[1];
		}

		pos[0] = StartPos[0];
		pos[1] = StartPos[1];
		pos[2] -= SPLEEF_BOARD_LEVEL_OFFSET;
	}

    SpleefState.RoundInitialized = 1;
}

void Debug(void)
{
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
    static int Active = 0;
    // DEBUG OPTIONS: L3 = Testing, R3 = Hurt Player
    if ((pad->btns & PAD_L3) == 0 && Active == 0)
	{
        Active = 1;

		Player * player = (Player*)PLAYER_STRUCT;
        player->PlayerPositionX = (float)400;
        player->PlayerPositionZ = (float)400;
        player->PlayerPositionY = (float)804;
	}
    else if ((pad->btns & PAD_R3) == 0 && Active == 0)
    {
		// VECTOR rotation = {703,245,200};
        // Active = 1;
		// Player * player = (Player*)PLAYER_STRUCT;
		// playerRespawn(PLAYER_STRUCT);
		// playerSetPosRot(PLAYER_STRUCT, &player->PlayerPosition, &rotation);
    }
    if (!(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0)
    {
        Active = 0;
    }
}

void initialize(void)
{
    // SourceBox = mobySpawn(SPLEEF_SPAWN_MOBY, 0);
    resetRoundState();
    Initialized = 1;
}

void gameStart(struct GameModule * module, PatchConfig_t * config, PatchGameConfig_t * gameConfig, PatchStateContainer_t * gameState)
{
	GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
	Player * localPlayer = (Player*)PLAYER_STRUCT;
	GameData * gameData = gameGetData();
	int i;

	// Ensure in game
	if (!gameSettings || !isInGame())
		return;

	// Determine if host
	SpleefState.IsHost = gameIsHost(localPlayer->Guber.Id.GID.HostId);

	if (!Initialized)
		initialize(gameConfig, gameState);

	int killsToWin = gameGetOptions()->GameFlags.MultiplayerGameFlags.KillsToWin;

	// 
	updateGameState(gameState);

#if DEBUG
	if (padGetButton(0, PAD_L3 | PAD_R3) > 0)
		SpleefState.GameOver = 1;
#endif

	if (!gameHasEnded() && !SpleefState.GameOver)
	{
		if (SpleefState.RoundResult[0])
		{
			if (SpleefState.RoundEndTicks)
			{
				// draw round message
				if (SpleefState.RoundResult[1] == localPlayer->PlayerId)
				{
					drawRoundMessage(SPLEEF_ROUND_WIN, 1.5);
				}
				else if (SpleefState.RoundResult[2] == localPlayer->PlayerId)
				{
					drawRoundMessage(SPLEEF_ROUND_SECOND, 1.5);
				}
				else if (SpleefState.RoundResult[3] == localPlayer->PlayerId)
				{
					drawRoundMessage(SPLEEF_ROUND_THIRD, 1.5);
				}
				else
				{
					drawRoundMessage(SPLEEF_ROUND_LOSS, 1.5);
				}

				// handle when round properly ends
				if (gameGetTime() > SpleefState.RoundEndTicks)
				{
					// increment round
					++SpleefState.RoundNumber;

					// reset round state
					resetRoundState();
				}
			}
			else
			{
				// Handle game outcome
				for (i = 1; i < 4; ++i)
				{
					int playerIndex = SpleefState.RoundResult[i];
					if (playerIndex >= 0) {
						SpleefState.PlayerPoints[playerIndex] += 4 - i;
						DPRINTF("player %d score %d\n", playerIndex, SpleefState.PlayerPoints[playerIndex]);
					}
				}

				// set when next round starts
				SpleefState.RoundEndTicks = gameGetTime() + (TIME_SECOND * 5);

				// update winner
				int winningScore = 0;
				getWinningPlayer(&SpleefState.WinningTeam, &winningScore);
				if (killsToWin > 0 && winningScore >= killsToWin)
				{
					SpleefState.GameOver = 1;
				}
			}
		}
		else
		{
			// iterate each player
			for (i = 0; i < GAME_MAX_PLAYERS; ++i)
			{
				SpleefState.PlayerKills[i] = gameData->PlayerStats.Kills[i];
			}

			// host specific logic
			if (SpleefState.IsHost && (gameGetTime() - SpleefState.RoundStartTicks) > (5 * TIME_SECOND))
			{
				int playersAlive = 0, playerCount = 0, lastPlayerAlive = -1;
				for (i = 0; i < GAME_MAX_PLAYERS; ++i)
				{
					if (SpleefState.RoundPlayerState[i] >= 0)
						++playerCount;
					if (SpleefState.RoundPlayerState[i] == 0)
						++playersAlive;
				}

				for (i = 0; i < GAME_MAX_PLAYERS; ++i)
				{
					Player * p = players[i];

					if (p)
					{
						// check if player is dead
						if (playerIsDead(p) || SpleefState.RoundPlayerState[i] == 1)
						{
							// player newly died
							if (SpleefState.RoundPlayerState[i] == 0)
							{
								DPRINTF("player %d died\n", i);
								SpleefState.RoundPlayerState[i] = 1;

								// set player to first/second/third if appropriate
								if (playersAlive < 4)
								{
									SpleefState.RoundResult[playersAlive] = i;
									DPRINTF("setting %d place to player %d\n", playersAlive, i);
								}
							}
						}
						else
						{
							lastPlayerAlive = i;
						}
					}
				}

				if ((playersAlive == 1 && playerCount > 1) || playersAlive == 0)
				{
					// end
					DPRINTF("end round: playersAlive:%d playerCount:%d\n", playersAlive, playerCount);
					if (lastPlayerAlive >= 0)
					{
						SpleefState.RoundResult[1] = lastPlayerAlive;
						DPRINTF("last player alive is %d\n", lastPlayerAlive);
					}
					setRoundOutcome(SpleefState.RoundResult[1], SpleefState.RoundResult[2], SpleefState.RoundResult[3]);
				}
			}
		}
	}
	else
	{
		// set winner
		gameSetWinner(SpleefState.WinningTeam, 0);

		// end game
		if (SpleefState.GameOver == 1)
		{
			gameEnd(4);
			SpleefState.GameOver = 2;
		}
	}

	// 
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
	{
		Player * p = players[i];
		if (!p)
			continue;

		if (!playerIsDead(p))
			playerSetHealth(p, PLAYER_MAX_HEALTH);
		else
			playerSetHealth(p, 0);
	}

	return;
}

int main(void)
{
    if (isInGame())
    {
        if (!Initialized)
            initialize();

    	Debug();
    }
    return 0;
}
