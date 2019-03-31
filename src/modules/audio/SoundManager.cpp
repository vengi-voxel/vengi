/**
 * @file
 */

#include "SoundManager.h"
#include "core/command/Command.h"
#include "core/Var.h"
#include <SDL.h>

namespace audio {

bool SoundManager::init() {
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
		return false;
	}
	return true;
}

void SoundManager::construct() {
}

void SoundManager::shutdown() {
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

}
