#include <tamtypes.h>
#include <libuya/stdio.h>
#include <libuya/game.h>
#include <libuya/player.h>
#include <libuya/pad.h>
#include <libuya/music.h>

int enableSingleplayerMusic = 1;
static char AdderTracks[] = {};

//debug
int PLAYING_TRACK = 0;
int StartSound = -1;


void CheckFirstBits(int Track, int LeftAudio, int RightAudio)
{
	// Get First 2 bytes of the track data
	LeftAudio = (LeftAudio >> 10);
	RightAudio = (RightAudio >> 10);
	if (LeftAudio == RightAudio)
	{
		AdderTracks[LeftAudio] = Track;
	}
	else
	{
		AdderTracks[LeftAudio] = Track;
		AdderTracks[RightAudio] = Track;
	}
}
void CampaignMusic(void)
{
	static int CustomSector = 0x1d84;
	static int FinishedConvertingTracks = 0;
	static int AddedTracks = 0;
	static int SetupMusic = 0;
	static short Music[][2] = {};

	// We go by each wad because we have to have Multiplayer one first.
	static short wadArray[] = {0x54d, 0x3f9, 0x403, 0x40d, 0x417, 0x421, 0x42b, 0x435, 0x43f, 0x449, 0x453, 0x45d, 0x467, 0x471, 0x47b, 0x485, 0x48f, 0x499, 0x4a3, 0x4ad, 0x4b7, 0x4c1, 0x4cb, 0x4d5, 0x4df, 0x4e9, 0x4f3, 0x4fd, 0x507, 0x511, 0x51b, 0x525, 0x52f, 0x539, 0x543};
	// We set how many songs we want from each wad because the bots audio is apart of the wad as well.
	static int wadArray_songPerWAD[] = {14, 2, 3, 3, 3, 3, 0, 4, 3, 4, 0, 1, 4, 2, 0, 2, 0, 2, 3, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	// check to see if multiplayer tracks are loaded
	if (!musicIsLoaded())
		return;

	musicSetSector(CustomSector);
	u32 NewTracksLocation = 0x001F8588; // Overwrites current tracks too.
	if (!FinishedConvertingTracks)
	{
		AddedTracks = 0;
		int MultiplayerSector = *(u32*)0x001F8584;
		int Stack = 0x000269300;
		int WAD_Table = 0x001f7f88;
		int a;
		int Offset = 0;
		int wadArray_size = sizeof(wadArray);

		// Zero out stack by the appropriate heap size (0x2a0 in this case)
		// This makes sure we get the correct values we need later on.
		memset((u32*)Stack, 0, 0x1818);

		// Loop through each WAD ID
		// Total: 35
		for(a = 0; a < 10; a++)
		{
			int WAD = wadArray[a];
			// Check if Map Sector is not zero
			if (WAD != 0)
			{
				internal_wadGetSectors(WAD, 1, Stack);
				int WAD_Sector = *(u32*)(Stack + 0x4);

				// make sure WAD_Sector isn't zero
				if (WAD_Sector != 0)
				{
					DPRINTF("WAD: 0x%X\n", WAD);
					DPRINTF("WAD Sector: 0x%X\n", WAD_Sector);

					// do music stuffs~
					// Get SP 2 MP Offset for current WAD_Sector.
					// In UYA we use our own sector.
					int SP2MP = WAD_Sector - CustomSector;
					// Remember we skip the first track because it is the start of the sp track, not the body of it.
					int b = 0;
					int Songs = Stack + (a == 0 ? 0x8 : 0x18);
					// while current song doesn't equal zero, then convert.
					// if it does equal zero, that means we reached the end of the list and we move onto the next batch of tracks.
					int j;
					for (j = 0; j < wadArray_songPerWAD[a]; ++j) {
						int Track_LeftAudio = *(u32*)(Songs + b);
						int Track_RightAudio = *(u32*)((u32)(Songs + b) + 0x8);
						int ConvertedTrack_LeftAudio = SP2MP + Track_LeftAudio;
						int ConvertedTrack_RightAudio = SP2MP + Track_RightAudio;
						// Checks the first 16 bits of track for 0, 1, 2, ect.
						// CheckFirstBits(AddedTracks, ConvertedTrack_LeftAudio, ConvertedTrack_RightAudio);
						// Store converted tracks for later
						// Music[AddedTracks][0] = (u32)ConvertedTrack_LeftAudio;
						// Music[AddedTracks][1] = (u32)ConvertedTrack_RightAudio;
						*(u32*)(NewTracksLocation) = (u32)ConvertedTrack_LeftAudio;
						*(u32*)(NewTracksLocation + 0x8) = (u32)ConvertedTrack_RightAudio;
						NewTracksLocation += 0x10;
						if (a == 0)
						{
							b += 0x10;
						}
						else
						{
							b += 0x20;
						}
						AddedTracks++;
					}
				}
			}
		// Zero out stack to finish the job.
		memset((u32*)Stack, 0, 0x1818);
		}
		
		
		
		// Doesn't use wad array.
		// for(a = 0; a < 35; a++)
		// {
		// 	Offset += 0x18;
		// 	int WAD = *(u32*)(WAD_Table + Offset);
		// 	// Check if Map Sector is not zero
		// 	if (WAD != 0)
		// 	{
		// 		internal_wadGetSectors(WAD, 1, Stack);
		// 		int WAD_Sector = *(u32*)(Stack + 0x4);

		// 		// make sure WAD_Sector isn't zero
		// 		if (WAD_Sector != 0)
		// 		{
		// 			printf("WAD: 0x%X\n", WAD);
		// 			printf("WAD Sector: 0x%X\n", WAD_Sector);

		// 			// do music stuffs~
		// 			// Get SP 2 MP Offset for current WAD_Sector.
		// 			// In UYA we use our own sector.
		// 			int SP2MP = WAD_Sector - CustomSector;
		// 			// Remember we skip the first track because it is the start of the sp track, not the body of it.
		// 			int b = 0;
		// 			int Songs = Stack + 0x18;
		// 			// while current song doesn't equal zero, then convert.
		// 			// if it does equal zero, that means we reached the end of the list and we move onto the next batch of tracks.
		// 			do
		// 			{
		// 				int Track_LeftAudio = *(u32*)(Songs + b);
		// 				int Track_RightAudio = *(u32*)((u32)(Songs + b) + 0x8);
		// 				int ConvertedTrack_LeftAudio = SP2MP + Track_LeftAudio;
		// 				int ConvertedTrack_RightAudio = SP2MP + Track_RightAudio;
		// 				// Checks the first 16 bits of track for 0, 1, 2, ect.
		// 				// CheckFirstBits(AddedTracks, ConvertedTrack_LeftAudio, ConvertedTrack_RightAudio);
		// 				// Store converted tracks for later
		// 				// Music[AddedTracks][0] = (u32)ConvertedTrack_LeftAudio;
		// 				// Music[AddedTracks][1] = (u32)ConvertedTrack_RightAudio;
		// 				*(u32*)(NewTracksLocation) = (u32)ConvertedTrack_LeftAudio;
		// 				*(u32*)(NewTracksLocation + 0x8) = (u32)ConvertedTrack_RightAudio;
		// 				AddedTracks++;
		// 			}
		// 			while (*(u32*)(Songs + b) != 0);
		// 		}
		// 		else
		// 		{
		// 			Offset -= 0x18;
		// 			a--;
		// 		}
		// 	}
		// 	else
		// 	{
		// 		a--;
		// 	}
		// // Zero out stack to finish the job.
		// memset((u32*)Stack, 0, 0x1818);
		// }

		FinishedConvertingTracks = 1;
		printf("Added Tracks: %d\n", AddedTracks);
	}
	

	int DefaultMultiplayerTracks = 0x0d; // This number will never change
	int StartingTrack = musicTrackRangeMin();
	int AllTracks = DefaultMultiplayerTracks + AddedTracks;
	int TotalTracks = (DefaultMultiplayerTracks - StartingTrack + 1) + AddedTracks;
	int CodeSegmentPointer = *(u32*)0x01FFFD00;
	// If not in main lobby, game lobby, ect.
	if(CodeSegmentPointer != 0x00574F88){
		// if TRACK_RANGE_MAX doesn't equal TotalTracks
		if(musicTrackRangeMax() != TotalTracks){
			int MusicFunctionData = CodeSegmentPointer + 0x1A8;
			*(u16*)MusicFunctionData = AllTracks;
		}
		// *(u32*)NewTracksLocation == 0 || 
		// if (!SetupMusic)
		// {
		// 	int Track;
		// 	for(Track = 0; Track < AddedTracks; ++Track)
		// 	{
		// 		u32 Left = (u32)Music[Track][0];
		// 		u32 Right = (u32)Music[Track][1];
		// 		u32 Add1 = 0x10000;
		// 		u32 Add2 = 0x20000;
		// 		u32 Add3 = 0x30000;
		// 		// if (Track == 8)
		// 		// {
		// 		// 	Right += Add1;
		// 		// }
		// 		// else if (Track >= 9 && Track <= 31)
		// 		// {
		// 		// 	Left += Add1;
		// 		// 	Right += Add1;
		// 		// }
		// 		// else if (Track == 32)
		// 		// {
		// 		// 	Left += Add1;
		// 		// 	Right += Add2;
		// 		// }
		// 		// else if (Track >= 33 && Track <= 56)
		// 		// {
		// 		// 	Left += Add2;
		// 		// 	Right += Add2;
		// 		// }
		// 		// else if (Track >= 57)
		// 		// {
		// 		// 	Left += Add3;
		// 		// 	Right += Add3;
		// 		// }
				
		// 		*(u32*)(NewTracksLocation) = Left;
		// 		*(u32*)(NewTracksLocation + 0x08) = Right;
		// 		NewTracksLocation += 0x10;
		// 	}
		// 	SetupMusic = 1;
		// }
	}

	// If in game
	if (isInGame())
	{
		PrevNextSong();

		static short CurrentTrack = 0;
		static short NextTrack = 0;
		music_Playing * music = musicGetTrackInfo();
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
	}
}

void PrevNextSong()
{
	//Exmaple for choosing track
	PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
	music_Playing * music = musicGetTrackInfo();
	// L3: Previous Sound
	if (StartSound == -1) {
		StartSound = music->track;
		printf("Starting Track: 0x%x", music->track);
	}
	if ((pad->btns & PAD_L3) == 0 && PLAYING_TRACK == 0)
	{
		// Setting PLAYING_TRACK to 1 will make it so the current playing sound will play once.
		PLAYING_TRACK = 1;
		StartSound -= 0x1; // Subtract 1 from StartSound
		musicStopTrack();
		musicPlayTrack(StartSound * 2, 1); // Play Sound
		printf("Sound Byte: 0x%x\n", StartSound); // print ID of sound played.
	}
	// R3: Next Sound
	if ((pad->btns & PAD_R3) == 0 && PLAYING_TRACK == 0)
	{
		PLAYING_TRACK = 1;
		StartSound += 0x1;
		musicStopTrack();
		musicPlayTrack(StartSound * 2, 1);
		//printf("Sound Byte: 0x%x\n", StartSound);
	}
	// Select: Transition Track
	if ((pad->btns & PAD_CIRCLE) == 0 && PLAYING_TRACK == 0)
	{
		PLAYING_TRACK = 1;
		StartSound = 0;
		// musicTransitionTrack(0,0,0,0);
		musicStopTrack();
		musicPlayTrack(0, 1);
		printf("Sound Byte: 0x%x\n", StartSound);
	}
	// If neither of the above are pressed, PLAYING_TRACK = 0.
	if (!(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0 && !(pad->btns & PAD_CIRCLE) == 0)
	{
		PLAYING_TRACK = 0;
	}
}

int main(void)
{
	// run normal hook
	((void (*)(void))0x00126780)();

	CampaignMusic();

	return 0;
}
