/**
 * @file
 */

#pragma once

#include "KeybindingParser.h"
#include <unordered_map>
#include <cstdint>

namespace util {

/**
 * @param keys Map of pressed keys
 * @param bindings Map of bindings
 * @param key The key that was pressed
 * @param modififer The modifier mask
 * @return @c true if the key+modifier combination lead to a command execution via
 * key bindings, @c false otherwise
 */
extern bool executeCommandsForBinding(std::unordered_map<int32_t, int16_t> keys, util::BindMap& bindings, int32_t key, int16_t modifier);

}
