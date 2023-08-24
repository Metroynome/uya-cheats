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

int state;
char weapon[3];

void InfiniteChargeboot(void)
{
	Player * player = (Player*)PLAYER_STRUCT;
	PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
	if (player->IsChargebooting == 1 && (pad->btns & PAD_R2) == 0 && player->StateTimer > 55)
	{
		player->StateTimer = 55;
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
            (float)player->Velocity[2] = 0.125;
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

        // WeaponStripMe (Hook: 0x004EF550)
        // ((void (*)(u32))0x004EF848)(PLAYER_STRUCT);

        // GiveMeRandomWeapons (Hook: 0x004EF56C)
        //((void (*)(u32, int))0x0050ED38)(PLAYER_STRUCT, 0x3);

        // Give Weapon (Hook: 0x0050EE0C)
        // - Quick Select Slots get set in order of which weapons are given first.
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x2, 2);
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x3, 2);
        // ((void (*)(u32, int, int))0x0053BD90)((u32)PLAYER_STRUCT + 0x1a40, 0x5, 2);

        // Equip Weapon: (Hook: 0x0050EE2C)
        // ((void (*)(u32, int))0x0053C2D0)((u32)PLAYER_STRUCT + 0x1a40, 0x2);
        // - Inside above address: 0x0053C398 (JAL)

		// Respawn Function
		//((void (*)(u32))0x004EF510)(PLAYER_STRUCT);
		// Remove save health to tnw player
		//*(u32*)0x004EF79C = 0;

		// Swap Teams (blue <-> red)
		player->Team = !player->Team;

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
		// Set Gattling Turret Health to 1.
		// DEBUGsetGattlingTurretHealth();
	}
    if (!(pad->btns & PAD_LEFT) == 0 && !(pad->btns & PAD_RIGHT) == 0 && !(pad->btns & PAD_UP) == 0 && !(pad->btns & PAD_DOWN) == 0)
    {
        Active = 0;
    }
}



// ===============================================================
// u32 playerDeobfuscate(u32 src)
// {
//     static int StackAddr[1];
//     int RandDataAddr = GetAddress(&vaPlayerObfuscateAddr);
// 	u32 Player_Addr = src;
//     u32 Player_Value = *(u8*)src;
//     int n = 0;
// 	int rawr = 0;
// 	do {
// 		u32 Offset = (u32)((int)Player_Addr - (u32)Player_Value & 7) + n;
// 		n = n + 5;
// 		*(u8*)((int)StackAddr + rawr) = *(u8*)((u32)RandDataAddr + (Player_Value + (Offset & 7) * 0xff));
// 		++rawr;
// 	} while (n < 0x28);
// 	// XORAddr (first 4 bytes of StackAddr)
// 	StackAddr[0] = (u32)(StackAddr[0]) ^ (u32)Player_Addr;
// 	// XORValue (second 4 bytes of StackAddr)
// 	StackAddr[1] = (u32)((u32)StackAddr[1] ^ StackAddr[0]);
// 	u32 Converted = StackAddr[1];
// 	return Converted;
// }

u32 playerDeobfuscateWeapon(u32 src)
{
    static int Stack[2];
    int RandDataAddr = 0x003aa181;
    u32 Player_Addr = src + 0x1;
    u32 Player_Value = *(u8*)Player_Addr;
    int rawr = 0;
    int n = 0;
    do {
        u32 Offset = (u32)((u32)Player_Addr - (u8)Player_Value & 7) + n;
        n = n + 3;
        *(u8*)((u32)Stack + rawr) = *(u8*)((u32)RandDataAddr + (Offset & 7) * 0xd1 + (u8)Player_Value);
        ++rawr;
    } while (n < 0x18);
    Stack[0] = (u32)Stack[0] ^ (u32)Player_Addr;
    Stack[2] = ((u32)Stack[1] ^ (u32)Stack[0]) >> 0x10;
    n = (u32)((u32)Stack[1] ^ (u32)Stack[0] ^ (u32)Stack[2]) * 0x10000 + 1;
    Stack[1] = (u32)Stack[2] / n;
    u64 hi = (u64)((u32)Stack[2] % n);
    Stack[2] = (u32)Stack[2] & 0xff;
    return (u8)Stack[2];
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
		Player * player = (Player*)PLAYER_STRUCT;
		// state = playerDeobfuscate(&player->State);
		weapon[0] = playerDeobfuscateWeapon(&player->QuickSelect.Slot[0]);
		weapon[1] = playerDeobfuscateWeapon(&player->QuickSelect.Slot[1]);
		weapon[2] = playerDeobfuscateWeapon(&player->QuickSelect.Slot[2]);


		InfiniteChargeboot();
		InfiniteHealthMoonjump();
        Debug();
    }
	
	uyaPostUpdate();

    return 0;
}
