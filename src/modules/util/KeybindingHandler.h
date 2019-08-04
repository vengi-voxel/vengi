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

/**
 * @param bindings Map of bindings
 * @param key The key that was pressed
 * @param modifier The modifier mask
 * @param skipModifier The modifier mask that should be skipped while checking for bindings. Might e.g. be the mask of already pressed modifiers
 * that were bound as keys. Ignoring those here allows us to find all matches of valid command bindings.
 * @return @c true if the key+modifier combination lead to a command execution via
 * key bindings, @c false otherwise
 */
extern bool executeCommandsForBinding(const util::BindMap& bindings, int32_t key, int16_t modifier, int16_t skipModifier, uint64_t now = 0ul);

extern bool isValidForBinding(int16_t pressedModMask, const std::string& command, int16_t commandModMask);

}
