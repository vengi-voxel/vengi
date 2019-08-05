/**
 * @file
 */

#pragma once

#include <SDL.h>
#include <string>

namespace util {
namespace button {

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

static const std::string LEFT_MOUSE_BUTTON = "left_mouse";
static const std::string RIGHT_MOUSE_BUTTON = "right_mouse";
static const std::string MIDDLE_MOUSE_BUTTON = "middle_mouse";
static const std::string X1_MOUSE_BUTTON = "x1_mouse";
static const std::string X2_MOUSE_BUTTON = "x2_mouse";
static const std::string MOUSE_WHEEL_UP = "wheelup";
static const std::string MOUSE_WHEEL_DOWN = "wheeldown";

}
}
