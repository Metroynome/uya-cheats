#include <tamtypes.h>
#include <libuya/stdio.h>
#include <libuya/game.h>
#include <libuya/player.h>
#include <libuya/pad.h>
#include <libuya/music.h>

int enableSingleplayerMusic = 1;

void runCampaignMusic(void)
{
	static int CustomSector = 0x1d8a;
	static char FinishedConvertingTracks = 0;
	static char SetupMusic = 0;
	static char AddedTracks = 0;
	static char TotalTracks = 0;
	// We go by each wad because we have to have Multiplayer one first.
	static short wadArray[][2] = {
		// wad, song per wad
		// Commented tracks/sectors are due to them being dialoge or messed up.
		{0x54d, 14}, // Multiplayer
		{0x3f9, 2}, // Veldin
		{0x403, 4}, // Florana
		{0x40d, 3}, // Starship Phoenix
		{0x417, 3}, // Marcadia
		{0x421, 4}, // Daxx (This doesn't have padding between music and dialog)
		{0x42b, 1}, // 
		{0x435, 4}, // Annihilation Nation
		{0x43f, 1}, // Aquatos
		{0x449, 1}, // Tyhrranosis
		{0x453, 1}, // Zeldrin Starport
		{0x45d, 1}, // Obani Gemini
		{0x467, 1}, // Blackwater City
		{0x471, 2}, // Holostar Studios
		{0x47b, 1}, // Koros
		{0x485, 1}, // Metropolis
		{0x48f, 1}, // Crash Site
		{0x499, 1}, // Aridia
		{0x4a3, 1}, // Quark's Hideout
		{0x4ad, 1}, // Mylon - Bioliterator
		{0x4b7, 2}, // Obani Draco
		{0x4c1, 1}, // Mylon - Command Center
		{0x4cb, 1}, // 
		{0x4d5, 1}, // Insomniac Museum
		// {0x4df, 0}, // 
		{0x4e9, 1}, // 
		{0x4f3, 1}, // 
		{0x4fd, 1}, // 
		{0x507, 1}, //
		{0x511, 1}, // 
		{0x51b, 1}, // 
		{0x525, 1}, // 
		{0x52f, 1}, // 
		{0x539, 1}, // 
		{0x543, 1}  // 
	};
	// if music isn't loaded or enable singleplayermusic isn't on, or in menus, return.
	u32 CodeSegmentPointer = *(u32*)0x01FFFD00;
	if (!musicGetSector() || CodeSegmentPointer == 0x00574F88 || !enableSingleplayerMusic)
		return;
	
	u32 NewTracksLocation = 0x001F8588; // Overwrites current tracks too.
	if (!FinishedConvertingTracks && musicGetSector() != CustomSector) {
		// Set custom Sector
		musicSetSector(CustomSector);

		AddedTracks = 0;
		int MultiplayerSector = *(u32*)0x001F8584;
		int Stack = 0x000269300;
		// int WAD_Table = 0x001f7f88; // Kept for historical purposes.
		int a;
		// Zero out stack by the appropriate heap size (0x2a0 in this case)
		// This makes sure we get the correct values we need later on.
		memset((u32*)Stack, 0, 0x1818);

		// Loop through each WAD ID
		for(a = 0; a < (sizeof(wadArray)/sizeof(wadArray[0])); a++)
		{
			int WAD = wadArray[a][0];
			// Check if Map Sector is not zero
			if (WAD != 0) {
				internal_wadGetSectors(WAD, 1, Stack);
				int WAD_Sector = *(u32*)(Stack + 0x4);

				// make sure WAD_Sector isn't zero
				if (WAD_Sector != 0) {
					DPRINTF("WAD: 0x%X\n", WAD);
					DPRINTF("WAD Sector: 0x%X\n", WAD_Sector);

					// do music stuffs~
					// Get SP 2 MP Offset for current WAD_Sector.
					// In UYA we use our own sector.
					int SP2MP = WAD_Sector - CustomSector;
					// Remember we skip the first track because it is the start of the sp track, not the body of it.
					int b = 0;
					int Songs = Stack + (a == 0 ? 0x8 : 0x18);
					int j;
					for (j = 0; j < wadArray[a][1]; ++j) {
						int Track_LeftAudio = *(u32*)(Songs + b);
						int Track_RightAudio = *(u32*)((u32)(Songs + b) + 0x8);
						int ConvertedTrack_LeftAudio = SP2MP + Track_LeftAudio;
						int ConvertedTrack_RightAudio = SP2MP + Track_RightAudio;
						*(u32*)(NewTracksLocation) = (u32)ConvertedTrack_LeftAudio;
						*(u32*)(NewTracksLocation + 0x8) = (u32)ConvertedTrack_RightAudio;
						NewTracksLocation += 0x10;
						b += (a == 0) ? 0x10 : 0x20;
						AddedTracks++;
					}
				}
			}
		// Zero out stack to finish the job.
		memset((u32*)Stack, 0, 0x1818);
		}
		DPRINTF("\nTracks: %d", AddedTracks);
		FinishedConvertingTracks = 1;
	}
	
	int DefaultMultiplayerTracks = 0x0d; // This number will never change
	// Due to us overwriting original gracks, no need to do extra math like in DL.
	TotalTracks = AddedTracks;
	if (*(u32*)musicTrackRangeMax() != TotalTracks) {
		int MusicFunctionData = CodeSegmentPointer + 0x1A8;
		*(u16*)MusicFunctionData = TotalTracks;
	}

	// If in game
	if (isInGame())
	{
		static short CurrentTrack = 0;
		static short NextTrack = 0;
		music_Playing* music = musicGetTrackInfo();
		// double check if min/max info are correct
		if (enableSingleplayerMusic) {
			if (*(int*)musicTrackRangeMax() != (TotalTracks - *(int*)musicTrackRangeMin()) || *(int*)musicTrackRangeMin() != 4) {
				*(int*)musicTrackRangeMin() = 4;
				*(int*)musicTrackRangeMax() = TotalTracks - *(int*)musicTrackRangeMin();
			}
		} else {
			if (*(int*)musicTrackRangeMax() != (DefaultMultiplayerTracks - *(int*)musicTrackRangeMin()) || *(int*)musicTrackRangeMin() != 4) {
				*(int*)musicTrackRangeMin() = 4;
				*(int*)musicTrackRangeMax() = DefaultMultiplayerTracks - *(int*)musicTrackRangeMin();
			}
		}
		// Fixes bug where music doesn't always want to start playing at start of game
		// might not be needed anymore due to forcing Min/Max Track info above
		if (music->track == -1 && music->status == 0) {
			// plays a random track
			int randomTrack = randRangeInt(*(int*)musicTrackRangeMin(), *(int*)musicTrackRangeMax()) % *(int*)musicTrackRangeMax();
			DPRINTF("\nrandomTrack: %d", randomTrack);
			musicPlayTrack(randomTrack, FLAG_KEEP_PLAYING_AFTER, 1024);
		}
		// If Status is 8 and both Current Track and Next Track equal zero
		if (music->unpause == UNPAUSE_LOADING && CurrentTrack == 0 && NextTrack == 0 && music->track != -1) {
			// Set CurrentTrack to Track.  This will make it so we know which was is currently playing.
			// The game automatically sets the track variable to the next track to play after the music starts.
			CurrentTrack = music->track;
		} else if ((music->unpause == UNPAUSE_KEEP_PLAYING_AFTER || music->unpause == UNPAUSE_STOP_PLAYING_AFTER) && NextTrack == 0) {
			// Set NextTrack to Track value.
			NextTrack = music->track;
		}
		// If NextTrack does not equal the Track, that means that the song has switched.
		// We need to move the NextTrack value into the CurrentTrack value, because it is now
		// playing that track.  Then we set the NextTrack to the Track value.
		else if (NextTrack != music->track)
		{
			CurrentTrack = NextTrack;
			NextTrack = music->track;
		}
		// If CurrentTrack is ger than the default Multiplayer tracks
		// and if CurrentTrack does not equal -1
		// and if the track duration is below 0x3000
		// and if Status2 is 2, or Current Playing
		if ((CurrentTrack > DefaultMultiplayerTracks * 2) && CurrentTrack != -1 && (music->remain <= 0x3000) && music->queuelen == QUEUELEN_PLAYING)
		{
			// This technically cues track 1 (the shortest track) with no sound to play.
			// Doing this lets the current playing track to fade out.
			musicTransitionTrack(0,0,0,0);
		}
	} else if (isInMenus() && FinishedConvertingTracks) {
		FinishedConvertingTracks = 0;
	}
}

int main(void)
{
	// run normal hook
	((void (*)(void))0x00126780)();

	runCampaignMusic();

	return 0;
}
