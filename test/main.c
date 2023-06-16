#include <tamtypes.h>

#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/weapon.h>
#include <libuya/interop.h>
#include <libuya/moby.h>

int GameRulesInitialized = 0;
int HasDisabledHealthboxes = 0;
short PlayerKills[GAME_MAX_PLAYERS];

int cheatsDisableHealthboxes(void)
{
    int count = 0;
    Moby* moby = mobyListGetStart();

    // Iterate through mobys and disable healthboxes
    while ((moby = mobyFindNextByOClass(moby, MOBY_ID_HEALTH_BOX_MULT)))
    {
        // move to 0,0,0
        memset(moby->Position, 0, sizeof(moby->Position));

        // move orb to 0,0,0
        if (moby->PVar)
        {
            void * subPtr = (void*)(*(u32*)(moby->PVar));
            if (subPtr)
            {
                Moby * orb = (Moby*)(*(u32*)(subPtr + 0x48));
                if (orb)
                {
                    memset(orb->Position, 0, sizeof(orb->Position));
                    ++count;
                }
            }
        }

        ++moby; // increment moby
    }
    
    return count;
}

void grInitialize(void)
{
	int i;

	// Initialize player kills to 0
	for (i = 0; i < GAME_MAX_PLAYERS; ++i)
		PlayerKills[i] = 0;

	HasDisabledHealthboxes = 0;
	GameRulesInitialized = 1;
}

void grGameStart(void)
{
	int i = 0;
	// Initialize
	if (GameRulesInitialized != 1)
	{
		grInitialize();
	}

	if (!HasDisabledHealthboxes)
		HasDisabledHealthboxes = cheatsDisableHealthboxes();
}





void Debug()
{
    PadButtonStatus * pad = (PadButtonStatus*)0x00225980;
    static int Active = 0;
    // DEBUG OPTIONS: L3 = Testing, R3 = Hurt Player
    if ((pad->btns & PAD_L3) == 0 && Active == 0)
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

		// Player * player = (Player*)PLAYER_STRUCT;
		// Moby * m = mobySpawn(MOBY_ID_SWINGSHOT_ORB, 0);
		// m->Position[0] = 580;
		// m->Position[2] = 200;
		// m->Position[1] = 280;
		// m->Scale = 15;
		
	}
    else if ((pad->btns & PAD_R3) == 0 && Active == 0)
    {
        Active = 1;
        // Hurt Player
        playerIncHealth(PLAYER_STRUCT, 1);
    }
    if (!(pad->btns & PAD_L3) == 0 && !(pad->btns & PAD_R3) == 0)
    {
        Active = 0;
    }
}

void patchResurrectWeaponOrdering_HookWeaponStripMe(Player * player)
{
	// backup currently equipped weapons
	// if (player->IsLocal) {
	// 	weaponOrderBackup[player->LocalPlayerIndex][0] = playerGetLocalEquipslot(player->LocalPlayerIndex, 0);
	// 	weaponOrderBackup[player->LocalPlayerIndex][1] = playerGetLocalEquipslot(player->LocalPlayerIndex, 1);
	// 	weaponOrderBackup[player->LocalPlayerIndex][2] = playerGetLocalEquipslot(player->LocalPlayerIndex, 2);
	// }

	// call hooked WeaponStripMe function after backup
	((void (*)(Player*))0x005e2e68)(player);
}

void patchResurrectWeaponOrdering_HookGiveMeRandomWeapons(Player* player, int weaponCount)
{
	// int i, j, matchCount = 0;

	// call hooked GiveMeRandomWeapons function first
	((void (*)(Player*, int))0x005f7510)(player, weaponCount);

	// then try and overwrite given weapon order if weapons match equipped weapons before death
	// if (player->IsLocal) {

	// 	// restore backup if they match (regardless of order) newly assigned weapons
	// 	for (i = 0; i < 3; ++i) {
	// 		int backedUpSlotValue = weaponOrderBackup[player->LocalPlayerIndex][i];
	// 		for (j = 0; j < 3; ++j) {
	// 			if (backedUpSlotValue == playerGetLocalEquipslot(player->LocalPlayerIndex, j)) {
	// 				matchCount++;
	// 			}
	// 		}
	// 	}

	// 	// if we found a match, set
	// 	if (matchCount == 3) {
	// 		// set equipped weapon in order
	// 		for (i = 0; i < 3; ++i) {
	// 			playerSetLocalEquipslot(player->LocalPlayerIndex, i, weaponOrderBackup[player->LocalPlayerIndex][i]);
	// 		}

	// 		// equip first slot weapon
	// 		playerEquipWeapon(player, weaponOrderBackup[player->LocalPlayerIndex][0]);
	// 	}
	// }
}

void patchResurrectWeaponOrdering(void)
{
	if (!isInGame())
		return;

	HOOK_JAL(0x004EF550, &patchResurrectWeaponOrdering_HookWeaponStripMe);
	HOOK_JAL(0x004EF56C, &patchResurrectWeaponOrdering_HookGiveMeRandomWeapons);
}

int main()
{
    if (isInGame())
    {
        //patchResurrectWeaponOrdering();
        Debug();
		grGameStart();
    }
    return 0;
}
