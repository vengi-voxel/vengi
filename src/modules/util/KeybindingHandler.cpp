/**
 * @file
 */

#include "core/Command.h"
#include "KeybindingHandler.h"
#include <SDL.h>

namespace util {

bool executeCommandsForBinding(std::unordered_map<int32_t, int16_t> keys, util::BindMap& bindings, int32_t key, int16_t modifier) {
	auto range = bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.first;
		const int16_t mod = i->second.second;
		if (mod == KMOD_NONE && modifier != 0 && modifier != KMOD_NUM) {
			continue;
		}
		if (mod != KMOD_NONE) {
			if (!(modifier & mod)) {
				continue;
			}
			const uint16_t flipped = (modifier & ~mod);
			if ((flipped & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) != 0) {
				continue;
			}
		}
		if (keys.find(key) == keys.end()) {
			if (command[0] == '+') {
				if (core::Command::execute(command + " true") == 1) {
					keys[key] = modifier;
				}
			} else {
				core::Command::execute(command);
			}
		}
		return true;
	}
	return false;
}


}
