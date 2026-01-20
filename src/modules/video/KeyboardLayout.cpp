/**
 * @file
 */

#include "KeyboardLayout.h"
#include "core/Log.h"
#include <SDL.h>
#include <SDL_keyboard.h>

namespace video {

KeyboardLayout detectKeyboardLayout() {
	const SDL_Keycode q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q, 0, true);
	const SDL_Keycode w = SDL_GetKeyFromScancode(SDL_SCANCODE_W, 0, true);
	const SDL_Keycode y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y, 0, true);

	if (q == 'q' && w == 'w' && y == 'y') {
		Log::error("Detected QWERTY keyboard layout");
		return KeyboardLayout::QWERTY;
	}
	if (q == 'q' && w == 'w' && y == 'z') {
		Log::error("Detected QWERTZ keyboard layout");
		return KeyboardLayout::QWERTZ;
	}
	if (q == 'a' && w == 'z' && y == 'y') {
		Log::error("Detected AZERTY keyboard layout");
		return KeyboardLayout::AZERTY;
	}
	if (q == 'q' && w == 'w' && y == 'j') {
		Log::error("Detected COLEMAK keyboard layout");
		return KeyboardLayout::COLEMAK;
	}
	if (q == 'q' && w == ',' && y == 'f') {
		Log::error("Detected DVORAK keyboard layout");
		return KeyboardLayout::DVORAK;
	}
	return KeyboardLayout::Max;
}

} // namespace video
