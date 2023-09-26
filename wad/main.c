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
#include <libuya/camera.h>
#include <libuya/gameplay.h>

int Active = 0;
int FinishedConvertingTracks = 0;
int AddedTracks = 0;

int main(void)
{
	// check to see if multiplayer tracks are loaded
	if (!musicIsLoaded())
	{
		Active = 0;
		return -1;
	}

	Active = 1;

	int NewTracksLocation = 0x001CF940;
	if (!FinishedConvertingTracks)
	{
		AddedTracks = 0;
		int MultiplayerSectorID = *(u32*)0x001CF85C;
		int Stack = 0x000269200;
		int Sector = 0x001f7f90;
		int end_address = 0x001f84b8;
		int Offset = 0;
		int a = 0;

		do
		{
			memset((u32*)Stack, 0, 0x26F0);
			Offset += 0x8;
			int MapSector = *(u32*)(Sector + Offset);
			if (MapSector != 0)
			{
				internal_wadGetSectors(MapSector, 1, Stack);
				int SectorSize = *(u32*)(Stack);
				int SectorID = *(u32*)(Stack + 0x4);
				// levelaudiowad
				if (SectorSize == 0x1818)
				{
					printf("\nAudioWAD (0x%x): 0x00%x : 0x%x", MapSector, (u32)(Sector + Offset), SectorID);
				}
				// levelscenewad
				else if (SectorSize == 0x26f0)
				{
					printf("\nSceneWAD (0x%x): 0x00%x : 0x%x", MapSector, (u32)(Sector + Offset), SectorID);
				}
				// levelwad
				else if (SectorSize == 0x60)
				{
					printf("\nLevelWAD (0x%x): 0x00%x : 0x%x", MapSector, (u32)(Sector + Offset), SectorID);
				}
				else
				{
					printf("\nUnidentified WAD (0x%x):", MapSector);
					printf("\nAddress: 0x%x", (u32)(Sector + Offset));
					printf("\nSize: 0x%x", *(u32*)(Sector + Offset));
				}
				a++;
			}
		} while ((u32)(Sector + Offset) <= end_address);
		memset((u32*)Stack, 0, 0x26F0);

		FinishedConvertingTracks = 1;
	};

	return 0;
}
