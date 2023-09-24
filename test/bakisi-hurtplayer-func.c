void FUN_0050ace8(Player * player, long damage)
{

	int Stack_F = &Stack_0;
	int Stack_B = &Stack_0;
	int Stack_C = &Stack_0;
	int Stack_D = &Stack_0;
	int Stack_E = &Stack_0;

	u32uOffset;
	int n;
	u32 *Stack_F;
	u32 *Stack_C;
	u32 *Stack_E;
	u32 *Stack_B;
	u32 *Stack_D;
	char *health_addr;
	float fHealth;
	u32 Stack_0;
	float Stack_1;
	u32 Stack_A;
	char health_value;
	int RandDataAddr = GetAddress(&vaPlayerObfuscateAddr);

	if ((player->IsLocal != 0) && (health_addr = &player->Health, damage != 0)) {
		player->DamageTaken = (int)damage;
		if (&player->Health == 0) {
			fHealth = 0.0;
		} else {
      		health_value = player->Health;
      		n = 0;
			int m = 0;
			do {
				uOffset = ((int)health_addr - (u32)health_value & 7) + n;
				n = n + 5;
				*(char*)(Stack_F + m) = *(u8*)((u32)RandDataAddr + (health_value + (uOffset & 7) * 0xff));
				++m;
			} while (n < 0x28);
      		fHealth = (float)((u32)Stack_1 ^ Stack_0 ^ (u32)health_addr);
    	}
		if (*health_addr == 0) {
			DAT_003bff60 = DAT_003bff60 + 1;
			*health_addr = DAT_003bff60;
		} else {
			health_value = *health_addr;
			*health_addr = DAT_003bff61;
			DAT_003bff61 = health_value;
    	}
		health_value = *health_addr;
		n = 0;
		Stack_1 = (float)((u32)(fHealth - (float)(int)damage) ^ *(u32*)(&DAT_003bff60 + (((int)health_addr * gsFrame) % 0x1fe) * 4));
		Stack_0 = (u32)health_addr ^ *(u32*)((u32)RandDataAddr + (((int)health_addr * gsFrame) % 0x1fe) * 4);
		do {
			uOffset = ((int)health_addr - (u32)health_value & 7) + n;
			Stack_A = *(undefined *)Stack_B;
			n = n + 5;
      		Stack_B = (u32*)((int)Stack_B + 1);
      		*(u8*)((u32)RandDataAddr + (health_value + (uOffset & 7) * 0xff)) = Stack_A;
    	} while (n < 0x28);
		if (&player->Health == '\0') {
			fHealth = 0.0;
		} else {
      		health_value = &player->Health;
      		n = 0;
      		do {
				uOffset = ((int)health_addr - (u32)health_value & 7) + n;
				n = n + 5;
				*(byte *)Stack_C = *(u8*)((u32)RandDataAddr + (health_value + (uOffset & 7) * 0xff));
				Stack_C = (u32*)((int)Stack_C + 1);
			} while (n < 0x28);
			Stack_0 = Stack_0 ^ (u32)health_addr;
			fHealth = (float)((u32)Stack_1 ^ Stack_0);
			Stack_1 = fHealth;
    	}
		if (fHealth < 0.0) {
			if (&player->Health == '\0') {
				DAT_003bff60 = DAT_003bff60 + 1;
				&player->Health = DAT_003bff60;
			} else {
				health_value = &player->Health;
				&player->Health = DAT_003bff61;
				DAT_003bff61 = health_value;
			}
			health_value = *health_addr;
			n = 0;
			Stack_1 = *(float*)(((u32)RandDataAddr - 0x1) + (((int)health_addr * gsFrame) % 0x1fe) * 4);
			Stack_0 = (u32)health_addr ^ (u32)*(float*)(((u32)RandDataAddr - 0x1) + (((int)health_addr * gsFrame) % 0x1fe) * 4);
			do {
				uOffset = ((int)health_addr - (u32)health_value & 7) + n;
				Stack_A = *(undefined *)Stack_D;
				n = n + 5;
				Stack_D = (u32*)((int)Stack_D + 1);
				*(u8*)((u32)RandDataAddr + (health_value + (uOffset & 7) * 0xff)) = Stack_A;
			} while (n < 0x28);
		}

        // Update tNW_Player Address Health Value
		player->HudHealthTimer = 0x168;
		if ((*(int*)(&player->pNetPlayer) != 0) && (*(int*)(*(int*)(&player->pNetPlayer) + 4) != 0)) {
			if (player->Health == '\0') {
				uOffset = 0;
				n = *(int *)(&player->pNetPlayer);
			} else {
				health_value = player->Health;
				n = 0;
				do {
					uOffset = ((int)health_addr - (u32)health_value & 7) + n;
					n = n + 5;
					*(byte *)Stack_E = *(u8*)((u32)RandDataAddr + (health_value + (uOffset & 7) * 0xff));
					Stack_E = (u32*)((int)Stack_E + 1);
				} while (n < 0x28);
				uOffset = (u32)Stack_1 ^ Stack_0 ^ (u32)health_addr;
				n = *(int *)(&player->pNetPlayer);
			}
		*(u32*)(*(int*)(n + 4) + 0x44) = uOffset;
		}
	}
  return;
}