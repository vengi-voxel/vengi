/**
 * @file
 */

#include "KeybindingParser.h"

namespace util {

void KeybindingParser::parseKeyAndCommand(std::string key, const std::string& command) {
	int modifier = KMOD_NONE;
	if (key.size() > 1) {
		if (core::string::contains(key, "+")) {
			std::vector<std::string> line;
			core::string::splitString(key, line, "+");
			for (const std::string& token : line) {
				const std::string& lower = core::string::toLower(token);
				if (lower == "shift") {
					modifier |= KMOD_SHIFT;
				} else if (lower == "left_shift") {
					modifier |= KMOD_LSHIFT;
				} else if (lower == "right_shift") {
					modifier |= KMOD_RSHIFT;
				} else if (lower == "alt") {
					modifier |= KMOD_ALT;
				} else if (lower == "left_alt") {
					modifier |= KMOD_LALT;
				} else if (lower == "right_alt") {
					modifier |= KMOD_RALT;
				} else if (lower == "ctrl") {
					modifier |= KMOD_CTRL;
				} else if (lower == "left_ctrl") {
					modifier |= KMOD_LCTRL;
				} else if (lower == "right_ctrl") {
					modifier |= KMOD_RCTRL;
				} else {
					key = token;
				}
			}
		}
	}

	SDL_Keycode keyCode;
	key = core::string::replaceAll(key, "_", " ");
	if (key == "lmb") {
		keyCode = CUSTOM_SDLK_MOUSE_LEFT;
	} else if (key == "rmb") {
		keyCode = CUSTOM_SDLK_MOUSE_RIGHT;
	} else if (key == "mmb") {
		keyCode = CUSTOM_SDLK_MOUSE_MIDDLE;
	} else if (key == "wheelup") {
		keyCode = CUSTOM_SDLK_MOUSE_WHEEL_UP;
	} else if (key == "wheeldown") {
		keyCode = CUSTOM_SDLK_MOUSE_WHEEL_DOWN;
	} else {
		keyCode = SDL_GetKeyFromName(key.c_str());
	}
	if (keyCode == SDLK_UNKNOWN) {
		Log::warn("could not get a valid key code for %s (skip binding for %s)", key.c_str(), command.c_str());
		++_invalidBindings;
		return;
	}
	_bindings.insert(std::make_pair(keyCode, CommandModifierPair(command, modifier)));
}

KeybindingParser::KeybindingParser(const std::string& key, const std::string& binding) :
		core::Tokenizer(""), _invalidBindings(0) {
	parseKeyAndCommand(key, binding);
}

KeybindingParser::KeybindingParser(const std::string& bindings) :
		core::Tokenizer(bindings), _invalidBindings(0) {
	for (;;) {
		if (!hasNext()) {
			break;
		}
		std::string key = next();
		if (!hasNext()) {
			break;
		}
		const std::string command = next();
		parseKeyAndCommand(key, command);
	}
}

}
