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

	((void (*)(Moby*))0x004184E0)(moby);
}

void resetRoundState(void)
{
    GameSettings * gameSettings = gameGetSettings();
	Player ** players = playerGetAll();
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

    vector_copy(pos, StartPos);
	memset(rot, 0, sizeof(rot));
	// pos[3] = SourceBox->Position[3];

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

                        // hbMoby->AnimSeq = SourceBox->AnimSeq;
						// hbMoby->PClass = SourceBox->PClass;
						// hbMoby->CollData = SourceBox->CollData;
						// hbMoby->MClass = SourceBox->MClass;

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
        Active = 1;
        Player * player = (Player*)PLAYER_STRUCT;
		Moby * m = mobySpawn(MOBY_ID_CRATE_RANDOM_PICKUP, 0);
		m->Position[0] = player->PlayerPositionX;
		m->Position[1] = player->PlayerPositionZ;
		m->Position[2] = player->PlayerPositionY;
		m->UpdateDist = 0xFF;
		m->Drawn = 0x01;
		m->DrawDist = 0x0080;
		m->Opacity = 0x80;
		m->State = 1;
		m->Scale = 0.0418;
		m->PUpdate = &boxUpdate;
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
