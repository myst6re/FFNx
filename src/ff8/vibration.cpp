/****************************************************************************/
//    Copyright (C) 2009 Aali132                                            //
//    Copyright (C) 2018 quantumpencil                                      //
//    Copyright (C) 2018 Maxime Bacoux                                      //
//    Copyright (C) 2020 Chris Rizzitello                                   //
//    Copyright (C) 2020 John Pritchard                                     //
//    Copyright (C) 2022 myst6re                                            //
//    Copyright (C) 2022 Julian Xhokaxhiu                                   //
//    Copyright (C) 2022 Tang-Tang Zhou                                     //
//                                                                          //
//    This file is part of FFNx                                             //
//                                                                          //
//    FFNx is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU General Public License as published by  //
//    the Free Software Foundation, either version 3 of the License         //
//                                                                          //
//    FFNx is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of        //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
//    GNU General Public License for more details.                          //
/****************************************************************************/
#include "vibration.h"
#include "../ff8.h"
#include "../log.h"
#include "../gamepad.h"
#include "../joystick.h"
#include "../patch.h"
#include "../vibration.h"
#include <xxhash.h>

char vibrateName[32] = "";

int ff8_vibrate_capability(int port)
{
	bool ret = false;

	if (trace_all) ffnx_trace("%s port=%d\n", __func__, port);

	return nxVibrationEngine.canRumble();
}

int ff8_game_is_paused(int callback)
{
	if (trace_all) ffnx_trace("%s callback=%p\n", __func__, callback);

	if (nxVibrationEngine.canRumble())
	{
		// Reroute to the vibrate pause menu
		int ret = ff8_externals.pause_menu_with_vibration(callback);

		int keyon = ff8_externals.get_keyon(0, 0);
		// Press pause again
		if ((keyon & 0x800) != 0) {
			return 1;
		}

		return ret;
	}

	return ff8_externals.pause_menu(callback);
}

void vibrate(int left, int right)
{
	const int port = 0;

	if (trace_all || (trace_gamepad && (left | right) != 0)) ffnx_trace("%s left=%d right=%d vibrate_option_enabled=%d\n", __func__, left, right, ff8_externals.gamepad_input_states[port].vibrate_option_enabled);

	if (ff8_externals.gamepad_input_states[port].vibrate_option_enabled == 255)
	{
		nxVibrationEngine.setLeftMotorValue(left);
		nxVibrationEngine.setRightMotorValue(right);
		nxVibrationEngine.rumbleUpdate();
	}
}

void apply_vibrate_calc(char port, int left, int right)
{
	ff8_gamepad_input_state *gamepad_state = ff8_externals.gamepad_input_states;
	if (left < 0) left = 0;
	if (right < 0) right = 0;

	// Keep the game implementation
	gamepad_state[port & 0x1].vibration_active = 1;
	gamepad_state[port & 0x1].left_motor_speed = left;
	gamepad_state[port & 0x1].right_motor_speed = right;

	vibrate(left, right);
}

const char *set_name(const char *name)
{
	return strncpy(vibrateName, name, sizeof(vibrateName));
}

size_t vibrate_data_size(const uint8_t *data)
{
	const uint32_t *header = (const uint32_t *)data;
	uint32_t max_pos = 0;

	while (header - (const uint32_t *)data < 64) {
		if (*header > max_pos) {
			max_pos = *header;
		}
		header++;
	}

	if (max_pos == 0) {
		return 0;
	}

	const uint16_t *it = (const uint16_t *)(data + max_pos);
	uint16_t left_size = it[0], right_size = it[1];

	return max_pos + 4 + left_size + right_size;
}

const char *vibrate_data_name(const uint8_t *data)
{
	if (trace_all || trace_gamepad) ffnx_trace("%s: data=0x%X\n", __func__, data);

	if (data == *ff8_externals.vibrate_data_battle)
	{
		return set_name("battle");
	}

	if (data == *ff8_externals.vibrate_data_main)
	{
		return set_name("main");
	}

	if (getmode_cached()->driver_mode == MODE_WORLDMAP)
	{
		return set_name("world");
	}

	size_t size = vibrate_data_size(data);
	if (size == 0)
	{
		return nullptr;
	}

	XXH64_hash_t hash = XXH3_64bits(data, size);

	if (trace_all || trace_gamepad) ffnx_trace("%s: hash=0x%llX\n", __func__, hash);

	switch (hash)
	{
	case 0x3A8B11844E20E005:
		return set_name("field");
	case 0x12ED0CB959EE1D06:
		return set_name("battle_quezacotl");
	case 0x5609D2CFA36FAA0A:
		return set_name("battle_shiva");
	case 0xCEFDFB6A96824726:
		return set_name("battle_ifrit");
	case 0x453F6DFFE0C9349F:
		return set_name("battle_siren");
	case 0xEBD9E3F6260E3959:
		return set_name("battle_brothers");
	case 0x9BFBB2F64D8D7481:
		return set_name("battle_diablos");
	case 0xBB77D755D1E18AAE:
		return set_name("battle_carbuncle");
	case 0x6CE5937D628A4A8C:
		return set_name("battle_leviathan");
	case 0x9FC2EA78F38A8DB2:
		return set_name("battle_pandemona");
	case 0xA0CB223A3095EF8E:
		return set_name("battle_cerberus");
	case 0x4618E0051D8EB26A:
		return set_name("battle_alexander");
	case 0x8226A772A7F38BE3:
		return set_name("battle_helltrain");
	case 0xB0A0606DC821934F:
		return set_name("battle_bahamut");
	case 0x9B9C719D7B06C9F6:
		return set_name("battle_cactuar");
	case 0xC5C7EEC0C8CA65F0:
		return set_name("battle_tonberry");
	case 0x52E0D8894C20D609:
		return set_name("battle_eden");
	case 0x928D553922FE8248:
		return set_name("battle_odin");
	case 0x79AC9002E7CD9699:
		return set_name("battle_gilgamesh_zantetsuken");
	case 0x50BE9269B62CFD2E:
		return set_name("battle_gilgamesh_excalipoor");
	case 0x182DC6DE431B0B59:
		return set_name("battle_phoenix");
	case 0xFCF682172921F687:
		return set_name("battle_moomba");
	case 0x2870C502E6C869CA:
		return set_name("battle_minimog");
	case 0x34A36AEB4BDB2873:
		return set_name("battle_boko_chocofire");
	case 0x71C550B41E196BF5:
		return set_name("battle_boko_chocoflare");
	case 0x58D0E469C7393C42:
		return set_name("battle_boko_chocometeor");
	case 0x711807BEA87AE308:
		return set_name("battle_boko_chocobocle");
	case 0x66A9AC8656CDA67E:
		return set_name("battle_meteor");
	case 0x8CB04A5C704B20BB:
		return set_name("battle_apocalypse");
	case 0xE443180422A17638:
		return set_name("battle_ultima");
	}

	snprintf(vibrateName, sizeof(vibrateName), "%llX", hash);

	return vibrateName;
}

uint32_t ff8_set_vibration_replace_id = 0;

int ff8_set_vibration(const uint8_t *data, int set, int intensity)
{
	if (trace_all || trace_gamepad) ffnx_trace("%s: set=%d intensity=%d\n", __func__, set, intensity);

	const char *name = vibrate_data_name(data);

	if (name != nullptr)
	{
		const uint8_t *dataOverride = nxVibrationEngine.vibrateDataOverride(name);
		if (dataOverride != nullptr)
		{
			data = dataOverride;
		}
		else if (data != *ff8_externals.vibrate_data_main && getmode_cached()->driver_mode == MODE_WORLDMAP)
		{
			// Disable vibration
			return 0;
		}
	}

	unreplace_function(ff8_set_vibration_replace_id);
	int ret = ((int(*)(const uint8_t*,int,int))ff8_externals.set_vibration)(data, set, intensity);
	rereplace_function(ff8_set_vibration_replace_id);

	return ret;
}
/*
int ff8_dinput_get_state(int port)
{
	//if (trace_all || trace_gamepad) ffnx_trace("%s: port=%d\n", __func__, port);

	uint8_t **ff8_input_state_copy_dword_209CBC0 = (uint8_t **)0x209CBBC;

	(*ff8_input_state_copy_dword_209CBC0)[-2] = ((*ff8_input_state_copy_dword_209CBC0)[-2] & 0xF) | (1 << 4);
	(*ff8_input_state_copy_dword_209CBC0)[1] = 64;

	return ((int(*)(int))0x4684F0)(port);
}*/

int ff8_get_analog_value(int port, int type, int offset)
{
	int ret = -1;

	if (port != 0) {
		return -1;
	}

	ff8_gamepad_input_state *gamepad_state = ff8_externals.gamepad_input_states;

	if (type == 2) {
		if (gamepad.leftStickY == 0.0f) {
			ret = 128;
		} else if (gamepad.leftStickY < -0.5f) {
			ret = 128 + 127;
		} else if (gamepad.leftStickY > 0.5f) {
			ret = 128 - 127;
		} else {
			ret = 128;
		}
	} else if (type == 3) {
		if (gamepad.leftStickX == 0.0f) {
			ret = 128;
		} else if (gamepad.leftStickX < -0.5f) {
			ret = 128 - 127;
		} else if (gamepad.leftStickX > 0.5f) {
			ret = 128 + 127;
		} else {
			ret = 128;
		}
	} else {
		return -1;
	}

	if (trace_all || trace_gamepad) ffnx_trace("%s: port=%d, type=%d, offset=%d, lX=%d, lY=%d, gamepadX=%f, gamepadY=%f, ret=%d, left_stick_x=%d, left_stick_y=%d\n", __func__, port, type, offset, ff8_externals.dinput_gamepad_state->lX, ff8_externals.dinput_gamepad_state->lY, gamepad.leftStickX, gamepad.leftStickY, ret, gamepad_state->entries[gamepad_state->entries_offset & 0x7].left_stick_x, gamepad_state->entries[gamepad_state->entries_offset & 0x7].left_stick_y);

	return ret;
}

void vibration_init()
{
	replace_call(uint32_t(ff8_externals.check_game_is_paused) + 0x88, ff8_game_is_paused);
	replace_function(ff8_externals.get_vibration_capability, ff8_vibrate_capability);
	replace_function(ff8_externals.vibration_apply, apply_vibrate_calc);
	replace_function(ff8_externals.vibration_clear_intensity, noop_a1);
	ff8_set_vibration_replace_id = replace_function(ff8_externals.set_vibration, ff8_set_vibration);

	//replace_call(0x56DD12, ff8_dinput_get_state);
	replace_function(0x49EA10, ff8_get_analog_value);
}
