#pragma once

#include "core/Tokenizer.h"
#include <SDL.h>
#include <unordered_map>

namespace ui {

typedef std::unordered_multimap<int32_t, std::pair<std::string, int16_t> > BindMap;

class KeybindingParser: public core::Tokenizer {
private:
	BindMap _bindings;
	int _invalidBindings;
public:
	KeybindingParser(const std::string& bindings) :
			core::Tokenizer(bindings), _invalidBindings(0) {
		for (;;) {
			if (!hasNext())
				break;
			std::string key = next();
			if (!hasNext())
				break;
			const std::string& command = next();

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
				++_invalidBindings;
				continue;
			}
			_bindings.insert(std::make_pair(keyCode, std::make_pair(command, modifier)));
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
