/**
 * @file
 */

#pragma once

#include "core/Tokenizer.h"
#include <SDL.h>
#include <unordered_map>

namespace core {

typedef std::pair<std::string, int16_t> CommandModifierPair;
typedef std::unordered_multimap<int32_t, CommandModifierPair> BindMap;

class KeybindingParser: public core::Tokenizer {
private:
	BindMap _bindings;
	int _invalidBindings;

	void parseKeyAndCommand(std::string key, const std::string& command) {
		int modifier = KMOD_NONE;
		if (key.size() > 1) {
			if (core::string::contains(key, "+")) {
				std::vector<std::string> line;
				core::string::splitString(key, line, "+");
				for (const std::string& token : line) {
					const std::string& lower = core::string::toLower(token);
					if (lower == "shift") {
						modifier |= KMOD_SHIFT;
					} else if (lower == "alt") {
						modifier |= KMOD_ALT;
					} else if (lower == "ctrl") {
						modifier |= KMOD_CTRL;
					} else {
						key = token;
					}
				}
			}
		}

		const SDL_Keycode keyCode = SDL_GetKeyFromName(key.c_str());
		if (keyCode == SDLK_UNKNOWN) {
			Log::warn("could not get a valid key code for %s (skip binding for %s)", key.c_str(), command.c_str());
			++_invalidBindings;
			return;
		}
		_bindings.insert(std::make_pair(keyCode, std::make_pair(command, modifier)));
	}

public:
	KeybindingParser(const std::string& key, const std::string& binding) :
			core::Tokenizer(""), _invalidBindings(0) {
		parseKeyAndCommand(key, binding);
	}

	KeybindingParser(const std::string& bindings) :
			core::Tokenizer(bindings), _invalidBindings(0) {
		for (;;) {
			if (!hasNext())
				break;
			std::string key = next();
			if (!hasNext())
				break;
			const std::string command = next();
			parseKeyAndCommand(key, command);
		}
	}

	inline int invalidBindings() const {
		return _invalidBindings;
	}

	inline const BindMap& getBindings() const {
		return _bindings;
	}
};

}
