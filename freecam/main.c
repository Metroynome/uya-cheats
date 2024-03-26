#include <tamtypes.h>

#include <libuya/string.h>
#include <libuya/player.h>
#include <libuya/utils.h>
#include <libuya/game.h>
#include <libuya/pad.h>
#include <libuya/stdio.h>
#include <libuya/uya.h>
#include <libuya/interop.h>
#include <libuya/graphics.h>
#include <libuya/math.h>
#include <libuya/math3d.h>


int Active = 0;
int ToggleScoreboard = 0;
int ToggleRenderAll = 0;
int Occlusion = 2;
VECTOR CameraPosition,
		PlayerBackup,
		targetPos,
		cameraPos,
		delta;

void MovementInputs(Player * player, PadButtonStatus * pad)
{
	// UYA Messes up because v would always be 0.
	VECTOR v;
	vector_copy(v, CameraPosition);
    float CameraYaw = player->fps.Vars.CameraZ.rotation;
    float CameraPitch = player->fps.Vars.CameraY.rotation;

	// get rotation from yaw and pitch
	float ySin = sinf(CameraYaw);
	float yCos = cosf(CameraYaw);
	float pSin = sinf(CameraPitch);
	float pCos = cosf(CameraPitch);

	// Handle Speed
	// Default Speed
	float MOVE_SPEED = .05;
	// L1: Fast Speed
	if ((pad->btns & PAD_L1) == 0 && (pad->btns & PAD_R1) != 0)
	{
		MOVE_SPEED = .3;
	}
	// R1: Slow Speed
	if ((pad->btns & PAD_R1) == 0 && (pad->btns & PAD_L1) != 0)
	{
		MOVE_SPEED = 0.01;
	}

	// Left Analog
	// Swapped library pad->ljoy with another line.
	// It adds more error for drifting analog sticks.
	float LeftAnalogH = (*(float*)0x00225a08);
	float LeftAnalogV = (*(float*)0x00225a0c);
	if ((LeftAnalogV || LeftAnalogH) != 0)
	{
		float hSpeed = LeftAnalogH * MOVE_SPEED;
		float vSpeed = -LeftAnalogV * MOVE_SPEED;

		// generate vertical and horizontal vectors
		v[0] += (yCos * vSpeed) + (ySin * hSpeed);
		v[1] += (ySin * vSpeed) + (-yCos * hSpeed);
		v[2] += (pSin * -vSpeed);
		v[3] = 0;
	}
	// D-Pad Up: Forward
	if ((pad->btns & PAD_UP) == 0)
	{
		v[0] += (yCos * MOVE_SPEED);
		v[1] += (ySin * MOVE_SPEED);
	}
	// D-Pad Down: Backward
	if ((pad->btns & PAD_DOWN) == 0)
	{
		v[0] += (-yCos * MOVE_SPEED);
		v[1] += (-ySin * MOVE_SPEED);
	}
	// D-Pad Left: Strafe Left
	if ((pad->btns & PAD_LEFT) == 0)
	{
		v[0] += (-ySin * MOVE_SPEED);
		v[1] += (yCos * MOVE_SPEED);
	}
	// D-Pad Right: Strafe Right
	if ((pad->btns & PAD_RIGHT) == 0)
	{
		v[0] += (ySin * MOVE_SPEED);
		v[1] += (-yCos * MOVE_SPEED);
	}
	// L2 = Move Down
	if ((pad->btns & PAD_L2) == 0 && (pad->btns & PAD_R2) != 0)
	{
		//v[2] = (pCos * -MOVE_SPEED);
		v[2] -= MOVE_SPEED;
	}
	// R2 = Move Up
	if ((pad->btns & PAD_R2) == 0 && (pad->btns & PAD_L2) != 0)
	{
		//v[2] = (pCos * MOVE_SPEED);
		v[2] += MOVE_SPEED;
	}
	// R3: Select target for lock rotation
	if ((pad->btns & PAD_R3) == 0)
	{
		vector_copy(targetPos, CameraPosition);
	}
	// Hold Circle: Lock Camera
	if ((pad->btns & PAD_CIRCLE) == 0)
	{
		vector_copy(delta, CameraPosition);
		delta[2] += 1;
		vector_subtract(delta, targetPos, delta);
		float len = vector_length(delta);
		float targetYaw = atan2f(delta[1] / len, delta[0] / len);
		float targetPitch = asinf(-delta[2] / len);
		player->fps.Vars.CameraZ.rotation = targetPitch;
		player->fps.Vars.CameraY.rotation = targetYaw;
	}
	
	// Add Vector to Camera Position
	// vector_add(CameraPosition, CameraPosition, v);

	vector_copy(CameraPosition, v);
}

void activate(Player * player)
{
	// Copy Current Player Camera Position and store it.
	vector_copy(CameraPosition, player->fps.CameraPos);

	// Copy Current Player Position and store it.
	vector_copy(PlayerBackup, player->playerPosition);

	// Let Camera go past the death barrier
	// *(u32*)0x005F40DC = 0x10000006;

	// Set Respawn Timer to Zero, then negative so player can't respawn
	// player->RespawnTimer = -1;

	// deactivate hud
	// hud->Flags.Healthbar = 0;
	// hud->Flags.Minimap = 0;
	// hud->Flags.Weapons = 0;
	// hud->Flags.Popup = 0;
	// hud->Flags.NormalScoreboard = 0;
}

void deactivate(Player * player)
{
	// Reset Player Position
	player->cheatX = PlayerBackup[0];
	player->cheatZ = PlayerBackup[1];
	player->cheatY = PlayerBackup[2];

	// Set Camera Distance to Default
	player->fps.Vars.CameraPositionOffset[0] = -6;

	// Don't let Camera go past death barrier
	// *(u32*)0x005F40DC = 0x10400006;

	// Reset Respawn timer
	player->timers.resurrectWait = 0;

	// Reset Occlusion
	if (Occlusion == 0)
    {
        Occlusion = 2;
        gfxOcclusion(Occlusion);
    }

	// reactivate hud
	// hud->Flags.Healthbar = 1;
	// hud->Flags.Minimap = 1;
	// hud->Flags.Weapons = 1;
	// hud->Flags.Popup = 1;
	// hud->Flags.NormalScoreboard = 1;
}

int main(void)
{
	// ensure we're in game
	if (!isInGame())
	{
		Active = 0;
		return -1;
	}

	// Get Local Player
	Player * player = (Player*)PLAYER_STRUCT;
	PadButtonStatus * pad = (PadButtonStatus*)player->pPad;
	// PlayerHUDFlags * hud = hudGetPlayerFlags(0);

	if (!Active)
	{
		// Don't activate if player is in Vehicle
		// Activate with L1 + R1 + L3
		if ((pad->btns & (PAD_L1 | PAD_R1 | PAD_L3)) == 0)
		{
			Active = 1;
			activate(player);
		}
	}
	else if (Active)
	{
		// Deactivate with L1 + R1 + R3
		if ((pad->btns & (PAD_L1 | PAD_R1 | PAD_R3)) == 0)
		{
			Active = 0;
			deactivate(player);
		}
	}
	
	if (!Active)
		return 0;

	// If start isn't open, let inputs go through.
	if (player->pauseTimer == 0)
	{
		// Select: Toggle Score
		// if ((pad->btns & PAD_SELECT) == 0 && ToggleScoreboard == 0)
		// {
		// 	ToggleScoreboard = 1;
		// 	hud->Flags.NormalScoreboard = !hud->Flags.NormalScoreboard;
		// }
		// else if (!(pad->btns & PAD_SELECT) == 0)
		// {
		// 	ToggleScoreboard = 0;
		// }
		// Square: Toggle Render All
		if ((pad->btns & PAD_SQUARE) == 0 && ToggleRenderAll == 0)
		{
			ToggleRenderAll = 1;
			
			// If Occlusion equals 2 (On) change it to zero (Off)
			Occlusion = (Occlusion == 2) ? 0 : 2;
		    gfxOcclusion(Occlusion);
		}
		else if (!(pad->btns & PAD_SQUARE) == 0)
		{
			ToggleRenderAll = 0;
		}
		// Handle All Movement Inputs
		MovementInputs(player, pad);
	}
	// Apply Camera Position
	vector_copy(player->fps.CameraPos, CameraPosition);

	// If player isn't dead, move player to X: Zero
	if (playerGetHealth(player) > 0)
	{
		player->cheatX = 0;
		player->cheatZ = PlayerBackup[1];
		player->cheatY = PlayerBackup[2] + 0x00100000;
	}

	// Force Hold Wrench
	player->wrenchOnly = 1;

	// Constanty Set Camera Distance to Zero
	player->fps.Vars.CameraPositionOffset[0] = 0;

	// fix death camera lock
	player->fps.Vars.CameraYMin = 1.48353;
	player->fps.Vars.CameraYMax = -1.22173;

	return 0;
}
