/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include <cute_binding.h>
#include <cute_c_runtime.h>
#include <cute_array.h>
#include <cute_alloc.h>

#include <internal/cute_alloc_internal.h>

#include <math.h>

//--------------------------------------------------------------------------------------------------
// Internal types.

static float s_binding_deadzone = 0.15f;
#define CF_BINDING_MAX_JOYPADS 8

enum CF_InputType
{
	CF_INPUT_TYPE_KEY,
	CF_INPUT_TYPE_MOUSE,
	CF_INPUT_TYPE_BUTTON,
	CF_INPUT_TYPE_TRIGGER,
};

struct CF_InputBinding
{
	CF_InputType type;
	bool positive;
	float threshold;
	union {
		int key;
		int button;
		int axis;
	};
};

struct CF_ButtonBindingInternal
{
	int index;
	float press_buffer;
	dyna CF_InputBinding* inputs = NULL;
	float last_timestep = 0;
	float last_press_time = -1;
	float last_release_time = -1;
	bool is_down = false;
	float v = 0;
	float v_raw = 0;
	bool pressed = false;
	bool released = false;
	bool press_consumed = false;
	bool release_consumed = false;
	float deadzone = -1;
	float repeat_delay = -1;
	float repeat_interval = 0;
	float next_repeat_time = 0;
	bool repeated = false;
};

struct CF_AxisBindingInternal
{
	int index;
	CF_AxisConflict conflict = CF_AXIS_CONFLICT_NEWEST;
	CF_ButtonBinding negative;
	CF_ButtonBinding positive;
};

struct CF_StickBindingInternal
{
	CF_AxisBinding x_axis;
	CF_AxisBinding y_axis;
	int player_index = 0;
	int analog_x_axis = -1;
	int analog_y_axis = -1;
	float circular_deadzone = -1;
};

struct CF_JoypadInfo
{
	int index;
	bool connected;
	CF_JoypadPowerLevel power_level;
	const char* name;
	CF_JoypadType type;
	uint16_t vendor;
	uint16_t product_id;
	const char* serial_number;
	uint16_t firmware_version;
	uint16_t product_version;
};

struct CF_JoypadConnectCallback
{
	void* udata;
	void (*fn)(int, bool, void*);
};

//--------------------------------------------------------------------------------------------------
// File-scope state.

static dyna CF_ButtonBindingInternal** s_button_bindings;
static dyna CF_JoypadInfo* s_joypads;
static dyna CF_JoypadConnectCallback* s_on_joypad_connects;
static bool s_binding_initialized;

//--------------------------------------------------------------------------------------------------
// Private helpers.

static float s_int16_to_float_raw(int16_t v)
{
	return (float)v / 32767.0f;
}

static float s_int16_to_float_dz(int16_t v, float dz)
{
	float normalized_v = (float)v / 32767.0f;
	float abs_v = fabsf(normalized_v);
	if (abs_v < dz) {
		return 0;
	}
	float sign = normalized_v > 0 ? 1.0f : -1.0f;
	float remapped = (abs_v - dz) / (1.0f - dz);
	return sign * remapped;
}

static float s_int16_to_float(int16_t v)
{
	return s_int16_to_float_dz(v, s_binding_deadzone);
}

static bool s_axis_is_down_raw(CF_InputBinding input, float v)
{
	if ((v > 0 && input.positive) || (v < 0 && !input.positive)) {
		if (fabsf(v) >= input.threshold) {
			return true;
		}
	}
	return false;
}

static bool s_axis_is_down(int index, CF_InputBinding input, float dz)
{
	float v = s_int16_to_float_dz(cf_joypad_axis(index, (CF_JoypadAxis)input.axis), dz);
	return s_axis_is_down_raw(input, v);
}

static bool s_axis_prev_is_down(int index, CF_InputBinding input, float dz)
{
	float v = s_int16_to_float_dz(cf_joypad_axis_prev(index, (CF_JoypadAxis)input.axis), dz);
	return s_axis_is_down_raw(input, v);
}

static bool s_input_is_down(int index, const dyna CF_InputBinding* inputs, float dz)
{
	for (int i = 0; i < asize(inputs); ++i) {
		CF_InputBinding input = inputs[i];
		if (input.type == CF_INPUT_TYPE_KEY) {
			if (cf_key_down((CF_KeyButton)input.key)) return true;
		} else if (input.type == CF_INPUT_TYPE_MOUSE) {
			if (cf_mouse_down((CF_MouseButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_BUTTON) {
			if (cf_joypad_button_down(index, (CF_JoypadButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_TRIGGER) {
			if (s_axis_is_down(index, input, dz)) return true;
		}
	}
	return false;
}

static bool s_input_get_pressed(int index, const dyna CF_InputBinding* inputs, float dz)
{
	for (int i = 0; i < asize(inputs); ++i) {
		CF_InputBinding input = inputs[i];
		if (input.type == CF_INPUT_TYPE_KEY) {
			if (cf_key_just_pressed((CF_KeyButton)input.key)) return true;
		} else if (input.type == CF_INPUT_TYPE_MOUSE) {
			if (cf_mouse_just_pressed((CF_MouseButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_BUTTON) {
			if (cf_joypad_button_just_pressed(index, (CF_JoypadButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_TRIGGER) {
			if (s_axis_is_down(index, input, dz) && !s_axis_prev_is_down(index, input, dz)) return true;
		}
	}
	return false;
}

static bool s_input_get_released(int index, const dyna CF_InputBinding* inputs, float dz)
{
	for (int i = 0; i < asize(inputs); ++i) {
		CF_InputBinding input = inputs[i];
		if (input.type == CF_INPUT_TYPE_KEY) {
			if (cf_key_just_released((CF_KeyButton)input.key)) return true;
		} else if (input.type == CF_INPUT_TYPE_MOUSE) {
			if (cf_mouse_just_released((CF_MouseButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_BUTTON) {
			if (cf_joypad_button_just_released(index, (CF_JoypadButton)input.button)) return true;
		} else if (input.type == CF_INPUT_TYPE_TRIGGER) {
			if (!s_axis_is_down(index, input, dz) && s_axis_prev_is_down(index, input, dz)) return true;
		}
	}
	return false;
}

static float s_input_get_value(int index, const dyna CF_InputBinding* inputs, float dz)
{
	float highest_value = 0;
	for (int i = 0; i < asize(inputs); ++i) {
		CF_InputBinding input = inputs[i];
		if (input.type == CF_INPUT_TYPE_KEY) {
			if (cf_key_down((CF_KeyButton)input.key)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_MOUSE) {
			if (cf_mouse_down((CF_MouseButton)input.button)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_BUTTON) {
			if (cf_joypad_button_down(index, (CF_JoypadButton)input.button)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_TRIGGER) {
			float v = s_int16_to_float_dz(cf_joypad_axis(index, (CF_JoypadAxis)input.axis), dz);
			if (s_axis_is_down_raw(input, v)) {
				float abs_v = fabsf(v);
				if (abs_v > highest_value) highest_value = abs_v;
			}
		}
	}
	return highest_value;
}

static float s_input_get_value_raw(int index, const dyna CF_InputBinding* inputs)
{
	float highest_value = 0;
	for (int i = 0; i < asize(inputs); ++i) {
		CF_InputBinding input = inputs[i];
		if (input.type == CF_INPUT_TYPE_KEY) {
			if (cf_key_down((CF_KeyButton)input.key)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_MOUSE) {
			if (cf_mouse_down((CF_MouseButton)input.button)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_BUTTON) {
			if (cf_joypad_button_down(index, (CF_JoypadButton)input.button)) return 1.0f;
		} else if (input.type == CF_INPUT_TYPE_TRIGGER) {
			float v = s_int16_to_float_raw(cf_joypad_axis(index, (CF_JoypadAxis)input.axis));
			float abs_v = fabsf(v);
			if (abs_v > highest_value) highest_value = abs_v;
		}
	}
	return highest_value;
}

static float s_effective_deadzone(CF_ButtonBindingInternal* b)
{
	return b->deadzone >= 0 ? b->deadzone : s_binding_deadzone;
}

static void s_button_binding_update(CF_ButtonBindingInternal* b)
{
	float dz = s_effective_deadzone(b);
	b->press_consumed = false;
	b->release_consumed = false;

	if (s_input_get_pressed(b->index, b->inputs, dz)) {
		b->last_timestep = (float)CF_SECONDS;
		b->last_press_time = (float)CF_SECONDS;
		b->pressed = true;
	} else {
		b->pressed = false;
	}

	if (s_input_get_released(b->index, b->inputs, dz)) {
		b->last_release_time = (float)CF_SECONDS;
		b->released = true;
	} else {
		b->released = false;
	}

	b->is_down = s_input_is_down(b->index, b->inputs, dz);
	b->v = s_input_get_value(b->index, b->inputs, dz);
	b->v_raw = s_input_get_value_raw(b->index, b->inputs);

	// Key repeat.
	b->repeated = false;
	if (b->is_down && b->repeat_delay >= 0) {
		if (b->pressed) {
			b->next_repeat_time = (float)CF_SECONDS + b->repeat_delay;
		} else if ((float)CF_SECONDS >= b->next_repeat_time) {
			b->repeated = true;
			b->next_repeat_time = (float)CF_SECONDS + b->repeat_interval;
		}
	}
}

static CF_JoypadInfo s_get_joypad_info(int index)
{
	CF_JoypadInfo joy;
	joy.index = index;
	joy.connected = cf_joypad_is_connected(index);
	joy.power_level = cf_joypad_power_level(index);
	joy.name = cf_joypad_name(index);
	joy.type = cf_joypad_type(index);
	joy.vendor = cf_joypad_vendor(index);
	joy.product_id = cf_joypad_product_id(index);
	joy.serial_number = cf_joypad_serial_number(index);
	joy.firmware_version = cf_joypad_firmware_version(index);
	joy.product_version = cf_joypad_product_version(index);
	return joy;
}

static void s_binding_init()
{
	if (s_binding_initialized) return;
	s_binding_initialized = true;
	for (int i = 0; i < CF_BINDING_MAX_JOYPADS; ++i) {
		apush(s_joypads, s_get_joypad_info(i));
	}
}

//--------------------------------------------------------------------------------------------------
// ButtonBinding API.

CF_ButtonBinding cf_make_button_binding(int player_index, float press_buffer)
{
	s_binding_init();
	CF_ButtonBindingInternal* b = CF_NEW(CF_ButtonBindingInternal);
	b->index = player_index;
	b->press_buffer = press_buffer;
	apush(s_button_bindings, b);
	CF_ButtonBinding result;
	result.id = (uint64_t)b;
	return result;
}

void cf_destroy_button_binding(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	for (int i = 0; i < asize(s_button_bindings); ++i) {
		if (s_button_bindings[i] == b) {
			adel(s_button_bindings, i);
			break;
		}
	}
	afree(b->inputs);
	CF_FREE(b);
}

void cf_button_binding_add_key(CF_ButtonBinding handle, CF_KeyButton key)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	CF_InputBinding input = {};
	input.type = CF_INPUT_TYPE_KEY;
	input.key = (int)key;
	apush(b->inputs, input);
}

void cf_button_binding_add_mouse_button(CF_ButtonBinding handle, CF_MouseButton button)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	CF_InputBinding input = {};
	input.type = CF_INPUT_TYPE_MOUSE;
	input.button = (int)button;
	apush(b->inputs, input);
}

void cf_button_binding_add_joypad_button(CF_ButtonBinding handle, CF_JoypadButton button)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	CF_InputBinding input = {};
	input.type = CF_INPUT_TYPE_BUTTON;
	input.button = (int)button;
	apush(b->inputs, input);
}

void cf_button_binding_add_trigger(CF_ButtonBinding handle, CF_JoypadAxis axis, float threshold, bool positive)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	CF_InputBinding input = {};
	input.type = CF_INPUT_TYPE_TRIGGER;
	input.axis = (int)axis;
	input.threshold = threshold;
	input.positive = positive;
	apush(b->inputs, input);
}

bool cf_button_binding_pressed(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	if (b->press_consumed) return false;
	if (b->last_press_time >= 0 && ((float)CF_SECONDS - b->last_press_time) <= b->press_buffer) {
		return true;
	}
	return b->pressed;
}

bool cf_button_binding_released(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	if (b->release_consumed) return false;
	if (b->last_release_time >= 0 && ((float)CF_SECONDS - b->last_release_time) <= b->press_buffer) {
		return true;
	}
	return b->released;
}

bool cf_button_binding_down(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	return b->is_down;
}

float cf_button_binding_value(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	return b->v;
}

float cf_button_binding_sign(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	if (b->v > 0) return 1.0f;
	else if (b->v == 0) return 0;
	else return -1.0f;
}

bool cf_button_binding_consume_press(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	bool had = cf_button_binding_pressed(handle);
	b->press_consumed = true;
	b->last_press_time = -1;
	return had;
}

bool cf_button_binding_consume_release(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	bool had = cf_button_binding_released(handle);
	b->release_consumed = true;
	b->last_release_time = -1;
	return had;
}

void cf_button_binding_set_deadzone(CF_ButtonBinding handle, float deadzone)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	b->deadzone = deadzone;
}

float cf_button_binding_value_raw(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	return b->v_raw;
}

void cf_button_binding_set_repeat(CF_ButtonBinding handle, float delay, float interval)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	b->repeat_delay = delay;
	b->repeat_interval = interval;
}

bool cf_button_binding_repeated(CF_ButtonBinding handle)
{
	CF_ButtonBindingInternal* b = (CF_ButtonBindingInternal*)handle.id;
	return b->repeated;
}

//--------------------------------------------------------------------------------------------------
// AxisBinding API.

CF_AxisBinding cf_make_axis_binding(int player_index)
{
	CF_AxisBindingInternal* a = CF_NEW(CF_AxisBindingInternal);
	a->index = player_index;
	a->negative = cf_make_button_binding(player_index, 0);
	a->positive = cf_make_button_binding(player_index, 0);
	CF_AxisBinding result;
	result.id = (uint64_t)a;
	return result;
}

void cf_destroy_axis_binding(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_destroy_button_binding(a->negative);
	cf_destroy_button_binding(a->positive);
	CF_FREE(a);
}

void cf_axis_binding_add_keys(CF_AxisBinding handle, CF_KeyButton negative, CF_KeyButton positive)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_key(a->negative, negative);
	cf_button_binding_add_key(a->positive, positive);
}

void cf_axis_binding_add_mouse_buttons(CF_AxisBinding handle, CF_MouseButton negative, CF_MouseButton positive)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_mouse_button(a->negative, negative);
	cf_button_binding_add_mouse_button(a->positive, positive);
}

void cf_axis_binding_add_joypad_buttons(CF_AxisBinding handle, CF_JoypadButton negative, CF_JoypadButton positive)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_joypad_button(a->negative, negative);
	cf_button_binding_add_joypad_button(a->positive, positive);
}

void cf_axis_binding_add_triggers(CF_AxisBinding handle, CF_JoypadAxis negative, CF_JoypadAxis positive, float threshold)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_trigger(a->negative, negative, threshold, false);
	cf_button_binding_add_trigger(a->positive, positive, threshold, true);
}

void cf_axis_binding_add_left_stick_x(CF_AxisBinding handle, float threshold)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_trigger(a->negative, CF_JOYPAD_AXIS_LEFTX, threshold, false);
	cf_button_binding_add_trigger(a->positive, CF_JOYPAD_AXIS_LEFTX, threshold, true);
}

void cf_axis_binding_add_left_stick_y(CF_AxisBinding handle, float threshold)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_trigger(a->negative, CF_JOYPAD_AXIS_LEFTY, threshold, true);
	cf_button_binding_add_trigger(a->positive, CF_JOYPAD_AXIS_LEFTY, threshold, false);
}

void cf_axis_binding_add_right_stick_x(CF_AxisBinding handle, float threshold)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_trigger(a->negative, CF_JOYPAD_AXIS_RIGHTX, threshold, false);
	cf_button_binding_add_trigger(a->positive, CF_JOYPAD_AXIS_RIGHTX, threshold, true);
}

void cf_axis_binding_add_right_stick_y(CF_AxisBinding handle, float threshold)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_add_trigger(a->negative, CF_JOYPAD_AXIS_RIGHTY, threshold, true);
	cf_button_binding_add_trigger(a->positive, CF_JOYPAD_AXIS_RIGHTY, threshold, false);
}

void cf_axis_binding_set_conflict(CF_AxisBinding handle, CF_AxisConflict mode)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	a->conflict = mode;
}

bool cf_axis_binding_pressed(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	return cf_button_binding_pressed(a->negative) || cf_button_binding_pressed(a->positive);
}

bool cf_axis_binding_released(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	return cf_button_binding_released(a->negative) || cf_button_binding_released(a->positive);
}

float cf_axis_binding_value(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	float negative = cf_button_binding_value(a->negative);
	float positive = cf_button_binding_value(a->positive);

	if (negative <= 0 && positive <= 0) return 0;
	if (negative > 0 && positive <= 0) return -negative;
	if (positive > 0 && negative <= 0) return positive;

	CF_ButtonBindingInternal* neg = (CF_ButtonBindingInternal*)a->negative.id;
	CF_ButtonBindingInternal* pos = (CF_ButtonBindingInternal*)a->positive.id;

	if (a->conflict == CF_AXIS_CONFLICT_CANCEL) return 0;
	else if (a->conflict == CF_AXIS_CONFLICT_OLDEST) {
		if (neg->last_timestep < pos->last_timestep) {
			return -negative;
		} else {
			return positive;
		}
	} else if (a->conflict == CF_AXIS_CONFLICT_NEWEST) {
		if (neg->last_timestep < pos->last_timestep) {
			return positive;
		} else {
			return -negative;
		}
	}
	return 0;
}

float cf_axis_binding_sign(CF_AxisBinding handle)
{
	float v = cf_axis_binding_value(handle);
	if (v > 0) return 1;
	if (v < 0) return -1;
	return 0;
}

bool cf_axis_binding_consume_press(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	bool a0 = cf_button_binding_consume_press(a->negative);
	bool a1 = cf_button_binding_consume_press(a->positive);
	return a0 | a1;
}

bool cf_axis_binding_consume_release(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	bool a0 = cf_button_binding_consume_release(a->negative);
	bool a1 = cf_button_binding_consume_release(a->positive);
	return a0 | a1;
}

void cf_axis_binding_set_deadzone(CF_AxisBinding handle, float deadzone)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	cf_button_binding_set_deadzone(a->negative, deadzone);
	cf_button_binding_set_deadzone(a->positive, deadzone);
}

float cf_axis_binding_value_raw(CF_AxisBinding handle)
{
	CF_AxisBindingInternal* a = (CF_AxisBindingInternal*)handle.id;
	float negative = cf_button_binding_value_raw(a->negative);
	float positive = cf_button_binding_value_raw(a->positive);
	if (negative <= 0 && positive <= 0) return 0;
	if (negative > 0 && positive <= 0) return -negative;
	if (positive > 0 && negative <= 0) return positive;
	return 0;
}

//--------------------------------------------------------------------------------------------------
// StickBinding API.

CF_StickBinding cf_make_stick_binding(int player_index)
{
	CF_StickBindingInternal* s = CF_NEW(CF_StickBindingInternal);
	s->x_axis = cf_make_axis_binding(player_index);
	s->y_axis = cf_make_axis_binding(player_index);
	s->player_index = player_index;
	CF_StickBinding result;
	result.id = (uint64_t)s;
	return result;
}

void cf_destroy_stick_binding(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	cf_destroy_axis_binding(s->x_axis);
	cf_destroy_axis_binding(s->y_axis);
	CF_FREE(s);
}

void cf_stick_binding_add_keys(CF_StickBinding handle, CF_KeyButton up, CF_KeyButton down, CF_KeyButton left, CF_KeyButton right)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	cf_axis_binding_add_keys(s->x_axis, left, right);
	cf_axis_binding_add_keys(s->y_axis, up, down);
}

void cf_stick_binding_add_wasd(CF_StickBinding handle)
{
	cf_stick_binding_add_keys(handle, CF_KEY_W, CF_KEY_S, CF_KEY_A, CF_KEY_D);
}

void cf_stick_binding_add_arrow_keys(CF_StickBinding handle)
{
	cf_stick_binding_add_keys(handle, CF_KEY_UP, CF_KEY_DOWN, CF_KEY_LEFT, CF_KEY_RIGHT);
}

void cf_stick_binding_add_dpad(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	cf_axis_binding_add_joypad_buttons(s->x_axis, CF_JOYPAD_BUTTON_DPAD_LEFT, CF_JOYPAD_BUTTON_DPAD_RIGHT);
	cf_axis_binding_add_joypad_buttons(s->y_axis, CF_JOYPAD_BUTTON_DPAD_DOWN, CF_JOYPAD_BUTTON_DPAD_UP);
}

void cf_stick_binding_add_left_stick(CF_StickBinding handle, float threshold)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	cf_axis_binding_add_left_stick_x(s->x_axis, threshold);
	cf_axis_binding_add_left_stick_y(s->y_axis, threshold);
	s->analog_x_axis = CF_JOYPAD_AXIS_LEFTX;
	s->analog_y_axis = CF_JOYPAD_AXIS_LEFTY;
}

void cf_stick_binding_add_right_stick(CF_StickBinding handle, float threshold)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	cf_axis_binding_add_right_stick_x(s->x_axis, threshold);
	cf_axis_binding_add_right_stick_y(s->y_axis, threshold);
	s->analog_x_axis = CF_JOYPAD_AXIS_RIGHTX;
	s->analog_y_axis = CF_JOYPAD_AXIS_RIGHTY;
}

bool cf_stick_binding_pressed(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	return cf_axis_binding_pressed(s->x_axis) || cf_axis_binding_pressed(s->y_axis);
}

bool cf_stick_binding_released(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	return cf_axis_binding_released(s->x_axis) || cf_axis_binding_released(s->y_axis);
}

CF_V2 cf_stick_binding_value(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;

	if (s->circular_deadzone >= 0 && s->analog_x_axis >= 0 && s->analog_y_axis >= 0) {
		// Check if any digital input is active (keys, dpad, buttons) -- if so, use the standard axis path.
		CF_AxisBindingInternal* ax = (CF_AxisBindingInternal*)s->x_axis.id;
		CF_AxisBindingInternal* ay = (CF_AxisBindingInternal*)s->y_axis.id;
		CF_ButtonBindingInternal* xn = (CF_ButtonBindingInternal*)ax->negative.id;
		CF_ButtonBindingInternal* xp = (CF_ButtonBindingInternal*)ax->positive.id;
		CF_ButtonBindingInternal* yn = (CF_ButtonBindingInternal*)ay->negative.id;
		CF_ButtonBindingInternal* yp = (CF_ButtonBindingInternal*)ay->positive.id;

		// Check for digital input on any of the four button bindings.
		bool has_digital = false;
		CF_ButtonBindingInternal* btns[4] = { xn, xp, yn, yp };
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < asize(btns[i]->inputs); ++j) {
				CF_InputBinding input = btns[i]->inputs[j];
				if (input.type == CF_INPUT_TYPE_KEY && cf_key_down((CF_KeyButton)input.key)) { has_digital = true; break; }
				if (input.type == CF_INPUT_TYPE_MOUSE && cf_mouse_down((CF_MouseButton)input.button)) { has_digital = true; break; }
				if (input.type == CF_INPUT_TYPE_BUTTON && cf_joypad_button_down(btns[i]->index, (CF_JoypadButton)input.button)) { has_digital = true; break; }
			}
			if (has_digital) break;
		}

		if (has_digital) {
			// Fall back to standard per-axis values (digital override).
			CF_V2 result;
			result.x = cf_axis_binding_value(s->x_axis);
			result.y = cf_axis_binding_value(s->y_axis);
			return result;
		}

		// Circular deadzone: get raw normalized values directly from joypad.
		float rx = s_int16_to_float_raw(cf_joypad_axis(s->player_index, (CF_JoypadAxis)s->analog_x_axis));
		float ry = s_int16_to_float_raw(cf_joypad_axis(s->player_index, (CF_JoypadAxis)s->analog_y_axis));
		float mag = sqrtf(rx * rx + ry * ry);
		float dz = s->circular_deadzone;
		if (mag < dz) {
			CF_V2 result = { 0, 0 };
			return result;
		}
		float remapped_mag = (mag - dz) / (1.0f - dz);
		float scale = remapped_mag / mag;
		CF_V2 result;
		result.x = rx * scale;
		result.y = ry * scale;
		return result;
	}

	CF_V2 result;
	result.x = cf_axis_binding_value(s->x_axis);
	result.y = cf_axis_binding_value(s->y_axis);
	return result;
}

CF_V2 cf_stick_binding_sign(CF_StickBinding handle)
{
	CF_V2 value = cf_stick_binding_value(handle);
	CF_V2 result;
	result.x = (value.x == 0) ? 0 : (value.x > 0 ? 1.0f : -1.0f);
	result.y = (value.y == 0) ? 0 : (value.y > 0 ? 1.0f : -1.0f);
	return result;
}

bool cf_stick_binding_consume_press(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	bool a0 = cf_axis_binding_consume_press(s->x_axis);
	bool a1 = cf_axis_binding_consume_press(s->y_axis);
	return a0 | a1;
}

bool cf_stick_binding_consume_release(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	bool a0 = cf_axis_binding_consume_release(s->x_axis);
	bool a1 = cf_axis_binding_consume_release(s->y_axis);
	return a0 | a1;
}

void cf_stick_binding_set_circular_deadzone(CF_StickBinding handle, float deadzone)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	s->circular_deadzone = deadzone;
	// Set both underlying axis deadzones to 0 so analog values pass through raw-normalized.
	cf_axis_binding_set_deadzone(s->x_axis, 0);
	cf_axis_binding_set_deadzone(s->y_axis, 0);
}

CF_V2 cf_stick_binding_value_raw(CF_StickBinding handle)
{
	CF_StickBindingInternal* s = (CF_StickBindingInternal*)handle.id;
	CF_V2 result;
	result.x = cf_axis_binding_value_raw(s->x_axis);
	result.y = cf_axis_binding_value_raw(s->y_axis);
	return result;
}

//--------------------------------------------------------------------------------------------------
// Joypad connection tracking.

void cf_register_joypad_connect_callback(void (*fn)(int player_index, bool connected, void* udata), void* udata)
{
	s_binding_init();
	CF_JoypadConnectCallback callback;
	callback.fn = fn;
	callback.udata = udata;
	apush(s_on_joypad_connects, callback);
}

//--------------------------------------------------------------------------------------------------
// Deadzone.

float cf_binding_get_deadzone()
{
	return s_binding_deadzone;
}

void cf_binding_set_deadzone(float deadzone)
{
	s_binding_deadzone = deadzone;
}

//--------------------------------------------------------------------------------------------------
// Per-frame update (called from cf_app_update).

void cf_binding_update()
{
	if (!s_binding_initialized) return;

	// Handle joypad connect/disconnect.
	for (int i = 0; i < asize(s_joypads); ++i) {
		CF_JoypadInfo* joypad = s_joypads + i;
		CF_JoypadInfo refresh = s_get_joypad_info(i);
		if (refresh.connected) {
			if (!joypad->connected) {
				*joypad = refresh;
				for (int j = 0; j < asize(s_on_joypad_connects); ++j) {
					s_on_joypad_connects[j].fn(i, true, s_on_joypad_connects[j].udata);
				}
			}
		} else {
			if (joypad->connected) {
				for (int j = 0; j < asize(s_on_joypad_connects); ++j) {
					s_on_joypad_connects[j].fn(i, false, s_on_joypad_connects[j].udata);
				}
				joypad->connected = false;
			}
		}
	}

	// Update all button bindings.
	for (int i = 0; i < asize(s_button_bindings); ++i) {
		s_button_binding_update(s_button_bindings[i]);
	}
}
