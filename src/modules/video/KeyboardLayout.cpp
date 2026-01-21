/**
 * @file
 */

#include "KeyboardLayout.h"
#include "core/Log.h"
#include <SDL.h>
#include <SDL_keyboard.h>

namespace video {

KeyboardLayout detectKeyboardLayout() {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const SDL_Keycode q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q, 0, true);
	const SDL_Keycode w = SDL_GetKeyFromScancode(SDL_SCANCODE_W, 0, true);
	const SDL_Keycode y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y, 0, true);
#else
	const SDL_Keycode q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q);
	const SDL_Keycode w = SDL_GetKeyFromScancode(SDL_SCANCODE_W);
	const SDL_Keycode y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y);
#endif

	if (q == 'q' && w == 'w' && y == 'y') {
		Log::debug("Detected QWERTY keyboard layout");
		return KeyboardLayout::QWERTY;
	}
	if (q == 'q' && w == 'w' && y == 'z') {
		Log::debug("Detected QWERTZ keyboard layout");
		return KeyboardLayout::QWERTZ;
	}
	if (q == 'a' && w == 'z' && y == 'y') {
		Log::debug("Detected AZERTY keyboard layout");
		return KeyboardLayout::AZERTY;
	}
	if (q == 'q' && w == 'w' && y == 'j') {
		Log::debug("Detected COLEMAK keyboard layout");
		return KeyboardLayout::COLEMAK;
	}
	if (q == 'q' && w == ',' && y == 'f') {
		Log::debug("Detected DVORAK keyboard layout");
		return KeyboardLayout::DVORAK;
	}
	return KeyboardLayout::Max;
}

} // namespace video
