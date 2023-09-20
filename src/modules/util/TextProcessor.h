/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

class KeyBindingHandler;

bool replacePlaceholders(const KeyBindingHandler &handler, const core::String &str, char *buf, size_t bufSize);

}
