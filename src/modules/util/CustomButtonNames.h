/**
 * @file
 */

#pragma once

#include <SDL.h>
#include "core/String.h"

#define CUSTOM_SDL_KEYCODE(X) SDL_SCANCODE_TO_KEYCODE((util::button::CUSTOM_SCANCODES + (X)))

namespace util {
namespace button {

#define CUSTOM_SDL_BUTTON_OFFSET (SDL_BUTTON_X2 + 10)
static const int32_t CUSTOM_SCANCODES               = SDL_NUM_SCANCODES + 1;
static const int32_t CUSTOM_SDLK_MOUSE_LEFT         = CUSTOM_SDL_KEYCODE(SDL_BUTTON_LEFT);
static const int32_t CUSTOM_SDLK_MOUSE_MIDDLE       = CUSTOM_SDL_KEYCODE(SDL_BUTTON_MIDDLE);
static const int32_t CUSTOM_SDLK_MOUSE_RIGHT        = CUSTOM_SDL_KEYCODE(SDL_BUTTON_RIGHT);
static const int32_t CUSTOM_SDLK_MOUSE_X1           = CUSTOM_SDL_KEYCODE(SDL_BUTTON_X1);
static const int32_t CUSTOM_SDLK_MOUSE_X2           = CUSTOM_SDL_KEYCODE(SDL_BUTTON_X2);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_UP     = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 0);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_DOWN   = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 1);
#undef CUSTOM_SDL_BUTTON_OFFSET

static const struct CustomButtonMapping {
	int32_t key;
	core::String name;
	uint16_t count;
} CUSTOMBUTTONMAPPING[] = {
	{CUSTOM_SDLK_MOUSE_LEFT, "left_mouse", 1u},
	{CUSTOM_SDLK_MOUSE_MIDDLE, "middle_mouse", 1u},
	{CUSTOM_SDLK_MOUSE_RIGHT, "right_mouse", 1u},
	{CUSTOM_SDLK_MOUSE_LEFT, "double_left_mouse", 2u},
	{CUSTOM_SDLK_MOUSE_MIDDLE, "double_middle_mouse", 2u},
	{CUSTOM_SDLK_MOUSE_RIGHT, "double_right_mouse", 2u},
	{CUSTOM_SDLK_MOUSE_X1, "x1_mouse", 1u},
	{CUSTOM_SDLK_MOUSE_X2, "x2_mouse", 1u},
	{CUSTOM_SDLK_MOUSE_WHEEL_UP, "wheelup", 1u},
	{CUSTOM_SDLK_MOUSE_WHEEL_DOWN, "wheeldown", 1u},
};

// note: doesn't contain all combinations
static constexpr struct ModifierMapping {
	int16_t modifier;
	const char *name;
} MODIFIERMAPPING[] = {
	{KMOD_LSHIFT, "left_shift"},
	{KMOD_RSHIFT, "right_shift"},
	{KMOD_LCTRL, "left_ctrl"},
	{KMOD_RCTRL, "right_ctrl"},
	{KMOD_LALT, "left_alt"},
	{KMOD_RALT, "right_alt"},
	{KMOD_ALT, "alt"},
	{KMOD_SHIFT, "shift"},
	{KMOD_CTRL, "ctrl"},
	{KMOD_ALT | KMOD_SHIFT, "alt+shift"},
	{KMOD_CTRL | KMOD_SHIFT, "ctrl+shift"},
	{KMOD_ALT | KMOD_CTRL, "alt+ctrl"},
	{KMOD_ALT | KMOD_SHIFT | KMOD_SHIFT, "ctrl+alt+shift"},
	{0, nullptr}
};

}
}
