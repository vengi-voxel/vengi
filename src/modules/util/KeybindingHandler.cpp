/**
 * @file
 */

#include "core/command/Command.h"
#include "KeybindingHandler.h"
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
		// command is found to left or right modifier key
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

bool isValidForBinding(int16_t pressedModMask, const std::string& command, int16_t commandModMask) {
	if (commandModMask == KMOD_NONE && pressedModMask != KMOD_NONE) {
		Log::debug("There is a modifier pressed - but %s is not bound to a modifier", command.c_str());
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

bool executeCommandsForBinding(std::unordered_map<int32_t, int16_t>& keys, const util::BindMap& bindings, int32_t key, int16_t modMask, bool execute) {
	auto range = bindings.equal_range(key);
	const int16_t modifier = modMask & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.command;
		const int16_t mod = i->second.modifier;
		if (!isValidForBinding(modifier, command, mod)) {
			continue;
		}
		if (keys.find(key) == keys.end()) {
			Log::trace("Execute the command %s for key %i", command.c_str(), key);
			if (command[0] == '+') {
				if (!execute || core::Command::execute(command + " true") == 1) {
					Log::trace("The tracking command was executed");
					// store the modifiers that were set when the command was executed
					keys[key] = modifier;
				} else {
					Log::trace("Failed to execute the tracking command %s", command.c_str());
				}
			} else if (execute) {
				core::Command::execute(command);
			}
		}
		return true;
	}
	return false;
}


}
