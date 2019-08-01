/**
 * @file
 */

#pragma once

#include "core/Tokenizer.h"
#include <SDL.h>
#include <unordered_map>

#define CUSTOM_SDL_KEYCODE(X) SDL_SCANCODE_TO_KEYCODE((util::CUSTOM_SCANCODES + (X)))

namespace util {

static const uint32_t CUSTOM_SCANCODES           = SDL_NUM_SCANCODES + 1;
static const uint32_t CUSTOM_SDLK_MOUSE_LEFT     = CUSTOM_SDL_KEYCODE(SDL_BUTTON_LEFT);
static const uint32_t CUSTOM_SDLK_MOUSE_MIDDLE   = CUSTOM_SDL_KEYCODE(SDL_BUTTON_MIDDLE);
static const uint32_t CUSTOM_SDLK_MOUSE_RIGHT    = CUSTOM_SDL_KEYCODE(SDL_BUTTON_RIGHT);


struct CommandModifierPair {
	inline CommandModifierPair(const std::string& _command, int16_t _modifier) :
			command(_command), modifier(_modifier) {
	}
	std::string command;
	int16_t modifier;
};
typedef std::unordered_multimap<int32_t, CommandModifierPair> BindMap;

class KeybindingParser: public core::Tokenizer {
private:
	BindMap _bindings;
	int _invalidBindings;

	void parseKeyAndCommand(std::string key, const std::string& command);
public:
	KeybindingParser(const std::string& key, const std::string& binding);

	KeybindingParser(const std::string& bindings);

	inline int invalidBindings() const {
		return _invalidBindings;
	}

	inline const BindMap& getBindings() const {
		return _bindings;
	}
};

}
