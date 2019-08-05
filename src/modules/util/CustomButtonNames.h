/**
 * @file
 */

#pragma once

#include <SDL.h>

namespace util {
namespace button {

// note: doesn't contain all combinations
static constexpr struct ModifierMapping {
	int16_t modifier;
	const char *name;
} MODIFIERMAPPING[] = {
	{KMOD_LSHIFT, "LEFT_SHIFT"},
	{KMOD_RSHIFT, "RIGHT_SHIFT"},
	{KMOD_LCTRL, "LEFT_CTRL"},
	{KMOD_RCTRL, "RIGHT_CTRL"},
	{KMOD_LALT, "LEFT_ALT"},
	{KMOD_RALT, "RIGHT_ALT"},
	{KMOD_ALT, "ALT"},
	{KMOD_SHIFT, "SHIFT"},
	{KMOD_CTRL, "CTRL"},
	{KMOD_ALT | KMOD_SHIFT, "ALT+SHIFT"},
	{KMOD_CTRL | KMOD_SHIFT, "CTRL+SHIFT"},
	{KMOD_ALT | KMOD_CTRL, "ALT+CTRL"},
	{KMOD_ALT | KMOD_SHIFT | KMOD_SHIFT, "CTRL+ALT+SHIFT"},
	{0, nullptr}
};

static const char* LEFT_MOUSE_BUTTON = "lmb";
static const char* RIGHT_MOUSE_BUTTON = "rmb";
static const char* MIDDLE_MOUSE_BUTTON = "mmb";
static const char* MOUSE_WHEEL_UP = "wheelup";
static const char* MOUSE_WHEEL_DOWN = "wheeldown";

}
}
