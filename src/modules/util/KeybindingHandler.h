/**
 * @file
 */

#pragma once

#include "KeybindingParser.h"
#include <unordered_map>
#include <stdint.h>

namespace util {

/**
 * @param bindings Map of bindings
 * @param key The key that was pressed
 * @param modifier The modifier mask
 * @return @c true if the key+modifier combination lead to a command execution via
 * key bindings, @c false otherwise
 */
extern bool executeCommandsForBinding(const util::BindMap& bindings, int32_t key, int16_t modifier, uint64_t now = 0ul);

extern bool isValidForBinding(int16_t pressedModMask, const std::string& command, int16_t commandModMask);

}
