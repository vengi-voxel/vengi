/**
 * @file
 */

#include "core/command/Command.h"
#include "KeybindingHandler.h"
#include "core/Array.h"
#include "core/String.h"
#include <SDL.h>

namespace util {

static inline bool checkModifierBitMask(int16_t mask, int16_t pressedModMask, int16_t commandModMask) {
	// extract that stuff we are interested in
	const int16_t command = commandModMask & mask;
	const int16_t pressed = pressedModMask & mask;
	if (pressed == mask) {
		// both of them ... erm.. no
		return false;
	}
	if (command == mask) {
		// command is bound to left or right modifier key
		if (!(pressed & mask)) {
			// but the modifiers aren't pressed
			return false;
		}
	} else if (command != pressed) {
		// only one of the modifiers is bound - so values must be equal in order to match
		return false;
	}
	// looks like the values are matching up
	return true;
}

bool isValidForBinding(int16_t pressedModMask, int16_t commandModMask) {
	if (commandModMask == KMOD_NONE && pressedModMask != KMOD_NONE) {
		return false;
	}
	if (commandModMask != KMOD_NONE) {
		if (!checkModifierBitMask(KMOD_SHIFT, pressedModMask, commandModMask)) {
			return false;
		}
		if (!checkModifierBitMask(KMOD_ALT, pressedModMask, commandModMask)) {
			return false;
		}
		if (!checkModifierBitMask(KMOD_CTRL, pressedModMask, commandModMask)) {
			return false;
		}
	}
	return true;
}

bool executeCommandsForBinding(const BindMap& bindings, int32_t key, int16_t modMask, uint64_t now) {
	auto range = bindings.equal_range(key);
	const int16_t modifier = modMask & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.command;
		const int16_t mod = i->second.modifier;
		if (!isValidForBinding(modifier, mod)) {
			continue;
		}
		Log::trace("Execute the command %s for key %i", command.c_str(), key);
		if (command[0] == '+') {
			if (core::Command::execute("%s %i %" PRId64, command.c_str(), key, now) == 1) {
				Log::trace("The tracking command was executed");
			} else {
				Log::trace("Failed to execute the tracking command %s", command.c_str());
			}
		} else {
			core::Command::execute(command);
		}
		return true;
	}
	return false;
}

bool KeyBindingHandler::executeCommands(int32_t key, int16_t modifier, uint64_t now) {
	// first try to find an exact match of key and current held modifiers
	if (executeCommandsForBinding(_bindings, key, modifier, now)) {
		return true;
	}
	// if no such exact match was found, try to remove those modifiers that should be ignored because they e.g. have their own bound command
	// this might happen if you bound a command to e.g. shift. Having shift pressed while pressing another key combination like ctrl+w would
	// not match if that special bound key shift wouldn't get removed from the mask to check.
	if (_pressedModifierMask != 0u && executeCommandsForBinding(_bindings, key, (int16_t)((uint32_t)modifier ^ _pressedModifierMask), now)) {
		return true;
	}
	// at last try to find a key that was bound without any modifier.
	if (executeCommandsForBinding(_bindings, key, 0, now)) {
		return true;
	}
	return false;
}

void KeyBindingHandler::construct() {
	core::Command::registerCommand("bindlist", [this] (const core::CmdArgs& args) {
		for (BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
			const CommandModifierPair& pair = i->second;
			const std::string& command = pair.command;
			const std::string& keyBinding = getKeyBindingsString(command.c_str());
			Log::info("%-25s %s", keyBinding.c_str(), command.c_str());
		}
	}).setHelp("Show all known key bindings");

	core::Command::registerCommand("bind", [this] (const core::CmdArgs& args) {
		if (args.size() != 2) {
			Log::error("Expected parameters: key+modifier command - got %i parameters", (int)args.size());
			return;
		}

		KeybindingParser p(args[0], args[1]);
		const BindMap& bindings = p.getBindings();
		for (BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
			const uint32_t key = i->first;
			const CommandModifierPair& pair = i->second;
			auto range = _bindings.equal_range(key);
			bool found = false;
			for (auto it = range.first; it != range.second; ++it) {
				if (it->second.modifier == pair.modifier) {
					it->second.command = pair.command;
					found = true;
					Log::info("Updated binding for key %s", args[0].c_str());
					break;
				}
			}
			if (!found) {
				_bindings.insert(std::make_pair(key, pair));
				Log::info("Added binding for key %s", args[0].c_str());
			}
		}
	}).setHelp("Bind a command to a key");
}

void KeyBindingHandler::shutdown() {
	std::string keybindings;

	for (BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
		const int32_t key = i->first;
		const CommandModifierPair& pair = i->second;
		const std::string keyName = core::string::toLower(SDL_GetKeyName(key));
		const int16_t modifier = pair.modifier;
		const char *modifierMaskString = getModifierName(modifier);
		std::string modifierKey;
		if (modifierMaskString != nullptr) {
			modifierKey.append(modifierMaskString);
			modifierKey.append("+");
		}
		const std::string& command = pair.command;
		keybindings += core::string::toLower(modifierKey) + keyName + " " + command + '\n';
	}
	Log::trace("%s", keybindings.c_str());
	core::App::getInstance()->filesystem()->write("keybindings.cfg", keybindings);
}

bool KeyBindingHandler::init() {
	return true;
}

bool KeyBindingHandler::load(const std::string& filename) {
	const std::string& bindings = core::App::getInstance()->filesystem()->load(filename);
	if (bindings.empty()) {
		return false;
	}
	Log::info("Load key bindings from %s", filename.c_str());
	const KeybindingParser p(bindings);
	_bindings.insert(p.getBindings().begin(), p.getBindings().end());
	return true;
}

std::string KeyBindingHandler::getKeyBindingsString(const char *cmd) const {
	int16_t modifier;
	int32_t key;
	if (!resolveKeyBindings(cmd, &modifier, &key)) {
		return "";
	}
	const char *name = getKeyName(key);
	if (modifier <= 0) {
		return name;
	}
	const char *modifierName = getModifierName(modifier);
	return core::string::format("%s+%s", modifierName, name);
}

bool KeyBindingHandler::resolveKeyBindings(const char *cmd, int16_t* modifier, int32_t* key) const {
	const char *match = strchr(cmd, ' ');
	const size_t size = match != nullptr ? (size_t)(intptr_t)(match - cmd) : strlen(cmd);
	for (const auto& b : _bindings) {
		const CommandModifierPair& pair = b.second;
		if (!strcmp(pair.command.c_str(), cmd) || !strncmp(pair.command.c_str(), cmd, size)) {
			if (modifier != nullptr) {
				*modifier = pair.modifier;
			}
			if (key != nullptr) {
				*key = b.first;
			}
			return true;
		}
	}
	return false;
}

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

const char* KeyBindingHandler::getKeyName(uint32_t key) {
	if (key == CUSTOM_SDLK_MOUSE_LEFT) {
		return "lmb";
	} else if (key == CUSTOM_SDLK_MOUSE_RIGHT) {
		return "rmb";
	} else if (key == CUSTOM_SDLK_MOUSE_MIDDLE) {
		return "mmb";
	}
	return SDL_GetKeyName((SDL_Keycode)key);
}

const char* KeyBindingHandler::getModifierName(int16_t modifier) {
	if (modifier == 0) {
		return nullptr;
	}
	for (int i = 0; i < lengthof(MODIFIERMAPPING); ++i) {
		if (MODIFIERMAPPING[i].modifier == modifier) {
			return MODIFIERMAPPING[i].name;
		}
	}
	return "<unknown>";
}

bool KeyBindingHandler::execute(int32_t key, int16_t modifier, bool pressed, uint64_t now) {
	int16_t code = 0;
	switch (key) {
	case SDLK_LCTRL:
		code = KMOD_LCTRL;
		break;
	case SDLK_RCTRL:
		code = KMOD_RCTRL;
		break;
	case SDLK_LSHIFT:
		code = KMOD_LSHIFT;
		break;
	case SDLK_RSHIFT:
		code = KMOD_RSHIFT;
		break;
	case SDLK_LALT:
		code = KMOD_LALT;
		break;
	case SDLK_RALT:
		code = KMOD_RALT;
		break;
	}

	if (pressed) {
		_keys.insert(key);

		if (code != 0) {
			// this is the case where a binding that needs a modifier should get executed, but
			// a key was pressed in the order: "key and then modifier".
			std::unordered_set<int32_t> recheck;
			for (auto& b : _bindings) {
				const CommandModifierPair& pair = b.second;
				const int32_t commandKey = b.first;
				if (pair.command[0] != '+') {
					// no action button command
					continue;
				}
				if (pair.modifier == 0) {
					continue;
				}
				if (!isPressed(commandKey)) {
					continue;
				}
				if (!isValidForBinding(modifier, pair.modifier)) {
					continue;
				}
				core::Command::execute("%s %i %" PRId64, pair.command.c_str(), commandKey, now);
				recheck.insert(commandKey);
			}
			// for those keys that were activated because only a modifier was pressed (bound to e.g. left_shift),
			// we have to disable the old action button that was just bound to the key without the modifier.
			for (int32_t checkKey : recheck) {
				auto range = _bindings.equal_range(checkKey);
				for (auto i = range.first; i != range.second; ++i) {
					const CommandModifierPair& pair = i->second;
					if (pair.modifier != 0) {
						continue;
					}
					core::Command::execute("-%s %i %" PRId64, &(pair.command.c_str()[1]), checkKey, now);
				}
			}
		}
		const bool retVal = executeCommands(key, modifier, now);
		if (retVal) {
			_pressedModifierMask |= (uint32_t)code;
		}
		return retVal;
	}
	bool handled = false;
	if (code != 0) {
		for (auto& b : _bindings) {
			const CommandModifierPair& pair = b.second;
			const int32_t commandKey = b.first;
			if (pair.command[0] != '+') {
				// no action button command
				continue;
			}
			if (!isValidForBinding(code, pair.modifier)) {
				continue;
			}
			if (!isPressed(commandKey)) {
				continue;
			}
			core::Command::execute("-%s %i %" PRId64, &(pair.command.c_str()[1]), commandKey, now);
			executeCommands(commandKey, modifier, now);
		}
		_pressedModifierMask &= ~(uint32_t)code;
	}
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.command;
		if (command[0] == '+') {
			core::Command::execute("-%s %i %" PRId64, &(command.c_str()[1]), key, now);
			handled = true;
		}
	}
	_keys.erase(key);
	return handled;
}

}
