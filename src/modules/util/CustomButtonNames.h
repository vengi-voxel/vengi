/**
 * @file
 */

#pragma once

#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_version.h>
#include "core/String.h"

#define CUSTOM_SDL_KEYCODE(X) SDL_SCANCODE_TO_KEYCODE((util::button::CUSTOM_SCANCODES + (X)))

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_NUM_SCANCODES SDL_SCANCODE_COUNT
#define KMOD_LSHIFT SDL_KMOD_LSHIFT
#define KMOD_RSHIFT SDL_KMOD_RSHIFT
#define KMOD_GUI SDL_KMOD_GUI
#define KMOD_LGUI SDL_KMOD_LGUI
#define KMOD_RGUI SDL_KMOD_RGUI
#define KMOD_LALT SDL_KMOD_LALT
#define KMOD_RALT SDL_KMOD_RALT
#define KMOD_ALT SDL_KMOD_ALT
#define KMOD_SHIFT SDL_KMOD_SHIFT
#define KMOD_CTRL SDL_KMOD_CTRL
#define KMOD_LCTRL SDL_KMOD_LCTRL
#define KMOD_RCTRL SDL_KMOD_RCTRL
#define KMOD_NONE SDL_KMOD_NONE
#endif

namespace util {
namespace button {

#ifdef __APPLE__
#define KMOD_CONTROL KMOD_GUI
#define KMOD_LCONTROL KMOD_LGUI
#define KMOD_RCONTROL KMOD_RGUI
#else
#define KMOD_CONTROL KMOD_CTRL
#define KMOD_LCONTROL KMOD_LCTRL
#define KMOD_RCONTROL KMOD_RCTRL
#endif

#define CUSTOM_SDL_BUTTON_OFFSET (SDL_BUTTON_X2 + 10)
static const int32_t CUSTOM_SCANCODES               = SDL_NUM_SCANCODES + 1;
static const int32_t CUSTOM_SDLK_MOUSE_LEFT         = CUSTOM_SDL_KEYCODE(SDL_BUTTON_LEFT);
static const int32_t CUSTOM_SDLK_MOUSE_MIDDLE       = CUSTOM_SDL_KEYCODE(SDL_BUTTON_MIDDLE);
static const int32_t CUSTOM_SDLK_MOUSE_RIGHT        = CUSTOM_SDL_KEYCODE(SDL_BUTTON_RIGHT);
static const int32_t CUSTOM_SDLK_MOUSE_X1           = CUSTOM_SDL_KEYCODE(SDL_BUTTON_X1);
static const int32_t CUSTOM_SDLK_MOUSE_X2           = CUSTOM_SDL_KEYCODE(SDL_BUTTON_X2);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_UP     = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 1);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_DOWN   = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 2);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_LEFT   = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 3);
static const int32_t CUSTOM_SDLK_MOUSE_WHEEL_RIGHT  = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 4);

static const int32_t CUSTOM_SDLK_PEN_TIP            = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 5);
static const int32_t CUSTOM_SDLK_PEN_ERASER         = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 6);
static const int32_t CUSTOM_SDLK_PEN_BUTTON0        = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 7);
static const int32_t CUSTOM_SDLK_PEN_BUTTON1        = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 8);
static const int32_t CUSTOM_SDLK_PEN_BUTTON2        = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 9);
static const int32_t CUSTOM_SDLK_PEN_BUTTON3        = CUSTOM_SDL_KEYCODE(CUSTOM_SDL_BUTTON_OFFSET + 10);
#undef CUSTOM_SDL_BUTTON_OFFSET

static const struct CustomButtonMapping {
	int32_t key;
	const char *name;
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
	{CUSTOM_SDLK_MOUSE_WHEEL_LEFT, "wheelleft", 1u},
	{CUSTOM_SDLK_MOUSE_WHEEL_RIGHT, "wheelright", 1u},
	{CUSTOM_SDLK_PEN_TIP, "pen_tip", 1u},
	{CUSTOM_SDLK_PEN_ERASER, "pen_eraser", 1u},
	{CUSTOM_SDLK_PEN_BUTTON0, "pen_button0", 1u},
	{CUSTOM_SDLK_PEN_BUTTON1, "pen_button1", 1u},
	{CUSTOM_SDLK_PEN_BUTTON2, "pen_button2", 1u},
	{CUSTOM_SDLK_PEN_BUTTON3, "pen_button3", 1u},
};

// note: doesn't contain all combinations
static constexpr struct ModifierMapping {
	int16_t modifier;
	const char *name;
} MODIFIERMAPPING[] = {
	{KMOD_LSHIFT, "left_shift"},
	{KMOD_RSHIFT, "right_shift"},
	{KMOD_LCONTROL, "left_ctrl"},
	{KMOD_RCONTROL, "right_ctrl"},
	{KMOD_LALT, "left_alt"},
	{KMOD_RALT, "right_alt"},
	{KMOD_ALT, "alt"},
	{KMOD_SHIFT, "shift"},
	{KMOD_CONTROL, "ctrl"},
	{KMOD_ALT | KMOD_SHIFT, "alt+shift"},
	{KMOD_CONTROL | KMOD_SHIFT, "ctrl+shift"},
	{KMOD_ALT | KMOD_CONTROL, "alt+ctrl"},
	{KMOD_CONTROL | KMOD_ALT | KMOD_SHIFT, "ctrl+alt+shift"},
	{0, nullptr}
};

}
}
